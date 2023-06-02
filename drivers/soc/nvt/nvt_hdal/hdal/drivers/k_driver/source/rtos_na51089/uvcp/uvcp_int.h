/*
    UVCP module internal header file

    UVCP module internal header file

    @file       uvcp_int.h
    @ingroup    mIDrvIO_UVCP
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2020.  All rights reserved.
*/

#ifndef _UVCP_INT_H
#define _UVCP_INT_H

#include "kwrap/error_no.h"
#include "kwrap/semaphore.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "kwrap/util.h"

#include "rcw_macro.h"
#include "kwrap/type.h"
#include "io_address.h"
#include "interrupt.h"
#include "uvcp.h"
#include "dma.h"
#include "dma_protected.h"

#ifndef IOADDR_UVCP_REG_BASE
#define IOADDR_UVCP_REG_BASE	0xF0690000
#endif


#define UVCP_SETREG(_OFS, value)          OUTW(IOADDR_UVCP_REG_BASE+(_OFS), (value))
#define UVCP_GETREG(_OFS)                 INW(IOADDR_UVCP_REG_BASE+(_OFS))


#define FLGPTN_UVCP 	 FLGPTN_BIT(0)
#define FLGPTN_UVCP_BRK  FLGPTN_BIT(1)

#define UVCP_POLLING DISABLE


#endif
