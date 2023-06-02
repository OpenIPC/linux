#ifndef __ISF_VDOPRC_API_H
#define __ISF_VDOPRC_API_H

#include <kwrap/type.h>

#if defined(__LINUX)
int vdoprc_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
