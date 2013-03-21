#ifndef MEMORY_H
#define MEMORY_H

#include <psptypes.h>

size_t GetMemoryTotalSize();
size_t GetMemoryFreeSize( bool fast );
void SetMemoryAssert( bool b );

#endif