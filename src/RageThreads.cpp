/*
 * If you're going to use threads, remember this: 
 *
 * Threads suck.
 *
 * If there's any way to avoid them, take it!  Threaded code an order of
 * magnitude more complicated, harder to debug and harder to make robust.
 *
 * That said, here are a few helpers for when they're unavoidable.
 */

#include "global.h"

#include "RageThreads.h"
#include "RageTimer.h"
#include "RageLog.h"
#include "RageUtil.h"

#include <cerrno>
#include <set>

#include "arch/Dialog/Dialog.h"

/* Assume TLS doesn't work until told otherwise.  It's ArchHooks's job to set this. */
bool RageThread::m_bSystemSupportsTLS = false;

#define MAX_THREADS 32
#define MAX_THREAD_NAME 32
#define MAX_THREAD_BUF_SIZE 256

#define INVALID_THREAD_ID (uint32_t)-1

struct ThreadSlot
{
	mutable char name[MAX_THREAD_NAME]; /* mutable so we can force nul-termination */

	/* Format this beforehand, since it's easier to do that than to do it under crash conditions. */
	char ThreadFormattedOutput[MAX_THREAD_BUF_SIZE];

	bool used;
	uint32_t id;

	#undef CHECKPOINT_COUNT
	#define CHECKPOINT_COUNT 5
	struct ThreadCheckpoint
	{
		const char *File, *Message;
		int Line;
		char FormattedBuf[MAX_THREAD_BUF_SIZE];

		ThreadCheckpoint() { Set( NULL, 0, NULL ); }
		void Set(const char *File_, int Line_, const char *Message_=NULL);
		const char *GetFormattedCheckpoint();
	};
	ThreadCheckpoint Checkpoints[CHECKPOINT_COUNT];
	int CurCheckpoint, NumCheckpoints;
	const char *GetFormattedCheckpoint( int lineno );

	ThreadSlot() { Init(); }
	void Init()
	{
		id = INVALID_THREAD_ID;
		CurCheckpoint = NumCheckpoints = 0;

		/* Reset used last; otherwise, a thread creation might pick up the slot. */
		used = false;
	}

	const char *GetThreadName() const;
};


void ThreadSlot::ThreadCheckpoint::Set(const char *File_, int Line_, const char *Message_)
{
	File=File_;
	Line=Line_;
	Message=Message_;
	snprintf( FormattedBuf, MAX_THREAD_BUF_SIZE, "        %s:%i %s",
		File, Line, Message? Message:"" );
}

const char *ThreadSlot::ThreadCheckpoint::GetFormattedCheckpoint()
{
	if( File == NULL )
		return NULL;

	/* Make sure it's terminated: */
	FormattedBuf [ sizeof(FormattedBuf)-1 ] = 0;

	return FormattedBuf;
}

const char *ThreadSlot::GetFormattedCheckpoint( int lineno )
{
	if( lineno >= CHECKPOINT_COUNT || lineno >= NumCheckpoints )
		return NULL;

	if( NumCheckpoints == CHECKPOINT_COUNT )
	{
		lineno += CurCheckpoint;
		lineno %= CHECKPOINT_COUNT;
	}

	return Checkpoints[lineno].GetFormattedCheckpoint();
}

static ThreadSlot g_ThreadSlots[MAX_THREADS];
struct ThreadSlot *g_pUnknownThreadSlot = NULL;

/* Lock this mutex before using or modifying pImpl.  Other values are just identifiers,
 * so possibly racing over them is harmless (simply using a stale thread ID, etc). */
static RageMutex g_ThreadSlotsLock("ThreadSlots");

static int FindEmptyThreadSlot()
{
	g_ThreadSlotsLock.Lock();

	for( int entry = 0; entry < MAX_THREADS; ++entry )
	{
		if( g_ThreadSlots[entry].used )
			continue;

		g_ThreadSlots[entry].used = true;
		g_ThreadSlotsLock.Unlock();
		return entry;
	}
	
	g_ThreadSlotsLock.Unlock();
	RageException::Throw("Out of thread slots!");
}

static void InitThreads()
{
	/* We don't have to worry about two threads calling this at on */
	static bool bInitialized = false;
	if( bInitialized )
		return;

	g_ThreadSlotsLock.Lock();

	/* Libraries might start threads on their own, which might call user callbacks,
	 * which could come back here.  Make sure we don't accidentally initialize twice. */
	if( bInitialized )
	{
		g_ThreadSlotsLock.Unlock();
		return;
	}

	bInitialized = true;

	/* Register the "unknown thread" slot. */
	int slot = FindEmptyThreadSlot();
	strcpy( g_ThreadSlots[slot].name, "Unknown thread" );
	g_ThreadSlots[slot].id = INVALID_THREAD_ID;
	strcpy( g_ThreadSlots[slot].ThreadFormattedOutput, "Unknown thread" );
	g_pUnknownThreadSlot = &g_ThreadSlots[slot];

	g_ThreadSlotsLock.Unlock();
}


static ThreadSlot *GetThreadSlotFromID( uint32_t iID )
{
	InitThreads();

	for( int entry = 0; entry < MAX_THREADS; ++entry )
	{
		if( !g_ThreadSlots[entry].used )
			continue;
		if( g_ThreadSlots[entry].id == iID )
			return &g_ThreadSlots[entry];
	}
	return NULL;
}

static ThreadSlot *GetCurThreadSlot()
{
	return GetThreadSlotFromID( RageThread::GetCurrentThreadID() );
}

static ThreadSlot *GetUnknownThreadSlot()
{
	return g_pUnknownThreadSlot;
}

RageThread::RageThread()
{
	m_pSlot = NULL;
	SetName( "" );
}

RageThread::~RageThread()
{

}

const char *ThreadSlot::GetThreadName() const
{
	/* This function may be called in crash conditions, so guarantee the string
	 * is null-terminated. */
	name[ sizeof(name)-1] = 0;

	return name;
}

static int Thread_start( SceSize arglen, void *argp )
{
	int (*pFunc)(void *pData) = (int(*)(void*))((uint32_t*)argp)[0];
	void *pData = (void*)((uint32_t*)argp)[1];

	pspIoChdir( NULL );
	pFunc( pData );

	sceKernelExitDeleteThread( 0 );
	return 0;
}

void RageThread::SetName( const char *name )
{
	strncpy( m_szName, name, 31 );
	m_szName[31] = 0;
}

void RageThread::Create( int (*fn)(void *), void *data, int stackSize )
{
	/* Don't create a thread that's already running: */
	ASSERT( m_pSlot == NULL );

	InitThreads();

	/* Lock unused slots, so nothing else uses our slot before we mark it used. */
	g_ThreadSlotsLock.Lock();

	int slotno = FindEmptyThreadSlot();
	m_pSlot = &g_ThreadSlots[slotno];
	
	if( strlen(m_szName) == 0 )
	{
		if( LOG )
			LOG->Warn("Created a thread without naming it first.");

		/* If you don't name it, I will: */
		strcpy( m_pSlot->name, "Joe" );
	} else {
		strncpy( m_pSlot->name, m_szName, MAX_THREAD_NAME );
		m_pSlot->name[MAX_THREAD_NAME-1] = 0;
	}

	if( LOG )
		LOG->Trace( "Starting thread: %s", m_szName );
	snprintf( m_pSlot->ThreadFormattedOutput, MAX_THREAD_BUF_SIZE, "Thread: %s", m_szName );

	/* Start a thread using our own startup function.  We pass the id to fill in,
	 * to make sure it's set before the thread actually starts.  (Otherwise, early
	 * checkpoints might not have a completely set-up thread slot.) */
	m_pSlot->id = sceKernelCreateThread( m_pSlot->name, Thread_start, 0x21, stackSize, PSP_THREAD_ATTR_USER|PSP_THREAD_ATTR_VFPU, NULL );
	ASSERT_M( m_pSlot->id >= 0, ssprintf("CreateThread(%s): %08X", m_pSlot->name, m_pSlot->id) );

	uint32_t args[] = { (uint32_t)fn, (uint32_t)data };
	sceKernelStartThread( m_pSlot->id, sizeof(args), args );

	g_ThreadSlotsLock.Unlock();
}

/* On startup, register the main thread's slot.  We do this in a static constructor,
 * and not InitThreads(), to guarantee it happens in the main thread. */
static struct SetupMainThread
{
	SetupMainThread()
	{
		g_ThreadSlotsLock.Lock();
		int slot = FindEmptyThreadSlot();
		strcpy( g_ThreadSlots[slot].name, "Main thread" );
		snprintf( g_ThreadSlots[slot].ThreadFormattedOutput, MAX_THREAD_BUF_SIZE, "Thread: %s", g_ThreadSlots[slot].name );
		g_ThreadSlots[slot].id = RageThread::GetCurrentThreadID();
		g_ThreadSlotsLock.Unlock();
	}
} SetupMainThreadObj;


const char *RageThread::GetCurThreadName()
{
	return GetThreadNameByID( GetCurrentThreadID() );
}

const char *RageThread::GetThreadNameByID( uint32_t iID )
{
	ThreadSlot *slot = GetThreadSlotFromID( iID );
	if( slot == NULL )
		return "???";

	return slot->GetThreadName();
}

bool RageThread::EnumThreadIDs( int n, uint32_t &iID )
{
	if( n >= MAX_THREADS )
		return false;

	g_ThreadSlotsLock.Lock();
	const ThreadSlot *slot = &g_ThreadSlots[n];

	if( slot->used )
		iID = slot->id;
	else
		iID = INVALID_THREAD_ID;
	g_ThreadSlotsLock.Unlock();
	return true;
}

int RageThread::Wait()
{
	ASSERT( m_pSlot != NULL );
	ASSERT( m_pSlot->id >= 0 );
	int ret = sceKernelWaitThreadEnd( m_pSlot->id, NULL );

	g_ThreadSlotsLock.Lock();
	m_pSlot->Init();
	m_pSlot = NULL;
	g_ThreadSlotsLock.Unlock();

	return ret;
}


void RageThread::HaltAllThreads( bool Kill )
{
	const uint32_t ThisThreadID = GetCurrentThreadID();
	for( int entry = 0; entry < MAX_THREADS; ++entry )
	{
		if( !g_ThreadSlots[entry].used )
			continue;
		if( ThisThreadID == g_ThreadSlots[entry].id )
			continue;

		if( Kill )
			sceKernelTerminateDeleteThread( g_ThreadSlots[entry].id );
		else
			sceKernelSuspendThread( g_ThreadSlots[entry].id );
	}
}

void RageThread::ResumeAllThreads()
{
	const uint32_t ThisThreadID = GetCurrentThreadID();
	for( int entry = 0; entry < MAX_THREADS; ++entry )
	{
		if( !g_ThreadSlots[entry].used )
			continue;
		if( ThisThreadID == g_ThreadSlots[entry].id )
			continue;

		sceKernelResumeThread( g_ThreadSlots[entry].id );
	}
}

uint32_t RageThread::GetCurrentThreadID()
{
	return sceKernelGetThreadId();
}

/* Normally, checkpoints are only seen in crash logs.  It's occasionally useful
 * to see them in logs, but this outputs a huge amount of text. */
static bool g_LogCheckpoints = false;
void Checkpoints::LogCheckpoints( bool on )
{
	g_LogCheckpoints = on;
}

void Checkpoints::SetCheckpoint( const char *file, int line, const char *message )
{
	ThreadSlot *slot = GetCurThreadSlot();
	if( slot == NULL )
		slot = GetUnknownThreadSlot();
	/* We can't ASSERT here, since that uses checkpoints. */
	if( slot == NULL )
		sm_crash( "GetUnknownThreadSlot() returned NULL" );

	slot->Checkpoints[slot->CurCheckpoint].Set( file, line, message );

	if( g_LogCheckpoints )
		LOG->Trace( "%s", slot->Checkpoints[slot->CurCheckpoint].FormattedBuf );

	++slot->CurCheckpoint;
	slot->NumCheckpoints = max( slot->NumCheckpoints, slot->CurCheckpoint );
	slot->CurCheckpoint %= CHECKPOINT_COUNT;
}

/* This is called under crash conditions.  Be careful. */
static const char *GetCheckpointLog( int slotno, int lineno )
{
	static char ret[1024*32];
	ret[0] = 0;

	ThreadSlot &slot = g_ThreadSlots[slotno];
	if( !slot.used )
		return NULL;

	/* Only show the "Unknown thread" entry if it has at least one checkpoint. */
	if( &slot == g_pUnknownThreadSlot && slot.GetFormattedCheckpoint( 0 ) == NULL )
		return NULL;

	if( lineno != 0 )
		return slot.GetFormattedCheckpoint( lineno-1 );

	slot.ThreadFormattedOutput[MAX_THREAD_BUF_SIZE-1] = 0;
	strcat(ret, slot.ThreadFormattedOutput);
	return ret;
}

const char *Checkpoints::GetLogs( const char *delim )
{
	static char ret[1024*32];
	ret[0] = 0;

	for( int slotno = 0; slotno < MAX_THREADS; ++slotno )
	{
		const char *buf = GetCheckpointLog( slotno, 0 );
		if( buf == NULL )
			continue;
		strcat( ret, buf );
		strcat( ret, delim );
		
		for( int line = 1; (buf = GetCheckpointLog( slotno, line )) != NULL; ++line )
		{
			strcat( ret, buf );
			strcat( ret, delim );
		}
	}	
	return ret;
}

RageMutex::RageMutex( const char *name )
{
	SetName( name );
	m_Mutex = sceKernelCreateSema( m_szName, 0, 1, 1, NULL );
	m_LockedBy = INVALID_THREAD_ID;
	m_LockCnt = 0;
}

RageMutex::~RageMutex()
{
	sceKernelDeleteSema( m_Mutex );
}

void RageMutex::SetName( const char *name )
{
	strncpy( m_szName, name, 31 );
	m_szName[31] = 0;
}

void RageMutex::Lock()
{
	uint32_t thisId = RageThread::GetCurrentThreadID();

	if( m_LockedBy == thisId )
	{
		++m_LockCnt;
		return;
	}

	sceKernelWaitSema( m_Mutex, 1, NULL );

	m_LockedBy = thisId;
}

bool RageMutex::TryLock()
{
	uint32_t thisId = RageThread::GetCurrentThreadID();

	if( m_LockedBy == thisId )
	{
		++m_LockCnt;
		return true;
	}

	SceUInt timeout = 0;
	if( sceKernelWaitSema( m_Mutex, 1, &timeout ) < 0 )
		return false;

	m_LockedBy = thisId;

	return true;
}

void RageMutex::Unlock()
{
	if( m_LockCnt )
	{
		--m_LockCnt;
		return;
	}

	m_LockedBy = INVALID_THREAD_ID;

	sceKernelSignalSema( m_Mutex, 1 );
}

bool RageMutex::IsLockedByThisThread() const
{
	return m_LockedBy == RageThread::GetCurrentThreadID();
}

LockMutex::LockMutex(RageMutex &pMutex, const char *file_, int line_): 
	mutex(pMutex),
	file(file_),
	line(line_)
{
	mutex.Lock();
	locked = true;

	if( file )
		locked_at = RageTimer::GetTimeSinceStart();
}

LockMutex::~LockMutex()
{
	if(locked)
		mutex.Unlock();
}

void LockMutex::Unlock()
{
	ASSERT( locked );
	locked = false;

	mutex.Unlock();

	if( file && locked_at != -1 )
	{
		const float dur = RageTimer::GetTimeSinceStart() - locked_at;
		if( dur > 0.015f )
			LOG->Trace( "Lock at %s:%i took %f", file, line, dur );
	}
}

RageSemaphore::RageSemaphore( const char *name, int iInitialValue, int iMaxValue )
{
	strncpy( m_szName, name, 31 );
	m_szName[31] = 0;
	m_Sema = sceKernelCreateSema( name, 0, iInitialValue, iMaxValue, NULL );
}

RageSemaphore::~RageSemaphore()
{
	sceKernelDeleteSema( m_Sema );
}

int RageSemaphore::GetValue() const
{
	SceKernelSemaInfo info;
	info.size = sizeof(SceKernelSemaInfo);
	sceKernelReferSemaStatus( m_Sema, &info );
	return info.currentCount;
}

void RageSemaphore::Post( int signal )
{
	sceKernelSignalSema( m_Sema, signal );
}

void RageSemaphore::Wait( int signal )
{
	sceKernelWaitSema( m_Sema, signal, NULL );
}

bool RageSemaphore::TryWait( int signal )
{
	SceUInt timeout = 0;
	return sceKernelWaitSema( m_Sema, signal, &timeout ) == 0;
}


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
