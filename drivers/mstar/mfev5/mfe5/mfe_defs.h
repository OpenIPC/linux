
#ifndef _MFE_DEFS_H_
#define _MFE_DEFS_H_

#include <linux/kernel.h>
#include <linux/slab.h>

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

#define MST_ASSERT(p)

/* mem operators (allc/free) */
#define MEM_ALLC(l)         kzalloc(l,GFP_KERNEL)
#define MEM_FREE(p)         kfree(p)
#define MEM_SETV(p,v,l)     memset(p,v,l)
#define MEM_COPY(p,q,l)     memcpy(p,q,l)

//#define MMFE_PRINT(fmt,args...) printk("[MMFE]" fmt, ##args)
#ifndef MMFE_PRINT
#define MMFE_PRINT(fmt,args...)
#endif

//#define MMFE_REGS_TRACE
#define MMFE_RCTL_PATCH
#define MMFE_IMI_LBW_ADDR  0x00000 /* From 0x00000 to 0x0FFFF */
#define MMFE_IMI_DBF_ADDR  0x10000 /* From 0x10000 to 0x13FFF */

#endif/*_MFE_DEFS_H_*/
