/*
 * xrp_firmware.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _XRP_FIRMWARE_H_
#define _XRP_FIRMWARE_H_

struct xvp;

#if IS_ENABLED(CONFIG_OF)
int xrp_request_firmware(struct xvp* xvp);
#else
int xrp_request_firmware(struct xvp* xvp)
{
    (void)xvp;
    return -EINVAL;
}
#endif

#endif /*_XRP_FIRMWARE_H_*/
