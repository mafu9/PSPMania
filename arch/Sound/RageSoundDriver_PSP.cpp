#include "global.h"
#include "RageSoundDriver_PSP.h"
#include "RageLog.h"
#include "RageUtil.h"

static const int channels = 2;
static const int bytes_per_frame = 2 * channels; /* 16-bit */
static const int samplerate = 44100;

static const int writeahead = 1024 * 1;

int RageSound_PSP::MixerThread_start( void *p )
{
	((RageSound_PSP*) p)->MixerThread();
	return 0;
}

void RageSound_PSP::MixerThread()
{
	sceKernelChangeThreadPriority( sceKernelGetThreadId(), 0x13 );

	while( !shutdown )
	{
		sceKernelDelayThread( 1000 );

		LockMut( m_Mutex );

		for( int i = 0; i < PSP_AUDIO_CHANNEL_MAX; ++i )
		{
			stream *s = stream_pool[i];

			/* We're only interested in PLAYING and FLUSHING sounds. */
			if( s->state != stream::PLAYING && s->state != stream::FLUSHING )
				continue;

			/* still playing */
			if( sceAudioGetChannelRestLength(s->chid) > 0 )
				continue;

			if( s->state == stream::FLUSHING && s->fullSema.GetValue() == 0 )
			{
				if( s->chid >= 0 )
				{
					sceAudioChRelease( s->chid );
					s->chid = -1;
				}
				s->state = stream::FINISHED;
				continue;
			}

			s->fullSema.Wait();

			int volume = (int)((float)PSP_AUDIO_VOLUME_MAX * s->snd->GetVolume());
			sceAudioOutput( s->chid, volume, s->outBuffer );
			s->last_cursor_pos += writeahead;

			if( s->state != stream::FLUSHING )
				s->emptySema.Post();
		}
	}
}

static int DecoderThread( void *p )
{
	RageSound_PSP::stream *s = (RageSound_PSP::stream*)p;
	char *buffer[2];
	int index = 0;

	sceKernelChangeThreadPriority( sceKernelGetThreadId(), 0x13 );

	buffer[0] = (char*)memalign( 64, writeahead * bytes_per_frame );
	buffer[1] = (char*)memalign( 64, writeahead * bytes_per_frame );
	ASSERT( buffer[0] != NULL && buffer[1] != NULL );

	while( 1 )
	{
		s->emptySema.Wait();

		if( s->shutdown )
			break;

		int bytes_read = 0;
		int bytes_left = writeahead * bytes_per_frame;

		/* Does the sound have a start time? */
		if( !s->start_time.IsZero() )
		{
			/* If the sound is supposed to start at a time past this buffer, insert silence. */
			const float fSecondsBeforeStart = -s->start_time.Ago();
			const int64_t iSilentFramesInThisBuffer = int64_t(fSecondsBeforeStart * samplerate);
			const int iSilentBytesInThisBuffer = clamp( int(iSilentFramesInThisBuffer * bytes_per_frame), 0, bytes_left );

			if( iSilentBytesInThisBuffer )
			{
				/* Fill the remainder of the buffer with silence. */
				memset( buffer[index], 0, iSilentBytesInThisBuffer );
				bytes_read += iSilentBytesInThisBuffer;
				bytes_left -= iSilentBytesInThisBuffer;
			}
			else
				s->start_time.SetZero();
		}
		if( bytes_left > 0 )
		{
			int got = s->snd->GetPCM( buffer[index] + bytes_read, bytes_left, s->write_cursor_pos + (bytes_read / bytes_per_frame) );
			bytes_read += got;
			bytes_left -= got;
			if( bytes_left > 0 )
			{
				memset( buffer[index] + bytes_read, 0, bytes_left );
				s->state = RageSound_PSP::stream::FLUSHING;
			}
		}
		s->outBuffer = buffer[index];
		s->write_cursor_pos += writeahead;

		s->fullSema.Post();
		index ^= 1;
	}

	free( buffer[0] );
	free( buffer[1] );
	return 0;
}

void RageSound_PSP::Update( float delta )
{
	for( int i = 0; i < PSP_AUDIO_CHANNEL_MAX; ++i )
	{
		if( stream_pool[i]->state != stream::FINISHED )
			continue;

		/* The sound has stopped and flushed all of its buffers. */
		if( stream_pool[i]->snd != NULL )
		{
			stream_pool[i]->snd->SoundIsFinishedPlaying();
			stream_pool[i]->snd = NULL;
		}

		/* Once we do this, the sound is once available for use; we must lock
		 * m_InactiveSoundMutex to take it out of INACTIVE again. */
		stream_pool[i]->state = stream::INACTIVE;
	}
}

void RageSound_PSP::StartMixing( RageSoundBase *snd )
{
	/* Lock INACTIVE sounds[], and reserve a slot. */
	m_InactiveSoundMutex.Lock();

	/* Find an unused buffer. */
	stream *s = NULL;
	for( int i = 0; i < PSP_AUDIO_CHANNEL_MAX; ++i )
	{
		if( stream_pool[i]->state == stream::INACTIVE )
		{
			s = stream_pool[i];
			break;
		}
	}
	if( s == NULL )
	{
		/* We don't have a free sound buffer. */
		m_InactiveSoundMutex.Unlock();
		return;
	}

	s->chid = sceAudioChReserve( PSP_AUDIO_NEXT_CHANNEL, writeahead, PSP_AUDIO_FORMAT_STEREO );
	if( s->chid >= 0 )
	{
		s->snd = snd;
		s->start_time = snd->GetStartTime();
		s->last_cursor_pos = 0;
		s->write_cursor_pos = 0;
		s->emptySema.Post();
		s->state = stream::PLAYING;
	}

	m_InactiveSoundMutex.Unlock();
}

void RageSound_PSP::StopMixing( RageSoundBase *snd )
{
	ASSERT( snd != NULL );

	/* Lock, to make sure the decoder thread isn't running on this sound while we do this. */
	m_Mutex.Lock();

	int i;
	for( i = 0; i < PSP_AUDIO_CHANNEL_MAX; ++i )
	{
		if( stream_pool[i]->snd == snd )
			break;
	}

	if( i == PSP_AUDIO_CHANNEL_MAX )
	{
		m_Mutex.Unlock();
		LOG->Trace( "not stopping a sound because it's not playing" );
		return;
	}

	if( stream_pool[i]->state == stream::PLAYING )
	{
		stream_pool[i]->fullSema.Wait();
		/* FLUSHING tells the mixer thread to release the stream. */
		stream_pool[i]->state = stream::FLUSHING;
	}

	/* This function is called externally (by RageSoundBase) to stop immediately.
	 * We need to prevent SoundStopped from being called; it should only be
	 * called when we stop implicitely at the end of a sound.  Set snd to NULL. */
	stream_pool[i]->snd = NULL;

	m_Mutex.Unlock();
}

int64_t RageSound_PSP::GetPosition( const RageSoundBase *snd ) const
{
	int i;
	for( i = 0; i < PSP_AUDIO_CHANNEL_MAX; ++i )
	{
		if( stream_pool[i]->snd == snd )
			break;
	}

	if( i == PSP_AUDIO_CHANNEL_MAX )
		RageException::Throw( "GetPosition: Sound %s is not being played", snd->GetLoadedFilePath().c_str() );

	int rest = sceAudioGetChannelRestLength( stream_pool[i]->chid );
	if( rest < 0 )
		rest = 0;

	/* XXX: This isn't quite threadsafe. */
	return stream_pool[i]->last_cursor_pos - rest;
}

int RageSound_PSP::GetSampleRate( int rate ) const
{
	return samplerate;
}

RageSound_PSP::RageSound_PSP():
	m_Mutex( "PSPSoundMutex" ),
	m_InactiveSoundMutex( "InactiveSoundMutex" )
{
	shutdown = false;

	for( int i = 0; i < PSP_AUDIO_CHANNEL_MAX; ++i )
	{
		stream_pool[i] = new stream;
		DecodingThread[i].SetName( ssprintf("Decoder thread %d", i+1) );
		DecodingThread[i].Create( DecoderThread, stream_pool[i], 0x3000 );
	}

	MixingThread.SetName( "Mixer thread" );
	MixingThread.Create( MixerThread_start, this );
}

RageSound_PSP::~RageSound_PSP()
{
	/* Signal the mixing thread to quit. */
	shutdown = true;
	LOG->Trace( "Shutting down mixer thread ..." );
	MixingThread.Wait();
	LOG->Trace( "Mixer thread shut down." );

	for( unsigned i = 0; i < PSP_AUDIO_CHANNEL_MAX; ++i )
	{
		if( stream_pool[i]->chid >= 0 )
			sceAudioChRelease( stream_pool[i]->chid );

		stream_pool[i]->shutdown = true;
		stream_pool[i]->emptySema.Post();
		DecodingThread[i].Wait();
		delete stream_pool[i];
	}
}