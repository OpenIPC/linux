
#ifndef _MFE6_DEF_H_
#define _MFE6_DEF_H_

typedef unsigned char       uchar;
typedef unsigned short      ushort;
typedef unsigned int        uint;

typedef unsigned char       uint8;
typedef unsigned short      uint16;
typedef unsigned int        uint32;
typedef unsigned long long  uint64;
typedef   signed char       int8;
typedef   signed short      int16;
typedef   signed int        int32;
typedef   signed long long  int64;

/* macros */
#define MIN(a,b)    ((a)>(b)?(b):(a))
#define MAX(a,b)    ((a)<(b)?(b):(a))

#define MST_ASSERT(p)

/* mem operators (allc/free) */
#if defined(LINUX_KERNEL)
#include <linux/kernel.h>
#include <linux/slab.h>
#define MEM_ALLC(l)         kzalloc(l,GFP_KERNEL)
#define MEM_FREE(p)         kfree(p)
#elif defined(REALTIME_OS)
#error "REALTIME_OS not implement yet"
#else /* Non-OS */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define EXPORT_SYMBOL(sym)
#define MEM_ALLC(l)         malloc(l)
#define MEM_FREE(p)         free(p)
#endif
#define MEM_SETV(p,v,l)     memset(p,v,l)
#define MEM_COPY(p,q,l)     memcpy(p,q,l)

/* Debug related configuration */
#define DBG_PRINT_ENABLE        0
/* Debugging related macro */
#if DBG_PRINT_ENABLE
#define MFE_DBG        printk
#else
#define MFE_DBG(...)
#endif

//#define MMFE_REGS_TRACE
#define MMFE_RCTL_PATCH
#define MMFE_IMI_DBF_ADDR  0x00000 /* From 0x00000 to 0x03FFF */
#define MMFE_IMI_LBW_ADDR  0x04000 /* From 0x04000 to 0x13FFF */

#endif/*_MFE6_DEF_H_*/
