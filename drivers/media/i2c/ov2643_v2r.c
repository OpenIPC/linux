/***************************************************************************
 *
 * OV OV2643 CameraCube module driver
 *
 * Copyright (C) VIRT@REAL <info@virt2real.com>
 *
 * This file is licensed under the terms of the GNU General Public License
 * version 2. This program is licensed "as is" without any warranty of any
 * kind, whether express or implied.
 */

#include <linux/videodev2.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/log2.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <media/v4l2-device.h>
#include <media/v4l2-common.h>
#include <media/v4l2-chip-ident.h>
#include <media/davinci/videohd.h>


#ifdef CONFIG_V2R_DEBUG
    #define OV2643_DEBUG
#endif

/* OV2643 has 8 I2C registers */
#define I2C_8BIT			1


#define OV2643_TERM_REG 		0xFF
#define OV2643_TERM_VAL 		0xFF



//Camera ID
#define OV2643_PIDH_MAGIC		0x26
#define OV2643_PIDL_MAGIC		0x43

//Camear ID registers
#define OV2643_REG_PIDH				0x0a
#define OV2643_REG_PIDL				0x0b
//Camera functional registers
#define OV2643_REG_GAIN				0x00
#define OV2643_REG_BGAIN			0x01
#define OV2643_REG_RGAIN			0x02
#define OV2643_REG_GGAIN			0x03
#define OV2643_REG_COM0C			0x0c
	#define SWAP_YUV        0x10	/* Swap Y/UV output sequence in YUV mode */
	#define ENABLE_OUTPUTS  0x06    /* Makes output for data and clock lines*/
#define OV2643_REG_COM0D			0x0d
#define OV2643_REG_COM0E			0x0e
#define OV2643_REG_AECH				0x0f
#define OV2643_REG_AECL				0x10
#define OV2643_CLKRC				0x11
#define OV2643_REG_COM12			0x12
#define OV2643_REG_COM13			0x13
#define OV2643_REG_COM14			0x14
#define OV2643_REG_COM15			0x15
#define OV2643_REG_COM16			0x16
#define OV2643_REG_HSTART			0x17
#define OV2643_REG_HSIZE			0x18
#define OV2643_REG_VSTART			0x19
#define OV2643_REG_VSIZE			0x1a
#define OV2643_REG_VSHFT			0x1b
#define OV2643_REG_COM20			0x20
#define OV2643_REG_AECGM			0x21
#define OV2643_REG_COM22			0x22
#define OV2643_REG_WPT				0x24
#define OV2643_REG_BPT				0x25
#define OV2643_REG_VPT				0x26
#define OV2643_REG_COM27			0x27
#define OV2643_REG_COM28			0x28
#define OV2643_REG_PLL				0x29
#define OV2643_REG_HEXHCL			0x2a
#define OV2643_REG_HEXHCH			0x2b
#define OV2643_REG_DM_LN			0x2c
#define OV2643_REG_ADVFL			0x2d
#define OV2643_REG_ADVFH			0x2e


/* OV2643_REG_COM14 */
#define GAIN_2x		0x00    /* 000 :   2x */
#define GAIN_4x		0x10    /* 001 :   4x */
#define GAIN_8x		0x20    /* 010 :   8x */
#define GAIN_16x	0x30    /* 011 :  16x */
#define GAIN_32x	0x40    /* 100 :  32x */
#define GAIN_64x	0x50    /* 101 :  64x */
#define GAIN_128x	0x60    /* 110 : 128x */



/* OV2643_REG_COM12 */
#define ITU656_ON_OFF	0x20	/* ITU656 protocol ON/OFF selection */
#define FMT_RGB565	0x04	/*      01 : RGB 565 */
				/* Output format control */
#define OFMT_RGB	0x02	/*      10 : RGB */
#define OFMT_YUV	0x00	/*	00 : YUV */

/*
 * The nominal xclk input frequency of the OV2643 is 18MHz, maximum
 * frequency is 45MHz, and minimum frequency is 6MHz.
 */
#define OV2643_XCLK_MIN   	6000000
#define OV2643_XCLK_NOM   	24000000
#define OV2643_XCLK_MAX 	24000000

/* FPS Capabilities */
#define OV2643_DEF_FPS		30

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


//The rest values will be declared later
#define OV2643_IMAGE_WIDTH	640
#define OV2643_IMAGE_HEIGHT	480

#define SENSOR_DETECTED		1
#define SENSOR_NOT_DETECTED	0

/**
 * struct ov2643_reg - ov2643 register format
 * @reg: 8bit offset to register
 * @val: 8bit register value
 *
 * Define a structure for OV2643 register initialization values
 */
struct ov2643_reg {
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
 * Array of image sizes supported by OV2643.  These must be ordered from
 * smallest image size to largest.
 */
const static struct capture_size ov2643_sizes[] = {
	{  OV2643_IMAGE_WIDTH, OV2643_IMAGE_HEIGHT },  /* VGA */
};

#define NUM_IMAGE_SIZES ARRAY_SIZE(ov2643_sizes)
#define NUM_FORMAT_SIZES 1


/**
 * struct struct frame_settings - struct for storage of sensor
 * frame settings
 */
struct ov2643_frame_settings {
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
 * struct struct ov2643_sensor_settings - struct for storage of
 * sensor settings.
 */
struct ov2643_sensor_settings {
	/* struct ov2643_clk_settings clk; */
	struct ov2643_frame_settings frame;
};

/**
 * struct struct ov2643_clock_freq - struct for storage of sensor
 * clock frequencies
 */
struct ov2643_clock_freq {
	u32 vco_clk;
	u32 mipi_clk;
	u32 ddr_clk;
	u32 vt_pix_clk;
};

#define OV2643_DRIVER_NAME  "ov2643"
#define OV2643_MOD_NAME "ov2643: "

/*
 * Our nominal (default) frame rate.
 */
#define OV2643_FRAME_RATE 30

#define COM12_RESET	(1 << 7)

//Some image formats will be added
enum image_size { VGA };
enum pixel_format { YUV };

static int debug;
module_param(debug, bool, 0644);
MODULE_PARM_DESC(debug, "Debug level (0-1)");

const static struct ov2643_reg initial_list[] = {
		{0x12, 0x80}, //reset
		{0xff, 0x64}, //delay 100ms
		{0x13, 0xff},
		//;pclk=72mhz,30fps/pclk=36mhz,15fps
		{0xc3, 0x1f},
		{0xc4, 0xff},
		{0x3d, 0x48},
		{0xdd, 0xa5},
		//;windows setup
		{0x20, 0x01},
		{0x21, 0x98},
		{0x22, 0x00},
		{0x23, 0x06},
		{0x24, 0x28},//;0x280=640
		{0x25, 0x04},
		{0x26, 0x1e},//;0x1e0=480
		{0x27, 0x04},
		{0x28, 0x40},
		//;format setting
		{0x12, 0x09},
		{0x39, 0xd0},
		{0xcd, 0x13},
		{0x3d, 0x08},
		//;frame setting
		{0x0e, 0x10},//clock 48Mhz PCLK
		{0x0f, 0x14},
		{0x10, 0x0a},
		{0x11, 0x00},
		{0x29, 0x07},//;dummy pixels//24.75M 0x29,0x07//24M 0x29,0x07,//->ov setting
		{0x2a, 0x93},               //24.75M 0x2a,0xd0//24M 0x2a,0x93,//->ov setting
		{0x2b, 0x02},//;dummy lines //24.75M 0x2b,0x02//24M 0x2b,0x02,//->ov setting
		{0x2c, 0x6a},               //24.75M 0x2c,0x6a//24M 0x2c,0x6a,//->ov setting
		//for 25fps 0x26a*1.2=0x2e6
		{0x1c, 0x25},//vsync width
		{0x1d, 0x02},
		{0x1e, 0x00},
		{0x1f, 0xb9},

		//common part except 0x3d,0xde
		{0x14, 0x87}, //band filter bit7: 1:50Hz 0:60Hz bit4:
		{0x15, 0x42},
		{0x3c, 0xa4},
		{0x18, 0x78},//set to default then set to csi_ae value to wake up quickly
		{0x19, 0x68},//
		{0x1a, 0x71},
		{0x37, 0xe8},
		{0x16, 0x90},
		{0x43, 0x00},
		{0x40, 0xfb},
		{0xa9, 0x44},
		{0x2f, 0xec},
		{0x35, 0x10},
		{0x36, 0x10},
		{0x0c, 0x00},
		{0x0d, 0x20},
		{0xd0, 0x93},
		{0xdc, 0x2b},
		{0xd9, 0x41},
		{0xd3, 0x02},
		{0xde, 0x7c},
		{0x3d, 0x08},
		{0x0c, 0x00},
		{0x9b, 0x69},
		{0x9c, 0x7d},
		{0x9d, 0x7d},
		{0x9e, 0x69},
		{0x35, 0x04},
		{0x36, 0x04},
		//;gamma
		//normal  ////enhanced////zqh3    ////zqh2    ////zqh1    ////reset   //
		{0x65, 0x04},//0x65,0x12,//0x65,0x04,//0x65,0x04,//0x65,0x07,//0x65,0x05,//
		{0x66, 0x07},//0x66,0x20,//0x66,0x07,//0x66,0x07,//0x66,0x12,//0x66,0x0c,//
		{0x67, 0x19},//0x67,0x39,//0x67,0x19,//0x67,0x19,//0x67,0x1f,//0x67,0x1c,//
		{0x68, 0x34},//0x68,0x4e,//0x68,0x34,//0x68,0x34,//0x68,0x35,//0x68,0x2a,//
		{0x69, 0x4a},//0x69,0x62,//0x69,0x4a,//0x69,0x4a,//0x69,0x4a,//0x69,0x39,//
		{0x6a, 0x5a},//0x6a,0x74,//0x6a,0x5a,//0x6a,0x5a,//0x6a,0x5d,//0x6a,0x45,//
		{0x6b, 0x67},//0x6b,0x85,//0x6b,0x67,//0x6b,0x6b,//0x6b,0x6f,//0x6b,0x52,//
		{0x6c, 0x71},//0x6c,0x92,//0x6c,0x71,//0x6c,0x78,//0x6c,0x7d,//0x6c,0x5d,//
		{0x6d, 0x7c},//0x6d,0x9e,//0x6d,0x7c,//0x6d,0x84,//0x6d,0x8a,//0x6d,0x68,//
		{0x6e, 0x8c},//0x6e,0xb2,//0x6e,0x8c,//0x6e,0x98,//0x6e,0x9f,//0x6e,0x7f,//
		{0x6f, 0x9b},//0x6f,0xc0,//0x6f,0x9b,//0x6f,0xa6,//0x6f,0xae,//0x6f,0x91,//
		{0x70, 0xa9},//0x70,0xcc,//0x70,0xa9,//0x70,0xb2,//0x70,0xbb,//0x70,0xa5,//
		{0x71, 0xc0},//0x71,0xe0,//0x71,0xc0,//0x71,0xc6,//0x71,0xd0,//0x71,0xc6,//
		{0x72, 0xd5},//0x72,0xee,//0x72,0xd5,//0x72,0xd5,//0x72,0xdf,//0x72,0xde,//
		{0x73, 0xe8},//0x73,0xf6,//0x73,0xe8,//0x73,0xe8,//0x73,0xe8,//0x73,0xef,//
		{0x74, 0x20},//0x74,0x11,//0x74,0x20,//0x74,0x20,//0x74,0x20,//0x74,0x16,//
		//;color matrix
		//ttune   //ov seeting //
		{0xab, 0x20},//0xab,0x28,//
		{0xac, 0x5b},//0xac,0x48,//
		{0xad, 0x05},//0xad,0x10,//
		{0xae, 0x1b},//0xae,0x18,//
		{0xaf, 0x76},//0xaf,0x75,//
		{0xb0, 0x90},//0xb0,0x8c,//
		{0xb1, 0x90},//0xb1,0x8d,//
		{0xb2, 0x8c},//0xb2,0x8c,//
		{0xb3, 0x04},//0xb3,0x00,//
		{0xb4, 0x98},//0xb4,0x98,//
		{0xb5, 0x00},//0xb5,0x00,//
		//;lens shading
		{0x40, 0xFB},//0x40,0x08,//
		{0x4c, 0x03},//0x4c,0x03,//
		{0x4d, 0x30},//0x4d,0xd0,//
		{0x4e, 0x02},//0x4e,0x02,//
		{0x4f, 0x5c},//0x4f,0x5c,//
		{0x50, 0x3e},//0x50,0x3e,//
		{0x51, 0x00},//0x51,0x00,//
		{0x52, 0x66},//0x52,0x66,//
		{0x53, 0x03},//0x53,0x03,//
		{0x54, 0x30},//0x54,0xd0,//
		{0x55, 0x02},//0x55,0x02,//
		{0x56, 0x5c},//0x56,0x5c,//
		{0x57, 0x47},//0x57,0x47,//
		{0x58, 0x00},//0x58,0x00,//
		{0x59, 0x66},//0x59,0x66,//
		{0x5a, 0x03},//0x5a,0x03,//
		{0x5b, 0x20},//0x5b,0xd0,//
		{0x5c, 0x02},//0x5c,0x02,//
		{0x5d, 0x5c},//0x5d,0x5c,/
		{0x5e, 0x3e},//0x5e,0x3e,//
		{0x5f, 0x00},//0x5f,0x00,//
		{0x60, 0x66},//0x60,0x66,//

		{0x41, 0x1f},
		{0xb5, 0x01},
		{0xb6, 0x07},
		{0xb9, 0x3c},
		{0xba, 0x28},
		{0xb7, 0x90},
		{0xb8, 0x08},
		{0xbf, 0x0c},
		{0xc0, 0x3e},
		{0xa3, 0x0a},
		{0xa4, 0x0f},
		{0xa5, 0x09},//denoise threshold
		{0xa6, 0x16},
		{0x9f, 0x0a},
		{0xa0, 0x0f},
		{0xa7, 0x0a},
		{0xa8, 0x0f},
		{0xa1, 0x18},//0xa1,0x10,
		{0xa2, 0x10},//0xa2,0x04,
		{0xa9, 0x00},//0xa9,0x04,
		{0xaa, 0xa6},
		//;awb
		{0x75, 0x68},//0x75,0x6a,//
		{0x76, 0x11},//0x76,0x11,//
		{0x77, 0x92},//0x77,0x92,//
		{0x78, 0xa1},//0x78,0xa1,//
		{0x79, 0xe1},//0x79,0xe1,//
		{0x7a, 0x02},//0x7a,0x02,//
		{0x7c, 0x0e},//0x7c,0x05,//
		{0x7d, 0x12},//0x7d,0x08,//
		{0x7e, 0x12},//0x7e,0x08,//
		{0x7f, 0x54},//0x7f,0x7c,//
		{0x80, 0x78},//0x80,0x58,//
		{0x81, 0xa2},//0x81,0x2a,//
		{0x82, 0x80},//0x82,0xc5,//
		{0x83, 0x4e},//0x83,0x46,//
		{0x84, 0x40},//0x84,0x3a,//
		{0x85, 0x4c},//0x85,0x54,//
		{0x86, 0x43},//0x86,0x44,//
		{0x87, 0xf8},//0x87,0xf8,//
		{0x88, 0x08},//0x88,0x08,//
		{0x89, 0x70},//0x89,0x70,//
		{0x8a, 0xf0},//0x8a,0xf0,//
		{0x8b, 0xf0},//0x8b,0xf0,//

		{0x90, 0xe3},
		{0x93, 0x10},
		{0x94, 0x20},
		{0x95, 0x10},
		{0x96, 0x18},

		{0xff, 250},//delay 255ms
		{0xff, 0xff}	/* END MARKER */
};

//For any sake
static const struct ov2643_reg ov2643_YYUV_regs[] = {
	{ OV2643_TERM_REG, OV2643_TERM_VAL },
};
static const struct ov2643_reg ov2643_vga_regs[] = {
		{ OV2643_TERM_REG, OV2643_TERM_VAL },
};

const static struct ov2643_reg *ov2643_reg_format_init[NUM_FORMAT_SIZES] = {
	ov2643_YYUV_regs
};

const static struct ov2643_reg *ov2643_reg_size_init[NUM_IMAGE_SIZES] = {
	ov2643_vga_regs,
};
//Forward declaration of driver operations
static int ov2643_get_control(struct v4l2_subdev *, struct v4l2_control *);
static int ov2643_set_control(struct v4l2_subdev *, struct v4l2_control *);
static int ov2643_query_control(struct v4l2_subdev *, struct v4l2_queryctrl *);

static enum image_size ov2643_find_size(unsigned int width, unsigned int height){
	enum image_size isize;
	unsigned long pixels = width * height;
#ifdef OV2643_DEBUG
	printk("ovm2643 find size\r\n");
#endif
	//for (isize = QVGA; isize < VGA; isize++)
	isize = VGA;
	{
		if (ov2643_sizes[isize + 1].height *
			ov2643_sizes[isize + 1].width > pixels)
			return isize;
	}
	return VGA;
}

struct ov2643 {
	struct v4l2_subdev sd;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
};
static inline struct ov2643 *to_ov2643(struct v4l2_subdev *sd){
	return container_of(sd, struct ov2643, sd);//Funny thing works
}

static struct i2c_driver ov2643_i2c_driver;
static unsigned long xclk_current = OV2643_XCLK_NOM;

/* list of image formats supported by ov2643 sensor */
const static struct v4l2_fmtdesc ov2643_formats[] = {
	{
		.description	= "YUYV 4:2:2",
		.pixelformat	= V4L2_PIX_FMT_YUYV,
	}
};

#define NUM_CAPTURE_FORMATS ARRAY_SIZE(ov2643_formats)

static int ov2643_read_reg(struct i2c_client *client, u8 reg, u8 *val){
	int ret;
	ret = i2c_smbus_read_byte_data(client, reg);
	if (ret >= 0) {
		*val = (unsigned char) ret;
                ret = 0;
        }
#ifdef OV2643_DEBUG
	printk("ov2643 read reg: client: %x, addr: %x, reg: %x, val: %x, ret: %x\r\n", (unsigned int)client->adapter, client->addr, reg, *val, ret);
#endif
	return ret;
}

static int ov2643_write_reg(struct i2c_client *client, u8 reg, u8 val){
	int ret = 0;
	if (!client->adapter) return -ENODEV;
#ifdef OV2643_DEBUG
	printk("ov2643 write reg: client: %x, addr: %x, reg: %x, val: %x\r\n", (unsigned int)client->adapter, client->addr, reg, val);
#endif
	if (reg == OV2643_TERM_REG){
		if (val == OV2643_TERM_VAL) return 0;
	}
	ret = i2c_smbus_write_byte_data(client, reg, val);
	if (reg == OV2643_REG_COM12 && (val & COM12_RESET)) msleep(5); /* Wait for reset to run */
	if (reg == OV2643_TERM_REG){
		msleep(val);
	}
	return ret;
}

static int ov2643_write_regs(struct v4l2_subdev *sd, struct ov2643_reg *vals){
	int err = 0;
	const struct ov2643_reg *list = vals;
	struct i2c_client *client = v4l2_get_subdevdata(sd);
	while (!((list->reg == OV2643_TERM_REG) && (list->val == OV2643_TERM_VAL))) {
		err = ov2643_write_reg(client, list->reg,list->val);
		if (err) return err;
		msleep(1);
		list++;
	}
	return 0;
}

static unsigned long ov2643sensor_calc_xclk(void){
	xclk_current = OV2643_XCLK_NOM;
	return xclk_current;
}

static int ov2643_configure(struct v4l2_subdev *sd){
	struct ov2643 *ov2643 = to_ov2643(sd);
	struct v4l2_pix_format *pix = &ov2643->pix;
	enum image_size isize;
	int err = 0;
	enum pixel_format pfmt = YUV;
	unsigned long xclk;
#ifdef OV2643_DEBUG
	printk("Configuring ov2643 camera chip\n");
#endif
#if 0 //Just for initial driver version
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
	xclk = ov2643sensor_calc_xclk();
	isize = ov2643_find_size(pix->width, pix->height);
	/* configure pixel format */
	err = ov2643_write_regs(sd, (struct ov2643_reg *)(ov2643_reg_format_init[pfmt]) );
	if (err){
		printk("Configure made error1 %d\r\n", err);
		return err;
	}
	/* configure size */
	err = ov2643_write_regs(sd, (struct ov2643_reg *)(ov2643_reg_size_init[isize]) );
	if (err){
		printk("Configure made error2 %d\r\n", err);
		return err;
	}
#endif
	return 0;
}

static int ov2643_init(struct v4l2_subdev *sd, u32 val){
	/* Reset and wait two milliseconds */
	struct i2c_client *client = v4l2_get_subdevdata(sd);
#ifdef OV2643_DEBUG
	printk("ov2643 initialization function\r\n");
#endif
	ov2643_write_reg(client, OV2643_REG_COM12, COM12_RESET);
    msleep(5);
	return ov2643_write_regs(sd, (struct ov2643_reg *)initial_list );
}

//All that clear about controls
static int ov2643_query_control(struct v4l2_subdev *sd, struct v4l2_queryctrl *qctr){
#if 0
	printk("ov2643 ioctl_queryctrl dummy method\r\n");
	printk("Def val %x\r\n",qctr->default_value);
	printk("Name %s\r\n", qctr->name);
	printk("Type %d\r\n", qctr->type);
	printk("ID 0x%08x\r\n", qctr->id);
	//printk("", gctr->)
#endif
	return -EINVAL;
}

static int ov2643_get_control(struct v4l2_subdev *sd, struct v4l2_control *ctrl){
#ifdef OV2643_DEBUG
	printk("ov2643 ioctl_g_ctrl dummy method\r\n");
#endif
	return -EINVAL;
}

static int ov2643_set_control(struct v4l2_subdev *sd,struct v4l2_control *ctrl){
	int retval = -EINVAL;
#ifdef OV2643_DEBUG
	printk("ov2643 ioctl_s_ctrl dummy method\r\n");
#endif
	return retval;
}

static int ov2643_enum_format(struct v4l2_subdev *sd, struct v4l2_fmtdesc *fmt){
	int index = fmt->index;
	enum v4l2_buf_type type = fmt->type;
	//
	memset(fmt, 0, sizeof(*fmt));
	fmt->index = index;
	fmt->type = type;
	//
#ifdef OV2643_DEBUG
	printk("%s: Enum format capability\n", __func__);
	printk("ov2643 ioctl_enum_fmt_cap\r\n");
#endif
	//
	switch (fmt->type) {
		case V4L2_BUF_TYPE_VIDEO_CAPTURE:
			if (index >= NUM_CAPTURE_FORMATS) return -EINVAL;
			break;
		default:
			return -EINVAL;
	}
	fmt->flags = ov2643_formats[index].flags;
	strlcpy(fmt->description, ov2643_formats[index].description, sizeof(fmt->description));
	fmt->pixelformat = ov2643_formats[index].pixelformat;
	return 0;
}

static int ov2643_set_stream(struct v4l2_subdev *sd, int enable){
#ifdef OV2643_DEBUG
	printk("OV2643 set stream - dummy method\r\n");
#endif
	return 0;
}

static int ov2643_try_format(struct v4l2_subdev *sd, struct v4l2_format *f){
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
  	printk("ov2643 try format:\r\n bytes per line: %d\r\npixelformat %c%c%c%c\r\n, colorspace %s\r\n, field %d\r\n, height %d, width %d, sizeimage %d\r\n",
  			f->fmt.pix.bytesperline,
  			((f->fmt.pix.pixelformat)&0xff),(((f->fmt.pix.pixelformat)>>8)&0xff),(((f->fmt.pix.pixelformat)>>16)&0xff),(((f->fmt.pix.pixelformat)>>24)&0xff),
  			colorspace,
  			f->fmt.pix.field,
  			f->fmt.pix.height,
  			f->fmt.pix.width,
  			f->fmt.pix.sizeimage
  	);
#endif
	isize = ov2643_find_size(pix->width, pix->height);
	pix->width = ov2643_sizes[isize].width;
	pix->height = ov2643_sizes[isize].height;
#ifdef OV2643_DEBUG
	printk("%s: Trying format\n", __func__);
#endif
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (pix->pixelformat == ov2643_formats[ifmt].pixelformat) break;
	}

	if (ifmt == NUM_CAPTURE_FORMATS) ifmt = 0;

	pix->pixelformat = ov2643_formats[ifmt].pixelformat;
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

static int ov2643_set_format(struct v4l2_subdev *sd, struct v4l2_format *f){
	struct ov2643 *ov2643 = to_ov2643(sd);
	struct v4l2_pix_format *pix = &f->fmt.pix;
	int rval;
#ifdef OV2643_DEBUG
	printk("ov2643 set format\r\n");
#endif
	rval = ov2643_try_format(sd, f);
	if (rval) {
		printk("%s: Error trying format\n", __func__);
		return rval;
	}
	rval = ov2643_configure(sd);
	if (!rval) {
		ov2643->pix = *pix;
	} else {
		printk("%s: Error configure format %d\n", __func__, rval);
	}
	return rval;
}

static int ov2643_get_format(struct v4l2_subdev *sd, struct v4l2_format *f){
	struct ov2643 *ov2643 = to_ov2643(sd);
	f->fmt.pix = ov2643->pix;
#ifdef OV2643_DEBUG
	printk("ov2643 ioctl_g_fmt_cap\r\n");
#endif
	return 0;
}

static int ov2643_get_param(struct v4l2_subdev *sd, struct v4l2_streamparm *a){
	struct ov2643 *ov2643 = to_ov2643(sd);
	struct v4l2_captureparm *cparm = &a->parm.capture;
#ifdef OV2643_DEBUG
	printk("ov2643 ioctl_g_parm\r\n");
#endif
	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)	return -EINVAL;
	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	cparm->capability = V4L2_CAP_TIMEPERFRAME;
	cparm->timeperframe = ov2643->timeperframe;
	return 0;
}

static int ov2643_get_chip_id(struct v4l2_subdev *sd, struct v4l2_dbg_chip_ident *id){
	struct i2c_client *client = v4l2_get_subdevdata(sd);;
#ifdef OV2643_DEBUG
	printk("ov2643 get chipid\r\n");
#endif
	if (id->match.type != V4L2_CHIP_MATCH_I2C_ADDR){
		printk("match type fails\r\n");
		return -EINVAL;
	}
	if (id->match.addr != client->addr){
		printk("match addr fails\r\n");
		return -ENODEV;
	}
	id->ident	= V4L2_IDENT_OV2643;
	id->revision	= 0;
#ifdef OV2643_DEBUG
	printk("ov2643 chip id ok\r\n");
#endif
	return 0;
}

static const struct v4l2_subdev_core_ops ov2643_core_ops = {
	.g_chip_ident = ov2643_get_chip_id,
	.init         = ov2643_init,
	.queryctrl    = ov2643_query_control,
	.g_ctrl	      = ov2643_get_control,
	.s_ctrl	      = ov2643_set_control,
};

static const struct v4l2_subdev_video_ops ov2643_video_ops = {
	.s_fmt    = ov2643_set_format,
	.g_fmt    = ov2643_get_format,//Check it correct
	.try_fmt  = ov2643_try_format,
	.s_stream = ov2643_set_stream,
	.enum_fmt = ov2643_enum_format,//Check it correct
	.g_parm   = ov2643_get_param//Check it correct
};

static const struct v4l2_subdev_ops ov2643_ops = {
	.core = &ov2643_core_ops,
	.video = &ov2643_video_ops,
};

static int ov2643_detect(struct i2c_client *client)
{
//Shadrin todo improve: may check model ID also
	u8 pidh, pidl;
#ifdef OV2643_DEBUG
	printk("Detect ov2643\r\n");
#endif
	if (!client) return -ENODEV;

	if (ov2643_read_reg(client, OV2643_REG_PIDH, &pidh)) return -ENODEV;
	if (ov2643_read_reg(client, OV2643_REG_PIDL, &pidl)) return -ENODEV;

	v4l_info(client, "model id detected 0x%02x%02x\n", pidh, pidl);
#ifdef OV2643_DEBUG
	printk("model id detected 0x%02x%02x\n", pidh, pidl);
#endif
	if ((pidh != OV2643_PIDH_MAGIC)|| (pidl != OV2643_PIDL_MAGIC)) {
		//return -ENODEV;
	}
	return 0;
}


static int ov2643_probe(struct i2c_client *client, const struct i2c_device_id *id){
	struct ov2643 *ov2643;
	struct v4l2_subdev *sd;
	int ret;
#ifdef OV2643_DEBUG
	printk("ov2643 probe enter\n");
#endif
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA)) {
		dev_err(&client->adapter->dev,"I2C-Adapter doesn't support I2C_FUNC_SMBUS_BYTE_DATA\n");
		return -EIO;
	}

	if (!client->dev.platform_data) {
		dev_err(&client->dev, "No platform data!!\n");
		return -ENODEV;
	}
	ov2643 = kzalloc(sizeof(struct ov2643), GFP_KERNEL);
	if (!ov2643) return -ENOMEM;
	ret = ov2643_detect(client);
	if (ret){
#ifdef OV2643_DEBUG
		printk("ov2643 detection failed\r\n");
#endif
		goto clean;
	}
	//Filling  ov2643 data stucture
	ov2643->pix.width = OV2643_IMAGE_WIDTH;
	ov2643->pix.height = OV2643_IMAGE_HEIGHT;
	ov2643->pix.pixelformat = V4L2_PIX_FMT_YUYV;
	ov2643->timeperframe.numerator = 1;
	ov2643->timeperframe.denominator = OV2643_FRAME_RATE;

	/* Register with V4L2 layer as slave device */
	sd = &ov2643->sd;
	v4l2_i2c_subdev_init(sd, client, &ov2643_ops);

	ret = ov2643_init(sd,0);
	v4l2_info(sd, "%s decoder driver registered !!\n", sd->name);
	return 0;

clean:
	kfree(ov2643);
	return ret;
}
static int ov2643_remove(struct i2c_client *client){
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct ov2643 *ov2643 = to_ov2643(sd);
	v4l2_device_unregister_subdev(sd);
	kfree(ov2643);
	return 0;
}

static const struct i2c_device_id ov2643_id[] = {
	{ "ov2643", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ov2643_id);

static struct i2c_driver ov2643_i2c_driver = {
	.driver = {
		.name = "ov2643",
	},
	.probe		= ov2643_probe,
	.remove		= ov2643_remove,
	.id_table	= ov2643_id,
};

static int __init ov2643_driver_init(void){
	int err;
#ifdef OV2643_DEBUG
	printk("OV2643 camera sensor init\r\n");
#endif
	err = i2c_add_driver(&ov2643_i2c_driver);
	if (err) {
		printk("Failed to register" OV2643_DRIVER_NAME ".\n");
		return err;
	}
	return 0;
}

static void __exit ov2643_driver_cleanup(void){
#ifdef OV2643_DEBUG
	printk("OV2643 camera driver cleanup\r\n");
#endif
	i2c_del_driver(&ov2643_i2c_driver);
}

module_init(ov2643_driver_init);
module_exit(ov2643_driver_cleanup);

MODULE_LICENSE("GPL V2");
MODULE_AUTHOR("Alexander V. Shadrin, alex.virt2real@gmail.com");
MODULE_DESCRIPTION("ov2643 primitive camera sensor driver");
