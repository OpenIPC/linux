
#ifndef _VHE_DEFS_H_
#define _VHE_DEFS_H_

#include <linux/kernel.h>
#include <linux/slab.h>

typedef unsigned long long  ulonglong;
typedef unsigned long       ulong;
typedef unsigned int        uint32, uint;
typedef unsigned short      uint16, ushort;
typedef unsigned char       uint8, uchar;
typedef   signed long long  int64;
typedef   signed int        int32;
typedef   signed short      int16;
typedef   signed char       int8;

/* macros */
#define MIN(a,b)    ((a)>(b)?(b):(a))
#define MAX(a,b)    ((a)<(b)?(b):(a))

/* memory manager operators (allc/free) */
#define MEM_ALLC(l)         kzalloc(l,GFP_KERNEL)
#define MEM_FREE(p)         kfree(p)
#define MEM_SETV(p,v,l)     memset(p,v,l)
#define MEM_COPY(p,q,l)     memcpy(p,q,l)

//#define MVHE_PRINT(fmt,args...) printk("[MVHE]" fmt, ##args)
#ifndef MVHE_PRINT
#define MVHE_PRINT(fmt,args...)
#endif

#endif//_VHE_DEFS_H_
