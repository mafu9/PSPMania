#include "global.h"
#include "RageInput.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "InputFilter.h"

#include <pspctrl.h>

RageInput*		INPUTMAN	= NULL;		// globally accessable input device

static int InputThread_Start( void *p )
{
	((RageInput *) p)->InputThreadMain();
	return 0;
}

RageInput::RageInput()
{
	LOG->Trace( "RageInput::RageInput()" );

	shutdown = false;

	sceCtrlSetSamplingCycle( 0 );
	sceCtrlSetSamplingMode( PSP_CTRL_MODE_ANALOG );

	if( PREFSMAN->m_bThreadedInput )
	{
		InputThread.SetName( "InputThread" );
		InputThread.Create( InputThread_Start, this, 0x800 );
	}
}

RageInput::~RageInput()
{
	if( InputThread.IsCreated() )
	{
		shutdown = true;
		LOG->Trace("Shutting down Input thread ...");
		InputThread.Wait();
		LOG->Trace("Input thread shut down.");
		shutdown = false;
	}
}

void RageInput::ButtonPressed( DeviceInput di, bool Down )
{
	if( di.ts.IsZero() )
		di.ts = LastUpdate.Half();

	INPUTFILTER->ButtonPressed( di, Down );
}

void RageInput::HandleInput( const RageTimer &tm )
{
	static const uint32_t buttonsBits[] = {
		PSP_CTRL_CROSS,
		PSP_CTRL_CIRCLE,
		PSP_CTRL_SQUARE,
		PSP_CTRL_TRIANGLE,
		PSP_CTRL_LTRIGGER,
		PSP_CTRL_RTRIGGER,
		PSP_CTRL_SELECT,
		PSP_CTRL_START
	};

	SceCtrlData padData;
	sceCtrlPeekBufferPositive( &padData, 1 );
	const uint32_t buttons = padData.Buttons;

	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_LEFT,		-1, tm ), (buttons & PSP_CTRL_LEFT) );
	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_RIGHT,		-1, tm ), (buttons & PSP_CTRL_RIGHT) );
	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_UP,		-1, tm ), (buttons & PSP_CTRL_UP) );
	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_DOWN,		-1, tm ), (buttons & PSP_CTRL_DOWN) );

	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_LEFT_2,	-1, tm ), (padData.Lx < 127 - 30) );
	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_RIGHT_2,	-1, tm ), (padData.Lx > 127 + 30) );
	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_UP_2,		-1, tm ), (padData.Ly < 127 - 30) );
	ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_DOWN_2,	-1, tm ), (padData.Ly > 127 + 30) );

	for( unsigned i = 0; i < ARRAYSIZE(buttonsBits); ++i )
		ButtonPressed( DeviceInput( DEVICE_JOY1, JOY_1+i,	-1, tm ), (buttons & buttonsBits[i]) );
}

void RageInput::Update( float fDeltaTime )
{
	RageTimer zero(0, 0);
	HandleInput( zero );

	LastUpdate.Touch();
}

void RageInput::InputThreadMain()
{
	sceKernelChangeThreadPriority( sceKernelGetThreadId(), 0x11 );

	while( !shutdown )
	{
		RageTimer now;
		HandleInput( now );
		sceKernelDelayThread( 50000 );
	}
}

void RageInput::GetDevicesAndDescriptions(vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut)
{
	vDevicesOut.push_back( DEVICE_JOY1 );
	vDescriptionsOut.push_back( "PSP Gamepad" );
}

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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
