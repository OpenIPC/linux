#ifndef __ISF_VDOOUT_PLATFORM_H
#define __ISF_VDOOUT_PLATFORM_H
#include <kwrap/type.h>

#if defined(__LINUX)
void* vdoout_alloc(int size);
void vdoout_free(void *buf);
#endif

#endif


