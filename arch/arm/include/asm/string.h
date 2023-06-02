#ifndef __ASM_ARM_STRING_H
#define __ASM_ARM_STRING_H

/*
 * We don't do inline string functions, since the
 * optimised inline asm versions are not small.
 *
 * The __underscore versions of some functions are for KASan to be able
 * to replace them with instrumented versions.
 */

#define __HAVE_ARCH_STRRCHR
extern char * strrchr(const char * s, int c);

#define __HAVE_ARCH_STRCHR
extern char * strchr(const char * s, int c);

#define __HAVE_ARCH_MEMCPY
extern void * memcpy(void *, const void *, __kernel_size_t);
extern void *__memcpy(void *dest, const void *src, __kernel_size_t n);

#define __HAVE_ARCH_MEMMOVE
extern void * memmove(void *, const void *, __kernel_size_t);
extern void *__memmove(void *dest, const void *src, __kernel_size_t n);

#define __HAVE_ARCH_MEMCHR
extern void * memchr(const void *, int, __kernel_size_t);

#define __HAVE_ARCH_MEMSET
extern void * memset(void *, int, __kernel_size_t);
extern void *__memset(void *s, int c, __kernel_size_t n);

/*
 * For files that are not instrumented (e.g. mm/slub.c) we
 * must use non-instrumented versions of the mem*
 * functions named __memcpy() etc. All such kernel code has
 * been tagged with KASAN_SANITIZE_file.o = n, which means
 * that the address sanitization argument isn't passed to the
 * compiler, and __SANITIZE_ADDRESS__ is not set. As a result
 * these defines kick in.
 */
#if defined(CONFIG_KASAN) && !defined(__SANITIZE_ADDRESS__)
#define memcpy(dst, src, len) __memcpy(dst, src, len)
#define memmove(dst, src, len) __memmove(dst, src, len)
#define memset(s, c, n) __memset(s, c, n)

#ifndef __NO_FORTIFY
#define __NO_FORTIFY /* FORTIFY_SOURCE uses __builtin_memcpy, etc. */
#endif

#endif

#endif
