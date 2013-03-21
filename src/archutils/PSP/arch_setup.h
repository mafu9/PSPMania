#ifndef ARCH_SETUP_PSP_H
#define ARCH_SETUP_PSP_H

#define PATH_MAX 256

#include <pspsdk.h>
#include <pspkernel.h>
#include <psppower.h>
#include <psputils.h>
#include <psputility.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>
#include <wchar.h>

#include "File.h"
#include "Memory.h"
#include <fastmath.h>

#define ENDIAN_LITTLE
#define NEED_STRTOF

struct tm *my_localtime_r( const time_t *timep, struct tm *result );
#define localtime_r my_localtime_r
struct tm *my_gmtime_r( const time_t *timep, struct tm *result );
#define gmtime_r my_gmtime_r

#define atoi(x) (int)strtol( x, NULL, 10 )
#define usleep(x) sceKernelDelayThread( x )

#if 0
#define HAVE_BYTE_SWAPS

inline uint32_t ArchSwap32( uint32_t n )
{
}

inline uint32_t ArchSwap24( uint32_t n )
{
}

inline uint16_t ArchSwap16( uint16_t n )
{
}
#endif


#endif

