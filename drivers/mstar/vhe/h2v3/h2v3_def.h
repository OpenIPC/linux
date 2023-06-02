
#ifndef _VHE_DEFS_H_
#define _VHE_DEFS_H_

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

//#define MVHE_PRINT(fmt,args...) printk("[H2v3]" fmt, ##args)
#ifndef MVHE_PRINT
#define MVHE_PRINT(fmt,args...)
#endif

/* math-utl */
typedef struct iir_t {
#define KAPA_ONE        1024
    int i_kapa;
    int i_data;
} iir_t;
inline static int iir_init(iir_t* iir, int data, int kapa)
{
    iir->i_kapa = kapa;
    iir->i_data = data;
    return iir->i_data;
}
inline static int iir_push(iir_t* iir, int data)
{
    int64 calc = (int64)data * iir->i_kapa;
    data = !iir->i_data ? data:iir->i_data;
    calc += (int64)data * (KAPA_ONE-iir->i_kapa);
    iir->i_data = (int)div_s64(calc,KAPA_ONE);
    return iir->i_data;
}
inline static int iir_data(iir_t* iir)
{
    return iir->i_data;
}

typedef struct acc_t {
#define ACC_SIZE    (1<<8)
#define ACC_MASK    (ACC_SIZE-1)
    int i_curr;
    int i_data[ACC_SIZE];
} acc_t;
inline static int acc_init(acc_t* acc, int data)
{
    int i = ACC_SIZE;
    while (i-- > 0)
        acc->i_data[i] = -i*data;
    acc->i_curr = 0;
    return data;
}
inline static int acc_push(acc_t* acc, int data)
{
    int v = data + acc->i_data[ACC_MASK&(acc->i_curr)];
    acc->i_curr--;
    acc->i_data[ACC_MASK&(acc->i_curr)] = v;
    return data;
}
inline static int acc_calc(acc_t* acc, int size)
{
    int lead = acc->i_data[ACC_MASK&(acc->i_curr)];
    int tail = acc->i_data[ACC_MASK&(acc->i_curr+size)];
    return (lead-tail);
}

#endif//_VHE_DEFS_H_
