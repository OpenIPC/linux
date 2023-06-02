/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/mach/sd.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/

#ifndef __MACH_GK_SD_H
#define __MACH_GK_SD_H

#include <mach/gpio.h>

/* ==========================================================================*/
#define GK_SD_MAX_SLOT_NUM              (2)

#define GK_SD_PRIVATE_CAPS_VDD_18       (0x1 << 0)
#define GK_SD_PRIVATE_CAPS_ADMA         (0x1 << 1)
#define GK_SD_PRIVATE_CAPS_DTO_BY_SDCLK (0x1 << 2)
#define GK_SD_PRIVATE_CAPS_DDR          (0x1 << 3)

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct gk_sd_slot {
    struct mmc_host             *pmmc_host;

    u32                         default_caps;
    u32                         active_caps;
    u32                         default_caps2;
    u32                         active_caps2;
    u32                         private_caps;

    struct gk_gpio_io_info      ext_power;
    struct gk_gpio_io_info      ext_reset;
    int                         fixed_cd;
    struct gk_irq_info          gpio_cd;
    u32                         cd_delay;    //jiffies
    int                         fixed_wp;
    struct gk_gpio_io_info      gpio_wp;

    int                         (*check_owner)(void);
    void                        (*request)(void);
    void                        (*release)(void);
    void                        (*set_int)(u32 mask, u32 on);
    void                        (*set_vdd)(u32 vdd);
    void                        (*set_bus_timing)(u32 timing);
};

struct gk_sd_controller {
    u32                     num_slots;
    struct gk_sd_slot   slot[GK_SD_MAX_SLOT_NUM];
    void                    (*set_pll)(u32);
    u32                     (*get_pll)(void);

    u32                     max_blk_mask;
    u32                     max_clock;
    u32                     active_clock;
    u32                     wait_tmo;
    u32                     pwr_delay;    //ms

    u32                     dma_fix;
    u32                     support_pll_scaler;
};

/* ==========================================================================*/
extern struct platform_device               gk_sd0;

extern int gk_init_sd(void);
extern void gk_detect_sd_slot(int bus, int slot, int fixed_cd);
extern void gk_set_sd_detect_pin(u32 gpio_pin);


#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#define SD_INSTANCES                        1
#define SD_SUPPORT_PLL_SCALER               0
#define SD_HAS_INTERNAL_MUXER               1
#define SD_HAS_INTERNAL_2ND_CDWP            0
#define SD_HAS_DELAY_CTRL                   0
#define SD_BUS_SWITCH_DLY                   0
#define SD_HAS_IO_DRIVE_CTRL                1

#define SD_HOST1_SUPPORT_XC                 0
#define SD_HOST2_SUPPORT_XC                 0

#define SD_SUPPORT_ACMD23                   1

#define SD_HAS_SDXC_CLOCK                   0

#define SD_HOST1_HOST2_HAS_MUX              0
#define SD_SUPPORT_ADMA                     0


/****************************************************/
/* Controller registers definitions                 */
/****************************************************/

#define SD_REG(x)                       (GK_VA_SDC + (x))      //(0xf0002000 + (x))

#define SD_DMA_ADDR_OFFSET              0x000
#define SD_BLK_SZ_OFFSET                0x00c    /* Half word */
#define SD_BLK_CNT_OFFSET               0x00a    /* Half word */
#define SD_ARG_OFFSET                   0x034
#define SD_XFR_OFFSET                   0x004    /* Half word */
#define SD_CMD_OFFSET                   0x016    /* Half word */
#define SD_RSP0_OFFSET                  0x01c
#define SD_RSP1_OFFSET                  0x020
#define SD_RSP2_OFFSET                  0x024
#define SD_RSP3_OFFSET                  0x028
#define SD_DATA_OFFSET                  0x040
#define SD_STA_OFFSET                   0x030
#define SD_HOST_OFFSET                  0x02c    /* Byte */
#define SD_PWR_OFFSET                   0x02d    /* Byte */
#define SD_GAP_OFFSET                   0x02e    /* Byte */
#define SD_WAK_OFFSET                   0x02f    /* Byte */
#define SD_CLK_OFFSET                   0x01a    /* Half word */
#define SD_TMO_OFFSET                   0x018    /* Byte */
#define SD_RESET_OFFSET                 0x019    /* Byte */
#define SD_NIS_OFFSET                   0x012    /* Half word */
#define SD_EIS_OFFSET                   0x014    /* Half word */
#define SD_NISEN_OFFSET                 0x00e    /* Half word */
#define SD_EISEN_OFFSET                 0x010    /* Half word */
#define SD_NIXEN_OFFSET                 0x006    /* Half word */
#define SD_EIXEN_OFFSET                 0x008    /* Half word */
#define SD_AC12ES_OFFSET                0x03c    /* Half word */
#define SD_CAP_OFFSET                   0x038
#define SD_CUR_OFFSET                   0x048
#define SD_ADMA_STA_OFFSET              0x054
#define SD_ADMA_ADDR_OFFSET             0x058
#define SD_XC_CTR_OFFSET                0x060
#define SD_BOOT_CTR_OFFSET              0x070
#define SD_BOOT_STA_OFFSET              0x074
#define SD_VOL_SW_OFFSET                0x07c
#define SD_SIST_OFFSET                  0x0fc    /* Half word */
#define SD_VER_OFFSET                   0x0fe    /* Half word */

#define SD_DMA_ADDR_REG                 SD_REG(SD_DMA_ADDR_OFFSET)
#define SD_BLK_SZ_REG                   SD_REG(SD_BLK_SZ_OFFSET)    /* Half word */
#define SD_BLK_CNT_REG                  SD_REG(SD_BLK_CNT_OFFSET)   /* Half word */
#define SD_ARG_REG                      SD_REG(SD_ARG_OFFSET)
#define SD_XFR_REG                      SD_REG(SD_XFR_OFFSET)       /* Half word */
#define SD_CMD_REG                      SD_REG(SD_CMD_OFFSET)       /* Half word */
#define SD_RSP0_REG                     SD_REG(SD_RSP0_OFFSET)
#define SD_RSP1_REG                     SD_REG(SD_RSP1_OFFSET)
#define SD_RSP2_REG                     SD_REG(SD_RSP2_OFFSET)
#define SD_RSP3_REG                     SD_REG(SD_RSP3_OFFSET)
#define SD_DATA_REG                     SD_REG(SD_DATA_OFFSET)
#define SD_STA_REG                      SD_REG(SD_STA_OFFSET)
#define SD_HOST_REG                     SD_REG(SD_HOST_OFFSET)      /* Byte */
#define SD_PWR_REG                      SD_REG(SD_PWR_OFFSET)       /* Byte */
#define SD_GAP_REG                      SD_REG(SD_GAP_OFFSET)       /* Byte */
#define SD_WAK_REG                      SD_REG(SD_WAK_OFFSET)       /* Byte */
#define SD_CLK_REG                      SD_REG(SD_CLK_OFFSET)       /* Half word */
#define SD_TMO_REG                      SD_REG(SD_TMO_OFFSET)       /* Byte */
#define SD_RESET_REG                    SD_REG(SD_RESET_OFFSET)     /* Byte */
#define SD_NIS_REG                      SD_REG(SD_NIS_OFFSET)       /* Half word */
#define SD_EIS_REG                      SD_REG(SD_EIS_OFFSET)       /* Half word */
#define SD_NISEN_REG                    SD_REG(SD_NISEN_OFFSET)     /* Half word */
#define SD_EISEN_REG                    SD_REG(SD_EISEN_OFFSET)     /* Half word */
#define SD_NIXEN_REG                    SD_REG(SD_NIXEN_OFFSET)     /* Half word */
#define SD_EIXEN_REG                    SD_REG(SD_EIXEN_OFFSET)     /* Half word */
#define SD_AC12ES_REG                   SD_REG(SD_AC12ES_OFFSET)    /* Half word */
#define SD_CAP_REG                      SD_REG(SD_CAP_OFFSET)
#define SD_CUR_REG                      SD_REG(SD_CUR_OFFSET)
#define SD_ADMA_STA_REG                 SD_REG(SD_ADMA_STA_OFFSET)
#define SD_ADMA_ADDR_REG                SD_REG(SD_ADMA_ADDR_OFFSET)
#define SD_XC_CTR_REG                   SD_REG(SD_XC_CTR_OFFSET)
#define SD_BOOT_CTR_REG                 SD_REG(SD_BOOT_CTR_OFFSET)
#define SD_BOOT_STA_REG                 SD_REG(SD_BOOT_STA_OFFSET)
#define SD_VOL_SW_REG                   SD_REG(SD_VOL_SW_OFFSET)
#define SD_SIST_REG                     SD_REG(SD_SIST_OFFSET)      /* Half word */
#define SD_VER_REG                      SD_REG(SD_VER_OFFSET)       /* Half word */

/* SD_BLK_SZ_REG */
#define SD_BLK_SZ_4KB                   0x0000
#define SD_BLK_SZ_8KB                   0x1000
#define SD_BLK_SZ_16KB                  0x2000
#define SD_BLK_SZ_32KB                  0x3000
#define SD_BLK_SZ_64KB                  0x4000
#define SD_BLK_SZ_128KB                 0x5000
#define SD_BLK_SZ_256KB                 0x6000
#define SD_BLK_SZ_512KB                 0x7000

/* SD_XFR_REG */
#define SD_XFR_MUL_SEL                  0x0020
#define SD_XFR_SGL_SEL                  0x0000
#define SD_XFR_CTH_SEL                  0x0010
#define SD_XFR_HTC_SEL                  0x0000
#define SD_XFR_DMA_EN                   0x0004
#define SD_XFR_AC12_EN                  0x0002
#define SD_XFR_BLKCNT_EN                0x0001

/* SD_CMD_REG */
#define SD_CMD_IDX(x)                   ((x) << 8)
#define SD_CMD_NORMAL                   0x00000000
#define SD_CMD_SUSPEND                  0x00000040
#define SD_CMD_RESUME                   0x00000080
#define SD_CMD_ABORT                    0x000000C0
#define SD_CMD_CHKIDX                   0x00000020
#define SD_CMD_DATA                     0x00000010
#define SD_CMD_CHKCRC                   0x00000008

#define SD_CMD_RSP_NONE                 0x00000000
#define SD_CMD_RSP_136                  0x00000001
#define SD_CMD_RSP_48                   0x00000002
#define SD_CMD_RSP_48BUSY               0x00000003

/* SD_STA_REG */
#define SD_STA_DAT_LSL(x)               ((((x) & 0x1e000000) >> 25) |(((x) & 0x00f00000) >> 20))
#define SD_STA_CMD_LSL(x)               (((x)  & 0x01000000) >> 24)
#define SD_STA_WPS_PL                   0x00080000
#define SD_STA_CSS                      0x00040000
#define SD_STA_CDP_L                    0x00020000
#define SD_STA_CARD_INSERTED            0x00010000
#define SD_STA_BUFFER_READ_EN           0x00000800
#define SD_STA_WRITE_XFR_ACTIVE         0x00000400
#define SD_STA_BUFFER_WRITE_EN          0x00000200
#define SD_STA_READ_XFR_ACTIVE          0x00000100
#define SD_STA_CMD_INHIBIT_DAT          0x00000004
#define SD_STA_DAT_ACTIVE               0x00000002
#define SD_STA_CMD_INHIBIT_CMD          0x00000001

#define SD_STA_WRITABLE                 0
#define SD_STA_READ_ONLY                1

/* SD_HOST_REG */
#define SD_HOST_HIGH_SPEED              0x08
#define SD_HOST_8BIT                    0x04
#define SD_HOST_4BIT                    0x02
#define SD_HOST_LED_ON                  0x01

/* SD_PWR_REG */
#define SD_PWR_3_3V                     0x0e
#define SD_PWR_3_0V                     0x0c
#define SD_PWR_1_8V                     0x0a
#define SD_PWR_ON                       0x01
#define SD_PWR_OFF                      0x00

/* SD_GAP_REG */
#define SD_GAP_INT_AT_GAP               0x08
#define SD_GAP_CONT_REQ                 0x04
#define SD_GAP_READ_WAIT                0x02
#define SD_GAP_STOP_AT_GAP              0x01

/* SD_WAK_REG */
#define SD_WAK_ON_CARD_RMV              0x04
#define SD_WAK_ON_CARD_INT              0x02
#define SD_WAK_ON_CARD_IST              0x01

/* SD_CLK_REG */
#define SD_CLK_DIV_256                  0x8000
#define SD_CLK_DIV_128                  0x4000
#define SD_CLK_DIV_64                   0x2000
#define SD_CLK_DIV_32                   0x1000
#define SD_CLK_DIV_16                   0x0800
#define SD_CLK_DIV_8                    0x0400
#define SD_CLK_DIV_4                    0x0200
#define SD_CLK_DIV_2                    0x0100
#define SD_CLK_DIV_1                    0x0000
#define SD_CLK_EN                       0x0004
#define SD_CLK_ICLK_STABLE              0x0002
#define SD_CLK_ICLK_EN                  0x0001

/* SD_TMO_REG */
/* SD_RESET_REG */
#define SD_RESET_DAT                    0x04
#define SD_RESET_ALL                    0x02
#define SD_RESET_CMD                    0x01

/* SD_NIS_REG */
#define SD_NIS_ERROR                    0x8000
#define SD_NIS_CARD                     0x0100
#define SD_NIS_REMOVAL                  0x0080
#define SD_NIS_READ_READY               0x0040
#define SD_NIS_INSERT                   0x0020
#define SD_NIS_WRITE_READY              0x0010
#define SD_NIS_XFR_DONE                 0x0008
#define SD_NIS_DMA                      0x0004
#define SD_NIS_BLOCK_GAP                0x0002
#define SD_NIS_CMD_DONE                 0x0001

/* SD_EIS_REG */
#define SD_EIS_ACMD12_ERR               0x0100
#define SD_EIS_CURRENT_ERR              0x0080
#define SD_EIS_DATA_BIT_ERR             0x0040
#define SD_EIS_DATA_CRC_ERR             0x0020
#define SD_EIS_DATA_TMOUT_ERR           0x0010
#define SD_EIS_CMD_IDX_ERR              0x0008
#define SD_EIS_CMD_BIT_ERR              0x0004
#define SD_EIS_CMD_CRC_ERR              0x0002
#define SD_EIS_CMD_TMOUT_ERR            0x0001


/* SD_NISEN_REG */
#define SD_NISEN_CARD                   0x0100
#define SD_NISEN_REMOVAL                0x0080
#define SD_NISEN_INSERT                 0x0040
#define SD_NISEN_READ_READY             0x0020
#define SD_NISEN_WRITE_READY            0x0010
#define SD_NISEN_DMA                    0x0008
#define SD_NISEN_BLOCK_GAP              0x0004
#define SD_NISEN_XFR_DONE               0x0002
#define SD_NISEN_CMD_DONE               0x0001

/* SD_EISEN_REG */
#define SD_EISEN_ADMA_ERR               0x0200
#define SD_EISEN_ACMD12_ERR             0x0100
#define SD_EISEN_CURRENT_ERR            0x0080
#define SD_EISEN_DATA_BIT_ERR           0x0040
#define SD_EISEN_DATA_TMOUT_ERR         0x0020
#define SD_EISEN_DATA_CRC_ERR           0x0010
#define SD_EISEN_CMD_IDX_ERR            0x0008
#define SD_EISEN_CMD_CRC_ERR            0x0004
#define SD_EISEN_CMD_BIT_ERR            0x0002
#define SD_EISEN_CMD_TMOUT_ERR          0x0001

/* SD_NIXEN_REG */
#define SD_NIXEN_CARD                   0x0100
#define SD_NIXEN_REMOVAL                0x0080
#define SD_NIXEN_INSERT                 0x0040
#define SD_NIXEN_READ_READY             0x0020
#define SD_NIXEN_WRITE_READY            0x0010
#define SD_NIXEN_DMA                    0x0008
#define SD_NIXEN_BLOCK_GAP              0x0004
#define SD_NIXEN_XFR_DONE               0x0002
#define SD_NIXEN_CMD_DONE               0x0001

/* SD_EIXEN_REG */
#define SD_EISEN_ADMA_ERR               0x0200
#define SD_EIXEN_ACMD12_ERR             0x0100
#define SD_EIXEN_CURRENT_ERR            0x0080
#define SD_EIXEN_DATA_BIT_ERR           0x0040
#define SD_EIXEN_DATA_CRC_ERR           0x0020
#define SD_EIXEN_CMD_CRC_ERR            0x0010
#define SD_EIXEN_DATA_TMOUT_ERR         0x0008
#define SD_EIXEN_CMD_IDX_ERR            0x0004
#define SD_EIXEN_CMD_BIT_ERR            0x0002
#define SD_EIXEN_CMD_TMOUT_ERR          0x0001

/* SD_AC12ES_REG */
#define SD_AC12ES_NOT_ISSUED            0x0040
#define SD_AC12ES_INDEX                 0x0020
#define SD_AC12ES_NOT_EXECED            0x0010
#define SD_AC12ES_END_BIT               0x0004
#define SD_AC12ES_CRC_ERROR             0x0002
#define SD_AC12ES_TMOUT_ERROR           0x0001

/* SD_ADMA_STA_REG */
#define SD_ADMA_STA_ST_STOP             0x00000000
#define SD_ADMA_STA_ST_FDS              0x00000001
#define SD_ADMA_STA_ST_TFR              0x00000003
#define SD_ADMA_STA_LEN_ERR             0x00000004

/* SD_CAP_REG */
#define SD_CAP_INTMODE                  0x08000000
#define SD_CAP_VOL_1_8V                 0x04000000
#define SD_CAP_VOL_3_0V                 0x02000000
#define SD_CAP_VOL_3_3V                 0x01000000
#define SD_CAP_DMA                      0x00800000
#define SD_CAP_SUS_RES                  0x00400000
#define SD_CAP_HIGH_SPEED               0x00200000
//#define SD_CAP_ADMA_SUPPORT           0x00080000
#define SD_CAP_MAX_512B_BLK             0x00000000
#define SD_CAP_MAX_1KB_BLK              0x00010000
#define SD_CAP_MAX_2KB_BLK              0x00020000
#define SD_CAP_BASE_FREQ(x)             (((x) & 0x3f00) >> 8)
#define SD_CAP_TOCLK_KHZ                0x00000000
#define SD_CAP_TOCLK_MHZ                0x00000080
#define SD_CAP_TOCLK_FREQ(x)            (((x) & 0x3f))

/* SD_XC_CTR_REG */
#define SD_XC_CTR_DDR_EN                0x00008000
#define SD_XC_CTR_VOL_1_8V              0x00000001
#define SD_XC_CTR_VOL_3_3V              0x00000000

/* SD_BOOT_CTR_REG */
#define SD_BOOT_CTR_RST_EN              0x00010000

/* SD_BOOT_STA_REG */
#define SD_BOOT_STA_END_ALT             0x01010000
#define SD_BOOT_STA_BOOT_RDY            0x00000001

/* SD_VOL_SW_REG */
#define SD_VOL_SW_CMD_STAT_H            0x00010000
#define SD_VOL_SW_DAT_STAT_H            0x00000007

/* SD_VER_REG */
#define SD_VER_VENDOR(x)                ((x) >> 8)
#define SD_VER_SPEC(x)                  ((x) & 0xf)

#define REG_SDIO_SYSADDRREG             SD_REG(SD_DMA_ADDR_OFFSET) /* read/write */
#define REG_SDIO_BLKREG                 SD_REG(SD_BLK_SZ_OFFSET) /* read/write */
#define REG_SDIO_ARGREG                 SD_REG(SD_ARG_OFFSET) /* read/write */
#define REG_SDIO_TRANMODEREG            SD_REG(SD_XFR_OFFSET) /* read/write */
#define REG_SDIO_RESP0REG               SD_REG(SD_RSP0_OFFSET) /* read */
#define REG_SDIO_RESP1REG               SD_REG(SD_RSP1_OFFSET) /* read */
#define REG_SDIO_RESP2REG               SD_REG(SD_RSP2_OFFSET) /* read */
#define REG_SDIO_RESP3REG               SD_REG(SD_RSP3_OFFSET) /* read */
#define REG_SDIO_BUFFERDATAPORTREG      SD_REG(SD_DATA_OFFSET) /* read/write */
#define REG_SDIO_PRESENTSTATEREG        SD_REG(SD_STA_OFFSET) /* read */
#define REG_SDIO_CONTROL00REG           SD_REG(SD_HOST_OFFSET) /* read/write */
#define REG_SDIO_CONTROL01REG           SD_REG(SD_CLK_OFFSET) /* read/write */
#define REG_SDIO_INTSTATUSREG           SD_REG(SD_NIS_OFFSET) /* read/write */
#define REG_SDIO_INTSTATUSENREG         SD_REG(SD_NISEN_OFFSET) /* read/write */
#define REG_SDIO_INTSIGENREG            SD_REG(SD_NIXEN_OFFSET) /* read/write */
#define REG_SDIO_AUTOCMD12ERRSTATUSREG  SD_REG(SD_AC12ES_OFFSET) /* read/write */
#define REG_SDIO_CAPREG                 SD_REG(SD_CAP_OFFSET) /* read */
#define REG_SDIO_MAXCURCAPREG           SD_REG(SD_CUR_OFFSET) /* read/write */
#define REG_SDIO_SLOTINTSTATUSREG       SD_REG(SD_SIST_OFFSET) /* read */

#endif /* __MACH_GK_SD_H */

