#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#define _ALIGNED(x) __attribute__((aligned(x)))
#define _PACKED_BEGIN
#define _PACKED_END __attribute__ ((packed))
#define _INLINE static inline
#define _ASM_NOP __asm__("nop");
#define _SECTION(sec)

//BSP identifier
#if !defined(_BSP_NA51089_)
#define _BSP_NA51089_
#endif

//todo identifier
#define _TODO           0

#endif

