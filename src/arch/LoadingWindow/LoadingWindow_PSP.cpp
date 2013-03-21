#include "global.h"
#include "LoadingWindow_PSP.h"
#include <pspdisplay.h>
#include <pspdebug.h>

LoadingWindow_PSP::LoadingWindow_PSP()
{
	pspDebugScreenInitEx( NULL, PSP_DISPLAY_PIXEL_FORMAT_4444, 1 );
	pspDebugScreenEnableBackColor( 0 );
	text[0] = '\0';
}

LoadingWindow_PSP::~LoadingWindow_PSP()
{
	pspDebugScreenSetXY( 0, 0 );
	pspDebugScreenSetTextColor( 0xFF000000 );
	pspDebugScreenPuts( text );
}

void LoadingWindow_PSP::SetText( const CString &str )
{
	// Because of direct access to VRAM,
    // this way is faster than 'pspDebugScreenClear'.
	pspDebugScreenSetXY( 0, 0 );
	pspDebugScreenSetTextColor( 0xFF000000 );
	pspDebugScreenPuts( text );

	snprintf( text, sizeof(text), "%s", str.c_str() );

	pspDebugScreenSetXY( 0, 0 );
	pspDebugScreenSetTextColor( 0xFFFFFFFF );
	pspDebugScreenPuts( text );
}