#ifndef __ZL380TW_H
#define __ZL380TW_H

#undef MICROSEMI_DEMO_PLATFORM /*leave this macro undefined unless requested by Microsemi*/
/*-------------------------------------------------------------*
 *    HOST MACROS - Define/undefine as desired
 *    -----------------------------------------
 *    Supported combinations:
 *    ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER + ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER  + MICROSEMI_HBI_I2C or MICROSEMI_HBI_SPI
 *    ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER + MICROSEMI_HBI_I2C or MICROSEMI_HBI_SPI
 *    ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER  + MICROSEMI_HBI_I2C or MICROSEMI_HBI_SPI
 *
 *    all of the above can be used with the ZL380XX_TW_UPDATE_FIRMWARE
 *-------------------------------------------------------------*/
#define ZL380XX_TW_ENABLE_ALSA_CODEC_DRIVER /*Define this macro to create a /sound/soc ALSA codec device driver*/
#define ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER   /*Define this macro to create a character device driver*/

/*Enable either one of this macro to create a SPI or an I2C device driver
*  to be used as the low-level transport for the ALSA and/or CHAR device read/write accesses
*/
#undef MICROSEMI_HBI_I2C          /*Enable this macro if the HBI interface between the host CPU and the Twolf is I2C*/
#ifdef MICROSEMI_HBI_I2C
    #undef MICROSEMI_HBI_SPI
    #define MICROSEMI_I2C_ADDR 0x45  /*if DIN pin is tied to ground, else if DIN is tied to 3.3V address must be 0x52*/
    #define CONTROLLER_I2C_BUS_NUM 0
#else
    #define MICROSEMI_HBI_SPI
    /*Define the SPI master signal config*/
    #define SPIM_CLK_SPEED  15000000
    #define SPIM_CHIP_SELECT 0
    #define SPIM_MODE SPI_MODE_0
    #define SPIM_BUS_NUM 0
#endif

//#undef ZL380XX_TW_UPDATE_FIRMWARE /*define if you want to update current firmware with a new one at power up*/
//#undef ZL380XX_TW_UPDATE_FIRMWARE
#define ZL380XX_TW_UPDATE_FIRMWARE

#ifdef ZL380XX_TW_UPDATE_FIRMWARE
//#define ZL380TW_INCLUDE_FIRMWARE_REQUEST_LIB
/*NOTE: Rename the *s3 firmware file as per below or simply change the file name below as per the firmware file name
*       If there is more than 1 device create new path name and pass them to the firmware loading function accordingly
*/
    #define  ZLS380TW0_TWOLF "ZLS38062.0_E1.1.0_App.s3" /*firmware for your zl380xx device 0*/

#define ZL380XX_TW_UPDATE_CONFIG
#ifdef ZL380XX_TW_UPDATE_CONFIG
    #define  ZLS380TW0_TWOLF_CRK "ZLS38062_20151223_AMBA.CR2" /*configuration record for your zl380xx device 0*/
#endif
#endif /*ZL380XX_TW_UPDATE_FIRMWARE*/

#undef ZL38040_SAVE_FWR_TO_FLASH  /*define if a slave flash is connected to the zl380tw and you want to save the loaded firmware/config to flash*/



/*HBI access to the T-wolf must not be interrupted by another process*/
#define PROTECT_CRITICAL_SECTION  /*define this macro to protect HBI critical section*/

/*The zl380tw device registration can be done using the Linux device OF matching scheme or
* or by using the old method via the board init file
* This driver includes the method to auto-detect the SPI or I2C master and register itself to that
* master as per the defined bus info above
*/
#define SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING  /*define this if you are using the linux device tree (*dts, *dtb, etc.. of maching lib*/
#ifdef SUPPORT_LINUX_DEVICE_TREE_OF_MATCHING
#define CONTROLLER_DTS_STRING  "ambarella" /*Change the string name accordingly to your device tree controller/driver maching name*/
#else
/*define this macro if the board_info registration is not already done in your board init file
* This macro will cause a new slave device (SPI or I2C) to be created and attached to the master
* device controller
* If you already assigned the ressource for this zl380tw driver in your board init file, then undef this macro.
*/
#define ZL380TW_DEV_BIND_SLAVE_TO_MASTER
#endif


/*Define the ZL380TW interrupt pin drive mode 1:TTL, 0: Open Drain(default)*/
#define HBI_CONFIG_INT_PIN_DRIVE_MODE		0
/*-------------------------------------------------------------*
 *     HOST MACROS - end
 *-------------------------------------------------------------*/


/* local defines */
#define MAX_TWOLF_ACCESS_SIZE_IN_BYTES 264 /*127 16-bit words*/
#define MAX_TWOLF_FIRMWARE_SIZE_IN_BYTES 128 /*128 8-bit words*/

/*The timberwolf device reset modes*/
#define ZL38040_RST_HARDWARE_RAM 0
#define ZL38040_RST_HARDWARE_ROM 1
#define ZL38040_RST_SOFTWARE     2
#define ZL38040_RST_AEC          3
#define ZL38040_RST_TO_BOOT      4

#ifdef ZL380XX_TW_ENABLE_CHAR_DEV_DRIVER

#define NUMBER_OF_ZL380xx_DEVICES 1
#define FIRST_MINOR 0
/*structure for IOCL access*/
typedef struct {
	__u16	addr;
	__u16	data;
} ioctl_zl380tw;


/* ioctl() calls that are permitted to the /dev/microsemi_spis_tw interface. */
#define TWOLF_MAGIC 'q'            /*Change this accordingly to your system*/
#define TWOLF_HBI_RD16		        _IOWR(TWOLF_MAGIC, 1,  ioctl_zl380tw *)
#define TWOLF_HBI_WR16		        _IOW(TWOLF_MAGIC, 2, ioctl_zl380tw *)
#define TWOLF_HBI_INIT		        _IOW(TWOLF_MAGIC, 3, __u16)
#define TWOLF_RESET        	        _IOW(TWOLF_MAGIC, 4,  __u16)
#define TWOLF_SAVE_FWR_TO_FLASH     _IO(TWOLF_MAGIC, 5)
#define TWOLF_LOAD_FWR_FROM_FLASH   _IOW(TWOLF_MAGIC, 6,  __u16)
#define TWOLF_SAVE_CFG_TO_FLASH     _IOW(TWOLF_MAGIC, 7,  __u16)
#define TWOLF_LOAD_CFG_FROM_FLASH   _IOW(TWOLF_MAGIC, 8,  __u16)
#define TWOLF_ERASE_IMGCFG_FLASH    _IOW(TWOLF_MAGIC, 9,  __u16)
#define TWOLF_ERASE_ALL_FLASH       _IO(TWOLF_MAGIC, 10)
#define TWOLF_STOP_FWR              _IO(TWOLF_MAGIC, 11)
#define TWOLF_START_FWR             _IO(TWOLF_MAGIC, 12)
#define TWOLF_LOAD_FWRCFG_FROM_FLASH    _IOW(TWOLF_MAGIC, 13,  __u16)
#define TWOLF_HBI_WR_ARB_SINGLE_WORD    _IOW(TWOLF_MAGIC, 14, __u16)
#define TWOLF_HBI_RD_ARB_SINGLE_WORD    _IOW(TWOLF_MAGIC, 15, __u16)
#define TWOLF_CMD_PARAM_REG_ACCESS	    _IOW(TWOLF_MAGIC, 16, __u16)
#define TWOLF_CMD_PARAM_RESULT_CHECK    _IO(TWOLF_MAGIC, 17)
#define TWOLF_BOOT_PREPARE              _IO(TWOLF_MAGIC, 18)
#define TWOLF_BOOT_SEND_MORE_DATA       _IOW(TWOLF_MAGIC, 19, int)
#define TWOLF_BOOT_CONCLUDE             _IO(TWOLF_MAGIC, 20)
#define TWOLF_LOAD_CFG		            _IOW(TWOLF_MAGIC, 21,  int)
#endif
/*------------------------------------------------------*/
/*TWOLF REGisters*/
#define ZL38040_CMD_REG             0x0032   /*Host Command register*/
#define ZL38040_CMD_IDLE            0x0000  /*idle/ operation complete*/
#define ZL38040_CMD_NO_OP           0x0001  /*no-op*/
#define ZL38040_CMD_IMG_CFG_LOAD    0x0002  /*load firmware and CR from flash*/
#define ZL38040_CMD_IMG_LOAD        0x0003  /*load firmware only from flash*/
#define ZL38040_CMD_IMG_CFG_SAVE    0x0004  /*save a firmware and CR to flash*/
#define ZL38040_CMD_IMG_CFG_ERASE   0x0005  /*erase a firmware and CR in flash*/
#define ZL38040_CMD_CFG_LOAD        0x0006  /*Load CR from flash*/
#define ZL38040_CMD_CFG_SAVE        0x0007  /*save CR to flash*/
#define ZL38040_CMD_FWR_GO          0x0008  /*start/restart firmware (GO)*/
#define ZL38040_CMD_HOST_LOAD_CMP   0x000D  /*Host Application Load Complete*/
#define ZL38040_CMD_HOST_FLASH_INIT 0x000B  /*Host Application flash discovery*/
#define ZL38040_CMD_FWR_STOP        0x8000  /*stop firmware */
#define ZL38040_CMD_CMD_IN_PROGRESS 0xFFFF  /*wait command is in progress */
#define ZL38040_CMD_APP_SLEEP		 0x8005  /*codec low power mode*/

#define PAGE_255_CHKSUM_LO_REG  0x000A
#define PAGE_255_CHKSUM_HI_REG  0x0008
#define CLK_STATUS_REG          0x0014   /*Clock status register*/
#define PAGE_255_BASE_LO_REG  0x000E
#define PAGE_255_BASE_HI_REG  0x000C
#define ZL38040_SW_FLAGS_REG     0x0006
#define ZL38040_SW_FLAGS_CMD     0x0001
#define ZL38040_SW_FLAGS_CMD_NORST     0x0004

#define ZL38040_DEVICE_ID_REG  0x0022

#define TWOLF_CLK_STATUS_HBI_BOOT       0x0001

#define HBI_CONFIG_REG			0xFD00
#define HBI_CONFIG_WAKE			1<<7
#define HBI_CONFIG_VAL (HBI_CONFIG_INT_PIN_DRIVE_MODE<<1)

#define ZL38040_CMD_PARAM_RESULT_REG   0x0034 /*Host Command Param/Result register*/
#define ZL38040_FWR_COUNT_REG   0x0026 /*Fwr on flash count register*/
#define ZL38040_FWR_EXEC_REG   0x012C  /*Fwr EXEC register*/

#define TOTAL_FWR_DATA_WORD_PER_LINE 24
#define TOTAL_FWR_DATA_BYTE_PER_LINE 128
#define TWOLF_STATUS_NEED_MORE_DATA 22
#define TWOLF_STATUS_BOOT_COMPLETE 23

#define TWOLF_MBCMDREG_SPINWAIT  10000
/*--------------------------------------------------------------------
 *    ALSA
 *--------------------------------------------------------------------*/
 /*Macros to enable one of the pre-defined audio cross-points*/
#define ZL38040_CR2_DEFAULT 0
#define ZL38040_CR2_STEREO_BYPASS      1
#define ZL38040_ADDA_LOOPBACK      2

/*Cached register range*/
#define ZL38040_CACHED_ADDR_LO  0x202
#define ZL38040_CACHED_ADDR_HI  0x23E
#define ZL38040_HBI_OFFSET_RANGE 128
#define ZL38040_CACHE_INDEX_TO_ADDR(index) (ZL38040_CACHED_ADDR_LO+(2*index))
#define ZL38040_ADDR_TO_CACHE_INDEX(addr) ((addr - ZL38040_CACHED_ADDR_LO)/2)


/*Page 1 registers*/
#define ZL38040_OUTPUT_PATH_EN_REG 	0x202
#define ZL38040_DAC1_EN  	(1 << 0)
#define ZL38040_DAC2_EN  	(1 << 1)
#define ZL38040_TDMA1_EN 	(1 << 2)
#define ZL38040_TDMA2_EN 	(1 << 3)
#define ZL38040_TDMA3_EN 	(1 << 4)
#define ZL38040_TDMA4_EN 	(1 << 5)
#define ZL38040_TDMB1_EN 	(1 << 6)
#define ZL38040_TDMB2_EN 	(1 << 7)
#define ZL38040_TDMB3_EN 	(1 << 8)
#define ZL38040_TDMB4_EN 	(1 << 9)
#define ZL38040_MIC_SIN_EN 	(1 << 10)
#define ZL38040_TDM_SOUT_EN (1 << 11)
/*Cross-point Audio config registers*/
#define ZL38040_DAC1_IN_PATH_REG 	0x210
#define ZL38040_DAC2_IN_PATH_REG 	0x212
#define ZL38040_TDM1L_IN_PATH_REG 	0x214
#define ZL38040_TDM1R_IN_PATH_REG 	0x216
#define ZL38040_TDMA3_IN_PATH_REG 	0x218
#define ZL38040_TDMA4_IN_PATH_REG 	0x21A
#define ZL38040_TDM2L_IN_PATH_REG 	0x21C
#define ZL38040_TDM2R_IN_PATH_REG 	0x21E
#define ZL38040_TDMB3_IN_PATH_REG 	0x220
#define ZL38040_TDMB4_IN_PATH_REG 	0x222
#define ZL38040_SIN_IN_PATH_REG 	0x224
#define ZL38040_RIN_IN_PATH_REG 	0x226
/*Cross-point Audio config values*/
#define ZL38040_MIC1_PATH 	0x01
#define ZL38040_MIC2_PATH 	0x02
#define ZL38040_MIC3_PATH 	0x03
#define ZL38040_MIC4_PATH 	0x04
#define ZL38040_MIC_SELECT  ZL38040_MIC1_PATH  /*Change this accordingly*/
#define ZL38040_TDMA1L_PATH 0x05
#define ZL38040_TDMA1R_PATH 0x06
#define ZL38040_TDMA3_PATH 	0x07
#define ZL38040_TDMA4_PATH 	0x08
#define ZL38040_TDMB2L_PATH 0x09
#define ZL38040_TDMB2R_PATH 0x0A
#define ZL38040_TDMB3_PATH 	0x0B
#define ZL38040_TDMB4_PATH 	0x0C
#define ZL38040_ROUT_PATH 	0x0D
#define ZL38040_SOUT_PATH 	0x0E
#define ZL38040_TGEN1_PATH 	0x0F
#define ZL38040_TGEN2_PATH 	0x10

#define ZL38040_TDMA_CFG_REG 		0x260
#define ZL38040_TDM_I2S_CFG_VAL 0x8000
#define ZL38040_TDM_PCM_CFG_VAL 0x0000
#define ZL38040_TDM_CLK_POL_VAL 0x0004
#define ZL38040_TDMA_FSALIGN 0x01     /*left justified*/
#define ZL38040_TDMA_CLK_CFG_REG    	0x262
#define ZL38040_TDM_TDM_MASTER_VAL (1<<15)
#define ZL38040_TDMA_CH1_CFG_REG    	0x268
#define ZL38040_TDMA_CH2_CFG_REG    	0x26A
/*TDM -  Channel configuration*/
#define ZL38040_TDMA_16BIT_LIN (1<<8)
#define ZL38040_TDMA_8BIT_ALAW (2<<8)
#define ZL38040_TDMA_8BIT_ULAW (3<<8)
#define ZL38040_TDMA_8BIT_G722 (4<<8)
#define ZL38040_TDMA_16BIT_LINHFS (6<<8)

#define ZL38040_TDMA_FSRATE_8KHZ (1)
#define ZL38040_TDMA_FSRATE_16KHZ (2)
#define ZL38040_TDMA_FSRATE_24KHZ (3)
#define ZL38040_TDMA_FSRATE_44_1KHZ (5)
#define ZL38040_TDMA_FSRATE_48KHZ (6)

#define ZL38040_MIC_EN_REG 			0x2B0
#define ZL38040_MIC1_EN		0x01
#define ZL38040_MIC2_EN		0x02
#define ZL38040_MIC3_EN		0x04
#define ZL38040_MIC4_EN		0x08

#define ZL38040_LOW_POWER_REG  0x0206

#define ZL38040_DAC1_EN_REG  0x02A0
#define ZL38040_DAC2_EN_REG  0x02A2
#define ZL38040_DACx_P_EN  (1<<15)
#define ZL38040_DACx_M_EN  (1<<14)




/*Page 2 registers*/
#define ZL38040_USRGAIN		  0x30A
#define ZL38040_SYSGAIN		  0x30C
#define ZL38040_MICGAIN		  0x2B2 /*Range 0-7 step +/-1 = 6dB each*/

#define ZL38040_DAC_CTRL_REG  0x030A   /*ROUT GAIN control*/
#define ZL38040_DAC_VOL_MAX   0x78     /*Max volume control for Speaker +21dB*/
#define ZL38040_DAC_VOL_MAX_EXT   0x82     /*Max volume control for Speaker +29dB*/
#define ZL38040_DAC_VOL_MIN   0x00     /*Min volume control for Speaker -24dB*/
#define ZL38040_DAC_VOL_STEP  0x01     /*volume step control for Speaker -/+0.375dB*/

#define ZL38040_MIC_VOL_CTRL_REG  0x030C    /*SIN GAIN control*/
#define ZL38040_MIC_VOL_MAX   0x1F     /*Max volume control for Speaker +22.5dB*/
#define ZL38040_MIC_VOL_MIN   0x00     /*Min volume control for Speaker -24dB*/
#define ZL38040_MIC_VOL_STEP  0x01     /*volume step control for Speaker -/+1.5dB*/
#define ZL38040_SOUT_VOL_CTRL_REG  0x030C    /*SOUT DIGITAL GAIN control*/
#define ZL38040_SOUT_VOL_MAX   0x0F     /*Max volume control for Speaker +21dB*/
#define ZL38040_SOUT_VOL_MIN   0x00     /*Min volume control for Speaker -24dB*/
#define ZL38040_SOUT_VOL_STEP  0x01     /*volume step control for Speaker -/+3.0dB*/

#define ZL38040_DAC1_GAIN_REG  0x0238
#define ZL38040_DAC2_GAIN_REG  0x023A
#define ZL38040_I2S1L_GAIN_REG  0x023C
#define ZL38040_I2S1R_GAIN_REG  0x023E
#define ZL38040_I2S2L_GAIN_REG  0x0244
#define ZL38040_I2S2R_GAIN_REG  0x0246
#define ZL38040_TDMA3_GAIN_REG  0x0240
#define ZL38040_TDMA4_GAIN_REG  0x0242
#define ZL38040_TDMB3_GAIN_REG  0x0248
#define ZL38040_TDMB4_GAIN_REG  0x024A



#define ZL38040_AEC_CTRL_REG1  0x0302
#define ZL38040_AEC_CTRL_REG0  0x0300
#define ZL38040_EAC_RST_EN  (1 << 0)
#define ZL38040_MASTER_BYPASS_EN  (1 << 1)
#define ZL38040_EQ_RCV_DIS_EN  (1 << 2)
#define ZL38040_AEC_BYPASS_EN  (1 << 4)
#define ZL38040_AUD_ENH_BYPASS_EN  (1 << 5)
#define ZL38040_SPKR_LIN_EN  (1 << 6)
#define ZL38040_MUTE_ROUT_EN  (1 << 7)
#define ZL38040_MUTE_SOUT_EN  (1 << 8)
#define ZL38040_MUTE_ALL_EN   (ZL38040_MUTE_ROUT_EN | ZL38040_MUTE_SOUT_EN)
#define ZL38040_RIN_HPF_DIS_EN  (1 << 9)
#define ZL38040_SIN_HPF_DIS_EN  (1 << 10)
#define ZL38040_HOWLING_DIS_EN  (1 << 11)
#define ZL38040_AGC_DIS_EN  (1 << 12)
#define ZL38040_NB_DIS_EN  (1 << 13)
#define ZL38040_SATT_DIS_EN  (1 << 14)
#define ZL38040_HOWLING_MB_DIS_EN  (1 << 15)
#define ZL38040_HPF_DIS (ZL38040_RIN_HPF_DIS_EN | ZL38040_SIN_HPF_DIS_EN)

#define ZL38040_LEC_CTRL_REG  0x037A

#define ZL38040_AEC_HPF_NULL_REG  0x0310


#endif /* __MICROSEMI_SPIS_TW_H */

