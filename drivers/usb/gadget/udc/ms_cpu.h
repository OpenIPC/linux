//------------------------------------------------------------------------------
//
// Copyright (c) 2008 MStar Semiconductor, Inc.  All rights reserved.
//
//------------------------------------------------------------------------------
// FILE
//      ms_cpu.h
//
// DESCRIPTION
//
// HISTORY
//
//------------------------------------------------------------------------------
#ifndef _MS_CPU_H_
#define _MS_CPU_H_

#ifdef __cplusplus
extern "C" {
#endif  //__cplusplus
//#include "mach/platform.h"

#define OTG_BIT0                    0x00000001
#define OTG_BIT1                    0x00000002
#define OTG_BIT2                    0x00000004
#define OTG_BIT3                    0x00000008
#define OTG_BIT4                    0x00000010
#define OTG_BIT5                    0x00000020
#define OTG_BIT6                    0x00000040
#define OTG_BIT7                    0x00000080
#define OTG_BIT8                    0x00000100
#define OTG_BIT9                    0x00000200
#define OTG_BIT10                   0x00000400
#define OTG_BIT11                   0x00000800
#define OTG_BIT12                   0x00001000
#define OTG_BIT13                   0x00002000
#define OTG_BIT14                   0x00004000
#define OTG_BIT15                   0x00008000
#define OTG_BIT16                   0x00010000
#define OTG_BIT17                   0x00020000
#define OTG_BIT18                   0x00040000
#define OTG_BIT19                   0x00080000
#define OTG_BIT20                   0x00100000
#define OTG_BIT21                   0x00200000
#define OTG_BIT22                   0x00400000
#define OTG_BIT23                   0x00800000
#define OTG_BIT24                   0x01000000
#define OTG_BIT25                   0x02000000
#define OTG_BIT26                   0x04000000
#define OTG_BIT27                   0x08000000
#define OTG_BIT28                   0x10000000
#define OTG_BIT29                   0x20000000
#define OTG_BIT30                   0x40000000
#define OTG_BIT31                   0x80000000

#define USB_REG_READ8(r)                readb((void *)(otgRegAddress + (r)))
#define USB_REG_READ16(r)               readw((void *)(otgRegAddress + (r)))
#define USB_REG_WRITE8(r,v)             writeb(v,(void *)(otgRegAddress + r))//OUTREG8(otgRegAddress + r, v)
#define USB_REG_WRITE16(r,v)            writew(v,(void *)(otgRegAddress + r))//OUTREG16(otgRegAddress + r, v)

#define UTMI_REG_READ8(r)               readb((void *)(utmiRegAddress + (r)))
#define UTMI_REG_READ16(r)              readw((void *)(utmiRegAddress + (r)))
#define UTMI_REG_WRITE8(r,v)            writeb(v,(void *)(utmiRegAddress + r))//OUTREG8(utmiRegAddress + r, v)
#define UTMI_REG_WRITE16(r,v)           writew(v,(void *)(utmiRegAddress + r))//OUTREG16(utmiRegAddress + r, v)

#define USBC_REG_READ8(r)               readb((void *)(usbcRegAddress + (r)))
#define USBC_REG_READ16(r)              readw((void *)(usbcRegAddress + (r)))
#define USBC_REG_WRITE8(r,v)            writeb(v,(void *)(usbcRegAddress + r))//OUTREG8(usbcRegAddress + r, v)
#define USBC_REG_WRITE16(r,v)           writew(v,(void *)(usbcRegAddress + r))//OUTREG16(usbcRegAddress + r, v)

#define FIFO_ADDRESS(e) (otgRegAddress + (e<<3) + M_FIFO_EP0)
#define FIFO_DATA_PORT  (otgRegAddress + M_REG_FIFO_DATA_PORT)

#ifdef BIG_ENDIAN
#define SWOP(X) ((X) = (((X)<<8)+((X)>>8)))
#define SWAP4(X) ((X) = ((X)<<24) + ((X)>>24) + (((X)>>8)&0x0000FF00) + (((X)<<8)&0x00FF0000) )
#else
#define SWAP4(X) (X = X)
#define SWOP(X)  (X = X)
#endif

#define RETAILMSG(a, b) printk(b)
#define _T(a) (KERN_INFO a)
#define TEXT
#define TRUE	1
#define FALSE	0

#ifdef __cplusplus
}
#endif  //__cplusplus

#endif  /* _MS_CPU_H_ */

