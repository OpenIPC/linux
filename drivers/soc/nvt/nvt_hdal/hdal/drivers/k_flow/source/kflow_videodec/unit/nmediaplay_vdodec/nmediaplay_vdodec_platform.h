#ifndef _NMEDIAVDODECPLATFORM_H
#define _NMEDIAVDODECPLATFORM_H
#include <kwrap/type.h>

extern void NMP_VdoDec_Lock_cpu(unsigned long *in_flags);
extern void NMP_VdoDec_Unlock_cpu(unsigned long *in_flags);
#if defined(__LINUX)
void* NMP_VdoDec_Alloc(int size);
void NMP_VdoDec_Free(void *buf);
#endif

#endif


