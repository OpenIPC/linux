/*
    Display object for driving DSI device

    @file       UT35067A0_dsi.c
    @ingroup
    @note       This panel MUST select ide clock to PLL1 ( 480 ). Once change to \n
				another frequence, the _IDE_FDCLK should be re calculated

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#include "dispdev_ifdsi.h"

#define FW_ROTATE       DISABLE//ENABLE

#if (FW_ROTATE == ENABLE)
#define PANEL_WIDTH     480
#define PANEL_HEIGHT    320
#else
#define PANEL_WIDTH     320
#define PANEL_HEIGHT    480
#endif

#define UT35067A0_IND_MSG(...)       debug_msg(__VA_ARGS__)

#define UT35067A0_ERR_MSG(...)       debug_msg(__VA_ARGS__)

#define UT35067A0_WRN_MSG(...)       debug_msg(__VA_ARGS__)

#define UT35067A0_TE_OFF              0
#define UT35067A0_TE_ON               1

#define UT35067A0_TE_PACKET           0
#define UT35067A0_TE_PIN              1


/*
    RGB888 = 1 pixel = 3bytes packet
    If DSI src = 240MHz, internal clock = 30MHz, data rate = 30MHz x 1bytes = 30MB / sec per lane
    2 lane = 60MB = 20Mpixel ' ide need > 20MHz
*/


#define DSI_FORMAT_RGB565          0    //N/A in UT35067A0
#define DSI_FORMAT_RGB666P         1    //N/A in UT35067A0
#define DSI_FORMAT_RGB666L         2    //N/A in UT35067A0
#define DSI_FORMAT_RGB888          3    //IDE use 480 & DSI use 480

#define DSI_OP_MODE_CMD_MODE       1
#define DSI_OP_MODE_VDO_MODE       0

#define DSI_PACKET_FORMAT          DSI_FORMAT_RGB888

//#define DSI_TARGET_CLK             480    //real chip use 480Mhz
#define DSI_TARGET_CLK             240    //real chip use 240Mhz
//#define DSI_TARGET_CLK             160  //FPGA use 160MHz
//#define DSI_TARGET_CLK             120
#define DSI_OP_MODE                DSI_OP_MODE_CMD_MODE

#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P) && (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
#error "Command mode not support RGB666P"
#endif


#if(DSI_PACKET_FORMAT == DSI_FORMAT_RGB888) || (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
#if(DSI_TARGET_CLK == 160)
#define _IDE_fDCLK      6.66f
#else
#if 0
#define _IDE_fDCLK      (((DSI_TARGET_CLK / 8)/3)*2)
#else
//#define _IDE_fDCLK      43000000 //(((((DSI_TARGET_CLK / 8) * 2)/3))+1)
#define _IDE_fDCLK      21500000 //(((((DSI_TARGET_CLK / 8) * 2)/3))+1)
#endif
#endif
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666P)
#if(DSI_TARGET_CLK == 160)
#define _IDE_fDCLK      8.88f
#elif(DSI_TARGET_CLK == 120)
#define _IDE_fDCLK      6.66f
#else
#define _IDE_fDCLK      ((((DSI_TARGET_CLK / 8) * 4)/9))
#endif
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
#if(DSI_TARGET_CLK == 120)
#define _IDE_fDCLK      7.5f
#else
#define _IDE_fDCLK      (((DSI_TARGET_CLK / 8)/2))
#endif
#endif




/*
    Panel Parameters for TCON UT35067A0
*/
//@{
/*Used in DSI*/
const T_PANEL_CMD t_cmd_mode_dsi[] = {
    {DSICMD_CMD,     0xF7},     //
    {DSICMD_DATA,    0xA9},     //
    {DSICMD_DATA,    0x51},     //
    {DSICMD_DATA,    0x2C},     //
    {DSICMD_DATA,    0x82},     //

    {DSICMD_CMD,     0xC0},     //  Power Control 1
    {DSICMD_DATA,    0x11},     //  Verg1out =4.52
    {DSICMD_DATA,    0x11},     //  Vreg2out = -4.12

    {DSICMD_CMD,     0xC1},     //  VGH=14.17,VGL =-9.42
    {DSICMD_DATA,    0x41},     //

    {DSICMD_CMD,     0xC5},     //
    {DSICMD_DATA,    0x00},     //
    {DSICMD_DATA,    0x17},     //
    {DSICMD_DATA,    0x80},     //

    {DSICMD_CMD,     0xB1},     //  Frame rate
    {DSICMD_DATA,    0xB0},     //  70Hz
    {DSICMD_DATA,    0x11},     //

    {DSICMD_CMD,     0xB4},     //  Display Inversion Control
    {DSICMD_DATA,    0x02},     //  2-dot

    {DSICMD_CMD,     0xB6},     //  Interface Mode Control
    {DSICMD_DATA,    0x02},     //  RGB/MCU Interface Control
    {DSICMD_DATA,    0x02},     //

    {DSICMD_CMD,     0x55},     //
    {DSICMD_DATA,    0x00},     //

    {DSICMD_CMD,     0xE9},     //
    {DSICMD_DATA,    0x00},     //

#if (DSI_OP_MODE == DSI_OP_MODE_CMD_MODE)
    {DSICMD_CMD,     0x36},     // set memory access control
    {DSICMD_DATA,    0x48},     // bit[3] = 0, RGB filter / bit[3] = 1, BGR filter
#if (DSI_PACKET_FORMAT == DSI_FORMAT_RGB565)
    #error "Not support RGB565"
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
    #error "Not support RGB666L"
#elif(DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
    {DSICMD_CMD,     0x3A},     // Interface pixel format
    {DSICMD_DATA,    0x77},     // x101, x101 [0x77]: 24 bits / pixel
#endif

    {DSICMD_CMD,     0xE0},     //
    {DSICMD_DATA,    0x00},     //
    {DSICMD_DATA,    0x06},     //
    {DSICMD_DATA,    0x06},     //
    {DSICMD_DATA,    0x08},     //
    {DSICMD_DATA,    0x18},     //
    {DSICMD_DATA,    0x0C},     //
    {DSICMD_DATA,    0x41},     //
    {DSICMD_DATA,    0x9B},     //
    {DSICMD_DATA,    0x4F},     //
    {DSICMD_DATA,    0x07},     //
    {DSICMD_DATA,    0x0E},     //
    {DSICMD_DATA,    0x0C},     //
    {DSICMD_DATA,    0x1C},     //
    {DSICMD_DATA,    0x1C},     //
    {DSICMD_DATA,    0x0F},     //

    {DSICMD_CMD,     0xE1},     //
    {DSICMD_DATA,    0x00},     //
    {DSICMD_DATA,    0x1B},     //
    {DSICMD_DATA,    0x1E},     //
    {DSICMD_DATA,    0x03},     //
    {DSICMD_DATA,    0x0E},     //
    {DSICMD_DATA,    0x04},     //
    {DSICMD_DATA,    0x35},     //
    {DSICMD_DATA,    0x24},     //
    {DSICMD_DATA,    0x49},     //
    {DSICMD_DATA,    0x04},     //
    {DSICMD_DATA,    0x0F},     //
    {DSICMD_DATA,    0x0E},     //
    {DSICMD_DATA,    0x37},     //
    {DSICMD_DATA,    0x3A},     //
    {DSICMD_DATA,    0x0F},     //

    //{CMDTEON, 1},             // Enable DSP TE (LCD_8), if the TE pin of DSI LCD doesn't connect to
    //{CMDTEEXTPIN, 1},         // DSP, don't enable it

    {DSICMD_CMD, 0x35 },         // Tearing Effect ON
    {DSICMD_DATA,  0x00},     //

    {DSICMD_CMD,     0x11},     // Sleep OUT
    {CMDDELAY_MS,    120},
    {DSICMD_CMD,     0x29},     // display on
#endif
};


const T_PANEL_CMD t_cmd_standby_dsi[] = {
    {DSICMD_CMD,     0x28},     // Display OFF
    {DSICMD_CMD,     0x10}      // Sleep in
};

const T_LCD_PARAM t_mode_dsi[] = {
    /***********       MI Serial Format 1      *************/
    {
        // tPANEL_PARAM
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
            //PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666P,
            PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666P,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB666L)
            PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB666L,
            //PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB666L,
#elif (DSI_PACKET_FORMAT == DSI_FORMAT_RGB888)
            PINMUX_DSI_1_LANE_VDO_SYNC_EVENT_RGB888,
            //PINMUX_DSI_1_LANE_VDO_SYNC_PULSE_RGB888,
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
			_IDE_fDCLK,                             //!< fd_clk
#if (FW_ROTATE == ENABLE)
            (0x68+PANEL_HEIGHT),                         //!< uiHSyncTotalPeriod
            PANEL_HEIGHT,                                //!< uiHSyncActivePeriod
            0x34,                                       //!< uiHSyncBackPorch
            0x04+PANEL_WIDTH,                           //!< uiVSyncTotalPeriod
            PANEL_WIDTH,                               //!< uiVSyncActivePeriod
            0x02,                                       //!< uiVSyncBackPorchOdd
            0x02,                                       //!< uiVSyncBackPorchEven
#else
            (0x68+PANEL_WIDTH),                         //!< uiHSyncTotalPeriod
            PANEL_WIDTH,                                //!< uiHSyncActivePeriod
            0x34,                                       //!< uiHSyncBackPorch
            0x04+PANEL_HEIGHT,                           //!< uiVSyncTotalPeriod
            PANEL_HEIGHT,                               //!< uiVSyncActivePeriod
            0x02,                                       //!< uiVSyncBackPorchOdd
            0x02,                                       //!< uiVSyncBackPorchEven
#endif
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

void dispdev_set_dsi_drv_config(DSI_CONFIG_ID id, UINT32 value)
{
	if (dsi_set_config(id, value) != E_OK)	{
		DBG_DUMP("dsi_set_config not support, id = 0x%x, value = 0x%x\r\n", (unsigned int)id, (unsigned int)value);
	}
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
        UT35067A0_WRN_MSG("Div = 0 force ++\r\n");
		div++;
	}
	pll_setClockRate(PLL_CLKSEL_DSI_CLKDIV, PLL_DSI_CLKDIV(div - 1));
#else
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_FREQ, DSI_TARGET_CLK * 1000000);
#endif
#if (DSI_TARGET_CLK == 160) //real is 150MHz
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 3);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 7);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 4);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 2);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 3);

    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 7);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 8);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);
#elif(DSI_TARGET_CLK == 240)
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 2);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 7);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 4);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 3);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 4);

    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 2);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 9);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 9);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 2);
#elif(DSI_TARGET_CLK == 120)
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TLPX, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_BTA_TA_GO, 4);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_PREPARE, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_ZERO, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_TRAIL, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_THS_EXIT, 1);

    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PREPARE, 0);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_ZERO, 4);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_POST, 7);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_PRE, 1);
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TCLK_TRAIL, 1);

#endif
    dispdev_set_dsi_drv_config(DSI_CONFIG_ID_DATALANE_NO, DSI_DATA_LANE_0);
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_TE_BTA_INTERVAL, 0x1F);
	//dispdev_set_dsi_drv_config(DSI_CONFIG_ID_CLK_PHASE_OFS, 0x3);
	//dispdev_set_dsi_drv_config(DSI_CONFIG_ID_PHASE_DELAY_ENABLE_OFS, 0x1);

	#if 0
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_CLK_LP_CTRL, 0x1);
	#else
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_CLK_LP_CTRL, 0x0);
	#endif
	dispdev_set_dsi_drv_config(DSI_CONFIG_ID_SYNC_DLY_CNT, 0xF);
}

#if defined __FREERTOS
int panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);

	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    DBG_DUMP("Hello, panel: UT35067A0\n");
    return 0;
}

void panel_exit(void)
{
    DBG_DUMP("UT35067A0, Goodbye\r\n");
}

#elif defined __KERNEL__
static int __init panel_init(void)
{
	PDISP_OBJ p_disp_obj;
	p_disp_obj = disp_get_display_object(DISP_1);

	p_disp_obj->dev_callback = &dispdev_get_lcd1_dev_obj;
    pr_info("Hello, panel: UT35067A0\n");
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

MODULE_DESCRIPTION("UT35067A0 Panel");
MODULE_AUTHOR("Novatek Corp.");
MODULE_LICENSE("GPL");
#endif


