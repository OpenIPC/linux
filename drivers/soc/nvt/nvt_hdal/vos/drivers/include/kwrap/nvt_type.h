#ifndef _VOS_NVT_TYPE_H_
#define _VOS_NVT_TYPE_H_

//#define VOS_LEGACY_ARCH32 0 //64-bit version
#define VOS_LEGACY_ARCH32 1 //32-bit version

#if defined(__KERNEL__)
#include <linux/types.h>
typedef s8                  INT8;           ///< Signed 8-bit integer
typedef s16                 INT16;          ///< Signed 16-bit integer
typedef s32                 INT32;          ///< Signed 32-bit integer
typedef s64                 INT64;          ///< Signed 64-bit integer
typedef u8                  UINT8;          ///< Unsigned 8-bit integer
typedef u16                 UINT16;         ///< Unsigned 16-bit integer
typedef u32                 UINT32;         ///< Unsigned 32-bit integer
typedef u64                 UINT64;         ///< Unsigned 64-bit integer
#else //defined(__KERNEL__)
#include <stdint.h>
typedef signed char         INT8;           ///< Signed 8-bit integer
typedef signed short        INT16;          ///< Signed 16-bit integer
#if VOS_LEGACY_ARCH32
typedef signed long         INT32;          ///< Signed 32-bit integer
#else
typedef signed int          INT32;          ///< Signed 32-bit integer
#endif
typedef signed long long    INT64;          ///< Signed 64-bit integer
typedef unsigned char       UINT8;          ///< Unsigned 8-bit integer
typedef unsigned short      UINT16;         ///< Unsigned 16-bit integer
#if VOS_LEGACY_ARCH32
typedef unsigned long       UINT32;         ///< Unsigned 32-bit integer
#else
typedef unsigned int        UINT32;         ///< Unsigned 32-bit integer
#endif
typedef unsigned long long  UINT64;         ///< Unsigned 64-bit integer
#endif

typedef INT8                B;              ///< Signed 8-bit integer
typedef INT16               H;              ///< Signed 16-bit integer
typedef INT32               W;              ///< Signed 32-bit integer
typedef UINT8               UB;             ///< Unsigned 8-bit integer
typedef UINT16              UH;             ///< Unsigned 16-bit integer
typedef UINT32              UW;             ///< Unsigned 32-bit integer
typedef INT8                VB;             ///< Variable data type (8 bits)
typedef INT16               VH;             ///< Variable data type (16 bits)
typedef INT32               VW;             ///< Variable data type (32 bits)

typedef void                *VP;            ///< Pointer to unspecified data type
typedef void                (*FP)(void);    ///< Program start address

typedef int                 ID;             ///< Object ID number
typedef int                 ER;             ///< Error code
typedef int                 PRI;            ///< Task priority
typedef unsigned int        ATR;            ///< Object attribute
typedef int                 TMO;            ///< Time-out

typedef void                VOID;
typedef char                CHAR;
typedef float               FLOAT;
typedef double              DOUBLE;
typedef unsigned int        UINT;
typedef int                 INT;
typedef unsigned int        BOOL;
typedef unsigned long       ULONG;

#ifndef DISABLE
#define DISABLE             0           ///< Feature is disabled
#endif

#ifndef ENABLE
#define ENABLE              1           ///< Feature is enabled
#endif

#ifndef FALSE
#define FALSE               0
#endif

#ifndef TRUE
#define TRUE                1
#endif

#ifndef NULL
#define NULL                ((void *)0)
#endif

/**
    Flag type definition

    Flag type definition
*/
#define FLGPTN_TYPE_32      0
#define FLGPTN_TYPE_64      1
#define FLGPTN_TYPE         FLGPTN_TYPE_32

#if (FLGPTN_TYPE == FLGPTN_TYPE_32)
// 32 bits flag patterns
typedef unsigned int        FLGPTN;                     ///< Flag patterns
typedef unsigned int        *PFLGPTN;                   ///< Flag patterns (Pointer)
#define FLGPTN_BIT(n)       ((FLGPTN)(1 << (n)))        ///< Bit of flag pattern
#define FLGPTN_BIT_ALL      ((FLGPTN)0xFFFFFFFF)
#elif (FLGPTN_TYPE == FLGPTN_TYPE_64)
// 64 bits flag patterns
typedef unsigned long long  FLGPTN;                     ///< Flag patterns
typedef unsigned long long  *PFLGPTN;                   ///< Flag patterns (Pointer)
#define FLGPTN_BIT(n)       ((FLGPTN)(1ull << (n)))     ///< Bit of flag pattern
#define FLGPTN_BIT_ALL      ((FLGPTN)0xFFFFFFFFFFFFFFFFull)
#else
#error Incorrect flag pattern type
#endif

/**
    @name Assert macros
*/
//@{
#define ASSERT_CONCAT_(a, b)    a##b
#define ASSERT_CONCAT(a, b)     ASSERT_CONCAT_(a, b)

#if defined(__COUNTER__)

#define STATIC_ASSERT(expr) \
enum { ASSERT_CONCAT(FAILED_STATIC_ASSERT_, __COUNTER__) = 1/(expr) }

#else

// This might cause compile error when writing STATIC_ASSERT at the same line
// in two (or more) files and one file include another one.
#define STATIC_ASSERT(expr) \
enum { ASSERT_CONCAT(FAILED_STATIC_ASSERT_, __LINE__) = 1/(expr) }

#endif

/**
    Macro to generate dummy element for enum type to expand enum size to word (4 bytes)
*/
//@{
#define ENUM_DUMMY4WORD(name)   E_##name = 0x10000000
//@}

/**
    Convert four characters into a four-character code.
*/
//@{
#define MAKEFOURCC(ch0, ch1, ch2, ch3) ((UINT32)(UINT8)(ch0) | ((UINT32)(UINT8)(ch1) << 8) | ((UINT32)(UINT8)(ch2) << 16) | ((UINT32)(UINT8)(ch3) << 24))
//@}

/**
    @name Align macros

    Align floor, align round, and align ceil

    @note base must be 2^n, where n = 1, 2, 3, ...
*/
//@{
#define ALIGN_FLOOR(value, base)  ((value) & ~((base)-1))                   ///< Align Floor
#define ALIGN_ROUND(value, base)  ALIGN_FLOOR((value) + ((base)/2), base)   ///< Align Round
#define ALIGN_CEIL(value, base)   ALIGN_FLOOR((value) + ((base)-1), base)   ///< Align Ceil

/**
    Information to create semaphore

    Information to create semaphore.
*/
typedef struct t_csem {
	VP      exinf;          ///< Extended information (Not referenced)
	ATR     sematr;         ///< Semaphore attribute (Not referenced)
	INT     isemcnt;        ///< Semaphore counter, but only support up to 255 now
	INT     maxsem;         ///< Semaphore Max counter value, but only support up to 255 now
} T_CSEM;

/**
    Information to create task

    Information to create task.
*/
typedef struct t_ctsk {
	VP      exinf;          ///< Extended information (Not referenced)
	ATR     tskatr;         ///< Task attributes (Not referenced)
	FP      task;           ///< Task start address
	PRI     itskpri;        ///< Initial task priority
	INT     stksz;          ///< Task stack size
	// Implementation-dependent information -begin
	VP      istack;         ///< Pointer to task stack
	// Implementation-dependent information -end
} T_CTSK;


struct  flg {
	volatile UB             f_stat;     /*  8 Event flags status               */
	UB                      f_type;     /*  9 Structure type                   */
	UH                      f_rsvd;     /* 10 Reserved (for alignment)         */
	FLGPTN                  f_flgs;     /* 12 Event flags                      */
	FLGPTN                  f_setptn;   /* 20 Held set-pattern                 */
	FLGPTN                  f_clrptn;   /* 24 Held clear-pattern               */
};

/**
    Information to create flag

    Information to create flag.
*/
typedef struct t_cflg {
	VP      exinf;          ///< Extended information (Not referenced)
	ATR     flgatr;         ///< Flag attribute (Not referenced)
	FLGPTN  iflgptn;        ///< Initial flag pattern
} T_CFLG;


typedef struct t_cmbx {
	VP      exinf;          ///< Extended information (Not referenced)
	ATR     mbxatr;         ///< mail box attribute (Not referenced)
} T_CMBX;


/**
    Information to create fixed memory pool

    Information to create fixed memory pool.
*/
typedef struct t_cmpf {
	VP      exinf;          ///< Extended information (Not referenced)
	ATR     mpfatr;         ///< Fixed memory pool attribute (Not referenced)
	INT     mpfcnt;         ///< Fixed memory pool block count
	INT     blfsz;          ///< Fixed memory pool block size
} T_CMPF;


/**
    Message type

    Message type.
*/
typedef UB         T_MSG;

#endif
