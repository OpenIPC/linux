/***************************************************************************
 *
 * OVM OV7690 CameraCube module driver
 *
 * Copyright (C) 2012 Alexander V. Shadrin <alex.virt2real@gmail.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#define DEBUG
#define OV7675
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/slab.h>

#include <media/v4l2-device.h>
#include <media/v4l2-subdev.h>
#include <media/ov7690.h>
#include <media/v4l2-chip-ident.h>

/* The ID values we are looking for */
#define OV7690_MOD_ID			0x0046
#define OV7690_MFR_ID			0x000B

/* OV7690 has 8 I2C registers */
#define I2C_8BIT			1

/* Terminating list entry for reg */
#define I2C_REG_TERM		0xFF
/* Terminating list entry for val */
#define I2C_VAL_TERM		0xFF
/* Terminating list entry for len */
#define I2C_LEN_TERM		0xFF

/* terminating token for reg list */
#define OV7690_TOK_TERM 		0xFF

/* delay token for reg list */
#define OV7690_TOK_DELAY		100

/* CSI2 Virtual ID */
#define OV7690_CSI2_VIRTUAL_ID	0x0

#define OV7690_MIDH_MAGIC		0x7f
#define OV7690_MIDL_MAGIC		0xa2
#define OV7690_PIDH_MAGIC		0x76

/* Used registers */
#define OV7690_REG_MIDH				0x1c
#define OV7690_REG_MIDL				0x1d
//
#define OV7690_REG_PIDH				0x0a
#define OV7690_REG_PIDL				0x0b
#define OV7690_REG_GAIN				0x00
#define OV7690_REG_BGAIN			0x01
#define OV7690_REG_RGAIN			0x02
#define OV7690_REG_GGAIN			0x03
#define OV7690_REG_COM0C			0x0c
	#define SWAP_YUV        0x10	/* Swap Y/UV output sequence in YUV mode */
	#define ENABLE_OUTPUTS  0x06    /* Makes output for data and clock lines*/
#define OV7690_REG_COM0D			0x0d
#define OV7690_REG_COM0E			0x0e
#define OV7690_REG_AECH				0x0f
#define OV7690_REG_AECL				0x10
#define OV7690_CLKRC				0x11
#define OV7690_REG_COM12			0x12
#define OV7690_REG_COM13			0x13
#define OV7690_REG_COM14			0x14
#define OV7690_REG_COM15			0x15
#define OV7690_REG_COM16			0x16
#define OV7690_REG_HSTART			0x17
#define OV7690_REG_HSIZE			0x18
#define OV7690_REG_VSTART			0x19
#define OV7690_REG_VSIZE			0x1a
#define OV7690_REG_VSHFT			0x1b
#define OV7690_REG_COM20			0x20
#define OV7690_REG_AECGM			0x21
#define OV7690_REG_COM22			0x22
#define OV7690_REG_WPT				0x24
#define OV7690_REG_BPT				0x25
#define OV7690_REG_VPT				0x26
#define OV7690_REG_COM27			0x27
#define OV7690_REG_COM28			0x28
#define OV7690_REG_PLL				0x29
#define OV7690_REG_HEXHCL			0x2a
#define OV7690_REG_HEXHCH			0x2b
#define OV7690_REG_DM_LN			0x2c
#define OV7690_REG_ADVFL			0x2d
#define OV7690_REG_ADVFH			0x2e


/* OV7690_REG_COM14 */
#define GAIN_2x		0x00    /* 000 :   2x */
#define GAIN_4x		0x10    /* 001 :   4x */
#define GAIN_8x		0x20    /* 010 :   8x */
#define GAIN_16x	0x30    /* 011 :  16x */
#define GAIN_32x	0x40    /* 100 :  32x */
#define GAIN_64x	0x50    /* 101 :  64x */
#define GAIN_128x	0x60    /* 110 : 128x */



/* OV7690_REG_COM12 */
#define ITU656_ON_OFF	0x20	/* ITU656 protocol ON/OFF selection */
#define FMT_RGB565	0x04	/*      01 : RGB 565 */
				/* Output format control */
#define OFMT_RGB	0x02	/*      10 : RGB */
#define OFMT_YUV	0x00	/*	00 : YUV */

/*
 * The nominal xclk input frequency of the OV7690 is 18MHz, maximum
 * frequency is 45MHz, and minimum frequency is 6MHz.
 */
#define OV7690_XCLK_MIN   	6000000
#define OV7690_XCLK_NOM   	24000000
#define OV7690_XCLK_MAX 	24000000

/* FPS Capabilities */
#define OV7690_DEF_FPS		30

#define I2C_RETRY_COUNT		25

/* HSTART */
#define HST_VGA			0x23
#define HST_QVGA		0x3F

/* HSIZE */
#define HSZ_VGA			0xA0
#define HSZ_QVGA		0x50

/* VSTART */
#define VST_VGA			0x07
#define VST_QVGA		0x03

/* VSIZE */
#define VSZ_VGA			0xF0
#define VSZ_QVGA		0x78

/* HOUTSIZE */
#define HOSZ_VGA		0xA0
#define HOSZ_QVGA		0x50

#define OV7690_IMAGE_WIDTH	640
#define OV7690_IMAGE_HEIGHT	480
#define OV7690_QVGA_WIDTH	320
#define OV7690_QVGA_HEIGHT	240

#define SENSOR_DETECTED		1
#define SENSOR_NOT_DETECTED	0

/**
 * struct ov7690_reg - ov7690 register format
 * @reg: 8bit offset to register
 * @val: 8bit register value
 *
 * Define a structure for OV7690 register initialization values
 */
struct ov7690_reg {
	u8 	reg;
	u8 	val;
};

/**
 * struct capture_size - image capture size information
 * @width: image width in pixels
 * @height: image height in pixels
 */
struct capture_size {
	unsigned long width;
	unsigned long height;
};

/*
 * Array of image sizes supported by OV7690.  These must be ordered from
 * smallest image size to largest.
 */
const static struct capture_size ov7690_sizes[] = {
	{  320, 240 },  /* QVGA */
	{  640, 480 },  /* VGA */
};

#define NUM_IMAGE_SIZES ARRAY_SIZE(ov7690_sizes)
#define NUM_FORMAT_SIZES 2

/**
 * struct struct mipi_settings - struct for storage of sensor
 * mipi settings
 */
struct ov7690_mipi_settings {
	u16	data_lanes;
	u16	ths_prepare;
	u16	ths_zero;
	u16	ths_settle_lower;
	u16	ths_settle_upper;
};

/**
 * struct struct frame_settings - struct for storage of sensor
 * frame settings
 */
struct ov7690_frame_settings {
	u16	frame_len_lines_min;
	u16	frame_len_lines;
	u16	line_len_pck;
	u16	x_addr_start;
	u16	x_addr_end;
	u16	y_addr_start;
	u16	y_addr_end;
	u16	x_output_size;
	u16	y_output_size;
	u16	x_even_inc;
	u16	x_odd_inc;
	u16	y_even_inc;
	u16	y_odd_inc;
	u16	v_mode_add;
	u16	h_mode_add;
	u16	h_add_ave;
};

/**
 * struct struct ov7690_sensor_settings - struct for storage of
 * sensor settings.
 */
struct ov7690_sensor_settings {
	/* struct ov7690_clk_settings clk; */
	struct ov7690_mipi_settings mipi;
	struct ov7690_frame_settings frame;
};

/**
 * struct struct ov7690_clock_freq - struct for storage of sensor
 * clock frequencies
 */
struct ov7690_clock_freq {
	u32 vco_clk;
	u32 mipi_clk;
	u32 ddr_clk;
	u32 vt_pix_clk;
};

#define OV7690_DRIVER_NAME  "ov7690"
#define OV7690_MOD_NAME "ov7690: "

/*
 * Our nominal (default) frame rate.
 */
#define OV7690_FRAME_RATE 30

#define COM12_RESET	(1 << 7)

enum image_size { QVGA, VGA };
enum pixel_format { YUV, RGB565 };

static int debug;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-1)");

#ifndef OV7675
const static struct ov7690_reg initial_list[] = {
	//{ OV7690_CLKRC, 0x00 },
	//{ OV7690_REG_PLL, 0x50 },
	//{ OV7690_REG_HEXHCL, 0x30 },
	//{ OV7690_REG_HEXHCH, 0x08 },
	//{ OV7690_REG_DM_LN, 0x00 },
	//{ OV7690_REG_COM15, 0x00 },
	//{ OV7690_REG_ADVFL, 0x00 },
	//{ OV7690_REG_ADVFH, 0x00 },
	//{ I2C_REG_TERM, I2C_VAL_TERM },
	{OV7690_REG_COM12, COM12_RESET},
#if 0
	{OV7690_REG_COM0C, ENABLE_OUTPUTS|SWAP_YUV},
#else
	{OV7690_REG_COM0C, ENABLE_OUTPUTS},
#endif
#if 0
	{0x48, 0x42},//Magic reserved bits
	{0x27, 0x80},//Dark sun enable ???
	{0x64, 0x10},//Magic reserved bits
	{0x68, 0xb4},//BLC target offset
	{0x69, 0x12},//Magic reserved bits
	{0x2f, 0x60},//Magic reserved bits
	{0x41, 0x43},//Magic reserved bits
	{0x44, 0x24},//Magic reserved bits
	{0x4b, 0x0e},//Magic reserved bits
	{0x4c, 0x7b},//Magic reserved bits
	{0x4d, 0x0a},//Magic reserved bits
	{OV7690_REG_PLL, 0x50},
	{OV7690_REG_VSHFT, 0x19},//Pixel shift
	{0x39, 0x80},//Magic reserved bits
	{0x80, 0x7f},//Automatic modes - WB, lens correction and so on
	{0x81, 0xff},//Automatic modes special digital effects and so on
	{0x91, 0x20},//AWB control registers
	{OV7690_REG_AECGM, 0x44},//AECGM
	{OV7690_CLKRC, 0x01},
	{OV7690_REG_COM12, OFMT_YUV},
	{0x82, 0x03},//Setting YUV422 format
	{0xd0, 0x248},//Boundary offset
	{OV7690_REG_HEXHCH, 0x38},//dummy pixels in h-direction h-byte
	{OV7690_REG_COM15, 0x14},//Auto frame rate????
	{OV7690_REG_COM16, 0x03},
	{OV7690_REG_HSTART, 0x69},
	{OV7690_REG_HSIZE, 0xa4},
	{OV7690_REG_VSTART, 0x0c},
	{OV7690_REG_VSIZE, 0xf6},
	{0x3e, 0x30},//PCLK settings ???
	{0xc8, 0x02},//horizontal input size msb
	{0xc9, 0x80},//horizontal input size lsb
	{0xca, 0x01},//vertical input size msb
	{0xcb, 0xe0},//vertical input size lsb
	{0xcc, 0x02},//horizontal output size msb
	{0xcd, 0x80},//horizontal output size lsb
	{0xce, 0x01},//vertical output size msb
	{0xcf, 0xe0},//vertical output size lsb
	{0x80, 0x7F},////Automatic modes - WB, lens correction and so on again
	{0x85, 0x10},//LENC gain enable
	{0x86, 0x00},//radius of circle where no compensation is applid
	{0x87, 0x00},//x-coordinate for lens correction
	{0x88, 0x00},//y-coordinate for lens correction
	//{0x89, 0x35},//r-channel comp coeff
	//{0x8a, 0x30},//g-channel comp coeff
	//{0x8b, 0x33},//b=channel comp coeff
	//{0xbb, 0xbe},//color matrix coeff 1
	//{0xbc, 0xc0},//color matrix coeff 2
	//{0xbd, 0x02},//color matrix coeff 3
	//{0xbe, 0x16},//color matrix coeff 4
	//{0xbf, 0xc2},//color matrix coeff 5
	//{0xc0, 0xd9},//color matrix coeff 6
	{0xc1, 0x1e},//regc1 - default value
	{0xb4, 0x36},//strange description
	{0xb5, 0x06},//DNS_TH_MAN ???
	{0xb7, 0x00},//OFFSET???
	{0x8f, 0x19},//AWB Control registers
	{0x90, 0x50},//AWB Control registers
	{0x91, 0x21},//AWB Control registers
	{0x92, 0x9c},//AWB Control registers
	{0x93, 0x9b},//AWB Control registers
	{0x94, 0x0c},//AWB Control registers
	{0x95, 0x0d},//AWB Control registers
	{0x96, 0xff},//AWB Control registers
	{0x97, 0x00},//AWB Control registers
	{0x98, 0x3f},//AWB Control registers
	{0x99, 0x30},//AWB Control registers
	{0x9a, 0x4d},//AWB Control registers
	{0x9b, 0x3d},//AWB Control registers
	{0x9c, 0xf0},//AWB Control registers
	{0x9d, 0xf0},//AWB Control registers
	{0x9e, 0xf0},//AWB Control registers
	{0x9f, 0xff},//AWB Control registers
	{0xa0, 0x5f},//AWB Control registers
	{0xa1, 0x61},//AWB Control registers
	{0xa2, 0x0c},//AWB Control registers
	//here we set 30fps
	{OV7690_REG_COM14, 0x21},//Automatic gain 16X, 50Hz filter
	{OV7690_REG_COM13, 0xf7},//AEC full enable
	{OV7690_CLKRC, 0x00},//Internal clock prescaler to 1
	{OV7690_REG_PLL, 0x50},//???
	{OV7690_REG_HEXHCL, 0x30},
	{OV7690_REG_HEXHCH, 0x08},
	{OV7690_REG_DM_LN, 0x00},
	{OV7690_REG_COM15, 0x00},//Auto frema rate off
	{OV7690_REG_ADVFL, 0x00},//
	{OV7690_REG_ADVFH, 0x00},
#endif
	{0xff, 0xff}	/* END MARKER */
};
#else 
const static struct ov7690_reg initial_list[] = {
    {OV7690_REG_COM12, COM12_RESET},	
    {0x11,0x00}, 
    {0x3a,0x0C},
    {0x3D,0xC0},
    {0x12,0x00},
    {0x15,0x00},
    {0xc1,0x7f},
    {0x17,0x13},
    {0x18,0x01},
    {0x32,0xbF},
    {0x19,0x03},
    {0x1a,0x7c},
    {0x03,0x0a},
    {0x0c,0x00},
    {0x3e,0x00},
    {0x70,0x3a},
    {0x71,0x35},
    {0x72,0x11},
    {0x73,0xf0},
    {0xa2,0x02},
    {0x7a,0x24},
    {0x7b,0x04},
    {0x7c,0x07},
    {0x7d,0x10},
    {0x7e,0x28},
    {0x7f,0x36},
    {0x80,0x44},
    {0x81,0x52},
    {0x82,0x60},
    {0x83,0x6c},
    {0x84,0x78},
    {0x85,0x8c},
    {0x86,0x9e},
    {0x87,0xbb},
    {0x88,0xd2},
    {0x89,0xe5},
    {0x13,0xe0},
    {0x00,0x00},
    {0x10,0x00},
    {0x0d,0x50},
    {0x42,0x40},
    {0x14,0x38},
    {0xa5,0x05},
    {0xab,0x07},
    {0x24,0x54}, 
    {0x25,0x4c}, 
    {0x26,0x82},   
    {0x9f,0x78},
    {0xa0,0x68},
    {0xa1,0x03},
    {0xa6,0xD8},
    {0xa7,0xD8},
    {0xa8,0xf0},
    {0xa9,0x90},
    {0xaa,0x14},
    {0x13,0xe5},
    {0x0e,0x61},
    {0x0f,0x4b},
    {0x16,0x02},
    {0x1e,0x07},
    {0x21,0x02},
    {0x22,0x91},
    {0x29,0x07},
    {0x33,0x0b},
    {0x35,0x0b},
    {0x37,0x1d},
    {0x38,0x71},
    {0x39,0x2a},
    {0x3c,0x78},
    {0x4d,0x40},
    {0x4e,0x20},
    {0x69,0x00},
    {0x6b,0x0a},
    {0x74,0x10},
    {0x8d,0x4f},
    {0x8e,0x00},
    {0x8f,0x00},
    {0x90,0x00},
    {0x91,0x00},
    {0x96,0x00},
    {0x9a,0x80},
    {0xb0,0x84},
    {0xb1,0x0c},
    {0xb2,0x0e},
    {0xb3,0x82},
    {0xb8,0x0a},
    {0x43,0x0a},
    {0x44,0xf0},
    {0x45,0x34},
    {0x46,0x58},
    {0x47,0x28},
    {0x48,0x3A},
    {0x59,0x88},
    {0x5a,0x88},
    {0x5b,0xe4},
    {0x5c,0x60},
    {0x5d,0x49},
    {0x5e,0x20},
    {0x6c,0x0a},
    {0x6d,0x55},
    {0x6e,0x11},
    {0x6f,0x9e},
    {0x6a,0x40},
    {0x01,0x56},
    {0x02,0x44},
    {0x13,0xe7},    
    {0x4f,0x73},
    {0x50,0x73},
    {0x51,0x00},
    {0x52,0x1F},
    {0x53,0x55},	
    {0x54,0x73},	
    {0x55,0x00},
    {0x56,0x40},
    {0x57,0x80},
    {0x58,0x9e},
    {0x3f,0x02},
    {0x75,0x23},
    {0x76,0xe1},
    {0x4c,0x00},
    {0x77,0x01},
    {0x3D,0xC2},
    {0x4b,0x09},
    {0xc9,0x60},
    {0x41,0x38},    
    {0x56,0x40},
    {0x34,0x11},
    {0x3b,0x0a},   
    {0xa4,0x89},
    {0x96,0x00},
    {0x97,0x30},
    {0x98,0x20},
    {0x99,0x30},
    {0x9a,0x84},
    {0x9b,0x29},
    {0x9c,0x03},
    {0x9d,0x4c},
    {0x9e,0x3f},
    {0x78,0x04},
    {0x79,0x01},
    {0xc8,0xf0},
    {0x79,0x0f},
    {0xc8,0x00},
    {0x79,0x10},
    {0xc8,0x7e},
    {0x79,0x0a},
    {0xc8,0x80},
    {0x79,0x0b},
    {0xc8,0x01},
    {0x79,0x0c},
    {0xc8,0x0f},
    {0x79,0x0d},
    {0xc8,0x20},
    {0x79,0x09},
    {0xc8,0x80},
    {0x79,0x02},
    {0xc8,0xc0},
    {0x79,0x03},
    {0xc8,0x40},
    {0x79,0x05},
    {0xc8,0x30},
    {0x79,0x26},    
    {0x62,0x00},
    {0x63,0x00},
    {0x64,0x10},
    {0x65,0x00},
    {0x66,0x05},
    {0x94,0x10},   
    {0x95,0x13},  
    {0xbb, 0xa1},
    {0x7a, 0x28}, 
    {0x7b, 0x04}, 
    {0x7c, 0x09}, 
    {0x7d, 0x16}, 
    {0x7e, 0x30}, 
    {0x7f, 0x3E}, 
    {0x80, 0x4B}, 
    {0x81, 0x59}, 
    {0x82, 0x67}, 
    {0x83, 0x72}, 
    {0x84, 0x7c}, 
    {0x85, 0x8e}, 
    {0x86, 0x9e}, 
    {0x87, 0xB6}, 
    {0x88, 0xcc}, 
    {0x89, 0xE2}, 
    {0x4f, 0x96},
    {0x50, 0x9b},
    {0x51, 0x05},
    {0x52, 0x1a},
    {0x53, 0x7f},
    {0x54, 0x99},
    {0x58, 0x1a},
    {0x4f, 0x7d},
    {0x50, 0x81},
    {0x51, 0x04},
    {0x52, 0x23},
    {0x53, 0x5a},
    {0x54, 0x7d},
    {0x58, 0x1a},
    {0x43, 0x0a},
    {0x44, 0xf2},
    {0x45, 0x4a},
    {0x46, 0x62},
    {0x47, 0x2a},
    {0x48, 0x3d},
    {0x59, 0xa6},
    {0x5a, 0xad},
    {0x5b, 0xbb},
    {0x5c, 0x7c},
    {0x5d, 0x53},
    {0x5e, 0x12},
    {0x6c, 0x0a},
    {0x6d, 0x65},
    {0x6e, 0x11},
    {0x6f, 0x94}, 
    {0x62,0x00},//0x00 for ca513
    {0x63,0x00},//0x00 for ca513
    {0x64,0x0e},//0x0e for ca513
    {0x65,0x00},
    {0x66,0x05},
    {0x94,0x0d},
    {0x95,0x11},    
    {0x24, 0x5e}, 
    {0x25, 0x54}, 
    {0x26, 0x93}, 
    {0x41, 0x38},
    {0x75, 0x64},  
    {0x76, 0xe1}, 
    {0x77, 0x01},  
    {0xc9, 0x30},  
    {0x61, 0x43},
    {0x11, 0x40},
    {0x13, 0xff},
    {0x14, 0x58},
    {0x75, 0x01},
    {0xff, 0xff}	/* END MARKER */
};
#endif
/*
 * register setting for color format
 */
static const struct ov7690_reg ov7690_RGB565_regs[] = {
	{ OV7690_REG_COM0C, ENABLE_OUTPUTS },//0x06 to enable out
	{ OV7690_REG_COM12, FMT_RGB565 | OFMT_RGB },
	{ I2C_REG_TERM, I2C_VAL_TERM },
};

static const struct ov7690_reg ov7690_YYUV_regs[] = {
#if 0
	{ OV7690_REG_COM0C, SWAP_YUV|ENABLE_OUTPUTS },
#else
	//{ OV7690_REG_COM0C, ENABLE_OUTPUTS | 1},
	{ OV7690_REG_COM0C, ENABLE_OUTPUTS},
#endif
	{ OV7690_REG_COM12, OFMT_YUV },
	{ I2C_REG_TERM, I2C_VAL_TERM },
};
//For any sake
//static const struct ov7690_reg ov7690_UVYY_regs[] = {
//	{ OV7690_REG_COM0C, 0x00 },
//	{ OV7690_REG_COM12, OFMT_YUV },
//	{ I2C_REG_TERM, I2C_VAL_TERM },
//};

static const struct ov7690_reg ov7690_qvga_regs[] = {
	{ 0x16, 0x03 },
	{ 0x17, 0x69 },
	{ 0x18, 0xa4 },
	{ 0x19, 0x06 },
	{ 0x1a, 0xf6 },
	{ 0x22, 0x10 },
	{ 0xc8, 0x02 },
	{ 0xc9, 0x80 },
	{ 0xca, 0x00 },
	{ 0xcb, 0xf0 },
	{ 0xcc, 0x01 },
	{ 0xcd, 0x40 },
	{ 0xce, 0x00 },
	{ 0xcf, 0xf0 },
	{ I2C_REG_TERM, I2C_VAL_TERM },
};

static const struct ov7690_reg ov7690_vga_regs[] = {
	{ OV7690_REG_COM16, 0x03 },//Magic reserved bits
	{OV7690_REG_HSTART, 0x69},
	{OV7690_REG_HSIZE, 0xa4},
	{OV7690_REG_VSTART, 0x0c},
	{OV7690_REG_VSIZE, 0xf6},
	{OV7690_REG_COM22, 0x00},//Optical black ouput disable
	{ 0xc8, 0x02 },//horizontal input size msb
	{ 0xc9, 0x80 },//horizontal input size lsb
	{ 0xca, 0x01 },//vertical input size msb
	{ 0xcb, 0xe0 },//vertical input size lsb
	{ 0xcc, 0x02 },//horizontal output size msb
	{ 0xcd, 0x80 },//horizontal output size lsb
	{ 0xce, 0x01 },//vertical output size msb
	{ 0xcf, 0xe0 },//vertical output size lsb
	{ I2C_REG_TERM, I2C_VAL_TERM },
};

const static struct ov7690_reg *ov7690_reg_format_init[NUM_FORMAT_SIZES] = {
	ov7690_YYUV_regs, ov7690_RGB565_regs,
};

const static struct ov7690_reg *ov7690_reg_size_init[NUM_IMAGE_SIZES] = {
	ov7690_qvga_regs, ov7690_vga_regs,
};
//Forward declaration of driver operations
static int ov7690_get_control(struct v4l2_subdev *, struct v4l2_control *);
static int ov7690_set_control(struct v4l2_subdev *, struct v4l2_control *);
static int ov7690_query_control(struct v4l2_subdev *, struct v4l2_queryctrl *);

static enum image_size ov7690_find_size(unsigned int width, unsigned int height){
	enum image_size isize;
	unsigned long pixels = width * height;
	printk("ovm7690 find size\r\n");
	for (isize = QVGA; isize < VGA; isize++) {
		if (ov7690_sizes[isize + 1].height *
			ov7690_sizes[isize + 1].width > pixels)
			return isize;
	}
	return VGA;
}

struct ov7690 {
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
};
static inline struct ov7690 *to_ov7690(struct v4l2_subdev *sd){
	return container_of(sd, struct ov7690, sd);//Funny thing works
}

static struct i2c_driver ov7690_i2c_driver;
static unsigned long xclk_current = OV7690_XCLK_NOM;

/* list of image formats supported by ov7690 sensor */
const static struct v4l2_fmtdesc ov7690_formats[] = {
	{
		.description	= "RGB565",
		.pixelformat	= V4L2_PIX_FMT_RGB565,
	},
	{
		.description	= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	}
};

#define NUM_CAPTURE_FORMATS ARRAY_SIZE(ov7690_formats)

static int ov7690_read_reg(struct i2c_client *client, u8 reg, u8 *val){
	int ret;
	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret >= 0) {
		*val = (unsigned char) ret;
                ret = 0;
        }
	printk("ov7690 read reg: client: %x, addr: %x, reg: %x, val: %x, ret: %x\r\n", (unsigned int)client->adapter, client->addr, reg, *val, ret);
	return ret;
}

static int ov7690_write_reg(struct i2c_client *client, u8 reg, u8 val){
	int ret = 0;
	if (!client->adapter) return -ENODEV;
	//printk("ov7690 write reg: client: %x, addr: %x, reg: %x, val: %x\r\n", (unsigned int)client->adapter, client->addr, reg, val);
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (reg == OV7690_REG_COM12 && (val & COM12_RESET)) msleep(5); /* Wait for reset to run */
	return ret;
}

static int ov7690_write_regs(struct v4l2_subdev *sd, struct ov7690_reg *vals){
	int err = 0;
	const struct ov7690_reg *list = vals;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	while (!((list->reg == I2C_REG_TERM) && (list->val == I2C_VAL_TERM))) {
		err = ov7690_write_reg(client, list->reg,list->val);
		if (err) return err;
		msleep(1);
		list++;
	}
	return 0;
}

static unsigned long ov7690sensor_calc_xclk(void){
	xclk_current = OV7690_XCLK_NOM;
	return xclk_current;
}

static int ov7690_configure(struct v4l2_subdev *sd){
	struct ov7690 *ov7690 = to_ov7690(sd);
	struct v4l2_pix_format *pix = &ov7690->pix;
	enum image_size isize;
	int err = 0;
	enum pixel_format pfmt = YUV;
	unsigned long xclk;

	printk("%s: Configuring \n", __func__);
	printk("Configuring ov7690_configure\n");
#ifdef OV7675
	printk("Configuring ov7695_chip\n");
        return 0;
#endif
	switch (pix->pixelformat) {
		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB565X:
			pfmt = RGB565;
			break;
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		default:
			pfmt = YUV;
			break;
	}
	xclk = ov7690sensor_calc_xclk();
	isize = ov7690_find_size(pix->width, pix->height);
	/* configure pixel format */
	err = ov7690_write_regs(sd, (struct ov7690_reg *)(ov7690_reg_format_init[pfmt]) );
	if (err){
		printk("Configure made error1 %d\r\n", err);
		return err;
	}
	/* configure size */
	err = ov7690_write_regs(sd, (struct ov7690_reg *)(ov7690_reg_size_init[isize]) );
	if (err){
		printk("Configure made error2 %d\r\n", err);
		return err;
	}
	return 0;
}

static int ov7690_init(struct v4l2_subdev *sd, u32 val){
	/* Reset and wait two milliseconds */
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	printk("ov7690 initialization function\r\n");
	ov7690_write_reg(client, OV7690_REG_COM12, COM12_RESET);
    msleep(5);
	return ov7690_write_regs(sd, (struct ov7690_reg *)initial_list );
}

//All that clear about controls
static int ov7690_query_control(struct v4l2_subdev *sd, struct v4l2_queryctrl *qctr){
#if 0
	printk("ov7690 ioctl_queryctrl dummy method\r\n");
	printk("Def val %x\r\n",qctr->default_value);
	printk("Name %s\r\n", qctr->name);
	printk("Type %d\r\n", qctr->type);
	printk("ID 0x%08x\r\n", qctr->id);
	//printk("", gctr->)
#endif
	return -EINVAL;
}

static int ov7690_get_control(struct v4l2_subdev *sd, struct v4l2_control *ctrl){
	printk("ov7690 ioctl_g_ctrl dummy method\r\n");
	return -EINVAL;
}

static int ov7690_set_control(struct v4l2_subdev *sd,struct v4l2_control *ctrl){
	int retval = -EINVAL;
	printk("ov7690 ioctl_s_ctrl dummy method\r\n");
	return retval;
}

static int ov7690_enum_format(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt){
	int index = fmt->index;
	enum v4l2_buf_type type = fmt->type;
	//
	memset(fmt, 0, sizeof(*fmt));
	fmt->index = index;
	fmt->type = type;
	//
	printk("%s: Enum format capability\n", __func__);
	printk("ov7690 ioctl_enum_fmt_cap\r\n");
	//
	switch (fmt->type) {
		case V4L2_BUF_TYPE_VIDEO_CAPTURE:
			if (index >= NUM_CAPTURE_FORMATS) return -EINVAL;
			break;
		default:
			return -EINVAL;
	}
	fmt->flags = ov7690_formats[index].flags;
	strlcpy(fmt->description, ov7690_formats[index].description, sizeof(fmt->description));
	fmt->pixelformat = ov7690_formats[index].pixelformat;
	return 0;
}

static int ov7690_set_stream(struct v4l2_subdev *sd, int enable){
	printk("OV7690 set stream - dummy method\r\n");
	return 0;
}

static int ov7690_try_format(struct v4l2_subdev *sd, struct v4l2_format *f){
	enum image_size isize;
	int ifmt;
	struct v4l2_pix_format *pix = &f->fmt.pix;
	char* colorspace = 0;
	switch(f->fmt.pix.colorspace){
		case V4L2_COLORSPACE_SMPTE170M 		: colorspace = "V4L2_COLORSPACE_SMPTE170M"; break;
		case V4L2_COLORSPACE_SMPTE240M 		: colorspace = "V4L2_COLORSPACE_SMPTE240M"; break;
		case V4L2_COLORSPACE_REC709    		: colorspace = "V4L2_COLORSPACE_REC709"; break;
		case V4L2_COLORSPACE_BT878     		: colorspace = "V4L2_COLORSPACE_BT878"; break;
		case V4L2_COLORSPACE_470_SYSTEM_M 	: colorspace = "V4L2_COLORSPACE_470_SYSTEM_M"; break;
		case V4L2_COLORSPACE_470_SYSTEM_BG 	: colorspace = "V4L2_COLORSPACE_470_SYSTEM_BG"; break;
		case V4L2_COLORSPACE_JPEG         	: colorspace = "V4L2_COLORSPACE_JPEG"; break;
		case V4L2_COLORSPACE_SRGB          	: colorspace = "V4L2_COLORSPACE_SRGB"; break;
		default								: colorspace = "NOT SET"; break;
	}
#if 0
  	printk("ov7690 try format:\r\n bytes per line: %d\r\npixelformat %c%c%c%c\r\n, colorspace %s\r\n, field %d\r\n, height %d, width %d, sizeimage %d\r\n",
  			f->fmt.pix.bytesperline,
  			((f->fmt.pix.pixelformat)&0xff),(((f->fmt.pix.pixelformat)>>8)&0xff),(((f->fmt.pix.pixelformat)>>16)&0xff),(((f->fmt.pix.pixelformat)>>24)&0xff),
  			colorspace,
  			f->fmt.pix.field,
  			f->fmt.pix.height,
  			f->fmt.pix.width,
  			f->fmt.pix.sizeimage
  	);
#endif
	isize = ov7690_find_size(pix->width, pix->height);
	pix->width = ov7690_sizes[isize].width;
	pix->height = ov7690_sizes[isize].height;

	printk("%s: Trying format\n", __func__);

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == ov7690_formats[ifmt].pixelformat) break;
	}

	if (ifmt == NUM_CAPTURE_FORMATS) ifmt = 0;

	pix->pixelformat = ov7690_formats[ifmt].pixelformat;
	pix->field = V4L2_FIELD_NONE;
	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->priv = 0;
	switch (pix->pixelformat) {
		case V4L2_PIX_FMT_YUYV:
		case V4L2_PIX_FMT_UYVY:
		default:
			pix->colorspace = V4L2_COLORSPACE_JPEG;
			break;
		case V4L2_PIX_FMT_RGB565:
		case V4L2_PIX_FMT_RGB565X:
			pix->colorspace = V4L2_COLORSPACE_SRGB;
			break;
	}
	return 0;
}

static int ov7690_set_format(struct v4l2_subdev *sd, struct v4l2_format *f){
	struct ov7690 *ov7690 = to_ov7690(sd);
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int rval;
	printk("ov7690 set format\r\n");
	rval = ov7690_try_format(sd, f);
	if (rval) {
		printk("%s: Error trying format\n", __func__);
		return rval;
	}
	rval = ov7690_configure(sd);
	if (!rval) {
		ov7690->pix = *pix;
	} else {
		printk("%s: Error configure format %d\n", __func__, rval);
	}
	return rval;
}

static int ov7690_get_format(struct v4l2_subdev *sd, struct v4l2_format *f){
	struct ov7690 *ov7690 = to_ov7690(sd);
	f->fmt.pix = ov7690->pix;
	printk("ov7690 ioctl_g_fmt_cap\r\n");
	return 0;
}

static int ov7690_get_param(struct v4l2_subdev *sd, struct v4l2_streamparm *a){
	struct ov7690 *ov7690 = to_ov7690(sd);
	struct v4l2_captureparm *cparm = &a->parm.capture;
	printk("ov7690 ioctl_g_parm\r\n");
	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)	return -EINVAL;
	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cparm->capability = V4L2_CAP_TIMEPERFRAME;
	cparm->timeperframe = ov7690->timeperframe;
	return 0;
}

static int ov7690_get_chip_id(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *id){
	struct i2c_client *client = v4l2_get_subdevdata(sd);;
	printk("ov7690 get chipid\r\n");
	if (id->match.type != V4L2_CHIP_MATCH_I2C_ADDR){
		printk("match type fails\r\n");
		return -EINVAL;
	}
	if (id->match.addr != client->addr){
		printk("match addr fails\r\n");
		return -ENODEV;
	}
	id->ident	= V4L2_IDENT_OV7690;
	id->revision	= 0;
	printk("ov7690 chip id ok\r\n");
	return 0;
}

static const struct v4l2_subdev_core_ops ov7690_core_ops = {
	.g_chip_ident = ov7690_get_chip_id,
	.init         = ov7690_init,
	.queryctrl    = ov7690_query_control,
	.g_ctrl	      = ov7690_get_control,
	.s_ctrl	      = ov7690_set_control,
};

static const struct v4l2_subdev_video_ops ov7690_video_ops = {
	.s_fmt    = ov7690_set_format,
	.g_fmt    = ov7690_get_format,//Check it correct
	.try_fmt  = ov7690_try_format,
	.s_stream = ov7690_set_stream,
	.enum_fmt = ov7690_enum_format,//Check it correct
	.g_parm   = ov7690_get_param//Check it correct
};

static const struct v4l2_subdev_ops ov7690_ops = {
	.core = &ov7690_core_ops,
	.video = &ov7690_video_ops,
};

static int ov7690_detect(struct i2c_client *client)
{
//Shadrin todo improve: may check model ID also
	u8 midh, midl;
	printk("Detect ov7690\r\n");
	if (!client) return -ENODEV;

	if (ov7690_read_reg(client, OV7690_REG_MIDH, &midh)) return -ENODEV;
	if (ov7690_read_reg(client, OV7690_REG_MIDL, &midl)) return -ENODEV;

	v4l_info(client, "model id detected 0x%02x%02x\n", midh, midl);
	printk("model id detected 0x%02x%02x\n", midh, midl);
	if ((midh != OV7690_MIDH_MAGIC)|| (midl != OV7690_MIDL_MAGIC)) {
		return -ENODEV;
	}
	return 0;
}

static int ov7690_probe(struct i2c_client *client, const struct i2c_device_id *id){
	struct ov7690 *ov7690;
	struct v4l2_subdev *sd;
	int ret;
	printk("ov7690 probing\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->adapter->dev,"I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE_DATA\n");
		printk("OV7690 failed to check i2c func\r\n");
		return -EIO;
	}

	if (!client->dev.platform_data) {
		dev_err(&client->dev, "No platform data!!\n");
		printk("OV7690 no platform data\r\n");
		return -ENODEV;
	}
	ov7690 = kzalloc(sizeof(struct ov7690), GFP_KERNEL);
	if (!ov7690) return -ENOMEM;
	ret = ov7690_detect(client);
	if (ret){
		printk("OV7690 detection failed\r\n");
		goto clean;
	}
	//Filling  ov7690 data stucture
	ov7690->pix.width = OV7690_IMAGE_WIDTH;
	ov7690->pix.height = OV7690_IMAGE_HEIGHT;
	ov7690->pix.pixelformat = V4L2_PIX_FMT_YUYV;
	ov7690->timeperframe.numerator = 1;
	ov7690->timeperframe.denominator = OV7690_FRAME_RATE;
		//Register at V4L2 as subdevice
	sd = &ov7690->sd;
	//
	v4l2_i2c_subdev_init(sd, client, &ov7690_ops);
	//
	ret = ov7690_init(sd,0);//Last parameter is just dummy value
    ov7690_configure(sd);//set initial parameters for sensor
	printk("%s decoder driver registered !!\r\n", sd->name);
	return 0;
clean:
	kfree(ov7690);
	printk("OV7690 probe failed\r\n");
	return ret;
}

static int ov7690_remove(struct i2c_client *client){
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov7690 *ov7690 = to_ov7690(sd);
	printk("OVM7690 Camera Cube driver remove\r\n");
	v4l2_device_unregister_subdev(sd);
	kfree(ov7690);
	return 0;
}

static const struct i2c_device_id ov7690_id[] = {
	{ OV7690_DRIVER_NAME, 0 },
	{ },
};
MODULE_DEVICE_TABLE(i2c, ov7690_id);

static struct i2c_driver ov7690_i2c_driver = {
	.driver = {
		.name = OV7690_DRIVER_NAME,
	},
	.probe = ov7690_probe,
	.remove = ov7690_remove,
	.id_table = ov7690_id,
};

static int __init ov7690driver_init(void){
	int err;
	printk("OVM7690 Camera Cube secsor init\r\n");
	err = i2c_add_driver(&ov7690_i2c_driver);
	if (err) {
		printk("Failed to register" OV7690_DRIVER_NAME ".\n");
		return err;
	}
	return 0;
}

static void __exit ov7690driver_cleanup(void){
	printk("OVM7690 Camera Cube driver cleanup\r\n");
	i2c_del_driver(&ov7690_i2c_driver);
}

module_init(ov7690driver_init);
module_exit(ov7690driver_cleanup);

MODULE_LICENSE("GPL V2");
MODULE_AUTHOR("Alexander V. Shadrin, alex.virt2real@gmail.com");
MODULE_DESCRIPTION("ov7690 camera sensor driver");
