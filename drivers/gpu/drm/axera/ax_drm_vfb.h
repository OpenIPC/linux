/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_DRM_VFB_H
#define __AX_DRM_VFB_H

struct axfb_cursor_pos {
	u16 x;
	u16 y;
};
struct axfb_cursor_res {
	u32 width;		/* Size of image */
	u32 height;
};

struct axfb_cursor_info {
	u16 enable;
	struct axfb_cursor_pos hot;
	struct axfb_cursor_res res;
};

struct axfb_colorkey {
    u32 enable;
    u32 inv;
    u32 key_low;
    u32 key_high;
};


#define AX_FBIOPUT_CURSOR_POS   _IOW('F', 0x21, struct axfb_cursor_pos)
#define AX_FBIOPUT_CURSOR_RES   _IOW('F', 0x22, struct axfb_cursor_res)
#define AX_FBIOPUT_CURSOR_SHOW  _IOW('F', 0x23, u16)
#define AX_FBIOPUT_ALPHA        _IOW('F', 0x24, u32)
#define AX_FBIOGET_CURSORINFO   _IOR('F', 0x25, struct axfb_cursor_info)
#define AX_FBIOGET_TYPE         _IOR('F', 0x26, u16)
#define AX_FBIOGET_COLORKEY     _IOR('F', 0x27, struct axfb_colorkey)
#define AX_FBIOPUT_COLORKEY     _IOW('F', 0x28, struct axfb_colorkey)

#endif /* __AX_DRM_VFB_H */
