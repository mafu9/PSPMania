/* Internal helper functions for RageFileDriverDirect. */

#ifndef RAGE_FILE_DRIVER_DIRECT_HELPERS_H
#define RAGE_FILE_DRIVER_DIRECT_HELPERS_H

#include <fcntl.h>

#if defined(_XBOX)
int DoMkdir( const CString &sPath, int perm );
int DoOpen( const CString &sPath, int flags, int perm );
int DoStat( const CString &sPath, struct stat *st );
int DoRemove( const CString &sPath );
int DoRmdir( const CString &sPath );
HANDLE DoFindFirstFile( const CString &sPath, WIN32_FIND_DATA *fd );
#else
#define DoOpen open
#define DoStat stat
#define DoMkdir mkdir
#define DoFindFirstFile FindFirstFile
#define DoRemove remove
#define DoRmdir rmdir
#endif

#ifdef PSP
#define chdir( path )					pspIoChdir( path )
#define open( fname, flags, ... )		pspIoOpen( fname, flags, 0777 )
#define close( handle )					pspIoClose( handle )
#define read( handle, data, size )		pspIoRead( handle, data, size )
#define write( handle, data, size )		pspIoWrite( handle, data, size )
#define lseek( handle, pos, whence )	pspIoLseek32( handle, pos, whence )
#define tell( handle )					pspIoLseek32( handle, 0, PSP_SEEK_CUR )
#define fsync( handle )					( 0 )

#ifdef O_RDONLY
#undef O_RDONLY
#endif
#ifdef O_WRONLY
#undef O_WRONLY
#endif
#ifdef O_RDWR
#undef O_RDWR
#endif
#ifdef O_APPEND
#undef O_APPEND
#endif
#ifdef O_CREAT
#undef O_CREAT
#endif
#ifdef O_TRUNC
#undef O_TRUNC
#endif
#ifdef O_EXCL
#undef O_EXCL
#endif
#ifdef O_SYNC
#undef O_SYNC
#endif
#ifdef O_NONBLOCK
#undef O_NONBLOCK
#endif
#ifdef O_NOCTTY
#undef O_NOCTTY
#endif
#ifdef O_BINARY
#undef O_BINARY
#endif
#ifdef O_TEXT
#undef O_TEXT
#endif
#ifdef O_NOINHERIT
#undef O_NOINHERIT
#endif

#define O_RDONLY PSP_O_RDONLY
#define O_WRONLY PSP_O_WRONLY
#define O_RDWR PSP_O_RDWR
#define O_APPEND PSP_O_APPEND
#define O_CREAT PSP_O_CREAT
#define O_TRUNC PSP_O_TRUNC
#define	O_EXCL PSP_O_EXCL
#define O_SYNC 0
#define O_NONBLOCK PSP_O_NBLOCK
#define O_NOCTTY 0
#define O_BINARY 0
#define O_TEXT 0
#define O_NOINHERIT 0
#endif

#if defined(WIN32)
int WinMoveFile( CString sOldPath, CString sNewPath );
#endif

#if !defined(O_BINARY)
#define O_BINARY 0
#endif

bool CreateDirectories( const CString &Path );
bool PathReady( const CString &path );

#include "RageUtil_FileDB.h"
class DirectFilenameDB: public FilenameDB
{
protected:
	virtual void PopulateFileSet( FileSet &fs, const CString &sPath );
	CString root;

public:
	DirectFilenameDB( const CString &root_ );
};

#endif

/*
 * Copyright (c) 2003-2004 Glenn Maynard, Chris Danford
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
