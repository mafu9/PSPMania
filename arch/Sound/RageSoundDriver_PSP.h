#ifndef RAGE_SOUND_PSP_H
#define RAGE_SOUND_PSP_H

#include "RageSound.h"
#include "RageThreads.h"
#include "RageSoundDriver.h"

#include <pspaudio.h>

class RageSound_PSP: public RageSoundDriver
{
private:
	/* This mutex serializes the decode thread and StopMixing. */
	RageMutex m_Mutex;

	/* The only place that takes sounds out of INACTIVE is StartMixing; this mutex
	 * serializes inactive sounds. */
	RageMutex m_InactiveSoundMutex;

	bool shutdown;

	static int MixerThread_start(void *p);
	void MixerThread();
	RageThread MixingThread;

	RageThread DecodingThread[PSP_AUDIO_CHANNEL_MAX];

public:
	struct stream
	{
		stream(): emptySema("EmptySema"), fullSema("FullSema")
		{
			state = INACTIVE;
			chid = -1;
			snd = NULL;
			outBuffer = NULL;
			shutdown = false;
			last_cursor_pos = 0;
			write_cursor_pos = 0;
		}

		enum {
			INACTIVE = 0,
			PLAYING,
			FLUSHING,
			FINISHED
		} state;

		int chid;
		RageSoundBase *snd;
		RageTimer start_time;
		char *outBuffer;
		bool shutdown;
		int64_t last_cursor_pos, write_cursor_pos;
		RageSemaphore emptySema, fullSema;
	} *stream_pool[PSP_AUDIO_CHANNEL_MAX];

	/* virtuals: */
	void StartMixing(RageSoundBase *snd);
	void StopMixing(RageSoundBase *snd);
	int64_t GetPosition( const RageSoundBase *snd ) const;
	int GetSampleRate( int rate ) const;

	void Update(float delta);

	RageSound_PSP();
	~RageSound_PSP();
};

#endif