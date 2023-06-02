/**
    NVT mmc function
    Define parameters and initial register value
    @file       na51000_mmchost.h
    @ingroup
    @note
    Copyright   Novatek Microelectronics Corp. 2016.  All rights reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#ifndef _SDIO_HOST_H
#define _SDIO_HOST_H

#include <mach/rcw_macro.h>
#include <linux/module.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/cpufreq.h>
#include <linux/mmc/host.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/mmc/mmc.h>
#include <linux/printk.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>
#include <plat/top.h>
#include <plat/pad.h>
#include <plat/nvt-mmc.h>
#include <plat/hardware.h>
#include "na51055_mmcreg.h"

/*pll.h*/
#define SDIO_CLK        34          /* SDIO  clock*/
#define SDIO2_CLK       35          /* SDIO2 clock*/
#define SDIO3_CLK       46          /* SDIO3 clock*/
/**********************************************************************/
/* Definitions */
/**********************************************************************/

/*
    SDIO host number encoding.
*/

#define SDIO_HOST_ID_1                      (0)     /* SDIO host*/
#define SDIO_HOST_ID_2                      (1)     /* SDIO2 host*/
#define SDIO_HOST_ID_3                      (2)     /* SDIO3 host*/
#define SDIO_HOST_ID_COUNT                  (3)     /* SDIO host count*/


/*
    SDIO send command execution result.

    Encoding of sdiohost_sendcmd() result.
*/

#define SDIO_HOST_CMD_OK                    (0)     /* command execution OK*/
#define SDIO_HOST_RSP_TIMEOUT               (-1)    /* response timeout*/
#define SDIO_HOST_RSP_CRCFAIL               (-2)    /* response CRC fail*/
#define SDIO_HOST_CMD_FAIL                  (-3)    /* command fail*/


/*
    SDIO data transfer result.

    Encoding of sdiohost_waitdataend() result.
*/

#define SDIO_HOST_DATA_OK                   (0)     /* data transfer OK*/
#define SDIO_HOST_DATA_TIMEOUT              (-1)    /* data block timeout*/
#define SDIO_HOST_DATA_CRCFAIL              (-2)    /* data block CRC fail*/
#define SDIO_HOST_DATA_FAIL                 (-3)    /* data transfer fail*/


#define SDIO_HOST_BOOT_ACK_OK               (0)
#define SDIO_HOST_BOOT_ACK_TIMEOUT          (-1)
#define SDIO_HOST_BOOT_ACK_ERROR            (-2)
#define SDIO_HOST_BOOT_END                  (0)
#define SDIO_HOST_BOOT_END_ERROR            (-1)

/*
    SDIO IO interrupt

    sdiohost_waitio() result
*/
#define SDIO_IO_INT_OK                      (0)     /* IO interrupt ok*/
#define SDIO_IO_INT_FAIL                    (-1)    /* IO interrupt fail*/

/*
    SDIO Support Voltage
*/
#define VOLTAGE_1800                        (1800)
#define VOLTAGE_3300                        (3300)

/*
    User mode pad driving definition
*/
#define PAD_DRIVING_5MA 5
#define PAD_DRIVING_10MA 10
#define PAD_DRIVING_15MA 15
#define PAD_DRIVING_20MA 20
#define PAD_DRIVING_25MA 25
#define PAD_DRIVING_30MA 30
#define PAD_DRIVING_35MA 35
#define PAD_DRIVING_40MA 40
#define PAD_DRIVING_6MA 6
#define PAD_DRIVING_16MA 16
#define PAD_DRIVING_4MA 4
#define PAD_DRIVING_8MA 8

/*
    SDIO response type

    @note for sdiohost_sendcmd()
*/
typedef enum {
	SDIO_HOST_RSP_NONE,         /* No response*/
	SDIO_HOST_RSP_SHORT,        /* Short response*/
	SDIO_HOST_RSP_LONG,         /* Long response*/
	SDIO_HOST_RSP_SHORT_TYPE2,  /* Short response timeout is 5 bus clock*/
	SDIO_HOST_RSP_LONG_TYPE2,   /* Long response timeout is 5 bus clock*/
	SDIO_HOST_RSP_VOLT_DETECT,  /* voltage detect response*/
	ENUM_DUMMY4WORD(SDIO_HOST_RESPONSE)
} SDIO_HOST_RESPONSE;

#define SDIO_HOST_MAX_VOLT_TIMER (0xFFF) /* max value of voltage switch timer*/

#define SDIO_DES_TABLE_NUM        (128)
#define SDIO_DES_WORD_SIZE        (3)   /*descriptor 3 word*/
#define SDIO_HOST_MAX_DATA_LENGTH (64*1024*1024)
#define SDIO_HOST_DATA_FIFO_DEPTH (16)

/*sdio_protocol.h*/
#define SDIO_HOST_WRITE_DATA                (FALSE)
#define SDIO_HOST_READ_DATA                 (TRUE)


/* Command Register Bit*/
#define SDIO_CMD_REG_INDEX                  0x0000003F  /* bit 5..0*/
#define SDIO_CMD_REG_NEED_RSP               0x00000040  /* bit 6*/
#define SDIO_CMD_REG_LONG_RSP               0x000000C0  /* bit 7*/
#define SDIO_CMD_REG_RSP_TYPE2              0x00000100  /* bit 8*/
#define SDIO_CMD_REG_APP_CMD                0x00000000  /* bit x*/
#define SDIO_CMD_REG_ABORT                  0x00000800  /* bit 11*/
#define SDIO_CMD_REG_VOLTAGE_SWITCH_DETECT  0x00001000  /* bit 12*/


/* Status/Interrupt Mask Register Bit*/
#define SDIO_STATUS_REG_RSP_CRC_FAIL        0x00000001  /* bit 0*/
#define SDIO_STATUS_REG_DATA_CRC_FAIL       0x00000002  /* bit 1*/
#define SDIO_STATUS_REG_RSP_TIMEOUT         0x00000004  /* bit 2*/
#define SDIO_STATUS_REG_DATA_TIMEOUT        0x00000008  /* bit 3*/
#define SDIO_STATUS_REG_RSP_CRC_OK          0x00000010  /* bit 4*/
#define SDIO_STATUS_REG_DATA_CRC_OK         0x00000020  /* bit 5*/
#define SDIO_STATUS_REG_CMD_SEND            0x00000040  /* bit 6*/
#define SDIO_STATUS_REG_DATA_END            0x00000080  /* bit 7*/
#define SDIO_STATUS_REG_INT                 0x00000100  /* bit 8*/
#define SDIO_STATUS_REG_READWAIT            0x00000200  /* bit 9*/
#define SDIO_STATUS_REG_EMMC_BOOTACKREV     0x00008000  /* bit 15*/
#define SDIO_STATUS_REG_EMMC_BOOTACKTOUT    0x00010000  /* bit 16*/
#define SDIO_STATUS_REG_EMMC_BOOTEND        0x00020000  /* bit 17*/
#define SDIO_STATUS_REG_EMMC_BOOTACKERR     0x00040000  /* bit 18*/
#define SDIO_STATUS_REG_DMA_ERR             0x00080000  /* bit 19*/
#define SDIO_INTMASK_ALL                    0x000003FF  /* bit 9..0*/

/* Bus Width Register bit definition*/
#define SDIO_BUS_WIDTH1                     0x00000000  /* bit 1..0 : 0x0*/
#define SDIO_BUS_WIDTH4                     0x00000001  /* bit 1..0 : 0x1*/
#define SDIO_BUS_WIDTH8                     0x00000002  /* bit 1..0 : 0x2*/
/*end sdio_protocol.h*/

/* Bus Status Mask Register*/
#define  SDIO_DATA_LVL_MASK	0xF00

/* sdio.h*/
/**
    @name SDIO source clock definition

    @note for sdio_setHSClk() and sdio2_setHSClk()
*/

#define SDIO_SRC_CLK_20MHZ      0   /* No use*/
#define SDIO_SRC_CLK_24MHZ      1   /* No use*/
#define SDIO_SRC_CLK_40MHZ      2   /* Source clock 40Mhz, only for SDIO2*/
#define SDIO_SRC_CLK_48MHZ      3   /* Source clock 48Mhz, SDIO/SDIO2 support*/
#define SDIO_SRC_CLK_60MHZ      4   /* No use*/
#define SDIO_SRC_CLK_80MHZ      5   /* Source clock 80Mhz, only for SDIO*/
#define SDIO_SRC_CLK_SSPLLMHZ   6   /* No use*/
#define SDIO_SRC_CLK_SSPLL2MHZ  7   /* No use*/
#define SDIO_SRC_CLK_SSPLL3MHZ  8   /* No use*/
#define SDIO_SRC_CLK_SSPLL4MHZ  9   /* No use*/
#define SDIO_SRC_CLK_SSPLL5MHZ 10   /* No use*/
#define SDIO_SRC_CLK_96MHZ     11   /* Source clock 96Mhz, only for SDIO*/
#define SDIO_SRC_CLK_PLL4      12   /* Source clock PLL4, SDIO/SDIO2 support*/
/* end sdio.h*/

/*  for nt96650_mmc.c ,the same define */
/* SDIO_STATUS_REG_RSP_CRC_FAIL , SDIO_STATUS_REG_DATA_CRC_FAIL*/
#define   MMCST_RSP_CRC_FAIL                  BIT(0)
#define   MMCST_DATA_CRC_FAIL                 BIT(1)
#define   MMCST_RSP_TIMEOUT                   BIT(2)
#define   MMCST_DATA_TIMEOUT                  BIT(3)
#define   MMCST_RSP_CRC_OK                    BIT(4)
#define   MMCST_DATA_CRC_OK                   BIT(5)
#define   MMCST_CMD_SENT                      BIT(6)
#define   MMCST_DATA_END                      BIT(7)
#define   MMCST_SDIO_INT                      BIT(8)
#define   MMCST_READ_WAIT                     BIT(9)
#define   MMCST_CARD_BUSY2READY               BIT(10)
#define   MMCST_VOL_SWITCH_END                BIT(11)
#define   MMCST_VOL_SWITCH_TIMEOUT            BIT(12)
#define   MMCST_RSP_VOL_SWITCH_FAIL           BIT(13)
#define   MMCST_VOL_SWITCH_GLITCH             BIT(14)
#define   MMCST_EMMC_BOOT_ACK_RECEIVE         BIT(15)
#define   MMCST_EMMC_BOOT_ACK_TIMEOUT         BIT(16)
#define   MMCST_EMMC_BOOT_END                 BIT(17)
#define   MMCST_EMMC_BOOT_ACK_ERROR           BIT(18)
#define   MMCST_DMA_ERROR                     BIT(19)
/*  end for nt96650_mmc.c ,the same define*/

#define SDIO_DLY_PHASE_UNIT_SFT	4
#define SDIO_DLY_CMD_PHASE_SFT	0
#define SDIO_DLY_DATA_SFT	20
#define SDIO_DLY_SAMPEDGE_DATA_SFT	12
#define SDIO_DLY_SAMPEDGE_DATA_POS	(0<<SDIO_DLY_SAMPEDGE_DATA_SFT)
#define SDIO_DLY_SAMPEDGE_DATA_NEG	(1<<SDIO_DLY_SAMPEDGE_DATA_SFT)
#define SDIO_DLY_CLK_SRC	13
#define SDIO_DLY_CLK_PAD	14  /* for 528 platform */
#define SDIO_DLY_DET_SEL	8
#define SDIO_DLY_DET_AUTO	19

#define SDIO_DLY_DS_DEFAULT             (SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(0<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(0<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(1<<SDIO_DLY_CLK_SRC) + \
						(0<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)
#define SDIO_DLY_HS_DEFAULT             (SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(0<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(0<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(1<<SDIO_DLY_CLK_SRC) + \
						(0<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)
#define SDIO_DLY_SDR50_DEFAULT      	(SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(2<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(6<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(1<<SDIO_DLY_CLK_SRC) + \
						(0<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)
#define SDIO_DLY_SDR104_DEFAULT		(SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(2<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(8<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(1<<SDIO_DLY_CLK_SRC) + \
						(0<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)

#define SDIO_DLY_DS_528_DEFAULT             (SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(0<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(0<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(0<<SDIO_DLY_CLK_SRC) + \
						(1<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)
#define SDIO_DLY_HS_528_DEFAULT             (SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(0<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(0<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(0<<SDIO_DLY_CLK_SRC) + \
						(1<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)
#define SDIO_DLY_SDR50_528_DEFAULT      	(SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(2<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(6<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(0<<SDIO_DLY_CLK_SRC) + \
						(1<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)
#define SDIO_DLY_SDR104_528_DEFAULT		(SDIO_DLY_SAMPEDGE_DATA_POS)+ \
						(2<<SDIO_DLY_PHASE_UNIT_SFT) + \
						(8<<SDIO_DLY_CMD_PHASE_SFT) + \
						(0<<SDIO_DLY_DATA_SFT) + \
						(0<<SDIO_DLY_CLK_SRC) + \
						(1<<SDIO_DLY_CLK_PAD) + \
						(1<<SDIO_DLY_DET_SEL) + \
						(1<<SDIO_DLY_DET_AUTO)

/*
    SDIO voltage switch result.

    Encoding of sdioHost_waitVoltageSwitch() result.
*/

typedef enum {
	SDIO_HOST_VOLT_SWITCH_OK,                   /* voltage switch OK*/
	SDIO_HOST_VOLT_SWITCH_TIMEOUT,              /* voltage switch timeout*/
	ENUM_DUMMY4WORD(SDIO_HOST_VOLT_SWITCH_RESULT)
} SDIO_HOST_VOLT_SWITCH_RESULT;




/*
    SDIO boot configuration

    SDIO boot configuration fr sdiohost_setbootconfig()
*/
/*
typedef enum
{
	SDIO_HOST_BOOT_ACK_DIS = 0,
	SDIO_HOST_BOOT_ACK_EN,
	ENUM_DUMMY4WORD(SDIO_HOST_BOOT_ACK)
} SDIO_HOST_BOOT_ACK;

typedef enum
{
	SDIO_HOST_BOOT_MODE_CMDLOW = 0,
	SDIO_HOST_BOOT_MODE_ALT,
	ENUM_DUMMY4WORD(SDIO_HOST_BOOT_ALT)
} SDIO_HOST_BOOT_ALT;

typedef enum
{
	SDIO_HOST_BOOT_CLK_ALWAYS = 0,
	SDIO_HOST_BOOT_CLK_PAUSE,
	ENUM_DUMMY4WORD(SDIO_HOST_BOOT_CLK)
} SDIO_HOST_BOOT_CLK;

typedef enum
{
	SDIO_HOST_BOOT_BUS_WIDTH_1 = 0,
	SDIO_HOST_BOOT_BUS_WIDTH_4,
	SDIO_HOST_BOOT_BUS_WIDHT_8,
	ENUM_DUMMY4WORD(SDIO_HOST_BOOT_BUSWIDTH)
} SDIO_HOST_BOOT_BUSWIDTH;


typedef enum
{
	SDIO_HOST_BOOT_SPEED_SDR = 0,
	SDIO_HOST_BOOT_SPEED_DDR,
	ENUM_DUMMY4WORD(SDIO_HOST_BOOT_DDR)
} SDIO_HOST_BOOT_DDR;



struct {
	SDIO_HOST_BOOT_ACK   uibootack;
	SDIO_HOST_BOOT_ALT   uibootalt;
	SDIO_HOST_BOOT_CLK   uibootclk;
	SDIO_HOST_BOOT_BUSWIDTH uibootbuswidth;
	SDIO_HOST_BOOT_DDR   uibootddr;
}SDIO_HOST_BOOT_CONFIG,*PSDIO_HOST_BOOT_CONFIG;
*/

typedef enum {
	SDIO_HOST_SETPOWER_VOL_3P3 = 0,
	SDIO_HOST_SETPOWER_VOL_1P8,
	SDIO_HOST_SETPOWER_VOL_0,
	ENUM_DUMMY4WORD(SDIO_HOST_SETPOWER_VOL)
} SDIO_HOST_SETPOWER_VOL;

/**
     SDIO Scatter DMA Descriptor

     SDIO Scatter DMA Descriptor
     @note for STRG_EXT_CMD_SDIO_SEG_DES
*/
struct STRG_SEG_DES {
	uint32_t  uisegaddr;                 /* segment address*/
	uint32_t  uisegsize;                 /* segment size*/
};

typedef enum {
	SDIO_MODE_DS = 25000000,
	SDIO_MODE_HS = 50000000,
	SDIO_MODE_SDR50 = 100000000,
	SDIO_MODE_SDR104 = 208000000,
	ENUM_DUMMY4WORD(SDIO_SPEED_MODE)
} SDIO_SPEED_MODE;

enum SDIO_MODE_DRIVING {
	SDIO_DS_MODE_CLK = 0,
	SDIO_DS_MODE_CMD,
	SDIO_DS_MODE_DATA,
	SDIO_HS_MODE_CLK,
	SDIO_HS_MODE_CMD,
	SDIO_HS_MODE_DATA,
	SDIO_SDR50_MODE_CLK,
	SDIO_SDR50_MODE_CMD,
	SDIO_SDR50_MODE_DATA,
	SDIO_SDR104_MODE_CLK,
	SDIO_SDR104_MODE_CMD,
	SDIO_SDR104_MODE_DATA,
	SDIO_MAX_MODE_DRIVING,
};

struct mmc_nvt_host {
	struct mmc_command *cmd;
	struct mmc_data *data;
	struct mmc_host *mmc;
	struct clk *clk;
	unsigned int mmc_input_clk;
	void __iomem *base;
	struct resource *mem_res;
	int id;
	int mmc_irq;
	int mmc_cd_irq;
	unsigned char bus_mode;
	unsigned char data_dir;
	unsigned char voltage_switch;
	u32 status;
	spinlock_t lock;

	/* buffer is used during PIO of one scatterlist segment, and
	 * is updated along with buffer_bytes_left.  bytes_left applies
	 * to all N blocks of the PIO transfer.
	 */
	u8 *buffer;
	u32 buffer_bytes_left;
	u32 bytes_left;

	/*u32 rxdma, txdma;*/
	bool use_dma;
	bool do_dma;
	bool sdio_int;

	/*early data*/
	bool data_early;

	int cd_gpio;
	int ro_gpio;
	int power_en;
	int cp_gpio;
	bool cd_detect_edge;
	bool ro_detect_edge;
	bool power_detect_edge;
	bool cp_gpio_value;
	int max_voltage;
	int cd_state;
	int neg_sample_edge;
	bool no_cd_flag;
	bool no_cd_need_power_cycle;
	int indly_sel;
	bool scan_indly_engineering_mode;
	bool force_power_cycle;
	int force_power_cycle_period;
	int power_down_delay_ms;

	/* Defines pad_driving of each mode, as:
	 * DS_MODE_CLK
	 * DS_MODE_CMD
	 * DS_MODE_DATA
	 * HS_MODE_CLK
	 * HS_MODE_CMD
	 * HS_MODE_DATA
	 * SDR50_MODE_CLK
	 * SDR50_MODE_CMD
	 * SDR50_MODE_DATA
	 * SDR104_MODE_CLK
	 * SDR104_MODE_CMD
	 * SDR104_MODE_DATA
	 */
	u32 pad_driving[SDIO_MAX_MODE_DRIVING];

	/* Scatterlist DMA uses one or more parameter RAM entries:
	 * the main one (associated with rxdma or txdma) plus zero or
	 * more links.  The entries for a given transfer differ only
	 * by memory buffer (address, length) and link field.
	 */
	/*struct edmacc_param	tx_template;*/
	/*struct edmacc_param	rx_template;*/
	unsigned		n_link;
	/*u32			links[MAX_NR_SG - 1];*/

	/* For PIO we walk scatterlists one segment at a time. */
	unsigned int		sg_len;
	struct scatterlist *sg;

	/* Version of the MMC/SD controller */
	u8 version;
	/* for ns in one cycle calculation */
	unsigned ns_in_one_cycle;
	/* Number of sg segments */
	u8 nr_sg;

	struct completion voltage_switch_complete;
	struct completion tuning_data_end;
	int voltage_switch_timeout;

	unsigned int vuisdio_destab[SDIO_DES_TABLE_NUM * SDIO_DES_WORD_SIZE];

#ifdef SDIO_SCATTER_DMA
	struct STRG_SEG_DES tmpdestable[SDIO_DES_TABLE_NUM];
#endif
};

/*********************************************************************/
/*  Function protopyes*/
/*********************************************************************/

/*
    host related function prototypes
*/

extern bool  sdiohost_getrdy(struct mmc_nvt_host *host, uint32_t id);
extern void  sdiohost_enclockout(struct mmc_nvt_host *host, uint32_t id);
extern void  sdiohost_disclockout(struct mmc_nvt_host *host, uint32_t id);

extern void   sdiohost_reset(struct mmc_nvt_host *host, uint32_t id);
extern int    sdiohost_sendcmd(struct mmc_nvt_host *host, uint32_t id,
		uint32_t cmd, SDIO_HOST_RESPONSE rsptype, bool beniointdetect);
extern uint32_t sdiohost_waitdataend(uint32_t id);
extern SDIO_HOST_VOLT_SWITCH_RESULT sdiohost_waitvoltageswitch(uint32_t id,
		uint32_t uitimeout);
extern void     sdiohost_setarg(struct mmc_nvt_host *host, uint32_t id,
		uint32_t arg);
extern void     sdiohost_getshortrsp(struct mmc_nvt_host *host, uint32_t id,
		uint32_t *prsp);
extern void     sdiohost_getlongrsp(struct mmc_nvt_host *host, uint32_t id,
		uint32_t *prsp3, uint32_t *prsp2, uint32_t *prsp1, uint32_t *prsp0);
extern void     sdiohost_setinten(struct mmc_nvt_host *host, uint32_t id,
		uint32_t bits);
extern void     sdiohost_disinten(struct mmc_nvt_host *host, uint32_t id,
		uint32_t bits);
extern void     sdiohost_setbuswidth(struct mmc_nvt_host *host, uint32_t id,
		uint32_t width);
extern uint32_t   sdiohost_getbuswidth(struct mmc_nvt_host *host, uint32_t id);
extern void     sdiohost_enclkout(struct mmc_nvt_host *host, uint32_t id,
		bool enableflag);
extern void     sdiohost_enclkddr(struct mmc_nvt_host *host, uint32_t id,
		bool bddr);
extern void     sdiohost_setbusclk(struct mmc_nvt_host *host, uint32_t id,
		uint32_t uiclock, uint32_t *ns);
extern uint32_t   sdiohost_getbusclk(struct mmc_nvt_host *host, uint32_t id);
extern void     sdiohost_setclktype(struct mmc_nvt_host *host, uint32_t id,
		bool brisingsample);
extern void     sdiohost_getclktype(struct mmc_nvt_host *host, uint32_t id,
		bool *brisingsample);
extern void     sdiohost_setclkcmdtype(struct mmc_nvt_host *host, uint32_t id,
		bool brisingsample);
extern uint32_t sdiohost_getblksize(struct mmc_nvt_host *host, uint32_t id);
extern void sdiohost_setblksize(struct mmc_nvt_host *host, uint32_t id,
		uint32_t size);
extern void sdiohost_setdatatimeout(struct mmc_nvt_host *host, uint32_t id,
		uint32_t timeout);
extern void sdiohost_resetdata(struct mmc_nvt_host *host, uint32_t id);
extern void sdiohost_waitfifoempty(struct mmc_nvt_host *host, uint32_t id);
extern void sdiohost_clrfifoen(struct mmc_nvt_host *host, uint32_t id);
extern uint32_t sdiohost_getfifodir(struct mmc_nvt_host *host, uint32_t id);
extern void sdiohost_setupdatatransferdma(struct mmc_nvt_host *host,
		uint32_t id, uint32_t uidmaaddress,
		uint32_t uidatalength, bool bisread, uint32_t *vuisdio_destab);
extern void sdiohost_setupdatatransferpio(struct mmc_nvt_host *host,
		uint32_t id, uint32_t uidmaaddress,
		uint32_t uidatalength, bool bisread);
extern int sdiohost_writeblock(struct mmc_nvt_host *host, uint32_t id,
		uint8_t *pbuf, uint32_t uilength);
extern int sdiohost_readblock(struct mmc_nvt_host *host, uint32_t id,
		uint8_t *pbuf, uint32_t uilength);
extern uint32_t sdiohost_setiointen(struct mmc_nvt_host *host, uint32_t id,
		bool ben);
extern bool   sdiohost_getiointen(struct mmc_nvt_host *host, uint32_t id);

extern int sdiohost_open(struct mmc_nvt_host *host, uint32_t id, int volatge_switch);
extern int sdiohost_sendsdcmd(struct mmc_nvt_host *host, uint32_t id,
		uint32_t cmdpart, uint32_t param);
extern void sdiohost_setstatus(struct mmc_nvt_host *host, uint32_t id,
		uint32_t status);
extern uint32_t sdiohost_getstatus(struct mmc_nvt_host *host, uint32_t id);
extern void sdiohost_setdesen(uint32_t id, uint32_t uien);
extern void sdiohost_setdestab(uint32_t id, uint32_t uidesaddr, uint32_t uidesnum,
			       uint32_t *vuisdio_destab);

extern void sdiohost_setpower(struct mmc_nvt_host *host, \
				SDIO_HOST_SETPOWER_VOL vol);

extern int sdiohost_getpower(struct mmc_nvt_host *host);

extern uint32_t sdiohost_getdata_status(struct mmc_nvt_host *host, uint32_t id);

extern u32 sdiohost_set_voltage_switch(struct mmc_nvt_host *host);

extern int sdiohost_setpaddriving(struct mmc_nvt_host *host, SDIO_SPEED_MODE mode);

extern void sdiohost_delay_chain(struct mmc_nvt_host *host, uint32_t clock_delay,
			uint32_t delay_chain_setting);

extern int sdiohost_tuning_cmd(struct mmc_nvt_host *host, u32 opcode, bool print_pattern_err);

extern void sdiohost_setphyclrdetval(struct mmc_nvt_host *host);

extern void sdiohost_setphydetch(struct mmc_nvt_host *host, u32 ch);

extern int sdiohost_getcmd(struct mmc_nvt_host *host);

extern void sdiohost_power_switch(struct mmc_nvt_host *host, bool enable);

extern u32 sdiohost_getphydetout(struct mmc_nvt_host *host);

extern void sdiohost_setdlyphase_sel(struct mmc_nvt_host *host, u32 sel);

extern void sdiohost_setphyphase_cmpen(struct mmc_nvt_host *host, u32 sel);

extern void sdiohost_power_down(struct mmc_nvt_host *host);

extern void sdiohost_power_up(struct mmc_nvt_host *host);

extern void sdiohost_stroredatarerult(uint32_t id, uint32_t dataintsts);

extern void sdio_copy_info(struct mmc_nvt_host *info);

extern void sdiohost_power_cycle(struct mmc_nvt_host *host, uint32_t delay_ms);

extern void sdiohost_set_indly_sel(struct mmc_nvt_host *host, uint32_t indly_sel);

extern void sdiohost_set_paddirection(struct mmc_nvt_host *host);
#endif
