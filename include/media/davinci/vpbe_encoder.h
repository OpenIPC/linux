/*
 * Copyright (C) 2007 Texas Instruments Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef VPBE_ENCODER_H
#define VPBE_ENCODER_H

#ifdef __KERNEL__
/* Kernel Header files */
#include <linux/i2c.h>
#include <linux/device.h>
#endif

#ifdef __KERNEL__
/* encoder standard related strctures */
#define VPBE_ENCODER_MAX_NO_OUTPUTS		            (3)
#define VPBE_ENCODER_MAX_NUM_STD			(13)
#define VPBE_DM355_ENCODER_MAX_NO_OUTPUTS		    (1)
#define VPBE_DM355_ENCODER_COMPOSITE_NUM_STD        (2)
#define VPBE_DM355_ENCODER_MAX_NUM_STD              (2)

#define VPBE_DM644X_ENCODER_MAX_NO_OUTPUTS		    (3)
#define VPBE_DM644X_ENCODER_COMPONENT_NUM_STD		(6)
#define VPBE_DM644X_ENCODER_SVIDEO_NUM_STD		    (2)
#define VPBE_DM644X_ENCODER_COMPOSITE_NUM_STD       (2)

#define VPBE_DM365_ENCODER_MAX_NO_OUTPUTS		(3)
#define VPBE_DM365_ENCODER_COMPOSITE_NUM_STD		(2)
#define VPBE_DM365_ENCODER_COMPONENT_NUM_STD		(9)
#define VPBE_DM365_ENCODER_SVIDEO_NUM_STD		(2)


#endif				/* End of #ifdef __KERNEL__ */

#endif				/* End of #ifndef VPBE_VENC_H */
