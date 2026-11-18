/*
 * cst3xx Touchscreen Controller Driver
 */

#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/pm_qos.h>
#include <linux/slab.h>
#include <linux/types.h>
#include "cst3xx.h"
#include "hyn_cst3xx_fw.h"

struct st1615_ts_finger {
	u16 x;
	u16 y;
	u8 t;
	bool is_valid;
};

struct cst3xx_ts_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct cst3xx_platform_data *pdata;
	struct work_struct  work;
	struct delayed_work dwork;
	struct workqueue_struct *workqueue;
	int irq;
	unsigned int_gpio;
	unsigned reset_gpio;
	unsigned en_gpio;
};

static int cst3xx_into_program_mode(struct i2c_client * client);
static int cst3xx_erase_program_area(struct i2c_client * client);
static void cst3xx_reset_ic(struct cst3xx_ts_data *ts,unsigned int ms);
static int cst3xx_write_program_data(struct i2c_client * client,const unsigned char *pdata);

static int cst3xx_check_checksum(struct i2c_client * client);

static int cst3xx_exit_program_mode(struct i2c_client * client);

static int cst3xx_update_firmware(struct cst3xx_ts_data *ts, const unsigned char *pdata);

static int cst3xx_firmware_info(struct i2c_client * client);

static int cst3xx_update_judge( unsigned char *pdata, int strict);

static int cst3xx_boot_update_fw(struct cst3xx_ts_data *ts,  unsigned char *pdata);

static int cst3xx_i2c_read(struct i2c_client *client, unsigned char *buf, int len);
static int cst3xx_i2c_read_register(struct i2c_client *client, unsigned char *buf, int len);
static int cst3xx_i2c_write(struct i2c_client *client, unsigned char *buf, int len);
static int cst3xx_i2c_test(struct i2c_client *client);
static void cst3xx_reset(struct cst3xx_ts_data *ts) ;
extern  void close_vcc3v3Power(void);
extern  void open_vcc3v3Power(void);

static unsigned char *pcst3xx_update_firmware = (unsigned char *)cst3_fw ; //the updating firmware
static unsigned int   g_cst3xx_ic_version  = 0;
static unsigned int   g_cst3xx_ic_checksum = 0;
static unsigned int   g_cst3xx_ic_checkcode =0;
static unsigned int   g_cst3xx_ic_project_id  = 0;
static unsigned int   g_cst3xx_ic_type  = 0;

static unsigned char  report_flag = 0;
static unsigned int x_temp_pos = 0;
static unsigned int y_temp_pos = 0;

#define HYN_DRAW_POINT

//#define USE_ANALOG_I2C     1 //i2c analog
//#define USE_HIGHEST_I2CA  1// x1000 highest i2c
#define USE_LOWEST_I2CA   1 // x1000 lowest i2c
//#define HYN_UPDATE_FIRMWARE_POWERON_ENABLE
//#define HYN_UPDATE_FIRMWARE_ENABLE

#pragma pack(1)
typedef struct
{
	u16 pid;                 //product id   //
	u16 vid;                 //version id   //
} st_tpd_info;
#pragma pack()

st_tpd_info tpd_info;

#define CST3XX_BIN_SIZE    (31*512 + 480)
#define CST3XX_MAX_X  340
#define CST3XX_MAX_Y  800


static void cst3xx_ts_power(struct cst3xx_ts_data *ts, bool poweron)
{
	if(gpio_is_valid(ts->en_gpio))	{
		if (!poweron)
			gpio_direction_output(ts->en_gpio, 1);
		else{
			gpio_direction_output(ts->en_gpio, 0);
			mdelay(100);
			gpio_direction_output(ts->reset_gpio, poweron);
		}
	}
	else {
		if (gpio_is_valid(ts->reset_gpio))
			gpio_direction_output(ts->reset_gpio, poweron);
		}
}

static int cst3xx_into_program_mode(struct i2c_client * client)
{
	int ret;
	unsigned char buf[4];

	buf[0] = 0xA0;
	buf[1] = 0x01;
	buf[2] = 0xAA;	//set cmd to enter program mode
	ret = cst3xx_i2c_write(client, buf, 3);
	if (ret < 0)  return -1;

	mdelay(2);

	buf[0] = 0xA0;
	buf[1] = 0x02;	//check whether into program mode
	ret = cst3xx_i2c_read_register(client, buf, 1);
	if (ret < 0)  return -1;

	if (buf[0] != 0x55) return -1;

	return 0;
}

static int cst3xx_erase_program_area(struct i2c_client * client)
{
	int ret;
	unsigned char buf[3];

	buf[0] = 0xA0;
	buf[1] = 0x02;
	buf[2] = 0x00;		//set cmd to erase main area
	ret = cst3xx_i2c_write(client, buf, 3);
	if (ret < 0) return -1;

	mdelay(5);

	buf[0] = 0xA0;
	buf[1] = 0x03;
	ret = cst3xx_i2c_read_register(client, buf, 1);
	if (ret < 0)  return -1;

	if (buf[0] != 0x55) return -1;

	return 0;
}

static void cst3xx_reset_ic(struct cst3xx_ts_data *ts,unsigned int ms)
{
#if 0
	unsigned char buf[4];
	buf[0] = 0xD1;
	buf[1] = 0x0E;
	cst3xx_i2c_write(ts->client, buf, 2);
#else
	gpio_direction_output(ts->reset_gpio,0);
	mdelay(2);
	gpio_direction_output(ts->reset_gpio,1);
#endif
	mdelay(ms);
}

static int cst3xx_write_program_data(struct i2c_client * client,
		const unsigned char *pdata)
{
	int i, ret;
	unsigned char *i2c_buf;
	unsigned short eep_addr;
	int total_kbyte;

	unsigned char temp_buf[8];
	unsigned short iic_addr;
	int  j;

	i2c_buf = kmalloc(sizeof(unsigned char)*(1024 + 2), GFP_KERNEL);
	if (i2c_buf == NULL)
		return -1;

	//make sure fwbin len is N*1K
	//total_kbyte = len / 1024;
	total_kbyte = 24;
	for (i=0; i<total_kbyte; i++) {
		i2c_buf[0] = 0xA0;
		i2c_buf[1] = 0x14;
		eep_addr = i << 10;		//i * 1024
		i2c_buf[2] = eep_addr;
		i2c_buf[3] = eep_addr>>8;
		ret = cst3xx_i2c_write(client, i2c_buf, 4);
		if (ret < 0)
			goto error_out;

		memcpy(i2c_buf, pdata + eep_addr, 1024);
		for(j=0; j<256; j++) {
			iic_addr = (j<<2);
			temp_buf[0] = (iic_addr+0xA018)>>8;
			temp_buf[1] = (iic_addr+0xA018)&0xFF;
			temp_buf[2] = i2c_buf[iic_addr+0];
			temp_buf[3] = i2c_buf[iic_addr+1];
			temp_buf[4] = i2c_buf[iic_addr+2];
			temp_buf[5] = i2c_buf[iic_addr+3];
			ret = cst3xx_i2c_write(client, temp_buf, 6);
			if (ret < 0)
				goto error_out;
		}

		i2c_buf[0] = 0xA0;
		i2c_buf[1] = 0x04;
		i2c_buf[2] = 0xEE;
		ret = cst3xx_i2c_write(client, i2c_buf, 3);
		if (ret < 0)
			goto error_out;

		mdelay(60);

		i2c_buf[0] = 0xA0;
		i2c_buf[1] = 0x05;
		ret = cst3xx_i2c_read_register(client, i2c_buf, 1);
		if (ret < 0)
			goto error_out;

		if (i2c_buf[0] != 0x55)
			goto error_out;
	}

	i2c_buf[0] = 0xA0;
	i2c_buf[1] = 0x03;
	i2c_buf[2] = 0x00;
	ret = cst3xx_i2c_write(client, i2c_buf, 3);
	if (ret < 0)
		goto error_out;

	mdelay(8);

	if (i2c_buf != NULL) {
		kfree(i2c_buf);
		i2c_buf = NULL;
	}

	return 0;

error_out:
	if (i2c_buf != NULL) {
		kfree(i2c_buf);
		i2c_buf = NULL;
	}
	return -1;
}

static int cst3xx_check_checksum(struct i2c_client * client)
{
	int ret;
	int i;
	unsigned int  checksum;
	unsigned int  bin_checksum;
	unsigned char buf[4];
	const unsigned char *pData;

	for(i=0; i<5; i++)
	{
		buf[0] = 0xA0;
		buf[1] = 0x00;
		ret = cst3xx_i2c_read_register(client, buf, 1);
		if(ret < 0)
		{
			mdelay(2);
			continue;
		}

		if(buf[0]!=0)
			break;
		else
			mdelay(2);
	}
	mdelay(2);

	if(buf[0]==0x01)
	{
		buf[0] = 0xA0;
		buf[1] = 0x08;
		ret = cst3xx_i2c_read_register(client, buf, 4);

		if(ret < 0)	return -1;

		// read chip checksum
		checksum = buf[0] + (buf[1]<<8) + (buf[2]<<16) + (buf[3]<<24);

		pData=(unsigned char  *)pcst3xx_update_firmware +24*1024+16;   //7*1024 +512
		bin_checksum = pData[0] + (pData[1]<<8) + (pData[2]<<16) + (pData[3]<<24);

		printk("  hyn the updated ic checksum is :0x%x. the updating firmware checksum is:0x%x------\n", checksum, bin_checksum);

		if(checksum!=bin_checksum)
		{
			printk(" cst3xx hyn check sum error.\n");
			return -1;
		}
	}
	else
	{
		printk(" cst3xx hyn No checksum.\n");
		return -1;
	}
	return 0;
}

static int cst3xx_exit_program_mode(struct i2c_client * client)
{
	int ret;
	unsigned char buf[3];

	buf[0] = 0xA0;
	buf[1] = 0x06;
	buf[2] = 0xEE;
	ret = cst3xx_i2c_write(client, buf, 3);
	if (ret < 0)
		return -1;

	mdelay(10);	//wait for restart

	return 0;
}

static int cst3xx_update_firmware(struct cst3xx_ts_data *ts, const unsigned char *pdata)
{
	int ret;
	int retry = 0;

	printk(" cst3xx----------upgrade cst3xx begain------------\n");
	disable_irq(ts->irq);
	mdelay(20);
START_FLOW:

#ifdef HYN_UPDATE_FIRMWARE_POWERON_ENABLE
	close_vcc3v3Power();
	mdelay(20);
	open_vcc3v3Power();
	mdelay(5+retry);
#else
	cst3xx_reset_ic(ts,5+retry);
#endif
	ret = cst3xx_into_program_mode(ts->client);
	if (ret < 0) {
		printk(" cst3xx[cst3xx]into program mode failed.\n");
		goto err_out;
	}

	ret = cst3xx_erase_program_area(ts->client);
	if (ret) {
		printk(" cst3xx[cst3xx]erase main area failed.\n");
		goto err_out;
	}

	ret = cst3xx_write_program_data(ts->client, pdata);
	if (ret < 0) {
		printk(" cst3xx[cst3xx]write program data into cstxxx failed.\n");
		goto err_out;
	}

	ret =cst3xx_check_checksum(ts->client);
	if (ret < 0) {
		printk(" cst3xx[cst3xx] after write program cst3xx_check_checksum failed.\n");
		goto err_out;
	}

	ret = cst3xx_exit_program_mode(ts->client);
	if (ret < 0) {
		printk(" cst3xx[cst3xx]exit program mode failed.\n");
		goto err_out;
	}

	cst3xx_reset_ic(ts,20);

	printk(" cst3xx hyn----------cst3xx_update_firmware  end------------\n");

	enable_irq(ts->irq);

	return 0;

err_out:
	if (retry < 30) {
		retry++;
		mdelay(20);
		goto START_FLOW;
	}
	else {
		enable_irq(ts->irq);
		return -1;
	}
}

/*******************************************************
Function:
    get firmware version, ic type...
Input:
    client: i2c client
Output:
    success: 0
    fail:	-1
*******************************************************/
static int cst3xx_firmware_info(struct i2c_client * client)
{
	int ret;
	unsigned char buf[28];
//	unsigned short ic_type, project_id;

	buf[0] = 0xD1;
	buf[1] = 0x01;
	ret = cst3xx_i2c_write(client, buf, 2);
	if (ret < 0) return -1;

	mdelay(40);

	buf[0] = 0xD1;
	buf[1] = 0xFC;
	ret = cst3xx_i2c_read_register(client, buf, 4);
	if (ret < 0) return -1;

	//0xCACA0000
	g_cst3xx_ic_checkcode = buf[3];
	g_cst3xx_ic_checkcode <<= 8;
	g_cst3xx_ic_checkcode |= buf[2];
	g_cst3xx_ic_checkcode <<= 8;
	g_cst3xx_ic_checkcode |= buf[1];
	g_cst3xx_ic_checkcode <<= 8;
	g_cst3xx_ic_checkcode |= buf[0];

	printk("linc cst3xx [cst3xx] the chip g_cst3xx_ic_checkcode:0x%x.\r\n",g_cst3xx_ic_checkcode);

	mdelay(2);

	buf[0] = 0xD2;
	buf[1] = 0x04;
	ret = cst3xx_i2c_read_register(client, buf, 4);
	if (ret < 0) return -1;
	g_cst3xx_ic_type = buf[3];
	g_cst3xx_ic_type <<= 8;
	g_cst3xx_ic_type |= buf[2];

	g_cst3xx_ic_project_id = buf[1];
	g_cst3xx_ic_project_id <<= 8;
	g_cst3xx_ic_project_id |= buf[0];

	printk("linc cst3xx [cst3xx] the chip ic g_cst3xx_ic_type :0x%x, g_cst3xx_ic_project_id:0x%x\r\n",
		g_cst3xx_ic_type, g_cst3xx_ic_project_id);

	mdelay(2);

	buf[0] = 0xD2;
	buf[1] = 0x08;
	ret = cst3xx_i2c_read_register(client, buf, 8);
	if (ret < 0) return -1;

	g_cst3xx_ic_version = buf[3];
	g_cst3xx_ic_version <<= 8;
	g_cst3xx_ic_version |= buf[2];
	g_cst3xx_ic_version <<= 8;
	g_cst3xx_ic_version |= buf[1];
	g_cst3xx_ic_version <<= 8;
	g_cst3xx_ic_version |= buf[0];

	g_cst3xx_ic_checksum = buf[7];
	g_cst3xx_ic_checksum <<= 8;
	g_cst3xx_ic_checksum |= buf[6];
	g_cst3xx_ic_checksum <<= 8;
	g_cst3xx_ic_checksum |= buf[5];
	g_cst3xx_ic_checksum <<= 8;
	g_cst3xx_ic_checksum |= buf[4];

	tpd_info.vid = g_cst3xx_ic_version;
	tpd_info.pid = 0x00;

	printk(" cst3xx [cst3xx] the chip ic version:0x%x, checksum:0x%x\r\n",
		g_cst3xx_ic_version, g_cst3xx_ic_checksum);

	if(g_cst3xx_ic_version==0xA5A5A5A5)
	{
		printk(" cst3xx [cst3xx] the chip ic don't have firmware. \n");
		return -1;
	}
	if((g_cst3xx_ic_checkcode&0xffff0000)!=0xCACA0000){
		printk("linc cst3xx [cst3xx] cst3xx_firmware_info read error .\r\n");
		return -1;
	}

	buf[0] = 0xD1;
	buf[1] = 0x09;
	ret = cst3xx_i2c_write(client, buf, 2);
	if (ret < 0) return -1;
		mdelay(5);

	return 0;
}

static int cst3xx_update_judge( unsigned char *pdata, int strict)
{
	unsigned short ic_type, project_id;
	unsigned int fw_checksum, fw_version;
	const unsigned int *p;
	int i;
	unsigned char *pBuf;

	fw_checksum = 0x55;
	p = (const unsigned int *)pdata;
	for (i=0; i<(CST3XX_BIN_SIZE-4); i+=4) {
		fw_checksum += (*p);
		p++;
	}

	if (fw_checksum != (*p)) {
		printk(" cst3xx[cst3xx]calculated checksum error:0x%x not equal 0x%x.\n", fw_checksum, *p);
		return -1;	//bad fw, so do not update
	}

	pBuf = &pdata[CST3XX_BIN_SIZE-16];

	project_id = pBuf[1];
	project_id <<= 8;
	project_id |= pBuf[0];

	ic_type = pBuf[3];
	ic_type <<= 8;
	ic_type |= pBuf[2];

	fw_version = pBuf[7];
	fw_version <<= 8;
	fw_version |= pBuf[6];
	fw_version <<= 8;
	fw_version |= pBuf[5];
	fw_version <<= 8;
	fw_version |= pBuf[4];

	fw_checksum = pBuf[11];
	fw_checksum <<= 8;
	fw_checksum |= pBuf[10];
	fw_checksum <<= 8;
	fw_checksum |= pBuf[9];
	fw_checksum <<= 8;
	fw_checksum |= pBuf[8];

	printk(" cst3xx[cst3xx]the updating firmware:project_id:0x%04x,ic type:0x%04x,version:0x%x,checksum:0x%x\n",
		project_id, ic_type, fw_version, fw_checksum);

	if (strict > 0) {
		if (g_cst3xx_ic_checksum != fw_checksum){
			if (g_cst3xx_ic_version >fw_version){
				printk("[cst3xx]fw version(0x%x), ic version(0x%x).\n",fw_version, g_cst3xx_ic_version);
				return -1;
			}
		}else{
			printk("[cst3xx]fw checksum(0x%x), ic checksum(0x%x).\n",fw_checksum, g_cst3xx_ic_checksum);
			return -1;
		}
	}

	return 0;
}

static int cst3xx_boot_update_fw(struct cst3xx_ts_data *ts,  unsigned char *pdata)
{
	int ret;
	int retry = 0;
	int flag = 0;

	while (retry++ < 3) {
		ret = cst3xx_firmware_info(ts->client);
		if (ret == 0) {
			flag = 1;
			break;
		}
	}

	if (flag == 1) {
		ret = cst3xx_update_judge(pdata, 1);
		if (ret < 0) {
			printk(" cst3xx[cst3xx] no need to update firmware.\n");
			return 0;
		}
	}

	ret = cst3xx_update_firmware(ts, pdata);
	if (ret < 0){
		printk(" cst3xx [cst3xx] update firmware failed.\n");
		return -1;
	}

	mdelay(50);

	ret = cst3xx_firmware_info(ts->client);
	if (ret < 0) {
		printk(" cst3xx [cst3xx] after update read version and checksum fail.\n");
		return -1;
	}

	return 0;
}

/*******************************Chose which I2C interface ********************************************/

#if USE_ANALOG_I2C

#define I2CSDA  GPIO_PC(27)
#define I2CSCL   GPIO_PC(26)

#define I2CSDA_HIGH gpio_direction_output(GPIO_PC(27),1)
#define I2CSDA_LOW  gpio_direction_output(GPIO_PC(27),0)

#define I2CSCL_HIGH  gpio_direction_output(GPIO_PC(26),1)
#define I2CSCL_LOW  gpio_direction_output(GPIO_PC(26),0)

#define delay_data 2
static void iicstart(void)
{
     I2CSDA_HIGH;
      I2CSCL_HIGH;
      __udelay(2);
      I2CSDA_LOW;
      __udelay(2);
       I2CSCL_LOW;
       __udelay(2);
}
static void iicstop(void)
{
     I2CSDA_LOW;
     I2CSCL_HIGH;
     __udelay(2);
     I2CSDA_HIGH;
     __udelay(2);

}
static unsigned char waitack(void)
{
   u8 ucErrTime=0;

    gpio_direction_input(GPIO_PC(27));
   //  __udelay(2);

  //  I2CSDA_HIGH;
  //  __udelay(2);
     I2CSCL_HIGH;
     __udelay(2);

    while(__gpio_get_value(GPIO_PC(27)))
    {
        ucErrTime++;
     //   printk("time out ++ \n");
        if(ucErrTime>250)
         {
                iicstop();
                return 1;
        }
    }
    I2CSCL_LOW;
     __udelay(2);
    return 0;
}
static void sendack(void)
{
     I2CSCL_LOW;
     __udelay(delay_data);
     I2CSDA_LOW;
     __udelay(delay_data);

     I2CSCL_HIGH;
     __udelay(delay_data);
     I2CSCL_LOW;
     __udelay(delay_data);

}
static void sendnotack(void)
{
     I2CSCL_LOW;
     __udelay(delay_data);
    I2CSDA_HIGH;
    __udelay(delay_data);

    I2CSCL_HIGH;
    __udelay(delay_data);
    I2CSCL_LOW;
    __udelay(delay_data);
}
static void iicsendbyte(unsigned char ch)
{
     unsigned char i;
     I2CSCL_LOW;
     for(i = 0;i<8;i++)
     {
          if(ch&0x80)
          {
              I2CSDA_HIGH;
          }
          else
          {
              I2CSDA_LOW;
          }
           __udelay(delay_data);
          ch<<=1;
          I2CSCL_HIGH;
          __udelay(delay_data);
               I2CSCL_LOW;
          __udelay(delay_data);
     }
}
static unsigned char iicreceivebyte(unsigned char ack)
{
    unsigned char i=8;
    unsigned char ddata=0;

    gpio_direction_input(GPIO_PC(27));
    __udelay(delay_data);

     for(i = 0;i<8;i++)
     {
         I2CSCL_LOW;
         __udelay(delay_data);
         I2CSCL_HIGH;
         __udelay(delay_data);

         ddata<<=1;
         if(gpio_get_value(GPIO_PC(27))) ddata++;
    }

    if(!ack)
        sendnotack();
     else
        sendack();

    return ddata;
}
static int cst3xx_i2c_read(struct i2c_client *client, unsigned char *buf, int len)
{
    int count = len;
     iicstart();
     __udelay(delay_data);

/*
	 iicsendbyte( (0x1a<<1) | 0);
     __udelay(delay_data);

     if(waitack())
     {
        iicstop();
                printk(" cst3xx_i2c_read wait ack timeoue \n");
        return 1;
     }

     iicsendbyte(buf[0]);
     __udelay(delay_data);
     waitack();
     __udelay(delay_data);
*/
     iicstart();
     __udelay(delay_data);

     iicsendbyte((0x1a << 1 ) | 1);
     __udelay(delay_data);
     if(waitack())
     {
        iicstop();
                printk(" cst3xx_i2c_read wait ack timeoue \n");
        return 1;
     }
     __udelay(delay_data);

    while(count)
    {
         if(count == 1) *buf = iicreceivebyte(0);
         else *buf = iicreceivebyte(1);
         count --;
         buf++;
    }
     iicstop();
     __udelay(delay_data);
     return 0;
}

static int cst3xx_i2c_write(struct i2c_client *client, unsigned char *buf, int len)
{
     int count = len;
      iicstart();
   //   __udelay(delay_data);

      iicsendbyte((0x1A << 1) | 0);
//      __udelay(delay_data);

     if(waitack())
     {
        iicstop();
        printk(" cst3xx_i2c_write send address wait ack timeoue \n");
        return 1;
     }

      while(count--)
      {
            iicsendbyte(*(buf++));
            __udelay(delay_data);
             if(waitack())
             {
                iicstop();
               printk(" cst3xx_i2c_write wait ack timeoue \n");
                return 1;
             }
            __udelay(delay_data);
      }

      iicstop();
      __udelay(delay_data);

	return 0;
}

static int cst3xx_i2c_read_register(struct i2c_client *client, unsigned char *buf, int len)
{
	int ret = -1;

	ret = cst3xx_i2c_write(client, buf, 2);
	if(ret) printk("failed to write cst3xx\n");

	ret = cst3xx_i2c_read(client, buf, len);
	if(ret) printk("failed to write cst3xx\n");
	return ret;
}

#elif  USE_LOWEST_I2CA
static int cst3xx_i2c_read(struct i2c_client *client, unsigned char *buf, int len)
{
	int ret = -1;
	int retries = 0;

	while (retries < 2) {
		ret = i2c_master_recv(client, buf, len);
		if(ret<=0)
		    retries++;
        else
            break;
	}

	return ret;
}

static int cst3xx_i2c_write(struct i2c_client *client, unsigned char *buf, int len)
{
	int ret = -1;
	int retries = 0;

	while (retries < 2) {
		ret = i2c_master_send(client, buf, len);
		if(ret<=0)
		    retries++;
        else
            break;
	}

	return ret;
}

static int cst3xx_i2c_read_register(struct i2c_client *client, unsigned char *buf, int len)
{
	int ret = -1;

    ret = cst3xx_i2c_write(client, buf, 2);

    ret = cst3xx_i2c_read(client, buf, len);

    return ret;
}

#elif USE_HIGHEST_I2CA
static int cst3xx_i2c_read(struct i2c_client *client, unsigned char *buf, int len)
{
	struct i2c_msg msg;
	int ret = -1;
	int retries = 0;

	msg.flags |= I2C_M_RD;
	msg.addr   = client->addr;
	msg.len    = len;
	msg.buf    = buf;

	while (retries < 2) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if(ret == 1)
			break;
		retries++;
	}

	return ret;
}
/*******************************************************
Function:
    read data from register.
Input:
    buf: first two byte is register addr, then read data store into buf
    len: length of data that to read
Output:
    success: number of messages
    fail:	negative errno
*******************************************************/
static int cst3xx_i2c_read_register(struct i2c_client *client, unsigned char *buf, int len)
{
	struct i2c_msg msgs[2];
	int ret = -1;
	int retries = 0;

	msgs[0].flags = 0;
	msgs[0].addr  = client->addr;
	msgs[0].len   = 2;
	msgs[0].buf   = buf;

	msgs[1].flags |= I2C_M_RD;
	msgs[1].addr   = client->addr;
	msgs[1].len    = len;
	msgs[1].buf    = buf;

	while (retries < 2) {
		ret = i2c_transfer(client->adapter, msgs, 2);
		if(ret == 2)
			break;
		retries++;
	}

	return ret;
}

static int cst3xx_i2c_write(struct i2c_client *client, unsigned char *buf, int len)
{
	struct i2c_msg msg;
	int ret = -1;
	int retries = 0;

	msg.flags = 0;
	msg.addr  = client->addr;
	msg.len   = len;
	msg.buf   = buf;

	while (retries < 2) {
		ret = i2c_transfer(client->adapter, &msg, 1);
		if(ret == 1)
			break;
		retries++;
	}

	return ret;
}
#else
#endif
/*********************************i2c end********************************/

static int cst3xx_i2c_test(struct i2c_client *client)
{
	int retry = 0;
	int ret;
	unsigned char buf[4];

	buf[0] = 0xD0;
	buf[1] = 0x00;
	while (retry++ < 5) {
		ret = cst3xx_i2c_write(client, buf, 2);
		if (ret >= 0){
			printk(" cst3xx hyn I2C TEST pass;\n");
			return ret;
		}

		mdelay(2);
	}

	if(retry>=5) printk(" cst3xx hyn I2C TEST error.ret:%d;\n", ret);

	return ret;
}

static void cst3xx_reset(struct cst3xx_ts_data *ts) {
	if(gpio_is_valid(ts->reset_gpio)) {
		gpio_direction_output(ts->reset_gpio,0);
		mdelay(100);
		gpio_direction_output(ts->reset_gpio,1);
		mdelay(100);
	}
}
/****************************  basic communication end*******************************/

static void cst3xx_touch_report(struct cst3xx_ts_data *ts)
{
	unsigned char buf[30];
	unsigned char i2c_buf[8];
	unsigned char key_status, key_id = 0, finger_id, sw;
	unsigned int  input_x = 0;
	unsigned int  input_y = 0;
	unsigned int  input_w = 0;
	unsigned int input_x_delta = 0;
	unsigned int input_y_delta = 0;
	unsigned char cnt_up, cnt_down;
	int   i, ret, idx;
	int cnt, i2c_len;

	int  len_1, len_2;

	key_status = 0;

	buf[0] = 0xD0;
	buf[1] = 0x00;
	ret = cst3xx_i2c_read_register(ts->client, buf, 7);
	if(ret < 0) {
		printk("  iic read touch point data failed.\n");
		goto OUT_PROCESS;
	}
  //   printk(" read buf : buf[0]: %x ,buf[1]: %x ,buf[2]: %x, buf[3]: %x,buf[4]: %x ,buf[5]: %x ,buf[6]: %x, buf[7]: %x,\n",buf[0],buf[1],buf[2],buf[3],buf[4],buf[5],buf[6],buf[7]);

	if(buf[6] != 0xAB) {
		printk(KERN_INFO " data is not valid..\r\n");
		goto OUT_PROCESS;
	}

    	cnt = buf[5] & 0x7F;
	if(cnt > 1) goto OUT_PROCESS;
	else if(cnt==0)     goto CLR_POINT;

	if(cnt == 0x01) {
		goto FINGER_PROCESS;
	}

FINGER_PROCESS:

    /*
     *  sw: buf[0] low 4 bit
     *  x pos:  buf[1] low 4 bit | buf[3] high 4 bit
     *  y pos:  buf[2] low 4 bit | buf[3] low  4 bit
     *  z press : buf[4]
     *  finger_id:buf[0] high 4 bit
     */
	i2c_buf[0] = 0xD0;
	i2c_buf[1] = 0x00;
	i2c_buf[2] = 0xAB;

	ret = cst3xx_i2c_write(ts->client, i2c_buf, 3);
	if(ret < 0) {
		printk(" cst3xx hyn send read touch info ending failed.\r\n");
		cst3xx_reset_ic(ts,20);
		goto END;
	}

	idx = 0;
	cnt_up = 0;
	cnt_down = 0;
	input_x = (unsigned int)((buf[idx + 1] << 4) | ((buf[idx + 3] >> 4) & 0x0F));
	input_y = (unsigned int)((buf[idx + 2] << 4) | (buf[idx + 3] & 0x0F));
	input_w = (unsigned int)(buf[idx + 4]);
	sw = (buf[idx] & 0x0F) >> 1;
	finger_id = (buf[idx] >> 4) & 0x0F;

	//printk(" no handle : cst3xxPoint x:%d, y:%d, w:%d, id:%d, sw:%d. \n", input_x, input_y, input_w,finger_id, sw);

	if (sw == 0x03 ) {
		//printk(" REPORT TOUCH DOWM Handle \n");
		x_temp_pos = input_x;
		y_temp_pos = input_y;
		input_report_key(ts->input_dev, BTN_TOUCH, 1);
    		input_report_abs(ts->input_dev, ABS_X, input_x);
		input_report_abs(ts->input_dev, ABS_Y, 960 - input_y);
    		input_report_abs(ts->input_dev, ABS_PRESSURE, 1);
    		input_sync(ts->input_dev);

		cnt_down++;
	}
	else if(sw == 0) {
                cnt_up++;
        }

        if((cnt_up>0) && (cnt_down==0) ){
		//printk(" REPORT TOUCH UP Handle \n");
		x_temp_pos = 0;
		y_temp_pos = 0;
		input_report_key(ts->input_dev, BTN_TOUCH, 0);
		input_report_abs(ts->input_dev, ABS_PRESSURE, 0);
		input_mt_sync(ts->input_dev);
	}
	input_sync(ts->input_dev);
	goto END;

CLR_POINT:
	input_report_key(ts->input_dev, BTN_TOUCH, 0);
	input_mt_sync(ts->input_dev);
	input_sync(ts->input_dev);

OUT_PROCESS:
	buf[0] = 0xD0;
	buf[1] = 0x00;
	buf[2] = 0xAB;
	ret = cst3xx_i2c_write(ts->client, buf, 3);
	if (ret < 0) {
		printk("  send read touch info ending failed.\n");
		cst3xx_reset_ic(ts,20);
	}
END:
	return;
}

/******************************* irq handle  begin************************************/
static void cst3xx_work_handler(struct work_struct *work)
{
	struct cst3xx_ts_data *cst3xx_ts = container_of(work, struct cst3xx_ts_data, work);
	int ret = 0;

	/******************here handle touch *******************/
	//printk(KERN_INFO "-----------enter cst3xx_work_handler------------\n");
	cst3xx_touch_report(cst3xx_ts);
	enable_irq(cst3xx_ts->irq);
}

static irqreturn_t cst3xx_ts_irq_handler(int irq, void *dev_id)
{
	struct cst3xx_ts_data *ts = dev_id;

	//printk( KERN_INFO "-----------enter cst3xx_ts_irq_handler------------\n");

	disable_irq_nosync(ts->irq);//use in interrupt,disable_irq will make dead lock.

	if (!work_pending(&ts->work)) {
		queue_work(ts->workqueue, &ts->work);
//		queue_delayed_work(ts->workqueue,&ts->dwork,1);//10ms * x
	} else {
		enable_irq(ts->irq);
	}

	return IRQ_HANDLED;
}
/******************************* irq handle  end************************************/

static int cst3xx_ts_probe(struct i2c_client *client,
					const struct i2c_device_id *id)
{
	printk( KERN_INFO "Enter cst3xx_ts_probe -------------------\n");

	int ret;
	struct cst3xx_ts_data *ts;
//	struct cst3xx_platform_data *pdata = (struct cst3xx_platform_data*)client->dev.platform_data;
	struct input_dev *input_dev;
	int error;

#if USE_ANALOG_I2C
	if (gpio_is_valid(I2CSDA))
	{
		gpio_free(I2CSDA);
		ret = gpio_request(GPIO_PC(27),"I2C1-SDA");
		if(ret < 0){
			printk("I2C1-SDA gpio request failed\n");
			return ret;
		}
	}
	if (gpio_is_valid(I2CSCL))
	{
		gpio_free(I2CSCL);
		ret = gpio_request(GPIO_PC(26),"I2C1-SCL");
		if(ret < 0){
			printk("I2C1-SCL gpio request failed\n");
			return ret;
		}
	}
#endif

	ts = devm_kzalloc(&client->dev, sizeof(*ts), GFP_KERNEL);
	if (!ts)
		return -ENOMEM;
	input_dev = devm_input_allocate_device(&client->dev);//input_allocate_device();
	if(!input_dev)
		return -ENOMEM;

//	ts->pdata = pdata;
	ts->client = client;
	ts->input_dev = input_dev;

	if(client->addr != 0x5A) //check address
	{
		client->addr = 0x5A;
		printk("Crystal_shen:i2c_client_HYN->addr=%d.\n",client->addr);

	}

	ts->int_gpio = of_get_named_gpio_flags(client->dev.of_node, "int-gpio", 0, NULL);
	printk("ts->int_gpio: %d\n", ts->int_gpio);
	if (gpio_is_valid(ts->int_gpio)) {
		//error = gpio_request(pdata->int_gpio,"cst3xx gpio irq");
		error = gpio_request_one(ts->int_gpio, GPIOF_IN, "cst3xx gpio irq");
		if (error < 0) {
			dev_err(&client->dev, "Failed to request GPIO %d, error %d\n",
				ts->int_gpio, error);
			return error;
		}

	/*	if (error < 0) {
			dev_err(&client->dev, "%s:failed to set gpio irq.\n",__func__);
			return error;
		}*/
		ts->irq = gpio_to_irq(ts->int_gpio);
		printk(" cst3xx ts -> irq = %d.\n",ts->irq);
	}

	ts->en_gpio = of_get_named_gpio_flags(client->dev.of_node, "en-gpio", 0, NULL);
	printk("ts->en_gpio: %d\n", ts->en_gpio);
	if (gpio_is_valid(ts->en_gpio)) {
		error = devm_gpio_request(&client->dev, ts->en_gpio, NULL);
		if (error) {
			dev_err(&client->dev,
				"Unable to request GPIO pin %d.\n",
				ts->en_gpio);
				return error;
		}
		gpio_direction_output(ts->en_gpio,0);
		mdelay(100);
	}

	ts->reset_gpio = of_get_named_gpio_flags(client->dev.of_node, "reset-gpio", 0, NULL);
	printk("ts->reset_gpio: %d\n", ts->reset_gpio);
	if (gpio_is_valid(ts->reset_gpio)) {
		error = devm_gpio_request(&client->dev, ts->reset_gpio, NULL);
		if (error) {
			dev_err(&client->dev,
				"Unable to request GPIO pin %d. error = %d\n",
				ts->reset_gpio, error);
				return error;
		}
	}
	cst3xx_reset(ts);
	printk( KERN_INFO "cst3xx reset -------------------\n");
	ret =  cst3xx_i2c_test(ts->client);
	if(ret < 0){
		printk( KERN_INFO "cst3xx hyn i2c communication failed.---------------\n");
//		return -1;
	}

	#ifdef HYN_UPDATE_FIRMWARE_ENABLE
		ret = cst3xx_boot_update_fw(ts,pcst3xx_update_firmware);//
		if(ret < 0){
		printk(" cst3xx hyn_boot_update_fw failed.\n");
		return -1;
		}
	#else
		ret = cst3xx_firmware_info(ts->client);
		if (ret < 0) {
			printk(" cst3xx [cst3xx] after update read version and checksum fail.\n");

		}
	#endif

	input_dev->name = "cst3xxx -touchscreen";
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = &client->dev;

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_ABS, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	set_bit(BTN_TOUCH, input_dev->keybit);

	set_bit(ABS_X, input_dev->absbit);
	set_bit(ABS_Y, input_dev->absbit);
	set_bit(ABS_PRESSURE, input_dev->absbit);

	input_set_abs_params(input_dev, ABS_X, 0, 960, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, 480, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, 1, 0 , 0);

/*	input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR, 0, MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_WIDTH_MAJOR, 0, MAX_AREA, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_X, ts->x_min, ts->x_max, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y, ts->y_min, ts->y_max, 0, 0);
*/
	error = input_register_device(input_dev);
	if (error) {
		dev_err(&client->dev, "Unable to register %s input device\n",
			input_dev->name);
		return error;
	}

	//INIT_DELAYED_WORK(&ts->dwork, cst3xx_work_handler);

	INIT_WORK(&ts->work, cst3xx_work_handler);
	ts->workqueue = create_singlethread_workqueue("cst3xx_wq");
#if 0
	error = devm_request_threaded_irq(&client->dev, client->irq,
					  NULL,
					  cst3xx_ts_irq_handler,
					  IRQF_ONESHOT,
					  client->name, ts);
	if (error) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		return error;
	}
#endif
  	error = request_irq(ts->irq,
				cst3xx_ts_irq_handler,
				IRQF_TRIGGER_FALLING | IRQF_ONESHOT,
				client->dev.driver->name,
				ts);
	if (error < 0) {
		dev_err(&client->dev, "%s: request irq failed\n",__func__);
		return error;
	}

	i2c_set_clientdata(client, ts);
	printk(KERN_INFO "-----------CST3XX probe end ------------\n");
	return 0;
}

static int cst3xx_ts_remove(struct i2c_client *client)
{
	struct cst3xx_ts_data *ts = i2c_get_clientdata(client);
	free_irq(ts->irq,ts);
	devm_free_irq(&ts->client->dev, ts->client->irq,ts);
	destroy_workqueue(ts->workqueue);
	input_unregister_device(ts->input_dev);
	devm_gpio_free(&ts->client->dev, ts->reset_gpio);
	gpio_free(ts->int_gpio);

#if USE_ANALOG_I2C
	gpio_free(I2CSDA);
	gpio_free(I2CSCL);
#endif

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cst3xx_ts_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cst3xx_ts_data *ts = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev)) {
		enable_irq_wake(client->irq);
	} else {
		disable_irq(client->irq);
		cst3xx_ts_power(ts, false);//reset
	}

	return 0;
}

static int cst3xx_ts_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct cst3xx_ts_data *ts = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev)) {
		disable_irq_wake(client->irq);
	} else {
		cst3xx_ts_power(ts, true);
		enable_irq(client->irq);
	}
	return 0;
}

#endif

static SIMPLE_DEV_PM_OPS(cst3xx_ts_pm_ops,
			 cst3xx_ts_suspend, cst3xx_ts_resume);

static const struct i2c_device_id cst3xx_ts_id[] = {
	{ CST3XX_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cst3xx_ts_id);

#ifdef CONFIG_OF
static const struct of_device_id cst3xx_ts_dt_ids[] = {
	{ .compatible = "ime,cst3xx", },
	{ }
};
MODULE_DEVICE_TABLE(of, cst3xx_ts_dt_ids);
#endif

static struct i2c_driver cst3xx_ts_driver = {
	.probe		= cst3xx_ts_probe,
	.remove		= cst3xx_ts_remove,
	.id_table	= cst3xx_ts_id,
	.driver = {
		.name	= CST3XX_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(cst3xx_ts_dt_ids),
		.pm	= &cst3xx_ts_pm_ops,
	},
};

module_i2c_driver(cst3xx_ts_driver);

MODULE_AUTHOR("Tiben");
MODULE_DESCRIPTION("SITRONIX CST3XX Touchscreen Controller Driver");
MODULE_LICENSE("GPL");
