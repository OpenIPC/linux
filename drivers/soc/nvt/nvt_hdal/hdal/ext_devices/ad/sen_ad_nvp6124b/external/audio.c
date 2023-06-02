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
#include "common.h"

#define AIG_DEF   0x0F
#define AOG_DEF   0x03
/*
param:
xxmaster:0[slave],1[master];
ch_num: audio channel number
samplerate: 0[8k], 1[16k]
bits: 0[16bits], 1[8bits]
*/
extern int chip_id[4];
extern unsigned int nvp6124_cnt;
extern unsigned int nvp6124_slave_addr[4];
void audio_init(unsigned char recmaster, unsigned char pbmaster, unsigned char ch_num, unsigned char samplerate, unsigned char bits)
{
	unsigned int i;
	for(i=0;i<nvp6124_cnt;i++)
	{
		gpio_i2c_write(nvp6124_slave_addr[i], 0xff, 0x01);

		if(i == 0)
		{
			gpio_i2c_write(nvp6124_slave_addr[i], 0x06, 0x1a);
			if(recmaster == 1)
			{
				gpio_i2c_write(nvp6124_slave_addr[i], 0x07, 0x80|(samplerate<<3)|(bits<<2));	//i2s rec: master
				gpio_i2c_write(nvp6124_slave_addr[i], 0x39, 0x01);
			}
			else
			{
				gpio_i2c_write(nvp6124_slave_addr[i], 0x07, 0x00|(samplerate<<3)|(bits<<2));	//i2s rec: slave
				gpio_i2c_write(nvp6124_slave_addr[i], 0x39, 0x81);
			}
			if(pbmaster == 1)
			{
				gpio_i2c_write(nvp6124_slave_addr[i], 0x13, 0x80|(samplerate<<3)|(bits<<2));	// PB I2C 8k 16bit : master
				gpio_i2c_write(nvp6124_slave_addr[i], 0xd5, 0x00);
			}
			else
			{
				gpio_i2c_write(nvp6124_slave_addr[i], 0x13, 0x00|(samplerate<<3)|(bits<<2));	// PB I2C 8k 16bit : slave
				gpio_i2c_write(nvp6124_slave_addr[i], 0xd5, 0x01);
			}

			if(8 == ch_num)
			{
				gpio_i2c_write(nvp6124_slave_addr[i], 0x06, 0x1b);
				gpio_i2c_write(nvp6124_slave_addr[i], 0x08, 0x02);
				gpio_i2c_write(nvp6124_slave_addr[i], 0x0f, 0x54);    //set I2S right sequence
				gpio_i2c_write(nvp6124_slave_addr[i], 0x10, 0x76);
			}
			else if(4 == ch_num)
			{
				gpio_i2c_write(nvp6124_slave_addr[i], 0x06, 0x1b);
				gpio_i2c_write(nvp6124_slave_addr[i], 0x08, 0x01);
				gpio_i2c_write(nvp6124_slave_addr[i], 0x0f, 0x32);   //set I2S right sequence
			}

			gpio_i2c_write(nvp6124_slave_addr[i], 0x23, 0x10);  // Audio playback out
			//gpio_i2c_write(nvp6124_slave_addr[i], 0x23, 0x18);  // Audio mix out | for AD loopback test
		}
		else
		{
			gpio_i2c_write(nvp6124_slave_addr[i], 0x06, 0x19);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x07, 0x80|(samplerate<<3)|(bits<<2));	//Rec I2C 16K 16bit : master
			gpio_i2c_write(nvp6124_slave_addr[i], 0x13, 0x00|(samplerate<<3)|(bits<<2));	// PB I2C 8k 16bit : slave
			gpio_i2c_write(nvp6124_slave_addr[i], 0x23, 0x10);  // Audio playback out
			gpio_i2c_write(nvp6124_slave_addr[i], 0xd5, 0x01);
		}
		gpio_i2c_write(nvp6124_slave_addr[i], 0x01, AIG_DEF);  // ch1 Audio input gain init
		gpio_i2c_write(nvp6124_slave_addr[i], 0x02, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x03, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x04, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x05, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x40, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x41, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x42, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x43, AIG_DEF);
		gpio_i2c_write(nvp6124_slave_addr[i], 0x22, AOG_DEF);

		gpio_i2c_write(nvp6124_slave_addr[i], 0x24, 0x14); //set mic_1's data to i2s_sp left channel
		gpio_i2c_write(nvp6124_slave_addr[i], 0x25, 0x15); //set mic_2's data to i2s_sp right channel
		if(chip_id[i] == NVP6124B_R0_ID)
		{
			gpio_i2c_write(nvp6124_slave_addr[i], 0x44, 0x11);  //audio LPF to reduce noise
			gpio_i2c_write(nvp6124_slave_addr[i], 0x94, 0x50);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x18);
			msleep(20);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x08);
		}
		else
		{
			#ifdef _INPUT_27MHZ_CLKSOURCE //for test yet
			gpio_i2c_write(nvp6124_slave_addr[i], 0x94, 0x20);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x32, 0x20);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x34, 0x01);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x35, 0x69);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x36, 0x99);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x18);
			msleep(20);
			gpio_i2c_write(nvp6124_slave_addr[i], 0x38, 0x08);
			#endif
		}

		printk("nvp6124 audio init,REC:%s,PB:%s\n", recmaster==1?"MASTER":"SLAVE", pbmaster==1?"MASTER":"SLAVE");
	}
}

