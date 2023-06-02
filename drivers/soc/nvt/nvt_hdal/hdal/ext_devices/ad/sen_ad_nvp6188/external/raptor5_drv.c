
/* ----------------------------------------------------------------------------------
 * 1. Header file include -----------------------------------------------------------
 * --------------------------------------------------------------------------------*/
#ifndef NVT_PLATFORM
#include <linux/string.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#endif

#include "raptor5_common.h"
#include "raptor5_video.h"
#include "raptor5_function.h"
#include "raptor5_table.h"

#ifndef NVT_PLATFORM
unsigned int init=0;
module_param(init, uint, S_IRUGO);
unsigned int fmt = 2; //0:960h 1:720P 2:1080P
module_param(fmt, uint, S_IRUGO);
unsigned int ntpal = 0; //0:ntsc/30p 1:pal/25p
module_param(ntpal, uint, S_IRUGO);
#endif
unsigned int mbps = NC_MIPI_BPS_1458Mbps;
module_param(mbps, uint, S_IRUGO);

#ifndef NVT_PLATFORM
unsigned int mux = 2; //2:2mux 4:4mux
module_param(mux, uint, S_IRUGO);
#endif


/* ----------------------------------------------------------------------------------
 * 2. Define ------------------------------------------------------------------------
 * --------------------------------------------------------------------------------*/
#define DRIVER_VERSION_MAJOR	1
#define DRIVER_VERSION_MINOR1	0
#define DRIVER_VERSION_MINOR2	0
#define RELEASE_YEAR			20
#define RELEASE_MONTH			8
#define RELEASE_DAY				26

#define I2C_0       (0)
#define I2C_1       (1)
#define I2C_2       (2)
#define I2C_3       (3)

#define EMBEDDED_DATA_ON  1
#define EMBEDDED_DATA_OFF 0


/* ----------------------------------------------------------------------------------
 * 3. Define variable ---------------------------------------------------------------
 * --------------------------------------------------------------------------------*/
static unsigned int s_nc_drv_chip_cnt = 0;
static unsigned int s_nc_drv_channel_cnt = 0;

NC_U8 g_nc_drv_chip_id[4]   = { 0xFF, 0xFF, 0xFF, 0xFF };
NC_U8 g_nc_drv_chip_rev[4]  = { 0xFF, 0xFF, 0xFF, 0xFF };
NC_U8 g_nc_drv_i2c_addr[4]  = { 0xFF, 0xFF, 0xFF, 0xFF };

#ifndef NVT_PLATFORM
struct semaphore  nc_drv_lock;
struct i2c_client *nc_i2c_client;

static struct i2c_board_info hi_info =
{
	I2C_BOARD_INFO("raptor5", 0x60 >> 1),
};
#endif

/* ----------------------------------------------------------------------------------
 * 4. External variable & function --------------------------------------------------
 * --------------------------------------------------------------------------------*/

/* ----------------------------------------------------------------------------------
 * 5. Function prototype ------------------------------------------------------------
 * --------------------------------------------------------------------------------*/
void 		  __I2CWriteByte8(unsigned char chip_addr, unsigned char reg_addr, unsigned char value);
unsigned char __I2CReadByte8(unsigned char chip_addr, unsigned char reg_addr);

void nc_drv_version_information_print(void);

int  nc_drv_chip_infomation_get(void);
void nc_drv_chip_infomation_to_app(nc_decoder_s *psVdtDevInfo);

void nc_drv_mipi_dphy_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, NC_MIPI_LANE_E lane, NC_MIPI_BPS_E clk, NC_MIPI_SWAP_E swap);
void nc_drv_arbiter_common_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, int emb, NC_MIPI_BPS_E clk);
void nc_drv_arbiter_embedded_data_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, int enable);

void nc_drv_dec_output_port_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid);

void nc_drv_decoder_init_set(void);
void nc_drv_decoder_initialize(NC_VO_WORK_MODE_E vomux);

#ifndef NVT_PLATFORM
int  nc_decoder_open(struct inode * inode, struct file * file);
int  nc_decoder_close(struct inode * inode, struct file * file);
long nc_decoder_ioctl(struct file *file, unsigned int cmd, unsigned long arg);


void __I2CWriteByte8(unsigned char chip_addr, unsigned char reg_addr, unsigned char value)
{
	int ret;
	unsigned char buf[2];
	struct i2c_client* client = nc_i2c_client;

	nc_i2c_client->addr = (chip_addr >> 1);

	buf[0] = reg_addr;
	buf[1] = value;

	ret = i2c_master_send(client, buf, 2);
	udelay(300);
}

unsigned char __I2CReadByte8(unsigned char chip_addr, unsigned char reg_addr)
{
    unsigned char         ret_data = 0xFF;
    int                   ret;
    struct i2c_client*    client = nc_i2c_client;
    static struct i2c_msg msg[2];
    unsigned char         buffer[2];
    unsigned int          data_width = 1;

    buffer[0] = reg_addr & 0xFF;

    msg[0].addr = client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = buffer;

    msg[1].addr = client->addr;
    msg[1].flags = client->flags | I2C_M_RD;
    msg[1].len = 1;
    msg[1].buf = buffer;

    ret = i2c_transfer(client->adapter, msg, 2);
    if (ret != 2) {
        printk("[%s %d] hi_i2c_transfer error, ret=%d.\n", __FUNCTION__, __LINE__, ret);
        return 0xff;
    }

    memcpy(&ret_data,buffer,data_width);

    return ret_data;
}
#endif

void nc_drv_version_information_print(void)
{
	int ii = 0;
	char szAppVersion[32];

	memset( &szAppVersion[0], 0, sizeof(szAppVersion));
	sprintf(szAppVersion, "%d.%d.%d/%02d%02d%02d",
			DRIVER_VERSION_MAJOR, DRIVER_VERSION_MINOR1, DRIVER_VERSION_MINOR2, RELEASE_YEAR, RELEASE_MONTH, RELEASE_DAY);

	printk("****************   NVP6188/6188C Device Driver Version Information  ****************\n");
	printk("    Driver. VERSION       = %s\n", szAppVersion);
	printk("    COMPILED              = %s %s\n", __TIME__, __DATE__);
	for(ii=0; ii<(int)s_nc_drv_chip_cnt; ii++)
		printk("    %d.              = %02X, %s\n", ii, g_nc_drv_chip_id[ii], nc_drv_common_chip_id_str_get(g_nc_drv_chip_id[ii]));
	printk("************************************************************************************\n");

}

int nc_drv_chip_infomation_get(void)
{
	int ret = -1;
	int chip;
	NC_U8 I2C_ACK = 0;
	NC_U8 NC_Decoder_i2c_addr[4] = { 0x60, 0x62, 0x64, 0x66 };

	/**********************************************************************************
	 * I2C Communication(Receive) Check
	 * 0 == 0x60, 1 == 0x62, 2 == 0x64, 3 == 0x66
	 * CheckVal == Success 0x00, Fail 0xFF
	 **********************************************************************************/
	for(chip=0; chip<4; chip++)
	{
		gpio_i2c_write(NC_Decoder_i2c_addr[chip], 0xFF, 0x00);
		I2C_ACK = gpio_i2c_read(NC_Decoder_i2c_addr[chip], 0xFF);
		if( I2C_ACK == 0 )
		{
			g_nc_drv_i2c_addr[s_nc_drv_chip_cnt] = NC_Decoder_i2c_addr[chip];
			s_nc_drv_chip_cnt++;
			printk("<<<<<<<<<<< [ Success::0x%02X ] Decoder I2C Communication!!!! >>>>>>>>>>>\n", NC_Decoder_i2c_addr[chip]);
		}
		else
			printk(">>>>>>>>>>> [ Fail::0x%02X ] Decoder I2C Communication!!!! <<<<<<<<<<<\n", NC_Decoder_i2c_addr[chip]);
	}

	if( s_nc_drv_chip_cnt <= 0 )
	{
		printk("Decoder Driver I2C(0x60, 0x62, 0x64, 0x66) Communication Error!!!!\n");
		return ret;
	}
	else
	{
		s_nc_drv_channel_cnt = s_nc_drv_chip_cnt * 4;  // Each Chip 4 Channel Input
	}

	/**********************************************************************************
	 * 1. Decoder ID Check
	 * 2. Decoder Revision Check
	 **********************************************************************************/
	for(chip=0; chip<(int)s_nc_drv_chip_cnt; chip++)
	{
		/* Decoder Device ID Check */
		gpio_i2c_write(g_nc_drv_i2c_addr[chip], 0xFF, BANK_0);
		g_nc_drv_chip_id[chip] = gpio_i2c_read(g_nc_drv_i2c_addr[chip], 0xF4);
		if(g_nc_drv_chip_id[chip] == 0xD3)
			g_nc_drv_chip_id[chip] = 0xD0;

		/* Decoder Device Revision Check */
		gpio_i2c_write(g_nc_drv_i2c_addr[chip], 0xFF, BANK_0);
		g_nc_drv_chip_rev[chip] = gpio_i2c_read(g_nc_drv_i2c_addr[chip], 0xF5);
	}

	ret = 0;
	printk("********************** Decoder Chip Information *********************\n");
	printk("Decoder Chip Count = %d\n", s_nc_drv_chip_cnt);
	printk("SlaveAddress    [0x%02X], [0x%02X], [0x%02X], [0x%02X]\n",g_nc_drv_i2c_addr[0],g_nc_drv_i2c_addr[1],g_nc_drv_i2c_addr[2],g_nc_drv_i2c_addr[3]);
	printk("DecoderID       [0x%02X], [0x%02X], [0x%02X], [0x%02X]\n",g_nc_drv_chip_id[0],g_nc_drv_chip_id[1],g_nc_drv_chip_id[2],g_nc_drv_chip_id[3]);
	printk("DecoderRevision [0x%02X], [0x%02X], [0x%02X], [0x%02X]\n",g_nc_drv_chip_rev[0],g_nc_drv_chip_rev[1],g_nc_drv_chip_rev[2],g_nc_drv_chip_rev[3]);
	printk("**********************************************************************\n");

	return ret;
}


void nc_drv_chip_infomation_to_app(nc_decoder_s *psVdtDevInfo)
{
	int ii = 0;

	/**********************************************************************
	 * Raptor5 Device Driver Information Initialize
	 **********************************************************************/
	nc_drv_common_info_chip_data_init_set(s_nc_drv_chip_cnt, g_nc_drv_chip_id, g_nc_drv_chip_rev, g_nc_drv_i2c_addr, NC_VO_WORK_MODE_2MUX);

	psVdtDevInfo->Total_Chip_Cnt = s_nc_drv_chip_cnt;

	for(ii=0; ii<4; ii++)
	{
		psVdtDevInfo->chip_id[ii]   = g_nc_drv_chip_id[ii];
		psVdtDevInfo->chip_rev[ii]  = g_nc_drv_chip_rev[ii];
		psVdtDevInfo->chip_addr[ii] = g_nc_drv_i2c_addr[ii];
	}
}
void nc_drv_mipi_dphy_lanepn_swap(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, NC_MIPI_LANE_E lane, int lswap)
{

	int phy = 2;
	int jj  = 0;
	int ii 	= 0;

	if(chipid == NC_DECODER_6188)
	{
		phy = 2;
	}
	else //if(chipid == NC_DECODER_6188C)
	{
		phy = 1;
	}

	for(jj=0; jj<phy; jj++)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_23+(0x10*jj));
		for(ii=0; ii<5; ii++)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40+(ii*0x20), lswap==0?0x4a:0x5a); //clk/lane0~3
		}
	}
}

void nc_drv_mipi_dphy_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, NC_MIPI_LANE_E lane, NC_MIPI_BPS_E bps, NC_MIPI_SWAP_E swap)
{
	int phy = 2;
	int jj  = 0;
	int ii 	= 0;

    NC_U8 lane_hs_tlpx_time = 0;
    NC_U8 lane_prepare_time = 0;
    NC_U8 lane_zero_time = 0;
    NC_U8 lane_trail_time = 0;

	if(chipid != NC_DECODER_6188 && chipid != NC_DECODER_6188C)
	{
		printk("[%s::%d]Unknown chip id!!!(%x)\n", __func__, __LINE__, chipid);
		return;
	}
	else
	{
		if(chipid == NC_DECODER_6188)
			phy = 2;
		else if(chipid == NC_DECODER_6188C)
			phy = 1;
		else
			return;
	}

	for(jj=0; jj<phy; jj++)
	{
        printk("[%s::%d]%x : dev(%d) lane(%s) bps(%s)\n", __func__, __LINE__, BANK_23+(0x10*jj), dev, nc_drv_common_mipi_lane_str_get(lane), nc_drv_common_mipi_clock_str_get(bps));

		/* LANE & PLL SET */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_23+(0x10*jj));

		if(lane == NC_MIPI_LANE_4)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x03);
		}
		else if(lane == NC_MIPI_LANE_2)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x01);
		}
		else if(lane == NC_MIPI_LANE_1)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x00);
		}
		else
		{
			printk("[%s::%d]mipi lane cfg Error!! %d\n", __FILE__, __LINE__, lane);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xe9, 0x03);
		}

        /* PLL Setting */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x03, 0x02);
        if(bps == NC_MIPI_BPS_594Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x2c);
        }
        else if(bps == NC_MIPI_BPS_756Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x38);
        }
        else if(bps == NC_MIPI_BPS_972Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x48);
		}
        else if(bps == NC_MIPI_BPS_1242Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x5C);
		}
        else if(bps == NC_MIPI_BPS_1458Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x6C);
		}
		else if(bps == NC_MIPI_BPS_297Mbps)
		{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x16);
		}
		else
		{
            printk("[%s::%d]mipi clock cfg Error!! %d\n", __FILE__, __LINE__, bps);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x04, 0x70);
		}
        /* Reset */
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0xe4);// PLL Enable & Analog reset on
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x7d);// All lanes Enable on Analog part
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0xe0);// Analog reset off
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x20, 0x1e);// Digital reset on
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x20, 0x1f);// Digital reset off
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xeb, 0x8d); // Short packet enable

        /* Clock & Data Lane Setting */
        if(bps == NC_MIPI_BPS_594Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);

            lane_hs_tlpx_time = 0xc5;
            lane_prepare_time = 0x08;
            lane_zero_time    = 0x09;
            lane_trail_time   = 0x30;
        }
        else if(bps == NC_MIPI_BPS_756Mbps)
        {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);

            lane_hs_tlpx_time = 0xc5;
            lane_prepare_time = 0x08;
            lane_zero_time    = 0x09;
            lane_trail_time   = 0x30;
    }
    else if(bps == NC_MIPI_BPS_972Mbps)
		{
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
           gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);
           lane_hs_tlpx_time = 0xc5;
           lane_prepare_time = 0x08;
           lane_zero_time    = 0x09;
           lane_trail_time   = 0x30;
    }
    else if(bps == NC_MIPI_BPS_1242Mbps)
    {
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc9);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x03);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x32);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x0f);

            lane_hs_tlpx_time = 0xc9;
            lane_prepare_time = 0x03;
            lane_zero_time    = 0x14;
            lane_trail_time   = 0x0f;
    }
    else if(bps == NC_MIPI_BPS_1458Mbps)
			{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xcd);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x42);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x36);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x0f);

            lane_hs_tlpx_time = 0xcd;
            lane_prepare_time = 0x42;
            lane_zero_time    = 0x0e;
            lane_trail_time   = 0x0f;
			}
	else if(bps == NC_MIPI_BPS_297Mbps)
    {
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xc5);
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x08);
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x20);
        gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x30);

        lane_hs_tlpx_time = 0xc5;
        lane_prepare_time = 0x08;
        lane_zero_time    = 0x09;
        lane_trail_time   = 0x30;
		}
		else
		{
            printk("[%s::%d]mipi clock cfg Error!! %d\n", __FILE__, __LINE__, bps);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0xcd);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x42);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x36);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0x0f);

            lane_hs_tlpx_time = 0xcd;
            lane_prepare_time = 0x42;
            lane_zero_time    = 0x0e;
            lane_trail_time   = 0x0f;
    }

			for(ii=0; ii<4; ii++)
			{
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x65+(ii*0x20), lane_hs_tlpx_time);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x66+(ii*0x20), lane_prepare_time);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x67+(ii*0x20), lane_zero_time);
            gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x68+(ii*0x20), lane_trail_time);
		}

		if(swap == NC_MIPI_SWAP_ENABLE)
		{
			for(ii=0; ii<4; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+(ii*0x20), 0x5a);	// MIPI Lane
			}
		}
		else
		{
			for(ii=0; ii<4; ii++)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x60+(ii*0x20), 0x4a);
			}
		}

		msleep(10);
	}
}


void nc_drv_arbiter_common_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, int emb, NC_MIPI_BPS_E bps)
{
	int arb_dev = 2;
	int ii 	= 0;
	NC_VO_WORK_MODE_E mux_mode = nc_drv_common_info_vo_mode_get(dev);

	if(chipid != NC_DECODER_6188 && chipid != NC_DECODER_6188C)
	{
		printk("[%s::%d]Unknown chip id!!!(%x)\n", __func__, __LINE__, chipid);
		return;
	}
	else
	{
		if(chipid == NC_DECODER_6188)	    arb_dev = 2;
		else if(chipid == NC_DECODER_6188C)	arb_dev = 1;
	}

	printk("[%s::%d]dev(%d), chipid(0x%02x), emb(%d)\n", __func__, __LINE__, dev, chipid, emb);

	if(emb == EMBEDDED_DATA_ON)
	{
		for(ii=0; ii<4; ii++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + ii);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x81, 0x80);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x80);
		}
	}
	else
	{
		for(ii=0; ii<4; ii++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + ii);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x81, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x00);
		}
	}

	for(ii=0; ii<arb_dev; ii++)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*ii));
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40, 0x01);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x10, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x11, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x49);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x45, 0x60);
		#if 0
		if((bps == NC_MIPI_BPS_594Mbps) || (clk == NC_MIPI_BPS_756Mbps))
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x0f);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x32, 0x92);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x34, 0xcd);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x36, 0x04);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x38, 0x58);
		}
		else
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x0f);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x32, 0xff);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x34, 0xcd);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x36, 0x04);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x38, 0xff);
		}
		#else
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x01);
		#endif

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2a, 0x0a);

		/* Short packet */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3d, 0x11); // odd, even
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3e, 0x11); // odd, even
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3c, 0x01);

		//if(chipid == NC_DECODER_6188)
		if(NC_VO_WORK_MODE_2MUX == mux_mode)
		{
			/* Embedded Data */
			if(emb == EMBEDDED_DATA_ON)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x0f);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x11);
			}
			else
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x00);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x00);
			}

			/* Arbiter 16bit mode */
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0xaa); // Channel 1, 3
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x0a); // Channel 1, 3
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, 0x6d);
		}
		//else if(chipid == NC_DECODER_6188C)
		else if(NC_VO_WORK_MODE_4MUX == mux_mode)
		{
			/* Embedded Data */
			if(emb == EMBEDDED_DATA_ON)
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x0f);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x55);
			}
			else
			{
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x00);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x00);
			}

			/* Arbiter 8bit mode */
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x00); // Channel 1, 2, 3, 4
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x00); // Channel 1, 2, 3, 4
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, 0x01);
		}
		else
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x00); // Channel 1, 2, 3, 4
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x00); // Channel 1, 2, 3, 4
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x0d, 0x01);
		}

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x40, 0x00);

	}
}

void nc_drv_arbiter_embedded_data_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid, int enable)
{
	int arb_dev = 2;
	int ii 	= 0;

	if(chipid != NC_DECODER_6188 && chipid != NC_DECODER_6188C)
	{
		printk("[%s::%d]chipid get Error!! %x\n", __FILE__, __LINE__, chipid);
		return;
	}

	printk("[%s::%d]chipid(%02x), Embedded(%d)\n", __func__, __LINE__, chipid, enable);

	if(enable == EMBEDDED_DATA_ON)
	{
		for(ii=0; ii<4; ii++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + ii);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x81, 0x80);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x80);
		}

		if(chipid == NC_DECODER_6188)
		{
			arb_dev = 2;
			for(ii=0; ii<arb_dev; ii++)
			{
				NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*ii));
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x0f);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x11);
			}
		}
		else if(chipid == NC_DECODER_6188C)
		{
			arb_dev = 1;
			for(ii=0; ii<arb_dev; ii++)
			{
				NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*ii));
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x0f);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x55);
			}
		}
	}
	else if(enable == EMBEDDED_DATA_OFF)
	{
		for(ii=0; ii<4; ii++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + ii);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x81, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x00);
		}

		if(chipid == NC_DECODER_6188)
		{
			arb_dev = 2;
			for(ii=0; ii<arb_dev; ii++)
			{
				NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*ii));
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x00);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x00);
			}
		}
		else if(chipid == NC_DECODER_6188C)
		{
			arb_dev = 1;
			for(ii=0; ii<arb_dev; ii++)
			{
				NC_DEVICE_DRIVER_BANK_SET(dev, BANK_20+(0x10*ii));
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1a, 0x92);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1b, 0x00);
				gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x1c, 0x00);
			}
		}
	}
}

void nc_drv_dec_output_port_init_set(NC_U8 dev, NC_DECODER_CHIP_MODEL_E chipid)
{
	NC_VO_WORK_MODE_E vo_mux = nc_drv_common_info_vo_mode_get(dev);

	//if(chipid == NC_DECODER_6188)
	if(NC_VO_WORK_MODE_2MUX == vo_mux)
	{
		int time_mux = 2;
		int ii = 0;

		for(ii=0; ii<time_mux; ii++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1 + (ii*0x30));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0, 0x44 + (ii*0x22)); // Port A - chn1 Only_y
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1, 0x44 + (ii*0x22));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC2, 0xcc + (ii*0x22)); // Port B - chn1 Only_c
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC3, 0xcc + (ii*0x22));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC4, 0x55 + (ii*0x22)); // Port C - chn2 Only_y
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC5, 0x55 + (ii*0x22));
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC6, 0xdd + (ii*0x22)); // Port D - chn2 Only_c
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC7, 0xdd + (ii*0x22));

			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xed, 0x0f);
		}
	}
	//else if(chipid == NC_DECODER_6188C)
	else if(NC_VO_WORK_MODE_4MUX == vo_mux)
	{
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC0, 0x00); // Port A - chn1
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC1, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC2, 0x11); // Port B - chn2
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC3, 0x11);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC4, 0x22); // Port C - chn3
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC5, 0x22);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC6, 0x33); // Port D - chn4
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xC7, 0x33);

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xed, 0x00);
	}
	else
	{
		printk("[%s::%d]Error!!\n", __func__, __LINE__);
		return;
	}
}

void nc_drv_decoder_init_set(void)
{
	int dev = 0;
	int chn = 0;
	NC_U8 chip_id = 0;

	for(dev=0; dev<(int)s_nc_drv_chip_cnt; dev++)
	{
		printk("[%s::%d]Addr[0x%02X], ID[0x%02X : %s] Initialize!!\n", __func__, __LINE__, g_nc_drv_i2c_addr[dev], g_nc_drv_chip_id[dev], nc_drv_common_chip_id_str_get(g_nc_drv_chip_id[dev]));

		chip_id = nc_drv_common_info_chip_id_get(dev);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);

		/* Internal Clock Divider Reset */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x40);

		/* External Interface Clock - PD_VCLK[5:4] */
		if(chip_id == NC_DECODER_6188 || chip_id == NC_DECODER_6188C)
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98, 0x30);
		}
		else
		{
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x98, 0x30);
			printk("[%s::%d]Error!! unknown id!![%x :: %x]\n", __func__, __LINE__, g_nc_drv_chip_id[dev], chip_id);
		}

		/* Clock Auto OFF */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a, 0x00);

		/* MIPI D-PHY Initialize */
		nc_drv_mipi_dphy_init_set(dev, nc_drv_common_info_chip_id_get(dev), NC_MIPI_LANE_4, mbps, NC_MIPI_SWAP_DISABLE);

		/* Decoder Initialize */
		for(chn=0; chn<4; chn++)
		{
			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_5 + chn);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0xd0);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x04);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x08, 0x55);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0xEE);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x59, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x76, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x77, 0x80);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x78, 0x00);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x79, 0x11);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0xB8);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7B, 0x11);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xb9, 0x72);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xB8, 0xB8);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2c, 0x00);

			NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00+chn, 0x10);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x22+(chn*0x04), 0x0b);
			gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x23+(chn*0x04), 0x41);
		}

		/* VFC Auto Mode Setting */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0xA0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x12, 0x04);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2E, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x70);	// Auto 0x7F, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0xff);	// Auto 0x7F, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3a, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3b, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3c, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3d, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3e, 0xff);	// Auto 0xFF, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x3f, 0x0f);	// Auto 0xFF, Manual 0x00

		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x70, 0x30);	// Auto 0x30 or f0, Manual 0x00
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x72, 0x05);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7A, 0xf0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x47);

		/* EQ Stage get level */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x74, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x76, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x78, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x75, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x77, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x79, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x01, 0x0c);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x2f, 0xc8);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x73, 0x23);

		/* H Scaler */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_9);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x96, 0x03);

		/* Audio Default Setting */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x05, 0x09);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x07, 0x88);	// Master Mode, 16Khz, 16bit_Width
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x22, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x44, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x00, 0x02);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x06, 0x1B);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x31, 0x0A);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x32, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x46, 0x10);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x47, 0x01);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x48, 0xD0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x49, 0x88);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x58, 0x02);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x59, 0x00);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x5d, 0x00); // NR Enable 0x9a, Disable 0x00

		/* MPP Coaxial mode select Ch1~4  */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xA8, 0x08);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xA9, 0x09);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xAA, 0x0A);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xAB, 0x0B);

		/* Disable Video clock, Video digital output */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xca, 0x00);

		/* Output port initialize */
		nc_drv_dec_output_port_init_set(dev, nc_drv_common_info_chip_id_get(dev));

		/* Common Arbiter initialize */
		nc_drv_arbiter_common_init_set(dev, nc_drv_common_info_chip_id_get(dev), EMBEDDED_DATA_OFF, NC_MIPI_BPS_1458Mbps);

		/* PLL Clock ****************************************
		 * 148.5Mhz : 1x82->0x14, 1x83->0x2c
		 * 297Mhz   : 1x82->0x12, 1x83->0x2c
		 ****************************************************/
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x82, 0x12);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x61);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x80, 0x60);

		/* TM Clock */
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa0, 0x20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa1, 0x20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa2, 0x20);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0xa3, 0x20);

		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7a, 0xff);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x7b, 0xff);


		nc_drv_video_input_initialize_set(dev);

		/* MIPI, Arbiter[7:4], Decoder[3:0] Reset */
		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_1);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x97, 0xf0);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x97, 0x0f);


		NC_DEVICE_DRIVER_BANK_SET(dev, BANK_13);
		gpio_i2c_write(g_nc_drv_i2c_addr[dev], 0x30, 0x7f);
	}
}

void nc_drv_decoder_initialize(NC_VO_WORK_MODE_E vomux)
{
	/**********************************************************************
	 * 1. I2C Communication Check
	 * 2. Decoder Chip ID Get
	 * 3. Decoder Chip Revision Get
	 **********************************************************************/
	nc_drv_chip_infomation_get();

	/**********************************************************************
	 * Device Driver Source Code Version
	 **********************************************************************/
	nc_drv_version_information_print();

	/**********************************************************************
	 * Raptor5 Device Driver Information Initialize
	 **********************************************************************/
	nc_drv_common_info_chip_data_init_set(s_nc_drv_chip_cnt, g_nc_drv_chip_id, g_nc_drv_chip_rev, g_nc_drv_i2c_addr, vomux);

	/**********************************************************************
	 * Raptor5 Device Initialize
	 **********************************************************************/
	nc_drv_decoder_init_set();
}


#ifndef NVT_PLATFORM
int nc_decoder_open(struct inode * inode, struct file * file)
{
	printk("[DRV] Raptor5 Driver Open\n");

	return 0;
}

int nc_decoder_close(struct inode * inode, struct file * file)
{
	printk("[DRV] Raptor5 Driver Close\n");
	return 0;
}

long nc_decoder_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	NC_S32 err = 0;

	NC_U32 __user *argp = (unsigned int __user *)arg;

	nc_decoder_s stNC_DRV;

	down(&nc_drv_lock);

	if( (err = copy_from_user(&stNC_DRV, argp, sizeof(nc_decoder_s))) < 0 )
	{
		printk("copy_from_user error!!\n");
		up(&nc_drv_lock);
		return err;
	}


	switch (cmd)
	{
		/*===============================================================================================
		* Decoder Chip Information Get
		*===============================================================================================*/
		case IOC_VDEC_CHIP_INFO_GET:		nc_drv_chip_infomation_to_app(&stNC_DRV);		break;

		/*===============================================================================================
		* VIDEO
		*===============================================================================================*/
		case IOC_VDEC_VIDEO_INPUT_INFO_GET:			nc_drv_video_setting_info_get(&stNC_DRV);			break;
		case IOC_VDEC_VIDEO_INPUT_VFC_GET:			nc_drv_video_input_vfc_status_get(&stNC_DRV);		break;
		case IOC_VDEC_VIDEO_INPUT_SET:				nc_drv_video_input_set(&stNC_DRV);					break;
		case IOC_VDEC_VIDEO_INPUT_EQ_STAGE_GET:		nc_drv_video_input_eq_stage_get(&stNC_DRV);			break;
		case IOC_VDEC_VIDEO_INPUT_EQ_STAGE_SET:		nc_drv_video_input_eq_stage_set(&stNC_DRV);			break;
		case IOC_VDEC_VIDEO_OUTPUT_SET:				nc_drv_video_output_port_set(&stNC_DRV);			break;
		case IOC_VDEC_VIDEO_OUTPUT_MANUAL_SET:		nc_drv_video_output_port_manual_set(&stNC_DRV);		break;
		case IOC_VDEC_VIDEO_OUTPUT_PATTERN_SET:		nc_drv_video_output_color_pattern_set(&stNC_DRV);	break;
		case IOC_VDEC_VIDEO_AUTO_MANUAL_MODE_SET:	nc_drv_video_auto_manual_mode_set(&stNC_DRV);		break;
		case IOC_VDEC_VIDEO_INPUT_MANUAL_SET:		nc_drv_video_input_manual_set(&stNC_DRV);			break;
		case IOC_VDEC_VIDEO_COLOR_TUNE_GET:			nc_drv_video_color_tune_get(&stNC_DRV);				break;
		case IOC_VDEC_VIDEO_COLOR_TUNE_SET:			nc_drv_video_color_tune_set(&stNC_DRV);				break;

		/*===============================================================================================
		* Coaxial Protocol
		*===============================================================================================*/
		case IOC_VDEC_COAX_INIT_SET:			nc_drv_coax_initialize_set(&stNC_DRV);								break;
		case IOC_VDEC_COAX_INIT_MANUAL_SET:		nc_drv_coax_initialize_manual_set(&stNC_DRV);						break;
		case IOC_VDEC_COAX_UP_COMMAMD_SET:		nc_drv_coax_up_stream_command_set(&stNC_DRV);						break;
		case IOC_VDEC_COAX_DOWN_DATA_GET:		nc_drv_coax_down_stream_data_get(&stNC_DRV);						break;
		case IOC_VDEC_COAX_FW_ACP_HEADER_GET:	nc_drv_coax_fwseq_1_ready_header_check_from_isp_recv(&stNC_DRV);	break;
		case IOC_VDEC_COAX_FW_READY_CMD_SET:	nc_drv_coax_fwseq_2_1_ready_cmd_to_isp_send(&stNC_DRV);				break;
		case IOC_VDEC_COAX_FW_READY_ACK_GET:	nc_drv_coax_fwseq_2_2_ready_cmd_ack_from_isp_recv(&stNC_DRV);		break;
		case IOC_VDEC_COAX_FW_START_CMD_SET:	nc_drv_coax_fwseq_3_1_start_cmd_to_isp_send(&stNC_DRV);				break;
		case IOC_VDEC_COAX_FW_START_ACK_GET:	nc_drv_coax_fwseq_3_2_start_cmd_ack_from_isp_recv(&stNC_DRV);		break;
		case IOC_VDEC_COAX_FW_SEND_DATA_SET:	nc_drv_coax_fwseq_4_1_one_packet_data_to_isp_send(&stNC_DRV);		break;
		case IOC_VDEC_COAX_FW_SEND_ACK_GET:		nc_drv_coax_fwseq_4_2_one_packet_data_ack_from_isp_recv(&stNC_DRV);	break;
		case IOC_VDEC_COAX_FW_END_CMD_SET:		nc_drv_coax_fwseq_5_1_end_cmd_to_isp_send(&stNC_DRV);				break;
		case IOC_VDEC_COAX_FW_END_ACK_GET:		nc_drv_coax_fwseq_5_2_end_cmd_ack_from_isp_recv(&stNC_DRV);			break;
		case IOC_VDEC_COAX_TEST_ISP_DATA_SET:	nc_drv_coax_test_isp_data_set(&stNC_DRV);							break;
		case IOC_VDEC_COAX_TX_REG_CTRL_SET:		nc_drv_coax_tx_reg_ctrl_set(&stNC_DRV);								break;
		case IOC_VDEC_COAX_TX_REG_CTRL_GET:		nc_drv_coax_tx_reg_ctrl_get(&stNC_DRV);								break;

		/*===============================================================================================
		* AUDIO
		*===============================================================================================*/
		case IOC_VDEC_AUDIO_INIT_SET:				nc_drv_audio_init_set(&stNC_DRV);			break;
		case IOC_VDEC_AUDIO_VIDEO_AOC_FORMAT_SET:	nc_drv_audio_video_format_set(&stNC_DRV);	break;

		/*===============================================================================================
		* MOTION
		*===============================================================================================*/
		case IOC_VDEC_MOTION_ON_OFF_SET:		nc_drv_motion_onoff_set(&stNC_DRV);				break;
		case IOC_VDEC_MOTION_BLOCK_SET:			nc_drv_motion_each_block_onoff_set(&stNC_DRV);	break;
		case IOC_VDEC_MOTION_BLOCK_GET:			nc_drv_motion_each_block_onoff_get(&stNC_DRV);	break;
		case IOC_VDEC_MOTION_TSEN_SET:			nc_drv_motion_motion_tsen_set(&stNC_DRV);		break;
		case IOC_VDEC_MOTION_PSEN_SET:			nc_drv_motion_motion_psen_set(&stNC_DRV);		break;
		case IOC_VDEC_MOTION_ALL_BLOCK_SET:		nc_drv_motion_all_block_onoff_set(&stNC_DRV);	break;
		case IOC_VDEC_MOTION_DETECTION_GET:		nc_drv_motion_detection_info_get(&stNC_DRV);	break;

		/*===============================================================================================
		* ETC
		*===============================================================================================*/
		case IOC_VDEC_REG_BANK_DUMP_GET:		nc_drv_common_bank_data_get(&stNC_DRV);		break;
		case IOC_VDEC_REG_DATA_SET:				nc_drv_common_register_data_set(&stNC_DRV);	break;
		case IOC_VDEC_REG_DATA_GET:				nc_drv_common_register_data_get(&stNC_DRV);	break;
		default:	printk("[%s::%d]Decoder Device Driver Unknown ioctl!!!0x%x\n", __FILE__, __LINE__, cmd);

	}

	if( (err = copy_to_user(argp, &stNC_DRV, sizeof(nc_decoder_s))) < 0 )
	{
		printk("%d::copy_to_user error!!\n", cmd);
		up(&nc_drv_lock);
		return err;
	}

	up(&nc_drv_lock);

	return 0;
}

static int i2c_client_init(void)
{
	struct i2c_adapter* i2c_adap;

	printk("[DRV] I2C Client Init_0\n");
	i2c_adap = i2c_get_adapter(0);
	nc_i2c_client = i2c_new_device(i2c_adap, &hi_info);
	i2c_put_adapter(i2c_adap);

	return 0;
}

static void i2c_client_exit(void)
{
	i2c_unregister_device(nc_i2c_client);
}

static struct file_operations raptor5_fops = {
	.owner      = THIS_MODULE,
	.unlocked_ioctl	= nc_decoder_ioctl,
	.open           = nc_decoder_open,
	.release        = nc_decoder_close
};

static struct miscdevice raptor5_dev = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "raptor5",
	.fops  		= &raptor5_fops,
};

////////////////////////////add start//////////////////////////////////
static void rp5_dump_page(int page)
{
  int i = 0;
  unsigned char data = 0;
  printk(KERN_CONT KERN_DEBUG "\n----------------- Page0x%02x Start -------------------\n", page);
  printk(KERN_CONT KERN_DEBUG "\033[1;31m" "     00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F\n" "\033[0m");
  gpio_i2c_write(g_nc_drv_i2c_addr[0], 0xff, page);
  for (i = 0; i <= 0xFF; i++) {
    if (i == 0 || i % 16 == 0)
      printk(KERN_CONT KERN_DEBUG "\033[1;31m" "%02X | " "\033[0m", i);
    data = gpio_i2c_read(g_nc_drv_i2c_addr[0], i);
    printk(KERN_CONT KERN_DEBUG "%02x ", data);
    if ((i > 0) && (((i + 1) % 16) == 0))
      printk(KERN_CONT KERN_DEBUG "\n");
  }
  printk(KERN_CONT KERN_DEBUG "----------------- Page0x%02x End  --------------------\n", page);
}

static ssize_t regs_show(struct device *dev,
                   struct device_attribute *attr, char *buf)
{
    int ret;
    int state;
    state = 0;
    ret = sprintf(buf, "echo page reg val > regs  or  echo 0xff page > regs\n");
    return ret;
}
static ssize_t regs_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
  int             ret;
  unsigned long   value, value1;
  char *next;
  char *tosearch;

    if (buf == NULL){
        return -1;
    }
  tosearch = buf;
  next = strsep(&tosearch, " ");
  value = simple_strtoul(next, NULL, 16);
  if (value == 0xff) {//dump bank.
    next = strsep(&tosearch, " ");
    value = simple_strtoul(next, NULL, 16);
    down(&nc_drv_lock);
    rp5_dump_page(value);
    up(&nc_drv_lock);
  } else {//write reg.
    down(&nc_drv_lock);
    gpio_i2c_write(g_nc_drv_i2c_addr[0], 0xff, value);
    next = strsep(&tosearch, " ");
    value = simple_strtoul(next, NULL, 16);
    next = strsep(&tosearch, " ");
    value1 = simple_strtoul(next, NULL, 16);
    gpio_i2c_write(g_nc_drv_i2c_addr[0], value, value1);
    up(&nc_drv_lock);
  }

    ret = count;
    return ret;
}

static DEVICE_ATTR_RW(regs);
static struct attribute *attributes[] = {
    &dev_attr_regs.attr,
    NULL
};

static struct kobject *rp5_ctrl_kobj;
static const struct attribute_group attr_group = {
    .attrs  = attributes,
};

////////////////////////////add end//////////////////////////////////


static int __init nvp6188_module_init(void)
{
	int ret = 0, ch, dev;
	nc_decoder_s ncde_rp5;
	NC_VIDEO_FMT_INIT_TABLE_S *stTableVideo;
	unsigned char vfmt[16]={0,1,2,2,0xff};

	ret = misc_register(&raptor5_dev);

	if (ret)
	{
		printk("ERROR: could not register raptor5_dev devices:%#x \n",ret);
		return -1;
	}

	printk("================================= raptor5_module_init_start =================================================\n");
	i2c_client_init();

	sema_init(&nc_drv_lock, 1);
	down(&nc_drv_lock);

	/* Decoder Initialize */
	nc_drv_decoder_initialize(mux==4?NC_VO_WORK_MODE_4MUX:NC_VO_WORK_MODE_2MUX);

	if(init)  //‰ª•‰??ØÂõ∫ÂÆöÊ®°ÂºèÊ?ËØïÁî®‰æ?
	{
		printk("[%s::%d] init[%d] mux[%d] fmt[%d]\n", __func__, __LINE__, init, mux, fmt);
		//step 1 : set mux mode parameter.
		//nc_drv_common_info_vo_mode_set(0, mux==4?NC_VO_WORK_MODE_4MUX:NC_VO_WORK_MODE_2MUX);
		for(dev=0;dev<s_nc_drv_chip_cnt;dev++)
		{
			ncde_rp5.OP_Mode = NC_VIDEO_SET_MODE_MANUAL;
			ncde_rp5.Chn = dev*4;
			nc_drv_video_auto_manual_mode_set(&ncde_rp5);
		}
		//step 2 : set chnanel mode.
		for(ch=0;ch<s_nc_drv_chip_cnt*4;ch++)
		{
			//switch(vfmt[ch])
			switch(fmt)
			{
				case 0:
						ncde_rp5.FormatStandard = FMT_SD;
						ncde_rp5.FormatResolution = FMT_H960_Btype_2EX;
						ncde_rp5.FormatFps = ntpal?FMT_PAL:FMT_NT;
				break;
				case 1:
						ncde_rp5.FormatStandard = FMT_AHD;
						ncde_rp5.FormatResolution = FMT_720P_Btype_EX;
						ncde_rp5.FormatFps = ntpal?FMT_25P:FMT_30P;
				break;
				case 9:
						ncde_rp5.FormatStandard = FMT_AHD;
						ncde_rp5.FormatResolution = FMT_960P;
						ncde_rp5.FormatFps = ntpal?FMT_25P:FMT_30P;
				break;
				case 2:
				default:
						ncde_rp5.FormatStandard = FMT_AHD;
						ncde_rp5.FormatResolution = FMT_1080P;
						ncde_rp5.FormatFps = ntpal?FMT_25P:FMT_30P;
				break;
			}
			ncde_rp5.VideoCable  = CABLE_3C2V;
			ncde_rp5.Chn = ch;
			nc_drv_video_input_manual_set(&ncde_rp5);
		}
	}
	up(&nc_drv_lock);
	printk("================================= raptor5_module_init_end ===================================================\n");

	rp5_ctrl_kobj = kobject_create_and_add("rp5_ctrl", kernel_kobj);
	sysfs_create_group(rp5_ctrl_kobj, &attr_group);

	return 0;
}

static void __exit nvp6188_module_exit(void)
{
	kobject_del(rp5_ctrl_kobj);
	misc_deregister(&raptor5_dev);
	i2c_client_exit();

	printk("RAPTOR5 DEVICE DRIVER UNLOAD SUCCESS\n");
}

module_init(nvp6188_module_init);
module_exit(nvp6188_module_exit);
#endif

MODULE_LICENSE("GPL");

/*******************************************************************************
 *	End of file
 *******************************************************************************/
