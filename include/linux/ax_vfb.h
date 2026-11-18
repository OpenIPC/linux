/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_VFB_H
#define __AX_VFB_H

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long int u64;

struct ax_fb_device {
	int id;

	u32 size;
	u32 width;
	u32 height;
	u32 bpp;
	u32 buf_nr;
	u64 buf_phyaddr;
	void *buf_viraddr;
	u16 is_cursor;

	struct {
		u64 key_low	: 30; /* [29:0] */
		u64 key_high	: 30; /* [59:30] */
		u64 enable	: 1; /* [60] */
		u64 inv		: 1; /* [61] */
	} colorkey;

	void *pdev;
	void *data;
};

int ax_vfb_register(struct ax_fb_device*vfbdev);
int ax_vfb_unregister(struct ax_fb_device *vfbdev);

#endif /* __AX_VFB_H */
