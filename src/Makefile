TARGET = stepmania

PSP_FW_VERSION = 500

# use ExtraMemory
PSP_LARGE_MEMORY = 1

# debug
DEBUG = 0
USE_PROFILE = 0
DEBUG_MAP = 0

# when signing, need to set 1.
BUILD_PRX = 1

# unfinished
WITHOUT_MOVIE = 1
WITHOUT_NETWORKING = 1

HAVE_WAV = 1
HAVE_VORBIS = 1
HAVE_MP3 = 1
USE_PSP_MEDIAENGIN_MP3 = 1

Screens = \
Screen.o ScreenAttract.o ScreenBookkeeping.o ScreenBranch.o ScreenCaution.o \
ScreenCenterImage.o ScreenClearBookkeepingData.o ScreenClearMachineStats.o \
ScreenCredits.o ScreenDemonstration.o ScreenEditCoursesMenu.o \
ScreenEditMenu.o ScreenEnding.o ScreenEvaluation.o ScreenExit.o \
ScreenEz2SelectMusic.o ScreenEz2SelectPlayer.o ScreenGameOver.o \
ScreenGameplay.o ScreenHowToPlay.o ScreenInsertCredit.o ScreenInstructions.o \
ScreenJukebox.o ScreenJukeboxMenu.o ScreenLogo.o ScreenMapControllers.o \
ScreenMiniMenu.o ScreenMusicScroll.o ScreenNameEntry.o \
ScreenNameEntryTraditional.o ScreenNetEvaluation.o ScreenNetSelectMusic.o \
ScreenNetworkOptions.o ScreenOptions.o ScreenOptionsMaster.o \
ScreenOptionsMasterPrefs.o ScreenPlayerOptions.o ScreenProfileOptions.o \
ScreenPrompt.o ScreenRanking.o ScreenReloadSongs.o ScreenResetToDefaults.o \
ScreenSandbox.o ScreenSelect.o ScreenSelectCharacter.o ScreenSelectDifficulty.o \
ScreenSelectGroup.o ScreenSelectMaster.o ScreenSelectMode.o ScreenSelectMusic.o \
ScreenSelectStyle.o ScreenSetTime.o ScreenSongOptions.o ScreenStage.o \
ScreenStyleSplash.o ScreenTest.o ScreenTestFonts.o ScreenTestInput.o \
ScreenTestSound.o ScreenTextEntry.o ScreenTitleMenu.o ScreenUnlock.o \
ScreenWithMenuElements.o

DataStructures = \
Attack.o BannerCache.o CatalogXml.o Character.o CodeDetector.o Course.o \
CourseUtil.o DateTime.o Difficulty.o EnumHelper.o Font.o FontCharAliases.o \
FontCharmaps.o Game.o GameConstantsAndTypes.o GameInput.o Grade.o HighScore.o \
Inventory.o LuaHelpers.o LyricsLoader.o ModeChoice.o NoteData.o NoteDataUtil.o \
NoteDataWithScoring.o NoteFieldPositioning.o NotesLoader.o NotesLoaderBMS.o \
NotesLoaderDWI.o NotesLoaderKSF.o NotesLoaderSM.o NotesWriterDWI.o \
NotesWriterSM.o NoteTypes.o PlayerAI.o PlayerNumber.o PlayerOptions.o \
Profile.o RadarValues.o RandomSample.o ScoreKeeperMAX2.o ScoreKeeperRave.o \
Song.o SongCacheIndex.o SongOptions.o SongUtil.o StageStats.o Steps.o \
StepsUtil.o Style.o StyleUtil.o TimingData.o TitleSubstitution.o Trail.o \
TrailUtil.o

FileTypes = IniFile.o MsdFile.o XmlFile.o

StepMania = StepMania.o global.o

LoadingWindow = arch/LoadingWindow/LoadingWindow_PSP.o

Sound = arch/Sound/RageSoundDriver_PSP.o

ArchHooks = arch/ArchHooks/ArchHooks.o arch/ArchHooks/ArchHooks_PSP.o

MovieTexture = 

ifeq ($(WITHOUT_MOVIE),0)
MovieTexture += arch/MovieTexture/MovieTexture.o \
		arch/MovieTexture/MovieTexture_Null.o
endif

ArchUtils = archutils/PSP/arch_setup.o archutils/PSP/File.o \
		archutils/PSP/GuList.o archutils/PSP/Math.o \
		archutils/PSP/Memory.o archutils/PSP/Fpu.o

Dialog = arch/Dialog/Dialog.o arch/Dialog/DialogDriver_PSP.o

Arch = $(LoadingWindow) $(Sound) $(ArchHooks) $(MovieTexture) \
	   $(ArchUtils) $(Dialog) arch/arch.o

RageSoundFileReaders = RageSoundReader_WAV.o

ifeq ($(HAVE_VORBIS),1)
RageSoundFileReaders += RageSoundReader_Vorbisfile.o
endif

ifeq ($(HAVE_MP3),1)
RageSoundFileReaders += RageSoundReader_MP3.o
endif

ActorsInGameplayAndMenus = \
Banner.o BGAnimation.o BGAnimationLayer.o ConditionalBGA.o DifficultyIcon.o \
MeterDisplay.o Transition.o

ActorsInMenus = \
BPMDisplay.o ComboGraph.o CourseContentsList.o CourseEntryDisplay.o \
DifficultyDisplay.o DifficultyList.o DifficultyMeter.o DifficultyRating.o \
DualScrollBar.o EditCoursesMenu.o EditCoursesSongMenu.o EditMenu.o \
FadingBanner.o GradeDisplay.o GraphDisplay.o GrooveGraph.o GrooveRadar.o \
GroupList.o HelpDisplay.o JukeboxMenu.o ListDisplay.o MenuTimer.o \
ModeSwitcher.o MusicBannerWheel.o MusicList.o \
MusicSortDisplay.o MusicWheel.o MusicWheelItem.o OptionIcon.o OptionIconRow.o \
OptionsCursor.o PaneDisplay.o ScrollBar.o ScrollingList.o SnapDisplay.o \
SongCreditDisplay.o TextBanner.o WheelNotifyIcon.o

ActorsInGameplay = \
ActiveAttackList.o ArrowBackdrop.o ArrowEffects.o AttackDisplay.o \
Background.o BeginnerHelper.o CharacterHead.o CombinedLifeMeterTug.o Combo.o \
DancingCharacters.o Foreground.o GhostArrow.o GhostArrowRow.o \
HoldGhostArrow.o HoldJudgment.o Judgment.o LifeMeterBar.o LifeMeterBattery.o \
LyricDisplay.o NoteDisplay.o NoteField.o PercentageDisplay.o Player.o \
ProTimingDisplay.o ReceptorArrow.o ReceptorArrowRow.o ScoreDisplay.o \
ScoreDisplayBattle.o ScoreDisplayNormal.o ScoreDisplayOni.o \
ScoreDisplayPercentage.o ScoreDisplayRave.o

Helpers = SDL_dither.o SDL_rotozoom.o SDL_SaveJPEG.o

PCRE = pcre/get.o pcre/maketables.o pcre/pcre.o pcre/study.o

RageFile = \
RageFile.o RageFileDriver.o RageFileManager.o RageFileDriverDirect.o \
RageFileDriverDirectHelpers.o RageFileDriverMemory.o RageFileDriverZip.o

Rage = $(Helpers) $(PCRE) $(RageFile) $(RageSoundFileReaders) \
RageBitmapTexture.o RageDisplay.o RageDisplay_PSP.o \
RageException.o RageInput.o RageInputDevice.o RageLog.o RageMath.o \
RageModelGeometry.o RageSound.o RageSoundManager.o RageSoundPosMap.o \
RageSoundReader_FileReader.o RageSoundReader_Preload.o \
RageSoundReader_Resample_Fast.o RageSoundResampler.o RageSurface.o \
RageSurfaceUtils.o RageSurfaceUtils_Palettize.o RageSurface_Load.o \
RageSurface_Load_PNG.o RageSurface_Load_JPEG.o RageSurface_Load_GIF.o \
RageSurface_Load_BMP.o RageSurface_Load_XPM.o RageTexture.o \
RageSurface_Save_BMP.o RageTextureID.o RageTextureManager.o RageThreads.o \
RageTimer.o RageUtil.o RageUtil_CharConversions.o RageUtil_BackgroundLoader.o \
RageUtil_FileDB.o

Actors = \
Actor.o ActorCommands.o ActorFrame.o ActorScroller.o ActorUtil.o BitmapText.o \
Model.o ModelManager.o ModelTypes.o Sprite.o

GlobalSingletons = \
AnnouncerManager.o Bookkeeper.o FontManager.o GameManager.o \
GameSoundManager.o GameState.o InputFilter.o InputMapper.o InputQueue.o \
NetworkSyncManager.o NetworkSyncServer.o NoteSkinManager.o PrefsManager.o \
ProfileManager.o ScreenManager.o SongManager.o ThemeManager.o UnlockSystem.o

ifeq ($(WITHOUT_NETWORKING),0)
# Compile NetworkSyncManager even if networking is disabled; it'll stub itself.
GlobalSingletons += ezsockets.o
endif

crypto = crypto/CryptMD5.o crypto/CryptNoise.o crypto/CryptRand.o crypto/CryptSHA.o

OBJS =	$(Screens) \
		$(DataStructures) \
		$(FileTypes) \
		$(StepMania) \
		$(Arch) \
		$(ActorsInGameplayAndMenus) \
		$(ActorsInMenus) \
		$(ActorsInGameplay) \
		$(Rage) \
		$(Actors) \
		$(GlobalSingletons) \
		$(crypto)

INCDIR =

CFLAGS = -O2 -G0 -Wall -Wno-switch

# just in case
CFLAGS += -falign-functions=32 -falign-loops -falign-labels -falign-jumps

# for PSP
CFLAGS += -DPSP

ifeq ($(DEBUG),1)
CFLAGS += -DDEBUG
endif

ifeq ($(USE_PROFILE),1)
CFLAGS += -pg -DENABLE_PROFILE
else
CFLAGS += -fomit-frame-pointer -fstrength-reduce
endif

ifeq ($(WITHOUT_NETWORKING),1)
CFLAGS += -DWITHOUT_NETWORKING
endif

ifeq ($(HAVE_WAV),0)
CFLAGS += -DNO_WAV_SUPPORT
endif

ifeq ($(HAVE_VORBIS),1)
CFLAGS += -DINTEGER_VORBIS
else
CFLAGS += -DNO_VORBIS_SUPPORT
endif

ifeq ($(HAVE_MP3),0)
CFLAGS += -DNO_MP3_SUPPORT
endif

ifeq ($(USE_PSP_MEDIAENGIN_MP3),1)
CFLAGS += -DPSP_MEDIAENGIN_MP3
endif

ifeq ($(WITHOUT_MOVIE),0)
CFLAGS += -DSUPPORT_MOVIE
endif

CXXFLAGS = $(CFLAGS) -fno-rtti -fno-strict-aliasing -finline-limit=300
#-fno-exceptions
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS = -Wl,--wrap,malloc -Wl,--wrap,realloc -Wl,--wrap,calloc \
		-Wl,--wrap,memalign -Wl,--wrap,free \

ifeq ($(DEBUG_MAP),1)
LDFLAGS += -Wl,-Map=$(TARGET)_map.txt
endif

PSP_LIBS = -lpsppower -lpsputility -lpsprtc -lpspgu -lpspaudio -lpspaudiocodec

ifeq ($(USE_PROFILE),1)
PSP_LIBS += -lpspprof
endif

LIBS = $(PSP_LIBS) -llua -lpng -lz -ljpeg -lvorbisidec -logg -lm -lstdc++

ifeq ($(USE_PSP_MEDIAENGIN_MP3),0)
LIBS += -lmad
endif

EXTRA_TARGETS = EBOOT.PBP

PSP_EBOOT_TITLE = PSPMania
#PSP_EBOOT_ICON = icon.png

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak