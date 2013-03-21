#include "global.h"
#include "SnapDisplay.h"
#include "GameManager.h"
#include "GameState.h"
#include "ThemeManager.h"
#include "Style.h"


SnapDisplay::SnapDisplay()
{
	m_sprIndicators[0].Load( THEME->GetPathToG("SnapDisplay icon 8x1") );
	m_sprIndicators[1].Load( THEME->GetPathToG("SnapDisplay icon 8x1") );
	// MD 11/12/03 - this assert doesn't allow us to set the editor's maximum
	// resolution to less than the maximum one we have available.  Meh.
	// ASSERT( m_sprIndicators[0].GetNumStates() == NUM_NOTE_TYPES );
	// ASSERT( m_sprIndicators[1].GetNumStates() == NUM_NOTE_TYPES );
	m_sprIndicators[0].StopAnimating();
	m_sprIndicators[1].StopAnimating();
	this->AddChild( &m_sprIndicators[0] );
	this->AddChild( &m_sprIndicators[1] );

	m_NoteType = NOTE_TYPE_4TH;
	
	m_iNumCols = 0;
}

void SnapDisplay::Load( PlayerNumber pn )
{
	m_iNumCols = GAMESTATE->GetCurrentStyle()->m_iColsPerPlayer;

	const float fX = ARROW_SIZE * (m_iNumCols/2 + 0.5f);
	m_sprIndicators[0].SetX( -fX );
	m_sprIndicators[1].SetX( +fX );
}

bool SnapDisplay::PrevSnapMode()
{
	if( m_NoteType == 0 )
		return false;
	m_NoteType = NoteType(m_NoteType-1);

	SnapModeChanged();
	return true;
}

bool SnapDisplay::NextSnapMode()
{
	if( m_NoteType == NOTE_TYPE_64TH )	// this is the smallest snap we should allow
		return false;
	m_NoteType = NoteType(m_NoteType+1);

	SnapModeChanged();
	return true;
}


void SnapDisplay::SnapModeChanged()
{
	m_sprIndicators[0].SetState( m_NoteType );
	m_sprIndicators[1].SetState( m_NoteType );
}

/*
 * (c) 2001-2002 Chris Danford
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
