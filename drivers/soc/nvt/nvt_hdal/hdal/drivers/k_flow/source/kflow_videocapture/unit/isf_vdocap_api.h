#ifndef __ISF_VDOCAP_API_H
#define __ISF_VDOCAP_API_H

#include <kwrap/type.h>

#if defined(__LINUX)
int vdocap_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
