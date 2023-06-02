#ifndef __ISF_VDOOUT_API_H
#define __ISF_VDOOUT_API_H
#include <kwrap/type.h>

#if defined(__LINUX)
int vdoout_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
