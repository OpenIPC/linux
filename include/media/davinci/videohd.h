/*
* videohd.h - Defines temporary HD standards for video drivers
*
* Copyright (C) 2009 Texas Instruments Incorporated - http://www.ti.com/
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License as
* published by the Free Software Foundation version 2.
*
* This program is distributed .as is. WITHOUT ANY WARRANTY of any
* kind, whether express or implied; without even the implied warranty
* of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* This header file will hold all Digital TV video HD/ED standards until same is
* supported by v4l2 sub system. Moving this to this file will help avoid
* unnecessary merge issues. Once the support is available, we could discard
* this file
*/

 /* videohd.h - Defines temporary HD standards for video drivers*/
#ifndef __VIDEOHD_H
#define __VIDEOHD_H

/* Digital TV standards */
#define V4L2_STD_525P_60        ((v4l2_std_id)(0x0001000000000000ULL))
#define V4L2_STD_625P_50        ((v4l2_std_id)(0x0002000000000000ULL))
#define V4L2_STD_720P_60        ((v4l2_std_id)(0x0004000000000000ULL))
#define V4L2_STD_720P_50        ((v4l2_std_id)(0x0008000000000000ULL))
#define V4L2_STD_1080I_60       ((v4l2_std_id)(0x0010000000000000ULL))
#define V4L2_STD_1080I_50       ((v4l2_std_id)(0x0020000000000000ULL))
#define V4L2_STD_1080P_60       ((v4l2_std_id)(0x0040000000000000ULL))
#define V4L2_STD_1080P_50       ((v4l2_std_id)(0x0080000000000000ULL))
#define V4L2_STD_720P_30        ((v4l2_std_id)(0x0100000000000000ULL))
#define V4L2_STD_1080I_30       ((v4l2_std_id)(0x0200000000000000ULL))
#define V4L2_STD_1080P_30       ((v4l2_std_id)(0x0400000000000000ULL))
#endif
