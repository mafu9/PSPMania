#ifndef FILE_H
#define FILE_H

#include <pspkerneltypes.h>

void pspGetDevice( char *buf, int len );
void pspGetCwd( char *buf, int len );
int pspIoChdir( const char *path );
int pspIoOpen( const char *file, int flags, SceMode mode );
int pspIoRead( int handle, void *data, SceSize size );
int pspIoWrite( int handle, const void *data, SceSize size );
int pspIoLseek32( int handle, int offset, int whence );
int pspIoClose( int handle );
void pspIoCloseAllAlive();
void pspIoLock();
void pspIoUnlock();
void TempLog( const char *fmt, ...);

#endif