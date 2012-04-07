#ifndef RAGE_THREADS_H
#define RAGE_THREADS_H

#include <pspkerneltypes.h>

struct ThreadSlot;
class RageThread
{
	ThreadSlot *m_pSlot;
	char m_szName[32];

	static bool m_bSystemSupportsTLS;

public:
	RageThread();
	~RageThread();

	void SetName( const char *name );
	void Create( int (*fn)(void *), void *data, int stackSize = 0x4000 );

	/* For crash handlers: kill or suspend all threads (except for
	 * the running one) immediately. */ 
	static void HaltAllThreads( bool Kill=false );

	/* If HaltAllThreads was called (with Kill==false), resume. */
	static void ResumeAllThreads();

	static uint32_t GetCurrentThreadID();

	static const char *GetCurThreadName();
	static const char *GetThreadNameByID( uint32_t iID );
	static bool EnumThreadIDs( int n, uint32_t &iID );
	int Wait();
	bool IsCreated() const { return m_pSlot != NULL; }

	/* A system can define HAVE_TLS, indicating that it can compile thread_local
	 * code, but an individual environment may not actually have functional TLS.
	 * If this returns false, thread_local variables are considered undefined. */
	static bool GetSupportsTLS() { return m_bSystemSupportsTLS; }
	
	static void SetSupportsTLS( bool b ) { m_bSystemSupportsTLS = b; }
};

namespace Checkpoints
{
	void LogCheckpoints( bool yes=true );
	void SetCheckpoint( const char *file, int line, const char *message );
	const char *GetLogs( const char *delim );
};

//#define CHECKPOINT (Checkpoints::SetCheckpoint(__FILE__, __LINE__, NULL))
//#define CHECKPOINT_M(m) (Checkpoints::SetCheckpoint(__FILE__, __LINE__, m))

/* Mutex class that follows the behavior of Windows mutexes: if the same
 * thread locks the same mutex twice, we just increase a refcount; a mutex
 * is considered unlocked when the refcount reaches zero.  This is more
 * convenient, though much slower on some archs.  (We don't have any tightly-
 * coupled threads, so that's OK.) */
class RageMutex
{
	SceUID m_Mutex;
	char m_szName[32];
	
	uint32_t m_LockedBy;
	int m_LockCnt;

public:
	void SetName( const char *name );
	void Lock();
	bool TryLock();
	void Unlock();
	bool IsLockedByThisThread() const;

	RageMutex() { RageMutex( "NoName" ); }
	RageMutex( const char *name );
	~RageMutex();
};

/* Lock a mutex on construction, unlock it on destruction.  Helps for functions
 * with more than one return path. */
class LockMutex
{
	RageMutex &mutex;

	const char *file;
	int line;
	float locked_at;
	bool locked;

public:
	LockMutex(RageMutex &mut, const char *file, int line);
	LockMutex(RageMutex &mut): mutex(mut), file(NULL), line(-1), locked_at(-1), locked(true) { mutex.Lock(); }
	~LockMutex();
	LockMutex(LockMutex &cpy): mutex(cpy.mutex), file(NULL), line(-1), locked_at(cpy.locked_at), locked(true) { mutex.Lock(); }

	/* Unlock the mutex (before this would normally go out of scope).  This can
	 * only be called once. */
	void Unlock();
};


/* Gar.  It works in VC7, but not VC6, so for now this can only be used once
 * per function.  If you need more than that, declare LockMutexes yourself. 
 * Also, VC6 doesn't support __FUNCTION__. */
#if _MSC_VER < 1300 /* VC6, not VC7 */
#define LockMut(m) LockMutex LocalLock(m, __FILE__, __LINE__)
#else
#define LockMut(m) LockMutex LocalLock(m, __FUNCTION__, __LINE__)
#endif

class RageSemaphore
{
	SceUID m_Sema;
	char m_szName[32];

public:
	RageSemaphore( const char *name, int iInitialValue = 0, int iMaxValue = 10000 );
	~RageSemaphore();

	int GetValue() const;
	void Post( int signal = 1 );
	void Wait( int signal = 1 );
	bool TryWait( int signal = 1 );
};

#endif

/*
 * Copyright (c) 2001-2004 Glenn Maynard
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
