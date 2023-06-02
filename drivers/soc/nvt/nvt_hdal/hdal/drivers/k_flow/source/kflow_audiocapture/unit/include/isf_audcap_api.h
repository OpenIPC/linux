#ifndef __ISF_AUDCAP_API_H_
#define __ISF_AUDCAP_API_H_
#include <kwrap/type.h>


#if defined(__LINUX)
int audcap_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
