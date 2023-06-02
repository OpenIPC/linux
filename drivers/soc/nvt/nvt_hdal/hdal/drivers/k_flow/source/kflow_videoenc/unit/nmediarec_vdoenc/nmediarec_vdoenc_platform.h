#ifndef _NMEDIAVDOENCPLATFORM_H
#define _NMEDIAVDOENCPLATFORM_H
#include <kwrap/type.h>

extern void NMR_VdoEnc_Lock_cpu(unsigned long *in_flags);
extern void NMR_VdoEnc_Unlock_cpu(unsigned long *in_flags);
extern void NMR_VdoEnc_Lock_cpu2(unsigned long *in_flags);
extern void NMR_VdoEnc_Unlock_cpu2(unsigned long *in_flags);
#endif


