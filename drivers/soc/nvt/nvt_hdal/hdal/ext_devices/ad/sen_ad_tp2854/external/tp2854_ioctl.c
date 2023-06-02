/*
 * tp2854.c
 */
#ifndef NVT_PLATFORM
#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/fcntl.h>
#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/proc_fs.h>

#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <asm/uaccess.h>
#include <asm/io.h>
//#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/poll.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#endif

#include "tp2802_def.h"
#include "tp2802.h"

#ifdef NVT_PLATFORM
#include "ad_drv_util.h"
void tp28xx_byte_write(unsigned char chip, unsigned char reg_addr, unsigned char value);
unsigned char tp28xx_byte_read(unsigned char chip, unsigned char reg_addr);
#endif
enum{
TP2854 =0x5400,
TP2854B=0x5401,
};

#ifndef NVT_PLATFORM
static struct i2c_board_info tp2854_info =
{
    I2C_BOARD_INFO("TP2854", 0x44),
};
static struct i2c_client* tp28xx_client;
#endif

#define I2C_INDEX 2 //TX2 I2C2 for camera

MODULE_DESCRIPTION("TechPoint TP2854 Linux Module");
MODULE_LICENSE("GPL");



#define DEBUG            1  //printk debug information on/off
#define CVBS_FRAME_OUTPUT 0  //1->output odd+even field to one frame


#define DEFAULT_FORMAT      TP2802_1080P25

#ifndef NVT_PLATFORM
static int HDC_enable = 1;
static int chips = 1;
#endif
static int mode = DEFAULT_FORMAT;
static int output[] = { //MIPI_1CH1LANE_594M,
                        MIPI_4CH4LANE_594M,
                        //MIPI_4CH4LANE_297M,
                        //MIPI_2CH4LANE_297M,
                        //MIPI_1CH4LANE_297M,
                      };

static unsigned int id[MAX_CHIPS];

#define TP2854_I2C_A 	0x44
#define TP2854_I2C_B 	0x45


unsigned char tp2802_i2c_addr[] = { TP2854_I2C_A,
                                    TP2854_I2C_B
                                  };


#define TP2802_I2C_ADDR(chip_id)    (tp2802_i2c_addr[chip_id])


typedef struct
{
    unsigned int			count[CHANNELS_PER_CHIP];
    unsigned int				  mode[CHANNELS_PER_CHIP];
    unsigned int               scan[CHANNELS_PER_CHIP];
    unsigned int				  gain[CHANNELS_PER_CHIP][4];
    unsigned int               std[CHANNELS_PER_CHIP];
    unsigned int                 state[CHANNELS_PER_CHIP];
    unsigned int                 force[CHANNELS_PER_CHIP];
    unsigned char addr;
} tp2802wd_info;

#ifndef NVT_PLATFORM
static tp2802wd_info watchdog_info[MAX_CHIPS];
volatile static unsigned int watchdog_state = 0;
struct task_struct *task_watchdog_deamon = NULL;

//static DEFINE_SPINLOCK(watchdog_lock);
struct semaphore watchdog_lock;
#endif
#define WATCHDOG_EXIT    0
#define WATCHDOG_RUNNING 1
#define WDT              1

#ifndef NVT_PLATFORM
int  TP2802_watchdog_init(void);
void TP2802_watchdog_exit(void);

static void TP2854_PTZ_mode(unsigned char, unsigned char, unsigned char);
#endif
static void TP2854_RX_init(unsigned char chip, unsigned char mod);

unsigned char ReverseByte(unsigned char dat)
{

    static const unsigned char BitReverseTable256[] =
    {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0,
    0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8,
    0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4,
    0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC,
    0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2,
    0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA,
    0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6,
    0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE,
    0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1,
    0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9,
    0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5,
    0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD,
    0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3,
    0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB,
    0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7,
    0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
    };
    return BitReverseTable256[dat];
}
/////////////////////////////////////////////////////////////////////
unsigned char PELCO_Decode(unsigned char *buf)
{
	unsigned char i, j;
	unsigned char cnt_high = 0;
	unsigned char cnt_low = 0;
	unsigned char dat = 0;

	for(j = 0; j < 6; j++ )
	{
				for(i = 0; i <8; i++ )
				{
					if(0x80&buf[j])
					{
						cnt_high++;
						cnt_low = 0;
					}
					else
					{
						if(0 == cnt_low)
						{
							dat <<= 1;
							if(cnt_high > 2) dat |= 0x01;
							cnt_high = 0;
						}
						cnt_low++;
					}
					buf[j] <<= 1;

				}
		}
		if( 0 == cnt_low && cnt_high > 2 )
        {
                dat <<= 1;
                dat |= 0x01;
        }
		return dat;
}
///////////////////////////////////////////////////////
unsigned int ConvertACPV1Data(unsigned char dat)
{
    unsigned int i, tmp=0;
    for(i = 0; i < 8; i++)
    {
        tmp <<= 3;

        if(0x01 & dat) tmp |= 0x06;
        else tmp |= 0x04;

        dat >>= 1;
    }
    return tmp;
}
#ifndef NVT_PLATFORM
void tp28xx_byte_write(unsigned char chip, unsigned char reg_addr, unsigned char value)
{
    int ret;
    unsigned char buf[2];
    struct i2c_client* client = tp28xx_client;

    tp28xx_client->addr = tp2802_i2c_addr[chip];

    buf[0] = reg_addr;
    buf[1] = value;

    ret = i2c_master_send(client, buf, 2);
    //return ret;
}

unsigned char tp28xx_byte_read(unsigned char chip, unsigned char reg_addr)
{
    int ret_data = 0xFF;
    int ret;
    struct i2c_client* client = tp28xx_client;
    unsigned char buf[2];
    struct i2c_msg msg[2];

    tp28xx_client->addr = tp2802_i2c_addr[chip];
    memset(msg, 0x0, sizeof(struct i2c_msg) * 2);

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = 1;
	msg[0].buf = buf;

	/* reg_addr config */
	buf[0] = reg_addr;

	msg[1].addr = client->addr;
	msg[1].flags = client->flags & I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = 1;
	msg[1].buf = buf;


    //buf[0] = reg_addr;
    //ret = i2c_master_recv(client, buf, 1);
    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret >= 0)
    {
        ret_data = buf[0];
    }
    return ret_data;
}
static int i2c_client_init(void)
{
    struct i2c_adapter* i2c_adap;

    i2c_adap = i2c_get_adapter(I2C_INDEX);
    tp28xx_client = i2c_new_device(i2c_adap, &tp2854_info);
    i2c_put_adapter(i2c_adap);

    return 0;
}

static void i2c_client_exit(void)
{
    i2c_unregister_device(tp28xx_client);
}
#endif

static void tp2802_write_table(unsigned char chip,
                               unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt)
{
    unsigned char i = 0;
    for(i = 0; i < tbl_cnt; i ++)
    {
        tp28xx_byte_write(chip, (addr + i), *(tbl_ptr + i));
    }
}

void TP2854_StartTX(unsigned char chip, unsigned char ch)
{
	unsigned char tmp;
	int i;
	tp28xx_byte_write(chip, 0xB6, (0x01<<(ch))); //clear flag

	tmp = tp28xx_byte_read(chip, 0x6f);
    tmp |= 0x01;
    tp28xx_byte_write(chip, 0x6f, tmp); //TX enable
    tmp &= 0xfe;
    tp28xx_byte_write(chip, 0x6f, tmp); //TX disable

    i = 100;
    while(i--)
    {
        if( (0x01<<(ch)) & tp28xx_byte_read(chip, 0xb6)) break;
        //udelay(1000);
        msleep(2);
    }
}

void HDA_SetACPV2Data(unsigned char chip, unsigned char reg,unsigned char dat)
{
    unsigned int i;
	unsigned int PTZ_pelco=0;

    for(i = 0; i < 8; i++)
    {
        PTZ_pelco <<= 3;

        if(0x80 & dat) PTZ_pelco |= 0x06;
        else PTZ_pelco |= 0x04;

        dat <<= 1;
    }

    tp28xx_byte_write(chip, reg + 0 , (PTZ_pelco>>16)&0xff);
    tp28xx_byte_write(chip, reg + 1 , (PTZ_pelco>>8)&0xff);
    tp28xx_byte_write(chip, reg + 2 , (PTZ_pelco)&0xff);
}
void HDA_SetACPV1Data(unsigned char chip, unsigned char reg,unsigned char dat)
{

    unsigned int i;
	unsigned int PTZ_pelco=0;

    for(i = 0; i < 8; i++)
    {
        PTZ_pelco <<= 3;

        if(0x01 & dat) PTZ_pelco |= 0x06;
        else PTZ_pelco |= 0x04;

        dat >>= 1;
    }

    tp28xx_byte_write(chip, reg + 0 , (PTZ_pelco>>16)&0xff);
    tp28xx_byte_write(chip, reg + 1 , (PTZ_pelco>>8)&0xff);
    tp28xx_byte_write(chip, reg + 2 , (PTZ_pelco)&0xff);

}

#ifndef NVT_PLATFORM
int tp2802_open(struct inode * inode, struct file * file)
{
    return SUCCESS;
}

int tp2802_close(struct inode * inode, struct file * file)
{
    return SUCCESS;
}
#endif

static void tp2802_set_work_mode_1080p25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_1080p25_raster, 9);
}

static void tp2802_set_work_mode_1080p30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_1080p30_raster, 9);
}

static void tp2802_set_work_mode_720p25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p25_raster, 9);
}

static void tp2802_set_work_mode_720p30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p30_raster, 9);
}

static void tp2802_set_work_mode_720p50(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p50_raster, 9);
}

static void tp2802_set_work_mode_720p60(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_720p60_raster, 9);
}

static void tp2802_set_work_mode_PAL(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_PAL_raster, 9);
}

static void tp2802_set_work_mode_NTSC(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_NTSC_raster, 9);
}

static void tp2802_set_work_mode_3M(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_3M_raster, 9);
}

static void tp2802_set_work_mode_5M(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_5M_raster, 9);
}
static void tp2802_set_work_mode_4M(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M_raster, 9);
}
static void tp2802_set_work_mode_3M20(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_3M20_raster, 9);
}
static void tp2802_set_work_mode_4M12(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M12_raster, 9);
}
static void tp2802_set_work_mode_6M10(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_6M10_raster, 9);
}
static void tp2802_set_work_mode_QHD15(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHD15_raster, 9);
}
static void tp2802_set_work_mode_QHD30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHD30_raster, 9);
}
static void tp2802_set_work_mode_QHD25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QHD25_raster, 9);
}
static void tp2802_set_work_mode_QXGA30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QXGA30_raster, 9);
}
static void tp2802_set_work_mode_QXGA25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_QXGA25_raster, 9);
}
/*
static void tp2802_set_work_mode_4M30(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M30_raster, 9);
}
static void tp2802_set_work_mode_4M25(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_4M25_raster, 9);
}
*/
static void tp2802_set_work_mode_5M20(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_5M20_raster, 9);
}
static void tp2802_set_work_mode_8M15(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_8M15_raster, 9);
}
static void tp2802_set_work_mode_8M12(unsigned chip)
{
    // Start address 0x15, Size = 9B
    tp2802_write_table(chip, 0x15, tbl_tp2802_8M12_raster, 9);
}
//#define AMEND


#ifndef NVT_PLATFORM
long tp2802_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    unsigned int __user *argp = (unsigned int __user *)arg;
    unsigned int i, j, chip, tmp, ret = 0;
   // unsigned long flags;

    tp2802_register		   dev_register;
    tp2802_image_adjust    image_adjust;
    tp2802_work_mode       work_mode;
    tp2802_video_mode	   video_mode;
    tp2802_video_loss      video_loss;
    tp2802_PTZ_data        PTZ_data;

    switch (_IOC_NR(cmd))
    {

    case _IOC_NR(TP2802_READ_REG):
    {
        if (copy_from_user(&dev_register, argp, sizeof(tp2802_register)))
            return FAILURE;

        down(&watchdog_lock);

        chip = dev_register.chip;

        tp2802_set_reg_page(chip, dev_register.ch);

        dev_register.value = tp28xx_byte_read(chip, dev_register.reg_addr);

        up(&watchdog_lock);

        if (copy_to_user(argp, &dev_register, sizeof(tp2802_register)))
            return FAILURE;

        break;
    }

    case _IOC_NR(TP2802_WRITE_REG):
    {
        if (copy_from_user(&dev_register, argp, sizeof(tp2802_register)))
            return FAILURE;

        down(&watchdog_lock);

        chip = dev_register.chip;

        tp2802_set_reg_page(chip, dev_register.ch);

        tp28xx_byte_write(chip, dev_register.reg_addr, dev_register.value);

        up(&watchdog_lock);
        break;
    }

    case _IOC_NR(TP2802_SET_VIDEO_MODE):
    {
        if (copy_from_user(&video_mode, argp, sizeof(tp2802_video_mode)))
            return FAILURE;

        if(video_mode.ch >= CHANNELS_PER_CHIP)  return FAILURE;

        down(&watchdog_lock);

        ret = tp2802_set_video_mode(video_mode.chip, video_mode.mode, video_mode.ch, video_mode.std);

        up(&watchdog_lock);

        if (!(ret))
        {

            watchdog_info[video_mode.chip].mode[video_mode.ch] = video_mode.mode;
            watchdog_info[video_mode.chip].std[video_mode.ch] = video_mode.std;
            return SUCCESS;
        }
        else
        {
            printk("Invalid mode:%d\n", video_mode.mode);
            return FAILURE;
        }

        break;
    }

    case _IOC_NR(TP2802_GET_VIDEO_MODE):
    {
        if (copy_from_user(&video_mode, argp, sizeof(tp2802_video_mode)))
            return FAILURE;

        if(video_mode.ch >= CHANNELS_PER_CHIP)  return FAILURE;
#if (WDT)
        video_mode.mode = watchdog_info[video_mode.chip].mode[video_mode.ch];
        video_mode.std = watchdog_info[video_mode.chip].std[video_mode.ch];
#else
        down(&watchdog_lock);

        chip = video_mode.chip;

        tp2802_set_reg_page(chip, video_mode.ch);

        tmp = tp28xx_byte_read(chip, 0x03);
        tmp &= 0x7; /* [2:0] - CVSTD */
        video_mode.mode = tmp;

        up(&watchdog_lock);
#endif
        if (copy_to_user(argp, &video_mode, sizeof(tp2802_video_mode)))
            return FAILURE;
        break;
    }

    case _IOC_NR(TP2802_GET_VIDEO_LOSS):/* get video loss state */
    {
        if (copy_from_user(&video_loss, argp, sizeof(tp2802_video_loss)))
            return FAILURE;

        if(video_loss.ch >= CHANNELS_PER_CHIP)  return FAILURE;
#if (WDT)
        video_loss.is_lost = ( VIDEO_LOCKED == watchdog_info[video_loss.chip].state[video_loss.ch] ) ? 0:1;
        if(video_loss.is_lost) video_loss.is_lost = ( VIDEO_UNLOCK == watchdog_info[video_loss.chip].state[video_loss.ch] ) ? 0:1;
#else
        down(&watchdog_lock);

        chip = video_loss.chip;

        tp2802_set_reg_page(chip, video_loss.ch);

        tmp = tp28xx_byte_read(chip, 0x01);
        tmp = (tmp & 0x80) >> 7;
        if(!tmp)
        {
            if(0x08 == tp28xx_byte_read(chip, 0x2f))
            {
                tmp = tp28xx_byte_read(chip, 0x04);
                if(tmp < 0x30) tmp = 0;
                else tmp = 1;
            }

        }
        video_loss.is_lost = tmp;   /* [7] - VDLOSS */

        up(&watchdog_lock);
#endif
        if (copy_to_user(argp, &video_loss, sizeof(video_loss)))
            return FAILURE;

        break;
    }

    case _IOC_NR(TP2802_SET_IMAGE_ADJUST):
    {
        if (copy_from_user(&image_adjust, argp, sizeof(tp2802_image_adjust)))
        {
            return FAILURE;
        }

        if(image_adjust.ch >= CHANNELS_PER_CHIP)  return FAILURE;

        down(&watchdog_lock);

        chip = image_adjust.chip;

        tp2802_set_reg_page(chip, image_adjust.ch);

        // Set Brightness
        tp28xx_byte_write(chip, BRIGHTNESS, image_adjust.brightness);

        // Set Contrast
        tp28xx_byte_write(chip, CONTRAST, image_adjust.contrast);

        // Set Saturation
        tp28xx_byte_write(chip, SATURATION, image_adjust.saturation);

        // Set Hue
        tp28xx_byte_write(chip, HUE, image_adjust.hue);

        // Set Sharpness
        tmp = tp28xx_byte_read(chip, SHARPNESS);
        tmp &= 0xe0;
        tmp |= (image_adjust.sharpness & 0x1F);
        tp28xx_byte_write(chip, SHARPNESS, tmp);

        up(&watchdog_lock);
        break;
    }

    case _IOC_NR(TP2802_GET_IMAGE_ADJUST):
    {
        if (copy_from_user(&image_adjust, argp, sizeof(tp2802_image_adjust)))
            return FAILURE;

        if(image_adjust.ch >= CHANNELS_PER_CHIP)  return FAILURE;

        down(&watchdog_lock);

        chip = image_adjust.chip;

        tp2802_set_reg_page(chip, image_adjust.ch);

        // Get Brightness
        image_adjust.brightness = tp28xx_byte_read(chip, BRIGHTNESS);

        // Get Contrast
        image_adjust.contrast = tp28xx_byte_read(chip, CONTRAST);

        // Get Saturation
        image_adjust.saturation = tp28xx_byte_read(chip, SATURATION);

        // Get Hue
        image_adjust.hue = tp28xx_byte_read(chip, HUE);

        // Get Sharpness
        image_adjust.sharpness = 0x1F & tp28xx_byte_read(chip, SHARPNESS);

        up(&watchdog_lock);

        if (copy_to_user(argp, &image_adjust, sizeof(tp2802_image_adjust)))
            return FAILURE;

        break;
    }
    case _IOC_NR(TP2802_SET_PTZ_DATA):
    {
        if (copy_from_user(&PTZ_data, argp, sizeof(tp2802_PTZ_data)))
        {
            return FAILURE;
        }

        if(PTZ_data.ch >= CHANNELS_PER_CHIP)  return FAILURE;

        down(&watchdog_lock);

        chip = PTZ_data.chip;
        if( TP2854 == id[chip] || TP2854B == id[chip])
        {
            TP2854_PTZ_mode(chip, PTZ_data.ch, PTZ_data.mode );

			for(i = 0; i < 24; i++)
			{
                tp28xx_byte_write(chip, 0x55+i, 0x00);
			}

			if(PTZ_HDC == PTZ_data.mode || PTZ_HDC_QHD == PTZ_data.mode || PTZ_HDC_8M15 == PTZ_data.mode || PTZ_HDC_8M12 == PTZ_data.mode) //HDC
			{

                           for(i = 0; i < 7; i++)
                            {
                                tp28xx_byte_write(chip, 0x6e, PTZ_data.data[i]);
                            }

                            TP2854_StartTX(chip, PTZ_data.ch);

            }
            else if(PTZ_HDA_4M25 == PTZ_data.mode || PTZ_HDA_4M15 == PTZ_data.mode) //HDA QHD
            {
                           for(i = 0; i < 8; i++)
                            {
                                tp28xx_byte_write(chip, 0x6e, 0x00);
                            }
                            TP2854_StartTX(chip, PTZ_data.ch);

                            for(i = 0; i < 8; i++)
                            {
                                tp28xx_byte_write(chip, 0x6e, ReverseByte(PTZ_data.data[i]));
                            }
                            TP2854_StartTX(chip, PTZ_data.ch);

            }
            else if(PTZ_HDA_1080P == PTZ_data.mode || PTZ_HDA_3M18 == PTZ_data.mode || PTZ_HDA_3M25 == PTZ_data.mode) //HDA 1080p
            {
								HDA_SetACPV2Data(chip, 0x58, 0x00);
								HDA_SetACPV2Data(chip, 0x5e, 0x00);
								HDA_SetACPV2Data(chip, 0x64, 0x00);
								HDA_SetACPV2Data(chip, 0x6a, 0x00);
								TP2854_StartTX(chip, PTZ_data.ch);
								HDA_SetACPV2Data(chip, 0x58, PTZ_data.data[0]);
								HDA_SetACPV2Data(chip, 0x5e, PTZ_data.data[1]);
								HDA_SetACPV2Data(chip, 0x64, PTZ_data.data[2]);
								HDA_SetACPV2Data(chip, 0x6a, PTZ_data.data[3]);
								TP2854_StartTX(chip, PTZ_data.ch);

            }
            else if( PTZ_HDA_720P == PTZ_data.mode ) //HDA 720p
            {
								HDA_SetACPV1Data(chip, 0x55, 0x00);
								HDA_SetACPV1Data(chip, 0x58, 0x00);
								HDA_SetACPV1Data(chip, 0x5b, 0x00);
								HDA_SetACPV1Data(chip, 0x5e, 0x00);
								TP2854_StartTX(chip, PTZ_data.ch);
								HDA_SetACPV1Data(chip, 0x55, PTZ_data.data[0]);
								HDA_SetACPV1Data(chip, 0x58, PTZ_data.data[1]);
								HDA_SetACPV1Data(chip, 0x5b, PTZ_data.data[2]);
								HDA_SetACPV1Data(chip, 0x5e, PTZ_data.data[3]);
								TP2854_StartTX(chip, PTZ_data.ch);
            }
            else if( PTZ_HDA_CVBS == PTZ_data.mode) //HDA 960H
            {
								HDA_SetACPV1Data(chip, 0x55, 0x00);
								HDA_SetACPV1Data(chip, 0x58, 0x00);
								HDA_SetACPV1Data(chip, 0x5b, 0x00);
								HDA_SetACPV1Data(chip, 0x5e, 0x00);
								TP2854_StartTX(chip, PTZ_data.ch);
								HDA_SetACPV1Data(chip, 0x55, PTZ_data.data[0]);
								HDA_SetACPV1Data(chip, 0x58, PTZ_data.data[1]);
								HDA_SetACPV1Data(chip, 0x5b, PTZ_data.data[2]);
								HDA_SetACPV1Data(chip, 0x5e, PTZ_data.data[3]);
								TP2854_StartTX(chip, PTZ_data.ch);
            }
            else //TVI
            {
                            //line1
                                tp28xx_byte_write(chip, 0x56 , 0x02);
                                tp28xx_byte_write(chip, 0x57 , PTZ_data.data[0]);
                                tp28xx_byte_write(chip, 0x58 , PTZ_data.data[1]);
                                tp28xx_byte_write(chip, 0x59 , PTZ_data.data[2]);
                                tp28xx_byte_write(chip, 0x5A , PTZ_data.data[3]);
                            //line2
                                tp28xx_byte_write(chip, 0x5C , 0x02);
                                tp28xx_byte_write(chip, 0x5D , PTZ_data.data[4]);
                                tp28xx_byte_write(chip, 0x5E , PTZ_data.data[5]);
                                tp28xx_byte_write(chip, 0x5F , PTZ_data.data[6]);
                                tp28xx_byte_write(chip, 0x60 , PTZ_data.data[7]);
                            //line3
                                tp28xx_byte_write(chip, 0x62 , 0x02);
                                tp28xx_byte_write(chip, 0x63 , PTZ_data.data[0]);
                                tp28xx_byte_write(chip, 0x64 , PTZ_data.data[1]);
                                tp28xx_byte_write(chip, 0x65 , PTZ_data.data[2]);
                                tp28xx_byte_write(chip, 0x66 , PTZ_data.data[3]);
                            //line4
                                tp28xx_byte_write(chip, 0x68 , 0x02);
                                tp28xx_byte_write(chip, 0x69 , PTZ_data.data[4]);
                                tp28xx_byte_write(chip, 0x6A , PTZ_data.data[5]);
                                tp28xx_byte_write(chip, 0x6B , PTZ_data.data[6]);
                                tp28xx_byte_write(chip, 0x6C , PTZ_data.data[7]);
                                TP2854_StartTX(chip, PTZ_data.ch);
            }


        }

        up(&watchdog_lock);
        break;
    }
    case _IOC_NR(TP2802_GET_PTZ_DATA):
    {
        if (copy_from_user(&PTZ_data, argp, sizeof(tp2802_PTZ_data)))
        {
            return FAILURE;
        }

        if(PTZ_data.ch >= CHANNELS_PER_CHIP)  return FAILURE;

        down(&watchdog_lock);

        chip = PTZ_data.chip;

        if( TP2854 == id[chip] || TP2854B == id[chip])
        {
            tp28xx_byte_write(chip, 0x40,  PTZ_data.ch); //bank switch
            if(PTZ_RX_TVI_CMD == PTZ_data.mode || PTZ_RX_TVI_BURST == PTZ_data.mode)
            {
            // line1
                PTZ_data.data[0]=tp28xx_byte_read(chip, 0x8C );
                PTZ_data.data[1]=tp28xx_byte_read(chip, 0x8D );
                PTZ_data.data[2]=tp28xx_byte_read(chip, 0x8E );
                PTZ_data.data[3]=tp28xx_byte_read(chip, 0x8F );
            //line2
                PTZ_data.data[4]=tp28xx_byte_read(chip, 0x92 );
                PTZ_data.data[5]=tp28xx_byte_read(chip, 0x93 );
                PTZ_data.data[6]=tp28xx_byte_read(chip, 0x94 );
                PTZ_data.data[7]=tp28xx_byte_read(chip, 0x95 );
            // line3
                PTZ_data.data[8]=tp28xx_byte_read(chip, 0x98 );
                PTZ_data.data[9]=tp28xx_byte_read(chip, 0x99 );
                PTZ_data.data[10]=tp28xx_byte_read(chip, 0x9a );
                PTZ_data.data[11]=tp28xx_byte_read(chip, 0x9b );
            //line4
                PTZ_data.data[12]=tp28xx_byte_read(chip, 0x9e );
                PTZ_data.data[13]=tp28xx_byte_read(chip, 0x9f );
                PTZ_data.data[14]=tp28xx_byte_read(chip, 0xa0 );
                PTZ_data.data[15]=tp28xx_byte_read(chip, 0xa1 );
            }
            else if(PTZ_RX_ACP1 == PTZ_data.mode || PTZ_RX_ACP2 == PTZ_data.mode || PTZ_RX_ACP3 == PTZ_data.mode)
            {

                for(i =0; i < 6; i++)
                    PTZ_data.data[i]=tp28xx_byte_read(chip, 0x8a+i );
                PTZ_data.data[8]=PELCO_Decode(PTZ_data.data);

                for(i =0; i < 6; i++)
                    PTZ_data.data[i]=tp28xx_byte_read(chip, 0x90+i );
                PTZ_data.data[9]=PELCO_Decode(PTZ_data.data);

                for(i =0; i < 6; i++)
                    PTZ_data.data[i]=tp28xx_byte_read(chip, 0x96+i );
                PTZ_data.data[10]=PELCO_Decode(PTZ_data.data);

                for(i =0; i < 6; i++)
                    PTZ_data.data[i]=tp28xx_byte_read(chip, 0x9c+i );
                PTZ_data.data[11]=PELCO_Decode(PTZ_data.data);

                for(i =0; i < 4; i++)
                    PTZ_data.data[i]=PTZ_data.data[8+i];
/*

                PTZ_data.data[0]=ReverseByte(tp28xx_byte_read(chip, 0x8f ));
                PTZ_data.data[1]=ReverseByte(tp28xx_byte_read(chip, 0x8e ));
                PTZ_data.data[2]=ReverseByte(tp28xx_byte_read(chip, 0x8d ));
                PTZ_data.data[3]=ReverseByte(tp28xx_byte_read(chip, 0x8c ));
                PTZ_data.data[4]=ReverseByte(tp28xx_byte_read(chip, 0x95 ));
                PTZ_data.data[5]=ReverseByte(tp28xx_byte_read(chip, 0x94 ));
                PTZ_data.data[6]=ReverseByte(tp28xx_byte_read(chip, 0x93 ));
                PTZ_data.data[7]=ReverseByte(tp28xx_byte_read(chip, 0x92 ));
*/
            }
            else
            {
                PTZ_data.data[0]=tp28xx_byte_read(chip, 0xa3 );
                PTZ_data.data[1]=tp28xx_byte_read(chip, 0xa3 );
                PTZ_data.data[2]=tp28xx_byte_read(chip, 0xa3 );
                PTZ_data.data[3]=tp28xx_byte_read(chip, 0xa3 );
                PTZ_data.data[4]=tp28xx_byte_read(chip, 0xa3 );
                PTZ_data.data[5]=tp28xx_byte_read(chip, 0xa3 );
                PTZ_data.data[6]=tp28xx_byte_read(chip, 0xa3 );

            }

        }

        up(&watchdog_lock);

        if (copy_to_user(argp, &PTZ_data, sizeof(tp2802_PTZ_data)))
        {
            return FAILURE;
        }

        break;
    }
    case _IOC_NR(TP2802_SET_SCAN_MODE):
    {
        if (copy_from_user(&work_mode, argp, sizeof(tp2802_work_mode)))
            return FAILURE;

        down(&watchdog_lock);

        if(work_mode.ch >= CHANNELS_PER_CHIP)
        {
            for(i = 0; i < CHANNELS_PER_CHIP; i++)
                watchdog_info[work_mode.chip].scan[i] = work_mode.mode;
        }
        else
        {
            watchdog_info[work_mode.chip].scan[work_mode.ch] = work_mode.mode;

        }

        up(&watchdog_lock);


        break;
    }
    case _IOC_NR(TP2802_DUMP_REG):
    {
        if (copy_from_user(&dev_register, argp, sizeof(tp2802_register)))
            return FAILURE;

        down(&watchdog_lock);

        for(i = 0; i < CHANNELS_PER_CHIP ; i++)
        {

            tp2802_set_reg_page(dev_register.chip, i);

            for(j = 0; j < 0x100; j++)
            {
                dev_register.value = tp28xx_byte_read(dev_register.chip, j);
                printk("%02x:%02x\n", j, dev_register.value );
            }
        }
        /*
        for(j = 0x40; j < 0x100; j++)
        {
            dev_register.value = tp28xx_byte_read(dev_register.chip, j);
            printk("%02x:%02x\n", j, dev_register.value );
        }*/
        tp2802_set_reg_page(dev_register.chip, MIPI_PAGE);
        for(j = 0x0; j < 0x40; j++)
        {
            dev_register.value = tp28xx_byte_read(dev_register.chip, j);
            printk("%02x:%02x\n", j, dev_register.value );
        }
        up(&watchdog_lock);

        if (copy_to_user(argp, &dev_register, sizeof(tp2802_register)))
            return FAILURE;

        break;
    }
    case _IOC_NR(TP2802_FORCE_DETECT):
    {
        if (copy_from_user(&work_mode, argp, sizeof(tp2802_work_mode)))
            return FAILURE;

        down(&watchdog_lock);

        if(work_mode.ch >= CHANNELS_PER_CHIP)
        {
            for(i = 0; i < CHANNELS_PER_CHIP; i++)
                watchdog_info[work_mode.chip].force[i] = 1;
        }
        else
        {
            watchdog_info[work_mode.chip].force[work_mode.ch] = 1;

        }

        up(&watchdog_lock);


        break;
    }

    case _IOC_NR(TP2802_SET_BURST_DATA):
    {
        if (copy_from_user(&PTZ_data, argp, sizeof(tp2802_PTZ_data)))
        {
            return FAILURE;
        }

        if(PTZ_data.ch >= CHANNELS_PER_CHIP)  return FAILURE;

        down(&watchdog_lock);

        chip = PTZ_data.chip;

        if( TP2854 == id[chip] || TP2854B == id[chip])
        {
            TP2854_PTZ_mode(chip, PTZ_data.ch, PTZ_data.mode );

                tp28xx_byte_write(chip, 0x55, 0x00);
                tp28xx_byte_write(chip, 0x5b, 0x00);

                            //line1
                                tp28xx_byte_write(chip, 0x56 , 0x03);
                                tp28xx_byte_write(chip, 0x57 , PTZ_data.data[0]);
                                tp28xx_byte_write(chip, 0x58 , PTZ_data.data[1]);
                                tp28xx_byte_write(chip, 0x59 , PTZ_data.data[2]);
                                tp28xx_byte_write(chip, 0x5A , PTZ_data.data[3]);
                            //line2
                                tp28xx_byte_write(chip, 0x5C , 0x03);
                                tp28xx_byte_write(chip, 0x5D , PTZ_data.data[4]);
                                tp28xx_byte_write(chip, 0x5E , PTZ_data.data[5]);
                                tp28xx_byte_write(chip, 0x5F , PTZ_data.data[6]);
                                tp28xx_byte_write(chip, 0x60 , PTZ_data.data[7]);
                            //line3
                                tp28xx_byte_write(chip, 0x62 , 0x03);
                                tp28xx_byte_write(chip, 0x63 , PTZ_data.data[8]);
                                tp28xx_byte_write(chip, 0x64 , PTZ_data.data[9]);
                                tp28xx_byte_write(chip, 0x65 , PTZ_data.data[10]);
                                tp28xx_byte_write(chip, 0x66 , PTZ_data.data[11]);
                            //line4
                                tp28xx_byte_write(chip, 0x68 , 0x03);
                                tp28xx_byte_write(chip, 0x69 , PTZ_data.data[12]);
                                tp28xx_byte_write(chip, 0x6A , PTZ_data.data[13]);
                                tp28xx_byte_write(chip, 0x6B , PTZ_data.data[14]);
                                tp28xx_byte_write(chip, 0x6C , PTZ_data.data[15]);

            TP2854_StartTX(chip, PTZ_data.ch);

        }

        up(&watchdog_lock);
        break;
    }
/*
    case _IOC_NR(TP2802_SET_VIDEO_INPUT):
    {
        if (copy_from_user(&dev_register, argp, sizeof(tp2802_register)))
            return FAILURE;

        if(dev_register.value > DIFF_VIN34)  return FAILURE;

        down(&watchdog_lock);

        chip = dev_register.chip;

        tmp = tp28xx_byte_read(chip, 0x41);
        tmp &= 0xf0;
        tmp |= dev_register.value;
        tp28xx_byte_write(chip, 0x41, tmp);

        if(dev_register.value < DIFF_VIN12)
        {
            //single
            tp28xx_byte_write(chip, 0x38, 0x40);
            tp28xx_byte_write(chip, 0x3d, 0x60);
            //tp28xx_byte_write(chip, 0x3e, 0x00);
        }
        else
        {
            //differential
            tp28xx_byte_write(chip, 0x38, 0x4c);
            tp28xx_byte_write(chip, 0x3d, 0x40);
            //tp28xx_byte_write(chip, 0x3e, 0x80);
        }

        watchdog_info[chip].force[0] = 1; //reset when Vin is changed

        up(&watchdog_lock);

        if (copy_to_user(argp, &dev_register, sizeof(tp2802_register)))
            return FAILURE;

        break;
    }
    case _IOC_NR(TP2802_SET_PTZ_MODE):
    {
        if (copy_from_user(&PTZ_data, argp, sizeof(tp2802_PTZ_data)))
        {
            return FAILURE;
        }

        if(PTZ_data.ch > PTZ_PIN_PTZ2)  return FAILURE;

        down(&watchdog_lock);

        chip = PTZ_data.chip;

        if( TP2854 == id[chip])
        {
            TP2854_PTZ_mode(chip, PTZ_data.ch, PTZ_data.mode );
            //TP2854_RX_init(chip, PTZ_data.mode);
        }
        up(&watchdog_lock);
        break;
    }
*/
    default:
    {
        printk("Invalid tp2802 ioctl cmd!\n");
        ret = -1;
        break;
    }
    }

    return ret;
}
#endif
//////////////////////////////////////////////////////////////////////////////
static void TP2854_RX_init(unsigned char chip, unsigned char mod)
{

    int i, index=0;
    unsigned char regA7=0x00;
    unsigned char regA8=0x00;

    //regC9~D7
    static const unsigned char PTZ_RX_dat[][15]=
    {
        {0x00,0x00,0x07,0x08,0x00,0x00,0x04,0x00,0x00,0x60,0x10,0x06,0xbe,0x39,0x27}, //TVI command
        {0x00,0x00,0x07,0x08,0x09,0x0a,0x04,0x00,0x00,0x60,0x10,0x06,0xbe,0x39,0x27}, //TVI burst
        {0x00,0x00,0x06,0x07,0x08,0x09,0x05,0xbf,0x11,0x60,0x0b,0x04,0xf0,0xd8,0x2f}, //ACP1 0.525
        {0x00,0x00,0x06,0x07,0x08,0x09,0x02,0xdf,0x88,0x60,0x10,0x04,0xf0,0xd8,0x17}, //ACP2 0.6
        //{0x00,0x00,0x06,0x07,0x08,0x09,0x04,0xec,0xe9,0x60,0x10,0x04,0xf0,0xd8,0x17}, //ACP3 0.35
        {0x00,0x00,0x07,0x08,0x09,0x0a,0x09,0xd9,0xd3,0x60,0x08,0x04,0xf0,0xd8,0x2f}, //ACP3 0.35
        {0x00,0x00,0x06,0x07,0x08,0x09,0x03,0x52,0x53,0x60,0x10,0x04,0xf0,0xd8,0x17}  //ACP1 0.525
    };

        if(PTZ_RX_TVI_CMD == mod)
        {
            index = 0;
            regA7 = 0x03;
            regA8 = 0x00;
        }
        else if(PTZ_RX_TVI_BURST == mod)
        {
            index = 1;
            regA7 = 0x03;
            regA8 = 0x00;
        }
        else if(PTZ_RX_ACP1 == mod)
        {
            index = 2;
            regA7 = 0x03;
            regA8 = 0x00;
        }
        else if(PTZ_RX_ACP2 == mod)
        {
            index = 3;
            regA7 = 0x27;
            regA8 = 0x0f;
        }
        else if(PTZ_RX_ACP3 == mod)
        {
            index = 4;
            regA7 = 0x03;
            regA8 = 0x00;
        }
        else if(PTZ_RX_TEST == mod)
        {
            index = 5;
            regA7 = 0x03;
            regA8 = 0x00;
        }

        for(i = 0; i < 15; i++)
        {
            tp28xx_byte_write(chip, 0xc9+i, PTZ_RX_dat[index][i]);
            tp28xx_byte_write(chip, 0xa8, regA8);
            tp28xx_byte_write(chip, 0xa7, regA7);
        }

}
#ifndef NVT_PLATFORM
static void TP2854_PTZ_mode(unsigned char chip, unsigned char ch, unsigned char mod)
{
    unsigned int tmp, i, index=0;

    static const unsigned char PTZ_reg[13]=
    {
        0x6f,0x70,0x71,0x72,0xc0,0xc1,0xc2,0xc3,0xc4,0xc5,0xc6,0xc7,0xc8
    };
    static const unsigned char PTZ_dat[][13]=
    {
        {0x02,0x40,0x20,0x00,0x00,0x00,0x0b,0x0c,0x0d,0x0e,0x19,0x78,0x21}, //TVI1.0
        {0x02,0x40,0x20,0x00,0x00,0x00,0x0b,0x0c,0x0d,0x0e,0x33,0xdf,0x21}, //TVI2.0
        {0x02,0x40,0x20,0x00,0x00,0x00,0x0e,0x0f,0x10,0x11,0x26,0xf0,0x57}, //A1080p for 2829 0.525
        {0x02,0x40,0x20,0x00,0x00,0x00,0x0e,0x0f,0x00,0x00,0x26,0xe0,0xef}, //A720p for 2829 0.525
        {0x02,0x40,0x20,0x00,0x00,0x00,0x0f,0x10,0x00,0x00,0x4a,0xf0,0x6f}, //960H for 2829
        {0x0a,0x5d,0xa0,0x00,0x00,0x00,0x10,0x11,0x12,0x13,0x15,0xb8,0x9f}, //HDC for 2829
        {0x02,0x40,0x20,0x00,0x00,0x00,0x0f,0x10,0x11,0x12,0x2c,0xf0,0x57}, //ACP 3M18 for 2829 8+0.6
        {0x02,0x40,0x20,0x00,0x00,0x00,0x0f,0x10,0x11,0x12,0x19,0xd0,0x17}, //ACP 3M2530 for 2829 4+0.35
        {0x06,0x5f,0x20,0x00,0x00,0x00,0x0f,0x10,0x12,0x16,0x16,0xd0,0x57}, //ACP 4M2530_5M20 for 2829 7+0.3
        {0x06,0x5f,0x20,0x00,0x00,0x00,0x0f,0x10,0x12,0x16,0x2c,0xf0,0x97}, //ACP 4M15 5M12.5 for 2829 8+0.6
        {0x0a,0x5d,0xa0,0x01,0x00,0x00,0x16,0x17,0x18,0x19,0x2a,0xf0,0x17}, //HDC QHD25/30 for 2829
        {0x0a,0x5f,0xa0,0x00,0x00,0x00,0x1a,0x1b,0x1c,0x1d,0x36,0x50,0x5f}, //HDC 4K12.5 for 2829
        {0x0a,0x5f,0xa0,0x01,0x00,0x00,0x1a,0x1b,0x1c,0x1d,0x36,0x50,0x57}, //HDC 4K15 for 2829
        {0x0a,0x5f,0xa0,0x01,0x00,0x00,0x1a,0x1b,0x1c,0x1d,0x2c,0x50,0x57}  //HDC 6M20 for 2829
    };

    tp28xx_byte_write(chip, 0x40, ch); //reg bank1 switch for 2826

        if(PTZ_TVI == mod)
        {
            tmp = tp28xx_byte_read(chip, 0xf5); //check TVI 1 or 2
            if( (tmp >>ch) & 0x01)
            {
                index = 1;
            }
            else
            {
                index = 0;
            }
        }
        else if(PTZ_HDA_1080P == mod) //HDA 1080p
        {
                index = 2;
        }
        else if(PTZ_HDA_720P == mod) //HDA 720p
        {
                index = 3;
        }
        else if(PTZ_HDA_CVBS == mod) //HDA CVBS
        {
                index = 4;
        }
        else if(PTZ_HDC == mod) // test
        {
                index = 5;
        }
        else if(PTZ_HDA_3M18 == mod) // test
        {
                index = 6;
        }
        else if(PTZ_HDA_3M25 == mod) // test
        {
                index = 7;
        }
        else if(PTZ_HDA_4M25 == mod) // test
        {
                index = 8;
        }
        else if(PTZ_HDA_4M15 == mod) // test
        {
                index = 9;
        }
        else if(PTZ_HDC_QHD == mod) // test
        {
                index = 10;
        }
        else if(PTZ_HDC_8M12 == mod) // test
        {
                index = 11;
        }
        else if(PTZ_HDC_8M15 == mod) // test
        {
                index = 12;
        }
        else if(PTZ_HDC_6M20 == mod) // test
        {
                index = 13;
        }


     for(i = 0; i < 13; i++)
     {
         tp28xx_byte_write(chip, PTZ_reg[i], PTZ_dat[index][i]);
     }

}
#endif
static void TP2854_reset_default(unsigned char chip, unsigned char ch)
{
    unsigned int tmp;

    tp28xx_byte_write(chip, 0x07, 0xC0);
    tp28xx_byte_write(chip, 0x0B, 0xC0);
    tmp = tp28xx_byte_read(chip, 0x26);
    tmp &= 0xfe;
    tp28xx_byte_write(chip, 0x26, tmp);
    tmp = tp28xx_byte_read(chip, 0xa7);
    tmp &= 0xfe;
    tp28xx_byte_write(chip, 0xa7, tmp);
    tmp = tp28xx_byte_read(chip, 0x06);
    tmp &= 0xfb;
    tp28xx_byte_write(chip, 0x06, tmp);
}
//////////////////////////////////////////////////////////////
static void TP2854_NTSC_DataSet(unsigned char chip)
{
    unsigned char tmp;
    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x13);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x40);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0x2d);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x70);
    tp28xx_byte_write(chip, 0x2c, 0x2a);
    tp28xx_byte_write(chip, 0x2d, 0x68);
    tp28xx_byte_write(chip, 0x2e, 0x57);

    tp28xx_byte_write(chip, 0x30, 0x62);
    tp28xx_byte_write(chip, 0x31, 0xbb);
    tp28xx_byte_write(chip, 0x32, 0x96);
    tp28xx_byte_write(chip, 0x33, 0xc0);
    //tp28xx_byte_write(chip, 0x35, 0x25);
    tp28xx_byte_write(chip, 0x38, 0x00);
    tp28xx_byte_write(chip, 0x39, 0x04);
    tp28xx_byte_write(chip, 0x3a, 0x32);
    tp28xx_byte_write(chip, 0x3B, 0x26);

    tp28xx_byte_write(chip, 0x18, 0x12);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}
static void TP2854_PAL_DataSet(unsigned char chip)
{
    unsigned char tmp;
    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x13);
    tp28xx_byte_write(chip, 0x0d, 0x51);

    tp28xx_byte_write(chip, 0x20, 0x48);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x37);
    tp28xx_byte_write(chip, 0x23, 0x3f);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0x2d);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x70);
    tp28xx_byte_write(chip, 0x2c, 0x2a);
    tp28xx_byte_write(chip, 0x2d, 0x64);
    tp28xx_byte_write(chip, 0x2e, 0x56);

    tp28xx_byte_write(chip, 0x30, 0x7a);
    tp28xx_byte_write(chip, 0x31, 0x4a);
    tp28xx_byte_write(chip, 0x32, 0x4d);
    tp28xx_byte_write(chip, 0x33, 0xf0);
    //tp28xx_byte_write(chip, 0x35, 0x25);
    tp28xx_byte_write(chip, 0x38, 0x00);
    tp28xx_byte_write(chip, 0x39, 0x04);
    tp28xx_byte_write(chip, 0x3a, 0x32);
    tp28xx_byte_write(chip, 0x3B, 0x26);

    tp28xx_byte_write(chip, 0x18, 0x17);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}
static void TP2854_V1_DataSet(unsigned char chip)
{
    unsigned char tmp;
    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x30);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0x2d);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x30);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0xbb);
    tp28xx_byte_write(chip, 0x32, 0x2e);
    tp28xx_byte_write(chip, 0x33, 0x90);
    //tp28xx_byte_write(chip, 0x35, 0x05);
    tp28xx_byte_write(chip, 0x38, 0x00);
    tp28xx_byte_write(chip, 0x39, 0x1c);
    tp28xx_byte_write(chip, 0x3a, 0x32);
    tp28xx_byte_write(chip, 0x3B, 0x26);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}
static void TP2854_V2_DataSet(unsigned char chip)
{
    unsigned char tmp;
    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x13);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x30);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0x2d);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x30);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0xbb);
    tp28xx_byte_write(chip, 0x32, 0x2e);
    tp28xx_byte_write(chip, 0x33, 0x90);
    //tp28xx_byte_write(chip, 0x35, 0x25);
    tp28xx_byte_write(chip, 0x38, 0x00);
    tp28xx_byte_write(chip, 0x39, 0x18);
    tp28xx_byte_write(chip, 0x3a, 0x32);
    tp28xx_byte_write(chip, 0x3B, 0x26);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}

/////HDA QHD30
static void TP2854_AQHDP30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tp28xx_byte_write(chip, 0x15, 0x23);
    tp28xx_byte_write(chip, 0x16, 0x16);
    tp28xx_byte_write(chip, 0x18, 0x32);

    tp28xx_byte_write(chip, 0x20, 0x80);
    tp28xx_byte_write(chip, 0x21, 0x86);
    tp28xx_byte_write(chip, 0x22, 0x36);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2d, 0xa0);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x6a);
    tp28xx_byte_write(chip, 0x32, 0xbe);
    tp28xx_byte_write(chip, 0x33, 0x80);
    //tp28xx_byte_write(chip, 0x35, 0x15);
    tp28xx_byte_write(chip, 0x39, 0x40);
}

/////HDA QHD25
static void TP2854_AQHDP25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tp28xx_byte_write(chip, 0x15, 0x23);
    tp28xx_byte_write(chip, 0x16, 0x16);
    tp28xx_byte_write(chip, 0x18, 0x32);

    tp28xx_byte_write(chip, 0x20, 0x80);
    tp28xx_byte_write(chip, 0x21, 0x86);
    tp28xx_byte_write(chip, 0x22, 0x36);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2d, 0xa0);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x6f);
    tp28xx_byte_write(chip, 0x32, 0xb5);
    tp28xx_byte_write(chip, 0x33, 0x80);
    //tp28xx_byte_write(chip, 0x35, 0x15);
    tp28xx_byte_write(chip, 0x39, 0x40);
}

/////HDA QXGA30
static void TP2854_AQXGAP30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);

    tp28xx_byte_write(chip, 0x13, 0x00);

    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x90);
    tp28xx_byte_write(chip, 0x21, 0x86);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0xad);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0xa0);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x68);
    tp28xx_byte_write(chip, 0x32, 0xbe);
    tp28xx_byte_write(chip, 0x33, 0x80);

    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x40);
    tp28xx_byte_write(chip, 0x3a, 0x12);
    tp28xx_byte_write(chip, 0x3b, 0x26);
}

/////HDA QXGA25
static void TP2854_AQXGAP25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x60;
    tp28xx_byte_write(chip, 0x14, tmp);
    tp28xx_byte_write(chip, 0x13, 0x00);

    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x90);
    tp28xx_byte_write(chip, 0x21, 0x86);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0xad);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0xa0);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x6c);
    tp28xx_byte_write(chip, 0x32, 0xbe);
    tp28xx_byte_write(chip, 0x33, 0x80);

    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x40);
    tp28xx_byte_write(chip, 0x3a, 0x12);
    tp28xx_byte_write(chip, 0x3b, 0x26);
}


/////HDC QHD30
static void TP2854_CQHDP30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);

    tp28xx_byte_write(chip, 0x13, 0x40);
    tp28xx_byte_write(chip, 0x15, 0x13);
    tp28xx_byte_write(chip, 0x16, 0xfa);
    tp28xx_byte_write(chip, 0x18, 0x38);
    tp28xx_byte_write(chip, 0x1c, 0x0c);
    tp28xx_byte_write(chip, 0x1d, 0x80);

    tp28xx_byte_write(chip, 0x20, 0x50);
    tp28xx_byte_write(chip, 0x21, 0x86);
    tp28xx_byte_write(chip, 0x22, 0x38);

    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xda);

    tp28xx_byte_write(chip, 0x2d, 0x6c);
    tp28xx_byte_write(chip, 0x2e, 0x50);

    tp28xx_byte_write(chip, 0x30, 0x75);
    tp28xx_byte_write(chip, 0x31, 0x39);
    tp28xx_byte_write(chip, 0x32, 0xc0);
    tp28xx_byte_write(chip, 0x33, 0x31);

    tp28xx_byte_write(chip, 0x39, 0x48);
}

/////HDC QHD25
static void TP2854_CQHDP25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);

    tp28xx_byte_write(chip, 0x13, 0x40);
    tp28xx_byte_write(chip, 0x15, 0x13);
    tp28xx_byte_write(chip, 0x16, 0xd8);
    tp28xx_byte_write(chip, 0x18, 0x30);
    tp28xx_byte_write(chip, 0x1c, 0x0c);
    tp28xx_byte_write(chip, 0x1d, 0x80);

    tp28xx_byte_write(chip, 0x20, 0x50);
    tp28xx_byte_write(chip, 0x21, 0x86);
    tp28xx_byte_write(chip, 0x22, 0x38);

    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xda);

    tp28xx_byte_write(chip, 0x2d, 0x6c);
    tp28xx_byte_write(chip, 0x2e, 0x50);

    tp28xx_byte_write(chip, 0x30, 0x75);
    tp28xx_byte_write(chip, 0x31, 0x39);
    tp28xx_byte_write(chip, 0x32, 0xc0);
    tp28xx_byte_write(chip, 0x33, 0x3b);

    tp28xx_byte_write(chip, 0x39, 0x48);
}

///////HDA QHD15
static void TP2854_AQHDP15_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
    tp28xx_byte_write(chip, 0x13, 0x00);

    tp28xx_byte_write(chip, 0x20, 0x38);
    tp28xx_byte_write(chip, 0x21, 0x46);

    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x01);

    tp28xx_byte_write(chip, 0x2d, 0x44);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x29);
    tp28xx_byte_write(chip, 0x31, 0x68);
    tp28xx_byte_write(chip, 0x32, 0x78);
    tp28xx_byte_write(chip, 0x33, 0x10);

    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x40);
    tp28xx_byte_write(chip, 0x3a, 0x12);
}

/////HDA QXGA18
static void TP2854_AQXGAP18_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
    tp28xx_byte_write(chip, 0x13, 0x00);

    tp28xx_byte_write(chip, 0x15, 0x13);
    tp28xx_byte_write(chip, 0x16, 0x10);
    tp28xx_byte_write(chip, 0x18, 0x68);

    tp28xx_byte_write(chip, 0x20, 0x48);
    tp28xx_byte_write(chip, 0x21, 0x46);
    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x05);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2d, 0x52);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x29);
    tp28xx_byte_write(chip, 0x31, 0x65);
    tp28xx_byte_write(chip, 0x32, 0x2b);
    tp28xx_byte_write(chip, 0x33, 0xd0);

    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x40);
    tp28xx_byte_write(chip, 0x3a, 0x12);
}

/////TVI QHD30/QHD25
static void TP2854_QHDP30_25_DataSet(unsigned char chip)
{
    unsigned char tmp;

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);

    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x15, 0x23);
    tp28xx_byte_write(chip, 0x16, 0x1b);
    tp28xx_byte_write(chip, 0x18, 0x38);

    tp28xx_byte_write(chip, 0x20, 0x50);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x58);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x74);
    tp28xx_byte_write(chip, 0x31, 0x58);
    tp28xx_byte_write(chip, 0x32, 0x9f);
    tp28xx_byte_write(chip, 0x33, 0x60);

    //tp28xx_byte_write(chip, 0x35, 0x05);
    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x48);
    tp28xx_byte_write(chip, 0x3a, 0x12);
    tp28xx_byte_write(chip, 0x3b, 0x26);
}

/////TVI 5M20
static void TP2854_5MP20_DataSet(unsigned char chip)
{
    unsigned char tmp;
    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x50);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x54);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x74);
    tp28xx_byte_write(chip, 0x31, 0xa7);
    tp28xx_byte_write(chip, 0x32, 0x18);
    tp28xx_byte_write(chip, 0x33, 0x50);

    //tp28xx_byte_write(chip, 0x35, 0x05);
    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x48);
    tp28xx_byte_write(chip, 0x3a, 0x12);
    tp28xx_byte_write(chip, 0x3b, 0x26);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}

/////HDA 5M20
static void TP2854_A5MP20_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
    tp28xx_byte_write(chip, 0x20, 0x80);
    tp28xx_byte_write(chip, 0x21, 0x86);
    tp28xx_byte_write(chip, 0x22, 0x36);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2d, 0xA0);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x77);
    tp28xx_byte_write(chip, 0x32, 0x0e);
    tp28xx_byte_write(chip, 0x33, 0xa0);
    tp28xx_byte_write(chip, 0x39, 0x48);
}

/////TVI 8M15
static void TP2854_8MP15_DataSet(unsigned char chip)
{
    unsigned char tmp;
    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x60);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x58);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x74);
    tp28xx_byte_write(chip, 0x31, 0x59);
    tp28xx_byte_write(chip, 0x32, 0xbd);
    tp28xx_byte_write(chip, 0x33, 0x60);

    //tp28xx_byte_write(chip, 0x35, 0x05);
    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x48);
    tp28xx_byte_write(chip, 0x3a, 0x12);
    tp28xx_byte_write(chip, 0x3b, 0x26);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}
///////HDA 5M12.5
static void TP2854_A5MP12_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);
    tp28xx_byte_write(chip, 0x13, 0x00);

    tp28xx_byte_write(chip, 0x20, 0x38);
    tp28xx_byte_write(chip, 0x21, 0x46);

    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x01);

    tp28xx_byte_write(chip, 0x2d, 0x44);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x29);
    tp28xx_byte_write(chip, 0x31, 0x68);
    tp28xx_byte_write(chip, 0x32, 0x78);
    tp28xx_byte_write(chip, 0x33, 0x10);

    tp28xx_byte_write(chip, 0x18, 0x14);
    tp28xx_byte_write(chip, 0x1d, 0xb8);
    tp28xx_byte_write(chip, 0x36, 0xbc);

    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x40);
    tp28xx_byte_write(chip, 0x3a, 0x12);
}
///////////////////////////////////////////////////////////////////
static void TP2854_A720P30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x02);
    tmp |= 0x04;
    tp28xx_byte_write(chip, 0x02, tmp);

    tp28xx_byte_write(chip, 0x0d, 0x70);

    tp28xx_byte_write(chip, 0x20, 0x40);
    tp28xx_byte_write(chip, 0x21, 0x46);

    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x01);

    tp28xx_byte_write(chip, 0x2c, 0x3a);
    tp28xx_byte_write(chip, 0x2d, 0x5a);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x9d);
    tp28xx_byte_write(chip, 0x31, 0xca);
    tp28xx_byte_write(chip, 0x32, 0x01);
    tp28xx_byte_write(chip, 0x33, 0xd0);
}
static void TP2854_A720P25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x02);
    tmp |= 0x04;
    tp28xx_byte_write(chip, 0x02, tmp);

    tp28xx_byte_write(chip, 0x0d, 0x71);

    tp28xx_byte_write(chip, 0x20, 0x40);
    tp28xx_byte_write(chip, 0x21, 0x46);

    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x01);

    tp28xx_byte_write(chip, 0x2c, 0x3a);
    tp28xx_byte_write(chip, 0x2d, 0x5a);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x9e);
    tp28xx_byte_write(chip, 0x31, 0x20);
    tp28xx_byte_write(chip, 0x32, 0x10);
    tp28xx_byte_write(chip, 0x33, 0x90);
}
static void TP2854_A1080P30_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x02);
    tmp |= 0x04;
    tp28xx_byte_write(chip, 0x02, tmp);

    tp28xx_byte_write(chip, 0x15, 0x01);
    tp28xx_byte_write(chip, 0x16, 0xf0);

    tp28xx_byte_write(chip, 0x0d, 0x72);

    tp28xx_byte_write(chip, 0x20, 0x38);
    tp28xx_byte_write(chip, 0x21, 0x46);

    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x0d);

    tp28xx_byte_write(chip, 0x2c, 0x3a);
    tp28xx_byte_write(chip, 0x2d, 0x54);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0xa5);
    tp28xx_byte_write(chip, 0x31, 0x95);
    tp28xx_byte_write(chip, 0x32, 0xe0);
    tp28xx_byte_write(chip, 0x33, 0x60);
}
static void TP2854_A1080P25_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x02);
    tmp |= 0x04;
    tp28xx_byte_write(chip, 0x02, tmp);

    tp28xx_byte_write(chip, 0x15, 0x01);
    tp28xx_byte_write(chip, 0x16, 0xf0);

    tp28xx_byte_write(chip, 0x0d, 0x73);

    tp28xx_byte_write(chip, 0x20, 0x3c);
    tp28xx_byte_write(chip, 0x21, 0x46);

    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x0d);

    tp28xx_byte_write(chip, 0x2c, 0x3a);
    tp28xx_byte_write(chip, 0x2d, 0x54);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0xa5);
    tp28xx_byte_write(chip, 0x31, 0x86);
    tp28xx_byte_write(chip, 0x32, 0xfb);
    tp28xx_byte_write(chip, 0x33, 0x60);
}
static void TP2854_1080P60_DataSet(unsigned char chip)
{
    unsigned char tmp;

    //tp28xx_byte_write(chip, 0x07, 0xc0);
    tp28xx_byte_write(chip, 0x0b, 0xc0);
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tp28xx_byte_write(chip, 0x15, 0x03);
    tp28xx_byte_write(chip, 0x16, 0xf0);
    tp28xx_byte_write(chip, 0x17, 0x80);
    tp28xx_byte_write(chip, 0x18, 0x12);
    tp28xx_byte_write(chip, 0x19, 0x38);
    tp28xx_byte_write(chip, 0x1a, 0x47);
    tp28xx_byte_write(chip, 0x1c, 0x08);
    tp28xx_byte_write(chip, 0x1d, 0x96);

    tp28xx_byte_write(chip, 0x20, 0x50);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x23, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xad);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x40);
    tp28xx_byte_write(chip, 0x2e, 0x70);

    tp28xx_byte_write(chip, 0x30, 0x74);
    tp28xx_byte_write(chip, 0x31, 0x9b);
    tp28xx_byte_write(chip, 0x32, 0xa5);
    tp28xx_byte_write(chip, 0x33, 0xe0);

    //tp28xx_byte_write(chip, 0x35, 0x05);
    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x48);
    tp28xx_byte_write(chip, 0x3a, 0x12);
    tp28xx_byte_write(chip, 0x3b, 0x26);

    tp28xx_byte_write(chip, 0x13, 0x00);
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);
}
/////HDC 8M12
static void TP2854_C8MP12_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tp28xx_byte_write(chip, 0x0c, 0x03);
    tp28xx_byte_write(chip, 0x0d, 0x50);

    tmp = tp28xx_byte_read(chip, 0x14);
    tmp &= 0x9f;
    tp28xx_byte_write(chip, 0x14, tmp);

    tp28xx_byte_write(chip, 0x13, 0x40);
    tp28xx_byte_write(chip, 0x15, 0x23);
    tp28xx_byte_write(chip, 0x16, 0xf8);
    tp28xx_byte_write(chip, 0x18, 0x50);

    tp28xx_byte_write(chip, 0x20, 0x68);
    tp28xx_byte_write(chip, 0x21, 0x84);
    tp28xx_byte_write(chip, 0x22, 0x36);
    tp28xx_byte_write(chip, 0x22, 0x3c);

    tp28xx_byte_write(chip, 0x25, 0xff);
    tp28xx_byte_write(chip, 0x26, 0x05);
    tp28xx_byte_write(chip, 0x27, 0xda);
    tp28xx_byte_write(chip, 0x28, 0x00);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2c, 0x0a);
    tp28xx_byte_write(chip, 0x2d, 0x84);
    tp28xx_byte_write(chip, 0x2e, 0x50);

    tp28xx_byte_write(chip, 0x30, 0x75);
    tp28xx_byte_write(chip, 0x31, 0x39);
    tp28xx_byte_write(chip, 0x32, 0xc0);
    tp28xx_byte_write(chip, 0x33, 0x32);

    tp28xx_byte_write(chip, 0x38, 0x40);
    tp28xx_byte_write(chip, 0x39, 0x48);
    tp28xx_byte_write(chip, 0x3a, 0x12);
    tp28xx_byte_write(chip, 0x3b, 0x26);
}
/////HDA 8M15
static void TP2854_A8MP15_DataSet(unsigned char chip)
{
    unsigned char tmp;
    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x40;
    tp28xx_byte_write(chip, 0x14, tmp);

    //tp28xx_byte_write(chip, 0x13, 0x00);
    //tp28xx_byte_write(chip, 0x15, 0x23);
    tp28xx_byte_write(chip, 0x16, 0x70);
    //tp28xx_byte_write(chip, 0x18, 0x32);

    //tp28xx_byte_write(chip, 0x20, 0x80);
    //tp28xx_byte_write(chip, 0x21, 0x86);
    //tp28xx_byte_write(chip, 0x22, 0x36);

    //tp28xx_byte_write(chip, 0x25, 0xff);
    //tp28xx_byte_write(chip, 0x26, 0x05);
    //tp28xx_byte_write(chip, 0x27, 0xad);

    //tp28xx_byte_write(chip, 0x2b, 0x60);
    //tp28xx_byte_write(chip, 0x2d, 0xa0);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x68);
    tp28xx_byte_write(chip, 0x32, 0x43);
    tp28xx_byte_write(chip, 0x33, 0x06);
    //tp28xx_byte_write(chip, 0x35, 0x15);
    tp28xx_byte_write(chip, 0x39, 0x40);
}
static void TP2854_A720P60_DataSet(unsigned char chip)
{
    unsigned char tmp;

    tmp = tp28xx_byte_read(chip, 0x14);
    tmp |= 0x60;
    tp28xx_byte_write(chip, 0x14, tmp);

    tp28xx_byte_write(chip, 0x20, 0x38);
    tp28xx_byte_write(chip, 0x21, 0x46);

    tp28xx_byte_write(chip, 0x25, 0xfe);
    tp28xx_byte_write(chip, 0x26, 0x01);

    tp28xx_byte_write(chip, 0x2d, 0x44);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x29);
    tp28xx_byte_write(chip, 0x31, 0x62);
    tp28xx_byte_write(chip, 0x32, 0x78);
    tp28xx_byte_write(chip, 0x33, 0x10);

    //tp28xx_byte_write(chip, 0x3B, 0x05);
}

static void TP2854_C1080P30_DataSet(unsigned char chip);
static void TP2854_C1080P25_DataSet(unsigned char chip);
static void TP2854_C720P30_DataSet(unsigned char chip);
static void TP2854_C720P25_DataSet(unsigned char chip);
static void TP2854_C720P60_DataSet(unsigned char chip);
static void TP2854_C720P50_DataSet(unsigned char chip);
static void TP2854_SYSCLK_V1(unsigned char chip, unsigned char ch);
static void TP2854_SYSCLK_V2(unsigned char chip, unsigned char ch);
static void TP2854_SYSCLK_V3(unsigned char chip, unsigned char ch);
static void TP28xx_reset_default(int chip, unsigned char ch);
static int tp2802_set_video_mode(unsigned char chip, unsigned char mode, unsigned char ch, unsigned char std)
{
    int err=0;
    unsigned int tmp;

    if(STD_HDA_DEFAULT == std) std = STD_HDA;

    // Set Page Register to the appropriate Channel
    tp2802_set_reg_page(chip, ch);

    switch(mode)
    //switch(mode&(~FLAG_HALF_MODE))
    {
    case TP2802_1080P25:
        tp28xx_byte_write(chip, 0x35, 0x05);
        tp2802_set_work_mode_1080p25(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        if( STD_HDA == std)
        {
                TP2854_A1080P25_DataSet(chip);
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {
            {
                TP2854_C1080P25_DataSet(chip);
            }
            if(STD_HDC == std) //HDC 1080p25 position adjust
               {
                    {
                        tp28xx_byte_write(chip, 0x15, 0x13);
                        tp28xx_byte_write(chip, 0x16, 0x60);
                        tp28xx_byte_write(chip, 0x17, 0x80);
                        tp28xx_byte_write(chip, 0x18, 0x29);
                        tp28xx_byte_write(chip, 0x19, 0x38);
                        tp28xx_byte_write(chip, 0x1A, 0x47);
                        tp28xx_byte_write(chip, 0x1C, 0x09);
                        tp28xx_byte_write(chip, 0x1D, 0x60);
                    }
               }
        }

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_1080P30:
        tp28xx_byte_write(chip, 0x35, 0x05);
        tp2802_set_work_mode_1080p30(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        if( STD_HDA == std)
        {
            {
                TP2854_A1080P30_DataSet(chip);
            }
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {
            {
                TP2854_C1080P30_DataSet(chip);
            }
            if(STD_HDC == std) //HDC 1080p30 position adjust
               {
                    {
                        tp28xx_byte_write(chip, 0x15, 0x13);
                        tp28xx_byte_write(chip, 0x16, 0x60);
                        tp28xx_byte_write(chip, 0x17, 0x80);
                        tp28xx_byte_write(chip, 0x18, 0x29);
                        tp28xx_byte_write(chip, 0x19, 0x38);
                        tp28xx_byte_write(chip, 0x1A, 0x47);
                        tp28xx_byte_write(chip, 0x1C, 0x09);
                        tp28xx_byte_write(chip, 0x1D, 0x60);
                    }
               }
        }

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_720P25:
        tp28xx_byte_write(chip, 0x35, 0x05);
        tp2802_set_work_mode_720p25(chip);
        tp28xx_byte_write(chip, 0x02, 0xc2);
        TP2854_V1_DataSet(chip);

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_720P30:
        tp28xx_byte_write(chip, 0x35, 0x05);
        tp2802_set_work_mode_720p30(chip);
        tp28xx_byte_write(chip, 0x02, 0xc2);
        TP2854_V1_DataSet(chip);

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_720P50:
        tp28xx_byte_write(chip, 0x35, 0x05);
        tp2802_set_work_mode_720p50(chip);
        tp28xx_byte_write(chip, 0x02, 0xc2);
        TP2854_V1_DataSet(chip);

        if(STD_HDA == std)
        {

        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {
            {
                TP2854_C720P50_DataSet(chip);
            }
            if(STD_HDC == std) //HDC 720p50 position adjust
               {

                    {
                        tp28xx_byte_write(chip, 0x15, 0x13);
                        tp28xx_byte_write(chip, 0x16, 0x0a);
                        tp28xx_byte_write(chip, 0x17, 0x00);
                        tp28xx_byte_write(chip, 0x18, 0x19);
                        tp28xx_byte_write(chip, 0x19, 0xd0);
                        tp28xx_byte_write(chip, 0x1A, 0x25);
                        tp28xx_byte_write(chip, 0x1C, 0x06);
                        tp28xx_byte_write(chip, 0x1D, 0x7a);
                    }
               }
        }

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_720P60:
        tp28xx_byte_write(chip, 0x35, 0x05);
        tp2802_set_work_mode_720p60(chip);
        tp28xx_byte_write(chip, 0x02, 0xc2);
        TP2854_V1_DataSet(chip);

        if(STD_HDA == std)
        {
            {
                TP2854_A720P60_DataSet(chip);
            }
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {

            {
                TP2854_C720P60_DataSet(chip);
            }
            if(STD_HDC == std) //HDC 720p60 position adjust
               {
                    {
                        tp28xx_byte_write(chip, 0x15, 0x13);
                        tp28xx_byte_write(chip, 0x16, 0x08);
                        tp28xx_byte_write(chip, 0x17, 0x00);
                        tp28xx_byte_write(chip, 0x18, 0x19);
                        tp28xx_byte_write(chip, 0x19, 0xd0);
                        tp28xx_byte_write(chip, 0x1A, 0x25);
                        tp28xx_byte_write(chip, 0x1C, 0x06);
                        tp28xx_byte_write(chip, 0x1D, 0x72);
                    }
               }

        }

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_720P30V2:
        tp28xx_byte_write(chip, 0x35, 0x25);
        tp2802_set_work_mode_720p60(chip);
        tp28xx_byte_write(chip, 0x02, 0xc2);
        TP2854_V2_DataSet(chip);

        if(STD_HDA == std)
        {
            {
                TP2854_A720P30_DataSet(chip);
            }
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {
            {
                TP2854_C720P30_DataSet(chip);
            }
            if(STD_HDC == std) //HDC 720p30 position adjust
               {
                    {
                        tp28xx_byte_write(chip, 0x15, 0x13);
                        tp28xx_byte_write(chip, 0x16, 0x08);
                        tp28xx_byte_write(chip, 0x17, 0x00);
                        tp28xx_byte_write(chip, 0x18, 0x19);
                        tp28xx_byte_write(chip, 0x19, 0xd0);
                        tp28xx_byte_write(chip, 0x1A, 0x25);
                        tp28xx_byte_write(chip, 0x1C, 0x06);
                        tp28xx_byte_write(chip, 0x1D, 0x72);
                    }
               }
        }

        TP2854_SYSCLK_V2(chip, ch);
        break;

    case TP2802_720P25V2:
        tp28xx_byte_write(chip, 0x35, 0x25);
        tp2802_set_work_mode_720p50(chip);
        tp28xx_byte_write(chip, 0x02, 0xc2);
        TP2854_V2_DataSet(chip);

        if(STD_HDA == std)
        {
            {
                TP2854_A720P25_DataSet(chip);
            }
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {
            {
                TP2854_C720P25_DataSet(chip);
            }
            if(STD_HDC == std) //HDC 720p25 position adjust
               {
                    {
                        tp28xx_byte_write(chip, 0x15, 0x13);
                        tp28xx_byte_write(chip, 0x16, 0x0a);
                        tp28xx_byte_write(chip, 0x17, 0x00);
                        tp28xx_byte_write(chip, 0x18, 0x19);
                        tp28xx_byte_write(chip, 0x19, 0xd0);
                        tp28xx_byte_write(chip, 0x1A, 0x25);
                        tp28xx_byte_write(chip, 0x1C, 0x06);
                        tp28xx_byte_write(chip, 0x1D, 0x7a);
                    }
               }

        }

        TP2854_SYSCLK_V2(chip, ch);
        break;

    case TP2802_PAL:
    	if( TP2854B == id[chip])
    	{
        	tp28xx_byte_write(chip, 0x35, 0x65);
    	}
	else
	{
        	tp28xx_byte_write(chip, 0x35, 0x25);
	}
        tp2802_set_work_mode_PAL(chip);
        tp28xx_byte_write(chip, 0x02, 0xc7);
        TP2854_PAL_DataSet(chip);

        TP2854_SYSCLK_V2(chip, ch);
        break;

    case TP2802_NTSC:
    	if( TP2854B == id[chip])
    	{
        	tp28xx_byte_write(chip, 0x35, 0x65);
    	}
	else
	{
        	tp28xx_byte_write(chip, 0x35, 0x25);
	}
        tp2802_set_work_mode_NTSC(chip);
        tp28xx_byte_write(chip, 0x02, 0xc7);
        TP2854_NTSC_DataSet(chip);

        TP2854_SYSCLK_V2(chip, ch);
        break;

    case TP2802_3M18:
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x30);
        tp2802_set_work_mode_3M(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_5M12:
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0xD0);
        tp2802_set_work_mode_5M(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        if( STD_HDA == std)
        {
               TP2854_A5MP12_DataSet(chip);
        }
        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_4M15:
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        tp2802_set_work_mode_4M(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_3M20:
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        tp2802_set_work_mode_3M20(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        tp28xx_byte_write(chip, 0x2d, 0x26);

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_4M12:
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0x08);
        tp2802_set_work_mode_4M12(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_6M10:
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0xbc);
        tp2802_set_work_mode_6M10(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);

        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_QHD30:
        tp28xx_byte_write(chip, 0x35, 0x15);
        tp28xx_byte_write(chip, 0x36, 0xdc);
        tp28xx_byte_write(chip, 0x02, 0xd0);
        tp2802_set_work_mode_QHD30(chip);
        TP2854_QHDP30_25_DataSet(chip);

        if( STD_HDA == std)
        {
                    TP2854_AQHDP30_DataSet(chip);
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {

                    TP2854_CQHDP30_DataSet(chip);
        }

        TP2854_SYSCLK_V3(chip, ch);
        break;

    case TP2802_QHD25:
        tp28xx_byte_write(chip, 0x35, 0x15);
        tp28xx_byte_write(chip, 0x36, 0xdc);
        tp28xx_byte_write(chip, 0x02, 0xd0);
        tp2802_set_work_mode_QHD25(chip);
        TP2854_QHDP30_25_DataSet(chip);
        if( STD_HDA == std)
        {
                    TP2854_AQHDP25_DataSet(chip);
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {

                    TP2854_CQHDP25_DataSet(chip);
        }

        TP2854_SYSCLK_V3(chip, ch);
        break;

    case TP2802_QHD15:
        tp28xx_byte_write(chip, 0x35, 0x15);
        tp28xx_byte_write(chip, 0x36, 0xdc);
        tp2802_set_work_mode_QHD15(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);
        if( STD_HDA == std)
        {
                    TP2854_AQHDP15_DataSet(chip);
        }
        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_QXGA18:
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        tp2802_set_work_mode_3M(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_V1_DataSet(chip);
        if( STD_HDA == std )
        {
                    TP2854_AQXGAP18_DataSet(chip);
        }
        TP2854_SYSCLK_V1(chip, ch);
        break;

    case TP2802_QXGA25:
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x72);
        tp2802_set_work_mode_QXGA25(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_AQXGAP25_DataSet(chip);

        TP2854_SYSCLK_V3(chip, ch);
        break;

    case TP2802_QXGA30:
        tp28xx_byte_write(chip, 0x35, 0x16);
        tp28xx_byte_write(chip, 0x36, 0x71);
        tp2802_set_work_mode_QXGA30(chip);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_AQXGAP30_DataSet(chip);

        TP2854_SYSCLK_V3(chip, ch);
        break;

    case TP2802_5M20:
        tp28xx_byte_write(chip, 0x35, 0x17);
        tp28xx_byte_write(chip, 0x36, 0xbc);
        tp2802_set_work_mode_5M20(chip);
        tp28xx_byte_write(chip, 0x02, 0xd0);
        TP2854_5MP20_DataSet(chip);
        if( STD_HDA == std)
        {
                    TP2854_A5MP20_DataSet(chip);
        }

        TP2854_SYSCLK_V3(chip, ch);
        //soft reset
        if(STD_TVI == std)
        {
            tmp = tp28xx_byte_read(chip, 0x06);
            tmp |=0x80;
            tp28xx_byte_write(chip, 0x06, tmp);
        }

        break;

    case TP2802_8M15:
        tp28xx_byte_write(chip, 0x35, 0x18);
        tp28xx_byte_write(chip, 0x36, 0xca);
        tp28xx_byte_write(chip, 0x02, 0xd0);
        tp2802_set_work_mode_8M15(chip);
        TP2854_8MP15_DataSet(chip);
        if( STD_HDA == std)
        {
                    TP2854_A8MP15_DataSet(chip);
        }
        else if(STD_HDC == std || STD_HDC_DEFAULT == std)
        {
        }
        TP2854_SYSCLK_V3(chip, ch);
        break;

    case TP2802_8M12:
        tp28xx_byte_write(chip, 0x35, 0x18);
        tp28xx_byte_write(chip, 0x36, 0xca);
        tp28xx_byte_write(chip, 0x02, 0xd0);
        tp2802_set_work_mode_8M12(chip);
        TP2854_8MP15_DataSet(chip);
        if( STD_HDA == std)
        {

        }
        else if(STD_HDC_DEFAULT == std)
        {
                    TP2854_C8MP12_DataSet(chip);
        }
        else if(STD_HDC == std)
        {
                    TP2854_C8MP12_DataSet(chip);
                    tp28xx_byte_write(chip, 0x1c, 0x13);
                    tp28xx_byte_write(chip, 0x1d, 0x10);
        }

        TP2854_SYSCLK_V3(chip, ch);
        break;

    case TP2802_1080P60:
        tp28xx_byte_write(chip, 0x35, 0x05);
        tp28xx_byte_write(chip, 0x02, 0xc0);
        TP2854_1080P60_DataSet(chip);
        TP2854_SYSCLK_V3(chip, ch);
        break;

    default:
        err = -1;
        break;
    }

    if( TP2854B == id[chip])
    {
        tmp = tp28xx_byte_read(chip, 0x02);
        tmp &= 0x7f;
        tp28xx_byte_write(chip, 0x02, tmp);
    }

    return err;
}


static void tp2802_set_reg_page(unsigned char chip, unsigned char ch)
{
    switch(ch)
    {
    case CH_1:
        tp28xx_byte_write(chip, 0x40, 0x00);
        break;  //
    case CH_2:
        tp28xx_byte_write(chip, 0x40, 0x01);
        break;  //
    case CH_3:
        tp28xx_byte_write(chip, 0x40, 0x02);
        break;  //
    case CH_4:
        tp28xx_byte_write(chip, 0x40, 0x03);
        break;  //
    case CH_ALL:
        tp28xx_byte_write(chip, 0x40, 0x04);
        break;  //
    case MIPI_PAGE:
        tp28xx_byte_write(chip, 0x40, 0x08);
        break;  //
    }
}
const char SYSTEM_AND[5]={0xfe,0xfd,0xfb,0xf7,0xf0};
const char __SYSTEM_MODE[5]={0x01,0x02,0x04,0x08,0x0f}; // NVT_PLATFORM: rename SYSTEM_MODE -> __SYSTEM_MODE for preventing build error
static void TP2854_SYSCLK_V2(unsigned char chip, unsigned char ch)
{
    unsigned char tmp;

	if( TP2854 == id[chip])
	{
    		tmp = tp28xx_byte_read(chip, 0x40);
		tp28xx_byte_write(chip, 0x40, 0x08);
		tp28xx_byte_write(chip, 0x23, 0x02);
		tp28xx_byte_write(chip, 0x13, 0x24);
		tp28xx_byte_write(chip, 0x14, 0x04);
		tp28xx_byte_write(chip, 0x15, 0x00);

		tp28xx_byte_write(chip, 0x25, 0x08);
		tp28xx_byte_write(chip, 0x26, 0x02);
		tp28xx_byte_write(chip, 0x27, 0x0b);

		tp28xx_byte_write(chip, 0x10, 0x88);
		tp28xx_byte_write(chip, 0x10, 0x08);
		tp28xx_byte_write(chip, 0x23, 0x00);
		tp28xx_byte_write(chip, 0x40, tmp);
	}
	else if( TP2854B == id[chip])
	{
		tp28xx_byte_write(chip, 0xf5, tp28xx_byte_read(chip, 0xf5) | __SYSTEM_MODE[ch]);

	}


}
static void TP2854_SYSCLK_V1(unsigned char chip, unsigned char ch)
{
    unsigned char tmp;

	if( TP2854 == id[chip])
	{
		tmp = tp28xx_byte_read(chip, 0x40);
		tp28xx_byte_write(chip, 0x40, 0x08);
		tp28xx_byte_write(chip, 0x23, 0x02);
		tp28xx_byte_write(chip, 0x13, 0x04);
		tp28xx_byte_write(chip, 0x14, 0x04);
		tp28xx_byte_write(chip, 0x15, 0x00);

		tp28xx_byte_write(chip, 0x25, 0x10);
		tp28xx_byte_write(chip, 0x26, 0x02);
		tp28xx_byte_write(chip, 0x27, 0x16);

		tp28xx_byte_write(chip, 0x10, 0x88);
		tp28xx_byte_write(chip, 0x10, 0x08);
		tp28xx_byte_write(chip, 0x23, 0x00);
		tp28xx_byte_write(chip, 0x40, tmp);
	}
	else if( TP2854B == id[chip])
	{
		tp28xx_byte_write(chip, 0xf5, tp28xx_byte_read(chip, 0xf5) & SYSTEM_AND[ch]);

		if( MIPI_4CH4LANE_297M == output[chip] || MIPI_2CH2LANE_297M == output[chip] || MIPI_4CH2LANE_594M == output[chip] || MIPI_1CH1LANE_297M == output[chip])
		{
			tp28xx_byte_write(chip, 0x35, tp28xx_byte_read(chip, 0x35) | 0x40 );
		}

	}


}
static void TP2854_SYSCLK_V3(unsigned char chip, unsigned char ch)
{
    unsigned char tmp;

	if( TP2854 == id[chip])
	{
		tmp = tp28xx_byte_read(chip, 0x40);
		tp28xx_byte_write(chip, 0x40, 0x08);
		tp28xx_byte_write(chip, 0x23, 0x02);
		tp28xx_byte_write(chip, 0x13, 0x04);
		tp28xx_byte_write(chip, 0x14, 0x05);
		tp28xx_byte_write(chip, 0x15, 0x04);

		tp28xx_byte_write(chip, 0x25, 0x10);
		tp28xx_byte_write(chip, 0x26, 0x02);
		tp28xx_byte_write(chip, 0x27, 0x18);

		tp28xx_byte_write(chip, 0x10, 0x88);
		tp28xx_byte_write(chip, 0x10, 0x08);
		tp28xx_byte_write(chip, 0x23, 0x00);
		tp28xx_byte_write(chip, 0x40, tmp);
	}
	else if( TP2854B == id[chip])
	{
		tp28xx_byte_write(chip, 0xf5, tp28xx_byte_read(chip, 0xf5) & SYSTEM_AND[ch]);

		if( MIPI_2CH4LANE_594M == output[chip] || MIPI_1CH4LANE_297M == output[chip])
		{
		}
		else
		{
			tp28xx_byte_write(chip, 0x35, tp28xx_byte_read(chip, 0x35) | 0x40 );
		}

	}

}

static void TP28xx_reset_default(int chip, unsigned char ch)
{
    //if(TP2854 == id[chip] )
    {
        TP2854_reset_default(chip, ch);
    }
}


static void TP2854_C1080P25_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x50);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x54);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa2);

}
static void TP2854_C720P25_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x3a);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);
    tp28xx_byte_write(chip, 0x2d, 0x36);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x67);
    tp28xx_byte_write(chip, 0x32, 0x6f);
    tp28xx_byte_write(chip, 0x33, 0x33);

}
static void TP2854_C720P50_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x3a);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x42);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa3);

}
static void TP2854_C1080P30_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x3c);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x4c);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa4);

}
static void TP2854_C720P30_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x30);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x37);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x48);
    tp28xx_byte_write(chip, 0x31, 0x67);
    tp28xx_byte_write(chip, 0x32, 0x6f);
    tp28xx_byte_write(chip, 0x33, 0x30);

}
static void TP2854_C720P60_DataSet(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x13, 0x40);

    tp28xx_byte_write(chip, 0x20, 0x30);

    tp28xx_byte_write(chip, 0x26, 0x01);
    tp28xx_byte_write(chip, 0x27, 0x5a);
    tp28xx_byte_write(chip, 0x28, 0x04);

    tp28xx_byte_write(chip, 0x2b, 0x60);

    tp28xx_byte_write(chip, 0x2d, 0x37);
    tp28xx_byte_write(chip, 0x2e, 0x40);

    tp28xx_byte_write(chip, 0x30, 0x41);
    tp28xx_byte_write(chip, 0x31, 0x82);
    tp28xx_byte_write(chip, 0x32, 0x27);
    tp28xx_byte_write(chip, 0x33, 0xa0);

}
///////////////////////////////////////////////////////////////

static void TP2854_output(unsigned char chip)
{

    tp28xx_byte_write(chip, 0x40, 0x08);

    tp28xx_byte_write(chip, 0x01, 0xf8);
    tp28xx_byte_write(chip, 0x02, 0x01);
    tp28xx_byte_write(chip, 0x08, 0x0f);

if(TP2854B == id[chip] )
{
		tp28xx_byte_write(chip, 0x10, 0x20);
		tp28xx_byte_write(chip, 0x11, 0x47);
		tp28xx_byte_write(chip, 0x12, 0x54);
		tp28xx_byte_write(chip, 0x13, 0xef);
		tp28xx_byte_write(chip, 0x0a, 0x80);
#if (CVBS_FRAME_OUTPUT)
		tp28xx_byte_write(chip, 0x21, 0x07);
#endif
        if (output[chip] == MIPI_4CH4LANE_594M) {
            tp28xx_byte_write(chip, 0x20, 0x44);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x15, 0x00);
            tp28xx_byte_write(chip, 0x25, 0x08);
            tp28xx_byte_write(chip, 0x26, 0x06);
            tp28xx_byte_write(chip, 0x27, 0x11);
            tp28xx_byte_write(chip, 0x29, 0x0a);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0x06);
            tp28xx_byte_write(chip, 0x14, 0x86);
            tp28xx_byte_write(chip, 0x14, 0x06);
        } else if (output[chip] == MIPI_2CH4LANE_594M) {
            tp28xx_byte_write(chip, 0x20, 0x24);
            tp28xx_byte_write(chip, 0x34, 0xd4); //output vin1&vin2
            tp28xx_byte_write(chip, 0x15, 0x00);
            tp28xx_byte_write(chip, 0x25, 0x08);
            tp28xx_byte_write(chip, 0x26, 0x06);
            tp28xx_byte_write(chip, 0x27, 0x11);
            tp28xx_byte_write(chip, 0x29, 0x0a);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0x06);
            tp28xx_byte_write(chip, 0x14, 0x86);
            tp28xx_byte_write(chip, 0x14, 0x06);
        } else if (output[chip] == MIPI_4CH4LANE_297M) {
            tp28xx_byte_write(chip, 0x20, 0x44);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x14, 0x47);
            tp28xx_byte_write(chip, 0x15, 0x01);
            tp28xx_byte_write(chip, 0x25, 0x04);
            tp28xx_byte_write(chip, 0x26, 0x03);
            tp28xx_byte_write(chip, 0x27, 0x09);
            tp28xx_byte_write(chip, 0x29, 0x02);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0xc7);
            tp28xx_byte_write(chip, 0x14, 0x47);
        } else if (output[chip] == MIPI_2CH4LANE_297M) {
            tp28xx_byte_write(chip, 0x20, 0x24);
            tp28xx_byte_write(chip, 0x34, 0xd4); //output vin1&vin2
            tp28xx_byte_write(chip, 0x14, 0x47);
            tp28xx_byte_write(chip, 0x15, 0x01);
            tp28xx_byte_write(chip, 0x25, 0x04);
            tp28xx_byte_write(chip, 0x26, 0x03);
            tp28xx_byte_write(chip, 0x27, 0x09);
            tp28xx_byte_write(chip, 0x29, 0x02);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0xc7);
            tp28xx_byte_write(chip, 0x14, 0x47);
        } else if (output[chip] == MIPI_1CH4LANE_297M) {
            tp28xx_byte_write(chip, 0x20, 0x14);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x14, 0x47);
            tp28xx_byte_write(chip, 0x15, 0x01);
            tp28xx_byte_write(chip, 0x25, 0x04);
            tp28xx_byte_write(chip, 0x26, 0x03);
            tp28xx_byte_write(chip, 0x27, 0x09);
            tp28xx_byte_write(chip, 0x29, 0x02);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0xc7);
            tp28xx_byte_write(chip, 0x14, 0x47);
        }else if (output[chip] == MIPI_4CH2LANE_594M) {
            tp28xx_byte_write(chip, 0x20, 0x42);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x15, 0x01);
            tp28xx_byte_write(chip, 0x25, 0x08);
            tp28xx_byte_write(chip, 0x26, 0x06);
            tp28xx_byte_write(chip, 0x27, 0x11);
            tp28xx_byte_write(chip, 0x29, 0x0a);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0x06);
            tp28xx_byte_write(chip, 0x14, 0x86);
            tp28xx_byte_write(chip, 0x14, 0x06);
        } else if (output[chip] == MIPI_2CH2LANE_594M) {
            tp28xx_byte_write(chip, 0x20, 0x22);
            tp28xx_byte_write(chip, 0x34, 0xd4); //output vin1&vin2
            tp28xx_byte_write(chip, 0x15, 0x01);
            tp28xx_byte_write(chip, 0x25, 0x08);
            tp28xx_byte_write(chip, 0x26, 0x06);
            tp28xx_byte_write(chip, 0x27, 0x11);
            tp28xx_byte_write(chip, 0x29, 0x0a);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0x06);
            tp28xx_byte_write(chip, 0x14, 0x86);
            tp28xx_byte_write(chip, 0x14, 0x06);
        }else if (output[chip] == MIPI_1CH2LANE_594M) {
            tp28xx_byte_write(chip, 0x20, 0x12);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x15, 0x01);
            tp28xx_byte_write(chip, 0x25, 0x08);
            tp28xx_byte_write(chip, 0x26, 0x06);
            tp28xx_byte_write(chip, 0x27, 0x11);
            tp28xx_byte_write(chip, 0x29, 0x0a);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0x06);
            tp28xx_byte_write(chip, 0x14, 0x86);
            tp28xx_byte_write(chip, 0x14, 0x06);
        }else if (output[chip] == MIPI_2CH2LANE_297M) {
            tp28xx_byte_write(chip, 0x20, 0x22);
            tp28xx_byte_write(chip, 0x34, 0xd4); //output vin1&vin2
            tp28xx_byte_write(chip, 0x14, 0x47);
            tp28xx_byte_write(chip, 0x15, 0x02);
            tp28xx_byte_write(chip, 0x25, 0x04);
            tp28xx_byte_write(chip, 0x26, 0x03);
            tp28xx_byte_write(chip, 0x27, 0x09);
            tp28xx_byte_write(chip, 0x29, 0x02);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0xc7);
            tp28xx_byte_write(chip, 0x14, 0x47);
        }else if (output[chip] == MIPI_1CH2LANE_297M) {
            tp28xx_byte_write(chip, 0x20, 0x12);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x14, 0x47);
            tp28xx_byte_write(chip, 0x15, 0x02);
            tp28xx_byte_write(chip, 0x25, 0x04);
            tp28xx_byte_write(chip, 0x26, 0x03);
            tp28xx_byte_write(chip, 0x27, 0x09);
            tp28xx_byte_write(chip, 0x29, 0x02);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0xc7);
            tp28xx_byte_write(chip, 0x14, 0x47);
        }else if (output[chip] == MIPI_1CH1LANE_594M) {
            tp28xx_byte_write(chip, 0x20, 0x11);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x15, 0x02);
            tp28xx_byte_write(chip, 0x25, 0x08);
            tp28xx_byte_write(chip, 0x26, 0x06);
            tp28xx_byte_write(chip, 0x27, 0x11);
            tp28xx_byte_write(chip, 0x29, 0x0a);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0x06);
            tp28xx_byte_write(chip, 0x14, 0x86);
            tp28xx_byte_write(chip, 0x14, 0x06);
        }else if (output[chip] == MIPI_1CH1LANE_297M) {
            tp28xx_byte_write(chip, 0x20, 0x11);
            tp28xx_byte_write(chip, 0x34, 0xe4);
            tp28xx_byte_write(chip, 0x14, 0x47);
            tp28xx_byte_write(chip, 0x15, 0x03);
            tp28xx_byte_write(chip, 0x25, 0x04);
            tp28xx_byte_write(chip, 0x26, 0x03);
            tp28xx_byte_write(chip, 0x27, 0x09);
            tp28xx_byte_write(chip, 0x29, 0x02);
            tp28xx_byte_write(chip, 0x33, 0x0f);
            tp28xx_byte_write(chip, 0x33, 0x00);
            tp28xx_byte_write(chip, 0x14, 0xc7);
            tp28xx_byte_write(chip, 0x14, 0x47);
        }
}
    tp28xx_byte_write(chip, 0x40, 0x04);

}

///////////////////////////////////////////////////////////////
static void tp2802_comm_init( int chip)
{
    //unsigned int val;

    tp2802_set_reg_page(chip, CH_ALL);

    //if(TP2854 == id[chip] )
    {
        TP2854_reset_default(chip, CH_ALL);

        tp2802_set_video_mode(chip, mode, CH_ALL, STD_TVI);

#if(WDT)
        tp28xx_byte_write(chip, 0x26, 0x04);
#endif

        TP2854_RX_init(chip, PTZ_RX_ACP1);

        TP2854_output(chip);
    }

}


#ifndef NVT_PLATFORM
static struct file_operations tp2802_fops =
{
    .owner      = THIS_MODULE,
    .unlocked_ioctl  = tp2802_ioctl,
    .open       = tp2802_open,
    .release    = tp2802_close
};

static struct miscdevice tp2802_dev =
{
    .minor		= MISC_DYNAMIC_MINOR,
    .name		= "tp2802dev",
    .fops  		= &tp2802_fops,
};


module_param(mode, uint, S_IRUGO);
module_param(chips, uint, S_IRUGO);
module_param_array(output, uint, &chips, S_IRUGO);
/////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////
static int __init tp2802_module_init(void)
{
    int ret = 0, i = 0, val = 0;

    printk("TP2854 driver version %d.%d.%d loaded\n",
           (TP2802_VERSION_CODE >> 16) & 0xff,
           (TP2802_VERSION_CODE >>  8) & 0xff,
           TP2802_VERSION_CODE & 0xff);

   //printk("Compiled %s %s\n", __DATE__, __TIME__);

    if (chips <= 0 || chips > MAX_CHIPS)
    {
        printk("TP2854 module param 'chips' invalid value:%d\n", chips);
        return FAILURE;
    }

    /* register misc device*/
    ret = misc_register(&tp2802_dev);
    if (ret)
    {
        printk("ERROR: could not register tp2854 device module\n");
        return ret;
    }

    i2c_client_init();

    sema_init(&watchdog_lock, 1);


    /* initize each tp2802*/
    for (i = 0; i < chips; i ++)
    {
        //page reset
        tp28xx_byte_write(i, 0x40, 0x00);


        val = tp28xx_byte_read(i, 0xfe);
        if(0x28 == val)
            printk("Detected TP28xx \n");
        else
            printk("Invalid chip %2x\n", val);

        id[i] = tp28xx_byte_read(i, 0xff);
        id[i] <<=8;
        id[i] +=tp28xx_byte_read(i, 0xfd);


        printk("Detected ID&revision %04x\n", id[i]);

        tp2802_comm_init(i);

    }


#if (WDT)
    ret = TP2802_watchdog_init();
    if (ret)
    {
        misc_deregister(&tp2802_dev);
        printk("ERROR: could not create watchdog\n");
        return ret;
    }
#endif

    printk("TP2854 Driver Init Successful!\n");

    return SUCCESS;
}

static void __exit tp2802_module_exit(void)
{
#if (WDT)
    TP2802_watchdog_exit();
#endif

    i2c_client_exit();

    misc_deregister(&tp2802_dev);
}
#endif

void tp283x_egain(unsigned int chip, unsigned int CGAIN_STD)
{
    unsigned int tmp, cgain;
    unsigned int retry = 30;

    tp28xx_byte_write(chip, 0x2f, 0x06);
    cgain = tp28xx_byte_read(chip, 0x04);
#if (DEBUG)
    printk("Cgain=0x%02x \r\n", cgain );
#endif

    if(cgain < CGAIN_STD )
    {
        while(retry)
        {
            retry--;

            tmp = tp28xx_byte_read(chip, 0x07);
            tmp &=0x3f;
            while(abs(CGAIN_STD-cgain))
            {
                if(tmp) tmp--;
                else break;
                cgain++;
            }

            tp28xx_byte_write(chip, 0x07, 0x80|tmp);
            if(0 == tmp) break;
            msleep(40);
            tp28xx_byte_write(chip, 0x2f, 0x06);
            cgain = tp28xx_byte_read(chip, 0x04);
#if (DEBUG)
            printk("Cgain=0x%02x \r\n", cgain );
#endif
            if(cgain > (CGAIN_STD+1))
            {
                tmp = tp28xx_byte_read(chip, 0x07);
                tmp &=0x3f;
                tmp +=0x02;
                if(tmp > 0x3f) tmp = 0x3f;
                tp28xx_byte_write(chip, 0x07, 0x80|tmp);
                if(0x3f == tmp) break;
                msleep(40);
                cgain = tp28xx_byte_read(chip, 0x04);
#if (DEBUG)
                printk("Cgain=0x%02x \r\n", cgain);
#endif
            }
            if(abs(cgain-CGAIN_STD) < 2)  break;
        }

    }
}
/////////////////////////////////////////////////////////////////
unsigned char tp28xx_read_egain(unsigned char chip)
{
    unsigned char gain;

            tp28xx_byte_write(chip, 0x2f, 0x00);
            gain = tp28xx_byte_read(chip, 0x04);
            return gain;

}

//////////////////////////////////////////////////////////////////
/******************************************************************************
 *
 * TP2802_watchdog_deamon()

 *
 ******************************************************************************/
#ifndef NVT_PLATFORM
static int TP2802_watchdog_deamon(void *data)
{
    //unsigned long flags;
    int iChip, i = 0;
    struct sched_param param = { .sched_priority = 99 };

    tp2802wd_info* wdi;

    struct timeval start, end;
    int interval;
    unsigned char status, cvstd, gain, tmp,flag_locked;
    unsigned char rx1,rx2;

    printk("TP2854_watchdog_deamon: start!\n");

    sched_setscheduler(current, SCHED_FIFO, &param);
    current->flags |= PF_NOFREEZE;

    set_current_state(TASK_INTERRUPTIBLE);

    while (watchdog_state != WATCHDOG_EXIT)
    {
        down(&watchdog_lock);

        do_gettimeofday(&start);

        for(iChip = 0; iChip < chips; iChip++)
        {

            wdi = &watchdog_info[iChip];

            for(i=0; i<CHANNELS_PER_CHIP; i++)//scan four inputs:
            {
                if(SCAN_DISABLE == wdi->scan[i]) continue;

                tp2802_set_reg_page(iChip, i);

                status = tp28xx_byte_read(iChip, 0x01);

                //state machine for video checking
                if(status & FLAG_LOSS) //no video
                {

                    if(VIDEO_UNPLUG != wdi->state[i]) //switch to no video
                    {
                        wdi->state[i] = VIDEO_UNPLUG;
                        wdi->count[i] = 0;
                        if(SCAN_MANUAL != wdi->scan[i]) wdi->mode[i] = INVALID_FORMAT;
#if (DEBUG)
                        printk("video loss ch%02x chip%2x\r\n", i, iChip );
#endif
                    }

                    if( 0 == wdi->count[i]) //first time into no video
                    {
                        //if(SCAN_MANUAL != wdi->scan[i]) TP28xx_reset_default(iChip, i);
                        //tp2802_set_video_mode(iChip, DEFAULT_FORMAT, i, STD_TVI);
                        TP28xx_reset_default(iChip, i);
                        wdi->count[i]++;
                    }
                    else
                    {
                        if(wdi->count[i] < MAX_COUNT) wdi->count[i]++;
                        continue;
                    }

                }
                else //there is video
                {
                    if( TP2802_PAL == wdi->mode[i] || TP2802_NTSC == wdi->mode[i] )
                        flag_locked = FLAG_HV_LOCKED;
                    else
                        flag_locked = FLAG_HV_LOCKED;

                    if( flag_locked == (status & flag_locked) ) //video locked
                    {
                        if(VIDEO_LOCKED == wdi->state[i])
                        {
                            if(wdi->count[i] < MAX_COUNT) wdi->count[i]++;
                        }
                        else if(VIDEO_UNPLUG == wdi->state[i])
                        {
                            wdi->state[i] = VIDEO_IN;
                            wdi->count[i] = 0;
#if (DEBUG)
                            printk("1video in ch%02x chip%2x\r\n", i, iChip);
#endif
                        }
                        else if(wdi->mode[i] != INVALID_FORMAT)
                        {
                            //if( FLAG_HV_LOCKED == (FLAG_HV_LOCKED & status) )//H&V locked
                            {
                                wdi->state[i] = VIDEO_LOCKED;
                                wdi->count[i] = 0;
#if (DEBUG)
                                printk("video locked %02x ch%02x chip%2x\r\n", status, i, iChip);
#endif
                            }

                        }
                    }
                    else  //video in but unlocked
                    {
                        if(VIDEO_UNPLUG == wdi->state[i])
                        {
                            wdi->state[i] = VIDEO_IN;
                            wdi->count[i] = 0;
#if (DEBUG)
                            printk("2video in ch%02x chip%2x\r\n", i, iChip);
#endif
                        }
                        else if(VIDEO_LOCKED == wdi->state[i])
                        {
                            wdi->state[i] = VIDEO_UNLOCK;
                            wdi->count[i] = 0;
#if (DEBUG)
                            printk("video unstable ch%02x chip%2x\r\n", i, iChip);
#endif
                        }
                        else
                        {
                            if(wdi->count[i] < MAX_COUNT) wdi->count[i]++;
                            if(VIDEO_UNLOCK == wdi->state[i] && wdi->count[i] > 2)
                            {
                                wdi->state[i] = VIDEO_IN;
                                wdi->count[i] = 0;
                                if(SCAN_MANUAL != wdi->scan[i]) TP28xx_reset_default(iChip, i);
#if (DEBUG)
                                printk("video unlocked ch%02x chip%2x\r\n",i, iChip);
#endif
                            }
                        }
                    }

                    if( wdi->force[i] ) //manual reset for V1/2 switching
                    {

                        wdi->state[i] = VIDEO_UNPLUG;
                        wdi->count[i] = 0;
                        wdi->mode[i] = INVALID_FORMAT;
                        wdi->force[i] = 0;
                        TP28xx_reset_default(iChip, i);
                        //tp2802_set_video_mode(iChip, DEFAULT_FORMAT, i);
                    }

                }

                //printk("video state %2x detected ch%02x count %4x\r\n", wdi->state[i], i, wdi->count[i] );
                if( VIDEO_IN == wdi->state[i] )
                {
                    if(SCAN_MANUAL != wdi->scan[i])
                    {

                        cvstd = tp28xx_byte_read(iChip, 0x03);
#if (DEBUG)
                        printk("video format %2x detected ch%02x chip%2x count%2x\r\n", cvstd, i, iChip, wdi->count[i]);
#endif
                        cvstd &= 0x0f;

                        {

                            wdi-> std[i] = STD_TVI;

                            if( TP2802_SD == (cvstd&0x07) )
                            {

                                    if(wdi->count[i] & 0x01)
                                    {
                                        wdi-> mode[i] = TP2802_PAL;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, STD_TVI);
                                    }
                                    else
                                    {
                                        wdi-> mode[i] = TP2802_NTSC;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, STD_TVI);

                                    }

                            }

                            else if((cvstd&0x07) < 6 )
                            {

                                if(SCAN_HDA == wdi->scan[i] || SCAN_HDC == wdi->scan[i] )
                                {
                                    if(SCAN_HDA == wdi->scan[i])          wdi-> std[i] = STD_HDA;
                                    else if(SCAN_HDC == wdi->scan[i])     wdi-> std[i] = STD_HDC;

                                    if( TP2802_720P25 == (cvstd&0x07) )
                                    {
                                        wdi-> mode[i] = TP2802_720P25V2;
                                    }
                                    else if( TP2802_720P30 == (cvstd&0x07) )
                                    {
                                        wdi-> mode[i] = TP2802_720P30V2;
                                    }
                                    else
                                    {
                                        wdi-> mode[i] = cvstd&0x07;
                                    }
                                    tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);

                                }
                                else //TVI
                                {
                                        if(TP2802_720P25V2 == (cvstd&0x0f) || TP2802_720P30V2 == (cvstd&0x0f) )
                                        {
                                            wdi-> mode[i] = cvstd&0x0f;
                                        }
                                        else
                                        {
                                            wdi-> mode[i] = cvstd&0x07;
                                        }
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                }

                            }
                            else //format is 7
                            {
                                if(SCAN_HDA == wdi->scan[i] ) wdi-> std[i] = STD_HDA;
                                if(SCAN_HDC == wdi->scan[i] ) wdi-> std[i] = STD_HDC;

                                    tp28xx_byte_write(iChip, 0x2f, 0x09);
                                    tmp = tp28xx_byte_read(iChip, 0x04);
#if (DEBUG)
                                    printk("detection %02x  ch%02x chip%2x\r\n", tmp, i,iChip);
#endif
                                    //if((tmp > 0x48) && (tmp < 0x55))
                                    if(0x4e == tmp)
                                    {
                                        //wdi-> mode[i] = TP2802_3M;
                                        if(SCAN_HDA == wdi->scan[i] || SCAN_AUTO == wdi->scan[i])
                                            wdi-> mode[i] = TP2802_QXGA18;
                                        else
                                            wdi-> mode[i] = TP2802_3M18;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    //else if((tmp > 0x55) && (tmp < 0x62))
                                    else if(0x5d == tmp)
                                    {
                                        if(wdi->count[i] & 1)
                                        {
                                            wdi-> mode[i] = TP2802_5M12;
                                            wdi-> std[i] = STD_HDA;
                                            tp2802_set_video_mode(iChip, wdi-> mode[i], i,  wdi-> std[i]);
                                        }
                                        else
                                        {
                                            wdi-> mode[i] = TP2802_4M15;
                                            tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                        }
                                    }
                                    else if(0x5c == tmp)
                                    {
                                        wdi-> mode[i] = TP2802_5M12;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    //else if((tmp > 0x70) && (tmp < 0x80))
                                    else if(0x75 == tmp)
                                    {
                                        wdi-> mode[i] = TP2802_6M10;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    //else if((tmp > 0x34) && (tmp < 0x40))
                                    else if(0x38 == tmp)
                                    {
                                        wdi-> mode[i] = TP2802_QXGA25; //current only HDA
                                        wdi-> std[i] = STD_HDA;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    //else if((tmp > 0x28) && (tmp < 0x34))
                                    else if(0x2e == tmp)
                                    {
                                        wdi-> mode[i] = TP2802_QXGA30; //current only HDA
                                        wdi-> std[i] = STD_HDA;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    else if(0x3a == tmp)  // invalid for TP2853C
                                    {
                                        if(TP2802_5M20 != wdi-> mode[i])
                                        {
                                            wdi-> mode[i] = TP2802_5M20;
                                            tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                        }
                                    }
                                    else if(0x89 == tmp)
                                    {
                                        wdi-> mode[i] = TP2802_1080P15;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    else if(0x22 == tmp)
                                    {
                                        wdi-> mode[i] = TP2802_1080P60;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }

                            }

                                    if( (wdi->count[i] & 1) && (wdi-> mode[i] == TP2802_1080P25))
                                    {
                                        wdi-> mode[i] = TP2802_8M12;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    if( (wdi->count[i] & 1) && (wdi-> mode[i] == TP2802_1080P30))
                                    {
                                        wdi-> mode[i] = TP2802_8M15;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    if( (wdi->count[i] & 1) && (wdi-> mode[i] == TP2802_720P30))
                                    {
                                        wdi-> mode[i] = TP2802_4M12;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    if( (wdi->count[i] & 1) && (wdi-> mode[i] == TP2802_720P60))
                                    {
                                        wdi-> mode[i] = TP2802_QHD30;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                    if( (wdi->count[i] & 1) && (wdi-> mode[i] == TP2802_720P50))
                                    {
                                        wdi-> mode[i] = TP2802_QHD25;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }

                                    if( (wdi->count[i] & 1) && (wdi-> mode[i] == TP2802_720P30V2))
                                    {
                                        wdi-> mode[i] = TP2802_QHD15;
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i,  wdi-> std[i]);
                                    }

                                    if( (wdi->count[i] > 2) && (wdi-> mode[i] == TP2802_QXGA18) )
                                    {

                                        if(SCAN_AUTO == wdi->scan[i])
                                        {
                                            wdi-> mode[i] = TP2802_3M18;
                                            tp2802_set_video_mode(iChip, wdi-> mode[i], i,  wdi-> std[i]);
                                        }
                                    }



                        }

                    }

                }

#define EQ_COUNT 10
                if( VIDEO_LOCKED == wdi->state[i]) //check signal lock
                {
                    if(0 == wdi->count[i] )
                    {
                        //if(SCAN_MANUAL == wdi->scan[i])
                        {

                            	tmp = tp28xx_byte_read(iChip, 0x26);
                            	tmp |= 0x01;
                            	tp28xx_byte_write(iChip, 0x26, tmp);
                        }

                        if( (SCAN_AUTO == wdi->scan[i] || SCAN_TVI == wdi->scan[i] ))
                        {

                            if( (TP2802_720P30V2==wdi-> mode[i]) || (TP2802_720P25V2==wdi-> mode[i]) )
                            {
                                tmp = tp28xx_byte_read(iChip, 0x03);
#if (DEBUG)
                                printk("CVSTD%02x  ch%02x chip%2x\r\n", tmp, i,iChip);
#endif
                                if( ! (0x08 & tmp) )
                                {
#if (DEBUG)
                                    printk("720P V1 Detected ch%02x chip%2x\r\n",i,iChip);
#endif
                                    wdi-> mode[i] &= 0xf7;
                                    tp2802_set_video_mode(iChip, wdi-> mode[i], i, STD_TVI); //to speed the switching

                                }
                            }
                            else if( (TP2802_720P30==wdi-> mode[i]) || (TP2802_720P25==wdi-> mode[i]) )
                            {
                                tmp = tp28xx_byte_read(iChip, 0x03);
#if (DEBUG)
                                printk("CVSTD%02x  ch%02x chip%2x\r\n", tmp, i,iChip);
#endif
                                if( 0x08 & tmp)
                                {
#if (DEBUG)
                                    printk("720P V2 Detected ch%02x chip%2x\r\n",i,iChip);
#endif
                                    wdi-> mode[i] |= 0x08;
                                    tp2802_set_video_mode(iChip, wdi-> mode[i], i, STD_TVI); //to speed the switching

                                }

                            }

                            //these code need to keep bottom
                            //if( id[iChip] >= TP2822 )
                            {
                                    tmp = tp28xx_byte_read(iChip, 0xa7);
                                    tmp &= 0xfe;
                                    tp28xx_byte_write(iChip, 0xa7, tmp);
                                    //tp28xx_byte_write(iChip, 0x2f, 0x0f);
                                    tp28xx_byte_write(iChip, 0x1f, 0x06);
                                    tp28xx_byte_write(iChip, 0x1e, 0x60);

                            }

                        }

                    }
                    else if(1 == wdi->count[i])
                    {

                                    tmp = tp28xx_byte_read(iChip, 0xa7);
                                    tmp |= 0x01;
                                    tp28xx_byte_write(iChip, 0xa7, tmp);

#if (DEBUG)
                        tmp = tp28xx_byte_read(iChip, 0x01);
                        printk("status%02x  ch%02x\r\n", tmp, i);
                        tmp = tp28xx_byte_read(iChip, 0x03);
                        printk("CVSTD%02x  ch%02x\r\n", tmp, i);
#endif
                    }
                    else if( wdi->count[i] < EQ_COUNT-3)
                    {

                        if( SCAN_AUTO == wdi->scan[i] )
                        {

                            if( STD_TVI == wdi-> std[i])
                            {
                                tmp = tp28xx_byte_read(iChip, 0x01);

                                if((TP2802_PAL == wdi-> mode[i]) || (TP2802_NTSC == wdi-> mode[i]))
                                {
                                    //nothing to do
                                }
                                else if(TP2802_QXGA18 == wdi-> mode[i])
                                {
                                    if(0x60 == (tmp&0x64) )
                                    {
                                        wdi-> std[i] = STD_HDA; //no CVI QXGA18
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                }
                                else if(TP2802_QHD15 == wdi-> mode[i] || TP2802_5M20 == wdi-> mode[i] || TP2802_5M12 == wdi-> mode[i] || TP2802_8M15 == wdi-> mode[i])
                                {
                                    if(0x60 == (tmp&0x64) )
                                    {
                                        wdi-> std[i] = STD_HDA; //no CVI QHD15/5M20/5M12.5
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                    }
                                }
                                else if(TP2802_QHD25 == wdi-> mode[i] || TP2802_QHD30 == wdi-> mode[i])
                                {
                                    tp28xx_byte_write(iChip, 0x2f, 0x0f);
                                    rx1 = tp28xx_byte_read(iChip, 0x04);

                                    if(rx1 > 0x30)               wdi-> std[i] = STD_HDA;
                                    else if(0x60 == (tmp&0x64) ) wdi-> std[i] = STD_HDC;
                                    tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                }
                                else if(0x60 == (tmp&0x64) )
                                {

                                    rx2 = tp28xx_byte_read(iChip, 0x94); //capture line7 to match 3M/4M RT

                                    if(HDC_enable)
                                    {
                                        if     (0xff == rx2)                wdi-> std[i] = STD_HDC;
                                        else if(0x00 == rx2)                wdi-> std[i] = STD_HDC_DEFAULT;
                                        else                                wdi-> std[i] = STD_HDA;
                                    }
                                    else
                                    {
                                        wdi-> std[i] = STD_HDA;
                                    }

                                    if(STD_TVI != wdi->std[i])
                                    {
                                        tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
    #if (DEBUG)
                                        printk("RX=%02x standard to %02x  ch%02x\r\n", rx2, wdi-> std[i], i);
    #endif
                                    }

                                }
                            }

                        }

                    }
                    else if( wdi->count[i] < EQ_COUNT) //skip
                    {

                        //wdi->gain[i][EQ_COUNT-wdi->count[i]] = tp28xx_read_egain(iChip);
                        wdi->gain[i][3] = wdi->gain[i][2];
                        wdi->gain[i][2] = wdi->gain[i][1];
                        wdi->gain[i][1] = wdi->gain[i][0];

                        wdi->gain[i][0] = tp28xx_read_egain(iChip);

                    }
                    else if( wdi->count[i] < EQ_COUNT+EQ_COUNT ) //add timeout handle
                    {
                        wdi->gain[i][3] = wdi->gain[i][2];
                        wdi->gain[i][2] = wdi->gain[i][1];
                        wdi->gain[i][1] = wdi->gain[i][0];

                        wdi->gain[i][0] = tp28xx_read_egain(iChip);

                        //if(abs(wdi->gain[i][3] - wdi->gain[i][0])< 0x20 && abs(wdi->gain[i][2] - wdi->gain[i][0])< 0x20 && abs(wdi->gain[i][1] - wdi->gain[i][0])< 0x20 )
                        if(abs(wdi->gain[i][3] - wdi->gain[i][0])< 0x02 && abs(wdi->gain[i][2] - wdi->gain[i][0])< 0x02 && abs(wdi->gain[i][1] - wdi->gain[i][0])< 0x02 )
                        {
                            wdi->count[i] = EQ_COUNT+EQ_COUNT-1; // exit when EQ stable
                        }
                    }
                    else if( wdi->count[i] == EQ_COUNT+EQ_COUNT )
                    {
                        gain = tp28xx_read_egain(iChip);

                        if( STD_TVI != wdi-> std[i] )
                        {

                                tp28xx_byte_write(iChip, 0x07, 0x80|(gain>>2));  // manual mode

                        }
                        else //TVI
                        {

                        }

                    }
                    else if(wdi->count[i] == EQ_COUNT+EQ_COUNT+1)
                    {

                            if(  SCAN_AUTO == wdi->scan[i])
                            {

                                if( HDC_enable )
                                {
                                        if(STD_HDC_DEFAULT == wdi->std[i] )
                                        {
                                            tp28xx_byte_write(iChip, 0x2f,0x0c);
                                            tmp = tp28xx_byte_read(iChip, 0x04);
                                            status = tp28xx_byte_read(iChip, 0x01);

                                            //if(0x10 == (0x11 & status) && (tmp < 0x18 || tmp > 0xf0))
                                            if(0x10 == (0x11 & status))
                                            //if((tmp < 0x18 || tmp > 0xf0))
                                            {
                                                wdi-> std[i] = STD_HDC;
                                            }
                                            else
                                            {
                                                wdi-> std[i] = STD_HDA;
                                            }
                                            tp2802_set_video_mode(iChip, wdi-> mode[i], i, wdi-> std[i]);
                                            #if (DEBUG)
                                            printk("reg01=%02x reg04@2f=0c %02x std%02x ch%02x\r\n", status, tmp, wdi-> std[i], i );
                                            #endif
                                        }
                                }
                            }

                            if( STD_TVI != wdi-> std[i] )
                            {
                                    tp283x_egain(iChip, 0x0c);
                            }
                            else //TVI
                            {

                            }



                    }
                    else
                    {
                        if( SCAN_AUTO == wdi->scan[i])
                        {

                            if((wdi-> mode[i] & (~FLAG_HALF_MODE)) < TP2802_3M18)
                            {
                                            tmp = tp28xx_byte_read(iChip, 0x03); //
                                            tmp &= 0x07;
                                            if(tmp != (wdi-> mode[i]&0x07) && tmp < TP2802_SD)
                                            {
                                            #if (DEBUG)
                                            printk("correct %02x from %02x ch%02x\r\n", tmp, wdi-> mode[i], i );
                                            #endif

                                                wdi->force[i] = 1;
                                            }
                            }
                        }


                    }

                }
            }
        }

        do_gettimeofday(&end);

        interval = 1000000*(end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec);

        //printk("WDT elapsed time %d.%dms\n", interval/1000, interval%1000);
        up(&watchdog_lock);

        /* sleep 0.5 seconds */
        schedule_timeout_interruptible(msecs_to_jiffies(500)+1);
    }

    set_current_state(TASK_RUNNING);

    printk("TP2854_watchdog_deamon: exit!\n");

    return 0;

}


/******************************************************************************
 *
 * cx25930_watchdog_init()

 *
 ******************************************************************************/
int __init TP2802_watchdog_init(void)
{
    struct task_struct *p_dog;
    int i, j;
    watchdog_state = WATCHDOG_RUNNING;
    memset(&watchdog_info, 0, sizeof(watchdog_info));

    for(i=0; i<MAX_CHIPS; i++)
    {
        watchdog_info[i].addr = tp2802_i2c_addr[i];
        for(j=0; j<CHANNELS_PER_CHIP; j++)
        {
            watchdog_info[i].count[j] = 0;
            watchdog_info[i].force[j] = 0;
            //watchdog_info[i].loss[j] = 1;
            watchdog_info[i].mode[j] = INVALID_FORMAT;
            watchdog_info[i].scan[j] = SCAN_AUTO;
            watchdog_info[i].state[j] = VIDEO_UNPLUG;
            watchdog_info[i].std[j] = STD_TVI;
        }

    }

    p_dog = kthread_create(TP2802_watchdog_deamon, NULL, "TP2854_WDT");

    if ( IS_ERR(p_dog))
    {
        printk("TP2854_watchdog_init: create watchdog_deamon failed!\n");
        return -1;
    }

    wake_up_process(p_dog);

    task_watchdog_deamon = p_dog;

    printk("TP2854_watchdog_init: done!\n");

    return 0;
}

/******************************************************************************
 *
 * cx25930_watchdog_exit()

 *
 ******************************************************************************/
void __exit TP2802_watchdog_exit(void)
{

    struct task_struct *p_dog = task_watchdog_deamon;
    watchdog_state = WATCHDOG_EXIT;

    if ( p_dog == NULL )
        return;

    wake_up_process(p_dog);

    kthread_stop(p_dog);

    yield();

    task_watchdog_deamon = NULL;

    printk("TP2854_watchdog_exit: done!\n");
}
module_init(tp2802_module_init);
module_exit(tp2802_module_exit);
#endif

