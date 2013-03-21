#include "global.h"

#include "StepMania.h"

//
// Rage global classes
//
#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageSoundManager.h"
#include "GameSoundManager.h"
#include "RageInput.h"
#include "RageTimer.h"
#include "RageException.h"
#include "RageMath.h"
#include "RageDisplay.h"
#include "RageThreads.h"

#include "arch/arch.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "arch/Dialog/Dialog.h"
#include <ctime>

#include "ProductInfo.h"

#include "Screen.h"
#include "CodeDetector.h"
#include "CommonMetrics.h"
#include "Game.h"

//
// StepMania global classes
//
#include "ThemeManager.h"
#include "NoteSkinManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "GameState.h"
#include "AnnouncerManager.h"
#include "ProfileManager.h"
#include "ScreenManager.h"
#include "GameManager.h"
#include "FontManager.h"
#include "InputFilter.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "SongCacheIndex.h"
#include "BannerCache.h"
#include "UnlockSystem.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "RageFileManager.h"
#include "Bookkeeper.h"
#include "ModelManager.h"
#include "NetworkSyncManager.h"

#define ZIPS_DIR "Packages/"

PSP_MODULE_INFO( "StepMania", PSP_MODULE_USER, 1, 0 );
PSP_MAIN_THREAD_ATTR( PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU );
PSP_HEAP_SIZE_KB( -1024 );

int g_argc = 0;
char **g_argv = NULL;

static bool g_bHasFocus = true;
static bool g_bQuitting = false;

static RageDisplay::VideoModeParams GetCurVideoModeParams()
{
	return RageDisplay::VideoModeParams(
			PREFSMAN->m_iDisplayWidth,
			PREFSMAN->m_iDisplayHeight,
			PREFSMAN->m_iDisplayColorDepth,
			PREFSMAN->m_iRefreshRate,
			PREFSMAN->m_bVsync,
			PREFSMAN->m_bInterlaced,
			PREFSMAN->m_bSmoothLines,
			PREFSMAN->m_bTrilinearFiltering,
			PREFSMAN->m_bAnisotropicFiltering,
			PREFSMAN->m_bPAL
	);
}

static void StoreActualGraphicOptions( bool initial )
{
	// find out what we actually have
	PREFSMAN->m_iDisplayColorDepth = DISPLAY->GetVideoModeParams().bpp;
	PREFSMAN->m_iRefreshRate = DISPLAY->GetVideoModeParams().rate;
	PREFSMAN->m_bVsync = DISPLAY->GetVideoModeParams().vsync;

	CString log = ssprintf("%dx%d %d color %d texture %dHz %s %s",
		PREFSMAN->m_iDisplayWidth,
		PREFSMAN->m_iDisplayHeight,
		PREFSMAN->m_iDisplayColorDepth, 
		PREFSMAN->m_iTextureColorDepth, 
		PREFSMAN->m_iRefreshRate,
		PREFSMAN->m_bVsync ? "Vsync" : "NoVsync",
		PREFSMAN->m_bSmoothLines? "AA" : "NoAA" );
	if( initial )
		LOG->Info( "%s", log.c_str() );
	else
		SCREENMAN->SystemMessage( log );
}

void ApplyGraphicOptions()
{ 
	bool bNeedReload = false;

	bNeedReload |= DISPLAY->SetVideoMode( GetCurVideoModeParams() );

	DISPLAY->ChangeCentering(
		PREFSMAN->m_iCenterImageTranslateX, 
		PREFSMAN->m_iCenterImageTranslateY,
		PREFSMAN->m_fCenterImageScaleX,
		PREFSMAN->m_fCenterImageScaleY );

	bNeedReload |= TEXTUREMAN->SetPrefs( 
		RageTextureManagerPrefs( 
			PREFSMAN->m_iTextureColorDepth, 
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete, 
			PREFSMAN->m_iMaxTextureResolution,
			PREFSMAN->m_bForceMipMaps ) );

	bNeedReload |= MODELMAN->SetPrefs( 
		ModelManagerPrefs( PREFSMAN->m_bDelayedModelDelete ) );

	if( bNeedReload )
		TEXTUREMAN->ReloadAll();

	StoreActualGraphicOptions( false );

	/* Give the input handlers a chance to re-open devices as necessary. */
	INPUTMAN->WindowReset();
}

void HandleException( const CString &error )
{
	if( g_bAutoRestart )
		HOOKS->RestartProgram();

	Dialog::Error( error ); // throw up a pretty error dialog
}
	
void ExitGame()
{
	g_bQuitting = true;
}

void ResetGame( bool ReturnToFirstScreen )
{
	ReadGamePrefsFromDisk();
	INPUTMAPPER->ReadMappingsFromDisk();

	GAMESTATE->Reset();
	
	if( !THEME->DoesThemeExist( THEME->GetCurThemeName() ) )
	{
		CString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
		if( THEME->DoesThemeExist( sGameName ) )
			THEME->SwitchThemeAndLanguage( sGameName, THEME->GetCurLanguage() );
		else
			THEME->SwitchThemeAndLanguage( "default", THEME->GetCurLanguage() );
		TEXTUREMAN->DoDelayedDelete();
	}
	SaveGamePrefsToDisk();

	if( PREFSMAN->m_bAutoMapOnJoyChange )
		INPUTMAPPER->AutoMapJoysticksForCurrentGame();

	if( ReturnToFirstScreen )
	{
		if( PREFSMAN->m_bFirstRun )
			SCREENMAN->SetNewScreen( FIRST_RUN_INITIAL_SCREEN );
		else
			SCREENMAN->SetNewScreen( INITIAL_SCREEN );

		PREFSMAN->m_bFirstRun = false;
		PREFSMAN->SaveGlobalPrefsToDisk();	// persist FirstRun setting in case we don't exit normally
	}

}

static void GameLoop();

static void CheckSettings()
{
}

#include "RageDisplay_PSP.h"

RageDisplay *CreateDisplay()
{
	return new RageDisplay_PSP( GetCurVideoModeParams() );
}


#define GAMEPREFS_INI_PATH "Data/GamePrefs.ini"
#define STATIC_INI_PATH "Data/Static.ini"

void ChangeCurrentGame( const Game* g )
{
	ASSERT( g );

	SaveGamePrefsToDisk();
	INPUTMAPPER->SaveMappingsToDisk();	// save mappings before switching the game

	GAMESTATE->m_pCurGame = g;

	ReadGamePrefsFromDisk( false );
	INPUTMAPPER->ReadMappingsFromDisk();

	/* Save the newly-selected game. */
	SaveGamePrefsToDisk();
}

void ReadGamePrefsFromDisk( bool bSwitchToLastPlayedGame )
{
	ASSERT( GAMESTATE );
	ASSERT( ANNOUNCER );
	ASSERT( THEME );

	IniFile ini;
	ini.ReadFile( GAMEPREFS_INI_PATH );	// it's OK if this fails
	ini.ReadFile( STATIC_INI_PATH );	// it's OK if this fails, too

	if( bSwitchToLastPlayedGame )
	{
		ASSERT( GAMEMAN != NULL );
		CString sGame;
		GAMESTATE->m_pCurGame = NULL;
		if( ini.GetValue("Options", "Game", sGame) )
			GAMESTATE->m_pCurGame = GAMEMAN->StringToGameType( sGame );
	}

	/* If the active game type isn't actually available, revert to the default. */
	if( GAMESTATE->m_pCurGame == NULL || !GAMEMAN->IsGameEnabled( GAMESTATE->m_pCurGame ) )
	{
		if( GAMESTATE->m_pCurGame != NULL )
			LOG->Warn( "Default note skin for \"%s\" missing, reverting to \"%s\"",
				GAMESTATE->m_pCurGame->m_szName, GAMEMAN->GetDefaultGame()->m_szName );
		GAMESTATE->m_pCurGame = GAMEMAN->GetDefaultGame();
	}

	/* If the default isn't available, our default note skin is messed up. */
	if( !GAMEMAN->IsGameEnabled( GAMESTATE->m_pCurGame ) )
		RageException::Throw( "Default note skin for \"%s\" missing", GAMESTATE->m_pCurGame->m_szName );

	CString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
	CString sAnnouncer = sGameName, sTheme = sGameName; //, sNoteSkin = sGameName;

	// if these calls fail, the three strings will keep the initial values set above.
	ini.GetValue( sGameName, "Announcer",			sAnnouncer );
	ini.GetValue( sGameName, "Theme",				sTheme );
	ini.GetValue( sGameName, "DefaultModifiers",	PREFSMAN->m_sDefaultModifiers );

	// it's OK to call these functions with names that don't exist.
	ANNOUNCER->SwitchAnnouncer( sAnnouncer );
	THEME->SwitchThemeAndLanguage( sTheme, PREFSMAN->m_sLanguage );

//	NOTESKIN->SwitchNoteSkin( sNoteSkin );
}


void SaveGamePrefsToDisk()
{
	if( !GAMESTATE )
		return;

	CString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
	IniFile ini;
	ini.ReadFile( GAMEPREFS_INI_PATH );	// it's OK if this fails

	ini.SetValue( sGameName, "Announcer",			ANNOUNCER->GetCurAnnouncerName() );
	ini.SetValue( sGameName, "Theme",				THEME->GetCurThemeName() );
	ini.SetValue( sGameName, "DefaultModifiers",	PREFSMAN->m_sDefaultModifiers );
	ini.SetValue( "Options", "Game",				sGameName );

	ini.WriteFile( GAMEPREFS_INI_PATH );
}

static void MountTreeOfZips( const CString &dir )
{
	vector<CString> dirs;
	dirs.push_back( dir );

	while( dirs.size() )
	{
		CString path = dirs.back();
		dirs.pop_back();

		if( !IsADirectory(path) )
			continue;

		vector<CString> zips;
		GetDirListing( path + "/*.zip", zips, false, true );
		GetDirListing( path + "/*.smzip", zips, false, true );

		for( unsigned i = 0; i < zips.size(); ++i )
		{
			if( !IsAFile(zips[i]) )
				continue;

			LOG->Trace( "VFS: found %s", zips[i].c_str() );
			FILEMAN->Mount( "zip", zips[i], "" );
		}

		GetDirListing( path + "/*", dirs, true, true );
	}
}

static void WriteLogHeader()
{
	LOG->Info( PRODUCT_NAME_VER );

#ifdef PSP
	pspTime cur_time;
	sceRtcGetCurrentClockLocalTime( &cur_time );

	LOG->Info( "Log starting %.4d-%.2d-%.2d %.2d:%.2d:%.2d", 
		cur_time.year, cur_time.month, cur_time.day, cur_time.hour, cur_time.minutes, cur_time.seconds );
#else
	time_t cur_time;
	time(&cur_time);
	struct tm now;
	localtime_r( &cur_time, &now );

	LOG->Info( "Log starting %.4d-%.2d-%.2d %.2d:%.2d:%.2d", 
		1900+now.tm_year, now.tm_mon+1, now.tm_mday, now.tm_hour+1, now.tm_min, now.tm_sec );
#endif
	LOG->Trace( " " );

	if( g_argc > 1 )
	{
		CString args;
		for( int i = 1; i < g_argc; ++i )
		{
			if( i>1 )
				args += " ";

			// surround all params with some marker, as they might have whitespace.
			// using [[ and ]], as they are not likely to be in the params.
			args += ssprintf( "[[%s]]", g_argv[i] );
		}
		LOG->Info( "Command line args (count=%d): %s", (g_argc - 1), args.c_str());
	}
}

static void ApplyLogPreferences()
{
	LOG->SetShowLogOutput( PREFSMAN->m_bShowLogOutput );
	LOG->SetLogToDisk( PREFSMAN->m_bLogToDisk );
	LOG->SetInfoToDisk( true );
	LOG->SetFlushing( PREFSMAN->m_bForceLogFlush );
	Checkpoints::LogCheckpoints( PREFSMAN->m_bLogCheckpoints );
}

/* Search for the commandline argument given; eg. "test" searches for
 * the option "--test".  All commandline arguments are getopt_long style:
 * --foo; short arguments (-x) are not supported.  (As commandline arguments
 * are not intended for common, general use, having short options isn't
 * needed.)  If argument is non-NULL, accept an argument. */
bool GetCommandlineArgument( const CString &option, CString *argument, int iIndex )
{
	const CString optstr = "--" + option;
	
	for( int arg = 1; arg < g_argc; ++arg )
	{
		const CString CurArgument = g_argv[arg];

		const size_t i = CurArgument.find( "=" );
		CString CurOption = CurArgument.substr(0,i);
		if( CurOption.CompareNoCase(optstr) )
			continue; /* no match */

		/* Found it. */
		if( iIndex )
		{
			--iIndex;
			continue;
		}

		if( argument )
		{
			if( i != CString::npos )
				*argument = CurArgument.substr( i+1 );
			else
				*argument = "";
		}
		
		return true;
	}

	return false;
}

int main(int argc, char* argv[])
{
	g_argc = argc;
	g_argv = argv;

	/* Set up arch hooks first.  This may set up crash handling. */
	HOOKS = MakeArchHooks();

	CString g_sErrorString = "";
#if !defined(DEBUG)
	/* Always catch RageExceptions; they should always have distinct strings. */
	try { /* RageException */
#endif

	/* Almost everything uses this to read and write files.  Load this early. */
	FILEMAN		= new RageFileManager( argv[0] );
	FILEMAN->MountInitialFilesystems();

	/* Set this up next.  Do this early, since it's needed for RageException::Throw. */
	LOG			= new RageLog;

	/* Whew--we should be able to crash safely now! */

	//
	// load preferences and mount any alternative trees.
	//
	PREFSMAN	= new PrefsManager;

	ApplyLogPreferences();

	WriteLogHeader();

	/* Set up alternative filesystem trees. */
	if( PREFSMAN->m_sAdditionalFolders != "" )
	{
		CStringArray dirs;
		split( PREFSMAN->m_sAdditionalFolders, ",", dirs, true );
		for( unsigned i=0; i < dirs.size(); i++)
			FILEMAN->Mount( "dir", dirs[i], "" );
	}
	if( PREFSMAN->m_sAdditionalSongFolders != "" )
	{
		CStringArray dirs;
		split( PREFSMAN->m_sAdditionalSongFolders, ",", dirs, true );
		for( unsigned i=0; i < dirs.size(); i++)
	        FILEMAN->Mount( "dir", dirs[i], "Songs" );
	}
	MountTreeOfZips( ZIPS_DIR );

	/* One of the above filesystems might contain files that affect preferences, eg Data/Static.ini.
	 * Re-read preferences. */
	PREFSMAN->ReadGlobalPrefsFromDisk();
	ApplyLogPreferences();

	{
		// for screenshots
		char buf[32];
		pspGetDevice( buf, sizeof(buf) );
		FILEMAN->Mount( "dir", (CString)buf + "/Picture/PSPMania/", "Screenshots" );

		int iCpuClock = PREFSMAN->m_iCpuClock;
		if( 19 <=iCpuClock && iCpuClock <= 333 )
			scePowerSetClockFrequency( iCpuClock, iCpuClock, iCpuClock/2 );
	}

	/* This needs PREFSMAN. */
	Dialog::Init();

	//
	// Create game objects
	//

	GAMESTATE	= new GameState;

	/* This requires PREFSMAN, for PREFSMAN->m_bShowLoadingWindow. */
	LoadingWindow *loading_window = MakeLoadingWindow();
	if( loading_window == NULL )
		RageException::Throw( "Couldn't open any loading windows." );

	loading_window->Paint();

	srand( time(NULL) );	// seed number generator	
	
	/* Do this early, so we have debugging output if anything else fails.  LOG and
	 * Dialog must be set up first.  It shouldn't take long, but it might take a
	 * little time; do this after the LoadingWindow is shown, since we don't want
	 * that to appear delayed. */
	HOOKS->DumpDebugInfo();

	CheckSettings();

	GAMEMAN		= new GameManager;
	THEME		= new ThemeManager;
	ANNOUNCER	= new AnnouncerManager;
	NOTESKIN	= new NoteSkinManager;

	/* Set up the theme and announcer. */
	ReadGamePrefsFromDisk();

	if( PREFSMAN->m_iSoundWriteAhead )
		LOG->Info( "Sound writeahead has been overridden to %i", PREFSMAN->m_iSoundWriteAhead );
	SOUNDMAN	= new RageSoundManager;
	SOUNDMAN->SetPrefs( PREFSMAN->m_fSoundVolume );
	SOUND		= new GameSoundManager;
	BOOKKEEPER	= new Bookkeeper;
	INPUTFILTER	= new InputFilter;
	INPUTMAPPER	= new InputMapper;
	INPUTQUEUE	= new InputQueue;
	SONGINDEX	= new SongCacheIndex;
	BANNERCACHE = new BannerCache;
	
	/* depends on SONGINDEX: */
	SONGMAN		= new SongManager;
	SONGMAN->InitAll( loading_window );		// this takes a long time
	PROFILEMAN	= new ProfileManager;
	PROFILEMAN->Init();				// must load after SONGMAN
	UNLOCKMAN	= new UnlockSystem;
    NSMAN       = new NetworkSyncManager( loading_window ); 

	delete loading_window;		// destroy this before init'ing Display
	


	DISPLAY = CreateDisplay();

	DISPLAY->ChangeCentering(
		PREFSMAN->m_iCenterImageTranslateX, 
		PREFSMAN->m_iCenterImageTranslateY,
		PREFSMAN->m_fCenterImageScaleX,
		PREFSMAN->m_fCenterImageScaleY );

	TEXTUREMAN	= new RageTextureManager;
	TEXTUREMAN->SetPrefs( 
		RageTextureManagerPrefs( 
			PREFSMAN->m_iTextureColorDepth, 
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete, 
			PREFSMAN->m_iMaxTextureResolution,
			PREFSMAN->m_bForceMipMaps ) );

	MODELMAN	= new ModelManager;
	MODELMAN->SetPrefs( 
		ModelManagerPrefs( PREFSMAN->m_bDelayedModelDelete ) );

	StoreActualGraphicOptions( true );

	SONGMAN->PreloadSongImages();

	/* This initializes objects that change the SDL event mask, and has other
	 * dependencies on the SDL video subsystem, so it must be initialized after DISPLAY. */
	INPUTMAN	= new RageInput;

	// These things depend on the TextureManager, so do them after!
	FONT		= new FontManager;
	SCREENMAN	= new ScreenManager;

	ResetGame();

	CodeDetector::RefreshCacheItems();

	/* Initialize which courses are ranking courses here. */
	SONGMAN->UpdateRankingCourses();

	/* Run the main loop. */
	GameLoop();

	PREFSMAN->SaveGlobalPrefsToDisk();
	SaveGamePrefsToDisk();

#if !defined(DEBUG)
	}
	catch( const RageException &e )
	{
		/* Gracefully shut down. */
		g_sErrorString = e.what();
	}
#endif

	SAFE_DELETE( SCREENMAN );
    SAFE_DELETE( NSMAN );
	/* Delete INPUTMAN before the other INPUTFILTER handlers, or an input
	 * driver may try to send a message to INPUTFILTER after we delete it. */
	SAFE_DELETE( INPUTMAN );
	SAFE_DELETE( INPUTQUEUE );
	SAFE_DELETE( INPUTMAPPER );
	SAFE_DELETE( INPUTFILTER );
	SAFE_DELETE( MODELMAN );
	SAFE_DELETE( PROFILEMAN );	// PROFILEMAN needs the songs still loaded
	SAFE_DELETE( UNLOCKMAN );
	SAFE_DELETE( SONGMAN );
	SAFE_DELETE( BANNERCACHE );
	SAFE_DELETE( SONGINDEX );
	SAFE_DELETE( SOUND ); /* uses GAMESTATE, PREFSMAN */
	SAFE_DELETE( PREFSMAN );
	SAFE_DELETE( GAMESTATE );
	SAFE_DELETE( GAMEMAN );
	SAFE_DELETE( NOTESKIN );
	SAFE_DELETE( THEME );
	SAFE_DELETE( ANNOUNCER );
	SAFE_DELETE( BOOKKEEPER );
	SAFE_DELETE( SOUNDMAN );
	SAFE_DELETE( FONT );
	SAFE_DELETE( TEXTUREMAN );
	SAFE_DELETE( DISPLAY );
	Dialog::Shutdown();
	SAFE_DELETE( LOG );
	SAFE_DELETE( FILEMAN );

	if( g_sErrorString != "" )
		HandleException( g_sErrorString );

	SAFE_DELETE( HOOKS );

	return 0;
}

CString SaveScreenshot( const CString &sDir, bool bSaveCompressed, int iIndex )
{
	//
	// Find a file name for the screenshot
	//
	FlushDirCache();

	vector<CString> files;
	GetDirListing( sDir + "screen*", files, false, false );
	sort( files.begin(), files.end() );

	/* Files should be of the form "screen######.xxx".  Ignore the extension; find
	 * the last file of this form, and use the next number.  This way, we don't
	 * write the same screenshot number for different formats (screen00011.bmp,
	 * screen00011.jpg), and we always increase from the end, so if screen00003.jpg
	 * is deleted, we won't fill in the hole (which makes screenshots hard to find). */
	if( iIndex == -1 ) 
	{
		iIndex = 0;

		for( int i = files.size()-1; i >= 0; --i )
		{
			static Regex re( "^screen([0-9]{5})\\....$" );
			vector<CString> matches;
			if( !re.Compare( files[i], matches ) )
				continue;

			ASSERT( matches.size() == 1 );
			iIndex = atoi( matches[0] )+1;
			break;
		}
	}

	//
	// Save the screenshot
	//
	/* If writing lossy to a memcard, use SAVE_LOSSY_LOW_QUAL, so we don't eat up
	 * lots of space with screenshots. */
	RageDisplay::GraphicsFileFormat fmt;
	if( bSaveCompressed )
		fmt = RageDisplay::SAVE_LOSSY_HIGH_QUAL;
	else
		fmt = RageDisplay::SAVE_LOSSLESS;

	CString sFileName = ssprintf( "screen%05d.%s",iIndex,bSaveCompressed ? "jpg" : "bmp" );
	CString sPath = sDir+sFileName;
	bool bResult = DISPLAY->SaveScreenshot( sPath, fmt );
	if( !bResult )
	{
		SCREENMAN->PlayInvalidSound();
		return "";
	}

	SCREENMAN->PlayScreenshotSound();

	// We wrote a new file, and SignFile won't pick it up unless we invalidate
	// the Dir cache.  There's got to be a better way of doing this than 
	// thowing out all the cache. -Chris
	FlushDirCache();

	return sFileName;
}

void InsertCoin( int iNum )
{
	GAMESTATE->m_iCoins += iNum;
	LOG->Trace("%i coins inserted, %i needed to play", GAMESTATE->m_iCoins, PREFSMAN->m_iCoinsPerCredit);
	BOOKKEEPER->CoinInserted();
	SCREENMAN->RefreshCreditsMessages();
	SCREENMAN->PlayCoinSound();
}

void InsertCredit()
{
	InsertCoin( PREFSMAN->m_iCoinsPerCredit );
}

/* Returns true if the key has been handled and should be discarded, false if
 * the key should be sent on to screens. */
bool HandleGlobalInputs( DeviceInput DeviceI, InputEventType type, GameInput GameI, MenuInput MenuI, StyleInput StyleI )
{
	/* None of the globals keys act on types other than FIRST_PRESS */
	if( type != IET_FIRST_PRESS ) 
		return false;

	switch( MenuI.button )
	{
	case MENU_BUTTON_OPERATOR:

		/* Global operator key, to get quick access to the options menu. Don't
		 * do this if we're on a "system menu", which includes the editor
		 * (to prevent quitting without storing changes). */
		if( !GAMESTATE->m_bIsOnSystemMenu )
		{
			SCREENMAN->SystemMessage( "OPERATOR" );
			GAMESTATE->Reset();
			SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		}
		return true;

	case MENU_BUTTON_COIN:
		/* Handle a coin insertion. */
		if( GAMESTATE->m_bEditing )	// no coins while editing
		{
			LOG->Trace( "Ignored coin insertion (editing)" );
			break;
		}
		InsertCoin();
		return false;	// Attract need to know because they go to TitleMenu on > 1 credit
	}

#ifndef PSP
	if(DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F2))
	{
		if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT) ) )
		{
			// HACK: Also save bookkeeping and profile info for debugging
			// so we don't have to play through a whole song to get new output.
			BOOKKEEPER->WriteToDisk();
			PROFILEMAN->SaveMachineProfile();
			FOREACH_PlayerNumber( p )
				if( PROFILEMAN->IsUsingProfile(p) )
					PROFILEMAN->SaveProfile( p );
			SCREENMAN->SystemMessage( "Stats saved" );
		}
		else
		{
			THEME->ReloadMetrics();
			TEXTUREMAN->ReloadAll();
			SCREENMAN->ReloadCreditsText();
			NOTESKIN->RefreshNoteSkinData( GAMESTATE->m_pCurGame );
			CodeDetector::RefreshCacheItems();
		

			/* If we're in screen test mode, reload the screen. */
			if( PREFSMAN->m_bScreenTestMode )
				ResetGame( true );
			else
				SCREENMAN->SystemMessage( "Reloaded metrics and textures" );
		}

		return true;
	}
#endif

	/* The default Windows message handler will capture the desktop window upon
	 * pressing PrntScrn, or will capture the foregroud with focus upon pressing
	 * Alt+PrntScrn.  Windows will do this whether or not we save a screenshot 
	 * ourself by dumping the frame buffer.  */
	/* Pressing F13 on an Apple keyboard sends KEY_PRINT.
	 * However, notebooks don't have F13. Use cmd-F12 then*/
	// "if pressing PrintScreen and not pressing Alt"
#ifndef PSP
	if( (DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PRTSC) &&
		 !INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RALT)) &&
		 !INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LALT))) ||
	    (DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F12) &&
		 (INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RMETA)) ||
	      INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LMETA)))))
	{
#else
	if( DeviceI == DeviceInput(DEVICE_JOY1, JOY_DOWN_2)
		&& INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_JOY1, JOY_5))
		&& INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_JOY1, JOY_6)) )
	{
#endif
		// If holding LShift save uncompressed, else save compressed
		bool bSaveCompressed = !INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_JOY1, JOY_10) );
		SaveScreenshot( "Screenshots/", bSaveCompressed );
		return true;	// handled
	}

	return false;
}

static void HandleInputEvents(float fDeltaTime)
{
	INPUTFILTER->Update( fDeltaTime );
	
	/* Hack: If the topmost screen hasn't been updated yet, don't process input, since
	 * we must not send inputs to a screen that hasn't at least had one update yet. (The
	 * first Update should be the very first thing a screen gets.)  We'll process it next
	 * time.  Do call Update above, so the inputs are read and timestamped. */
	if( SCREENMAN->GetTopScreen()->IsFirstUpdate() )
		return;

	static InputEventArray ieArray;
	ieArray.clear();	// empty the array
	INPUTFILTER->GetInputEvents( ieArray );

	/* If we don't have focus, discard input. */
	if( !g_bHasFocus )
		return;

	for( unsigned i=0; i<ieArray.size(); i++ )
	{
		DeviceInput DeviceI = (DeviceInput)ieArray[i];
		InputEventType type = ieArray[i].type;
		GameInput GameI;
		MenuInput MenuI;
		StyleInput StyleI;

		INPUTMAPPER->DeviceToGame( DeviceI, GameI );
		
		if( GameI.IsValid()  &&  type == IET_FIRST_PRESS )
			INPUTQUEUE->RememberInput( GameI );
		if( GameI.IsValid() )
		{
			INPUTMAPPER->GameToMenu( GameI, MenuI );
			INPUTMAPPER->GameToStyle( GameI, StyleI );
		}

		// HACK:  Numlock is read is being pressed if the NumLock light is on.
		// Filter out all NumLock repeat messages
		/* XXX: Is this still needed?  If so, it should probably be done in the
		 * affected input driver. */
//		if( DeviceI.device == DEVICE_KEYBOARD && DeviceI.button == KEY_NUMLOCK && type != IET_FIRST_PRESS )
//			continue;	// skip

		if( HandleGlobalInputs(DeviceI, type, GameI, MenuI, StyleI ) )
			continue;	// skip
		
		// check back in event mode
		if( PREFSMAN->m_bEventMode &&
			CodeDetector::EnteredCode(GameI.controller,CodeDetector::CODE_BACK_IN_EVENT_MODE) )
		{
			MenuI.player = PLAYER_1;
			MenuI.button = MENU_BUTTON_BACK;
		}

		SCREENMAN->Input( DeviceI, type, GameI, MenuI, StyleI );
	}
}

void FocusChanged( bool bHasFocus )
{
	g_bHasFocus = bHasFocus;

	LOG->Trace( "App %s focus", g_bHasFocus? "has":"doesn't have" );

	/* If we lose focus, we may lose input events, especially key releases. */
	INPUTFILTER->Reset();
}

static void CheckSkips( float fDeltaTime )
{
	if( !PREFSMAN->m_bLogSkips )
		return;

	static int iLastFPS = 0;
	int iThisFPS = DISPLAY->GetFPS();

	/* If vsync is on, and we have a solid framerate (vsync == refresh and we've sustained this
	 * for at least one frame), we expect the amount of time for the last frame to be 1/FPS. */

	if( iThisFPS != DISPLAY->GetVideoModeParams().rate || iThisFPS != iLastFPS )
	{
		iLastFPS = iThisFPS;
		return;
	}

	const float fExpectedTime = 1.0f / iThisFPS;
	const float fDifference = fDeltaTime - fExpectedTime;
	if( fabsf(fDifference) > 0.002f && fabsf(fDifference) < 0.100f )
		LOG->Trace("GameLoop timer skip: %i FPS, expected %.3f, got %.3f (%.3f difference)",
			iThisFPS, fExpectedTime, fDeltaTime, fDifference );
}

static void GameLoop()
{
	RageTimer timer;
	while(!g_bQuitting)
	{

		/*
		 * Update
		 */
		float fDeltaTime = timer.GetDeltaTime();

		if( PREFSMAN->m_fConstantUpdateDeltaSeconds > 0 )
			fDeltaTime = PREFSMAN->m_fConstantUpdateDeltaSeconds;
		
		CheckSkips( fDeltaTime );

#ifndef PSP
		if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_TAB) ) ) {
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_ACCENT) ) )
				fDeltaTime = 0; /* both; stop time */
			else
				fDeltaTime *= 4;
		}
		else if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_ACCENT) ) )
		{
			fDeltaTime /= 4;
		}
#endif

		DISPLAY->Update( fDeltaTime );

		/* Update SOUNDMAN early (before any RageSound::GetPosition calls), to flush position data. */
		SOUNDMAN->Update( fDeltaTime );

		/* Update song beat information -before- calling update on all the classes that
		 * depend on it.  If you don't do this first, the classes are all acting on old 
		 * information and will lag.  (but no longer fatally, due to timestamping -glenn) */
		SOUND->Update( fDeltaTime );
		TEXTUREMAN->Update( fDeltaTime );
		GAMESTATE->Update( fDeltaTime );
		SCREENMAN->Update( fDeltaTime );
		NSMAN->Update( fDeltaTime );

		/* Important:  Process input AFTER updating game logic, or input will be acting on song beat from last frame */
		HandleInputEvents( fDeltaTime );
		HOOKS->Update( fDeltaTime );
		/*
		 * Render
		 */
		SCREENMAN->Draw();

		/* If we don't have focus, give up lots of CPU. */
//		if( !g_bHasFocus )
//			usleep( 10000 );// give some time to other processes and threads
//		else
//			usleep( 1000 );	// give some time to other processes and threads
	}

	GAMESTATE->EndGame();
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

