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
#endif

//#include "gpio_i2c.h"
#include "video.h"
#include "nvp6124_reg.h"
#include "coax_protocol.h"

/*nvp6124 1080P 色彩推荐配置*/
#define BRI_CENTER_VAL_NTSC 0xF4
#define BRI_CENTER_VAL_PAL  0xF4
#define CON_CENTER_VAL_NTSC 0x90
#define CON_CENTER_VAL_PAL  0x90
#define SAT_CENTER_VAL_NTSC 0x80
#define SAT_CENTER_VAL_PAL  0x80
#define HUE_CENTER_VAL_NTSC 0x00
#define HUE_CENTER_VAL_PAL  0x00


/*nvp6124 720P 色彩推荐配置*/
#define BRI_CENTER_VAL_NTSC_720P 0x08
#define BRI_CENTER_VAL_PAL_720P  0x08
#define CON_CENTER_VAL_NTSC_720P 0x88
#define CON_CENTER_VAL_PAL_720P  0x88
#define SAT_CENTER_VAL_NTSC_720P 0x90
#define SAT_CENTER_VAL_PAL_720P  0x90
#define HUE_CENTER_VAL_NTSC_720P 0xFD
#define HUE_CENTER_VAL_PAL_720P  0x00

/*nvp6124 960H 色彩推荐配置*/
#define BRI_CENTER_VAL_NTSC_960H 0xFF
#define BRI_CENTER_VAL_PAL_960H  0x00
#define CON_CENTER_VAL_NTSC_960H 0x92
#define CON_CENTER_VAL_PAL_960H  0x90
#define SAT_CENTER_VAL_NTSC_960H 0x88
#define SAT_CENTER_VAL_PAL_960H  0x88
#define HUE_CENTER_VAL_NTSC_960H 0x01
#define HUE_CENTER_VAL_PAL_960H  0x00

unsigned int nvp6124_con_tbl[2]  = {CON_CENTER_VAL_NTSC, CON_CENTER_VAL_PAL};
unsigned int nvp6124_hue_tbl[2]  = {HUE_CENTER_VAL_NTSC, HUE_CENTER_VAL_PAL};
unsigned int nvp6124_sat_tbl[2]  = {SAT_CENTER_VAL_NTSC, SAT_CENTER_VAL_PAL};
unsigned int nvp6124_bri_tbl[2]  = {BRI_CENTER_VAL_NTSC, BRI_CENTER_VAL_PAL};

unsigned int nvp6124_con_tbl_720P[2]  = {CON_CENTER_VAL_NTSC_720P, CON_CENTER_VAL_PAL_720P};
unsigned int nvp6124_hue_tbl_720P[2]  = {HUE_CENTER_VAL_NTSC_720P, HUE_CENTER_VAL_PAL_720P};
unsigned int nvp6124_sat_tbl_720P[2]  = {SAT_CENTER_VAL_NTSC_720P, SAT_CENTER_VAL_PAL_720P};
unsigned int nvp6124_bri_tbl_720P[2]  = {BRI_CENTER_VAL_NTSC_720P, BRI_CENTER_VAL_PAL_720P};


unsigned int nvp6124_con_tbl_960H[2]  = {CON_CENTER_VAL_NTSC_960H, CON_CENTER_VAL_PAL_960H};
unsigned int nvp6124_hue_tbl_960H[2]  = {HUE_CENTER_VAL_NTSC_960H, HUE_CENTER_VAL_PAL_960H};
unsigned int nvp6124_sat_tbl_960H[2]  = {SAT_CENTER_VAL_NTSC_960H, SAT_CENTER_VAL_PAL_960H};
unsigned int nvp6124_bri_tbl_960H[2]  = {BRI_CENTER_VAL_NTSC_960H, BRI_CENTER_VAL_PAL_960H};

unsigned char nvp6124_motion_sens_tbl[8]= {0xe0,0xc8,0xa0,0x98,0x78,0x68,0x50,0x48};
unsigned char ch_mode_status[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};
unsigned char ch_vfmt_status[16]={0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff};

extern unsigned int nvp6124_cnt;
extern int chip_id[4];
extern unsigned int vloss;

void nvp6124_common_init(void);
void nvp6124_write_table(unsigned char chip_addr, unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt);
void nvp6124b_syncchange(unsigned char ch, nvp6124_video_mode *pvmode);


extern unsigned int nvp6124_slave_addr[4];
void NVP6124_AfeReset(unsigned char ch)
{
	unsigned int i = 0;
	if(ch == 0xFF)
	{
		for(i=0;i<nvp6124_cnt;i++)
		{
			gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x00);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x02, gpio_i2c_read(nvp6124_slave_addr[i], 0x02)|0x0F);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x02, gpio_i2c_read(nvp6124_slave_addr[i], 0x02)&0xF0);
		}
		printk("NVP6124_AfeReset ALL Channel done\n");
	}
	else
	{
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x02, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x02)|(0x01<<(ch%4)));
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x02, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x02)&0xF0);
		printk("NVP6124_AfeReset ch[%d] done\n", ch);
	}

}

void nvp6124_datareverse(void)
{
/*
BANK1 0xD2[5:2],每个bit控制一个bt656的数据顺序，1为反序，0为正序。
*/
	unsigned int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xD2, 0x3C);
	}
	printk("nvp6124 data reversed\n");
}

unsigned char chipon297MHz[4]={0};
void nvp6124_set_clockmode(unsigned char chip, unsigned char is297MHz)
{
	gpio_i2c_write(nvp6124_slave_addr[chip], 0xFF, 0x01);
	gpio_i2c_write(nvp6124_slave_addr[chip], 0x80, 0x40);
	if(is297MHz == 1)
	{
		gpio_i2c_write(nvp6124_slave_addr[chip], 0x82, 0x12);
		chipon297MHz[chip] = 1;
	}
	else
	{
		gpio_i2c_write(nvp6124_slave_addr[chip], 0x82, 0x14);
		chipon297MHz[chip] = 0;
	}
	gpio_i2c_write(nvp6124_slave_addr[chip], 0x83, 0x2C);
	gpio_i2c_write(nvp6124_slave_addr[chip], 0x80, 0x61);
	gpio_i2c_write(nvp6124_slave_addr[chip], 0x80, 0x60);

	printk("chip %d nvp6124_set_clockmode %d\n", chip, is297MHz);
}

void nvp6124_system_init(void)
{
	unsigned int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
		if(chip_id[i] == NVP6124B_R0_ID)
		{
			gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x82, 0x12);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x83, 0x2C);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x3e, 0x10);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x61);
			msleep(100);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x40);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x81, 0x02);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x97, 0x00);
			msleep(10);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
			msleep(10);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x97, 0x0F);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x18);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x08);
			msleep(10);
			printk("nvp6124b_system_init\n");
			msleep(100);
			gpio_i2c_write(nvp6124_slave_addr[i], 0xCA, 0xAE);
		}
		else
		{
			gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x82, 0x14);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x83, 0x2C);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x3e, 0x10);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x61);
			msleep(100);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x40);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x81, 0x02);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x97, 0x00);
			msleep(10);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
			msleep(10);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x97, 0x0F);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x18);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x08);
			msleep(10);
			printk("nvp6124_system_init\n");
			msleep(100);
			if(chip_id[i] == NVP6124_R0_ID)
				gpio_i2c_write(nvp6124_slave_addr[i], 0xCA, 0xFF);
			else
				gpio_i2c_write(nvp6124_slave_addr[i], 0xCA, 0xAE);  //only enable 2 ports on 6114A
			chipon297MHz[i] = 0;
		}
	}
}

void software_reset(void)
{
	unsigned int i = 0;
	for(i=0;i<nvp6124_cnt;i++)
	{
	    gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
	    gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x40);
	    gpio_i2c_write(nvp6124_slave_addr[i], 0x81, 0x02);
	    gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x61);
	    gpio_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
	    msleep(100);
	    gpio_i2c_write(nvp6124_slave_addr[i], 0x80, 0x60);
	    gpio_i2c_write(nvp6124_slave_addr[i], 0x81, 0x00);
	    msleep(100);
	}
	printk("\n\r nvp6124 software reset!!!");
}

void nvp6124_common_init(void)
{
	unsigned int i = 0;

	for(i=0;i<nvp6124_cnt;i++)
	{
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x00);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B0_Buf,254);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x01);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B1_Buf,254);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x02);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B2_Buf,254);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x03);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B3_Buf,254);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x04);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B4_Buf,254);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x09);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_B9_Buf,254);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0A);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_BA_Buf,254);
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x0B);
        nvp6124_write_table(nvp6124_slave_addr[i],0x00,NVP6124_BB_Buf,254);
	}

	nvp6124_system_init();
}

/*nvp6124b视频模式值转换*/
static unsigned int nvp6124b_vdsts2_nvp6124(unsigned int vdsts)
{
	unsigned int ret;
	switch(vdsts)
	{
		case 0x00:			//nvp6124b NTSC SD状态值
			ret = 0x01;		//nvp6124  NTSC SD状态值
		break;
		case 0x10:          //nvp6124b PAL SD状态值
			ret = 0x02;
		break;
		case 0x20:			//nvp6124b NT 720P/30状态值
			ret = 0x04;
		break;
		case 0x21:
			ret = 0x08;
		break;
		case 0x22:
			ret = 0x10;
		break;
		case 0x23:
			ret = 0x20;
		break;
		case 0x30:
			ret = 0x40;
		break;
		case 0x31:
			ret = 0x80;
		break;
		case 0xFF:
			ret = 0x00;
		break;
		default:
			ret = vdsts;
		break;
	}
	return ret;
}

#define CNT 5
unsigned int video_fmt_debounce(unsigned char ch)
{
	unsigned int idx=CNT-1;
	unsigned int tmp, buf[CNT]={0,0,0,0,0};
	int i, j,cnt[CNT]={0,0,0,0,0};

	for(i=0; i<CNT; i++)
	{
		if(chip_id[ch/4] == NVP6124B_R0_ID)
		{
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
			tmp = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xF0);
			buf[i] = nvp6124b_vdsts2_nvp6124(tmp);
			if(buf[i] == 0)
			{
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
				buf[i] = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xD0+(ch%4));
			}
		}
		else
		{
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
			buf[i] = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xD0+(ch%4));
		}
	}
	for(i=0; i<CNT; i++)
    {
        for(j=0; j<CNT; j++)
        {
            if(buf[i] == buf[j])
            {
            	cnt[i]++;
            }
        }
        if(i>0 && cnt[i-1]>=cnt[i])
	        idx = i-1;
    }
	printk("video_fmt_debounce ch[%d] buf[%d] = %x\n", ch, idx, buf[idx]);
	return buf[idx];
}

/*视频输入制式检测函数*/
void video_fmt_det(nvp6124_input_videofmt *pvideofmt)
{
	unsigned int i;
	unsigned char tmp;
	static nvp6124_input_videofmt videofmt;

	for(i=0; i<nvp6124_cnt*4; i++)
	{
	    if(chip_id[i/4] != NVP6124B_R0_ID)
		{
			gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
			pvideofmt->getvideofmt[i] = gpio_i2c_read(nvp6124_slave_addr[i/4], 0xD0+i%4);
		}
		else
		{
			gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x05 + i%4);
			tmp =  gpio_i2c_read(nvp6124_slave_addr[i/4], 0xF0);
			pvideofmt->getvideofmt[i] = nvp6124b_vdsts2_nvp6124(tmp);
			if(pvideofmt->getvideofmt[i] == 0x00)
			{
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
				pvideofmt->getvideofmt[i] = gpio_i2c_read(nvp6124_slave_addr[i/4], 0xD0+i%4);
			}
		}

		if(videofmt.getvideofmt[i] != pvideofmt->getvideofmt[i])
		{
			pvideofmt->getvideofmt[i] = video_fmt_debounce(i);
			videofmt.getvideofmt[i] = pvideofmt->getvideofmt[i];
		}

	    if(chip_id[i/4] != NVP6124B_R0_ID)
		{
			if(    ((pvideofmt->getvideofmt[i] == 0x40)&&(((vloss>>i)&0x01) == 0x00))
				|| ((pvideofmt->getvideofmt[i] == 0x10)&&(((vloss>>i)&0x01) == 0x00)))
			{
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0x23+4*(i%4), 0x41);
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x05+i%4);
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0x47, 0xee);
			}
			else
			{
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x00);
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0x23+4*(i%4), 0x43);
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x05+i%4);
				gpio_i2c_write(nvp6124_slave_addr[i/4], 0x47, 0x04);
			}
		}
		else
		{
			if(ch_mode_status[i] == NVP6124_VI_1080P_2530 && ch_vfmt_status[i] == NTSC)
			{
				if(((vloss>>i)&0x01) == 0x00)
				{
					gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x05+i%4);
					gpio_i2c_write(nvp6124_slave_addr[i/4], 0x47, 0xEE);
				}
				else
				{
					gpio_i2c_write(nvp6124_slave_addr[i/4], 0xFF, 0x05+i%4);
					gpio_i2c_write(nvp6124_slave_addr[i/4], 0x47, 0x04);
				}
			}
		}
	}
}

unsigned int nvp6124_getvideoloss(void)
{
	unsigned int vloss=0, i;
	//static unsigned int vlossbak=0xFFFF;

	for(i=0;i<nvp6124_cnt;i++)
	{
		gpio_i2c_write(nvp6124_slave_addr[i], 0xFF, 0x00);
		vloss|=(gpio_i2c_read(nvp6124_slave_addr[i], 0xB8)&0x0F)<<(4*i);
	}
	#if 0
	if(vlossbak != vloss)
	{
		for(ch=0;ch<nvp6124_cnt*4;ch++)
		{
			if(((vloss>>ch)&0x01)==0x00 && ((vlossbak>>ch)&0x01)==0x01)
				NVP6124_AfeReset(ch);
		}
		vlossbak = vloss;
	}
	#endif
	return vloss;
}

/*
chip:chip select[0,1,2,3];
portsel: port select[0,1];
portmode: port mode select[1mux,2mux,4mux]
chid:  channel id, 1mux[0,1,2,3], 2mux[0,1], 4mux[0]
*/
void nvp6114a_set_portmode(unsigned char chip, unsigned char portsel, unsigned char portmode, unsigned char chid)
{
	unsigned char chipaddr = nvp6124_slave_addr[chip];
	if(portsel>1)
		printk("nvp6114a_set_portmode portsel[%d] error!!!\n", portsel);
	if(portmode == NVP6124_OUTMODE_2MUX_FHD)
	{
		if(chipon297MHz[chip] == 0)
			nvp6124_set_clockmode(chip, 1);
	}
	else
	{
		if(chipon297MHz[chip] == 1)
			nvp6124_set_clockmode(chip, 0);
	}
	switch(portmode)
	{
		case NVP6124_OUTMODE_1MUX_SD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x36);
		break;
		case NVP6124_OUTMODE_1MUX_HD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x46);
		break;
		case NVP6124_OUTMODE_1MUX_HD5060:
		case NVP6124_OUTMODE_1MUX_FHD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x66);
			break;
		case NVP6124_OUTMODE_2MUX_SD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x22);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x46);
			break;
		case NVP6124_OUTMODE_2MUX_HD_X:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x11);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x46);
			break;
		case NVP6124_OUTMODE_2MUX_FHD_X:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x11);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x66);
			break;
		case NVP6124_OUTMODE_2MUX_HD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x22);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x66);
			break;
		case NVP6124_OUTMODE_4MUX_SD:
		case NVP6124_OUTMODE_4MUX_HD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, 0x10);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, 0x32);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x88);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x66);
			break;
		case NVP6124_OUTMODE_4MUX_HD_X:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, 0x10);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, 0x32);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x33);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x66);
			break;
		case NVP6124_OUTMODE_2MUX_FHD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x88+chid*2, 0x88);
			gpio_i2c_write(chipaddr, 0x89+chid*2, 0x88);
			gpio_i2c_write(chipaddr, 0x8C+chid*2, 0x42);
			gpio_i2c_write(chipaddr, 0x8D+chid*2, 0x42);
			gpio_i2c_write(chipaddr, 0xC2+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC3+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC6, gpio_i2c_read(chipaddr, 0xC4));
			gpio_i2c_write(chipaddr, 0xC7, gpio_i2c_read(chipaddr, 0xC5));
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x22);
			gpio_i2c_write(chipaddr, 0xCD+(portsel<<1), 0x46);
			break;
		default:
			printk("portmode %d not supported yet\n", portmode);
			break;
  	}
	printk("nvp6114a_set_portmode portsel %d portmode %d setting\n", portsel, portmode);
}


/*
chip:chip select[0,1,2,3];
portsel: port select[0,1];
portmode: port mode select[1mux,2mux,4mux]
chid:  channel id, 1mux[0,1,2,3], 2mux[0,1], 4mux[0]
NOTE:
portsel == 0,对应芯片硬件VDO1,寄存器描述VPORT_2;
portsel == 1,对应芯片硬件VDO2,寄存器描述VPORT_1;
*/
int nvp6124b_set_portmode(unsigned char chip, unsigned char portsel, unsigned char portmode, unsigned char chid)
{
	unsigned char chipaddr = nvp6124_slave_addr[chip];
	unsigned char tmp=0, reg1=0, reg2=0;

	if(portsel>1)
	{
		printk("nvp6124b_set_portmode portsel[%d] error!!!\n", portsel);
		return -1;
	}

	//portsel = portsel^1;	//port switch

	switch(portmode)
	{
		case NVP6124_OUTMODE_1MUX_SD:
			/*输出720H或者960H单通道,数据37.125MHz,时钟37.125MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0xA0+chid, 0x00);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xC8+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x86);
		break;
		case NVP6124_OUTMODE_1MUX_HD:
			/*输出720P或者1280H或者1440H单通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0xA0+chid, 0x00);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x16);
		break;
		case NVP6124_OUTMODE_1MUX_HD5060:
		case NVP6124_OUTMODE_1MUX_FHD:
			/*输出720P@5060或者1080P单通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0xA0+chid, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_2MUX_SD:
			/*输出720H或者960H 2通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			//coverity[returned_value]
			reg1 = gpio_i2c_read(chipaddr, 0xFD);
			reg2 = gpio_i2c_read(chipaddr, 0xFE);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x02);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x02);
			gpio_i2c_write(chipaddr, 0xA0+chid*2, 0x00);
			gpio_i2c_write(chipaddr, 0xA1+chid*2, 0x00);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			//gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x16);
			if(portsel==0)
				gpio_i2c_write(chipaddr, 0xCF, 0x1A);
			else
				gpio_i2c_write(chipaddr, 0xCD, 0x16);
			break;
		case NVP6124_OUTMODE_2MUX_HD_X:
			/*输出HD-X 2通道,数据74.25MHz,时钟74.25MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0xA0+chid*2, 0x00);
			gpio_i2c_write(chipaddr, 0xA1+chid*2, 0x00);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x98:0xBA);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x16);
			break;
		case NVP6124_OUTMODE_2MUX_FHD_X:
			/*输出FHD-X 2通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0xA0+chid*2, 0x01);
			gpio_i2c_write(chipaddr, 0xA1+chid*2, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x10:0x01);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_2MUX_HD:
			/*输出HD 2通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0xA0+chid*2, 0x03);
			gpio_i2c_write(chipaddr, 0xA1+chid*2, 0x03);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_4MUX_SD:
			/*输出720H或者960H 4通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0xA0, 0x03);
			gpio_i2c_write(chipaddr, 0xA1, 0x03);
			gpio_i2c_write(chipaddr, 0xA2, 0x03);
			gpio_i2c_write(chipaddr, 0xA3, 0x03);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_4MUX_HD:
			/*输出720P 4通道,数据297MHz,时钟148.5MHz,双沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0xA0, 0x00);
			gpio_i2c_write(chipaddr, 0xA1, 0x00);
			gpio_i2c_write(chipaddr, 0xA2, 0x00);
			gpio_i2c_write(chipaddr, 0xA3, 0x00);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_4MUX_HD_X:
			/*输出HD-X 4通道,数据148.5MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x00);
			gpio_i2c_write(chipaddr, 0xA0, 0x00);
			gpio_i2c_write(chipaddr, 0xA1, 0x00);
			gpio_i2c_write(chipaddr, 0xA2, 0x00);
			gpio_i2c_write(chipaddr, 0xA3, 0x00);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x98);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0xBA);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_2MUX_FHD:
			/*FHD,3840H,HDEX 2mux任意混合,数据297MHz,时钟148.5MHz,双沿采样.
			SOC VI端通过丢点，实现3840H->960H, HDEX->720P  */
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0xA0+chid*2, 0x01);
			gpio_i2c_write(chipaddr, 0xA1+chid*2, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			//gpio_i2c_write(chipaddr, 0xCD, 0x65);  //297MHz clock test
			//gpio_i2c_write(chipaddr, 0xCF, 0x6A);
			break;
		case NVP6124_OUTMODE_1MUX_HD_X:   //nvp6124b
		case NVP6124_OUTMODE_1MUX_FHD_X:
			break;
		case NVP6124_OUTMODE_4MUX_FHD_X:
			/*输出FHD-X 4通道,数据297MHz,时钟148.5MHz,单沿采样.*/
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0xA0, 0x01);
			gpio_i2c_write(chipaddr, 0xA1, 0x01);
			gpio_i2c_write(chipaddr, 0xA2, 0x01);
			gpio_i2c_write(chipaddr, 0xA3, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x98);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0xBA);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_4MUX_MIX:
			/*HD,1920H,FHD-X 4mux任意混合,数据297MHz,时钟148.5MHz,双沿采样
			SOC VI端通过丢点，实现1920H->960H  */
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0xFD);
			if(((tmp&0x0F) == 0x02) || ((tmp&0x0F) == 0x03))
				reg1 |= 0x08;
			else
				reg1 &= 0xF0;
			if((((tmp>>4)&0x0F) == 0x02) || (((tmp>>4)&0x0F) == 0x03))
				reg1 |= 0x80;
			else
				reg1 &= 0x0F;
			tmp = gpio_i2c_read(chipaddr, 0xFE);
			if(((tmp&0x0F) == 0x02) || ((tmp&0x0F) == 0x03))
				reg2 |= 0x08;
			else
				reg2 &= 0xF0;
			if((((tmp>>4)&0x0F) == 0x02) || (((tmp>>4)&0x0F) == 0x03))
				reg2 |= 0x80;
			else
				reg2 &= 0x0F;
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x01);
			gpio_i2c_write(chipaddr, 0xA0, 0x01);
			gpio_i2c_write(chipaddr, 0xA1, 0x01);
			gpio_i2c_write(chipaddr, 0xA2, 0x01);
			gpio_i2c_write(chipaddr, 0xA3, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10|reg1);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32|reg2);
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x43);
			break;
		case NVP6124_OUTMODE_2MUX_MIX:
			/*HD,1920H,FHD-X 2MUX任意混合,数据148.5MHz,时钟148.5MHz,单沿采样
			SOC VI端通过丢点，实现1920H->960H  */
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			tmp = gpio_i2c_read(chipaddr, 0xFD);
			if(((tmp&0x0F) == 0x02) || ((tmp&0x0F) == 0x03))
				reg1 |= 0x08;
			else
				reg1 &= 0xF0;
			if((((tmp>>4)&0x0F) == 0x02) || (((tmp>>4)&0x0F) == 0x03))
				reg1 |= 0x80;
			else
				reg1 &= 0x0F;
			tmp = gpio_i2c_read(chipaddr, 0xFE);
			if(((tmp&0x0F) == 0x02) || ((tmp&0x0F) == 0x03))
				reg2 |= 0x08;
			else
				reg2 &= 0xF0;
			if((((tmp>>4)&0x0F) == 0x02) || (((tmp>>4)&0x0F) == 0x03))
				reg2 |= 0x80;
			else
				reg2 &= 0x0F;
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x90+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0x92+portsel, 0x03);
			gpio_i2c_write(chipaddr, 0xA0+chid*2, 0x03);
			gpio_i2c_write(chipaddr, 0xA1+chid*2, 0x03);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?(0x10|reg1):(0x32|reg2));
			tmp = gpio_i2c_read(chipaddr, 0xC8) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8, tmp);
			gpio_i2c_write(chipaddr, 0xCF-(portsel<<1), 0x46);
			break;
		default:
			printk("portmode %d not supported yet\n", portmode);
			break;
  	}
	printk("nvp6124b_set_portmode portsel %d portmode %d  ch %d setting\n", portsel, portmode, chid);
	return 0;
}


void nvp6124_set_fpc(unsigned char ch, unsigned char vformat)
{
	unsigned char tmp;
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	tmp = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x54);
	if(vformat==PAL)
		tmp &= ~(0x10<<(ch%4));
	else
		tmp |= (0x10<<(ch%4));
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x54, tmp);
}


void nvp6124b_syncchange(unsigned char ch, nvp6124_video_mode *pvmode)
{
	unsigned char vformat = pvmode->vformat[ch];

	if(vformat == NTSC)
	{
		if(pvmode->chmode[ch] == NVP6124_VI_1080P_2530)
		{
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x00);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+(ch%4)*4,0x43);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+(ch%4),0x4E);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (ch%4)+0x8E,0x09);

			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x05+(ch%4));
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x24,0x1A);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x47,0xEE);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50,0xC6);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xBB,0x04);
		}
		else if(pvmode->chmode[ch]  == NVP6124_VI_720P_5060)
		{
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x00);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+(ch%4)*4,0x43);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+(ch%4),0xa8);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (ch%4)+0x8E,0x09);

			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x05+(ch%4));
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x24,0x2A); //
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x47,0xEE);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50,0xC6);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xBB,0x04);
		}
		else
		{
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x00);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+(ch%4)*4,0x43);
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x05+(ch%4));
			gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x47,0x04);
		}
	}
	else
	{
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x00);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+(ch%4)*4,0x43);
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xff,0x05+(ch%4));
		gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x47,0x04);
	}
}

void nvp6124_each_mode_setting(nvp6124_video_mode *pvmode)
{
	unsigned char tmp;
	unsigned char ch, chmode[16];
	unsigned char pn_value_sd_nt_comet[4] = {0x4D,0x0E,0x88,0x6C};
	unsigned char pn_value_720p_30[4] = 	{0xEE,0x00,0xE5,0x4E};
	unsigned char pn_value_720p_60[4] = 	{0x2C,0xF9,0xC5,0x52};
	unsigned char pn_value_fhd_nt[4] = 		{0x2C,0xF0,0xCA,0x52};
	unsigned char pn_value_sd_pal_comet[4]= {0x75,0x35,0xB4,0x6C};
	unsigned char pn_value_720p_25[4] = 	{0x46,0x08,0x10,0x4F};
	unsigned char pn_value_720p_50[4] = 	{0x2C,0xE7,0xCF,0x52};
	unsigned char pn_value_fhd_pal[4] = 	{0xC8,0x7D,0xC3,0x52};
	unsigned char vformat[16];

  	for(ch=0;ch<(nvp6124_cnt*4);ch++)
  	{
		chmode[ch] = pvmode->chmode[ch];
		vformat[ch] = pvmode->vformat[ch];
		if(chip_id[ch/4] != NVP6124B_R0_ID)
		{
			if(chmode[ch]>NVP6124_VI_1440H && chmode[ch]<NVP6124_VI_BUTT)
				printk("!!!chmode[%d]=%d is out of range!!!\n", ch, chmode[ch]);
		}
  	}

	for(ch=0;ch<(nvp6124_cnt*4);ch++)
	{
		if(chmode[ch] < NVP6124_VI_BUTT)
		{
			switch(chmode[ch])
			{
				case NVP6124_VI_SD:
				case NVP6124_VI_1920H:
				case NVP6124_VI_720H:
				case NVP6124_VI_1280H:
				case NVP6124_VI_1440H:
				case NVP6124_VI_960H2EX:
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x08+ch%4, vformat[ch]==PAL?0xDD:0xA0);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x0c+ch%4, nvp6124_bri_tbl_960H[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x10+ch%4, nvp6124_con_tbl_960H[vformat[ch]%2]);
					//coverity[identical_branches]
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x14+ch%4, vformat[ch]==PAL?0x80:0x80);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x18+ch%4, vformat[ch]==PAL?0x18:0x18);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), vformat[ch]==PAL?0x02:0x82);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0B);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), 0x43);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x30+ch%4, vformat[ch]==PAL?0x12:0x11);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x3c+ch%4, nvp6124_sat_tbl_960H[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+ch%4, nvp6124_hue_tbl_960H[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44+ch%4, vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x48+ch%4, vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x4c+ch%4, vformat[ch]==PAL?0x04:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+ch%4, vformat[ch]==PAL?0x04:0x00);
					nvp6124_set_fpc(ch, vformat[ch]);
					if(chmode[ch]==NVP6124_VI_SD)
					{
						if(chip_id[ch/4] == NVP6124B_R0_ID)
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x60:0xC0);
						else
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x80:0x90);
					}
					else if(chmode[ch]==NVP6124_VI_960H2EX)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x80:0x90);
					else if(chmode[ch]==NVP6124_VI_720H)
					{
						if(chip_id[ch/4] == NVP6124B_R0_ID)
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x70:0x70);
						else
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0xB0:0x40);
					}
					else if(chmode[ch]==NVP6124_VI_1280H)
					{
						if(chip_id[ch/4] == NVP6124B_R0_ID)
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x50:0x50);
						else
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x80:0x90);
					}

					else if(chmode[ch]==NVP6124_VI_1440H)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x90:0xA0);
					else
					{
						if(chip_id[ch/4] == NVP6124B_R0_ID)
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x88:0x98);
						else
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4, vformat[ch]==PAL?0x3B:0x4B);
					}
					//coverity[identical_branches]
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x5c+ch%4, vformat[ch]==PAL?0x1e:0x1e);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x64+ch%4, vformat[ch]==PAL?0x0d:0x08);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						if(chmode[ch]==NVP6124_VI_SD || chmode[ch]==NVP6124_VI_720H)
						{
							//coverity[identical_branches]
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x11:0x11);
							if(chmode[ch]==NVP6124_VI_720H)
							{
								printk("ch %d setted to 720H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
								//coverity[identical_branches]
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x11:0x11);
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x00:0x00);
							}
							else
							{
								printk("ch %d setted to 960H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x10:0x00);
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x08:0x07);
							}
						}
						else if(chmode[ch]==NVP6124_VI_1280H)
						{
							printk("ch %d setted to 1280H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
							//coverity[identical_branches]
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x20:0x20);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x10:0x10);
		 					gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x20:0x20);
						}
						else if(chmode[ch]==NVP6124_VI_1440H)
						{
							printk("ch %d setted to 1440H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x30:0x30);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x10:0x10);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x10:0x10);
						}
						else if(chmode[ch]==NVP6124_VI_960H2EX)
						{
							printk("ch %d setted to 3840H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x11:0x11);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x10:0x10);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x22:0x22);
						}
						else
						{
							printk("ch %d setted to 1920H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x40:0x40);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x11:0x11);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x10:0x10);
						}
					}
					else
					{
						if(chmode[ch]==NVP6124_VI_SD || chmode[ch]==NVP6124_VI_720H)
						{
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x11:0x11);
							if(chmode[ch]==NVP6124_VI_720H)
							{
								printk("ch %d setted to 720H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x11:0x11);
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x20:0x00);
							}
							else
							{
								printk("ch %d setted to 960H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x10:0x00);
			 					gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x08:0x07);
							}
						}
						else if(chmode[ch]==NVP6124_VI_1280H)
						{
							printk("ch %d setted to 1280H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x20:0x20);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x10:0x10);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x17:0x17);
						}
						else if(chmode[ch]==NVP6124_VI_1440H)
						{
							printk("ch %d setted to 1440H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x30:0x30);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x10:0x10);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x0B:0x0B);
						}
						else
						{
							printk("ch %d setted to 1920H %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4, vformat[ch]==PAL?0x40:0x40);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4, vformat[ch]==PAL?0x00:0x00);
			 				gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E, vformat[ch]==PAL?0x10:0x10);
						}
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x93+ch%4, 0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x98+ch%4, vformat[ch]==PAL?0x07:0x04);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						if(chmode[ch]==NVP6124_VI_960H2EX)
		 					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4, vformat[ch]==PAL?0x16:0x15);
						else
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4, vformat[ch]==PAL?0x15:0x15);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa4+ch%4, vformat[ch]==PAL?0x05:0x07);
					}
					else
					{
	 					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4, vformat[ch]==PAL?0x00:0x10);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa4+ch%4, vformat[ch]==PAL?0x00:0x01);
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x01);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						if(chmode[ch]==NVP6124_VI_SD || chmode[ch]==NVP6124_VI_720H)
						{
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x06:0x06);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0xa6:0xa6);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)&(~(1<<(ch%4))));
						}
						else if(chmode[ch]==NVP6124_VI_1280H)
						{
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x06:0x06);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x26:0x26);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));

						}
						else if(chmode[ch]==NVP6124_VI_1440H)
						{
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x06:0x06);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x26:0x26);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
						}
						else if(chmode[ch]==NVP6124_VI_960H2EX)
						{
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x06:0x06);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x46:0x46);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
						}
						else
						{
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x06:0x06);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x06:0x06);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
						}
					}
					else
					{
						if(chmode[ch]==NVP6124_VI_SD || chmode[ch]==NVP6124_VI_720H)
						{
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x7e:0x7e);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x26:0x26);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)&(~(1<<(ch%4))));
						}
						else if(chmode[ch]==NVP6124_VI_1280H)
						{
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x7e:0x7e);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x56:0x56);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
						}
						else if(chmode[ch]==NVP6124_VI_1440H)
						{
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x7e:0x7e);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x56:0x56);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
						}
						else //if(chmode[ch]==NVP6124_VI_1920H)
						{
							//coverity[identical_branches]
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x46:0x46);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x47:0x47);
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
						}
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x02);
					tmp = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x00<<((ch%2)*4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table(nvp6124_slave_addr[ch/4], 0x00, NVP6124_B5678_SD_Buf, 254 );
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x06,0x40);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x0F,vformat[ch]==PAL?0x13:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x1B,0x08);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x20,0x88);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x1E,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x2C,vformat[ch]==PAL?0x08:0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x35,vformat[ch]==PAL?0x17:0x15);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x62,0x00);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x62,vformat[ch]==PAL?0x20:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA1,vformat[ch]==PAL?0x10:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA2,vformat[ch]==PAL?0x0E:0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA3,vformat[ch]==PAL?0x70:0x50);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA8,vformat[ch]==PAL?0x40:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAC,vformat[ch]==PAL?0x10:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAD,vformat[ch]==PAL?0x08:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAE,vformat[ch]==PAL?0x04:0x14);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC0,vformat[ch]==PAL?0x0D:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x25,vformat[ch]==PAL?0xCA:0xDA);
					if(chmode[ch]==NVP6124_VI_1280H || chmode[ch]==NVP6124_VI_1440H)
					{
						//coverity[identical_branches]
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x62, vformat[ch]==PAL?0x20:0x20);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x64, vformat[ch]==PAL?0x0D:0x0D);
					}
					else if(chmode[ch]==NVP6124_VI_960H2EX)
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x6B, 0x10);
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x09);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+(ch%4), 0x60);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x44)&(~(1<<(ch%4))));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat[ch]==PAL?pn_value_sd_pal_comet[0]:pn_value_sd_nt_comet[0]);	//ch%41 960H
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat[ch]==PAL?pn_value_sd_pal_comet[1]:pn_value_sd_nt_comet[1]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat[ch]==PAL?pn_value_sd_pal_comet[2]:pn_value_sd_nt_comet[2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat[ch]==PAL?pn_value_sd_pal_comet[3]:pn_value_sd_nt_comet[3]);
				break;
				case NVP6124_VI_720P_2530:
				case NVP6124_VI_HDEX:
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					//coverity[identical_branches]
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x08+ch%4,vformat[ch]==PAL?0x60:0x60);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x0c+ch%4,nvp6124_bri_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x10+ch%4,nvp6124_con_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x14+ch%4,vformat[ch]==PAL?0x90:0x90);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x18+ch%4,vformat[ch]==PAL?0x30:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), 0x92);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0A);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), 0x43);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x30+ch%4,0x15);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x30+ch%4,0x12);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x3c+ch%4,nvp6124_sat_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+ch%4,nvp6124_hue_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44+ch%4,vformat[ch]==PAL?0x30:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x48+ch%4,vformat[ch]==PAL?0x30:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x4c+ch%4,vformat[ch]==PAL?0x04:0x04);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+ch%4,vformat[ch]==PAL?0x04:0x04);
					nvp6124_set_fpc(ch, vformat[ch]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4,vformat[ch]==PAL?0x80:0x90);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x5c+ch%4,vformat[ch]==PAL?0x9e:0x9e);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x64+ch%4,vformat[ch]==PAL?0xb1:0xb2);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4,vformat[ch]==PAL?0x07:0x06);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4,vformat[ch]==PAL?0x10:0x10);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						if(chmode[ch] == NVP6124_VI_HDEX)
							gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E,0x07);
						else
							gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E,0x0d);
					}
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E,vformat[ch]==PAL?0x0d:0x0d);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x93+ch%4,0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x98+ch%4,vformat[ch]==PAL?0x07:0x04);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4,0x15);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4,0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa4+ch%4,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x01);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x0A:0x0A);

						if(chmode[ch] == NVP6124_VI_HDEX)
						{
							printk("ch %d setted to HDEX(2560x720) %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x4A:0x5A);
						}
						else
						{
							printk("ch %d setted to 720P %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
							gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x0A:0x0A);
						}

						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x9E, vformat[ch]==PAL?0x55:0x55);
					}
					else
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4,vformat[ch]==PAL?0x5C:0x5C);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4,vformat[ch]==PAL?0x40:0x40);
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x02);
					tmp = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x05<<((ch%2)*4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table(nvp6124_slave_addr[ch/4], 0x00, NVP6124_B5678_Buf,254 );
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x01,0x0D);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x06,0x40);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x1E,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x35,vformat[ch]==PAL?0x17:0x15);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x7A,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x7B,vformat[ch]==PAL?0x00:0x81);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA1,vformat[ch]==PAL?0x10:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA2,vformat[ch]==PAL?0x0E:0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA3,vformat[ch]==PAL?0x70:0x50);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA8,vformat[ch]==PAL?0x40:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAC,vformat[ch]==PAL?0x10:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAD,vformat[ch]==PAL?0x08:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAE,vformat[ch]==PAL?0x04:0x14);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x25,0xDB);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x2B,0x78);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x59,0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58,0x13);
					if(chmode[ch] == NVP6124_VI_HDEX)
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x54, 0x20);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x55, 0x11);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x6B, 0x01);
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC0,0x16);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC1, 0x13);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC8, 0x04);
					}
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC1,0x14);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xD8,0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xD9,0x0E);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDA,0x12);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDB,0x14);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDC,0x1C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDD,0x2C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDE,0x34);

					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x09);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+(ch%4),0x00);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44, 0x00);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x44)|(1<<(ch%4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat[ch]==PAL?pn_value_720p_25[0]:pn_value_720p_30[0]);	//ch%41 960H
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat[ch]==PAL?pn_value_720p_25[1]:pn_value_720p_30[1]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat[ch]==PAL?pn_value_720p_25[2]:pn_value_720p_30[2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat[ch]==PAL?pn_value_720p_25[3]:pn_value_720p_30[3]);
				break;
				case NVP6124_VI_720P_5060:
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x08+ch%4,vformat[ch]==PAL?0x60:0x60);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x0c+ch%4,nvp6124_bri_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x10+ch%4,nvp6124_con_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x14+ch%4,vformat[ch]==PAL?0x90:0x90);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x18+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), 0x92);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0A);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), vformat[ch]==PAL?0x43:0x43);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x30+ch%4,vformat[ch]==PAL?0x12:0x12);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x3c+ch%4,nvp6124_sat_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+ch%4,nvp6124_hue_tbl_720P[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44+ch%4,vformat[ch]==PAL?0x30:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x48+ch%4,vformat[ch]==PAL?0x30:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x4c+ch%4,vformat[ch]==PAL?0x04:0x04);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+ch%4,vformat[ch]==PAL?0x04:0x04);
					nvp6124_set_fpc(ch, vformat[ch]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4,vformat[ch]==PAL?0xc0:0xb0);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x5c+ch%4,vformat[ch]==PAL?0x9e:0x9e);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x64+ch%4,vformat[ch]==PAL?0xb1:0xb2);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4,vformat[ch]==PAL?0x05:0x04);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4,vformat[ch]==PAL?0x10:0x10);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E,vformat[ch]==PAL?0x0b:0x09);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x93+ch%4,0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x98+ch%4,vformat[ch]==PAL?0x07:0x04);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa4+ch%4,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x01);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4,vformat[ch]==PAL?0x00:0x00);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4,vformat[ch]==PAL?0x42:0x42);
					}
					else
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4,vformat[ch]==PAL?0x4d:0x4d);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4,vformat[ch]==PAL?0x84:0x84);
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x02);
					tmp = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x05<<((ch%2)*4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table(nvp6124_slave_addr[ch/4], 0x00, NVP6124_B5678_Buf,254 );
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x01,0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x06,0x40);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x1E,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x35,vformat[ch]==PAL?0x17:0x15);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x7A,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x7B,vformat[ch]==PAL?0x00:0x81);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA1,vformat[ch]==PAL?0x10:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA2,vformat[ch]==PAL?0x0E:0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA3,vformat[ch]==PAL?0x70:0x50);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA8,vformat[ch]==PAL?0x40:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAC,vformat[ch]==PAL?0x10:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAD,vformat[ch]==PAL?0x08:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAE,vformat[ch]==PAL?0x04:0x14);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x2B,0x78);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58,0x13);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x59,0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x24,vformat[ch]==PAL?0x2A:0x1A);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50,vformat[ch]==PAL?0x84:0x86);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xBB,vformat[ch]==PAL?0x00:0xE4);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xD8,0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xD9,0x0E);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDA,0x12);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDB,0x14);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDC,0x1C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDD,0x2C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDE,0x34);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF,0x09);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+(ch%4), 0x00);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44, 0x00);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x44)|(1<<(ch%4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat[ch]==PAL?pn_value_720p_50[0]:pn_value_720p_60[0]);	//ch%41 960H
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat[ch]==PAL?pn_value_720p_50[1]:pn_value_720p_60[1]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat[ch]==PAL?pn_value_720p_50[2]:pn_value_720p_60[2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat[ch]==PAL?pn_value_720p_50[3]:pn_value_720p_60[3]);
					printk("ch %d setted to 720P@RT %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
				break;
				case NVP6124_VI_1080P_2530:
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x08+ch%4,vformat[ch]==PAL?0x60:0x60);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x0c+ch%4,nvp6124_bri_tbl[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x10+ch%4,nvp6124_con_tbl[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x14+ch%4,vformat[ch]==PAL?0x90:0x90);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x18+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x21+4*(ch%4), 0x92);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0B);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x22+4*(ch%4), 0x0A);
					//coverity[identical_branches]
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x23+4*(ch%4), vformat[ch]==PAL?0x43:0x43);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x30+ch%4,0x15);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x30+ch%4,0x12);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x3c+ch%4,nvp6124_sat_tbl[vformat[ch]%2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+ch%4,nvp6124_hue_tbl[vformat[ch]%2]);
					//coverity[identical_branches]
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x48+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x4c+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+ch%4,vformat[ch]==PAL?0x00:0x00);
					nvp6124_set_fpc(ch, vformat[ch]);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4,vformat[ch]==PAL?0x78:0x57);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58+ch%4,vformat[ch]==PAL?0x6a:0x49);
					//coverity[identical_branches]
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x5c+ch%4,vformat[ch]==PAL?0x9e:0x9e);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x64+ch%4,vformat[ch]==PAL?0xbf:0x8d);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x81+ch%4,vformat[ch]==PAL?0x03:0x02);
					//coverity[identical_branches]
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x85+ch%4,vformat[ch]==PAL?0x00:0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x89+ch%4,vformat[ch]==PAL?0x10:0x10);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], ch%4+0x8E,vformat[ch]==PAL?0x0a:0x09);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x93+ch%4,0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x98+ch%4,vformat[ch]==PAL?0x07:0x04);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4,0x15);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa0+ch%4,0x00);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xa4+ch%4,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x01);

					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						//coverity[identical_branches]
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4, vformat[ch]==PAL?0x00:0x00);//NVP6124A NYH
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4, vformat[ch]==PAL?0x42:0x42);
					}
					else
					{
						//coverity[identical_branches]
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x88+ch%4,vformat[ch]==PAL?0x4c:0x4c);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x8c+ch%4,vformat[ch]==PAL?0x84:0x84);
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xd7, gpio_i2c_read(nvp6124_slave_addr[ch/4], 0xd7)|(1<<(ch%4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x02);
					tmp = gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x16+(ch%4)/2, (tmp&(ch%2==0?0xF0:0x0F))|(0x05<<((ch%2)*4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x05+ch%4);
					nvp6124_write_table(nvp6124_slave_addr[ch/4], 0x00, NVP6124_B5678_Buf,254 );
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x01,0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x06,0x40);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x1E,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x35,vformat[ch]==PAL?0x17:0x15);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x7A,vformat[ch]==PAL?0x00:0x01);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x7B,vformat[ch]==PAL?0x00:0x81);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA1,vformat[ch]==PAL?0x10:0x30);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA2,vformat[ch]==PAL?0x0E:0x0C);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA3,vformat[ch]==PAL?0x70:0x50);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xA8,vformat[ch]==PAL?0x40:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAC,vformat[ch]==PAL?0x10:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAD,vformat[ch]==PAL?0x08:0x20);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xAE,vformat[ch]==PAL?0x04:0x14);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x2A,0x72);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x2B,0xA8);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x58,0x13);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x59,0x01);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC0, 0x16);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC1, 0x13);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC8, 0x04);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x47, 0xEE);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50, 0xC4);
					}
					else
					{
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC0,0x17);
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xC1,0x14);
					}
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xD8,0x10);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xD9,0x1F);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDA,0x2B);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDB,0x7F);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDC,0xFF);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDD,0xFF);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xDE,0xFF);

					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x24,vformat[ch]==PAL?0x2A:0x1A);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50,vformat[ch]==PAL?0x84:0x86);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xBB,vformat[ch]==PAL?0x00:0xE4);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x09);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x40+(ch%4), 0x00);
					if(chip_id[ch/4] == NVP6124B_R0_ID)
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44, 0x00);
					else
						gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x44,gpio_i2c_read(nvp6124_slave_addr[ch/4], 0x44)|(1<<(ch%4)));
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x50+4*(ch%4),vformat[ch]==PAL?pn_value_fhd_pal[0]:pn_value_fhd_nt[0]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x51+4*(ch%4),vformat[ch]==PAL?pn_value_fhd_pal[1]:pn_value_fhd_nt[1]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x52+4*(ch%4),vformat[ch]==PAL?pn_value_fhd_pal[2]:pn_value_fhd_nt[2]);
					gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x53+4*(ch%4),vformat[ch]==PAL?pn_value_fhd_pal[3]:pn_value_fhd_nt[3]);
					printk("ch %d setted to 1080P %s\n", ch, vformat[ch]==PAL?"PAL":"NTSC");
				break;
				default:
					printk("ch%d wrong mode detected!!!\n", ch);
					break;
			}
			ch_mode_status[ch] = chmode[ch];
			ch_vfmt_status[ch] = vformat[ch];
			if(chip_id[ch/4] == NVP6124B_R0_ID)
			{
				nvp6124b_syncchange(ch, pvmode);
				//TODO
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x09);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x64, 0x18);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x65, 0xC2);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x66, 0x01);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x67, 0x1E);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x68, 0x02);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x69, 0x64);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x6A, 0x60);
				gpio_i2c_write(nvp6124_slave_addr[ch/4], 0x6B, 0x3C);
			}
			//init_acp(ch);
			acp_each_setting(ch);
		}
	}
}


/*
chip:chip select[0,1,2,3];
portsel: port select[0,1,2,3];
portmode: port mode select[1mux,2mux,4mux]
chid:  channel id, 1mux[0,1,2,3], 2mux[0,1], 4mux[0]
*/
void nvp6124_set_portmode(unsigned char chip, unsigned char portsel, unsigned char portmode, unsigned char chid)
{
	unsigned char tmp=0;
	unsigned char chipaddr = nvp6124_slave_addr[chip];
	if(portmode == NVP6124_OUTMODE_2MUX_FHD || portmode == NVP6124_OUTMODE_4MUX_HD)
	{
		if(chipon297MHz[chip] == 0)
			nvp6124_set_clockmode(chip, 1);
	}
	else
	{
		if(chipon297MHz[chip] == 1)
			nvp6124_set_clockmode(chip, 0);
	}
	switch(portmode)
	{
		case NVP6124_OUTMODE_1MUX_SD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);    //打开端口[3:0]和时钟[7:4],和硬件相关。
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x36);    //时钟频率设置
		break;
		case NVP6124_OUTMODE_1MUX_HD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
		break;
		case NVP6124_OUTMODE_1MUX_HD5060:
		case NVP6124_OUTMODE_1MUX_FHD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, (chid<<4)|chid);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, (chid<<4)|chid);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);
			break;
		case NVP6124_OUTMODE_2MUX_SD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			break;
		case NVP6124_OUTMODE_2MUX_HD_X:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x10:0x01);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			break;
		case NVP6124_OUTMODE_2MUX_HD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);
			break;
		case NVP6124_OUTMODE_2MUX_FHD_X:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x10:0x01);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);
			break;
		case NVP6124_OUTMODE_4MUX_SD:
		case NVP6124_OUTMODE_4MUX_HD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x80:0x08);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);
			break;
		case NVP6124_OUTMODE_4MUX_HD_X:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x32);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, 0x10);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, 0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x30:0x03);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x66);
			break;
		case NVP6124_OUTMODE_2MUX_FHD:
			gpio_i2c_write(chipaddr, 0xFF, 0x00);
			gpio_i2c_write(chipaddr, 0x56, 0x10);
			gpio_i2c_write(chipaddr, 0xFF, 0x01);
			gpio_i2c_write(chipaddr, 0x88+chid*2, 0x88);
			gpio_i2c_write(chipaddr, 0x89+chid*2, 0x88);
			gpio_i2c_write(chipaddr, 0x8C+chid*2, 0x42);
			gpio_i2c_write(chipaddr, 0x8D+chid*2, 0x42);
			gpio_i2c_write(chipaddr, 0xC0+portsel*2, chid==0?0x10:0x32);
			gpio_i2c_write(chipaddr, 0xC1+portsel*2, chid==0?0x10:0x32);
			tmp = gpio_i2c_read(chipaddr, 0xC8+portsel/2) & (portsel%2?0x0F:0xF0);
			tmp |= (portsel%2?0x20:0x02);
			gpio_i2c_write(chipaddr, 0xC8+portsel/2, tmp);
			tmp = gpio_i2c_read(chipaddr, 0xCA);
			tmp |= ((0x01<<(portsel+4)) | (0x01<<(portsel)));
			gpio_i2c_write(chipaddr, 0xCA, tmp);
			gpio_i2c_write(chipaddr, 0xCC+portsel, 0x46);
			break;
		default:
			printk("portmode %d not supported yet\n", portmode);
			break;

  	}
	printk("nvp6124_set_portmode portsel %d portmode %d setting\n", portsel, portmode);
}


int nvp6124_set_chnmode(unsigned char ch, unsigned char vfmt, unsigned char chnmode)
{
	nvp6124_video_mode vmode;
	unsigned int i;
	if((ch >= nvp6124_cnt*4) || chnmode >= NVP6124_VI_BUTT)
	{
		printk("channel/mode is out of range\n");
		return -1;
	}
	for(i=0;i<nvp6124_cnt*4;i++)
	{
		if(ch == i)
		{
			vmode.vformat[i] = vfmt;
			vmode.chmode[i] = chnmode;
		}
		else
			vmode.chmode[i] = NVP6124_VI_BUTT;
	}
	nvp6124_each_mode_setting(&vmode);
	return 0;
}

void nvp6124_video_set_contrast(unsigned int ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(value >= 100)
	{
		if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl[v_format]+value-100));
		else if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_720P_5060 || ch_mode_status[ch] == NVP6124_VI_HDEX)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_720P[v_format]+value-100));
		else //cvbs
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_960H[v_format]+value-100));
	}
	else
	{
		if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl[v_format]+(0xff-(98-value))));
		else if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_720P_5060)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_720P[v_format]+(0xff-(98-value))));
		else
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x10+(ch%4)),(nvp6124_con_tbl_960H[v_format]+(0xff-(98-value))));
	}
}

void nvp6124_video_set_brightness(unsigned int ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(value >= 100)
	{
		if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl[v_format]+value-100));
		else if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_720P_5060 || ch_mode_status[ch] == NVP6124_VI_HDEX)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_720P[v_format]+value-100));
		else
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_960H[v_format]+value-100));
	}
	else
	{
		if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl[v_format]+(0xff-(98-value))));
		else if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_720P_5060 || ch_mode_status[ch] == NVP6124_VI_HDEX)
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_720P[v_format]+(0xff-(98-value))));
		else
			gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x0c+(ch%4)),(nvp6124_bri_tbl_960H[v_format]+(0xff-(98-value))));
	}
}

void nvp6124_video_set_saturation(unsigned int ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x3c+(ch%4)),(nvp6124_sat_tbl[v_format]+value-100));
	else if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_720P_5060 || ch_mode_status[ch] == NVP6124_VI_HDEX)
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x3c+(ch%4)),(nvp6124_sat_tbl_720P[v_format]+value-100));
	else
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x3c+(ch%4)),(nvp6124_sat_tbl_960H[v_format]+value-100));
}

void nvp6124_video_set_hue(unsigned int ch, unsigned int value, unsigned int v_format)
{
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	if(ch_mode_status[ch] == NVP6124_VI_1080P_2530)
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x40+(ch%4)), (nvp6124_hue_tbl[v_format]+value-100));
	else if(ch_mode_status[ch] == NVP6124_VI_720P_2530 || ch_mode_status[ch] == NVP6124_VI_720P_5060 || ch_mode_status[ch] == NVP6124_VI_HDEX)
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x40+(ch%4)), (nvp6124_hue_tbl_720P[v_format]+value-100));
	else
		gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x40+(ch%4)), (nvp6124_hue_tbl_960H[v_format]+value-100));
}

void nvp6124_video_set_sharpness(unsigned int ch, unsigned int value)
{
	gpio_i2c_write(nvp6124_slave_addr[ch/4], 0xFF, 0x00);
	gpio_i2c_write(nvp6124_slave_addr[ch/4], (0x14+(ch%4)), (0x90+value-100));
}

void nvp6124_write_table(unsigned char chip_addr, unsigned char addr, unsigned char *tbl_ptr, unsigned char tbl_cnt)
{
	unsigned char i = 0;

	for(i = 0; i < tbl_cnt; i ++)
	{
		gpio_i2c_write(chip_addr, (addr + i), *(tbl_ptr + i));
	}
}

