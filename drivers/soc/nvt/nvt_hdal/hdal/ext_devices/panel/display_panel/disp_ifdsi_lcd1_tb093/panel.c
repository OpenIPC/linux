/*
    Display object for driving DSI device

    @file       NT35521.c
    @ingroup
    @note       This panel MUST select ide clock to PLL1 ( 480 ). Once change to \n
				another frequence, the _IDE_FDCLK should be re calculated

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dispdev_ifdsi.h"

#define PANEL_WIDTH     320//720
#define PANEL_HEIGHT    1280//1280

#define NT35521_IND_MSG(...)       debug_msg(__VA_ARGS__)

#define NT35521_ERR_MSG(...)       debug_msg(__VA_ARGS__)

#define NT35521_WRN_MSG(...)       debug_msg(__VA_ARGS__)
#define NT35521_TE_OFF              0
#define NT35521_TE_ON               1

#define NT35521_TE_PACKET           0
#define NT35521_TE_PIN              1


#define DSI_FORMAT_RGB565          0    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB666P         1    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB666L         2    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB888          3    //ide use 480 & DSI use 480

#define DSI_OP_MODE_CMD_MODE       1
#define DSI_OP_MODE_VDO_MODE       0
#define DSI_PACKET_FORMAT          DSI_FORMAT_RGB888  //NT35521 only support RGB888

//#define DSI_TARGET_CLK             480    //real chip use 480Mhz
//#define DSI_TARGET_CLK             240    //real chip use 240Mhz
//#define DSI_TARGET_CLK             160  //FPGA use 160MHz
//#define DSI_TARGET_CLK             120
//#define DSI_TARGET_CLK             54
#define DSI_OP_MODE                DSI_OP_MODE_VDO_MODE//DSI_OP_MODE_CMD_MODE
#if 0
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P) && (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
#error "Command mode not support RGB666P"
#endif

//#if (_FPGA_EMULATION_ == ENABLE)
#if defined (_NVT_FPGA_)

#if 0
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#define _IDE_FDCLK      60000000 //((((DSI_TARGET_CLK / 8) * 2)/3)*3)
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
#if (DSI_TARGET_CLK == 240)
#define _IDE_FDCLK      26660000
#else
//#define _IDE_fDCLK      ((((((DSI_TARGET_CLK / 8) * 2))*9)/4))
#if(DSI_TARGET_CLK == 120)
#define _IDE_FDCLK      13330000
#else
#define _IDE_FDCLK      26660000
#endif
#endif
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
#define _IDE_FDCLK      ((((DSI_TARGET_CLK / 8) * 2) / 2))
#endif

#else
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#define _IDE_FDCLK      20000000 //(((((DSI_TARGET_CLK / 8) * 2)/3))+1)
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
#define _IDE_FDCLK      27000000 //((((((DSI_TARGET_CLK / 8) * 2))*4)/9)+1)
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
#define _IDE_FDCLK      27000000 //(((((DSI_TARGET_CLK / 8) * 2)/ 2))+1)
#endif //#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#endif //#if 0

#else
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P) && (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
#error "Command mode not support RGB666P"
#endif
#if(DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#if(DSI_TARGET_CLK == 480)
#define _IDE_FDCLK      100000000 //(480/24)*4 = 80MHz. (IDE = 297MHz, 297/3= 99MHz)
#else
#define _IDE_FDCLK      297000000 //(960/24)*4 = 160MHz. (IDE = 297MHz, 297/1= 297MHz)
#endif
#endif

#endif

#endif

#if 0
#define DSI_TARGET_CLK             480
#define _IDE_FDCLK      (((((DSI_TARGET_CLK / 8) *4)/3))+1)
#endif

#if 0
#define DSI_TARGET_CLK             160
#define _IDE_FDCLK      297000000 //(960/24)*4 = 160MHz. (IDE = 297MHz, 297/1= 297MHz)

#endif

#if 0
#define DSI_TARGET_CLK             240//240//160
#define _IDE_FDCLK      60000000//37125000 //(960/24)*4 = 160MHz. (IDE = 297MHz, 297/1= 297MHz)
#endif


#if 0
#define DSI_TARGET_CLK             240//160
#define _IDE_FDCLK      75000000//37125000 //(960/24)*4 = 160MHz. (IDE = 297MHz, 297/1= 297MHz)
#endif

#if 1
#define DSI_TARGET_CLK             240//160
#define _IDE_FDCLK      70000000//37125000 //(960/24)*4 = 160MHz. (IDE = 297MHz, 297/1= 297MHz)
#endif


#if 0
#define DSI_TARGET_CLK             480//160
#define _IDE_FDCLK      128800000//37125000 //(960/24)*4 = 160MHz. (IDE = 297MHz, 297/1= 297MHz)
#endif


#if 0
#define DSI_TARGET_CLK             480//240//160
#define _IDE_FDCLK      297000000//37125000 //(960/24)*4 = 160MHz. (IDE = 297MHz, 297/1= 297MHz)
#endif

/*
    panel Parameters for TCON NT35521
*/
//@{
/*Used in DSI*/
const T_PANEL_CMD t_cmd_mode_dsi[] = {

	{DSICMD_CMD,     0x01},
	{CMDDELAY_MS,    120},

	{DSICMD_CMD,     0xF0},
	{DSICMD_DATA,    0x5A},
	{DSICMD_DATA,    0x59},

	{DSICMD_CMD,     0xF1},
	{DSICMD_DATA,    0xA5},
	{DSICMD_DATA,    0xA6},

	{DSICMD_CMD,     0xF7},
	{DSICMD_DATA,    0x20},
	{DSICMD_DATA,    0x00},

	{DSICMD_CMD,     0xB4},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x04},
	{DSICMD_DATA,    0x05},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x1E},
	{DSICMD_DATA,    0x1F},
	{DSICMD_DATA,    0x01},
	{DSICMD_DATA,    0x10},
	{DSICMD_DATA,    0x11},
	{DSICMD_DATA,    0x12},
	{DSICMD_DATA,    0x13},
	{DSICMD_DATA,    0x0C},
	{DSICMD_DATA,    0x0D},
	{DSICMD_DATA,    0x0E},
	{DSICMD_DATA,    0x0F},
	{DSICMD_DATA,    0x03},
	{DSICMD_DATA,    0x03},
	{DSICMD_DATA,    0x03},
	{DSICMD_DATA,    0x03},
	{DSICMD_DATA,    0x03},
	{DSICMD_DATA,    0x03},
	{DSICMD_DATA,    0x03},


	{DSICMD_CMD,     0xB0},
	{DSICMD_DATA,    0xC6},
	{DSICMD_DATA,    0xBB},
	{DSICMD_DATA,    0xBB},
	{DSICMD_DATA,    0xBB},
	{DSICMD_DATA,    0x33},
	{DSICMD_DATA,    0x33},
	{DSICMD_DATA,    0x33},
	{DSICMD_DATA,    0x33},
	{DSICMD_DATA,    0x24},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x9C},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x80},

	{DSICMD_CMD,     0xB1},
	{DSICMD_DATA,    0x53},
	{DSICMD_DATA,    0xA0},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x85},
	{DSICMD_DATA,    0x04},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x7C},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x5F},


	{DSICMD_CMD,     0xB2},
	{DSICMD_DATA,    0x07},
	{DSICMD_DATA,    0x09},
	{DSICMD_DATA,    0x08},
	{DSICMD_DATA,    0x8B},
	{DSICMD_DATA,    0x08},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x22},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x44},
	{DSICMD_DATA,    0xD9},

	{DSICMD_CMD,     0xB6},
	{DSICMD_DATA,    0x22},
	{DSICMD_DATA,    0x22},

	{DSICMD_CMD,     0xB7},
	{DSICMD_DATA,    0x01},
	{DSICMD_DATA,    0x01},
	{DSICMD_DATA,    0x09},
	{DSICMD_DATA,    0x0D},
	{DSICMD_DATA,    0x11},
	{DSICMD_DATA,    0x19},
	{DSICMD_DATA,    0x1D},
	{DSICMD_DATA,    0x15},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x25},
	{DSICMD_DATA,    0x21},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x02},
	{DSICMD_DATA,    0xF7},
	{DSICMD_DATA,    0x38},


	{DSICMD_CMD,     0xB8},
	{DSICMD_DATA,    0xB8},
	{DSICMD_DATA,    0x52},
	{DSICMD_DATA,    0x02},
	{DSICMD_DATA,    0xCC},

	{DSICMD_CMD,     0xBA},
	{DSICMD_DATA,    0x27},
	{DSICMD_DATA,    0xD3},

	{DSICMD_CMD,     0xBD},
	{DSICMD_DATA,    0x43},
	{DSICMD_DATA,    0x0E},
	{DSICMD_DATA,    0x0E},
	{DSICMD_DATA,    0x4B},
	{DSICMD_DATA,    0x4B},
	{DSICMD_DATA,    0x32},
	{DSICMD_DATA,    0x14},

#if 0
	{DSICMD_CMD,     0xC1},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x0F},
	{DSICMD_DATA,    0x0E},
	{DSICMD_DATA,    0x01},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x36},
	{DSICMD_DATA,    0x3A},
	{DSICMD_DATA,    0x08},
#endif
#if 1
		{DSICMD_CMD,	 0xC1},
		{DSICMD_DATA,	 0x00},
		{DSICMD_DATA,	 0xF0},
		{DSICMD_DATA,	 0xF0},
		{DSICMD_DATA,	 0x18},
		{DSICMD_DATA,	 0x00},
		{DSICMD_DATA,	 0x0A},
		{DSICMD_DATA,	 0x0C},
		{DSICMD_DATA,	 0x18},
#endif


	{DSICMD_CMD,     0xC2},
	{DSICMD_DATA,    0x22},
	{DSICMD_DATA,    0xF8},

	{DSICMD_CMD,     0xC3},
	{DSICMD_DATA,    0x22},
	{DSICMD_DATA,    0x31},

	{DSICMD_CMD,     0xC6},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,	 0xFF},
	{DSICMD_DATA,	 0x00},
	{DSICMD_DATA,	 0x00},
	{DSICMD_DATA,	 0xFF},
	{DSICMD_DATA,	 0x00},
	{DSICMD_DATA,	 0x00},

	{DSICMD_CMD,     0xC8},
	{DSICMD_DATA,    0x7C},
	{DSICMD_DATA,    0x65},
	{DSICMD_DATA,	 0x55},
	{DSICMD_DATA,	 0x49},
	{DSICMD_DATA,	 0x45},
	{DSICMD_DATA,	 0x36},
	{DSICMD_DATA,	 0x3a},
	{DSICMD_DATA,	 0x23},
	{DSICMD_DATA,    0x3B},
	{DSICMD_DATA,    0x39},
	{DSICMD_DATA,	 0x38},
	{DSICMD_DATA,	 0x55},
	{DSICMD_DATA,	 0x43},
	{DSICMD_DATA,	 0x4C},
	{DSICMD_DATA,	 0x3F},
	{DSICMD_DATA,	 0x3C},
	{DSICMD_DATA,    0x2F},
	{DSICMD_DATA,    0x1C},
	{DSICMD_DATA,	 0x06},
	{DSICMD_DATA,	 0x7C},
	{DSICMD_DATA,	 0x66},
	{DSICMD_DATA,	 0x55},
	{DSICMD_DATA,	 0x49},
	{DSICMD_DATA,	 0x45},
	{DSICMD_DATA,	 0x36},
	{DSICMD_DATA,	 0x3A},
	{DSICMD_DATA,	 0x23},
	{DSICMD_DATA,	 0x3B},
	{DSICMD_DATA,	 0x38},
	{DSICMD_DATA,	 0x38},
	{DSICMD_DATA,	 0x55},
	{DSICMD_DATA,	 0x43},
	{DSICMD_DATA,	 0x4C},
	{DSICMD_DATA,	 0x3F},
	{DSICMD_DATA,	 0x3C},
	{DSICMD_DATA,	 0x2F},
	{DSICMD_DATA,	 0x1C},
	{DSICMD_DATA,	 0x06},


	{DSICMD_CMD,     0xD0},
	{DSICMD_DATA,    0x07},
	{DSICMD_DATA,    0xFF},
	{DSICMD_DATA,    0xFF},

	{DSICMD_CMD,     0xD2},
	{DSICMD_DATA,    0x63},
	{DSICMD_DATA,    0x0B},
	{DSICMD_DATA,    0x08},
	{DSICMD_DATA,    0x88},

	{DSICMD_CMD,     0xD4},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x32},
	{DSICMD_DATA,    0x04},
	{DSICMD_DATA,    0x51},

	{DSICMD_CMD,     0xF1},
	{DSICMD_DATA,    0x5A},
	{DSICMD_DATA,    0x59},


	{DSICMD_CMD,     0xF0},
	{DSICMD_DATA,    0xA5},
	{DSICMD_DATA,    0xA6},

	// Sleep out
	{DSICMD_CMD,     0x11},
	{CMDDELAY_MS,    200},

	// Display on
	{DSICMD_CMD,     0x29},
	{CMDDELAY_MS,    50},

	//{DSICMD_CMD,     0x0a},
	//{DSICMD_DATA,    0x01},

};


const T_PANEL_CMD t_cmd_standby_dsi[] = {
	{DSICMD_CMD,     0x28},         // Display OFF
	{CMDDELAY_MS,    10},
	//{DSICMD_CMD,     0x10},      // Sleep in
	//{CMDDELAY_MS,    10},
};

const T_LCD_PARAM t_mode_dsi[] = {
	/***********       MI Serial Format 1      *************/
	{
		// tPANEL_PARAM
		{
			/* Old prototype */
			//PINMUX_DSI_4_LANE_CMD_MODE_RGB565,    //!< LCDMode
			//PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB565,//!< LCDMode
			//PINMUX_DSI_4_LANE_CMD_MODE_RGB565,
			//PINMUX_DSI_4_LANE_CMD_MODE_RGB666L,
#if (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
#if(DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
			PINMUX_DSI_4_LANE_CMD_MODE_RGB888,
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
			PINMUX_DSI_4_LANE_CMD_MODE_RGB666L,
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
			PINMUX_DSI_4_LANE_CMD_MODE_RGB565,
#endif
#else
#if(DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
			//PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB888,
			PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB888,
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
			PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666L,
			//PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666L,
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
			PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB666P,
			//PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB666P,
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
			///PINMUX_DSI_4_LANE_VDO_SYNC_PULSE_RGB565,
			PINMUX_DSI_4_LANE_VDO_SYNC_EVENT_RGB565,
#endif
#endif
			_IDE_FDCLK,                             //!< fd_clk
			824,//0x36+PANEL_WIDTH+0x3A,//420,//(0x100 + PANEL_WIDTH),                   //!< uiHSyncTotalPeriod
			320,//PANEL_WIDTH,                            //!< uiHSyncActivePeriod
			240,//0x36,//24,//0x80,                                   //!< uiHSyncBackPorch
			1304,//0x0F+PANEL_HEIGHT+0x0E,//324 + PANEL_HEIGHT,//0x20 + PANEL_HEIGHT,                    //!< ui_vsync_total_period
			1280,//PANEL_HEIGHT,                           //!< ui_vsync_active_period
			10,//0x0F,//294,//0x10,                                   //!< ui_vsync_back_porch_odd
			10,//0x0F,//294,//0x10,                                   //!< ui_vsync_back_porch_even
#if 1
			PANEL_WIDTH,                            //!< ui_buffer_width
			PANEL_HEIGHT,                           //!< ui_buffer_height
			PANEL_WIDTH,                            //!< ui_window_width
			PANEL_HEIGHT,                           //!< ui_window_height
#else
			PANEL_HEIGHT,                            //!< ui_buffer_width
			PANEL_WIDTH,                           //!< ui_buffer_height
			PANEL_HEIGHT,                            //!< ui_window_width
			PANEL_WIDTH,                           //!< ui_window_height
#endif
			FALSE,                                  //!< b_ycbcr_format

			/* New added parameters */
			0x01,                                   //!< ui_hsync_sync_width
			0x01                                    //!< ui_vsync_sync_width
		},

		// T_IDE_PARAM
		{
			/* Old prototype */
			PINMUX_LCD_SEL_GPIO,            //!< pinmux_select_lcd;
			ICST_CCIR601,                   //!< icst;
			{FALSE, FALSE},                  //!< dithering[2];
			DISPLAY_DEVICE_MIPIDSI,         //!< **DONT-CARE**
			IDE_PDIR_RGB,                   //!< pdir;
			IDE_LCD_R,                      //!< odd;
			IDE_LCD_G,                      //!< even;
			TRUE,                           //!< hsinv;
			TRUE,                           //!< vsinv;
			FALSE,                          //!< hvldinv;
			FALSE,                          //!< vvldinv;
			TRUE,                           //!< clkinv;
			FALSE,                          //!< fieldinv;
			FALSE,                          //!< **DONT-CARE**
			FALSE,                          //!< interlace;
			FALSE,                          //!< **DONT-CARE**
			0x40,                           //!< ctrst;
			0x00,                           //!< brt;
			0x40,                           //!< cmults;
			FALSE,                          //!< cex;
			FALSE,                          //!< **DONT-CARE**
			TRUE,                           //!< **DONT-CARE**
			TRUE,                           //!< tv_powerdown;
			{0x00, 0x00},                   //!< **DONT-CARE**

			/* New added parameters */
			FALSE,                          //!< yc_ex
			FALSE,                          //!< hlpf
			{FALSE, FALSE, FALSE},          //!< subpix_odd[3]
			{FALSE, FALSE, FALSE},          //!< subpix_even[3]
			{IDE_DITHER_5BITS, IDE_DITHER_6BITS, IDE_DITHER_5BITS}, //!< dither_bits[3]
			FALSE                           //!< clk1/2
		},

		(T_PANEL_CMD *)t_cmd_mode_dsi,                 //!< p_cmd_queue
		sizeof(t_cmd_mode_dsi) / sizeof(T_PANEL_CMD),  //!< n_cmd
	}
};

const T_LCD_ROT *t_rot_dsi = NULL;

//@}

T_LCD_ROT *dispdev_get_lcd_rotate_dsi_cmd(UINT32 *mode_number)
{
#if 0
	if (t_rot_dsi != NULL) {
		*mode_number = sizeof(t_rot_dsi) / sizeof(T_LCD_ROT);
	} else
#endif
	{
		*mode_number = 0;
	}
	return (T_LCD_ROT *)t_rot_dsi;
}

T_LCD_PARAM *dispdev_get_config_mode_dsi(UINT32 *mode_number)
{
	*mode_number = sizeof(t_mode_dsi) / sizeof(T_LCD_PARAM);
	return (T_LCD_PARAM *)t_mode_dsi;
}

T_PANEL_CMD *dispdev_get_standby_cmd_dsi(UINT32 *cmd_number)
{
	*cmd_number = sizeof(t_cmd_standby_dsi) / sizeof(T_PANEL_CMD);
	return (T_PANEL_CMD *)t_cmd_standby_dsi;
}


void dispdev_set_dsi_config(DSI_CONFIG *p_dsi_config)
{
#if 0
	// DSI input source clock = 480
	// Target can be 480 / 240 / 160 / 120
	FLOAT   dsi_target_clk = DSI_TARGET_CLK;
	UINT32  div;


	div = (UINT32)(p_dsi_config->f_dsi_src_clk / dsi_target_clk);

	if (div == 0) {
		NT35521_WRN_MSG("div = 0 force ++\r\n");
		div++;
	}
	pll_setClockRate(PLL_CLKSEL_DSI_CLKDIV, PLL_DSI_CLKDIV(div - 1));
#else
	dsi_set_config(DSI_CONFIG_ID_FREQ, DSI_TARGET_CLK * 1000000);
#endif
#if (DSI_TARGET_CLK == 160) //real is 150MHz
	dsi_set_config(DSI_CONFIG_ID_TLPX, 1);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 0); //FPGA: 120MHz, GO = 0
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 0); //FPGA: 120MHz, SURE = 0

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 1);
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 4);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 2);
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 3);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 1);
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 7);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 8);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 1);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
#elif(DSI_TARGET_CLK == 960)
	dsi_set_config(DSI_CONFIG_ID_TLPX, 3);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 21);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 0);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GET, 20);

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 5);//4
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 6);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 3); //7
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 6);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 5);//3
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 2);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);

	dsi_set_config(DSI_CONFIG_ID_BTA_HANDSK_TMOUT_VAL, 0x40);
#elif(DSI_TARGET_CLK == 480)
	dsi_set_config(DSI_CONFIG_ID_TLPX, 3);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 21);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 0);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GET, 20);

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 4);
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 6);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 7);
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 6);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 2);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);

	dsi_set_config(DSI_CONFIG_ID_BTA_HANDSK_TMOUT_VAL, 0x40);
#elif(DSI_TARGET_CLK == 240)
	dsi_set_config(DSI_CONFIG_ID_TLPX, 3);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 21);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_SURE, 0);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GET, 20);

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 4);
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 6);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 7);
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 6);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 16);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 2);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);

	dsi_set_config(DSI_CONFIG_ID_BTA_HANDSK_TMOUT_VAL, 0x40);
//#elif(DSI_TARGET_CLK == 120)
//    dsi_set_config(DSI_CONFIG_ID_TLPX, 0);
//    dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 3);

//    dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 1);
//    dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 2);
//    dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 1);
//    dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 1);

//    dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 0);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 5);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 8);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 1);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
//#elif(DSI_TARGET_CLK == 54)
//    dsi_set_config(DSI_CONFIG_ID_TLPX, 1);
//    dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 4);

//    dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 1);
//    dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 0);
//    dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 1);
//    dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 4);

//    dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 0);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 2);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 0);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 1);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
#endif
	dsi_set_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_3);
	dsi_set_config(DSI_CONFIG_ID_TE_BTA_INTERVAL, 0x1F);
	dsi_set_config(DSI_CONFIG_ID_CLK_PHASE_OFS, 0x3); //mask check
	//dsi_setConfig(DSI_CONFIG_ID_PHASE_DELAY_ENABLE_OFS, 0x1); //Shaun&KT: must disable clock phase delay

	dsi_set_config(DSI_CONFIG_ID_CLK_LP_CTRL, 0x0);//0x0 check
	dsi_set_config(DSI_CONFIG_ID_SYNC_DLY_CNT, 0xF);

	dsi_set_config(DSI_CONFIG_ID_EOT_PKT_EN, TRUE);//mask check

	// test lane swap 1230
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D0, 1);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D1, 2);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D2, 3);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D3, 0);

	// test lane swap 2301
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D0, 2);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D1, 3);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D2, 0);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D3, 1);

	// test lane swap 3012
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D0, 3);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D1, 0);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D2, 1);
	//dsi_set_config(DSI_CONFIG_ID_LANSEL_D3, 2);

}

#if defined __FREERTOS
int panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);

	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    DBG_DUMP("Hello, panel: tb093\n");
    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("tb093, Goodbye\r\n");
}

#elif defined __KERNEL__
static int __init panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);

	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    pr_info("Hello, panel: tb093\n");
    return 0;
}

static void __exit panel_exit(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);

	p_disp_obj->dev_callback = NULL;
    printk(KERN_INFO "Goodbye\n");
}

module_init(panel_init);
module_exit(panel_exit);

MODULE_DESCRIPTION("tb093 Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
#endif


