#include "global.h"
#include "MusicList.h"
#include "ThemeManager.h"

/* If this actor is used anywhere other than SelectGroup, we
 * can add a setting that changes which metric group we pull
 * settings out of, so it can be configured separately. */
#define NUM_COLUMNS		THEME->GetMetricI("MusicList","NumColumns")
#define NUM_ROWS		THEME->GetMetricI("MusicList","NumRows")
#define START_X			THEME->GetMetricF("MusicList","StartX")
#define START_Y			THEME->GetMetricF("MusicList","StartY")
#define SPACING_X		THEME->GetMetricF("MusicList","SpacingX")
#define CROP_WIDTH		THEME->GetMetricF("MusicList","CropWidth")
#define INIT_COMMAND	THEME->GetMetric ("MusicList","InitCommand")

MusicList::MusicList()
{
	CurGroup = 0;
}

void MusicList::Load()
{
	const int iNumColumns = NUM_COLUMNS;
	const CString sTitlesFontPath = THEME->GetPathToF( "MusicList titles" );
	const CString sInitCommand = INIT_COMMAND;
	const float fStartX = START_X, fSpacingX = SPACING_X, fStartY = START_Y;
	for( int i=0; i<iNumColumns; i++ )
	{
		m_textTitles[i].LoadFromFont( sTitlesFontPath );
		m_textTitles[i].SetXY( fStartX + i*fSpacingX, fStartY );
		m_textTitles[i].Command( sInitCommand );
		this->AddChild( &m_textTitles[i] );
	}
}

void MusicList::AddGroup()
{
	m_ContentsText.push_back(group());
}

void MusicList::AddSongsToGroup(const vector<Song*> &Songs)
{
	// Generate what text will show in the contents for each group
	unsigned group = m_ContentsText.size()-1;

	m_ContentsText[group].m_iNumSongsInGroup = Songs.size();

	const int iNumColumns = NUM_COLUMNS, iNumRows = NUM_ROWS;
	for( int c=0; c<iNumColumns; c++ )	// foreach col
	{
		CString sText;
		for( int r=0; r<iNumRows; r++ )	// foreach row
		{
			unsigned iIndex = c*iNumRows + r;
			if( iIndex >= Songs.size() )
				continue;

			if( c == iNumColumns-1 && r == iNumRows-1 && Songs.size() != unsigned(iNumColumns*iNumRows) )
			{
				sText += ssprintf( "%i more.....", int(Songs.size() - iNumColumns * iNumRows + 1) );
				continue;
			}

			CString sTitle = Songs[iIndex]->GetFullDisplayTitle();
			// TODO:  Move this crop threshold into a theme metric or make automatic based on column width
			if( sTitle.GetLength() > 40 )
				sTitle = sTitle.Left( 37 ) + "...";

			CString sTrTitle = Songs[iIndex]->GetFullTranslitTitle();
			if( sTrTitle.GetLength() > 40 )
				sTrTitle = sTrTitle.Left( 37 ) + "...";

			/* If the main title isn't complete for this font, and we have a translit,
			 * use it. */
			if(m_textTitles[c].StringWillUseAlternate(sTitle, sTrTitle))
				sText += sTrTitle;
			else
				sText += sTitle;
			sText += "\n";
		}
		m_ContentsText[group].ContentsText[c] = sText;
	}

}

/* TODO: tween? */
void MusicList::SetGroupNo(int group)
{
	CurGroup = group;

	const int iNumColumns = NUM_COLUMNS;
	const float fCropWidth = CROP_WIDTH;
	for( int c=0; c<iNumColumns; c++ )
	{
		m_textTitles[c].SetText( m_ContentsText[CurGroup].ContentsText[c] );
		m_textTitles[c].CropToWidth( int(fCropWidth/m_textTitles[c].GetZoom()) );
	}
}

void MusicList::TweenOnScreen()
{
	const int iNumColumns = NUM_COLUMNS;
	for( int i=0; i<iNumColumns; i++ )
	{
		m_textTitles[i].SetDiffuse( RageColor(1,1,1,0) );
		m_textTitles[i].BeginTweening( 0.5f );
		m_textTitles[i].BeginTweening( 0.5f );
		m_textTitles[i].SetDiffuse( RageColor(1,1,1,1) );
	}
}

void MusicList::TweenOffScreen()
{
	const int iNumColumns = NUM_COLUMNS;
	for( int i=0; i<iNumColumns; i++ )
	{
		m_textTitles[i].StopTweening();
		m_textTitles[i].BeginTweening( 0.7f );
		m_textTitles[i].BeginTweening( 0.5f );
		m_textTitles[i].SetDiffuse( RageColor(1,1,1,0) );
	}
}

/*
 * (c) 2001-2003 Chris Danford
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
