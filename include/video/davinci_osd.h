/*
 * Header file for the TI DaVinci On-Screen Display Manager
 *
 * Andy Lowe (alowe@mvista.com), MontaVista Software
 *
 * 2007 (c) MontaVista Software, Inc. This file is licensed under
 * the terms of the GNU General Public License version 2. This program
 * is licensed "as is" without any warranty of any kind, whether express
 * or implied.
 */

#ifndef _DAVINCI_OSD_H
#define _DAVINCI_OSD_H

struct davinci_osd_platform_data {
	unsigned char invert_field;
};

/**
 * enum davinci_disp_layer
 * @WIN_OSD0: On-Screen Display Window 0
 * @WIN_VID0: Video Window 0
 * @WIN_OSD1: On-Screen Display Window 1
 * @WIN_VID1: Video Window 1
 *
 * Description:
 * An enumeration of the DaVinci display layers.
 */
enum davinci_disp_layer {
	WIN_OSD0,
	WIN_VID0,
	WIN_OSD1,
	WIN_VID1,
};

/**
 * enum davinci_osd_layer
 * @OSDWIN_OSD0: On-Screen Display Window 0
 * @OSDWIN_OSD1: On-Screen Display Window 1
 *
 * Description:
 * An enumeration of the DaVinci OSD layers.
 */
enum davinci_osd_layer {
	OSDWIN_OSD0,
	OSDWIN_OSD1,
};

/**
 * enum davinci_pix_format
 * @PIXFMT_1BPP: 1-bit-per-pixel bitmap
 * @PIXFMT_2BPP: 2-bits-per-pixel bitmap
 * @PIXFMT_4BPP: 4-bits-per-pixel bitmap
 * @PIXFMT_8BPP: 8-bits-per-pixel bitmap
 * @PIXFMT_RGB565: 16-bits-per-pixel RGB565
 * @PIXFMT_YCbCrI: YUV 4:2:2
 * @PIXFMT_RGB888: 24-bits-per-pixel RGB888
 * @PIXFMT_YCrCbI: YUV 4:2:2 with chroma swap
 * @PIXFMT_NV12: YUV 4:2:0 planar
 * @PIXFMT_OSD_ATTR: OSD Attribute Window pixel format (4bpp)
 *
 * Description:
 * An enumeration of the DaVinci pixel formats.
 */
enum davinci_pix_format {
	PIXFMT_1BPP = 0,
	PIXFMT_2BPP,
	PIXFMT_4BPP,
	PIXFMT_8BPP,
	PIXFMT_RGB565,
	PIXFMT_YCbCrI,
	PIXFMT_RGB888,
	PIXFMT_YCrCbI,
	PIXFMT_NV12,
	PIXFMT_OSD_ATTR,
};

/**
 * enum davinci_h_exp_ratio
 * @H_EXP_OFF: no expansion (1/1)
 * @H_EXP_9_OVER_8: 9/8 expansion ratio
 * @H_EXP_3_OVER_2: 3/2 expansion ratio
 *
 * Description:
 * An enumeration of the available horizontal expansion ratios.
 */
enum davinci_h_exp_ratio {
	H_EXP_OFF,
	H_EXP_9_OVER_8,
	H_EXP_3_OVER_2,
};

/**
 * enum davinci_v_exp_ratio
 * @V_EXP_OFF: no expansion (1/1)
 * @V_EXP_6_OVER_5: 6/5 expansion ratio
 *
 * Description:
 * An enumeration of the available vertical expansion ratios.
 */
enum davinci_v_exp_ratio {
	V_EXP_OFF,
	V_EXP_6_OVER_5,
};

/**
 * enum davinci_zoom_factor
 * @ZOOM_X1: no zoom (x1)
 * @ZOOM_X2: x2 zoom
 * @ZOOM_X4: x4 zoom
 *
 * Description:
 * An enumeration of the available zoom factors.
 */
enum davinci_zoom_factor {
	ZOOM_X1,
	ZOOM_X2,
	ZOOM_X4,
};

/**
 * enum davinci_clut
 * @ROM_CLUT: ROM CLUT
 * @RAM_CLUT: RAM CLUT
 *
 * Description:
 * An enumeration of the available Color Lookup Tables (CLUTs).
 */
enum davinci_clut {
	ROM_CLUT,
	RAM_CLUT,
};

/**
 * enum davinci_rom_clut
 * @ROM_CLUT0: Macintosh CLUT
 * @ROM_CLUT1: CLUT from DM270 and prior devices
 *
 * Description:
 * An enumeration of the ROM Color Lookup Table (CLUT) options.
 */
enum davinci_rom_clut {
	ROM_CLUT0,
	ROM_CLUT1,
};

/**
 * enum davinci_blending_factor
 * @OSD_0_VID_8: OSD pixels are fully transparent
 * @OSD_1_VID_7: OSD pixels contribute 1/8, video pixels contribute 7/8
 * @OSD_2_VID_6: OSD pixels contribute 2/8, video pixels contribute 6/8
 * @OSD_3_VID_5: OSD pixels contribute 3/8, video pixels contribute 5/8
 * @OSD_4_VID_4: OSD pixels contribute 4/8, video pixels contribute 4/8
 * @OSD_5_VID_3: OSD pixels contribute 5/8, video pixels contribute 3/8
 * @OSD_6_VID_2: OSD pixels contribute 6/8, video pixels contribute 2/8
 * @OSD_8_VID_0: OSD pixels are fully opaque
 *
 * Description:
 * An enumeration of the DaVinci pixel blending factor options.
 */
enum davinci_blending_factor {
	OSD_0_VID_8,
	OSD_1_VID_7,
	OSD_2_VID_6,
	OSD_3_VID_5,
	OSD_4_VID_4,
	OSD_5_VID_3,
	OSD_6_VID_2,
	OSD_8_VID_0,
};

/**
 * enum davinci_blink_interval
 * @BLINK_X1: blink interval is 1 vertical refresh cycle
 * @BLINK_X2: blink interval is 2 vertical refresh cycles
 * @BLINK_X3: blink interval is 3 vertical refresh cycles
 * @BLINK_X4: blink interval is 4 vertical refresh cycles
 *
 * Description:
 * An enumeration of the DaVinci pixel blinking interval options.
 */
enum davinci_blink_interval {
	BLINK_X1,
	BLINK_X2,
	BLINK_X3,
	BLINK_X4,
};

/**
 * enum davinci_cursor_h_width
 * @H_WIDTH_1: horizontal line width is 1 pixel
 * @H_WIDTH_4: horizontal line width is 4 pixels
 * @H_WIDTH_8: horizontal line width is 8 pixels
 * @H_WIDTH_12: horizontal line width is 12 pixels
 * @H_WIDTH_16: horizontal line width is 16 pixels
 * @H_WIDTH_20: horizontal line width is 20 pixels
 * @H_WIDTH_24: horizontal line width is 24 pixels
 * @H_WIDTH_28: horizontal line width is 28 pixels
 */
enum davinci_cursor_h_width {
	H_WIDTH_1,
	H_WIDTH_4,
	H_WIDTH_8,
	H_WIDTH_12,
	H_WIDTH_16,
	H_WIDTH_20,
	H_WIDTH_24,
	H_WIDTH_28,
};

/**
 * enum davinci_cursor_v_width
 * @V_WIDTH_1: vertical line width is 1 line
 * @V_WIDTH_2: vertical line width is 2 lines
 * @V_WIDTH_4: vertical line width is 4 lines
 * @V_WIDTH_6: vertical line width is 6 lines
 * @V_WIDTH_8: vertical line width is 8 lines
 * @V_WIDTH_10: vertical line width is 10 lines
 * @V_WIDTH_12: vertical line width is 12 lines
 * @V_WIDTH_14: vertical line width is 14 lines
 */
enum davinci_cursor_v_width {
	V_WIDTH_1,
	V_WIDTH_2,
	V_WIDTH_4,
	V_WIDTH_6,
	V_WIDTH_8,
	V_WIDTH_10,
	V_WIDTH_12,
	V_WIDTH_14,
};

/**
 * struct davinci_cursor_config
 * @xsize: horizontal size in pixels
 * @ysize: vertical size in lines
 * @xpos: horizontal offset in pixels from the left edge of the display
 * @ypos: vertical offset in lines from the top of the display
 * @interlaced: Non-zero if the display is interlaced, or zero otherwise
 * @h_width: horizontal line width
 * @v_width: vertical line width
 * @clut: the CLUT selector (ROM or RAM) for the cursor color
 * @clut_index: an index into the CLUT for the cursor color
 *
 * Description:
 * A structure describing the configuration parameters of the hardware
 * rectangular cursor.
 */
struct davinci_cursor_config {
	unsigned xsize;
	unsigned ysize;
	unsigned xpos;
	unsigned ypos;
	int interlaced;
	enum davinci_cursor_h_width h_width;
	enum davinci_cursor_v_width v_width;
	enum davinci_clut clut;
	unsigned char clut_index;
};

/**
 * struct davinci_disp_callback
 * @next: used internally by the display manager to maintain a liked list of
 *        callbacks
 * @mask: a bitmask specifying the display manager event(s) for which the
 *        callback will be invoked
 * @handler: the callback routine
 * @arg: a null pointer that is passed as the second argument to the callback
 *       routine
 */
struct davinci_disp_callback {
	struct davinci_disp_callback *next;
	unsigned mask;
	void (*handler) (unsigned event, void *arg);
	void *arg;
};

/* display manager events */
#define DAVINCI_DISP_END_OF_FRAME	1
#define DAVINCI_DISP_FIRST_FIELD	2
#define DAVINCI_DISP_SECOND_FIELD	4

/**
 * struct davinci_layer_config
 * @pixfmt: pixel format
 * @line_length: offset in bytes between start of each line in memory
 * @xsize: number of horizontal pixels displayed per line
 * @ysize: number of lines displayed
 * @xpos: horizontal offset in pixels from the left edge of the display
 * @ypos: vertical offset in lines from the top of the display
 * @interlaced: Non-zero if the display is interlaced, or zero otherwise
 *
 * Description:
 * A structure describing the configuration parameters of an On-Screen Display
 * (OSD) or video layer related to how the image is stored in memory.
 * @line_length must be a multiple of the cache line size (32 bytes).
 */
struct davinci_layer_config {
	enum davinci_pix_format pixfmt;
	unsigned line_length;
	unsigned xsize;
	unsigned ysize;
	unsigned xpos;
	unsigned ypos;
	int interlaced;
};


/**
 * struct davinci_fb_desc
 * @cbcr_ofst: offset of the cbcr data from the beginning of the frame buffer
 * @yd_offset: offset into the Y-plane where the layer should start displaying
 *
 * Description:
 * A structure describing additional information about the frame buffers being
 * passed to the display.  This may be needed when the buffers have a
 * non-standard layout.
 * @yd_ofst must be 64-byte aligned.
 */
struct davinci_fb_desc {
    unsigned long cbcr_ofst;
    unsigned long yd_ofst;
};

/**
 * davinci_disp_request_layer
 * @layer: layer id
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Request to use an On-Screen Display (OSD) or video layer (window).
 */
int davinci_disp_request_layer(enum davinci_disp_layer layer);

/**
 * davinci_disp_release_layer
 * @layer: layer id
 *
 * Description:
 * Release a layer previously obtained via davinci_disp_request_layer().
 */
void davinci_disp_release_layer(enum davinci_disp_layer layer);

/**
 * davinci_disp_init_layer
 * @layer: layer id
 *
 * Description:
 * Initialize all parameters that are uniquely associated with the specified
 * display layer to their default values.  Parameters that are common to
 * multiple display layers (e.g. expansion ratios) are not affected.  The
 * default state of a layer is disabled with X1 zoom.  The OSD layers default
 * to 8-bits-per-pixel bitmap format, RAM CLUT, REC601 attenuation disabled,
 * color key blending disabled, and are fully opaque.  Video layers maintain
 * their current pixel format, either YCbCrI or YCrCbI, but will default to
 * YCbCrI when first initialized.
 * In general, it is not necessary for a display manager user to call this
 * routine.  The display manager calls this routine automatically for every
 * layer at initialization time and for an individual layer when the layer is
 * released.
 */
void davinci_disp_init_layer(enum davinci_disp_layer layer);

/**
 * davinci_disp_enable_layer
 * @layer: layer id
 * Returns: Zero if successful, or non-zero otherwise
 *
 * Description:
 * Enable a video layer so that it is displayed.
 */
int davinci_disp_enable_layer(enum davinci_disp_layer layer, int otherwin);

/**
 * davinci_disp_disable_layer
 * @layer: layer id
 *
 * Description:
 * Disable a video layer so that it is not displayed.
 */
void davinci_disp_disable_layer(enum davinci_disp_layer layer);

/**
 * davinci_disp_layer_is_enabled
 * @layer: layer id
 * Returns: 1 if the layer is enabled, or 0 otherwise
 *
 * Description:
 * Determine whether or not a video layer is enabled..
 */
int davinci_disp_layer_is_enabled(enum davinci_disp_layer layer);

/**
 * davinci_disp_set_layer_config
 * @layer: layer id
 * @lconfig: a pointer to a davinci_layer_config structure
 * Returns: 1 if the requested configuration is rejected, or 0 otherwise.
 *          When the configuration is rejected, the value of @lconfig on
 *          exit will be the current layer configuration.
 *
 * Description:
 * Configure the parameters of an On-Screen Display (OSD) or video layer
 * related to how the image is stored in memory.  On entry, the values of the
 * members of the @lconfig struct specify the desired configuration.  On exit,
 * the values of the members of the @lconfig struct will be updated to reflect
 * the actual configuration, which is subject to the constraints of the
 * DaVinci OSD controller.
 */
int davinci_disp_set_layer_config(enum davinci_disp_layer layer,
				  struct davinci_layer_config *lconfig);

/**
 * davinci_disp_try_layer_config
 * @layer: layer id
 * @lconfig: a pointer to a davinci_layer_config structure
 * Returns: 1 if the requested configuration is rejected, or 0 otherwise.
 *          When the configuration is rejected, the value of @lconfig on
 *          exit will be the current layer configuration.
 *
 * Description:
 * Evaluate the parameters of an On-Screen Display (OSD) or video layer
 * configuration.  On entry, the values of the members of the @lconfig struct
 * specify the desired configuration.  On exit, the values of the members of the
 * @lconfig struct will be updated to reflect the closest actual configuration
 * which could currently be set and enabled subject to the constraints of the
 * DaVinci OSD controller.  The actual layer configuration is not modified by
 * this routine.
 *
 * Note that some of the constraints depend on the current configuration of
 * other windows, so it is possible for a configuration returned by
 * davinci_disp_try_layer_config() to fail to be set or enabled successfully
 * if the configuration of other windows has changed.
 */
int davinci_disp_try_layer_config(enum davinci_disp_layer layer,
				  struct davinci_layer_config *lconfig);

/**
 * davinci_disp_get_layer_config
 * @layer: layer id
 * @lconfig: a pointer to a davinci_layer_config structure
 *
 * Description:
 * Get the parameters of an On-Screen Display (OSD) or video layer
 * related to how the image is stored in memory.  On exit, the values of the
 * members of the @lconfig struct will be updated to reflect the current
 * configuration.
 */
void davinci_disp_get_layer_config(enum davinci_disp_layer layer,
				   struct davinci_layer_config *lconfig);

/**
 * davinci_disp_start_layer
 * @layer: layer id
 * @fb_base_phys: physical base address of the framebuffer
 *
 * Description:
 * Initialize the framebuffer base address \of an On-Screen Display (OSD) or
 * video layer.  Display of the layer may be either enabled or disabled on
 * entry and will be unchanged on exit.  @fb_base_phys must be cache-line
 * (32-byte) aligned.
 */
void davinci_disp_start_layer(enum davinci_disp_layer layer,
			      unsigned long fb_base_phys,
			      struct davinci_fb_desc *fb_desc);

/**
 * davinci_disp_set_interpolation_filter
 * @filter: non-zero to enable the interpolation filter, or zero to disable
 *
 * Description:
 * Globally enable or disable the scaling (zoom and expansion) interpolation
 * filter.  It applies to all OSD and video windows.
 */
void davinci_disp_set_interpolation_filter(int filter);

/**
 * davinci_disp_get_interpolation_filter
 * Returns: 1 if the interpolation filter is enabled, or 0 otherwise
 *
 * Description:
 * Get the enabled/disabled status of the scaling (zoom and expansion)
 * interpolation filter.
 */
int davinci_disp_get_interpolation_filter(void);

/**
 * davinci_disp_set_osd_expansion
 * @h_exp: the horizontal expansion ratio
 * @v_exp: the vertical expansion ratio
 * Returns: zero if successful, or non-zero otherwise
 *
 * Description:
 * Configure the expansion ratio for the OSD windows.
 */
int davinci_disp_set_osd_expansion(enum davinci_h_exp_ratio h_exp,
				   enum davinci_v_exp_ratio v_exp);

/**
 * davinci_disp_get_osd_expansion
 * @h_exp: the horizontal expansion ratio
 * @v_exp: the vertical expansion ratio
 *
 * Description:
 * Get the expansion ratio for the OSD windows.
 */
void davinci_disp_get_osd_expansion(enum davinci_h_exp_ratio *h_exp,
				    enum davinci_v_exp_ratio *v_exp);

/**
 * davinci_disp_set_vid_expansion
 * @h_exp: the horizontal expansion ratio
 * @v_exp: the vertical expansion ratio
 * Returns: zero if successful, or non-zero otherwise
 *
 * Description:
 * Configure the expansion ratio for the video windows.
 */
int davinci_disp_set_vid_expansion(enum davinci_h_exp_ratio h_exp,
				   enum davinci_v_exp_ratio v_exp);

/**
 * davinci_disp_get_vid_expansion
 * @h_exp: the horizontal expansion ratio
 * @v_exp: the vertical expansion ratio
 *
 * Description:
 * Get the expansion ratio for the video windows.
 */
void davinci_disp_get_vid_expansion(enum davinci_h_exp_ratio *h_exp,
				    enum davinci_v_exp_ratio *v_exp);

/**
 * davinci_disp_set_zoom
 * @layer: layer id
 * @h_zoom: horizontal zoom factor
 * @v_zoom: vertical zoom factor
 *
 * Description:
 * Set the horizontal and vertical zoom factors.
 */
void davinci_disp_set_zoom(enum davinci_disp_layer layer,
			   enum davinci_zoom_factor h_zoom,
			   enum davinci_zoom_factor v_zoom);

/**
 * davinci_disp_get_zoom
 * @layer: layer id
 * @h_zoom: horizontal zoom factor
 * @v_zoom: vertical zoom factor
 *
 * Description:
 * Get the horizontal and vertical zoom factors.
 */
void davinci_disp_get_zoom(enum davinci_disp_layer layer,
			   enum davinci_zoom_factor *h_zoom,
			   enum davinci_zoom_factor *v_zoom);

/**
 * davinci_disp_set_background
 * @clut: the CLUT selector
 * @clut_index: an index into the CLUT
 *
 * Description:
 * Set the background color that is displayed in any region not covered by one
 * of the display layers.  The color is selected by specifying a Color Lookup
 * Table (either ROM or RAM) and an index into the CLUT.
 */
void davinci_disp_set_background(enum davinci_clut clut,
				 unsigned char clut_index);

/**
 * davinci_disp_get_background
 * @clut: the CLUT selector
 * @clut_index: an index into the CLUT
 *
 * Description:
 * Get the background color that is displayed in any region not covered by one
 * of the display layers.  The color is selected by specifying a Color Lookup
 * Table (either ROM or RAM) and an index into the CLUT.
 */
void davinci_disp_get_background(enum davinci_clut *clut,
				 unsigned char *clut_index);

/**
 * davinci_disp_set_rom_clut
 * @rom_clut: the ROM CLUT selector
 *
 * Description:
 * Choose which of the two ROM Color Lookup Tables will be used.
 */
void davinci_disp_set_rom_clut(enum davinci_rom_clut rom_clut);

/**
 * davinci_disp_get_rom_clut
 * Returns: the ROM CLUT selector
 *
 * Description:
 * Query which of the two ROM Color Lookup Tables is currently selected.
 */
enum davinci_rom_clut davinci_disp_get_rom_clut(void);

/**
 * davinci_disp_set_clut_ycbcr
 * @clut_index: an index into the CLUT
 * @y: Luma (Y) value
 * @cb: Blue Chroma (Cb) value
 * @cr: Red Chroma (Cr) value
 *
 * Description:
 * Set a YCbCr value in the Color Lookup Table (CLUT).
 */
void davinci_disp_set_clut_ycbcr(unsigned char clut_index, unsigned char y,
				 unsigned char cb, unsigned char cr);

/**
 * davinci_disp_set_clut_rgb
 * @clut_index: an index into the CLUT
 * @r: Red value
 * @g: Green value
 * @b: Blue value
 *
 * Description:
 * Set an RGB value in the Color Lookup Table (CLUT).
 */
void davinci_disp_set_clut_rgb(unsigned char clut_index, unsigned char r,
			       unsigned char g, unsigned char b);

/**
 * davinci_disp_set_osd_clut
 * @osdwin: OSD0 or OSD1 layer id
 * @clut: the CLUT selector
 *
 * Description:
 * Select whether the RAM CLUT or the ROM CLUT is to be used for an OSD window.
 * The selection is only significant if the window is using a 1-, 2-, 4-, or
 * 8-bit pixel format.
 */
void davinci_disp_set_osd_clut(enum davinci_osd_layer osdwin,
			       enum davinci_clut clut);

/**
 * davinci_disp_get_osd_clut
 * @osdwin: OSD0 or OSD1 layer id
 * Returns: the CLUT selector
 *
 * Description:
 * Query whether the RAM CLUT or the ROM CLUT is currently selected for an OSD
 * window.  The selection is only significant if the window is using a 1-, 2-,
 * 4-, or 8-bit pixel format.
 */
enum davinci_clut davinci_disp_get_osd_clut(enum davinci_osd_layer osdwin);

/**
 * davinci_disp_enable_color_key
 * @osdwin: OSD0 or OSD1 layer id
 * @colorkey: the transparency color key
 *
 * Description:
 * Enable transparency color key blending.  Any pixel in the specified OSD
 * window which matches the color key will be transparent (or partially
 * transparent) and allow the underlying video pixel to be displayed based on
 * the blending factor.
 *
 * Interpretation of the @colorkey argument is determined by the pixel format
 * assigned to the window.  The pixel format must be specified via
 * davinci_disp_set_layer_config() before calling this routine.
 *
 * %PIXFMT_1BPP pixels matching @colorkey bit 0 are blended.
 * %PIXFMT_2BPP pixels matching @colorkey bits 1-0 are blended.
 * %PIXFMT_4BPP pixels matching @colorkey bits 3-0 are blended.
 * %PIXFMT_8BPP pixels matching @colorkey bits 7-0 are blended.
 * %PIXFMT_RGB565 pixels matching @colorkey bits 15-0 are blended.
 * %PIXFMT_YCbCrI pixels with Luma (Y) matching @colorkey bits 15-8 are blended.
 * %PIXFMT_RGB888 pixels matching @colorkey bits 23-0 are blended.
 * %PIXFMT_YCrCbI pixels with Luma (Y) matching @colorkey bits 15-8 are blended.
 * Color keying is not applicable to %PIXFMT_OSD_ATTR pixels.
 *
 * For DM644x processors, only the four %PIXFMT_nBPP pixel formats and
 * %PIXFMT_RGB565 are supported.  The color key for the bitmap formats is fixed
 * at zero on DM644x processors, so the value passed in the @colorkey argument
 * is only significant for the %PIXFMT_RGB565 pixel format.
 */
void davinci_disp_enable_color_key(enum davinci_osd_layer osdwin,
				   unsigned colorkey);

/**
 * davinci_disp_disable_color_key
 * @osdwin: OSD0 or OSD1 layer id
 *
 * Description:
 * Disable transparency color key blending for the specified OSD layer.  All
 * pixels in the OSD window will be blended with video pixels according to the
 * blending factor.
 */
void davinci_disp_disable_color_key(enum davinci_osd_layer osdwin);

/**
 * davinci_disp_set_blending_factor
 * @osdwin: OSD0 or OSD1 layer id
 * @blend: the blending factor
 *
 * Description:
 * Set the blending factor for an OSD window.  The blending factor determines
 * the degree of opacity or transparency of OSD pixels.  Transparent or
 * partially transparent pixels allow the underlying video pixels to be
 * displayed based on the blending factor.  All OSD pixels are blended unless
 * color keying is enabled, in which case only pixels matching the color key
 * are blended.
 */
void davinci_disp_set_blending_factor(enum davinci_osd_layer osdwin,
				      enum davinci_blending_factor blend);

/**
 * davinci_disp_get_blending_factor
 * @osdwin: OSD0 or OSD1 layer id
 * Returns: the blending factor
 *
 * Description:
 * Get the blending factor for an OSD window.
 */
enum davinci_blending_factor davinci_disp_get_blending_factor(enum
							      davinci_osd_layer
							      osdwin);

/**
 * davinci_disp_set_rec601_attenuation
 * @osdwin: OSD0 or OSD1 layer id
 * @enable: non-zero to enable REC601 attenuation, or zero to disable
 *
 * Description:
 * Enable or disable REC601 attenuation of data in an OSD window.
 */
void davinci_disp_set_rec601_attenuation(enum davinci_osd_layer osdwin,
					 int enable);

/**
 * davinci_disp_get_rec601_attenuation
 * @osdwin: OSD0 or OSD1 layer id
 * Returns: 1 if REC601 attenuation is enabled, or 0 otherwise
 *
 * Description:
 * Get the REC601 attenuation status for an OSD window.
 */
int davinci_disp_get_rec601_attenuation(enum davinci_osd_layer osdwin);

/**
 * davinci_disp_set_palette_map
 * @osdwin: OSD0 or OSD1 layer id
 * @pixel_value: a bitmap pixel value
 * @clut_index: an index into the CLUT
 *
 * Description:
 * A 256-entry Color Lookup Table (CLUT) is used for bitmap pixel formats.
 * Every entry in the CLUT is used when the color depth is 8 bits.  However,
 * only a subset of the CLUT entries are needed for color depths of 1, 2,
 * or 4 bits.  Each pixel value for color depths of 1, 2, or 4 bits can be
 * mapped to any of the 256 CLUT entries.  Two CLUT entries are used for a
 * color depth of 1 bit, four CLUT entries are used for a color depth of
 * 2 bits, and sixteen CLUT entries are used for a color depth of 4 bits.
 *
 * Interpretation of the @pixel_value argument is determined by the pixel
 * format assigned to the window.  The pixel format must be specified via
 * davinci_disp_set_layer_config() before calling this routine.
 */
void davinci_disp_set_palette_map(enum davinci_osd_layer osdwin,
				  unsigned char pixel_value,
				  unsigned char clut_index);

/**
 * davinci_disp_get_palette_map
 * @osdwin: OSD0 or OSD1 layer id
 * @pixel_value: a bitmap pixel value
 * Returns: an index into the CLUT
 *
 * Description:
 * Given a pixel value, return the corresponding clut index from the palette
 * map.
 *
 * Interpretation of the @pixel_value argument is determined by the pixel
 * format assigned to the window.  The pixel format must be specified via
 * davinci_disp_set_layer_config() before calling this routine.
 */
unsigned char davinci_disp_get_palette_map(enum davinci_osd_layer osdwin,
				  unsigned char pixel_value);

/**
 * davinci_disp_set_blink_attribute
 * @enable: non-zero to enable blinking, zero to disable
 * @blink: the blinking interval
 *
 * Description:
 * Set the blinking attributes when OSD1 is configured as an attribute window.
 * If blinking is enabled, then OSD0 pixels which have their corresponding
 * per-pixel blinking bit set in OSD1 will blink.  The blinking interval is a
 * multiple of the vertical refresh interval.
 */
void davinci_disp_set_blink_attribute(int enable,
				      enum davinci_blink_interval blink);

/**
 * davinci_disp_get_blink_attribute
 * @enable: one if blinking is enabled, or zero if disabled
 * @blink: the blinking interval
 *
 * Description:
 * Get the blinking attributes when OSD1 is configured as an attribute window.
 * If blinking is enabled, then OSD0 pixels which have their corresponding
 * per-pixel blinking bit set in OSD1 will blink.  The blinking interval is a
 * multiple of the vertical refresh interval.
 */
void davinci_disp_get_blink_attribute(int *enable,
				      enum davinci_blink_interval *blink);

/**
 * davinci_disp_cursor_enable
 *
 * Description:
 * Enable display of the hardware cursor.
 */
void davinci_disp_cursor_enable(void);

/**
 * davinci_disp_cursor_disable
 *
 * Description:
 * Disable display of the hardware cursor.
 */
void davinci_disp_cursor_disable(void);

/**
 * davinci_disp_cursor_is_enabled
 * Returns: 1 if cursor is enabled, or 0 otherwise
 *
 * Description:
 * Query whether or not display of the hardware cursor is enabled.
 */
int davinci_disp_cursor_is_enabled(void);

/**
 * davinci_disp_set_cursor_config
 * @cursor: the cursor configuration
 *
 * Description:
 * Set the configuration (color, line width, position, and size) of the hardware
 * rectangular cursor.
 */
void davinci_disp_set_cursor_config(struct davinci_cursor_config *cursor);

/**
 * davinci_disp_get_cursor_config
 * @cursor: the cursor configuration
 *
 * Description:
 * Get the configuration (color, line width, position, and size) of the hardware
 * rectangular cursor.
 */
void davinci_disp_get_cursor_config(struct davinci_cursor_config *cursor);

/**
 * davinci_disp_set_field_inversion
 * @enable: non-zero to enable field signal inversion, or zero to disable
 *
 * Description:
 * Enable or disable field signal inversion, which inverts the polarity of the
 * field ID signal from the video encoder.  This setting applies to all OSD and
 * video windows.  It is only significant when the display is interlaced.
 */
void davinci_disp_set_field_inversion(int enable);

/**
 * davinci_disp_register_callback
 * @callback: a pointer to a davinci_disp_callback struct
 * Returns: zero if successful, or non-zero otherwise
 *
 * Description:
 * Register a callback routine for a DaVinci display manager event.
 * @callback->handler is a pointer to the callback routine.  The first argument
 * passed to the callback routine is the display manager event bitmask
 * indicating which event(s) occurred.  The second argument passed to the
 * callback routine is @callback->arg.  @callback->mask is a bitmask specifying
 * the events for which the caller wishes to be notified.  @callback->next is
 * used internally by the display manager and need not be initialized by the
 * caller.  Once a callback routine has been registered, the caller must not
 * modify the @callback struct until the callback has been unregistered.
 *
 * The display manager events are %DAVINCI_DISP_END_OF_FRAME,
 * %DAVINCI_DISP_FIRST_FIELD, and %DAVINCI_DISP_SECOND_FIELD.
 */
int davinci_disp_register_callback(struct davinci_disp_callback *callback);

/**
 * davinci_disp_unregister_callback
 * @callback: a pointer to a davinci_disp_callback struct
 * Returns: zero if successful, or non-zero otherwise
 *
 * Description:
 * Unregister a display manager callback routine previously registered with
 * davinci_disp_register_callback().
 */
int davinci_disp_unregister_callback(struct davinci_disp_callback *callback);

#ifdef __KERNEL__
void osd_write_left_margin(u32 val);

void osd_write_upper_margin(u32 val);

u32 osd_read_left_margin(void);

u32 osd_read_upper_margin(void);
#endif

#endif
