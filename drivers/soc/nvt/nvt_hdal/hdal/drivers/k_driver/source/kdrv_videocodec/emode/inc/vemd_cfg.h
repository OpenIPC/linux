#ifndef _VEMD_CFG_H_
#define _VEMD_CFG_H_

#include "kwrap/type.h"

#if defined(__LINUX)
#elif defined(__FREERTOS)
#endif

int vemd_parse_cfg(char *buf, size_t buf_size);
int vemd_gen_cfg(char *buf, size_t buf_size);

#endif // _VEMD_CFG_H_
