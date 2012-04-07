/*
 * RageSoundReader_MP3 - Sound reader for MP3s
 */

#ifndef RAGE_SOUND_READER_MP3_H
#define RAGE_SOUND_READER_MP3_H

#include "RageSoundReader_FileReader.h"
#include "RageFile.h"

struct madlib_t;

class RageSoundReader_MP3: public SoundReader_FileReader
{
	int SampleRate;
    int Channels;

	CString filename;
    RageFile file;

#ifdef PSP_MEDIAENGIN_MP3
	int id3v2tagSize;
	int streamSize;
	int totalSamples;
	int decodedSamples;
	int frameNum;
	int firstOffset;
	int mixOutput;
	int mixBufCount;

	bool vbr;
	int vbrHeaderSize;
	int encoderDelay;
	int samplePerFrame;
	int bitrate;
	int version;

	uint16_t *mixBuffer;
	uint8_t *dataBuffer;
	unsigned long *codecBuffer;

	enum { SEEK_TABLE_SIZE = 1024 };
	struct SeekTable
	{
		int data[SEEK_TABLE_SIZE];
		int fill;
		int step;
	}table;

	int ReadMpegAudioHeader();
	int GetEncoderDelayAndPadding();
	int SeekNextFrame();
	void UpdateSeekTable();
	int DecodeFrame();
	int Seek( int sample );
#else
    madlib_t *mad;


	bool MADLIB_rewind();
	int SetPosition_toc( int ms, bool Xing );
	int SetPosition_hard( int ms );
	int SetPosition_estimate( int ms );

	int fill_buffer();
	int do_mad_frame_decode( bool headers_only=false );
	int resync();
	void synth_output();
	int seek_stream_to_byte( int byte );
	bool handle_first_frame();
#endif

	int GetLengthInternal( bool fast );
	int GetLengthConst( bool fast ) const;

public:
	OpenResult Open( const CString &filename );
	void Close();
	int GetLength() const { return GetLengthConst( false ); }
	int GetLength_Fast() const { return GetLengthConst( true ); }
	int SetPosition_Accurate( int ms );
	int SetPosition_Fast( int ms );
	int Read( char *buf, unsigned len );
	int GetSampleRate() const { return SampleRate; }

	RageSoundReader_MP3();
	~RageSoundReader_MP3();
	RageSoundReader_MP3( const RageSoundReader_MP3 & ); /* not defined; don't use */
	SoundReader *Copy() const;
};

#endif

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

