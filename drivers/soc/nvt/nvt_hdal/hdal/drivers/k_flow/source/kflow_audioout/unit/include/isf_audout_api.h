#ifndef __ISF_AUDOUT_API_H_
#define __ISF_AUDOUT_API_H_
#include <kwrap/type.h>


#if defined(__LINUX)
int audout_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
