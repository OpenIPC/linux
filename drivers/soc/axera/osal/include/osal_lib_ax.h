/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_LIBRARY_AX__H__
#define __OSAL_LIBRARY_AX__H__

#ifdef __cplusplus
extern "C" {
#endif
#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
#include <linux/stdarg.h>
#else
#include <stdarg.h>
#endif
#include "osal_type_ax.h"
#include <linux/types.h>

#define __AX_OSAL_ALIGN_MASK(x, mask) (((x) + (mask)) & ~(mask))
#define __AX_OSAL_ALIGN(x, a) __AX_OSAL_ALIGN_MASK(x, (typeof(x))(a) - 1)
#define AX_OSAL_ALIGN(x, a) __AX_OSAL_ALIGN((x), (a))
#define AX_OSAL_ALIGN_DOWN(x, a) __AX_OSAL_ALIGN((x) - ((a) - 1), (a))
#define AX_OSAL_IS_ALIGNED(x, a) (((x) & ((typeof(x))(a) - 1)) == 0)

#define AX_OSAL_ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define AX_OSAL_abs(x) ((x < 0) ? -x : x)
#define AX_OSAL_min(x, y) ((x < 0) ? -x : x)
#define AX_OSAL_max(x, y) ((x < 0) ? -x : x)
#define AX_OSAL_clamp(val, lo, hi) AX_OSAL_min((typeof(val))AX_OSAL_max(val, lo), hi)

#define AX_OSAL_swap(a, b) { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; }

char *AX_OSAL_LIB_strcpy(char *dest, const char *src);
char *AX_OSAL_LIB_strncpy(char *dest, const char *src, int count);
int AX_OSAL_LIB_strlcpy(char *dest, const char *src, int count);
char *AX_OSAL_LIB_strcat(char *dest, const char *src);
char *AX_OSAL_LIB_strncat(char *dest, const char *src, int count);
int AX_OSAL_LIB_strlcat(char *dest, const char *src, int count);
int AX_OSAL_LIB_strcmp(const char *cs, const char *ct);
int AX_OSAL_LIB_strncmp(const char *cs, const char *ct, int count);
int AX_OSAL_LIB_strnicmp(const char *s1, const char *s2, int len);
int AX_OSAL_LIB_strcasecmp(const char *s1, const char *s2);
int AX_OSAL_LIB_strncasecmp(const char *s1, const char *s2, int len);
char *AX_OSAL_LIB_strchr(const char *s, int c);
char *AX_OSAL_LIB_strnchr(const char *s, int count, int c);
char *AX_OSAL_LIB_strrchr(const char *s, int c);
char *AX_OSAL_LIB_strstr(const char *s1, const char *s2);
char *AX_OSAL_LIB_strnstr(const char *s1, const char *s2, int len);
int AX_OSAL_LIB_strlen(const char *s);
int AX_OSAL_LIB_strnlen(const char *s, int count);
char *AX_OSAL_LIB_strpbrk(const char *cs, const char *ct);
char *AX_OSAL_LIB_strsep(const char **s, const char *ct);
int AX_OSAL_LIB_strspn(const char *s, const char *accept);
int AX_OSAL_LIB_strcspn(const char *s, const char *reject);

void *AX_OSAL_LIB_memset(void *str, int c, int count);
void *AX_OSAL_LIB_memmove(void *dest, const void *src, int count);
void *AX_OSAL_LIB_memscan(void *addr, int c, int seze);
void *AX_OSAL_LIB_memcpy(void *ct, const void *cs, int count);
void *AX_OSAL_LIB_memchr(const void *s, int c, int n);
void *AX_OSAL_LIB_memchar_inv(const void *start, int c, int bytes);

#define AX_VA_LIST va_list

unsigned long AX_OSAL_LIB_simple_strtoull(const char *cp, char **endp, unsigned int base);
unsigned long AX_OSAL_LIB_simple_strtoul(const char *cp,  char **endp,  unsigned int base);
long AX_OSAL_LIB_simple_strtol(const char *cp,  char **endp,  unsigned int base);
long long int AX_OSAL_LIB_simple_strtoll(const char *cp,  char **endp,  unsigned int base);
int AX_OSAL_LIB_snprintf(char *buf, int size, const char *fmt, ...);
int AX_OSAL_LIB_scnprintf(char *buf, int size, const char *fmt, ...);
int AX_OSAL_LIB_sprintf(char *buf, const char *fmt, ...);
int AX_OSAL_LIB_vsscanf(char *buf, const char *fmt, ...);
int AX_OSAL_LIB_vsnprintf(char *str, int size, const char *fmt, AX_VA_LIST args);



unsigned long long int AX_OSAL_LIB_div_u64(unsigned long long int dividend, unsigned int divisor);
long long int AX_OSAL_LIB_div_s64(long long int dividend, int divisor);
unsigned long long int AX_OSAL_LIB_div64_u64(unsigned long long int dividend, unsigned long long int divisor);
long long int AX_OSAL_LIB_LIB_div64_s64(long long int dividend, long long int divisor);
unsigned long long int AX_OSAL_LIB_div_u64_rem(unsigned long long int dividend, unsigned int divisor);
long long int AX_OSAL_LIB_div_s64_rem(long long int dividend, int divisor);
unsigned long long int AX_OSAL_LIB_div64_u64_rem(unsigned long long int dividend, unsigned long long int divisor);
unsigned int AX_OSAL_LIB_random(void);

void AX_OSAL_LIB_sort_r(void *base, size_t num, size_t size,cmp_r_func_t cmp_func,swap_func_t swap_func,const void *priv);
void AX_OSAL_LIB_sort(void *base, size_t num, size_t size,cmp_func_t cmp_func,swap_func_t swap_func);

#ifdef __cplusplus
}
#endif

#endif /*__OSAL_LIBRARY_AX__H__*/

