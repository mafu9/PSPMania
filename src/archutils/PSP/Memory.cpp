#include "global.h"
#include "Memory.h"

extern "C"
{
void *__real_malloc( size_t size );
void *__real_realloc( void *ptr, size_t size );
void *__real_calloc( size_t n, size_t size );
void *__real_memalign( size_t align, size_t size );
void __real_free( void *ptr );

void *__wrap_malloc( size_t size );
void *__wrap_realloc( void *ptr, size_t size );
void *__wrap_calloc( size_t n, size_t size );
void *__wrap_memalign( size_t align, size_t size );
void __wrap_free( void *ptr );
};

struct MemoryList
{
	void *buf;
	struct MemoryList *next;
};

static SceUID g_Mutex = -1;
static SceUID g_MutexOwner;
static int g_MutexCount;
static size_t g_TotalMemory;
static bool g_bMemoryAssert = true;

static void InitAlloc()
{
	g_Mutex = sceKernelCreateSema( "MemoyMutex", 0, 1, 1, NULL );
	g_MutexOwner = -1;
	g_TotalMemory = GetMemoryFreeSize( false );
}

static void LockAlloc()
{
	if( g_Mutex < 0 )
		InitAlloc();

	SceUID thid = sceKernelGetThreadId();

	if( thid != g_MutexOwner )
	{
		sceKernelWaitSema( g_Mutex, 1, NULL );
		g_MutexOwner = thid;
	}
	else
	{
		g_MutexCount++;
	}
}

static void UnlockAlloc()
{
	if( g_MutexCount != 0 )
	{
		g_MutexCount--;
	}
	else
	{
		g_MutexOwner = -1;
		sceKernelSignalSema( g_Mutex, 1 );
	}
}

static bool IsSingleLocked()
{
	return g_MutexCount == 0;
}

static void MemoryError()
{
	pspIoLock();
	pspIoCloseAllAlive();

	sceKernelExitGame();
}

void* __wrap_malloc( size_t size )
{
	LockAlloc();

	void *ptr = __real_malloc( size );
	if( ptr == NULL && size != 0 && IsSingleLocked() && g_bMemoryAssert )
	{
		TempLog( "malloc failed: %ubytes\r\n", size );
		MemoryError();
	}

	UnlockAlloc();
	return ptr;
}

void* __wrap_realloc( void *ptr, size_t size )
{
	LockAlloc();

	void *new_ptr = __real_realloc( ptr, size );
	if( new_ptr == NULL && size != 0 && IsSingleLocked() && g_bMemoryAssert )
	{
		TempLog( "realloc failed: %p %ubytes\r\n", ptr, size );
		MemoryError();
	}

	UnlockAlloc();
	return new_ptr;
}

void *__wrap_calloc( size_t n, size_t size )
{
	LockAlloc();

	void *ptr = __real_calloc( n, size );
	if( ptr == NULL && n != 0 && size != 0 && IsSingleLocked() && g_bMemoryAssert )
	{
		TempLog( "calloc failed: %ublocks %ubytes\r\n", n, size );
		MemoryError();
	}

	UnlockAlloc();
	return ptr;
}

void* __wrap_memalign( size_t align, size_t size )
{
	LockAlloc();

	void *ptr = __real_memalign( align, size );
	if( ptr == NULL && size > 0 && IsSingleLocked() && g_bMemoryAssert )
	{
		TempLog( "memalign failed: %ubytes(aligned %u)\r\n", size, align );
		MemoryError();
	}

	UnlockAlloc();
	return ptr;
}

void __wrap_free( void *ptr )
{
	LockAlloc();

	__real_free( ptr );

	UnlockAlloc();
}

size_t GetMemoryTotalSize()
{
	if( g_Mutex < 0 )
		InitAlloc();

	return g_TotalMemory;
}

size_t GetMemoryFreeSize( bool fast )
{
	MemoryList *memTop = NULL, *memPrev = NULL;
	size_t freeSize = 0, bufSize = 20*1024*1024;
	size_t minSize = (fast ? 1024 : 2);

	LockAlloc();

	while( 1 )
	{
		MemoryList *memCurr = (MemoryList*)malloc( sizeof(MemoryList) );
		if( memCurr == NULL )
			break;

		memCurr->buf = malloc( bufSize );
		if( memCurr->buf == NULL )
		{
			free( memCurr );
			freeSize += sizeof(MemoryList);

			if( bufSize <= minSize )
				break;

			bufSize /= 2;
			continue;
		}

		freeSize += bufSize;

		if( memPrev == NULL )
			memTop = memCurr;
		else
			memPrev->next = memCurr;

		memCurr->next = NULL;
		memPrev = memCurr;
	}

	MemoryList *memCurr = memTop;
	while( memCurr != NULL )
	{
		MemoryList *memNext = memCurr->next;
		free( memCurr->buf );
		free( memCurr );
		memCurr = memNext;
	}

	UnlockAlloc();

	return freeSize;
}

void SetMemoryAssert( bool b )
{
	LockAlloc();
	g_bMemoryAssert = b;
	UnlockAlloc();
}
