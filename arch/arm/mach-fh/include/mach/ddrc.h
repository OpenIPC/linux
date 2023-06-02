/*
 *	Copyright (c) 2010 Shanghai Fullhan Microelectronics Co., Ltd.
 *				All Rights Reserved. Confidential.
 *
 *This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef DDRC_H_
#define DDRC_H_

#define OFFSET_DENAL_CTL_31         (0x007c)
#define OFFSET_DENAL_CTL_57         (0x00e4)
#define OFFSET_DENAL_CTL_97         (0x0184)

#define DDRC_CONTROLLER_BUSY        (1 << 24)
#define DDRC_CKE_STATUS             (1 << 8)

#define DDRC_LP_CMD_SELFREFRESH     (10 << 8)
#define DDRC_LP_CMD_EXITLOWPOWER    (1 << 8)

#define DDRC_LPI_SR_WAKEUP_TIME     (3 << 24)
#define DDRC_CKSRX_DELAY            (1 << 0)

#endif /* DDRC_H_ */
