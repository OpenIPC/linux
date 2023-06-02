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
#include <linux/uaccess.h>
#include <asm/io.h>
#include <asm/system.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/list.h>
#include <asm/delay.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <asm/bitops.h>
#include <asm/irq.h>
#include <linux/moduleparam.h>
#include <linux/ioport.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#endif

#include "video.h"
#include "common.h"
#include "coax_protocol.h"

extern unsigned int nvp6124_slave_addr[4];
extern unsigned char ch_mode_status[16];
extern unsigned int nvp6124_cnt;
extern unsigned int vloss;
extern void nvp6124_sw_reset(unsigned char ch);

//#define _6124B_EQ_ADJ_COLOR_
static volatile unsigned char stage_update[16];

static unsigned char ANALOG_EQ_1080P[8]		= {0x13,0x03,0x53,0x73,0x73,0x73,0x73,0x73};
static unsigned char DIGITAL_EQ_1080P[8]	= {0x00,0x00,0x00,0x00,0x8B,0x8F,0x8F,0x8F};
#ifdef _6124B_EQ_ADJ_COLOR_
static unsigned char BRI_EQ_1080P[8]		= {0xF4,0xF4,0xF4,0xF4,0xF8,0xF8,0xF8,0xF8};
static unsigned char CON_EQ_1080P[8]		= {0x90,0x90,0x90,0x90,0x90,0x90,0x80,0x80};
static unsigned char SAT_EQ_1080P[8]		= {0x80,0x80,0x80,0x78,0x78,0x78,0x78,0x78};
static unsigned char BRI_EQ_720P[9]			= {0xF4,0xF4,0xF4,0xF4,0xF8,0xF8,0xF8,0xF8,0xF8};
static unsigned char CON_EQ_720P[9]			= {0x90,0x90,0x90,0x90,0x88,0x88,0x84,0x90,0x90};
static unsigned char SAT_EQ_720P[9]			= {0x84,0x84,0x84,0x80,0x80,0x80,0x80,0x84,0x84};
#endif
#ifndef NVT_PLATFORM
static unsigned char SHARP_EQ_1080P[8]		= {0x90,0x90,0x99,0x99,0x99,0x99,0x99,0x90};
static unsigned char PEAK_EQ_1080P[8]		= {0x00,0x10,0x00,0x00,0x00,0x00,0x50,0x00};
static unsigned char CTI_EQ_1080P[8]		= {0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A};
static unsigned char C_LOCK_EQ_1080P[8]		= {0x92,0x92,0x92,0x92,0x92,0xA2,0xA2,0xA2};
static unsigned char UGAIN_EQ_1080P[8]		= {0x00,0x00,0x00,0x00,0x10,0x10,0x20,0x00};
static unsigned char VGAIN_EQ_1080P[8]		= {0x00,0x00,0x00,0x00,0x10,0x10,0x20,0x00};

static unsigned char SHARP_EQ_720P[9]		= {0x90,0x90,0x99,0x99,0x99,0x99,0x99,0x90,0x90};
static unsigned char PEAK_EQ_720P[9]		= {0x00,0x20,0x10,0x10,0x00,0x00,0x40,0x20,0x20};
#endif
static unsigned char CKILL_EQ_720P[9]		= {0x0B,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A};
#ifndef NVT_PLATFORM
static unsigned char CTI_EQ_720P[9]			= {0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A,0x0A};
static unsigned char C_LOCK_EQ_720P[9]		= {0x92,0x92,0x92,0x92,0x92,0x92,0xA2,0x92,0xA2};
static unsigned char UGAIN_EQ_720P[9]		= {0x30,0x30,0x30,0x30,0x30,0x30,0x40,0x30,0x30};
static unsigned char VGAIN_EQ_720P[9]		= {0x30,0x30,0x30,0x30,0x30,0x30,0x40,0x30,0x30};
#endif
static unsigned char ANALOG_EQ_720P[9]		= {0x13,0x03,0x13,0x23,0x43,0x53,0x73,0x03,0x13};
static unsigned char DIGITAL_EQ_720P[9]		= {0x00,0x00,0x00,0x00,0x88,0x8F,0x8F,0x00,0x00};

static unsigned char eq_stage[16]			= {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static void nvp6124b_ckill_eq(unsigned int ch,  unsigned int stage)
{
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
    if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_HDEX)
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x22+(ch%4)*4),CKILL_EQ_720P[stage]);
}

static void set_color_burst_threshold(unsigned char ch, unsigned char eq_stage, unsigned char flag)
{
	if(flag == 1)
	{
		if(ch_mode_status[ch] == NVP6124_VI_720P_2530  || ch_mode_status[ch] == NVP6124_VI_HDEX)
		{
		}
		else
		{
			if(eq_stage==6||eq_stage==5)
			{
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0B);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x05+1*(ch%4));
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x27, 0x30);
			}
		}
	}
	else
	{
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x00);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0A);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x05+1*(ch%4));
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x27, 0x57);

	}

}

static void set_analog_stage(unsigned char ch, unsigned char stage)
{
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x05+ch%4);
	if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
	{
		printk("ANALOG EQ SETUP\n");
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x01, 0x0C);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58, ANALOG_EQ_1080P[stage]);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x59, 0x01);
	}
	else
	{
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x01, 0x0E);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58, ANALOG_EQ_720P[stage]);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x59, 0x00);
	}
}

static void set_iir_stage(unsigned char ch, unsigned char stage)
{
	//if(ch == 0)
	//	printk("stage = %x, mode = %x\n",stage, ch_mode_status[ch]);
	if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
	{
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x09);
		switch(stage)
		{
			case 0 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
			break;
			case 1 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
			break;
			case 2 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA0);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x84+0x20*(ch%4),0x01);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x86+0x20*(ch%4),0x23);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x87+0x20*(ch%4),0xC3);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+0x20*(ch%4),0xF6);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+0x20*(ch%4),0x34);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8A+0x20*(ch%4),0xCD);
			break;
			case 3 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA2);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
			break;
			case 4 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA3);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
			break;
			case 5 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA4);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
			break;
			case 6 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA5);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0xA0);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+0x20*(ch%4),0x01);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8d+0x20*(ch%4),0x28);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8E + 0x20*(ch%4),0x0A);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8F+0x20*(ch%4),0xCF);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x90+0x20*(ch%4),0xF8);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x91+0x20*(ch%4),0x0F);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x92+0x20*(ch%4),0xD7);
			break;
			case 7 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x82+0x20*(ch%4),0x00);
			break;
		}
	}
	else if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_HDEX)
	{
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x09);
		switch(stage)
		{
			case 0 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
			break;
			case 1 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0x00);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
			break;
			case 2 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA1);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
			break;
			case 3 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA2);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0x00);
			break;
			case 4 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA2);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0xA2);
			break;
			case 5 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA2);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0xA2);
			break;
			case 6 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA3);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0xA1);
			break;
			case 7 :
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x80+0x20*(ch%4),0xA3);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+0x20*(ch%4),0xA1);
			break;
		}
	}
	else
	{
	}
}

static unsigned char get_sarv0_stage(unsigned char resol, unsigned int sarv0)
{
	unsigned char sarv0_stg = 0;

	if(resol == NVP6124_VI_1080P_2530)
	{
		if	   (sarv0 <= 0x0F) sarv0_stg = 1;
		else if(sarv0 <= 0x15) sarv0_stg = 2;
		else if(sarv0 <= 0x1D) sarv0_stg = 3;
		else if(sarv0 <= 0x24) sarv0_stg = 4;
		else if(sarv0 <= 0x2B) sarv0_stg = 5;
		else				   sarv0_stg = 6;
	}
	else if((resol == NVP6124_VI_720P_2530) || (resol == NVP6124_VI_HDEX))
	{
		if	   (sarv0 <= 0x0E) sarv0_stg = 1;
		else if(sarv0 <= 0x0F) sarv0_stg = 2;
		else if(sarv0 <= 0x11) sarv0_stg = 3;
		else if(sarv0 <= 0x14) sarv0_stg = 4;
		else if(sarv0 <= 0x18) sarv0_stg = 5;
		else				  sarv0_stg = 6;
	}

	return sarv0_stg;
}

static unsigned char get_sarv3_stage(unsigned int resol, unsigned int sarv3)
{
	unsigned char sarv3_stg = 0;

	if(resol == NVP6124_VI_1080P_2530)
	{
		if	   (sarv3 <= 0x3EC) sarv3_stg = 1;
		else if(sarv3 <= 0x55F) sarv3_stg = 2;
		else if(sarv3 <= 0x73F) sarv3_stg = 3;
		else if(sarv3 <= 0x92C) sarv3_stg = 4;
		else if(sarv3 <= 0xACD) sarv3_stg = 5;
		else					sarv3_stg = 6;
	}
	else if((resol == NVP6124_VI_720P_2530) || (resol == NVP6124_VI_HDEX))
	{
		if	   (sarv3 <= 0x396) sarv3_stg = 1;
		else if(sarv3 <= 0x3EE) sarv3_stg = 2;
		else if(sarv3 <= 0x478) sarv3_stg = 3;
		else if(sarv3 <= 0x537) sarv3_stg = 4;
		else if(sarv3 <= 0x614) sarv3_stg = 5;
		else				    sarv3_stg = 6;
	}

	return sarv3_stg;
}

static unsigned int get_yeq_stage(unsigned char resol, unsigned int y_minus_slp)
{
	unsigned char y_eq = 0;

	if(resol == NVP6124_VI_1080P_2530)
	{
		if     (y_minus_slp >= 0x1D9)    y_eq = 1;
		else if(y_minus_slp >= 0x1AD)    y_eq = 2;
		else if(y_minus_slp >= 0x18C)    y_eq = 3;
		else if(y_minus_slp >= 0x16F)    y_eq = 4;
		else if(y_minus_slp >= 0x155)    y_eq = 5;
		else							 y_eq = 6;
	}
	else if((resol == NVP6124_VI_720P_2530) || (resol == NVP6124_VI_HDEX))
	{
		if     (y_minus_slp >= 0x18B)    y_eq = 1;
		else if(y_minus_slp >= 0x176)    y_eq = 2;
		else if(y_minus_slp >= 0x164)    y_eq = 3;
		else if(y_minus_slp >= 0x151)    y_eq = 4;
		else if(y_minus_slp >= 0x142)    y_eq = 5;
		else							 y_eq = 6;
	}

	return y_eq;

}

#define ACP_CLR_CNT			1
#define ACP_SET_CNT			(ACP_CLR_CNT+1)
#define ACP_READ_START_CNT	(ACP_SET_CNT)
#define RETRY_CNT			3
#define COMPARE_NUM			3
#define COUNT_NUM			4
#define LOOP_BUF_SIZE		4

static unsigned char check_sarv0_stage[16][LOOP_BUF_SIZE];
static unsigned char check_sarv3_stage[16][LOOP_BUF_SIZE];
static unsigned char check_y_stage[16][LOOP_BUF_SIZE];
static unsigned char acp_val[16][LOOP_BUF_SIZE];
static unsigned char acp_ptn[16][LOOP_BUF_SIZE];
static unsigned char pre_vidmode[16];
static unsigned char video_on[16];
static unsigned char eq_loop_cnt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned char loop_cnt[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned char bypass_flag_retry[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned char ystage_flag_retry[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#ifndef NVT_PLATFORM
static unsigned char sarv0_flag_retry[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
#endif
static unsigned char one_setting[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned char bypass_flag[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
static unsigned char acp_isp_wr_en[16]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

static void lossfakechannel(unsigned char ch, unsigned char bypassflag)
{
	unsigned char tmp;

	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	tmp = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x7A+((ch%4)<2?0x00:0x01));
	if(ch%2 == 0)
		tmp = tmp & 0xF0;
	else
		tmp = tmp & 0x0F;

	if(bypassflag)
		tmp = tmp | (0x0F << (ch%2)*4);
	else
		tmp = tmp | (0x01 << (ch%2)*4);

	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x7A+((ch%4)<2?0x00:0x01),tmp);

}

static void bubble_sort(unsigned char *buf ,unsigned char len)
{
	int i,j,tmp;

	for(i = 0; i < len; i++)
	{
		for(j = 0; j < len - 1 ; j++)
		{
			if(buf[j] > buf[j+1])
			{
				tmp = buf[j];
				buf[j] = buf[j+1];
				buf[j+1] = tmp;
			}
		}
	}
}

#ifndef NVT_PLATFORM
static unsigned char calc_stage_gap(unsigned char *buf)
{
	unsigned char cnt,zero_cnt=0;
	unsigned char tmpbuf[LOOP_BUF_SIZE];

	memcpy(tmpbuf,buf,LOOP_BUF_SIZE);

	bubble_sort(tmpbuf, LOOP_BUF_SIZE);

	for(cnt = 0; cnt < LOOP_BUF_SIZE; cnt++)
	{
		if(tmpbuf[cnt] == 0) zero_cnt++;
	}

	return(abs(tmpbuf[LOOP_BUF_SIZE-1] - tmpbuf[zero_cnt]));
}
#endif

static unsigned char calc_arr_mean(unsigned char *buf)
{
	unsigned char cnt,tmp=0,zero_cnt=0;
	unsigned char tmpbuf[LOOP_BUF_SIZE];

	memcpy(tmpbuf,buf,LOOP_BUF_SIZE);

	bubble_sort(tmpbuf, LOOP_BUF_SIZE);

	for(cnt = 0; cnt < LOOP_BUF_SIZE; cnt++)
	{
		if(tmpbuf[cnt] == 0) zero_cnt++;

		tmp = tmp + tmpbuf[cnt];
	}
	tmp = abs( tmp / (LOOP_BUF_SIZE-zero_cnt));

	for(cnt = 0; cnt <LOOP_BUF_SIZE; cnt++)
		printk("cnt[%d]  origin = %x   sort= %x\n",cnt, buf[cnt], tmpbuf[cnt]);
	printk("mean value ========================== %d\n",tmp);


	return tmp;
}

static int check_stage(unsigned char ch, unsigned char ystage, unsigned char sarv0, unsigned char sarv3)
{
	if((ystage == 0xFF) || (sarv0 == 0xFF) || (sarv3== 0xFF))
		return -1;
	else
	{
		if(abs(ystage-sarv0) > 1)
			return -1;
		if(abs(ystage-sarv3) > 1)
			return -1;
	}

	return 1;
}

static unsigned char compare_arr(unsigned char cur_ptr, unsigned char *buf)
{
	unsigned char start_ptr;
	unsigned char tmp;
	if(cur_ptr == 1)		start_ptr = LOOP_BUF_SIZE - 1;
	else if(cur_ptr == 0)	start_ptr = LOOP_BUF_SIZE - 2;
	else					start_ptr = cur_ptr - 2;

	if((buf[start_ptr%LOOP_BUF_SIZE] == buf[(start_ptr+1)%LOOP_BUF_SIZE])						&&
       (buf[(start_ptr+1)%LOOP_BUF_SIZE] == buf[(start_ptr+2)%LOOP_BUF_SIZE]))
		return buf[start_ptr];
	else
	{
		if(buf[start_ptr%LOOP_BUF_SIZE] >= buf[(start_ptr+1)%LOOP_BUF_SIZE])
			tmp = buf[start_ptr%LOOP_BUF_SIZE];
		else
			tmp = buf[(start_ptr+1)%LOOP_BUF_SIZE];
		if(tmp < buf[(start_ptr+2)%LOOP_BUF_SIZE])
			tmp = buf[(start_ptr+2)%LOOP_BUF_SIZE];
		return tmp;
	}
}



static void init_eq_stage(unsigned char ch)
{
	unsigned char default_stage=0;

	if((ch_mode_status[ch] == NVP6124_VI_720P_2530) || (ch_mode_status[ch] == NVP6124_VI_HDEX))
	{
		printk("720P ch %d init_eq_stage\n", ch);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x05+ch%4);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58, ANALOG_EQ_720P[default_stage]);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,((ch%4)<2)?0x0A:0x0B);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (ch%2==0)?0x3B:0xBB, DIGITAL_EQ_720P[default_stage]);
	}
	else
	{
		printk("1080P ch %d init_eq_stage\n", ch);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x05+ch%4);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58, ANALOG_EQ_1080P[default_stage]);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,((ch%4)<2)?0x0A:0x0B);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (ch%2==0)?0x3B:0xBB, DIGITAL_EQ_1080P[default_stage]);
	}

	memset(check_y_stage[ch],0x00,sizeof(check_y_stage[ch]));
	memset(check_sarv0_stage[ch],0x00,sizeof(check_sarv0_stage[ch]));
	memset(check_sarv3_stage[ch],0x00,sizeof(check_sarv3_stage[ch]));
}

static volatile unsigned int min_sarv0[4]={0xFFFF,};
static volatile unsigned int max_sarv0[4]={0x0000,};
static volatile unsigned int min_sarv3[4]={0xFFFF,};
static volatile unsigned int max_sarv3[4]={0x0000,};
static volatile unsigned int min_ystage[4]={0xFFFF,};
static volatile unsigned int max_ystage[4]={0x0000,};

//void nvp6124b_set_equalizer(nvp6124_input_videofmt *pvideoeq)
void nvp6124b_set_equalizer(unsigned char ch)
{
	unsigned char i,tmp_acp_val,tmp_acp_ptn;
	unsigned char last_ystage, last_sarv0, last_sarv3;
	//unsigned char ch;
	unsigned char vidmode[16];
	unsigned char agc_lock;
	unsigned int  y_ref_status[16];
	unsigned int  sarv_0[16];
	unsigned int  sarv_3[16];

	//for(ch=0; ch<nvp6124_cnt*4; ch++)
	{
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
		vidmode[ch] = gpio_i2c_read(nvp6124_slave_addr[ch/4],0xF0);

		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
		agc_lock = gpio_i2c_read(nvp6124_slave_addr[ch/4],0xEC);

		if(((((vloss>>(ch)))&0x01) == 0x00) && (((agc_lock>>(ch%4))&0x01) == 0x01))
		{
			if(vidmode[ch] <= 0x34)
				video_on[ch] = 1;
			else
			{
				vidmode[ch]  = 0xFF;
				video_on[ch] = 0;
			}
		}
		else
		{
			vidmode[ch]  = 0xFF;
			video_on[ch] = 0;
		}

		if(1)
		{
			if(video_on[ch])
			{
				if(loop_cnt[ch] != 0xFF)
				{
					loop_cnt[ch]++;
					if(loop_cnt[ch] > 200) loop_cnt[ch] = ACP_READ_START_CNT;
					if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
					{
						if(loop_cnt[ch] == ACP_CLR_CNT)	acp_reg_rx_clear(ch);
						else							acp_each_setting(ch);
					}

					eq_loop_cnt[ch]++;
					eq_loop_cnt[ch] %= LOOP_BUF_SIZE;

					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					y_ref_status[ch] = gpio_i2c_read(nvp6124_slave_addr[ch/4],0xEA)&0x1F;
					y_ref_status[ch] <<= 8;
					y_ref_status[ch] |= gpio_i2c_read(nvp6124_slave_addr[ch/4],0xEB);


					sarv_0[ch] = gpio_i2c_read(nvp6124_slave_addr[ch/4],0xF8);
					sarv_3[ch] = gpio_i2c_read(nvp6124_slave_addr[ch/4],0xFC);
					sarv_3[ch] <<= 8;
					sarv_3[ch] |= gpio_i2c_read(nvp6124_slave_addr[ch/4],0xFB);

					sarv_3[ch] = (sarv_3[ch]>sarv_0[ch]*65) ? sarv_0[ch]*65 :
						         (sarv_3[ch]<sarv_0[ch]*63) ? sarv_0[ch]*63 : sarv_3[ch]; // 16 bit i2c read error compensation

					check_y_stage[ch][eq_loop_cnt[ch]]		= get_yeq_stage(ch_mode_status[ch], y_ref_status[ch]);
					check_sarv0_stage[ch][eq_loop_cnt[ch]] 	= get_sarv0_stage(ch_mode_status[ch],sarv_0[ch]);
					check_sarv3_stage[ch][eq_loop_cnt[ch]]  = get_sarv3_stage(ch_mode_status[ch],sarv_3[ch]);
					acp_val[ch][eq_loop_cnt[ch]]			= read_acp_status(ch);
					acp_ptn[ch][eq_loop_cnt[ch]]			= read_acp_pattern(ch);
					//if(ch == 0)
					//{
					if(sarv_0[ch] < min_sarv0[ch])				min_sarv0[ch] = sarv_0[ch];
					else if(sarv_0[ch] > max_sarv0[ch])			max_sarv0[ch] = sarv_0[ch];

					if(sarv_3[ch] < min_sarv3[ch])				min_sarv3[ch] = sarv_3[ch];
					else if(sarv_3[ch] > max_sarv3[ch])			max_sarv3[ch] = sarv_3[ch];

					if(y_ref_status[ch] < min_ystage[ch]) 		min_ystage[ch] = y_ref_status[ch];
					else if(y_ref_status[ch] > max_ystage[ch])	max_ystage[ch] = y_ref_status[ch];

					printk("--------------------------------------\n");
					printk("ystage=%x cur=%03x min=%03x max=%03x\n",check_y_stage[ch][eq_loop_cnt[ch]], y_ref_status[ch], min_ystage[ch], max_ystage[ch]);
					printk("sarv0 =%x cur=%03x min=%03x max=%03x\n",check_sarv0_stage[ch][eq_loop_cnt[ch]], sarv_0[ch],min_sarv0[ch], max_sarv0[ch]);
					printk("sarv3 =%x cur=%03x min=%03x max=%03x\n",check_sarv3_stage[ch][eq_loop_cnt[ch]], sarv_3[ch],min_sarv3[ch], max_sarv3[ch]);
				}

				if((loop_cnt[ch] != 0xFF) && (loop_cnt[ch] >= ACP_READ_START_CNT))
				{
					if(one_setting[ch] == 0)
					{
						bypass_flag[ch] = 0xFF;
						if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
						{
							tmp_acp_val = compare_arr(eq_loop_cnt[ch],acp_val[ch]);
							tmp_acp_ptn = compare_arr(eq_loop_cnt[ch],acp_ptn[ch]);
						}
						else
						{
							tmp_acp_val = 0x55;
							tmp_acp_ptn = 0x01;
						}
						//if(ch == 0)
						{
							printk("acp_val = %x   acp_ptn = %x\n", acp_val[ch][eq_loop_cnt[ch]], acp_ptn[ch][eq_loop_cnt[ch]]);
							printk("tmp_acp_val = %x   tmp_acp_ptn = %x\n", tmp_acp_val, tmp_acp_ptn);
						}

						if((tmp_acp_val == 0x55) && (tmp_acp_ptn == 0x01))
						{
							bypass_flag[ch] = 0;
							one_setting[ch] = 1;
						}
						else
						{
							bypass_flag_retry[ch]++;
							bypass_flag_retry[ch] %= 200;
							if(bypass_flag_retry[ch] == RETRY_CNT)
							{
								bypass_flag[ch] = 1;
								one_setting[ch] = 1;
								lossfakechannel(ch, bypass_flag[ch]);
							}
						}
					}

					if(bypass_flag[ch] == 1)
					{
						stage_update[ch] = 0;
						loop_cnt[ch] = 0xFF;
						//check_y_stage[ch][eq_loop_cnt[ch]] = 7;
					}
					else if(bypass_flag[ch] == 0)
					{
						if( loop_cnt[ch] >= (ACP_READ_START_CNT+bypass_flag_retry[ch]))
						{
							last_ystage = compare_arr(eq_loop_cnt[ch],check_y_stage[ch]);
							last_sarv0  = compare_arr(eq_loop_cnt[ch],check_sarv0_stage[ch]);
							last_sarv3  = compare_arr(eq_loop_cnt[ch],check_sarv3_stage[ch]);

							//if(ch == 0)
							{
								printk("last_ystage = %d\n",last_ystage);
								printk("last_sarv0  = %d\n",last_sarv0);
								printk("last_sarv3  = %d\n",last_sarv3);
							}

							if(check_stage(ch, last_ystage,last_sarv0,last_sarv3) != -1)
							{
								stage_update[ch] = 1;
							}
							else
							{
								ystage_flag_retry[ch]++;

								if(ystage_flag_retry[ch]>=RETRY_CNT)
								{
									stage_update[ch] = 1;
									ystage_flag_retry[ch] = 0;
									if((last_ystage > last_sarv0) && (last_ystage > last_sarv3))
										check_y_stage[ch][eq_loop_cnt[ch]] = last_sarv3;
									else if(	(last_ystage < last_sarv0) 	&&
												(last_ystage < last_sarv3) 	&&
												(last_sarv0 != 0xFF) 		&&
												(last_sarv3 != 0xFF))
										check_y_stage[ch][eq_loop_cnt[ch]] = last_sarv3;
									else
										check_y_stage[ch][eq_loop_cnt[ch]] = calc_arr_mean(check_y_stage[ch]);
									printk("check_y_stage[ch][eq_loop_cnt[ch]] = %d\n",check_y_stage[ch][eq_loop_cnt[ch]]);
								}
							}
						}
					}
					else //bypass_flag == 0xff
					{
						stage_update[ch] = 0;
					}
				}
			}
			else
			{
				if(pre_vidmode[ch] != vidmode[ch])
				{
					stage_update[ch] = 1;

					for(i=0;i<LOOP_BUF_SIZE;i++)
					{
						check_y_stage[ch][i]=0;
						check_sarv0_stage[ch][i]=0;
						check_sarv3_stage[ch][i]=0;
						acp_val[ch][i] = 0;
						acp_ptn[ch][i] = 0;
					}
					eq_loop_cnt[ch]=0;
					check_y_stage[ch][eq_loop_cnt[ch]] = 0;
					loop_cnt[ch] =0;
					bypass_flag_retry[ch]=0;
					ystage_flag_retry[ch]=0;
					one_setting[ch] = 0;
					acp_isp_wr_en[ch]=0;
					lossfakechannel(ch, 0);

					min_sarv0[ch] = 0xFFFF;
					max_sarv0[ch] = 0x0000;
					min_sarv3[ch] = 0xFFFF;
					max_sarv3[ch] = 0x0000;
					min_ystage[ch] = 0xFFFF;
					max_ystage[ch] = 0x0000;
				}
				else
					stage_update[ch] = 0;
			}
			pre_vidmode[ch] = vidmode[ch];

			if(stage_update[ch])
			{
				stage_update[ch] = 0;
				acp_isp_wr_en[ch]=1;
				eq_stage[ch] = check_y_stage[ch][eq_loop_cnt[ch]];

				if(video_on[ch])
					loop_cnt[ch] = 0xFF;
				else
				{
					init_eq_stage(ch);
					set_color_burst_threshold(ch,eq_stage[ch],0x00);
				}

				printk("[%x][%d]CH[%d]-Stage update : eq_stage = %d\n",acp_val[ch][eq_loop_cnt[ch]],acp_ptn[ch][eq_loop_cnt[ch]],ch,eq_stage[ch]);
				nvp6124b_ckill_eq(ch, eq_stage[ch]);
				set_analog_stage(ch, eq_stage[ch]);
				set_iir_stage(ch, eq_stage[ch]);
				set_color_burst_threshold(ch,eq_stage[ch],0x01);
			}
		}
	}
}



