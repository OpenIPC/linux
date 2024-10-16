#include "lindevb720p.h"

static void lcd_power_on(u32 sel);
static void lcd_power_off(u32 sel);
static void lcd_bl_open(u32 sel);
static void lcd_bl_close(u32 sel);

static void lcd_panel_init(u32 sel);
static void lcd_panel_exit(u32 sel);

#define panel_reset(sel, val) sunxi_lcd_gpio_set_value(sel, 0, val)

static void lcd_cfg_panel_info(struct panel_extend_para *info)
{
	u32 i = 0, j = 0;
	u32 items;
	u8 lcd_gamma_tbl[][2] = {
	    {0, 0},     {15, 15},   {30, 30},   {45, 45},   {60, 60},
	    {75, 75},   {90, 90},   {105, 105}, {120, 120}, {135, 135},
	    {150, 150}, {165, 165}, {180, 180}, {195, 195}, {210, 210},
	    {225, 225}, {240, 240}, {255, 255},
	};

	u32 lcd_cmap_tbl[2][3][4] = {
	    {
		{LCD_CMAP_G0, LCD_CMAP_B1, LCD_CMAP_G2, LCD_CMAP_B3},
		{LCD_CMAP_B0, LCD_CMAP_R1, LCD_CMAP_B2, LCD_CMAP_R3},
		{LCD_CMAP_R0, LCD_CMAP_G1, LCD_CMAP_R2, LCD_CMAP_G3},
	    },
	    {
		{LCD_CMAP_B3, LCD_CMAP_G2, LCD_CMAP_B1, LCD_CMAP_G0},
		{LCD_CMAP_R3, LCD_CMAP_B2, LCD_CMAP_R1, LCD_CMAP_B0},
		{LCD_CMAP_G3, LCD_CMAP_R2, LCD_CMAP_G1, LCD_CMAP_R0},
	    },
	};

	items = sizeof(lcd_gamma_tbl) / 2;
	for (i = 0; i < items - 1; i++) {
		u32 num = lcd_gamma_tbl[i + 1][0] - lcd_gamma_tbl[i][0];

		for (j = 0; j < num; j++) {
			u32 value = 0;

			value =
			    lcd_gamma_tbl[i][1] +
			    ((lcd_gamma_tbl[i + 1][1] - lcd_gamma_tbl[i][1]) *
			     j) /
				num;
			info->lcd_gamma_tbl[lcd_gamma_tbl[i][0] + j] =
			    (value << 16) + (value << 8) + value;
		}
	}
	info->lcd_gamma_tbl[255] = (lcd_gamma_tbl[items - 1][1] << 16) +
				   (lcd_gamma_tbl[items - 1][1] << 8) +
				   lcd_gamma_tbl[items - 1][1];

	memcpy(info->lcd_cmap_tbl, lcd_cmap_tbl, sizeof(lcd_cmap_tbl));
}

static s32 lcd_open_flow(u32 sel)
{
	LCD_OPEN_FUNC(sel, lcd_power_on, 10);
	LCD_OPEN_FUNC(sel, lcd_panel_init, 10);
	LCD_OPEN_FUNC(sel, sunxi_lcd_tcon_enable, 50);
	LCD_OPEN_FUNC(sel, lcd_bl_open, 0);
	return 0;
}

static s32 lcd_close_flow(u32 sel)
{
	LCD_CLOSE_FUNC(sel, lcd_bl_close, 0);
	LCD_CLOSE_FUNC(sel, lcd_panel_exit, 200);
	LCD_CLOSE_FUNC(sel, sunxi_lcd_tcon_disable, 0);
	LCD_CLOSE_FUNC(sel, lcd_power_off, 500);

	return 0;
}

static void lcd_power_on(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 1);
	sunxi_lcd_power_enable(sel, 0);
	sunxi_lcd_delay_ms(10);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(50);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(50);
	panel_reset(sel, 1);
	sunxi_lcd_delay_ms(50);

}

static void lcd_power_off(u32 sel)
{
	sunxi_lcd_pin_cfg(sel, 0);
	sunxi_lcd_delay_ms(20);
	panel_reset(sel, 0);
	sunxi_lcd_delay_ms(5);
	sunxi_lcd_power_disable(sel, 0);
}

static void lcd_bl_open(u32 sel)
{
	sunxi_lcd_pwm_enable(sel);
	sunxi_lcd_backlight_enable(sel);
}

static void lcd_bl_close(u32 sel)
{
	sunxi_lcd_backlight_disable(sel);
	sunxi_lcd_pwm_disable(sel);
}

#define REGFLAG_DELAY         0xFE
#define REGFLAG_END_OF_TABLE  0xFF  /* END OF REGISTERS MARKER */

struct LCM_setting_table {
	u8 cmd;
	u32 count;
	u8 para_list[64];
};

static struct LCM_setting_table lcm_initialization_setting[] = {
    {0xB9,  3, {0xF1, 0x12, 0x83} },
    {0xBA, 27, {0x33, 0x81, 0x05, 0xF9, 0x0E, 0x0E, 0x20, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x44, 0x25,
				0x00, 0x91, 0x0A, 0x00, 0x00, 0x02, 0x4F, 0xD1,
				0x00, 0x00, 0x37} },
    {0xB8,  1, {0x26} },
    {0xBF,  3, {0x02, 0x10, 0x00} },
    {0xB3, 10, {0x07, 0x0B, 0x1E, 0x1E, 0x03, 0xFF, 0x00,
                0x00, 0x00, 0x00} },
    {0xC0,  9, {0x73, 0x73, 0x50, 0x50, 0x00, 0x00, 0x08,
                0x70, 0x00} },
    {0xBC,  1, {0x46} },
    {0xCC,  1, {0x0B} },
    {0xB4,  1, {0x80} },
    {0xB2,  3, {0xC8, 0x12, 0xA0} },
    {0xE3, 14, {0x07, 0x07, 0x0B, 0x0B, 0x03, 0x0B, 0x00,
                0x00, 0x00, 0x00, 0xFF, 0x80, 0xC0, 0x10} },
    {0xC1, 12, {0x53, 0x00, 0x32, 0x32, 0x77, 0xF1, 0xFF,
                0xFF, 0xCC, 0xCC, 0x77, 0x77} },
    {0xB5,  2, {0x09, 0x09} },
    {0xB6,  2, {0xB7, 0xB7} },
    {0xE9, 63, {0xC2, 0x10, 0x0A, 0x00, 0x00, 0x81, 0x80,
                0x12, 0x30, 0x00, 0x37, 0x86, 0x81, 0x80, 0x37,
                0x18, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x05, 0x00, 0x00, 0x00, 0x00, 0xF8, 0xBA, 0x46,
                0x02, 0x08, 0x28, 0x88, 0x88, 0x88, 0x88, 0x88,
                0xF8, 0xBA, 0x57, 0x13, 0x18, 0x38, 0x88, 0x88,
                0x88, 0x88, 0x88, 0x00, 0x00, 0x00, 0x03, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
    {0xEA, 61, {0x07, 0x12, 0x01, 0x01, 0x02, 0x3C, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x8F, 0xBA, 0x31,
                0x75, 0x38, 0x18, 0x88, 0x88, 0x88, 0x88, 0x88,
                0x8F, 0xBA, 0x20, 0x64, 0x28, 0x08, 0x88, 0x88,
                0x88, 0x88, 0x88, 0x23, 0x10, 0x00, 0x00, 0x04,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x00} },
    {0xE0, 34, {0x00, 0x02, 0x04, 0x1A, 0x23, 0x3F, 0x2C,
                0x28, 0x05, 0x09, 0x0B, 0x10, 0x11, 0x10, 0x12,
                0x12, 0x19, 0x00, 0x02, 0x04, 0x1A, 0x23, 0x3F,
                0x2C, 0x28, 0x05, 0x09, 0x0B, 0x10, 0x11, 0x10,
                0x12, 0x12, 0x19} },
    {0x11,  1, {0x00} },
    {REGFLAG_DELAY, 120, {} },
    {0x29,  1, {0x00} },
    {REGFLAG_DELAY, 50, {} },
	{REGFLAG_END_OF_TABLE, 0x00, {} }
};

static void lcd_panel_init(u32 sel)
{
	u32 i = 0;

	sunxi_lcd_dsi_clk_enable(sel);
	sunxi_lcd_delay_ms(120);

	for (i = 0;; i++) {
		if (lcm_initialization_setting[i].cmd == REGFLAG_END_OF_TABLE)
			break;
		else if (lcm_initialization_setting[i].cmd == REGFLAG_DELAY)
			sunxi_lcd_delay_ms(lcm_initialization_setting[i].count);
		else {
			dsi_dcs_wr(0, lcm_initialization_setting[i].cmd,
				   lcm_initialization_setting[i].para_list,
				   lcm_initialization_setting[i].count);
		}
	}
}

static void lcd_panel_exit(u32 sel)
{
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x28);
	sunxi_lcd_delay_ms(10);
	sunxi_lcd_dsi_dcs_write_0para(sel, 0x10);
	sunxi_lcd_delay_ms(10);
}

/*sel: 0:lcd0; 1:lcd1*/
static s32 lcd_user_defined_func(u32 sel, u32 para1, u32 para2, u32 para3)
{
	return 0;
}

struct __lcd_panel lindevb720p_panel = {
	.name = "lindevb720p",
	.func = {
		.cfg_panel_info = lcd_cfg_panel_info,
			.cfg_open_flow = lcd_open_flow,
			.cfg_close_flow = lcd_close_flow,
			.lcd_user_defined_func = lcd_user_defined_func,
	},
};
