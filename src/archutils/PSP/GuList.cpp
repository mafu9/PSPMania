#include "global.h"
#include "GuList.h"
#include <pspgu.h>

static unsigned int __attribute__((aligned(64)))g_List[NUM_GU_LIST];
static bool g_bListRunning = false;

bool StartGuList()
{
	if( g_bListRunning )
		return false;

	sceGuStart( GU_DIRECT, g_List );
	g_bListRunning = true;
	return true;
}

bool FinishGuList()
{
	if( !g_bListRunning )
		return false;

	ASSERT( sceGuFinish() <= GU_LIST_SIZE );

	g_bListRunning = false;
	return true;
}