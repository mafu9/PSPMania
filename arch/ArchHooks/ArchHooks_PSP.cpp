#include "global.h"
#include "ProductInfo.h"
#include "RageLog.h"
#include "RageThreads.h"
#include "ArchHooks_PSP.h"
#include "StepMania.h"
#include "RageUtil.h"

#include "../../archutils/PSP/Fpu.h"
#ifdef ENABLE_PROFILE
#include <pspprof.h>
#endif

// called when exiting game from HOME button
static int exit_callback( int arg1, int arg2, void *common )
{
#if 0 // be careful. settings files arent be saved.
	if( LOG )
		LOG->Flush();

	pspIoLock();
	pspIoCloseAllAlive();

#ifdef ENABLE_PROFILE
	gprof_cleanup();
#endif

	sceKernelExitGame(); // return to XMB
#else

#ifdef ENABLE_PROFILE
	gprof_cleanup();
#endif

	ExitGame();
#endif
	return 0;
}

// called when changed the power state
static int power_callback( int unknown, int pwrflags, void *common )
{
	int *suspend = (int*)common;

	if( pwrflags & (PSP_POWER_CB_POWER_SWITCH|PSP_POWER_CB_SUSPENDING) ) // go into sleep mode
	{
		if( *suspend == 0 )
		{
			pspIoLock();
			pspIoCloseAllAlive(); // must close files because the system release its file handles before sleep mode
			*suspend = 1;
		}
	}
	else if( pwrflags & PSP_POWER_CB_RESUME_COMPLETE )
	{
		if( *suspend != 0 )
		{
			pspIoUnlock();
			*suspend = 0;
		}
	}

	sceKernelDelayThread( 1000000 ); // must delay
	return 0;
}

static int CallbackThread( SceSize args, void *argp )
{
	int suspend = 0;
	sceKernelRegisterExitCallback( sceKernelCreateCallback("Exit Callback", exit_callback, NULL) );
	scePowerRegisterCallback( -1, sceKernelCreateCallback("Power Callback", power_callback, &suspend) );
	sceKernelSleepThreadCB();
	return 0;
}

static int SetUpCallbacks()
{
	SceUID thid = sceKernelCreateThread( "update_thread", CallbackThread, 0x11, 0xFA0, 0, NULL );	
	if( thid >= 0 )
		sceKernelStartThread( thid, 0, NULL );

	return thid;
}

ArchHooks_PSP::ArchHooks_PSP()
{
	SetUpCallbacks();

	scePowerSetClockFrequency( 333, 333, 166 );

	DisableFPUExceptions();

	// create for a screenshot folder
	{
		char buf[32];
		pspGetDevice( buf, sizeof(buf) );

		const CString sDevice = buf;
		sceIoRemove( sDevice + "/" PRODUCT_NAME ".txt" );
		sceIoMkdir( sDevice + "/Picture", 0777 );
		sceIoMkdir( sDevice + "/Picture/PSPMania", 0777 );
	}

	{
		char root[PATH_MAX];
		getcwd( root, PATH_MAX );
		pspIoChdir( root );
	}

	// init VFPU random number generator
	{
		uint32_t seed = time( NULL );
		__asm__ volatile (
			"mtv		%0,	S000\n"
			"vrnds.s	S000\n"
			: : "r"(seed)
		);
	}

	int ret;

	ret = sceUtilityLoadAvModule( PSP_AV_MODULE_AVCODEC );
	if( ret < 0 )
		FAIL_M( ssprintf("Failed to load avcodec module: %08X\r\n", ret) );
}

ArchHooks_PSP::~ArchHooks_PSP()
{
	sceUtilityUnloadAvModule( PSP_AV_MODULE_AVCODEC );

#ifdef ENABLE_PROFILE
	gprof_cleanup();
#endif

	sceKernelExitGame();
}

void ArchHooks_PSP::DumpDebugInfo()
{
	uint32_t memory = GetMemoryTotalSize() / 1048576;
	const char *firmware = "Custom Firmware";

	uint32_t version = sceKernelDevkitVersion(); // get the version of PSP
	uint32_t major = (version >> 24) & 0xF;
	uint32_t minor = ((version >> 16) & 0xF) * 10 + ((version >> 8) & 0xF);

	LOG->Info( "System Software: %s %u.%02u", firmware, major, minor );
	LOG->Info( "Memory: %u MB", memory );
}