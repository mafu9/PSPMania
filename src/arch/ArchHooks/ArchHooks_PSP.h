#ifndef ARCH_HOOKS_PSP_H
#define ARCH_HOOKS_PSP_H

#include "ArchHooks.h"
class ArchHooks_PSP: public ArchHooks
{
public:
    ArchHooks_PSP();
    ~ArchHooks_PSP();

	void DumpDebugInfo();
};

#undef ARCH_HOOKS
#define ARCH_HOOKS ArchHooks_PSP

#endif