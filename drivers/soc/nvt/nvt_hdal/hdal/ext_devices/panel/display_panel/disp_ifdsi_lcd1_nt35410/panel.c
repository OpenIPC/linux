/*
    Display object for driving DSI device

    @file       NT35410.c
    @ingroup
    @note       This panel MUST select ide clock to PLL1 ( 480 ). Once change to \n
				another frequence, the _IDE_FDCLK should be re calculated

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dispdev_ifdsi.h"

#define PANEL_WIDTH     360
#define PANEL_HEIGHT    640

#define NT35410_IND_MSG(...)       debug_msg(__VA_ARGS__)

#define NT35410_ERR_MSG(...)       debug_msg(__VA_ARGS__)

#define NT35410_WRN_MSG(...)       debug_msg(__VA_ARGS__)
#define NT35410_TE_OFF              0
#define NT35410_TE_ON               1

#define NT35410_TE_PACKET           0
#define NT35410_TE_PIN              1

#define NT35410_NEWPANEL            1

/*
    RGB888 = 1 pixel = 3bytes packet
    If DSI src = 240MHz, internal clock = 30MHz, data rate = 30MHz x 1bytes = 30MB / sec per lane
    2 lane = 60MB = 20Mpixel ' ide need > 20MHz
*/


#define DSI_FORMAT_RGB565          0    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB666P         1    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB666L         2    //ide use 480 & DSI use 480
#define DSI_FORMAT_RGB888          3    //ide use 480 & DSI use 480

#define DSI_OP_MODE_CMD_MODE       1
#define DSI_OP_MODE_VDO_MODE       0
#define DSI_PACKET_FORMAT          DSI_FORMAT_RGB666L

//#define DSI_TARGET_CLK             480    //real chip use 480Mhz
//#define DSI_TARGET_CLK             240    //real chip use 240Mhz
#define DSI_TARGET_CLK             160  //FPGA use 160MHz
//#define DSI_TARGET_CLK             120
//#define DSI_TARGET_CLK             54
#define DSI_OP_MODE                DSI_OP_MODE_CMD_MODE

#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P) && (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
#error "Command mode not support RGB666P"
#endif

//#if (_FPGA_EMULATION_ == ENABLE)
#if defined (_NVT_FPGA_) 


#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#define _IDE_FDCLK      20000000 //(((((DSI_TARGET_CLK / 8) * 2)/3))+1)
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
#define _IDE_FDCLK      27000000 //((((((DSI_TARGET_CLK / 8) * 2))*4)/9)+1)
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
#define _IDE_FDCLK      27000000 //(((((DSI_TARGET_CLK / 8) * 2)/ 2))+1)
#endif

#else

#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#if (DSI_TARGET_CLK == 160)
//??6.66f //FPGA is 150MHz, 150/8/3 = 6.25, but PLL2 = 54, so 54/8 = 6.75
#define _IDE_FDCLK      6750000 //(((((DSI_TARGET_CLK / 8) * 2)/3))+1)
#else
#define _IDE_FDCLK      15000000//(((DSI_TARGET_CLK / 8)/3))
#endif
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)

#if (DSI_TARGET_CLK == 160)
#define _IDE_FDCLK      18000000 //FPGA is 150MHz, (150/8*2*4)/9 = 16.66f
#elif(DSI_TARGET_CLK == 120)
#define _IDE_FDCLK      6660000
#else
#define _IDE_FDCLK      15000000//((((DSI_TARGET_CLK / 8) *2 * 4)/9))
#endif

#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
if(DSI_TARGET_CLK == 120)
#define _IDE_FDCLK      16000000
#else
#define _IDE_FDCLK      38000000//(((DSI_TARGET_CLK / 8) * 2 /2) + 1)
#endif
#endif


/*
    panel Parameters for TCON NT35410
*/
//@{
/*Used in DSI*/
const T_PANEL_CMD t_cmd_mode_dsi[] = {
#if 1
	// SW reset
	{DSICMD_CMD,     0x01},
	{CMDDELAY_MS,    10},

	// Sleep out
	{DSICMD_CMD,     0x11},
	{CMDDELAY_MS,    10},

#if (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)

	// 480 x 800
	{DSICMD_CMD,     0x2A},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x01},
	{DSICMD_DATA,    0x67},
	//{CMDDELAY_MS,    10},
	{DSICMD_CMD,     0x2B},     // set page address
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x02},
	{DSICMD_DATA,    0x7F},
#else
	{DSICMD_CMD,     0xB8},
	{DSICMD_DATA,    0x28},     // Change resolution to 320 x 480
	{DSICMD_CMD,     0x2A},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x01},
	{DSICMD_DATA,    0x3F},
	{DSICMD_CMD,     0x2B},     // set page address
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x01},
	{DSICMD_DATA,    0xDF},     // 320 x 480
#endif
#if (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
	{DSICMD_CMD,     0x36},     // set memory access control
	{DSICMD_DATA,    0x00},     // bit[3] = 0, RGB filter / bit[3] = 1, BGR filter
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
	{DSICMD_CMD,     0x3A},     // Interface pixel format
	{DSICMD_DATA,    0x05},     // x101, x101 [0x55]: 16 bits / pixel
	{DSICMD_CMD,     0x3A},     // Interface pixel format
	{DSICMD_DATA,    0x55},     // x101, x101 [0x55]: 16 bits / pixel
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
	{DSICMD_CMD,     0x3A},     // Interface pixel format
	{DSICMD_DATA,    0x06},     // x101, x101 [0x55]: 16 bits / pixel
	{DSICMD_CMD,     0x3A},     // Interface pixel format
	{DSICMD_DATA,    0x66},     // x101, x101 [0x55]: 16 bits / pixel
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
	{DSICMD_CMD,     0x3A},     // Interface pixel format
	{DSICMD_DATA,    0x77},     // x101, x101 [0x55]: 16 bits / pixel
	{DSICMD_CMD,     0x3A},     // Interface pixel format
	{DSICMD_DATA,    0x77},     // x101, x101 [0x55]: 16 bits / pixel
#endif
	{DSICMD_CMD,     0xF0},     //Test mode enable
	{DSICMD_DATA,    0xAA},
	{DSICMD_DATA,    0x55},
	{DSICMD_DATA,    0x52},

	// TE always on(not auto off), DSI can not to issue 35H(set_te_on) per each frame --begin
	{DSICMD_CMD,     0xF5},
	{DSICMD_DATA,    0x33},
	{DSICMD_DATA,    0x47},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x60},
	{DSICMD_DATA,    0x9C},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x03},
	{DSICMD_DATA,    0x22},
	{DSICMD_DATA,    0x22},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x34},
	{DSICMD_DATA,    0x90},
	{DSICMD_DATA,    0x10},
	{DSICMD_DATA,    0x20},     //0x30: te off auto, 0x20: te not auto off
	// TE always on, DSI can not to issue 35H(set_te_on) per each frame --end

	// FPGA for device TLPX slower -- begin
	{DSICMD_CMD,     0xEC},
	{DSICMD_DATA,    0x60},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x23},
//#if (_FPGA_EMULATION_ == ENABLE)
#if defined (_NVT_FPGA_) 

	{DSICMD_DATA,    0x30},     //TLPX: 0x60 or 0x30
#else
	{DSICMD_DATA,    0x60},     //TLPX: 0x60 or 0x30
#endif
	// FPGA for device TLPX slower -- end

	//{CMDTEON,        NT35410_TE_ON},
	//{CMDTEEXTPIN,    NT35410_TE_PACKET},
	//{CMDTEEXTPIN,    NT35410_TE_PIN},


	//{CMDTEONEACHFRAME,0x1},
	///*{CMDTEONEACHFRAMERTY, 0x01},   //if:0x30: te off auto, must set te on one frame*/
	//{CMDBTA,         0x0},
	//{DSICMD_CMD,     0x35},     //TE-ON
#endif

#if(DSI_OP_MODE == DSI_OP_MODE_VDO_MODE)
	{DSICMD_CMD,     0xF0},     //Test mode enable
	{DSICMD_DATA,    0xAA},
	{DSICMD_DATA,    0x55},
	{DSICMD_DATA,    0x52},

	/*
	    {DSICMD_CMD,     0xEC},
	    {DSICMD_DATA,    0x60},
	    {DSICMD_DATA,    0x00},
	    {DSICMD_DATA,    0x00},
	    {DSICMD_DATA,    0x23},
	    {DSICMD_DATA,    0x60},    //LTPX: 0x60 or 0x30
	*/
	/*
	    {DSICMD_CMD,     0xC1},    //unknow command
	    {DSICMD_DATA,    0x45},
	    {DSICMD_DATA,    0x45},
	    {DSICMD_DATA,    0x45},
	    {DSICMD_DATA,    0xFF},
	    {DSICMD_DATA,    0x05},
	    {DSICMD_DATA,    0x10},
	*/
	/*
	    {DSICMD_CMD,     0xB1},     //unknow command
	    {DSICMD_DATA,    0xFF},
	    {DSICMD_DATA,    0x03},
	    {DSICMD_DATA,    0xFF},
	    {DSICMD_DATA,    0x03},
	    {DSICMD_DATA,    0x03},
	*/


	{DSICMD_CMD,     0xB0},     // switch to video mode
	{DSICMD_DATA,    0x00},
	{DSICMD_DATA,    0x7A},

	{DSICMD_CMD,     0x32},     // switch to video mode
#endif

	{DSICMD_CMD,     0x29},     // display on
	{CMDDELAY_MS,    10},

	{CMDDELAY_MS,    10},
	{CMDDELAY_MS,    10},
	{CMDDELAY_MS,    10},
#endif
};


const T_PANEL_CMD t_cmd_standby_dsi[] = {
	{DSICMD_CMD,     0x28},         // Display OFF
//	{CMDDELAY_MS,    10},
    {DSICMD_CMD,     0x10},      // Sleep in
//    {CMDDELAY_MS,    10},
};

const T_LCD_PARAM t_mode_dsi[] = {
	/***********       MI Serial Format 1      *************/
	{
		// T_PANEL_PARAM
		{
			/* Old prototype */
			//PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P,   //!< LCDMode
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P,   //!< LCDMode
			//PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L,
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L,   //!< LCDMode
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB565,
#if (DSI_OP_MODE == DSI_OP_MODE_VDO_MODE)
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB565,
			PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB565,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P,
			PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L,
			PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
			//PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB888,
			PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB888,
#endif

#else
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
			PINMUX_DSI_1_LANE_CMD_MODE_RGB565,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
			PINMUX_DSI_1_LANE_CMD_MODE_RGB666L,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
			PINMUX_DSI_1_LANE_CMD_MODE_RGB888,
#endif
#endif
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB888,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB888, //OK
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB565,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB565, //OK
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666L,
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666L,
			//PINMUX_DSI_2_LANE_VDO_SYNC_EVENT_RGB666P,
			//PINMUX_DSI_2_LANE_VDO_SYNC_PULSE_RGB666P,
			_IDE_FDCLK,                             //!< fd_clk
			(0xD0 + PANEL_WIDTH),                       //!< uiHSyncTotalPeriod
			PANEL_WIDTH,                            //!< ui_hsync_active_period
			0x68,                                       //!< uiHSyncBackPorch
			0x20 + PANEL_HEIGHT,                    //!< ui_vsync_total_period
			PANEL_HEIGHT,                           //!< ui_vsync_active_period
			0x10,                                   //!< ui_vsync_back_porch_odd
			0x10,                                   //!< ui_vsync_back_porch_even
			PANEL_WIDTH,                            //!< ui_buffer_width
			PANEL_HEIGHT,                           //!< ui_buffer_height
			PANEL_WIDTH,                            //!< ui_window_width
			PANEL_HEIGHT,                           //!< ui_window_height
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
			{TRUE, FALSE},                  //!< dithering[2];
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
		NT35410_WRN_MSG("div = 0 force ++\r\n");
		div++;
	}
	pll_setClockRate(PLL_CLKSEL_DSI_CLKDIV, PLL_DSI_CLKDIV(div - 1));
#else
	dsi_set_config(DSI_CONFIG_ID_FREQ, DSI_TARGET_CLK * 1000000);
#endif
#if (DSI_TARGET_CLK == 160) //real is 150MHz
	dsi_set_config(DSI_CONFIG_ID_TLPX, 1);
	dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 7);

	dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 1);
	dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 4);
	dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 2);
	dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 3);

	dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 1);
	dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 7);
	dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 8);
	dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 1);
	dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);

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
//#elif(DSI_TARGET_CLK == 120)
//    dsi_set_config(DSI_CONFIG_ID_TLPX, 3);
//    dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 4);

//    dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 4);
//    dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 8);
//    dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 7);
//    dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 6);

//    dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 3);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 48);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 48);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 7);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 3);
//#elif(DSI_TARGET_CLK == 54)
//    dsi_set_config(DSI_CONFIG_ID_TLPX, 1);
//    dsi_set_config(DSI_CONFIG_ID_BTA_TA_GO, 4);

//    dsi_set_config(DSI_CONFIG_ID_THS_PREPARE, 2);
//    dsi_set_config(DSI_CONFIG_ID_THS_ZERO, 0);
//    dsi_set_config(DSI_CONFIG_ID_THS_TRAIL, 7);
//    dsi_set_config(DSI_CONFIG_ID_THS_EXIT, 4);

//    dsi_set_config(DSI_CONFIG_ID_TCLK_PREPARE, 0);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_ZERO, 2);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_POST, 0);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_PRE, 1);
//    dsi_set_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
#endif
	dsi_set_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_0);
	dsi_set_config(DSI_CONFIG_ID_TE_BTA_INTERVAL, 0x1F);
	dsi_set_config(DSI_CONFIG_ID_CLK_LP_CTRL, 0x1);
	dsi_set_config(DSI_CONFIG_ID_SYNC_DLY_CNT, 0xF);
}

#if defined __FREERTOS
int panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    DBG_DUMP("Hello, panel: NT35410\n");
    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("NT35410, Goodbye\r\n");
}

#elif defined __KERNEL__
static int __init panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);
	
	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    pr_info("Hello, panel: NT35410\n");
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

MODULE_DESCRIPTION("NT35410 Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
#endif


