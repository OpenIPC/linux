/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/string.h>
#include <linux/version.h>
#include "osal_lib_ax.h"

char *AX_OSAL_LIB_strcpy(char * dest, const char * src)
{
	return strcpy(dest, src);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strcpy);

char *AX_OSAL_LIB_strncpy(char * dest, const char * src, int count)
{
	return strncpy(dest, src, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strncpy);

int AX_OSAL_LIB_strlcpy(char * dest, const char * src, int count)
{
	return strlcpy(dest, src, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strlcpy);

char *AX_OSAL_LIB_strcat(char * dest, const char * src)
{
	return strcat(dest, src);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strcat);

char *AX_OSAL_LIB_strncat(char * dest, const char * src, int count)
{
	return strncat(dest, src, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strncat);

int AX_OSAL_LIB_strlcat(char * dest, const char * src, int count)
{
	return strlcat(dest, src, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strlcat);

int AX_OSAL_LIB_strcmp(const char * cs, const char * ct)
{
	return strcmp(cs, ct);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strcmp);

int AX_OSAL_LIB_strncmp(const char * cs, const char * ct, int count)
{
	return strncmp(cs, ct, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strncmp);

int AX_OSAL_LIB_strnicmp(const char * s1, const char * s2, int len)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_LIB_strnicmp);

int AX_OSAL_LIB_strcasecmp(const char * s1, const char * s2)
{
	return strcasecmp(s1, s2);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strcasecmp);

int AX_OSAL_LIB_strncasecmp(const char * s1, const char * s2, int len)
{
	return strncasecmp(s1, s2, len);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strncasecmp);

char *AX_OSAL_LIB_strchr(const char * s, int c)
{
	return strchr(s, c);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strchr);

char *AX_OSAL_LIB_strnchr(const char * s, int count, int c)
{
	return strnchr(s, count, c);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strnchr);

char *AX_OSAL_LIB_strrchr(const char * s, int c)
{
	return strrchr(s, c);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strrchr);

char *AX_OSAL_LIB_strstr(const char * s1, const char * s2)
{
	return strstr(s1, s2);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strstr);

char *AX_OSAL_LIB_strnstr(const char * s1, const char * s2, int len)
{
	return strnstr(s1, s2, len);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strnstr);

int AX_OSAL_LIB_strlen(const char * s)
{
	return strlen(s);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strlen);

int AX_OSAL_LIB_strnlen(const char * s, int count)
{
	return strnlen(s, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strnlen);

char *AX_OSAL_LIB_strpbrk(const char * cs, const char * ct)
{
	return strpbrk(cs, ct);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strpbrk);

char *AX_OSAL_LIB_strsep(const char ** s, const char * ct)
{
	return strsep((char **) s, ct);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strsep);

int AX_OSAL_LIB_strspn(const char * s, const char * accept)
{
	return strspn((char *) s, accept);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strspn);

int AX_OSAL_LIB_strcspn(const char * s, const char * reject)
{
	return strcspn((char *) s, reject);
}

EXPORT_SYMBOL(AX_OSAL_LIB_strcspn);

void *AX_OSAL_LIB_memset(void * str, int c, int count)
{
	return memset(str, c, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_memset);

void *AX_OSAL_LIB_memmove(void * dest, const void * src, int count)
{
	return memmove(dest, src, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_memmove);

void *AX_OSAL_LIB_memscan(void * addr, int c, int size)
{
	return memscan(addr, c, size);
}

EXPORT_SYMBOL(AX_OSAL_LIB_memscan);

void *AX_OSAL_LIB_memcpy(void * ct, const void * cs, int count)
{
	return memcpy(ct, cs, count);
}

EXPORT_SYMBOL(AX_OSAL_LIB_memcpy);

void *AX_OSAL_LIB_memchr(const void * s, int c, int n)
{
	return memchr(s, c, n);
}

EXPORT_SYMBOL(AX_OSAL_LIB_memchr);

void *AX_OSAL_LIB_memchar_inv(const void * start, int c, int bytes)
{
	return memchr_inv(start, c, bytes);
}

EXPORT_SYMBOL(AX_OSAL_LIB_memchar_inv);

unsigned long AX_OSAL_LIB_simple_strtoull(const char * cp, char ** endp, unsigned int base)
{
	return simple_strtoull(cp, (char **) endp, base);
}

EXPORT_SYMBOL(AX_OSAL_LIB_simple_strtoull);

unsigned long AX_OSAL_LIB_simple_strtoul(const char * cp, char ** endp, unsigned int base)
{
	return simple_strtoul(cp, (char **) endp, base);
}

EXPORT_SYMBOL(AX_OSAL_LIB_simple_strtoul);

long AX_OSAL_LIB_simple_strtol(const char * cp, char ** endp, unsigned int base)
{
	return simple_strtol(cp, (char **) endp, base);
}

EXPORT_SYMBOL(AX_OSAL_LIB_simple_strtol);

s64 AX_OSAL_LIB_simple_strtoll(const char * cp, char ** endp, unsigned int base)
{
	return simple_strtoll(cp, (char **) endp, base);
}

EXPORT_SYMBOL(AX_OSAL_LIB_simple_strtoll);

int AX_OSAL_LIB_snprintf(char * buf, int size, const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);

	return i;
}

EXPORT_SYMBOL(AX_OSAL_LIB_snprintf);

int AX_OSAL_LIB_scnprintf(char * buf, int size, const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vscnprintf(buf, size, fmt, args);
	va_end(args);

	return i;
}

EXPORT_SYMBOL(AX_OSAL_LIB_scnprintf);

int AX_OSAL_LIB_sprintf(char * buf, const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, INT_MAX, fmt, args);
	va_end(args);

	return i;
}

EXPORT_SYMBOL(AX_OSAL_LIB_sprintf);

int AX_OSAL_LIB_vsscanf(char * buf, const char * fmt, ...)
{
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsscanf(buf, fmt, args);
	va_end(args);

	return i;
}

EXPORT_SYMBOL(AX_OSAL_LIB_vsscanf);

int AX_OSAL_LIB_vsnprintf(char * str, int size, const char * fmt, AX_VA_LIST args)
{
	return vsnprintf(str, size, fmt, args);
}

EXPORT_SYMBOL(AX_OSAL_LIB_vsnprintf);
