/*
 * zl380tw.c  --  zl380tw ALSA Soc Audio driver
 *
 * Copyright 2014 Microsemi Inc.
 *   History:
 *        2015/02/15  - created driver that combines a CHAR,
                           an ASLSA and a SPI/I2C driver in on single module
 *        2015/03/24  - Added compatibility to linux kernel 2.6.x to 3.x.x
 *                      - Verified with Linux kernels 2.6.38, and 3.10.50
 *        2015/09/30  - Added option to compile the firmware/config in c format with the kernel
 *        2015/10/7   - Updated the save config to flash to allow saving multiple config to flash
 * This program is free software you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option)any later version.
 */

#define DEBUG
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/device.h>
#include <linux/sysfs.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/version.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>

#include "zl380tw.h"
#ifdef ZL380XX_TW_UPDATE_FIRMWARE
#ifdef ZL380TW_INCLUDE_FIRMWARE_REQUEST_LIB
#include <linux/firmware.h>
#else
/*compile the firmware.c and config.c in*/
#include "zl380tw_firmware.c"
#include "zl380tw_config.c"
#endif  /*ZL380TW_INCLUDE_FIRMWARE_REQUEST_LIB*/
#endif  /*ZL380XX_TW_UPDATE_FIRMWARE*/

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#include <linux/compat.h>
#include <linux/of.h>
#include <linux/of_device.h>
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

#ifdef MICROSEMI_HBI_SPI
#include <linux/spi/spi.h>
#endif

#ifdef MICROSEMI_HBI_I2C
#include <linux/i2c.h>
#endif

#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/pcm_params.h>
#include <sound/tlv.h>
#endif

#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#endif

/*define this macro for critical region protection*/
#ifdef PROTECT_CRITICAL_SECTION
#include <linux/mutex.h>
#endif

#define MSCCDEBUG
#undef MSCCDEBUG2

#ifdef MSCCDEBUG
#define TW_DEBUG1(s, args...) \
	pr_err("%s %d: "s, __func__, __LINE__, ##args);

#else
#define TW_DEBUG1(s, args...)
#endif

#ifdef MSCCDEBUG2
#define TW_DEBUG2(s, args...) \
	pr_err("%s %d: "s, __func__, __LINE__, ##args);

#else
#define TW_DEBUG2(s, args...)
#endif


/*TWOLF HBI ACCESS MACROS-------------------------------*/
#define HBI_PAGED_READ(offset,length) \
    ((u16)(((u16)(offset) << 8) | (length)))
#define HBI_DIRECT_READ(offset,length) \
    ((u16)(0x8000 | ((u16)(offset) << 8) | (length)))
#define HBI_PAGED_WRITE(offset,length) \
    ((u16)(HBI_PAGED_READ(offset,length) | 0x0080))
#define HBI_DIRECT_WRITE(offset,length) \
    ((u16)(HBI_DIRECT_READ(offset,length) | 0x0080))
#define HBI_GLOBAL_DIRECT_WRITE(offset,length) \
    ((u16)(0xFC00 | ((offset) << 4) | (length)))
#define HBI_CONFIGURE(pinConfig) \
    ((u16)(0xFD00 | (pinConfig)))
#define HBI_SELECT_PAGE(page) \
    ((u16)(0xFE00 | (page)))
#define HBI_NO_OP \
    ((u16)0xFFFF)

/*HBI access type*/
#define TWOLF_HBI_READ 0
#define TWOLF_HBI_WRITE 1
/*HBI address type*/
#define TWOLF_HBI_DIRECT 2
#define TWOLF_HBI_PAGED  3

/* driver private data */
struct zl380tw {
#ifdef MICROSEMI_HBI_SPI
	struct spi_device	*spi;
#endif
#ifdef MICROSEMI_HBI_I2C
	struct i2c_client   *i2c;
#endif
	u8  *pData;
	int sysclk_rate;
#ifdef PROTECT_CRITICAL_SECTION
	struct list_head	device_entry;
#endif
	u8 flag;
	u8 init_done;
	int pwr_pin;
	unsigned int pwr_active;
	int rst_pin;
	unsigned int rst_active;

} *zl380tw_priv;

#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER
dev_t t_dev;

/* For registration of character device */
static struct cdev c_dev;
static ioctl_zl380tw zl380tw_ioctl_buf;
#endif
static int module_usage_count;

static unsigned twHBImaxTransferSize =(MAX_TWOLF_ACCESS_SIZE_IN_BYTES);
module_param(twHBImaxTransferSize, uint, S_IRUGO);
MODULE_PARM_DESC(twHBImaxTransferSize, "total number of data bytes >= 264");

#ifdef ZL380XX_TW_UPDATE_FIRMWARE
#ifdef ZL380TW_INCLUDE_FIRMWARE_REQUEST_LIB
static char *zl380tw_firmware_name = ZLS380TW0_TWOLF;
module_param(zl380tw_firmware_name, charp, 0000);
MODULE_PARM_DESC(zl380tw_firmware_name, "A character string");
#ifdef ZL380XX_TW_UPDATE_CONFIG
static char *zl380tw_config_name = ZLS380TW0_TWOLF_CRK;
module_param(zl380tw_config_name, charp, 0000);
MODULE_PARM_DESC(zl380tw_config_name, "A character string");
#endif /*ZL380XX_TW_UPDATE_CONFIG*/
#endif /*ZL380TW_INCLUDE_FIRMWARE_REQUEST_LIB*/
#endif /*ZL380XX_TW_UPDATE_FIRMWARE*/

/* if mutual exclusion is required for your particular platform
 * then add mutex lock/unlock to this driver
 */
#ifdef PROTECT_CRITICAL_SECTION
static DEFINE_MUTEX(lock);
static DEFINE_MUTEX(zl380tw_list_lock);
static LIST_HEAD(zl380tw_list);
#endif

/* if mutual exclusion is required for your particular platform
 * then add mutex lock/unlock to this driver
 */

static void zl380twEnterCritical(void)
{
#ifdef PROTECT_CRITICAL_SECTION
    mutex_lock(&lock);
#endif
}

static void zl380twExitCritical(void)
{
#ifdef PROTECT_CRITICAL_SECTION
    mutex_unlock(&lock);
#endif
}


/*  write up to 252 bytes data */
/*  zl380tw_nbytes_wr()- rewrite one or up to 252 bytes to the device
 *  \param[in]     ptwdata     pointer to the data to write
 *  \param[in]     numbytes    the number of bytes to write
 *
  *  return ::status = the total number of bytes transferred successfully
 *                    or a negative error code
 */
static int
zl380tw_nbytes_wr(struct zl380tw *zl380tw,
                  int numbytes, u8 *pData)
{

    int status;
#ifdef MICROSEMI_HBI_SPI
    struct spi_message msg;
	struct spi_transfer xfer = {
		.len = numbytes,
		.tx_buf = pData,
	};

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);
	status = spi_sync(zl380tw->spi, &msg);
#endif
#ifdef MICROSEMI_HBI_I2C
    status = i2c_master_send(zl380tw->i2c, pData, numbytes);
#endif
	if (status < 0)
	    return -EFAULT;

    return 0;
}

/* zl38040_hbi_rd16()- Decode the 16-bit T-WOLF Regs Host address into
 * page, offset and build the 16-bit command acordingly to the access type.
 * then read the 16-bit word data and store it in pdata
 *  \param[in]
 *                .addr     the 16-bit HBI address
 *                .pdata    pointer to the data buffer read or write
 *
 *  return ::status
 */
static int zl380tw_hbi_rd16(struct zl380tw *zl380tw,
                             u16 addr, u16 *pdata)
{

	u16 cmd;
	u8 page;
	u8 offset;
    u8 i = 0;
    u8 buf[4] = {0, 0, 0, 0};

    int status =0;
#ifdef MICROSEMI_HBI_I2C
    struct  i2c_msg     msg[2];
#endif

	page = addr >> 8;
	offset = (addr & 0xFF)>>1;

	if (page == 0) /*Direct page access*/
	{
	    cmd = HBI_DIRECT_READ(offset, 0);/*build the cmd*/
	    buf[i++] = (cmd >> 8) & 0xFF;
	    buf[i++] = (cmd & 0xFF);

	} else { /*indirect page access*/
	    if (page != 0xFF) {
	        page  -=  1;
	    }
	    cmd = HBI_SELECT_PAGE(page);
	    i = 0;
    	/*select the page*/
	    buf[i++] = (cmd >> 8) & 0xFF;
	    buf[i++] = (cmd & 0xFF);
    	cmd = HBI_PAGED_READ(offset, 0); /*build the cmd*/
	    buf[i++] = (cmd >> 8) & 0xFF;
	    buf[i++] = (cmd & 0xFF);
	}
	/*perform the HBI access*/

#ifdef MICROSEMI_HBI_SPI
	status = spi_write_then_read(zl380tw->spi, buf, i, buf, 2);
#endif
#ifdef MICROSEMI_HBI_I2C
	memset(msg,0,sizeof(msg));

    /* Make write msg 1st with write command */
    msg[0].addr    = zl380tw->i2c->addr;
    msg[0].len     = i;
    msg[0].buf     = buf;
//    TW_DEBUG2("i2c addr = 0x%04x\n", msg[0].addr);
//    TW_DEBUG2("numbytes:%d, cmdword = 0x%02x, %02x\n", numbytes, msg[0].buf[0], msg[0].buf[1]);
    /* Make read msg */
    msg[1].addr = zl380tw->i2c->addr;
    msg[1].len  = 2;
    msg[1].buf  = buf;
    msg[1].flags = I2C_M_RD;
    status = i2c_transfer(zl380tw->i2c->adapter, msg, 2);
#endif
	if (status <0)
	    return status;

    *pdata = (buf[0]<<8) | buf[1] ; /* Byte_HI, Byte_LO */
    return 0;
}

/* zl38040_hbi_wr16()- this function is used for single word access by the
 * ioctl read. It decodes the 16-bit T-WOLF Regs Host address into
 * page, offset and build the 16-bit command acordingly to the access type.
 * then write the command and data to the device
 *  \param[in]
 *                .addr      the 16-bit HBI address
 *                .data      the 16-bit word to write
 *
 *  return ::status
 */
static int zl380tw_hbi_wr16(struct zl380tw *zl380tw,
                             u16 addr, u16 data)
{

	u16 cmd;
	u8 page;
	u8 offset;
    u8 i =0;
    u8 buf[6] = {0, 0, 0, 0, 0, 0};
    int status =0;
	page = addr >> 8;
	offset = (addr & 0xFF)>>1;

	if (page == 0) /*Direct page access*/
	{
	    cmd = HBI_DIRECT_WRITE(offset, 0);/*build the cmd*/
	    buf[i++] = (cmd >> 8) & 0xFF;
	    buf[i++] = (cmd & 0xFF);

	} else { /*indirect page access*/
	    if (page != 0xFF) {
	        page  -=  1;
	    }
	    cmd = HBI_SELECT_PAGE(page);
	    i = 0;
    	/*select the page*/
	    buf[i++] = (cmd >> 8) & 0xFF;
	    buf[i++] = (cmd & 0xFF);
    	cmd = HBI_PAGED_WRITE(offset, 0); /*build the cmd*/
	    buf[i++] = (cmd >> 8) & 0xFF;
	    buf[i++] = (cmd & 0xFF);
	}
    buf[i++] = (data >> 8) & 0xFF ;
    buf[i++] = (data & 0xFF) ;

    status = zl380tw_nbytes_wr(zl380tw, i, buf);
	if (status <0)
	    return status;
    return 0;
}

/*poll a specific bit of a register for clearance
* [paran in] addr: the 16-bit register to poll
* [paran in] bit: the bit position (0-15) to poll
* [paran in] timeout: the if bit is not cleared within timeout exit loop
*/
static int zl380tw_monitor_bit(struct zl380tw *zl380tw, u16 addr, u8 bit, u16 timeout)
{
	u16 data = 0xBAD;
    do {
		zl380tw_hbi_rd16(zl380tw, addr, &data);
		msleep(10);
   } while ((((data & (1 << bit))>>bit) > 0) &&  (timeout-- >0));

	if (timeout <= 0) {
		TW_DEBUG1(" Operation Mode, in timeout = %d \n", timeout);
		return -1;
    }

	return 0;
}

/* zl380tw_reset(): use this function to reset the device.
 *
 *
 * Input Argument: mode  - the supported reset modes:
 *         VPROC_ZL38040_RST_HARDWARE_ROM,
 *         VPROC_ZL38040_RST_HARDWARE_ROM,
 *         VPROC_ZL38040_RST_SOFT,
 *         VPROC_ZL38040_RST_AEC
 *         VPROC_ZL38040_RST_TO_BOOT
 * Return:  type error code (0 = success, else= fail)
 */
static int zl380tw_reset(struct zl380tw *zl380tw,
                         u16 mode)
{

    u16 addr = CLK_STATUS_REG;
    u16 data = 0;
    int monitor_bit = -1; /*the bit (range 0 - 15) of that register to monitor*/

    /*PLATFORM SPECIFIC code*/
    if (mode  == ZL38040_RST_HARDWARE_RAM) {       /*hard reset*/
	    /*hard reset*/
        data = 0x0005;
    } else if (mode == ZL38040_RST_HARDWARE_ROM) {  /*power on reset*/
	    /*hard reset*/
        data = 0x0009;
	} else if (mode == ZL38040_RST_AEC) { /*AEC method*/
        addr = 0x0300;
        data = 0x0001;
        monitor_bit = 0;
    } else if (mode == ZL38040_RST_SOFTWARE) { /*soft reset*/
        addr = 0x0006;
        data = 0x0002;
        monitor_bit = 1;
    } else if (mode == ZL38040_RST_TO_BOOT) { /*reset to bootrom mode*/
        data = 0x0001;
    } else {
        TW_DEBUG1("Invalid reset type\n");
        return -EINVAL;
    }
    if (zl380tw_hbi_wr16(zl380tw, addr, data) < 0)
        return -EFAULT;
    msleep(50); /*wait for the HBI to settle*/

    if (monitor_bit >= 0) {
        if (zl380tw_monitor_bit(zl380tw, addr, monitor_bit, 1000) < 0)
           return -EFAULT;
    }
    return 0;
}


/* tw_mbox_acquire(): use this function to
 *   check whether the host or the device owns the mailbox right
 *
 * Input Argument: None
 * Return: error code (0 = success, else= fail)
 */
static int zl380tw_mbox_acquire(struct zl380tw *zl380tw)
{

    int status =0;
   /*Check whether the host owns the command register*/
    u16 i=0;
    u16 temp = 0x0BAD;

    for (i = 0; i < TWOLF_MBCMDREG_SPINWAIT; i++) {
        status = zl380tw_hbi_rd16(zl380tw, ZL38040_SW_FLAGS_REG, &temp);
        if ((status < 0)) {
            TW_DEBUG1("ERROR %d: \n", status);
            return status;
        }
        if (!(temp & ZL38040_SW_FLAGS_CMD)) {break;}
        msleep(10); /*release*/
        TW_DEBUG2("cmdbox =0x%04x timeout count = %d: \n", temp, i);
    }
    TW_DEBUG2("timeout count = %d: \n", i);
    if ((i>= TWOLF_MBCMDREG_SPINWAIT) && (temp != ZL38040_SW_FLAGS_CMD)) {
        return -EBUSY;
    }
    /*read the Host Command register*/
    return 0;
}

/* zl380tw_cmdreg_acquire(): use this function to
 *   check whether the last command is completed
 *
 * Input Argument: None
 * Return: error code (0 = success, else= fail)
 */
static int zl380tw_cmdreg_acquire(struct zl380tw *zl380tw)
{

    int status =0;
    /*Check whether the host owns the command register*/
    u16 i=0;
    u16 temp = 0x0BAD;

    for (i = 0; i < TWOLF_MBCMDREG_SPINWAIT; i++) {
        status = zl380tw_hbi_rd16(zl380tw, ZL38040_CMD_REG, &temp);
        if ((status < 0)) {
            TW_DEBUG1("ERROR %d: \n", status);
            return status;
        }
        if (temp == ZL38040_CMD_IDLE) {break;}
        msleep(10); /*wait*/
        TW_DEBUG2("cmdReg =0x%04x timeout count = %d: \n", temp, i);
    }
    TW_DEBUG2("timeout count = %d: \n", i);
    if ((i>= TWOLF_MBCMDREG_SPINWAIT) && (temp != ZL38040_CMD_IDLE)) {
        return -EBUSY;
    }
    /*read the Host Command register*/
    return 0;
}

/* zl380tw_write_cmdreg(): use this function to
 *   access the host command register (mailbox access type)
 *
 * Input Argument: cmd - the command to send
 * Return: error code (0 = success, else= fail)
 */

static int zl380tw_write_cmdreg(struct zl380tw *zl380tw, u16 cmd)
{

    int status = 0;
    u16 buf = cmd;
    /*Check whether the host owns the command register*/

    status = zl380tw_mbox_acquire(zl380tw);
    if ((status < 0)) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    /*write the command into the Host Command register*/

    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_REG, buf);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    if ((cmd & 0x8000) >> 15)
        buf = ZL38040_SW_FLAGS_CMD_NORST;
    else
        buf = ZL38040_SW_FLAGS_CMD;

    /*Release the command reg*/
    buf = ZL38040_SW_FLAGS_CMD;
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_SW_FLAGS_REG, buf);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    return zl380tw_cmdreg_acquire(zl380tw);
}

/* Write 16bit HBI Register */
/* zl380tw_wr16()- write a 16bit word
 *  \param[in]     cmdword  the 16-bit word to write
 *
 *  return ::status
 */
static int zl380tw_wr16(struct zl380tw *zl380tw,
                        u16 cmdword)
{

	u8 buf[2] = {(cmdword >> 8) & 0xFF, (cmdword & 0xFF)};
	int status = 0;
#ifdef MICROSEMI_HBI_SPI
	struct spi_message msg;
	struct spi_transfer xfer = {
		.len = 2,
		.tx_buf = buf,
	};

	spi_message_init(&msg);

	spi_message_add_tail(&xfer, &msg);
	status = spi_sync(zl380tw->spi, &msg);
#endif
#ifdef MICROSEMI_HBI_I2C
    status = i2c_master_send(zl380tw->i2c, buf, 2);
#endif
	return status;
}

/*To initialize the Twolf HBI interface
 * Param[in] - cmd_config  : the 16-bit HBI init command ored with the
 *                           8-bit configuration.
 *              The command format is cmd_config = 0xFD00 | CONFIG_VAL
 *              CONFIG_VAL: is you desired HBI config value
 *              (see device datasheet)
 */
static int zl380tw_hbi_init(struct zl380tw *zl380tw,
                            u16 cmd_config)
{
	return zl380tw_wr16(zl380tw, HBI_CONFIGURE(cmd_config));
}


#if defined(ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER) || defined(ZL380XX_TW_UPDATE_FIRMWARE)


static int zl380tw_cmdresult_check(struct zl380tw *zl380tw)
{
    int status = 0;
    u16 buf;
    status = zl380tw_hbi_rd16(zl380tw, ZL38040_CMD_PARAM_RESULT_REG, &buf);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    if (buf !=0) {
        TW_DEBUG1("Command failed...Resultcode = 0x%04x\n", buf);
        return buf;
    }
    return 0;
}

/*stop_fwr_to_bootmode - use this function to stop the firmware currently
 *running
 * And set the device in boot mode
 * \param[in] none
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_stop_fwr_to_bootmode(struct zl380tw *zl380tw)
{
    return zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_FWR_STOP);
}

/*start_fwr_from_ram - use this function to start/restart the firmware
 * previously stopped with VprocTwolfFirmwareStop()
 * \param[in] none
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_start_fwr_from_ram(struct zl380tw *zl380tw)
{
    if (zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_FWR_GO) < 0) {
        return -EFAULT;
    }
    zl380tw->init_done = 1; /*firmware is running in the device*/
    return 0;
}

/*tw_init_check_flash - use this function to check if there is a flash on board
 * and initialize it
 * \param[in] none
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_init_check_flash(struct zl380tw *zl380tw)
{
     /*if there is a flash on board initialize it*/
    return zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_HOST_FLASH_INIT);
}

/*tw_erase_flash - use this function to erase all firmware image
* and related  config from flash
 * previously stopped with VprocTwolfFirmwareStop()
 * \param[in] none
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_erase_flash(struct zl380tw *zl380tw)
{

    int status =0;
     /*if there is a flash on board initialize it*/
    status = zl380tw_init_check_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    /*erase all config/fwr*/
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_PARAM_RESULT_REG, 0xAA55);
    if (status < 0) {
        return status;
    }

    /*erase firmware*/
    return zl380tw_write_cmdreg(zl380tw, 0x0009);

}
/*erase_fwrcfg_from_flash - use this function to erase a psecific firmware image
* and related  config from flash
 * previously stopped with VprocTwolfFirmwareStop()
 * Input Argument: image_number   - (range 1-14)
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_erase_fwrcfg_from_flash(struct zl380tw *zl380tw,
                                                           u16 image_number)
{
    int status = 0;
    if (image_number <= 0) {
        return -EINVAL;
    }
    /*save the config/fwr to flash position image_number */
    status = zl380tw_stop_fwr_to_bootmode(zl380tw);
    if (status < 0) {
        return status;
    }
    msleep(50);
     /*if there is a flash on board initialize it*/
    status = zl380tw_init_check_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_PARAM_RESULT_REG, image_number);
    if (status < 0) {
        return status;
    }
    status  = zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_IMG_CFG_ERASE);
    if (status < 0) {
        return status;
    }

    return zl380tw_cmdresult_check(zl380tw);
}


/* tw_save_image_to_flash(): use this function to
 *     save both the config record and the firmware to flash. It Sets the bit
 *     which initiates a firmware save to flash when the device
 *     moves from a STOPPED state to a RUNNING state (by using the GO bit)
 *
 * Input Argument: None
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int zl380tw_save_image_to_flash(struct zl380tw *zl380tw)
{
    int status = 0;
    /*Save firmware to flash*/

#if 0
     /*delete all applications on flash*/
    status = zl380tw_erase_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: tw_erase_flash\n", status);
        return status;
    }
#else
     /*if there is a flash on board initialize it*/
    status = zl380tw_init_check_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
#endif
    /*save the image to flash*/
    status = zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_IMG_CFG_SAVE);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: zl380tw_write_cmdreg\n", status);
        return status;
    }
    return zl380tw_cmdresult_check(zl380tw);
    /*return status;*/
}

/*load_fwr_from_flash - use this function to load a specific firmware image
* from flash
 * previously stopped with VprocTwolfFirmwareStop()
 * Input Argument: image_number   - (range 1-14)
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_load_fwr_from_flash(struct zl380tw *zl380tw,
                                                       u16 image_number)
{
    int status = 0;

    if (image_number <= 0) {
        return -EINVAL;
    }

    status = zl380tw_stop_fwr_to_bootmode(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

     /*if there is a flash on board initialize it*/
    status = zl380tw_init_check_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    /*load the fwr to flash position image_number */
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_PARAM_RESULT_REG, image_number);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    status = zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_IMG_LOAD);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    return zl380tw_start_fwr_from_ram(zl380tw);
}

/*zl380tw_load_fwrcfg_from_flash - use this function to load a specific firmware image
* related and config from flash
 * previously stopped with VprocTwolfFirmwareStop()
 * Input Argument: image_number   - (range 1-14)
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_load_fwrcfg_from_flash(struct zl380tw *zl380tw,
                                                  u16 image_number)
{

    int status = 0;
    if (image_number <= 0) {
        return -EINVAL;
    }
    status = zl380tw_stop_fwr_to_bootmode(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

     /*if there is a flash on board initialize it*/
    status = zl380tw_init_check_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    /*save the config to flash position image_number */
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_PARAM_RESULT_REG, image_number);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    status = zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_IMG_CFG_LOAD);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    return zl380tw_start_fwr_from_ram(zl380tw);
}



/*zl380tw_load_cfg_from_flash - use this function to load a specific firmware image
* from flash
 *  * Input Argument: image_number   - (range 1-14)
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_load_cfg_from_flash(struct zl380tw *zl380tw,
                                                    u16 image_number)
{
    int status = 0;

    if (image_number <= 0) {
        return -EINVAL;
    }
     /*if there is a flash on board initialize it*/
    status = zl380tw_init_check_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    /*load the config from flash position image_number */
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_PARAM_RESULT_REG, image_number);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    return zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_CFG_LOAD);
}

/* save_cfg_to_flash(): use this function to
 *     save the config record to flash. It Sets the bit
 *     which initiates a config save to flash when the device
 *     moves from a STOPPED state to a RUNNING state (by using the GO bit)
 *
 * \retval ::0 success
 * \retval ::-EINVAL or device error code
 */
static int  zl380tw_save_cfg_to_flash(struct zl380tw *zl380tw, u16 image_number)
{
    int status = 0;
    u16 buf = 0;
    /*Save config to flash*/

     /*if there is a flash on board initialize it*/
    status = zl380tw_init_check_flash(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    /*check if a firmware exists on the flash*/
    status = zl380tw_hbi_rd16(zl380tw, ZL38040_FWR_COUNT_REG, &buf);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    if (buf ==0)
        return -EBADSLT; /*There is no firmware on flash position image_number to save config for*/

    /*save the config to flash position */
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_PARAM_RESULT_REG, image_number);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    /*save the config to flash position */
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_CMD_REG, 0x8002);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    /*save the config to flash position */
    status = zl380tw_hbi_wr16(zl380tw, ZL38040_SW_FLAGS_REG, 0x0004);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    status = zl380tw_cmdreg_acquire(zl380tw);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: tw_cmdreg_acquire\n", status);
        return status;
    }
    /*Verify wheter the operation completed sucessfully*/
    return zl380tw_cmdresult_check(zl380tw);
}

/*AsciiHexToHex() - to convert ascii char hex to integer hex
 * pram[in] - str - pointer to the char to convert.
 * pram[in] - len - the number of character to convert (2:u8, 4:u16, 8:u32).

 */
static unsigned int AsciiHexToHex(const char * str, unsigned char len)
{
    unsigned int val = 0;
    char c;
    unsigned char i = 0;
    for (i = 0; i< len; i++)
    {
        c = *str++;
		val <<= 4;

        if (c >= '0' && c <= '9')
        {
            val += c & 0x0F;
            continue;
        }

        c &= 0xDF;
        if (c >= 'A' && c <= 'F')
        {
            val += (c & 0x07) + 9;
            continue;
        }
        return 0;
    }
    return val;
}

/* These 3 functions provide an alternative method to loading an *.s3
 *  firmware image into the device
 * Procedure:
 * 1- Call zl380tw_boot_prepare() to put the device in boot mode
 * 2- Call the zl380tw_boot_Write() repeatedly by passing it a pointer
 *    to one line of the *.s3 image at a time until the full image (all lines)
 *    are transferred to the device successfully.
 *    When the transfer of a line is complete, this function will return the sta-
 *    tus VPROC_STATUS_BOOT_LOADING_MORE_DATA. Then when all lines of the image
 *    are transferred the function will return the status
 *         VPROC_STATUS_BOOT_LOADING_CMP
 * 3- zl380tw_boot_conclude() to complete and verify the completion status of
 *    the image boot loading process
 *
 */
static int zl380tw_boot_prepare(struct zl380tw *zl380tw)
{

    u16 buf = 0;
    int status = 0;
	status = zl380tw_hbi_wr16(zl380tw, CLK_STATUS_REG, 1); /*go to boot rom mode*/
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }
    msleep(50); /*required for the reset to cmplete*/
    /*check whether the device has gone into boot mode as ordered*/
    status = zl380tw_hbi_rd16(zl380tw, (u16)ZL38040_CMD_PARAM_RESULT_REG, &buf);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    if ((buf != 0xD3D3)) {
        TW_DEBUG1("ERROR: HBI is not accessible, cmdResultCheck = 0x%04x\n",
		buf);
        return  -EFAULT;
    }
	return 0;
}
/*----------------------------------------------------------------------------*/

static int zl380tw_boot_conclude(struct zl380tw *zl380tw)
{
    int status = 0;
	status = zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_HOST_LOAD_CMP); /*loading complete*/
    if (status < 0) {
        TW_DEBUG1("ERROR %d: \n", status);
        return status;
    }

    /*check whether the device has gone into boot mode as ordered*/
    return zl380tw_cmdresult_check(zl380tw);
}

/*----------------------------------------------------------------------------*/
/*  Read up to 256 bytes data */
/*  slave_zl380xx_nbytesrd()- read one or up to 256 bytes from the device
 *  \param[in]     ptwdata     pointer to the data read
 *  \param[in]     numbytes    the number of bytes to read
 *
 *  return ::status = the total number of bytes received successfully
 *                   or a negative error code
 */
static int
zl380tw_nbytes_rd(struct zl380tw *zl380tw, u8 numbytes, u8 *pData, u8 hbiAddrType)
{
    int status = 0;
    int tx_len = (hbiAddrType == TWOLF_HBI_PAGED) ? 4 : 2;
	u8 tx_buf[4] = {pData[0], pData[1], pData[2], pData[3]};

#ifdef MICROSEMI_HBI_SPI
    struct spi_message msg;

	struct spi_transfer txfer = {
		.len = tx_len,
		.tx_buf = tx_buf,
	};
    struct spi_transfer rxfer = {
		.len = numbytes,
		.rx_buf = zl380tw->pData,
	};

	spi_message_init(&msg);
	spi_message_add_tail(&txfer, &msg);
	spi_message_add_tail(&rxfer, &msg);
	status = spi_sync(zl380tw->spi, &msg);

#endif
#ifdef MICROSEMI_HBI_I2C
    struct  i2c_msg     msg[2];
	memset(msg,0,sizeof(msg));

    msg[0].addr    = zl380tw->i2c->addr;
    msg[0].len     = tx_len;
    msg[0].buf     = tx_buf;

    msg[1].addr = zl380tw->i2c->addr;
    msg[1].len  = numbytes;
    msg[1].buf  = zl380tw->pData;
    msg[1].flags = I2C_M_RD;
    status = i2c_transfer(zl380tw->i2c->adapter, msg, 2);
#endif
	if (status <0)
	    return status;

#ifdef MSCCDEBUG2
    {
        int i = 0;
        printk("RD: Numbytes = %d, addrType = %d\n", numbytes, hbiAddrType);
        for(i=0;i<numbytes;i++)
        {
            printk("0x%02x, ", zl380tw->pData[i]);
        }
        printk("\n");
    }
#endif
    return 0;
}


static int
zl380tw_hbi_access(struct zl380tw *zl380tw,
       u16 addr, u16 numbytes, u8 *pData, u8 hbiAccessType)
{

	u16 cmd;
	u8 page = addr >> 8;
	u8 offset = (addr & 0xFF)>>1;
	int i = 0;
	u8 hbiAddrType = 0;
	u8 buf[MAX_TWOLF_ACCESS_SIZE_IN_BYTES];
	int status = 0;

	u8 numwords = (numbytes/2);

#ifdef MICROSEMI_HBI_SPI
	if (zl380tw->spi == NULL) {
	    TW_DEBUG1("spi device is not available \n");
	    return -EFAULT;
    }
#endif
#ifdef MICROSEMI_HBI_I2C
	if (zl380tw->i2c == NULL) {
	    TW_DEBUG1("i2c device is not available \n");
	    return -EFAULT;
    }
#endif
	if ((numbytes > MAX_TWOLF_ACCESS_SIZE_IN_BYTES) || (numbytes ==0))
	    return -EINVAL;

	if (pData == NULL)
	    return -EINVAL;

    if (!((hbiAccessType == TWOLF_HBI_WRITE) ||
          (hbiAccessType == TWOLF_HBI_READ)))
         return -EINVAL;

	if (page == 0) /*Direct page access*/
	{
	    if (hbiAccessType == TWOLF_HBI_WRITE)
           cmd = HBI_DIRECT_WRITE(offset, numwords-1);/*build the cmd*/
        else
           cmd = HBI_DIRECT_READ(offset, numwords-1);/*build the cmd*/

	    buf[i++] = (cmd >> 8) & 0xFF ;
        buf[i++] = (cmd & 0xFF) ;
        hbiAddrType = TWOLF_HBI_DIRECT;
	} else { /*indirect page access*/
	    i = 0;
	    if (page != 0xFF) {
	        page  -=  1;
	    }
	    cmd = HBI_SELECT_PAGE(page);
    	/*select the page*/
		buf[i++] = (cmd >> 8) & 0xFF ;
        buf[i++] = (cmd & 0xFF) ;

        /*address on the page to access*/
        if (hbiAccessType == TWOLF_HBI_WRITE)
    	   cmd = HBI_PAGED_WRITE(offset, numwords-1);
        else
    	   cmd = HBI_PAGED_READ(offset, numwords-1);/*build the cmd*/

		 buf[i++] = (cmd >> 8) & 0xFF ;
         buf[i++] = (cmd & 0xFF) ;
    	 hbiAddrType = TWOLF_HBI_PAGED;
	}
	memcpy(&buf[i], pData, numbytes);
#ifdef MSCCDEBUG2
    {
        int j = 0;
        int displaynum = numbytes;
        if (hbiAccessType == TWOLF_HBI_WRITE)
        	displaynum = numbytes;
        else
        	displaynum = i;

        printk("SENDING:: Numbytes = %d, accessType = %d\n", numbytes, hbiAccessType);
        for(j=0;j<numbytes;j++)
        {
            printk("0x%02x, ", pData[j]);
        }
        printk("\n");
    }
#endif
	if (hbiAccessType == TWOLF_HBI_WRITE)
       status = zl380tw_nbytes_wr(zl380tw, numbytes+i, buf);
    else
       status = zl380tw_nbytes_rd(zl380tw, numbytes, buf, hbiAddrType);
    if (status < 0)
        return -EFAULT;

    return status;
}
/*
 * __ Upload a romcode  by blocks
 * the user app will call this function by passing it one line from the *.s3
 * file at a time.
 * when the firmware boot loading process find the execution address
 * in that block of data it will return the number 23
 * indicating that the transfer on the image data is completed, otherwise,
 * it will return 22 indicating tha tit expect more data.
 * If error, then a negative error code will be reported
 */
static int zl380tw_boot_Write(struct zl380tw *zl380tw,
                              char *blockOfFwrData) /*0: HBI; 1:FLASH*/
{

/*Use this method to load the actual *.s3 file line by line*/
    int status = 0;
    int rec_type, i=0, j=0;
    u8 numbytesPerLine = 0;
    u8 buf[MAX_TWOLF_FIRMWARE_SIZE_IN_BYTES];
    unsigned long address = 0;
    u8 page255Offset = 0x00;
    u16 cmd = 0;

    TW_DEBUG2("firmware line# = %d :: blockOfFwrData = %s\n",++myCounter, blockOfFwrData);

    if (blockOfFwrData == NULL) {
       TW_DEBUG1("blockOfFwrData[0] = %c\n", blockOfFwrData[0]);
       return -EINVAL;
    }
    /* if this line is not an srecord skip it */
    if (blockOfFwrData[0] != 'S') {
        TW_DEBUG1("blockOfFwrData[0] = %c\n", blockOfFwrData[0]);
        return -EINVAL;
    }
    /* get the srecord type */
    rec_type = blockOfFwrData[1] - '0';

    numbytesPerLine = AsciiHexToHex(&blockOfFwrData[2], 2);
    //TW_DEBUG2("numbytesPerLine = %d\n", numbytesPerLine);
    if (numbytesPerLine == 0) {
         TW_DEBUG1("blockOfFwrData[3] = %c\n", blockOfFwrData[3]);
         return -EINVAL;
    }

    /* skip non-existent srecord types and block header */
    if ((rec_type == 4) || (rec_type == 5) || (rec_type == 6) ||
                                              (rec_type == 0)) {
        return TWOLF_STATUS_NEED_MORE_DATA;
    }

    /* get the info based on srecord type (skip checksum) */
    address = AsciiHexToHex(&blockOfFwrData[4], 8);
    buf[0] = (u8)((address >> 24) & 0xFF);
    buf[1] = (u8)((address >> 16) & 0xFF);
    buf[2] = (u8)((address >> 8) & 0xFF);
    buf[3] = 0;
    page255Offset = (u8)(address & 0xFF);
    /* store the execution address */
    if ((rec_type == 7) || (rec_type == 8) || (rec_type == 9)) {
        /* the address is the execution address for the program */
        //TW_DEBUG2("execAddr = 0x%08lx\n", address);
        /* program the program's execution start register */
        buf[3] = (u8)(address & 0xFF);
        status = zl380tw_hbi_access(zl380tw,
                       ZL38040_FWR_EXEC_REG, 4, buf, TWOLF_HBI_WRITE);

        if(status < 0) {
            TW_DEBUG1("ERROR % d: unable to program page 1 execution address\n", status);
            return status;
        }
        TW_DEBUG2("Loading firmware data complete...\n");
        return TWOLF_STATUS_BOOT_COMPLETE;  /*BOOT_COMPLETE Sucessfully*/
    }

    /* put the address into our global target addr */


    //TW_DEBUG2("TW_DEBUG2:gTargetAddr = 0x%08lx: \n", address);
    status = zl380tw_hbi_access(zl380tw,
                       PAGE_255_BASE_HI_REG, 4, buf, TWOLF_HBI_WRITE);
    if (status < 0) {
        TW_DEBUG1("ERROR %d: gTargetAddr = 0x%08lx: \n", status, address);
        return -EFAULT;
    }

    /* get the data bytes */
    j = 12;
    //TW_DEBUG2("buf[]= 0x%02x, 0x%02x, \n", buf[0], buf[1]);
    for (i = 0; i < numbytesPerLine - 5; i++) {
        buf[i] = AsciiHexToHex(&blockOfFwrData[j], 2);
        j +=2;
        //TW_DEBUG2("0x%02x, ", buf[i+4]);
    }
    /* write the data to the device */
    cmd = (u16)(0xFF<<8) | (u16)page255Offset;
    status = zl380tw_hbi_access(zl380tw,
                       cmd, (numbytesPerLine - 5), buf, TWOLF_HBI_WRITE);
    if(status < 0) {
        return status;
    }

    //TW_DEBUG2("Provide next block of data...\n");
    return TWOLF_STATUS_NEED_MORE_DATA; /*REQUEST STATUS_MORE_DATA*/
}

#endif


/*----------------------------------------------------------------------*
 *   The kernel driver functions are defined below
 *-------------------------DRIVER FUNCTIONs-----------------------------*/
/* zl380tw_ldfwr()
 * This function basically  will load the firmware into the Timberwolf device
 * at power up. this is convenient for host pluging system that does not have
 * a slave EEPROM/FLASH to store the firmware, therefore, and that
 * requires the device to be fully operational at power up
*/
#ifdef ZL380XX_TW_UPDATE_FIRMWARE
#ifdef ZL380TW_INCLUDE_FIRMWARE_REQUEST_LIB
static int zl380tw_ldfwr(struct zl380tw *zl380tw)
{

    int status = 0;

    const struct firmware *twfw;
    u8 numbytesPerLine = 0;
    u32 j =0;
    u8 block_size = 0;

    zl380twEnterCritical();

    if (ZLS380TW0_TWOLF == NULL){
          TW_DEBUG1("err %d, invalid firmware filename %s\n",
                                                       status, ZLS380TW0_TWOLF);
          zl380twExitCritical();
          return -EINVAL;
    }

    status = request_firmware(&twfw, ZLS380TW0_TWOLF,
#ifdef MICROSEMI_HBI_SPI
    &zl380tw->spi->dev
#endif
#ifdef MICROSEMI_HBI_I2C
    &zl380tw->i2c->dev
#endif
    );
    if (status) {
          TW_DEBUG1("err %d, request_firmware failed to load %s\n",
                                                       status, ZLS380TW0_TWOLF);
          zl380twExitCritical();
          return -EINVAL;
    }

    /*check validity of the S-record firmware file*/
    if (twfw->data[0] != 'S') {
         TW_DEBUG1("Invalid S-record %s file for this device\n", ZLS380TW0_TWOLF);
          release_firmware(twfw);
          zl380twExitCritical();
          return -EINVAL;
    }

    /* Allocating memory for the data buffer if not already done*/
    if (!zl380tw->pData) {
        zl380tw->pData  = kmalloc(MAX_TWOLF_FIRMWARE_SIZE_IN_BYTES, GFP_KERNEL);
        if (!zl380tw->pData) {
    		TW_DEBUG1("can't allocate memory\n");
            release_firmware(twfw);
            zl380twExitCritical();
            return -ENOMEM;
        }
        memset(zl380tw->pData, 0, MAX_TWOLF_FIRMWARE_SIZE_IN_BYTES);
    }

    status = zl380tw_boot_prepare(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, tw boot prepare failed\n", status);
          goto fwr_cleanup;
    }

    TW_DEBUG1("loading firmware %s into the device ...\n", ZLS380TW0_TWOLF);

    do {
		numbytesPerLine = AsciiHexToHex(&twfw->data[j+2], 2);
        block_size = (4 + (2*numbytesPerLine));

        memcpy(zl380tw->pData, &twfw->data[j], block_size);
		j += (block_size+2);

        status = zl380tw_boot_Write(zl380tw, zl380tw->pData);
        if ((status != (int)TWOLF_STATUS_NEED_MORE_DATA) &&
            (status != (int)TWOLF_STATUS_BOOT_COMPLETE)) {
              TW_DEBUG1("err %d, tw boot write failed\n", status);
              goto fwr_cleanup;
        }

    } while ((j < twfw->size) && (status != TWOLF_STATUS_BOOT_COMPLETE));

    status = zl380tw_boot_conclude(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, twfw->size = %d, tw boot conclude -firmware loading failed\n", status, twfw->size);
          goto fwr_cleanup;
    }
#ifdef ZL38040_SAVE_FWR_TO_FLASH
    status = zl380tw_save_image_to_flash(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, twfw->size = %d, saving firmware failed\n", status, twfw->size);
          goto fwr_cleanup;
    }
#endif  /*ZL38040_SAVE_FWR_TO_FLASH*/
    status = zl380tw_start_fwr_from_ram(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, twfw->size = %d, starting firmware failed\n", status, twfw->size);
          goto fwr_cleanup;
    }
/*loading the configuration record*/
#ifdef ZL380XX_TW_UPDATE_CONFIG
   {
        u16 reg = 0  ,val =0;
        release_firmware(twfw);
        j = 6; /*skip the CR2K key word*/
        numbytesPerLine = 10;
        status = request_firmware(&twfw, ZLS380TW0_TWOLF_CRK,
    #ifdef MICROSEMI_HBI_SPI
        &zl380tw->spi->dev
    #endif
    #ifdef MICROSEMI_HBI_I2C
        &zl380tw->i2c->dev
    #endif
        );
        if (status) {
              TW_DEBUG1("err %d, request_firmware failed to load %s\n",
                                                           status, ZLS380TW0_TWOLF_CRK);
              goto fwr_cleanup;
        }
        TW_DEBUG1("loading config %s into the device ...\n", ZLS380TW0_TWOLF_CRK);

        do {
            if(twfw->data[j] == 'E')
                 break;

            if((twfw->data[j] != 'C') && (twfw->data[j+1] != 'R')) {
        		reg = AsciiHexToHex(&twfw->data[j], 4);
                val = AsciiHexToHex(&twfw->data[j+6], 4);
                status = zl380tw_hbi_wr16(zl380tw, reg, val);
                if (status < 0) {
                      TW_DEBUG1("err %d, tw config write failed\n", status);
                      goto fwr_cleanup;
                }
                //TW_DEBUG1("reg = 0x%04x, val=0x%04x\n", reg, val);
            }
    		j += 12;   /*move to next pair of values*/

        } while (j < twfw->size);
#ifdef ZL38040_SAVE_FWR_TO_FLASH
    	if (zl380tw_hbi_rd16(zl380tw, ZL38040_FWR_COUNT_REG, &val) < 0) {
    		return -EIO;
        }
        if (val == 0) {
             TW_DEBUG1("err: there is no firmware on flash\n");
             return -1;
        }

        status = zl380tw_save_cfg_to_flash(zl380tw, val);
        if (status < 0) {
              TW_DEBUG1("err %d, tw save config to flash failed\n", status);
        }
        TW_DEBUG1("zl380tw image/cfg saved to flash position %d - success...\n", val);

#endif   /*ZL38040_SAVE_FWR_TO_FLASH*/
    }
#endif  /*ZL380XX_TW_UPDATE_CONFIG*/
	 /*status = zl380tw_reset(zl380tw, ZL38040_RST_SOFTWARE); */
    goto fwr_cleanup;

fwr_cleanup:
    zl380twExitCritical();
    release_firmware(twfw);

    return status;
}
#else
/*VprocTwolfLoadConfig() - use this function to load a custom or new config
 * record into the device RAM to override the default config
 * \retval ::VPROC_STATUS_SUCCESS
 * \retval ::VPROC_STATUS_ERR_HBI
 */
static int zl380tw_load_config(struct zl380tw *zl380tw,
    dataArr *pCr2Buf,
    unsigned short numElements)
{
    int status = 0;
    int i;
    u8 buf[MAX_TWOLF_FIRMWARE_SIZE_IN_BYTES];
    /*stop the current firmware but do not reset the device and do not go to boot mode*/

    /*send the config to the device RAM*/
    for (i=0; i<numElements; i++) {
        u8 j=0, k=0;
        for (k = 0; k < ZL380XX_CFG_BLOCK_SIZE; k++) {
            buf[j] = (u8)((st_twConfig[i].value[k] >> 8) & 0xFF);
            buf[j+1] = (u8)((st_twConfig[i].value[k]) & 0xFF);
            j +=2;
        }
        status = zl380tw_hbi_access(zl380tw, st_twConfig[i].reg, ZL380XX_CFG_BLOCK_SIZE*2, buf, TWOLF_HBI_WRITE);
    }

    return status;
}
/* zl380tw_boot_alt Use this alternate method to load the st_twFirmware.c
 *(converted *.s3 to c code) to the device
 */
static int zl380tw_ldfwr(struct zl380tw *zl380tw)
{
    u16 index = 0;
    u8 buf[MAX_TWOLF_FIRMWARE_SIZE_IN_BYTES];
    u8 page255Offset = 0x00;

    int status = 0;

    TW_DEBUG1("Using the firmware-config c code loading ...\n");
    zl380twEnterCritical();
    status = zl380tw_boot_prepare(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, tw boot prepare failed\n", status);
          zl380twExitCritical();
          return -1;
    }

    while (index < firmwareStreamLen) {
        int i=0, j=0;
		/* put the address into our global target addr */
        buf[0] = (u8)((st_twFirmware[index].targetAddr >> 24) & 0xFF);
        buf[1] = (u8)((st_twFirmware[index].targetAddr >> 16) & 0xFF);
        buf[2] = (u8)((st_twFirmware[index].targetAddr >> 8) & 0xFF);
        buf[3] = 0;
		page255Offset = (u8)(st_twFirmware[index].targetAddr & 0xFF);
		/* write the data to the device */
		if (st_twFirmware[index].numWords != 0) {


            status = zl380tw_hbi_access(zl380tw,
                       PAGE_255_BASE_HI_REG, 4, buf, TWOLF_HBI_WRITE);
            if (status < 0) {
                TW_DEBUG1("ERROR %d: gTargetAddr = 0x%08lx: \n", status, st_twFirmware[index].targetAddr);
                zl380twExitCritical();
                return -EFAULT;
            }



            for (i = 0; i < st_twFirmware[index].numWords; i++) {
                buf[j] = (u8)((st_twFirmware[index].buf[i] >> 8) & 0xFF);
                buf[j+1] = (u8)((st_twFirmware[index].buf[i]) & 0xFF);
                j+=2;
                //TW_DEBUG2("0x%02x, ", buf[i+4]);
            }
            /* write the data to the device */

            status = zl380tw_hbi_access(zl380tw,((u16)(0xFF<<8) | (u16)page255Offset),
                               (st_twFirmware[index].numWords*2), buf, TWOLF_HBI_WRITE);
            if(status < 0) {
                zl380twExitCritical();
                return status;
            }
		}
		index++;
    }

    /*
     * convert the number of bytes to two 16 bit
     * values and write them to the requested page register
     */
    /* even number of bytes required */

    /* program the program's execution start register */
    buf[0] = (u8)((executionAddress >> 24) & 0xFF);
    buf[1] = (u8)((executionAddress >> 16) & 0xFF);
    buf[2] = (u8)((executionAddress >> 8) & 0xFF);
    buf[3] = (u8)(executionAddress & 0xFF);
    status = zl380tw_hbi_access(zl380tw,
                   ZL38040_FWR_EXEC_REG, 4, buf, TWOLF_HBI_WRITE);

    if(status < 0) {
        TW_DEBUG1("ERROR % d: unable to program page 1 execution address\n", status);
        zl380twExitCritical();
        return status;
    }

    /* print out the srecord program info */
    TW_DEBUG2("prgmBase 0x%08lx\n", programBaseAddress);
    TW_DEBUG2("execAddr 0x%08lx\n", executionAddress);

    status = zl380tw_boot_conclude(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, tw boot conclude -firmware loading failed\n", status);
          zl380twExitCritical();
          return -1;
    }

#ifdef ZL38040_SAVE_FWR_TO_FLASH
    status = zl380tw_save_image_to_flash(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, saving firmware to flash failed\n", status);
          zl380twExitCritical();
          return status;
    }
#endif  /*ZL38040_SAVE_FWR_TO_FLASH*/
    status = zl380tw_start_fwr_from_ram(zl380tw);
    if (status < 0) {
          TW_DEBUG1("err %d, starting firmware failed\n", status);
          zl380twExitCritical();
          return status;
    }
#ifdef ZL380XX_TW_UPDATE_CONFIG
    zl380tw_load_config(zl380tw, (dataArr *)st_twConfig, (u16)configStreamLen);
    if (status < 0) {
          TW_DEBUG1("err %d, loading config failed\n", status);
          zl380twExitCritical();
          return status;
    }
#ifdef ZL38040_SAVE_FWR_TO_FLASH
    {
    	u16 val = 0;
        if (zl380tw_hbi_rd16(zl380tw, ZL38040_FWR_COUNT_REG, &val) < 0) {
    		return -EIO;
        }
        if (val == 0) {
             TW_DEBUG1("err: there is no firmware on flash\n");
             return -1;
        }
        status = zl380tw_save_cfg_to_flash(zl380tw, val);
        if (status < 0) {
              TW_DEBUG1("err %d, tw save config to flash failed\n", status);
              zl380twExitCritical();
              return status;
        }
        TW_DEBUG1("zl380tw image/cfg saved to flash position %d - success...\n", val);
    }
#endif   /*ZL38040_SAVE_FWR_TO_FLASH*/
#endif   /*ZL380XX_TW_UPDATE_CONFIG*/
    TW_DEBUG1("zl380tw boot init complete - success...\n");
    zl380twExitCritical();
    return 0;
}
#endif  /*ZL380TW_INCLUDE_FIRMWARE_REQUEST_LIB*/
#endif  /*ZL380XX_TW_UPDATE_FIRMWARE*/


/*--------------------------------------------------------------------
 *    ALSA  SOC CODEC driver
 *--------------------------------------------------------------------*/

#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER

/* ALSA soc codec default Timberwolf register settings
 * 3 Example audio cross-point configurations
 */
/* ALSA soc codec default Timberwolf register settings
 * 3 Example audio cross-point configurations
 */
#define CODEC_CONFIG_REG_NUM 42

u8 reg_stereo[] ={ 0x00, 0x00, 0x00, 0x06, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};


/*4-port m
ode with AEC enabled  - ROUT to DAC1
 * MIC1 -> SIN (ADC)
 * I2S-L -> RIN (TDM Rx path)
 * SOUT -> I2S-L
 * ROUT -> DACx
 *reg 0x202 - 0x22A
 */
u8 reg_cache_default[CODEC_CONFIG_REG_NUM] ={0x0c, 0x05, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                0x00, 0x0d, 0x00, 0x0d, 0x00, 0x0e, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x0d,
                0x00, 0x00, 0x00, 0x00, 0x00, 0x0d, 0x00, 0x01, 0x06, 0x05, 0x00, 0x02, 0x00, 0x00};



/*Formatting for the Audio*/
#define zl380tw_DAI_RATES            (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000 | SNDRV_PCM_RATE_48000)
#define zl380tw_DAI_FORMATS          (SNDRV_PCM_FMTBIT_S16_LE)
#define zl380tw_DAI_CHANNEL_MIN      1
#define zl380tw_DAI_CHANNEL_MAX      2

static void zl380tw_fill_codec_cache(struct snd_soc_codec *codec,
            u8* pData, int numbytes)
{
     int i = 0;
     u8* cache = codec->reg_cache;
     for (i=0; i< numbytes; i++) {
         cache[i] = pData[i];
         //dev_info(codec->dev, "pData[%d]=0x%02x, cache[%d]=0x%02x\n", i, pData[i], i, cache[i]);
    }
}

static u16 zl380tw_read_codec_cache(struct snd_soc_codec *codec, u16 reg)
{
     u8 i = reg - ZL38040_CACHED_ADDR_LO;
     if (i < CODEC_CONFIG_REG_NUM) {
         u8* cache = codec->reg_cache;
         //dev_info(codec->dev, "cache: 0x%04x = 0x%04x\n", reg, (cache[i] << 8) | cache[i+1]);
         return (cache[i] << 8) | cache[i+1];
     }
    return 0xFBAD;
}

static int zl380tw_find_index_codec_cache(struct snd_soc_codec *codec, u8 val)
{
     int i = 0;
     u8* cache = codec->reg_cache;
     /*start the search from reg 210h*/
     for (i=14; i< CODEC_CONFIG_REG_NUM; i++) {
         if (cache[i] == val) {
             //dev_info(codec->dev, "cache[%d]=0x%02x\n", i, cache[i]);
             if (i%2)
               --i;
             return i;
         }
    }
    return -1;
}

static unsigned int zl380tw_reg_read(struct snd_soc_codec *codec,
            unsigned int reg)
{
    unsigned int value = 0;

    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
    u16 buf;
	if (zl380tw_hbi_rd16(zl380tw, reg, &buf) < 0) {
		return -EIO;
    }
    value = buf;

    return value;

}

static int zl380tw_reg_write(struct snd_soc_codec *codec,
            unsigned int reg, unsigned int value)
{
	struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);

	if (zl380tw_hbi_wr16(zl380tw, reg, value) < 0) {
		return -EIO;
    }
    return 0;
}


/*ALSA- soc codec I2C/SPI read control functions*/
static int zl380tw_control_read(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{
    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int mask = mc->max;
	unsigned int invert = mc->invert;
	u16 val;


	zl380twEnterCritical();
	if (zl380tw_hbi_rd16(zl380tw, reg, &val)) {
	    zl380twExitCritical();
		return -EIO;
    }
    zl380twExitCritical();

	ucontrol->value.integer.value[0] = ((val >> shift) & mask);

	if (invert)
		ucontrol->value.integer.value[0] =
			mask - ucontrol->value.integer.value[0];

	return 0;
}
/*ALSA- soc codec I2C/SPI write control functions*/
static int zl380tw_control_write(struct snd_kcontrol *kcontrol,
		struct snd_ctl_elem_value *ucontrol)
{

    struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);

	struct soc_mixer_control *mc =
		(struct soc_mixer_control *)kcontrol->private_value;
	unsigned int reg = mc->reg;
	unsigned int shift = mc->shift;
	unsigned int mask = mc->max;
	unsigned int invert = mc->invert;
	unsigned int val = (ucontrol->value.integer.value[0] & mask);
    u16 valt;

	if (invert)
		val = mask - val;

	zl380twEnterCritical();
	if (zl380tw_hbi_rd16(zl380tw, reg, &valt) < 0) {
	    zl380twExitCritical();
		return -EIO;
    }
	if (((valt >> shift) & mask) == val) {
	    zl380twExitCritical();
		return 0;
	}
	valt &= ~(mask << shift);
	valt |= val << shift;

	if (zl380tw_reg_write(codec, reg, valt) < 0) {
	    zl380twExitCritical();
		return -EIO;
    }
    zl380twExitCritical();

	return 0;
}


int zl380tw_mute_r(struct snd_soc_codec *codec, int on)
{

    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
	u16 val;

	zl380twEnterCritical();

	if (zl380tw_hbi_rd16(zl380tw, ZL38040_AEC_CTRL_REG0, &val) < 0){
		zl380twExitCritical();
		return -EIO;
	}
	if (((val >> 7) & 1) == on){
		zl380twExitCritical();
		return 0;
	}
	val &= ~(1 << 7);
	val |= on << 7;

	if (zl380tw_reg_write(codec, ZL38040_AEC_CTRL_REG0, val) < 0){
		zl380twExitCritical();
		return -EIO;
	}

    zl380twExitCritical();
	return 0;
}

int zl380tw_mute_s(struct snd_soc_codec *codec, int on)
{
	u16 val;
    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);

	zl380twEnterCritical();

	if (zl380tw_hbi_rd16(zl380tw, ZL38040_AEC_CTRL_REG0, &val)){
		zl380twExitCritical();
		return -EIO;
	}
	if (((val >> 8) & 1) == on){
		zl380twExitCritical();
		return 0;
	}
	val &= ~(1 << 8);
	val |= on << 8;

	if (zl380tw_reg_write(codec, ZL38040_AEC_CTRL_REG0, val)){
		zl380twExitCritical();
		return -EIO;
	}

	zl380twExitCritical();
	return 0;
}

/* configure_codec() - configure the cross-point to either pure 2-channel stereo
 * or for 4-port mode with Achoustic Echo Canceller
 * mode: 0 -> Stereo bypass
 *       1 -> 4-port mode AEC
 *       2 ->
 */
static int zl380tw_configure_codec(struct snd_soc_codec *codec, u8 mode)
{


 	struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
    u8 *pData;

 	switch (mode) {
		case ZL38040_CR2_DEFAULT: {
		    pData = codec->reg_cache;
		    return zl380tw_hbi_access(zl380tw, ZL38040_OUTPUT_PATH_EN_REG, CODEC_CONFIG_REG_NUM, pData, TWOLF_HBI_WRITE);
		}
		case ZL38040_CR2_STEREO_BYPASS: {

		    u16 rin_src = zl380tw_read_codec_cache(codec, ZL38040_RIN_IN_PATH_REG);
		    u16 sin1_src = zl380tw_read_codec_cache(codec, ZL38040_SIN_IN_PATH_REG);

            pData = reg_stereo;
		    if ((rin_src != 0) || (sin1_src != 0))
		          zl380tw_hbi_access(zl380tw, ZL38040_DAC1_IN_PATH_REG, 28, pData, TWOLF_HBI_WRITE);
		    if (rin_src != 0) {
		        int rout_crp = zl380tw_find_index_codec_cache(codec, ZL38040_ROUT_PATH);
		        /*find where ROUT is going*/
		        if (rout_crp >=0) {
		             zl380tw_hbi_wr16(zl380tw, ZL38040_CACHED_ADDR_LO+rout_crp, (rin_src & 0xFF));
		             //dev_info(codec->dev, "reg[rout]=0x%04x <-- rin_src = 0x%04x\n", ZL38040_CACHED_ADDR_LO+rout_crp, rin_src);
		        }
            }

		    if (sin1_src != 0) {
		        int sout_crp = zl380tw_find_index_codec_cache(codec, ZL38040_SOUT_PATH);
		        /*find where ROUT is going*/
		        if (sout_crp >=0) {
		            zl380tw_hbi_wr16(zl380tw, ZL38040_CACHED_ADDR_LO+sout_crp, sin1_src);
		            //dev_info(codec->dev, "reg[sout]=0x%04x <-- sin1_src=0x%04x\n", ZL38040_CACHED_ADDR_LO+sout_crp, sin1_src);
		        }
            }

	        return 0;
	    }

		default: {
		    return -EINVAL;
		}
	}

}
/*The DACx, I2Sx, TDMx Gains can be used in both AEC mode or Stereo bypass mode
 * however the AEC Gains can only be used when AEC is active.
 * Each input source of the cross-points has two input sources A and B.
 * The Gain for each source can be controlled independantly.
 */
static const struct snd_kcontrol_new zl380tw_snd_controls[] = {
		SOC_SINGLE_EXT("DAC1 GAIN INA", ZL38040_DAC1_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("DAC2 GAIN INA", ZL38040_DAC2_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S1L GAIN INA", ZL38040_I2S1L_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S1R GAIN INA", ZL38040_I2S1R_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S2L GAIN INA", ZL38040_I2S2L_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S2R GAIN INA", ZL38040_I2S2R_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMA3 GAIN INA", ZL38040_TDMA3_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMA4 GAIN INA", ZL38040_TDMA4_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMB3 GAIN INA", ZL38040_TDMB3_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMB4 GAIN INA", ZL38040_TDMB4_GAIN_REG, 0, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("DAC1 GAIN INB", ZL38040_DAC1_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("DAC2 GAIN INB", ZL38040_DAC2_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S1L GAIN INB", ZL38040_I2S1L_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S1R GAIN INB", ZL38040_I2S1R_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S2L GAIN INB", ZL38040_I2S2L_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("I2S2R GAIN INB", ZL38040_I2S2R_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMA3 GAIN INB", ZL38040_TDMA3_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMA4 GAIN INB", ZL38040_TDMA4_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMB3 GAIN INB", ZL38040_TDMB3_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("TDMB4 GAIN INB", ZL38040_TDMB4_GAIN_REG, 8, 0x6, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC ROUT GAIN", ZL38040_USRGAIN, 0, 0x78, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC ROUT GAIN EXT", ZL38040_USRGAIN, 7, 0x7, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC SOUT GAIN", ZL38040_SYSGAIN, 8, 0xf, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC SIN GAIN", ZL38040_SYSGAIN, 0, 0x1f, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC MIC GAIN", ZL38040_MICGAIN, 0, 0x7, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("MUTE SPEAKER ROUT", ZL38040_AEC_CTRL_REG0, 7, 1, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("MUTE MIC SOUT", ZL38040_AEC_CTRL_REG0, 8, 1, 0,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC Bypass", ZL38040_AEC_CTRL_REG0, 4, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC Audio enh bypass", ZL38040_AEC_CTRL_REG0, 5, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC Master Bypass", ZL38040_AEC_CTRL_REG0, 1, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("ALC GAIN", ZL38040_AEC_CTRL_REG1, 12, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("ALC Disable", ZL38040_AEC_CTRL_REG1, 10, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC Tail disable", ZL38040_AEC_CTRL_REG1, 12, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC Comfort Noise", ZL38040_AEC_CTRL_REG1, 6, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("NLAEC Disable", ZL38040_AEC_CTRL_REG1, 14, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("AEC Adaptation", ZL38040_AEC_CTRL_REG1, 1, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("NLP Disable", ZL38040_AEC_CTRL_REG1, 5, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("Noise Inject", ZL38040_AEC_CTRL_REG1, 6, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("LEC Disable", ZL38040_LEC_CTRL_REG, 4, 1, 1,
				zl380tw_control_read, zl380tw_control_write),
		SOC_SINGLE_EXT("LEC Adaptation", ZL38040_LEC_CTRL_REG, 1, 1, 1,
				zl380tw_control_read, zl380tw_control_write),

};


int zl380tw_add_controls(struct snd_soc_codec *codec)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
	return snd_soc_add_controls(codec, zl380tw_snd_controls,
			ARRAY_SIZE(zl380tw_snd_controls));
#else
	return snd_soc_add_codec_controls(codec, zl380tw_snd_controls,
			ARRAY_SIZE(zl380tw_snd_controls));
#endif
}

static int zl380tw_hw_params(struct snd_pcm_substream *substream,
		struct snd_pcm_hw_params *params,
		struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->codec;
    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
#ifdef MICROSEMI_DEMO_PLATFORM

 	unsigned int buf2, buf3, buf4;

 	unsigned int buf0 = zl380tw_reg_read(codec, ZL38040_TDMA_CLK_CFG_REG);

    zl380twEnterCritical();
 	buf2 = zl380tw_reg_read(codec, ZL38040_TDMA_CH1_CFG_REG);
 	buf3 = zl380tw_reg_read(codec, ZL38040_TDMA_CH2_CFG_REG);
 	buf4 = zl380tw_reg_read(codec, ZL38040_AEC_CTRL_REG0);

    if ((buf2 == 0x0BAD) || (buf3 == 0x0BAD)) {
	   	zl380twExitCritical();
        return -1;
    }
 	switch (params_format(params)) {
		case SNDRV_PCM_FORMAT_S16_LE:
			buf2 |= (ZL38040_TDMA_16BIT_LIN);
			buf3 |= (ZL38040_TDMA_16BIT_LIN);
			buf3 |= 2;  /*If PCM use 2 timeslots*/
			break;
		case SNDRV_PCM_FORMAT_MU_LAW:
		    buf2 |= (ZL38040_TDMA_8BIT_ULAW);
			buf3 |= (ZL38040_TDMA_8BIT_ULAW);
			buf3 |= 1;  /*If PCM use 1 timeslots*/
		    break;
		case SNDRV_PCM_FORMAT_A_LAW:
		    buf2 |= (ZL38040_TDMA_8BIT_ALAW );
			buf3 |= (ZL38040_TDMA_8BIT_ALAW);
			buf3 |= 1;  /*If PCM use 1 timeslots*/
		    break;
		case SNDRV_PCM_FORMAT_S20_3LE:
			break;
		case SNDRV_PCM_FORMAT_S24_LE:
			break;
		case SNDRV_PCM_FORMAT_S32_LE:
			break;
		default: {
			zl380twExitCritical();
			return -EINVAL;
		}
	}
    zl380tw_reg_write(codec, ZL38040_TDMA_CLK_CFG_REG, buf0);

	zl380tw_reg_write(codec, ZL38040_TDMA_CH1_CFG_REG, buf2);
	zl380tw_reg_write(codec, ZL38040_TDMA_CH2_CFG_REG, buf3);
	zl380twExitCritical();
#endif
    /* If the ZL38040 TDM is configured as a slace I2S/PCM,
	 * The rate settings are not necessary. The Zl38040 has the ability to
	 * auto-detect the master rate and configure itself accordingly
	 * If configured as master, the rate must be set acccordingly
	 */
    zl380twEnterCritical();

    //dev_info(codec->dev, "b4: init_done=%d, flag=%d\n", zl380tw->init_done , zl380tw->flag);
    /*initialize codec cache to default config*/
    if((zl380tw->init_done) /*&& (codec->reg_cache==NULL)*/) {

        u8* cache = codec->reg_cache;
        if(zl380tw_hbi_access(zl380tw, ZL38040_OUTPUT_PATH_EN_REG, CODEC_CONFIG_REG_NUM, cache, TWOLF_HBI_READ) < 0)
        {
              zl380twExitCritical();
              return -EFAULT;
        }
        zl380tw_fill_codec_cache(codec, zl380tw->pData, CODEC_CONFIG_REG_NUM);
        zl380tw->init_done = 0;
        zl380tw->flag = 1;
    }
    //dev_info(codec->dev, "aft: init_done=%d, flag=%d\n", zl380tw->init_done , zl380tw->flag);
    switch (params_rate(params)) {
	    case 8000:
	    case 16000:
	        if (zl380tw->flag > 1) {
	             if (zl380tw_configure_codec(codec, ZL38040_CR2_DEFAULT) >=0)
	                 zl380tw->flag = 1;
	        }
		    break;
		/*case 44100:  */
	    case 48000:
	        if (zl380tw->flag ==1) {
		         zl380tw_configure_codec(codec, ZL38040_CR2_STEREO_BYPASS);
		         zl380tw->flag = 2;
		    }
		    break;
		default: {
			zl380twExitCritical();
			return -EINVAL;
		}
    }
	zl380tw_reset(zl380tw, ZL38040_RST_SOFTWARE);
    zl380twExitCritical();

	return 0;
}

static int zl380tw_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	/*zl380tw_mute_s(codec, mute);*/
	return zl380tw_mute_r(codec, mute);
}

static int zl380tw_set_dai_fmt(struct snd_soc_dai *codec_dai,
		unsigned int fmt)
{
#ifdef MICROSEMI_DEMO_PLATFORM
    struct snd_soc_codec *codec = codec_dai->codec;
    struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);
    unsigned int buf, buf0;

    zl380twEnterCritical();
    buf = zl380tw_reg_read(codec, ZL38040_TDMA_CFG_REG);
    buf0 = zl380tw_reg_read(codec, ZL38040_TDMA_CLK_CFG_REG);
    if ((buf == 0x0BAD) || (buf0 == 0x0BAD)) {
	   	zl380twExitCritical();
        return -1;
    }

	dev_info(codec_dai->dev, "zl380tw_set_dai_fmt\n");
	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_DSP_B:
			break;
		case SND_SOC_DAIFMT_DSP_A:
			break;
		case SND_SOC_DAIFMT_RIGHT_J:
			if ((buf & ZL38040_TDMA_FSALIGN) >> 1)
		        buf &= (~ZL38040_TDMA_FSALIGN);
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			if (!((buf & ZL38040_TDMA_FSALIGN) >> 1))
		        buf |= (ZL38040_TDMA_FSALIGN);
			break;
		case SND_SOC_DAIFMT_I2S:
	        if (!((buf & ZL38040_TDM_I2S_CFG_VAL) >> 1)) {
                buf |= (ZL38040_TDM_I2S_CFG_VAL );
             }
			break;
		default: {
			zl380twExitCritical();
			return -EINVAL;
		}
	}

	/* set master/slave TDM interface */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBM_CFM:
			buf0 |= ZL38040_TDM_TDM_MASTER_VAL;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:
			if ((buf0 & ZL38040_TDM_TDM_MASTER_VAL) >> 1) {
			    buf0 &= (~ZL38040_TDM_TDM_MASTER_VAL);
           }
			break;
		default: {
			zl380twExitCritical();
			return -EINVAL;
		}
	}

    zl380tw_reg_write(codec, ZL38040_TDMA_CFG_REG, buf);
    zl380tw_reg_write(codec, ZL38040_TDMA_CLK_CFG_REG, buf0);
	zl380tw_reset(zl380tw, ZL38040_RST_SOFTWARE);
	zl380twExitCritical();
#endif
	return 0;
}

static int zl380tw_set_dai_sysclk(struct snd_soc_dai *codec_dai, int clk_id,
		unsigned int freq, int dir)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);

 	zl380tw->sysclk_rate = freq;

	return 0;
}

static const struct snd_soc_dai_ops zl380tw_dai_ops = {
		.set_fmt        = zl380tw_set_dai_fmt,
		.set_sysclk     = zl380tw_set_dai_sysclk,
		.hw_params   	= zl380tw_hw_params,
		.digital_mute   = zl380tw_mute,

};

static struct snd_soc_dai_driver zl380tw_dai = {
		.name = "zl380tw-hifi",
		.playback = {
				.stream_name = "Playback",
				.channels_min = zl380tw_DAI_CHANNEL_MIN,
				.channels_max = zl380tw_DAI_CHANNEL_MAX,
				.rates = zl380tw_DAI_RATES,
				.formats = zl380tw_DAI_FORMATS,
		},
		.capture = {
				.stream_name = "Capture",
				.channels_min = zl380tw_DAI_CHANNEL_MIN,
				.channels_max = zl380tw_DAI_CHANNEL_MAX,
				.rates = zl380tw_DAI_RATES,
				.formats = zl380tw_DAI_FORMATS,
		},
		.ops = &zl380tw_dai_ops,
};
EXPORT_SYMBOL(zl380tw_dai);

static int zl380tw_set_bias_level(struct snd_soc_codec *codec,
    enum snd_soc_bias_level level)
{
	struct zl380tw *zl380tw = snd_soc_codec_get_drvdata(codec);

    zl380twEnterCritical();
    switch (level) {
	case SND_SOC_BIAS_ON:
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_STANDBY:
		/*wake up from sleep*/
		zl380tw_hbi_init(zl380tw, (HBI_CONFIG_VAL | HBI_CONFIG_WAKE));
        msleep(10);
		/*Clear the wake up bit*/
	    zl380tw_hbi_init(zl380tw, HBI_CONFIG_VAL);

		break;
	case SND_SOC_BIAS_OFF:
		 /*Low power sleep mode*/
		zl380tw_write_cmdreg(zl380tw, ZL38040_CMD_APP_SLEEP);

		break;
	}
	zl380twExitCritical();

	//codec->dapm.bias_level = level;
	return 0;
}

static int zl380tw_suspend(struct snd_soc_codec *codec
#if (LINUX_VERSION_CODE < KERNEL_VERSION(3,0,0))
   , pm_message_t state
#endif
)
{
	zl380twEnterCritical();
	zl380tw_set_bias_level(codec, SND_SOC_BIAS_OFF);
	zl380twExitCritical();
	return 0;
}

static int zl380tw_resume(struct snd_soc_codec *codec)
{
	zl380twEnterCritical();
	zl380tw_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
    zl380twExitCritical();
	return 0;
}

static void zl380tw_sub_probe_clean(void);

static int zl380tw_probe(struct snd_soc_codec *codec)
{
	dev_info(codec->dev, "Probing zl380tw SoC CODEC driver\n");
	return zl380tw_add_controls(codec);
}

static int zl380tw_remove(struct snd_soc_codec *codec)
{
	return 0;
}

static struct snd_soc_codec_driver soc_codec_dev_zl380tw = {
	.probe =	zl380tw_probe,
	.remove =	zl380tw_remove,
	.suspend =	zl380tw_suspend,
	.resume =	zl380tw_resume,
    .read  = zl380tw_reg_read,
	.write = zl380tw_reg_write,
	.set_bias_level = zl380tw_set_bias_level,
	.reg_cache_size = CODEC_CONFIG_REG_NUM,
	.reg_word_size = sizeof(u8),
	.reg_cache_default = reg_cache_default,
	.reg_cache_step = 1,
};
EXPORT_SYMBOL(soc_codec_dev_zl380tw);
#endif /*ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER*/

/*--------------------------------------------------------------------
 *    ALSA  SOC CODEC driver  - END
 *--------------------------------------------------------------------*/


/*--------------------------------------------------------------------
 *    CHARACTER type Host Interface driver
 *--------------------------------------------------------------------*/
#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER

/* read 16bit HBI Register */
/* slave_rd16()- read a 16bit word
 *  \param[in]     pointer the to where to store the data
 *
 *  return ::status
 */
static int zl380tw_rd16(struct zl380tw *zl380tw, u16 *pdata)
{
	u8 buf[2] = {0, 0};
	int status = 0;
#ifdef MICROSEMI_HBI_SPI
	struct spi_message msg;
	struct spi_transfer xfer = {
		.len = 2,
		.rx_buf = buf,
	};

	spi_message_init(&msg);

	spi_message_add_tail(&xfer, &msg);
	status = spi_sync(zl380tw->spi, &msg);
#endif
#ifdef MICROSEMI_HBI_I2C
    status = i2c_master_recv(zl380tw->i2c, buf, 2);
#endif
	if (status < 0) {
		return status;
	}

    *pdata = (buf[0]<<8) | buf[1] ; /* Byte_HI, Byte_LO */
	return 0;
}


 static long zl380tw_io_ioctl(
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
   struct inode *i,
#endif
   struct file *filp,
   unsigned int cmd, unsigned long arg)
{
	int retval =0;
	u16 buf =0;
	struct zl380tw *zl380tw = filp->private_data;

	switch (cmd) {

	case TWOLF_HBI_WR16:
		if (copy_from_user(&zl380tw_ioctl_buf,
			               (ioctl_zl380tw *)arg,
			                sizeof(ioctl_zl380tw)))
			return -EFAULT;
		zl380twEnterCritical();
		retval = zl380tw_hbi_wr16(zl380tw, (u16)zl380tw_ioctl_buf.addr,
                                     zl380tw_ioctl_buf.data);
        zl380twExitCritical();
	break;
	case TWOLF_HBI_RD16:
		if (copy_from_user(&zl380tw_ioctl_buf,
			               (ioctl_zl380tw *)arg,
			                sizeof(ioctl_zl380tw)))
			return -EFAULT;
		zl380twEnterCritical();
		retval = zl380tw_hbi_rd16(zl380tw, (u16)zl380tw_ioctl_buf.addr,
                                    &zl380tw_ioctl_buf.data);
        zl380twExitCritical();
		if (!retval) {
			if (copy_to_user((ioctl_zl380tw *)arg,
			                  &zl380tw_ioctl_buf,
                              sizeof(ioctl_zl380tw)))
				return -EFAULT;
		} else
			return -EAGAIN;
	break;
	case TWOLF_BOOT_PREPARE :
        zl380twEnterCritical();
	    retval = zl380tw_boot_prepare(zl380tw);
	    zl380twExitCritical();
	break;
	case TWOLF_BOOT_SEND_MORE_DATA: {
		if (copy_from_user(zl380tw->pData,
			               (char *)arg,
			                MAX_TWOLF_ACCESS_SIZE_IN_BYTES))
			return -EFAULT;
		zl380twEnterCritical();
		retval = zl380tw_boot_Write(zl380tw, zl380tw->pData);
        zl380twExitCritical();
	    break;
	}
	case TWOLF_BOOT_CONCLUDE :
        zl380twEnterCritical();
	    retval = zl380tw_boot_conclude(zl380tw);
	    zl380twExitCritical();
	break;

	case TWOLF_CMD_PARAM_REG_ACCESS :
		retval = __get_user(buf, (u16 __user *)arg);
		if (retval ==0) {
		    zl380twEnterCritical();
		    retval = zl380tw_write_cmdreg(zl380tw, buf);
		    zl380twExitCritical();
        }
	break;
	case TWOLF_CMD_PARAM_RESULT_CHECK :
        zl380twEnterCritical();
	    retval = zl380tw_cmdresult_check(zl380tw);
	    zl380twExitCritical();
	break;
	case TWOLF_RESET :
		retval = __get_user(buf, (u16 __user *)arg);
		if (retval ==0) {
	       zl380twEnterCritical();
		   retval = zl380tw_reset(zl380tw, buf);
		   zl380twExitCritical();
        }
	break;
	case TWOLF_SAVE_FWR_TO_FLASH :
         zl380twEnterCritical();
		 retval = zl380tw_save_image_to_flash(zl380tw);
		 zl380twExitCritical();
	break;
	case TWOLF_LOAD_FWR_FROM_FLASH :
         retval = __get_user(buf, (u16 __user *)arg);
         if (retval ==0) {
             zl380twEnterCritical();
		     retval = zl380tw_load_fwr_from_flash(zl380tw, buf);
		     zl380twExitCritical();
         }
	break;
	case TWOLF_SAVE_CFG_TO_FLASH :
         retval = __get_user(buf, (u16 __user *)arg);
         if (retval ==0) {
             zl380twEnterCritical();
		     retval = zl380tw_save_cfg_to_flash(zl380tw, buf);
		     zl380twExitCritical();
         }
	break;
	case TWOLF_LOAD_CFG_FROM_FLASH :
         retval = __get_user(buf, (u16 __user *)arg);
         if (retval ==0) {
             zl380twEnterCritical();
		     retval = zl380tw_load_cfg_from_flash(zl380tw, buf);
		     zl380twExitCritical();
         }
	break;
	case TWOLF_ERASE_IMGCFG_FLASH :
         retval = __get_user(buf, (u16 __user *)arg);
         if (retval ==0) {
             zl380twEnterCritical();
		     retval = zl380tw_erase_fwrcfg_from_flash(zl380tw, buf);
		     zl380twExitCritical();
         }
	break;
	case TWOLF_LOAD_FWRCFG_FROM_FLASH :
         retval = __get_user(buf, (u16 __user *)arg);
         if (retval ==0) {
             zl380twEnterCritical();
		     retval = zl380tw_load_fwrcfg_from_flash(zl380tw, buf);
		     zl380twExitCritical();
         }
	break;
	case TWOLF_HBI_WR_ARB_SINGLE_WORD :
        retval = __get_user(buf, (u16 __user *)arg);
        if (retval ==0) {
            zl380twEnterCritical();
		    retval = zl380tw_wr16(zl380tw, buf);
		    zl380twExitCritical();
        }
	break;
	case TWOLF_HBI_RD_ARB_SINGLE_WORD :
         zl380twEnterCritical();
		 retval = zl380tw_rd16(zl380tw, &buf);
		 zl380twExitCritical();
         if (retval ==0)
            retval = __put_user(buf, (__u16 __user *)arg);
	break;
	case TWOLF_HBI_INIT :
         retval = __get_user(buf, (u16 __user *)arg);
         if (retval ==0) {
            zl380twEnterCritical();
		    retval = zl380tw_hbi_init(zl380tw, buf);
		    zl380twExitCritical();
         }
	break;
	case TWOLF_ERASE_ALL_FLASH :
         zl380twEnterCritical();
         retval = zl380tw_reset(zl380tw, ZL38040_RST_TO_BOOT);
         if (retval ==0)
		     retval = zl380tw_erase_flash(zl380tw);
		 zl380twExitCritical();
	break;
	case TWOLF_STOP_FWR :
         zl380twEnterCritical();
		 retval = zl380tw_stop_fwr_to_bootmode(zl380tw);
		 zl380twExitCritical();
	break;
	case TWOLF_START_FWR :
         zl380twEnterCritical();
		 retval = zl380tw_start_fwr_from_ram(zl380tw);
		 zl380twExitCritical();
	break;

	default:
		printk(KERN_DEBUG "ioctl: Invalid Command Value");
		retval = -EINVAL;
	}
	return retval;
}



/*----------------------------------------------------------------------*
 *   The ZL38040/05x/06x/08x kernel specific aceess functions are defined below
 *-------------------------ZL380xx FUNCTIONs-----------------------------*/

/* This function is best used to simply retrieve pending data following a
 * previously sent write command
 * The data is returned in bytes format. Up to 256 data bytes.
 */

static ssize_t zl380tw_io_read(struct file *filp, char __user *buf, size_t count,
                                                           loff_t *f_pos)
{
    /* This access uses the spi/i2c command frame format - where both
     * the whole data is read in one active chip_select
     */

	struct zl380tw	*zl380tw = filp->private_data;

	int	status = 0;
    u16 cmd = 0;
	if (count > twHBImaxTransferSize)
		return -EMSGSIZE;

#ifdef MICROSEMI_HBI_SPI
	if (zl380tw->spi == NULL) {
	    TW_DEBUG1("spi device is not available \n");
	    return -ESHUTDOWN;
    }
#endif
#ifdef MICROSEMI_HBI_I2C
	if (zl380tw->i2c == NULL) {
	    TW_DEBUG1("zl380tw_io_read::i2c device is not available \n");
	    return -ESHUTDOWN;
    }

#endif
    zl380twEnterCritical();
	if (copy_from_user(zl380tw->pData, buf, count)) {
       zl380twExitCritical();
	   return -EFAULT;
    }
    cmd = (*(zl380tw->pData + 0) << 8) | (*(zl380tw->pData + 1));
    /*read the data*/
    status = zl380tw_hbi_access(zl380tw,
                       cmd, count, zl380tw->pData, TWOLF_HBI_READ);
	if (status < 0) {
          zl380twExitCritical();
          return -EFAULT;
    }
	if (copy_to_user(buf, zl380tw->pData, count)) {
          zl380twExitCritical();
		  return -EFAULT;
    }
    zl380twExitCritical();
	return 0;
}

/* Write multiple bytes (up to 254) to the device
 * the data should be formatted as follows
 * cmd_type,
 * cmd_byte_low,
 * cmd_byte_hi,
 * data0_byte_low,
 * data0_byte_hi
 *  ...
 * datan_byte_low, datan_byte_hi   (n < 254)
 */

static ssize_t zl380tw_io_write(struct file *filp, const char __user *buf,
		size_t count, loff_t *f_pos)
{
    /* This access use the spi/i2c command frame format - where both
     * the command and the data to write are sent in one active chip_select
     */
	struct zl380tw	*zl380tw = filp->private_data;

	int status = 0;
	u16 cmd = 0;

	if (count > twHBImaxTransferSize)
		return -EMSGSIZE;
    zl380twEnterCritical();
	if (copy_from_user(zl380tw->pData, buf, count)) {
        zl380twExitCritical();
	    return -EFAULT;
    }
	cmd = (*(zl380tw->pData + 0) << 8) | (*(zl380tw->pData + 1));
    TW_DEBUG2("count = %d\n",count);
    /*remove the cmd numbytes from the count*/
    status = zl380tw_hbi_access(zl380tw,
                       cmd, count-2, (zl380tw->pData+2), TWOLF_HBI_WRITE);
    if (status < 0) {
       zl380twExitCritical();
       return -EFAULT;
    }
    zl380twExitCritical();
	return status;
}

static int zl380tw_io_open(struct inode *inode, struct file *filp)
{

    if (module_usage_count) {
		printk(KERN_ERR "microsemi_slave_zl380xx device alrady opened\n");
		return -EBUSY;
	}

	module_usage_count++;
	filp->private_data = zl380tw_priv;

	return 0;
}

static int zl380tw_io_close(struct inode *inode, struct file *filp)
{

	//struct zl380tw *zl380tw = filp->private_data;
	filp->private_data = NULL;

	if (module_usage_count) {
		module_usage_count--;
	}

	return 0;
}

static const struct file_operations zl380tw_fops = {
	.owner =	THIS_MODULE,
	.open =		zl380tw_io_open,
	.read =     zl380tw_io_read,
	.write =    zl380tw_io_write,
	.release =	zl380tw_io_close,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,35))
	.ioctl =	zl380tw_io_ioctl
#else
	.unlocked_ioctl =	zl380tw_io_ioctl
#endif
};

/*----------------------------------------------------------------------------*/



static struct class *zl380tw_class;
#endif /*ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER */
 /*--------------------------------------------------------------------
 *    CHARACTER type Host Interface driver - END
 *--------------------------------------------------------------------*/

#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
/*IMPORTANT note: Change this controller string maching accordingly per your *.dts or *dtsi compatible definition file*/
static struct of_device_id zl380tw_of_match[] = {
	{ .compatible = "ambarella,zl380tw",},
	{},
};
MODULE_DEVICE_TABLE(of, zl380tw_of_match);
#endif /*SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING*/

static int zl380tw_sub_probe(void)
{
	/* Allocate driver data */
	zl380tw_priv = kzalloc(sizeof(*zl380tw_priv), GFP_KERNEL);
	if (zl380tw_priv == NULL)
		return -ENOMEM;

    /* Allocating memory for the data buffer */
    zl380tw_priv->pData  = kmalloc(twHBImaxTransferSize, GFP_KERNEL);
    if (!zl380tw_priv->pData) {
        printk(KERN_ERR "Error allocating %d bytes pdata memory",
                                                 twHBImaxTransferSize);
        kfree(zl380tw_priv);
		return -ENOMEM;
    }
    memset(zl380tw_priv->pData, 0, twHBImaxTransferSize);
    return 0;
}

static void zl380tw_sub_probe_clean(void)
{
        kfree(zl380tw_priv);
	    kfree((void *)zl380tw_priv->pData);
	    zl380tw_priv->pData = NULL;
}


#ifdef ZL380TW_DEV_BIND_SLAVE_TO_MASTER
/* Bind the client driver to a master (SPI or I2C) adapter */

static int zl380xx_slaveMaster_binding(void) {
#ifdef MICROSEMI_HBI_SPI
    struct spi_board_info spi_device_info = {
        .modalias = "zl380tw",
        .max_speed_hz = SPIM_CLK_SPEED,
        .bus_num = SPIM_BUS_NUM,
        .chip_select = SPIM_CHIP_SELECT,
        .mode = SPIM_MODE,
    };
    struct spi_device *client;
    struct spi_master *master;
    /* create a new slave device, given the master and device info
	 * if another device is alread assigned to that bus.cs, then this will fail
	 */
    /* get the master device, given SPI the bus number*/
    master = spi_busnum_to_master( spi_device_info.bus_num );
    if( !master )
        return -ENODEV;
    printk(KERN_INFO "SPI master device found at bus = %d\n", master->bus_num);

    client = spi_new_device( master, &spi_device_info );
    if( !client )
        return -ENODEV;
    printk(KERN_INFO "SPI slave device %s, module alias %s added at bus = %d, cs =%d\n",
           client->dev.driver->name, client->modalias, client->master->bus_num, client->chip_select);
#endif
#ifdef MICROSEMI_HBI_I2C /*#ifdef MICROSEMI_HBI_I2C*/
    struct i2c_board_info i2c_device_info = {
    	I2C_BOARD_INFO("zl380tw", MICROSEMI_I2C_ADDR),
    	.platform_data	= NULL,
    };
    struct i2c_client   *client;
    /*create a new client for that adapter*/
    /*get the i2c adapter*/
    struct i2c_adapter *master = i2c_get_adapter(CONTROLLER_I2C_BUS_NUM);
    if( !master )
        return -ENODEV;
    printk(KERN_INFO "found I2C master device %s \n", master->name);

    client = i2c_new_device( master, &i2c_device_info);
    if( !client)
        return -ENODEV;

    printk(KERN_INFO "I2C slave device %s, module alias %s attached to I2C master device %s\n",
           client->dev.driver->name, client->name, master->name);

    i2c_put_adapter(master);
#endif /*MICROSEMI_HBI_SPI/I2C interface selection macro*/

    return 0;

}

static int zl380tw_slaveMaster_unbind(void)
{
#ifdef MICROSEMI_HBI_SPI
        spi_unregister_device(zl380tw_priv->spi);
#endif
#ifdef MICROSEMI_HBI_I2C /*#ifdef MICROSEMI_HBI_I2C*/
        i2c_unregister_device(zl380tw_priv->i2c);
#endif

	return 0;
}

#endif //ZL380TW_DEV_BIND_SLAVE_TO_MASTER
/*--------------------------------------------------------------------
* SPI driver registration
*--------------------------------------------------------------------*/

#ifdef MICROSEMI_HBI_SPI
static int zl380tw_spi_probe(struct spi_device *spi)
{

	int err, ret;
	enum of_gpio_flags flags;

	err = zl380tw_sub_probe();
	if (err < 0) {
		return err;
	}
	dev_dbg(&spi->dev, "probing zl380tw spi device\n");

	spi->master->bus_num = 1;//SPIM_BUS_NUM;
	spi->mode = SPI_MODE_0;
	spi->max_speed_hz = SPIM_CLK_SPEED;
	spi->chip_select = SPIM_CHIP_SELECT;
	spi->bits_per_word = 8;

	err = spi_setup(spi);
	if (err < 0) {
        zl380tw_sub_probe_clean();
		return err;
	}

	/* Initialize the driver data */
	spi_set_drvdata(spi, zl380tw_priv);
	zl380tw_priv->spi = spi;

	zl380tw_priv->pwr_pin = of_get_named_gpio_flags(spi->dev.of_node, "zl380,pwr-gpio", 0, &flags);
	zl380tw_priv->pwr_active = !!(flags & OF_GPIO_ACTIVE_LOW);

	if(zl380tw_priv->pwr_pin < 0 || !gpio_is_valid(zl380tw_priv->pwr_pin)) {
		printk(KERN_ERR "zl380 power pin is invalid\n");
		zl380tw_priv->pwr_pin = -1;
		return -1;
	}

	if(gpio_is_valid(zl380tw_priv->pwr_pin)) {
		ret = devm_gpio_request(&spi->dev, zl380tw_priv->pwr_pin, "zl380xx pwr");
		if(ret < 0) {
			dev_err(&spi->dev, "Failed to request pwr pin for zl380xx: %d\n", ret);
			return ret;
		}
		gpio_direction_output(zl380tw_priv->pwr_pin, zl380tw_priv->pwr_active);
	}

	zl380tw_priv->rst_pin = of_get_named_gpio_flags(spi->dev.of_node, "zl380,rst-gpio", 0, &flags);
	zl380tw_priv->rst_active = !!(flags & OF_GPIO_ACTIVE_LOW);

	if(zl380tw_priv->rst_pin < 0 || !gpio_is_valid(zl380tw_priv->rst_pin)) {
		printk(KERN_ERR "zl380 power pin is invalid\n");
		zl380tw_priv->rst_pin = -1;
		return -1;
	}

	if(gpio_is_valid(zl380tw_priv->rst_pin)) {
		ret = devm_gpio_request(&spi->dev, zl380tw_priv->rst_pin, "zl380xx reset");
		if(ret < 0) {
			dev_err(&spi->dev, "Failed to request reset pin for zl380xx: %d\n", ret);
			return ret;
		}
		gpio_direction_output(zl380tw_priv->rst_pin, zl380tw_priv->rst_active);
		msleep(10);
		gpio_direction_output(zl380tw_priv->rst_pin, !zl380tw_priv->rst_active);
	}

#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER
    {
        u16 buf = 0;

        zl380tw_priv->flag = 0;

        zl380tw_hbi_rd16(zl380tw_priv, ZL38040_DEVICE_ID_REG, &buf);
        if((buf > 38000)  && (buf < 38090)) {
              zl380tw_priv->init_done =1;
              printk("SPI slave device %d found at bus::cs = %d::%d\n", buf, spi->master->bus_num, spi->chip_select);

        } else {
			zl380tw_priv->init_done =0;
        }
    }

	err = snd_soc_register_codec(&spi->dev, &soc_codec_dev_zl380tw, &zl380tw_dai, 1);
	if(err < 0) {
		zl380tw_sub_probe_clean();
		dev_dbg(&spi->dev, "zl380tw spi device not created!!!\n");
		return err;
	}
#endif

#ifdef ZL380XX_TW_UPDATE_FIRMWARE
    if (zl380tw_ldfwr(zl380tw_priv) < 0) {
        dev_dbg(&spi->dev, "error loading the firmware into the codec\n");
#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER
        snd_soc_unregister_codec(&spi->dev);
#endif
        zl380tw_sub_probe_clean();
        return -ENODEV;
    }
#endif
    dev_dbg(&spi->dev, "zl380tw codec device created...\n");

	return 0;
}

static int  zl380tw_spi_remove(struct spi_device *spi)
{

#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER
	snd_soc_unregister_codec(&spi->dev);
#endif
	kfree(spi_get_drvdata(spi));
	if (!module_usage_count) {

	   kfree((void *)zl380tw_priv->pData);
	   zl380tw_priv->pData = NULL;
    }
	return 0;
}

static struct spi_driver zl380tw_spi_driver = {
	.driver = {
		.name = "zl380tw",
		.owner = THIS_MODULE,
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
        .of_match_table = zl380tw_of_match,
#endif
	},
	.probe = zl380tw_spi_probe,
	.remove = zl380tw_spi_remove,
};
#endif

#ifdef MICROSEMI_HBI_I2C

static int zl380tw_i2c_probe(struct i2c_client *i2c,
			    const struct i2c_device_id *id)
{
    int err = zl380tw_sub_probe();
	if (err < 0) {
		return err;
    }

	dev_dbg(&i2c->dev, "probing zl380tw I2C device\n");
    //i2c->addr = MICROSEMI_I2C_ADDR;

	i2c_set_clientdata(i2c, zl380tw_priv);
	zl380tw_priv->i2c = i2c;
	printk(KERN_ERR "i2c slave device address = 0x%04x\n", i2c->addr);

#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER
    {
        u16 buf = 0;

        zl380tw_priv->flag = 0;

        zl380tw_hbi_rd16(zl380tw_priv, ZL38040_DEVICE_ID_REG, &buf);
        if((buf > 38000)  && (buf < 38090)) {
              zl380tw_priv->init_done =1;
              printk(KERN_ERR "I2C slave device %d found at address = 0x%04x\n", buf, i2c->addr);

        } else {
              zl380tw_priv->init_done =0;
        }
    }

	err = snd_soc_register_codec(&i2c->dev, &soc_codec_dev_zl380tw, &zl380tw_dai, 1);
	if(err < 0) {
		zl380tw_sub_probe_clean();
		dev_dbg(&i2c->dev, "zl380tw I2c device not created!!!\n");
		return err;
	}
#endif

#ifdef ZL380XX_TW_UPDATE_FIRMWARE
    if (zl380tw_ldfwr(zl380tw_priv) < 0) {
        dev_dbg(&i2c->dev, "error loading the firmware into the codec\n");
#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER
        snd_soc_unregister_codec(&i2c->dev);
#endif
        zl380tw_sub_probe_clean();
        return -ENODEV;
    }
#endif
    dev_dbg(&i2c->dev, "zl380tw I2C codec device created...\n");
	return err;
}

static int zl380tw_i2c_remove(struct i2c_client *i2c)
{

#ifdef ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER
	snd_soc_unregister_codec(&i2c->dev);
#endif
	kfree(i2c_get_clientdata(i2c));
	if (!module_usage_count) {

	   kfree((void *)zl380tw_priv->pData);
	   zl380tw_priv->pData = NULL;
    }
	return 0;
}

static struct i2c_device_id zl380tw_id_table[] = {
    {"zl380tw", 0 },
    {}
 };

static struct i2c_driver zl380tw_i2c_driver = {
	.driver = {
		.name	= "zl380tw",
		.owner = THIS_MODULE,
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
        .of_match_table = zl380tw_of_match,
#endif
	},
	.probe = zl380tw_i2c_probe,
	.remove = zl380tw_i2c_remove,
	.id_table = zl380tw_id_table,
};

#endif


static int __init zl380tw_init(void)
{
    int status;

#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER
	struct device *dev;
	status = alloc_chrdev_region(&t_dev, FIRST_MINOR, NUMBER_OF_ZL380xx_DEVICES,
                                                 "zl380tw");
	if (status < 0) {
		printk(KERN_ERR "Failed to register character device");
        return status;
    }

    /*create the device class*/
	zl380tw_class = class_create(THIS_MODULE, "zl380tw");
	if (zl380tw_class == NULL) {
        printk(KERN_ERR "Error %d creating class zl380tw", status);
		unregister_chrdev_region(t_dev, NUMBER_OF_ZL380xx_DEVICES);
		return -1;
	}

	/* registration of character device */
	dev = device_create(zl380tw_class, NULL,
                    t_dev, NULL, "zl380tw");
	if (IS_ERR(dev)) {
        printk(KERN_ERR "Error %d creating device zl380tw", status);
        class_destroy(zl380tw_class);
		unregister_chrdev_region(t_dev, NUMBER_OF_ZL380xx_DEVICES);
    }
    status = IS_ERR(dev) ? PTR_ERR(dev) : 0;
    if (status  == 0) {
    	/* Initialize of character device */
    	cdev_init(&c_dev, &zl380tw_fops);
    	/* addding character device */
    	status = cdev_add(&c_dev, t_dev, NUMBER_OF_ZL380xx_DEVICES);
    	if (status < 0) {
    		printk(KERN_ERR "Error %d adding zl380tw", status);
            class_destroy(zl380tw_class);
            device_destroy(zl380tw_class, t_dev);
            cdev_del(&c_dev);
    		unregister_chrdev_region(t_dev, NUMBER_OF_ZL380xx_DEVICES);
    		return -1;
    	}
    }
#endif
#ifdef MICROSEMI_HBI_SPI
	status = spi_register_driver(&zl380tw_spi_driver);
#endif
#ifdef MICROSEMI_HBI_I2C
    status = i2c_add_driver(&zl380tw_i2c_driver);
#endif
    if (status < 0) {
#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER
        class_destroy(zl380tw_class);
        cdev_del(&c_dev);
        unregister_chrdev_region(t_dev, NUMBER_OF_ZL380xx_DEVICES);
#endif
    }
#ifdef ZL380TW_DEV_BIND_SLAVE_TO_MASTER
    status =  zl380xx_slaveMaster_binding();
    if (status < 0) {
        printk(KERN_ERR "error =%d\n", status);
#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER
        class_destroy(zl380tw_class);
        device_destroy(zl380tw_class, t_dev);
        cdev_del(&c_dev);
		unregister_chrdev_region(t_dev, NUMBER_OF_ZL380xx_DEVICES);
#endif
#ifdef MICROSEMI_HBI_SPI
	    spi_unregister_driver(&zl380tw_spi_driver);
#endif
#ifdef MICROSEMI_HBI_I2C
	    i2c_del_driver(&zl380tw_i2c_driver);
#endif
        return -1;
	}
    status =0;
#endif
    return status;
}
module_init(zl380tw_init);

static void __exit zl380tw_exit(void)
{
#ifdef ZL380TW_DEV_BIND_SLAVE_TO_MASTER
    zl380tw_slaveMaster_unbind();
#endif
#ifdef MICROSEMI_HBI_SPI
	spi_unregister_driver(&zl380tw_spi_driver);
#endif
#ifdef MICROSEMI_HBI_I2C
	i2c_del_driver(&zl380tw_i2c_driver);
#endif
#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER
    device_destroy(zl380tw_class, t_dev);
	class_destroy(zl380tw_class);
	cdev_del(&c_dev);
	unregister_chrdev_region(t_dev, NUMBER_OF_ZL380xx_DEVICES);
#endif
}
module_exit(zl380tw_exit);

MODULE_AUTHOR("Jean Bony <jean.bony@microsemi.com>");
MODULE_DESCRIPTION(" Microsemi Timberwolf i2c/spi/char/alsa codec driver");
MODULE_LICENSE("GPL");
