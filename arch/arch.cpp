/*
 * This file provides functions to create driver objects.
 */

#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"

#include "PrefsManager.h"
#include "arch.h"
#include "arch_psp.h"

#include "LoadingWindow/LoadingWindow_Null.h"

LoadingWindow *MakeLoadingWindow()
{
	if( !PREFSMAN->m_bShowLoadingWindow )
		return new LoadingWindow_Null;
	else
		return new LoadingWindow_PSP;
}

RageSoundDriver *MakeRageSoundDriver()
{
	return new RageSound_PSP;
}
