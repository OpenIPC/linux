#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/uaccess.h>

#include "ssenif_api.h"
#include "ssenif_drv.h"
#include "ssenif_dbg.h"
#include "dal_ssenif.h"
#ifdef SSENIF_TEST_VX1
#include "comm/hwclock.h"
#endif

extern BOOL ssenif_debug_dump(CHAR *str_cmd);

#ifdef SSENIF_TEST_VX1
void emu_ssenif_vx1_open_imx291(PDAL_SSENIFOBJ pssenif_vx1, UINT32 ui_mode)
{
	pssenif_vx1->set_vx1_config(DAL_SSENIFVX1_CFGID_I2C_SPEED,          DAL_SSENIFVX1_I2CSPEED_400KHZ);
	pssenif_vx1->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_SLAVEADDR,       0x1A);
	pssenif_vx1->set_vx1_config(DAL_SSENIFVX1_CFGID_SENREG_ADDR_BC,         2);
	pssenif_vx1->set_vx1_config(DAL_SSENIFVX1_CFGID_SENREG_DATA_BC,         1);

	pssenif_vx1->sensor_i2c_write(0x3002, 0x01); //Master mode stop
	msleep(20);
	pssenif_vx1->sensor_i2c_write(0x3000, 0x01); // standby
	msleep(20);
	pssenif_vx1->sensor_i2c_write(0x3005, 0x01);
	pssenif_vx1->sensor_i2c_write(0x3007, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3009, 0x01);
	pssenif_vx1->sensor_i2c_write(0x300A, 0xF0);
	pssenif_vx1->sensor_i2c_write(0x300F, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3010, 0x21);
	pssenif_vx1->sensor_i2c_write(0x3012, 0x64);
	pssenif_vx1->sensor_i2c_write(0x3013, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3016, 0x09);
	pssenif_vx1->sensor_i2c_write(0x3018, 0x65); //VMAX
	pssenif_vx1->sensor_i2c_write(0x3019, 0x04);
	pssenif_vx1->sensor_i2c_write(0x301c, 0x98); //HMAX
	pssenif_vx1->sensor_i2c_write(0x301d, 0x08);

	pssenif_vx1->sensor_i2c_write(0x3046, 0x01);
	pssenif_vx1->sensor_i2c_write(0x304B, 0x0a);
	pssenif_vx1->sensor_i2c_write(0x305C, 0x18); //INCK
	pssenif_vx1->sensor_i2c_write(0x305D, 0x03);
	pssenif_vx1->sensor_i2c_write(0x305E, 0x20);
	pssenif_vx1->sensor_i2c_write(0x305F, 0x01);

	pssenif_vx1->sensor_i2c_write(0x3070, 0x02);
	pssenif_vx1->sensor_i2c_write(0x3071, 0x11);
	pssenif_vx1->sensor_i2c_write(0x309B, 0x10);
	pssenif_vx1->sensor_i2c_write(0x30A2, 0x02);
	pssenif_vx1->sensor_i2c_write(0x30A6, 0x20);
	pssenif_vx1->sensor_i2c_write(0x30A8, 0x20);
	pssenif_vx1->sensor_i2c_write(0x30AA, 0x20);
	pssenif_vx1->sensor_i2c_write(0x30AC, 0x20);
	pssenif_vx1->sensor_i2c_write(0x30B0, 0x43);

	pssenif_vx1->sensor_i2c_write(0x3119, 0x9E);
	pssenif_vx1->sensor_i2c_write(0x311C, 0x1E);
	pssenif_vx1->sensor_i2c_write(0x311E, 0x08);
	pssenif_vx1->sensor_i2c_write(0x3128, 0x05);
	pssenif_vx1->sensor_i2c_write(0x3129, 0x00);
	pssenif_vx1->sensor_i2c_write(0x313D, 0x83);
	pssenif_vx1->sensor_i2c_write(0x3150, 0x03);
	pssenif_vx1->sensor_i2c_write(0x315E, 0x1A); //INCKSEL5
	pssenif_vx1->sensor_i2c_write(0x3164, 0x1A); //INCKSEL6
	pssenif_vx1->sensor_i2c_write(0x317C, 0x00); //ADBIT2
	pssenif_vx1->sensor_i2c_write(0x317E, 0x00);
	pssenif_vx1->sensor_i2c_write(0x31EC, 0x0E);


	pssenif_vx1->sensor_i2c_write(0x32b8, 0x50);
	pssenif_vx1->sensor_i2c_write(0x32b9, 0x10);
	pssenif_vx1->sensor_i2c_write(0x32ba, 0x00);
	pssenif_vx1->sensor_i2c_write(0x32bb, 0x04);
	pssenif_vx1->sensor_i2c_write(0x32C8, 0x50);
	pssenif_vx1->sensor_i2c_write(0x32C9, 0x10);
	pssenif_vx1->sensor_i2c_write(0x32CA, 0x00);
	pssenif_vx1->sensor_i2c_write(0x32CB, 0x04);


	pssenif_vx1->sensor_i2c_write(0x332c, 0xD3);
	pssenif_vx1->sensor_i2c_write(0x332d, 0x10);
	pssenif_vx1->sensor_i2c_write(0x332e, 0x0D);
	pssenif_vx1->sensor_i2c_write(0x3358, 0x06);
	pssenif_vx1->sensor_i2c_write(0x3359, 0xE1);
	pssenif_vx1->sensor_i2c_write(0x335A, 0x11);
	pssenif_vx1->sensor_i2c_write(0x3360, 0x1E);
	pssenif_vx1->sensor_i2c_write(0x3361, 0x61);
	pssenif_vx1->sensor_i2c_write(0x3362, 0x10);
	pssenif_vx1->sensor_i2c_write(0x33B0, 0x50);
	pssenif_vx1->sensor_i2c_write(0x33B2, 0x1A);
	pssenif_vx1->sensor_i2c_write(0x33B3, 0x04);


	pssenif_vx1->sensor_i2c_write(0x3405, 0x10);
	pssenif_vx1->sensor_i2c_write(0x3407, 0x03);
	pssenif_vx1->sensor_i2c_write(0x3414, 0x0A);
	pssenif_vx1->sensor_i2c_write(0x3418, 0x49);
	pssenif_vx1->sensor_i2c_write(0x3419, 0x04);
	pssenif_vx1->sensor_i2c_write(0x342C, 0x47);
	pssenif_vx1->sensor_i2c_write(0x342D, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3430, 0x0F);
	pssenif_vx1->sensor_i2c_write(0x3431, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3441, 0x0C);
	pssenif_vx1->sensor_i2c_write(0x3442, 0x0C);
	pssenif_vx1->sensor_i2c_write(0x3443, 0x03);
	pssenif_vx1->sensor_i2c_write(0x3444, 0x20);
	pssenif_vx1->sensor_i2c_write(0x3445, 0x25);
	pssenif_vx1->sensor_i2c_write(0x3446, 0x57);
	pssenif_vx1->sensor_i2c_write(0x3447, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3448, 0x37);
	pssenif_vx1->sensor_i2c_write(0x3449, 0x00);
	pssenif_vx1->sensor_i2c_write(0x344A, 0x1F);
	pssenif_vx1->sensor_i2c_write(0x344B, 0x00);
	pssenif_vx1->sensor_i2c_write(0x344C, 0x1F);
	pssenif_vx1->sensor_i2c_write(0x344D, 0x00);
	pssenif_vx1->sensor_i2c_write(0x344E, 0x1F);
	pssenif_vx1->sensor_i2c_write(0x344F, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3450, 0x77);
	pssenif_vx1->sensor_i2c_write(0x3451, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3452, 0x1F);
	pssenif_vx1->sensor_i2c_write(0x3453, 0x00);
	pssenif_vx1->sensor_i2c_write(0x3454, 0x17);
	pssenif_vx1->sensor_i2c_write(0x3472, 0x9C);
	pssenif_vx1->sensor_i2c_write(0x3473, 0x07);

	pssenif_vx1->sensor_i2c_write(0x3000, 0x00); // operating
	msleep(20);
	pssenif_vx1->sensor_i2c_write(0x3002, 0x00); //Master mode start


#if 0
	// Show the Vx1 Mainlink Speed after Mainlink Locked.
	while (!pssenif_vx1->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_LOCK)) {
		msleep(10);
	}
	emu_msg(("Vx1 mainlink bus speed = %d Mhz\r\n", pssenif_vx1->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_SPEED)));
#endif

}
#endif

int nvt_dal_ssenif_api_write_reg(PSSENIF_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	unsigned long reg_addr = 0, reg_value = 0;

	if (argc != 2) {
		nvt_dbg(ERR, "wrong argument:%d", argc);
		return -EINVAL;
	}

	if (kstrtoul(pargv[0], 0, &reg_addr)) {
		nvt_dbg(ERR, "invalid reg addr:%s\n", pargv[0]);
		return -EINVAL;
	}

	if (kstrtoul(pargv[1], 0, &reg_value)) {
		nvt_dbg(ERR, "invalid rag value:%s\n", pargv[1]);
		return -EINVAL;

	}

	iowrite32(reg_value, (void __iomem *)(0xFD000000 + (reg_addr & 0xFFFFF)));
	nvt_dbg(IND, "W REG 0x%lx to 0x%lx\n", (0xFD000000 + (reg_addr & 0xFFFFF)), reg_addr);
	return 0;
}

int nvt_dal_ssenif_api_write_pattern(PSSENIF_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
#ifdef SSENIF_TEST_VX1
	PDAL_SSENIFOBJ  PSEN1DALOBJ;
	UINT32  curtime1, curtime2;

	PSEN1DALOBJ = dal_ssenif_get_object(DAL_SSENIF_ID_VX1);
	//PSEN1DALOBJ = dal_ssenif_get_object(DAL_SSENIF_ID_VX1_2);
	PSEN1DALOBJ->init();

	// Set TX_TYPE must be assign before first vx1 .open().
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_TXTYPE, DAL_SSENIFVX1_TXTYPE_THCV241);

	if (PSEN1DALOBJ->is_opened()) {
		PSEN1DALOBJ->stop();
		PSEN1DALOBJ->close();
	}

	PSEN1DALOBJ->open();

	while (1) {
		msleep(500);

		if (PSEN1DALOBJ->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_PLUG)) {
			printk("sublink ready = %d\r\n", PSEN1DALOBJ->get_vx1_config(DAL_SSENIFVX1_CFGID_GET_PLUG));
			break;
		}
	}


	/* Use Vx1 Tx's GPIO to reset sensor if needed. */
	PSEN1DALOBJ->set_vx1_gpio(DAL_SSENIFVX1_GPIO_0, 0);
	msleep(10);
	PSEN1DALOBJ->set_vx1_gpio(DAL_SSENIFVX1_GPIO_0, 1);
	msleep(20);

	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSORTYPE,         DAL_SSENIF_SENSORTYPE_SONY_NONEHDR);
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_VALID_WIDTH,        1920);
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_PIXEL_DEPTH,        12);
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_VX1LANE_NUMBER,     1);
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_MIPILANE_NUMBER,    4);
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_HDR_ENABLE,         DISABLE);
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_DATAMUX_SEL,        DAL_SSENIFVX1_DATAMUX_3BYTE_RAW);

	// The sensor target MCLK is 37.125Mhz. But the EVB only provide 24MHz.
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_TARGET_MCLK, 37125000);
	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_REAL_MCLK,   24000000);

	PSEN1DALOBJ->set_vx1_config(DAL_SSENIFVX1_CFGID_SENSOR_CKSPEED_BPS, 288000000);

	PSEN1DALOBJ->start();

	emu_ssenif_vx1_open_imx291(PSEN1DALOBJ, 0);

	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	curtime1 = hwclock_get_counter();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_VD);

	curtime2 = hwclock_get_counter();

	printk("Frame Time %llu us\n", (curtime2 - curtime1) / 10);

#endif
#ifdef SSENIF_TEST_LVDS1
	PDAL_SSENIFOBJ  PSEN1DALOBJ;
	UINT32  curtime1, curtime2;

	printk("Turn ON SSENIF: AR0237 4lane Non-HDR \n");

	PSEN1DALOBJ = dal_ssenif_get_object(DAL_SSENIF_ID_LVDS);
	PSEN1DALOBJ->init();

	if (PSEN1DALOBJ->is_opened()) {
		PSEN1DALOBJ->stop();
		PSEN1DALOBJ->close();
	}

	PSEN1DALOBJ->open();

	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_SENSORTYPE,       DAL_SSENIF_SENSORTYPE_ONSEMI);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_DLANE_NUMBER,     4);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_CLANE_SWITCH,     DAL_SSENIF_CFGID_CLANE_LVDS_USE_C0C4);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH,      12);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_STOP_METHOD,      DAL_SSENIF_STOPMETHOD_FRAME_END);

	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_WIDTH,      1920);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_HEIGHT,     1080);

	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_0,       DAL_SSENIF_LANESEL_HSI_D0);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_1,       DAL_SSENIF_LANESEL_HSI_D1);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_2,       DAL_SSENIF_LANESEL_HSI_D2);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_3,       DAL_SSENIF_LANESEL_HSI_D3);

	PSEN1DALOBJ->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT, DAL_SSENIF_LANESEL_HSI_LOW4, 0);

	PSEN1DALOBJ->start();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	curtime1 = hwclock_get_counter();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	curtime2 = hwclock_get_counter();

	printk("Frame Time %llu us\n", (curtime2 - curtime1) / 10);

#if 0
	while (1) {
		static u32 cnt;

		PSEN1DALOBJ->stop();
		PSEN1DALOBJ->start();
		if (PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND) == DAL_SSENIF_INTERRUPT_ABORT) {
			break;
		}
		curtime1 = hwclock_get_counter();
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
		curtime2 = hwclock_get_counter();
		printk("Frame Time %llu us(%d)\n", (curtime2 - curtime1) / 10, cnt++);
	}
#endif
#endif

#ifdef SSENIF_TEST_LVDS2
	PDAL_SSENIFOBJ  PSEN1DALOBJ;
	UINT32  curtime1, curtime2;

	printk("Turn ON SSENIF: AR0237 4lane HDR \n");

	PSEN1DALOBJ = dal_ssenif_get_object(DAL_SSENIF_ID_LVDS);
	PSEN1DALOBJ->init();

	if (PSEN1DALOBJ->is_opened()) {
		PSEN1DALOBJ->stop();
		PSEN1DALOBJ->close();
	}

	PSEN1DALOBJ->open();

	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_SENSORTYPE,       DAL_SSENIF_SENSORTYPE_ONSEMI);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_DLANE_NUMBER,     4);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_CLANE_SWITCH,     DAL_SSENIF_CFGID_CLANE_LVDS_USE_C0C4);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_PIXEL_DEPTH,      12);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_STOP_METHOD,      DAL_SSENIF_STOPMETHOD_FRAME_END2);

	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_WIDTH,      1920);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_VALID_HEIGHT,     1080);

	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE,     0);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_IMGID_TO_SIE2,    1);

	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_0,       DAL_SSENIF_LANESEL_HSI_D0);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_1,       DAL_SSENIF_LANESEL_HSI_D1);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_2,       DAL_SSENIF_LANESEL_HSI_D2);
	PSEN1DALOBJ->set_lvds_config(DAL_SSENIFLVDS_CFGID_OUTORDER_3,       DAL_SSENIF_LANESEL_HSI_D3);

	PSEN1DALOBJ->set_lvds_laneconfig(DAL_SSENIFLVDS_LANECFGID_SYNCCODE_DEFAULT, DAL_SSENIF_LANESEL_HSI_LOW4, 0);
	PSEN1DALOBJ->start();

	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	curtime1 = hwclock_get_counter();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	curtime2 = hwclock_get_counter();

	printk("Frame Time %ld us\n", (curtime2 - curtime1) / 10);
#endif

#ifdef SSENIF_TEST_CSI
	PDAL_SSENIFOBJ  PSEN1DALOBJ;
	UINT32  curtime1, curtime2;

#if 0 // NON HDR
	printk("Turn ON SSENIF: IMX317 4lane Non-HDR \n");

	PSEN1DALOBJ = dal_ssenif_get_object(DAL_SSENIF_ID_CSI);
	PSEN1DALOBJ->init();

	if (PSEN1DALOBJ->is_opened()) {
		PSEN1DALOBJ->stop();
		PSEN1DALOBJ->close();
	}

	PSEN1DALOBJ->open();

	//PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD,       20000);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_SENSORTYPE,         DAL_SSENIF_SENSORTYPE_SONY_NONEHDR);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK, 24000000);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK,   24000000);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DLANE_NUMBER,       4);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT,       2160);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_CLANE_SWITCH,       DAL_SSENIF_CFGID_CLANE_CSI_USE_C2);//EVB CSI1 Using C2 clock lane

	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_STOP_METHOD,        DAL_SSENIF_STOPMETHOD_FRAME_END);

	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE0_PIN,      DAL_SSENIF_LANESEL_HSI_D0);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE1_PIN,      DAL_SSENIF_LANESEL_HSI_D1);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE2_PIN,      DAL_SSENIF_LANESEL_HSI_D2);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE3_PIN,      DAL_SSENIF_LANESEL_HSI_D3);

	PSEN1DALOBJ->start();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	curtime1 = hwclock_get_counter();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND);
	curtime2 = hwclock_get_counter();

	printk("Frame Time %ld us\n", (curtime2 - curtime1) / 10);

#else //HDR
	printk("Turn ON SSENIF: IMX317 4lane HDR \n");

	PSEN1DALOBJ = dal_ssenif_get_object(DAL_SSENIF_ID_CSI);
	PSEN1DALOBJ->init();

	if (PSEN1DALOBJ->is_opened()) {
		PSEN1DALOBJ->stop();
		PSEN1DALOBJ->close();
	}

	PSEN1DALOBJ->open();

	//PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_TIMEOUT_PERIOD,       20000);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_SENSORTYPE,         DAL_SSENIF_SENSORTYPE_SONY_LI_DOL);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_TARGET_MCLK, 24000000);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_SENSOR_REAL_MCLK,   24000000);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DLANE_NUMBER,       4);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_VALID_HEIGHT,       2160);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_CLANE_SWITCH,       DAL_SSENIF_CFGID_CLANE_CSI_USE_C2);//EVB CSI1 Using C2 clock lane

	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_STOP_METHOD,        DAL_SSENIF_STOPMETHOD_FRAME_END2);

	// This IMX317 setting is 2 frames DOL HDR
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE,       1);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_IMGID_TO_SIE2,      2);

	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE0_PIN,      DAL_SSENIF_LANESEL_HSI_D0);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE1_PIN,      DAL_SSENIF_LANESEL_HSI_D1);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE2_PIN,      DAL_SSENIF_LANESEL_HSI_D2);
	PSEN1DALOBJ->set_csi_config(DAL_SSENIFCSI_CFGID_DATALANE3_PIN,      DAL_SSENIF_LANESEL_HSI_D3);

	PSEN1DALOBJ->start();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	curtime1 = hwclock_get_counter();
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	PSEN1DALOBJ->wait_interrupt(DAL_SSENIF_INTERRUPT_FRAMEEND2);
	curtime2 = hwclock_get_counter();

	printk("Frame Time %ld us\n", (curtime2 - curtime1) / 10);

#endif

#endif
	return 0;
}

int nvt_dal_ssenif_api_read_info(PSSENIF_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	ssenif_debug_dump(NULL);
	return 0;
}

int nvt_dal_ssenif_api_read_reg(PSSENIF_MODULE_INFO pmodule_info, unsigned char argc, char **pargv)
{
	int reg;

	DBG_DUMP("TOP REG (0xF0010000)::\n");
	for (reg = 0x00000; reg < 0x00100; reg += 16) {
		DBG_DUMP("0x%03X: 0x%08X 0x%08X 0x%08X 0x%08X\n", reg & 0x1FF, ioread32((void __iomem *)(0xFD010000 + (reg)))
				 , ioread32((void __iomem *)(0xFD010000 + (reg + 4))), ioread32((void __iomem *)(0xFD010000 + (reg + 8))), ioread32((void __iomem *)(0xFD010000 + (reg + 12))));
	}
	DBG_DUMP("CG REG (0xF0020000)::\n");
	for (reg = 0x00000; reg < 0x00100; reg += 16) {
		DBG_DUMP("0x%03X: 0x%08X 0x%08X 0x%08X 0x%08X\n", reg & 0x1FF, ioread32((void __iomem *)(0xFD020000 + (reg)))
				 , ioread32((void __iomem *)(0xFD020000 + (reg + 4))), ioread32((void __iomem *)(0xFD020000 + (reg + 8))), ioread32((void __iomem *)(0xFD020000 + (reg + 12))));
	}

	return 0;
}

