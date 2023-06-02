/*------------------------------------------------------------------------------
	Copyright (c) 2009 MStar Semiconductor, Inc.  All rights reserved.
------------------------------------------------------------------------------*/
/*------------------------------------------------------------------------------
    PROJECT: MSB250x Linux BSP
    DESCRIPTION:
          MSB250x dual role USB device controllers


    HISTORY:
         6/11/2010     Calvin Hung    First Revision

-------------------------------------------------------------------------------*/
#ifndef _MSB250X_UDC_IOCTL_H
#define _MSB250X_UDC_IOCTL_H
/*
 * Ioctl definitions
 */

/* Use 'C' as magic number */
#define MSB250X_UDC_IOC_MAGIC  'C'

#define MSB250X_UDC_CONN_CHG _IOR(MSB250X_UDC_IOC_MAGIC, 0, int) /* Get connection change status. */
#define MSB250X_UDC_SET_CONN _IOW(MSB250X_UDC_IOC_MAGIC, 1, int)
#define MSB250X_UDC_GET_CONN _IOR(MSB250X_UDC_IOC_MAGIC, 2, int)
#define MSB250X_UDC_GET_LINESTAT _IOR(MSB250X_UDC_IOC_MAGIC, 3, int)

#define MSB250X_UDC_IOC_MAXNR 14

#endif /* _MSB250X_UDC_IOCTL_H */
