#ifndef _VOS_LIBC_STDIO_H_
#define _VOS_LIBC_STDIO_H_
#include <stdarg.h>

extern int  sscanf_s(const char *buf, const char *fmt, ...);
extern int  vsscanf_s(const char *buf, const char *fmt, va_list args);

#endif
