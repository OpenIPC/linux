#ifndef _GTYPES_H_
#define _GTYPES_H_

#ifdef __LINUX__
#include <linux/slab.h>    /* kmalloc */
#include <linux/kernel.h>  /* printk */
#include <linux/wait.h>    /* wait queue */
#include <asm/uaccess.h>   /* copy user<->kernel */
#include <asm/page.h>      /* paddr<->vaddr */
#include <linux/semaphore.h> /* semaphore */
#include <linux/poll.h>    /* poll */
#include <linux/ioport.h>
#include <linux/spinlock.h>
#include <asm/io.h>
#include <linux/interrupt.h>
#include <mach/hardware.h>
#include <linux/synclink.h>
#include <linux/module.h>
#endif

/* necessary undefines */
#ifdef GFALSE
    #undef GFALSE
#endif
#ifdef GTRUE
    #undef GTRUE
#endif


/*!
*******************************************************************************
**
** \name GOKESDK Error Codes
**
*******************************************************************************
*/
/*@{*/
enum
{
    GD_OK = 0,                     //!< No error.
    GD_ERR_BAD_PARAMETER,          //!< Bad parameter passed.
    GD_ERR_OUT_OF_MEMORY,          //!< Memory allocation failed.
    GD_ERR_ALREADY_INITIALIZED,    //!< Device already initialised.
    GD_ERR_NOT_INITIALIZED,        //!< Device not initialised.
    GD_ERR_ALREADY_OPEN,           //!< Device could not be opened again.
    GD_ERR_FEATURE_NOT_SUPPORTED,  //!< Feature or function is not available.
    GD_ERR_TIMEOUT,                //!< Timeout occured.
    GD_ERR_DEVICE_BUSY,            //!< The device is busy.
    GD_ERR_INVALID_HANDLE,         //!< The handle is invalid.
};
/*@}*/


/*!
*******************************************************************************
**
** \name GOKESDK Data Types
**
*******************************************************************************
*/
/*@{*/
typedef unsigned char  U8;         //!< 8 bit unsigned integer.
typedef unsigned short U16;        //!< 16 bit unsigned integer.
#ifdef __LINUX__
typedef unsigned int   U32;        //!< 32 bit unsigned integer.
typedef signed int     S32;        //!< 32 bit signed integer.
#else
typedef unsigned long  U32;        //!< 32 bit unsigned integer.
typedef signed long    S32;        //!< 32 bit signed integer.
#endif
typedef signed char    S8;         //!< 8 bit signed integer.
typedef signed short   S16;        //!< 16 bit signed integer.
typedef unsigned long long   U64;        //!< 64 bit unsigned integer.

#ifdef __LINUX__
typedef int            GERR;
#else
typedef unsigned long  GERR;       //!< GOKESDK error/return code.
#endif
typedef unsigned long  GD_HANDLE;  //!< GOKESDK handle.

/*! \brief Defines a GOKESDK boolean. */
#ifndef GTYPES_NO_BOOL
typedef enum
{
    GFALSE = 0,  //!< Logical false.
    GTRUE  = 1   //!< Logical true.
} GBOOL;
#endif
/*@}*/




/*!
*******************************************************************************
**
** \brief Defines a GOKESDK null.
**
*******************************************************************************
*/
#ifndef NULL
    #define NULL 0
#endif


/*
*******************************************************************************
**
** Defines the keyword for inlining a function.
**
*******************************************************************************
*/

#define GH_INLINE_LEVEL 2

#if defined(__HIGHC__)
#define GH_INLINE _Inline
#endif
#if defined(__GNUC__)
#define GH_INLINE static __inline__
#endif

#ifdef GISR_IS_VOID
    #define GISR1 void             /* customize C-Interrupt handler */
    #define GISR2 void
#else
    #define GISR1 irqreturn_t
    #define GISR2 irqreturn_t
#endif

#endif