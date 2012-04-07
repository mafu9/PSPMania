/*
 * The system of PSP has limit at file max open. It may be 10 files.
 * This code enables to break the limit pseudoly, but cannot use async file operations.
 */

#include "global.h"
#include "ProductInfo.h"
#include "StepMania.h"
#include "RageThreads.h"
#include "RageLog.h"
#include "RageUtil.h"

#define MAX_FILE_HANDLE 32
#define MAX_FILE_OPEN 6

struct PspIoHandle
{
	bool bUsed;
	char file[PATH_MAX];
	int flags;
	SceMode mode;
	int offset;
	int fileSize;

	PspIoHandle() { bUsed = false; }
};

class PspIoAlive
{
public:
	PspIoAlive();
	~PspIoAlive() { Clear(); }

	void PushBack( int index, SceUID fd );
	void Erase( int index );
	void Clear();
	SceUID FindFd( int index ) const;

private:
	int back;

	struct Data
	{
		int index;
		SceUID fd;
	}data[MAX_FILE_OPEN];
};

static RageMutex g_IoMutex("IoFile");
static PspIoHandle g_IoHandles[MAX_FILE_HANDLE];
static PspIoAlive g_IoAlive;
static CString g_CurrentDir;

PspIoAlive::PspIoAlive()
{
	back = 0;
	for( int i = 0; i < MAX_FILE_OPEN; ++i )
	{
		data[i].index = -1;
		data[i].fd = -1;
	}
}

void PspIoAlive::PushBack( int index, SceUID fd )
{
	if( data[back].fd >= 0 )
		sceIoClose( data[back].fd );

	data[back].index = index;
	data[back].fd = fd;

	++back;
	if( back >= MAX_FILE_OPEN )
		back = 0;
}

void PspIoAlive::Erase( int index )
{
	for( int i = 0; i < MAX_FILE_OPEN; ++i )
	{
		if( data[i].index == index )
		{
			if( data[i].fd >= 0 )
				sceIoClose( data[i].fd );

			data[i].index = -1;
			data[i].fd = -1;
			return;
		}
	}
}

void PspIoAlive::Clear()
{
	for( int i = 0; i < MAX_FILE_OPEN; ++i )
	{
		if( data[i].fd >= 0 )
			sceIoClose( data[i].fd );

		data[i].index = -1;
		data[i].fd = -1;
	}
}

SceUID PspIoAlive::FindFd( int index ) const
{
	for( int i = 0; i < MAX_FILE_OPEN; ++i )
	{
		if( data[i].index == index )
			return data[i].fd;
	}

	return -1;
}

static PspIoHandle *pspIoReopen( int index, SceUID &fd )
{
	PspIoHandle *io = &g_IoHandles[index];
	if( !io->bUsed )
	{
		fd = -1;
		return NULL;
	}

	fd = g_IoAlive.FindFd( index );
	if( fd < 0 )
	{
		fd = sceIoOpen( io->file, io->flags, io->mode );
		if( fd >= 0 )
		{
			if( io->offset > 0 )
				sceIoLseek32( fd, io->offset, PSP_SEEK_SET );

			g_IoAlive.PushBack( index, fd );
		}
	}

	return io;
}

void pspIoCloseAllAlive()
{
	g_IoMutex.Lock();
	g_IoAlive.Clear();
	g_IoMutex.Unlock();
}

void pspGetDevice( char *buf, int len )
{
	if( len <= 0 || g_argv == NULL )
		return;

	const char *firstSlash = strchr( g_argv[0], '/' );
	if( firstSlash == NULL )
	{
		buf[0] = '\0';
		return;
	}

	len = min(len,  firstSlash - g_argv[0] + 1);
	strncpy( buf, g_argv[0], len );
	buf[len - 1] = '\0';
}

void pspGetCwd( char *buf, int len )
{
	if( len <= 0 )
		return;

	g_IoMutex.Lock();

	if( g_CurrentDir.empty() )
	{
		g_IoMutex.Unlock();
		buf[0] = '\0';
		return;
	}

	strncpy( buf, g_CurrentDir, len );
	g_IoMutex.Unlock();

	buf[len - 1] = '\0';
}

int pspIoChdir( const char *path )
{
	LockMut( g_IoMutex );

	if( path == NULL )
	{
		if( sceIoChdir( g_CurrentDir ) >= 0 )
			return 0;
	}
	else if( sceIoChdir( path ) >= 0 )
	{
		g_CurrentDir = path;
		return 0;
	}

	return -1;
}

int pspIoOpen( const char *file, int flags, SceMode mode )
{
	LockMut( g_IoMutex );

	SceUID fd = sceIoOpen( file, flags, mode );
	if( fd < 0 )
		return -1;

	// serch empty
	int index;
	for( index = 0; index < MAX_FILE_HANDLE; ++index )
	{
		if( !g_IoHandles[index].bUsed )
			break;
	}

	// not empty
	if( index == MAX_FILE_HANDLE )
		return -1;

	PspIoHandle *io = &g_IoHandles[index];
	io->bUsed = true;
	if( strlen( file ) < 4 || file[3] != ':' ) // fix to full path
	{
		snprintf( io->file, PATH_MAX, "%s/%s", g_CurrentDir.c_str(), file );
	}
	else
	{
		strncpy( io->file, file, PATH_MAX );
		io->file[PATH_MAX-1] = '\0';
	}
	io->flags = flags;
	io->mode = mode;
	io->offset = 0;
	io->fileSize = sceIoLseek32( fd, 0, PSP_SEEK_END );
	sceIoLseek32( fd, 0, PSP_SEEK_SET );

	g_IoAlive.PushBack( index, fd );
	return index;
}

int pspIoRead( int handle, void *data, SceSize size )
{
	if( handle < 0 || handle >= MAX_FILE_HANDLE )
		return -1;

	g_IoMutex.Lock();

	SceUID fd;
	PspIoHandle *io = pspIoReopen( handle, fd );
	if( fd < 0 )
	{
		g_IoMutex.Unlock();
		return -1;
	}

	int bytes = sceIoRead( fd, data, size );
	io->offset += bytes;

	g_IoMutex.Unlock();
	return bytes;
}

int pspIoWrite( int handle, const void *data, SceSize size )
{
	if( handle < 0 || handle >= MAX_FILE_HANDLE )
		return -1;

	g_IoMutex.Lock();

	SceUID fd;
	PspIoHandle *io = pspIoReopen( handle, fd );
	if( fd < 0 )
	{
		g_IoMutex.Unlock();
		return -1;
	}

	int bytes = sceIoWrite( fd, data, size );
	io->offset += bytes;

	// update file size
	if( io->offset > io->fileSize )
		io->fileSize = io->offset;

	g_IoMutex.Unlock();
	return bytes;
}

int pspIoLseek32( int handle, int offset, int whence )
{
	if( handle < 0 || handle >= MAX_FILE_HANDLE )
		return -1;

	g_IoMutex.Lock();

	PspIoHandle *io = &g_IoHandles[handle];
	if( !io->bUsed )
	{
		g_IoMutex.Unlock();
		return -1;
	}

	switch( whence )
	{
	case PSP_SEEK_SET:
		break;

	case PSP_SEEK_CUR:
		offset += io->offset;
		break;

	case PSP_SEEK_END:
		offset += io->fileSize;
		break;
	}

	if( offset < 0 )
		offset = 0;
	else if( offset > io->fileSize )
		offset = io->fileSize;

	// changed offset
	if( offset != io->offset )
	{
		SceUID fd = g_IoAlive.FindFd( handle );

		// seek if really opened
		if( fd >= 0 )
			sceIoLseek32( fd, offset, PSP_SEEK_SET );

		io->offset = offset;
	}

	g_IoMutex.Unlock();
	return offset;
}

int pspIoClose( int handle )
{
	if( handle < 0 || handle >= MAX_FILE_HANDLE )
		return -1;

	g_IoMutex.Lock();

	g_IoHandles[handle].bUsed = false;
	g_IoAlive.Erase( handle );

	g_IoMutex.Unlock();
	return 0;
}

void pspIoLock()
{
	g_IoMutex.Lock();
}

void pspIoUnlock()
{
	g_IoMutex.Unlock();
}

void TempLog( const char *fmt, ...)
{
	static RageMutex mut("TempLog");
	LockMut( mut );

	char device[32];
	pspGetDevice( device, sizeof(device) );

	char logFile[PATH_MAX];
	snprintf( logFile, PATH_MAX, "%s/" PRODUCT_NAME ".txt", device );

	SceUID fd = sceIoOpen( logFile, PSP_O_WRONLY|PSP_O_APPEND|PSP_O_CREAT, 0777 );
	if( fd < 0 )
		return;

	char buf[256];

	va_list ap;
	va_start( ap, fmt );
	vsnprintf( buf, sizeof(buf), fmt, ap );
	va_end( ap );

	sceIoWrite( fd, buf, strlen( buf ) );
	sceIoClose( fd );
}