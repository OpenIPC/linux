/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __DT_BINDINGS_DISPLAY_AX620X_H
#define __DT_BINDINGS_DISPLAY_AX620X_H

#define AX_DISPC_FMT_RGB565			1
#define AX_DISPC_FMT_RGB666			2
#define AX_DISPC_FMT_RGB666LP		        3
#define AX_DISPC_FMT_RGB888			4
#define AX_DISPC_FMT_RGB101010		        5
#define AX_DISPC_FMT_YUV422			6
#define AX_DISPC_FMT_YUV422_10		        7

#define MIPI_DSI_FMT_RGB888			0
#define MIPI_DSI_FMT_RGB666			1
#define MIPI_DSI_FMT_RGB666_PACKED	        2
#define MIPI_DSI_FMT_RGB565			3

#define MIPI_DSI_MODE_VIDEO			(1 << 0)
#define MIPI_DSI_MODE_VIDEO_BURST		(1 << 1)
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	        (1 << 2)
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	        (1 << 3)
#define MIPI_DSI_MODE_VIDEO_HSE			(1 << 4)
#define MIPI_DSI_MODE_VIDEO_HFP			(1 << 5)
#define MIPI_DSI_MODE_VIDEO_HBP			(1 << 6)
#define MIPI_DSI_MODE_VIDEO_HSA			(1 << 7)
#define MIPI_DSI_MODE_VSYNC_FLUSH		(1 << 8)
#define MIPI_DSI_MODE_EOT_PACKET		(1 << 9)
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	        (1 << 10)
#define MIPI_DSI_MODE_LPM			(1 << 11)

#define MIPI_DSI_ESD_CHECK_READ_REG_MODE	0

#endif /*__DT_BINDINGS_DISPLAY_AX620X_H */
