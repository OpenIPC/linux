/*
 * Anyka SPI Nor Flash Controller Driver
 *
 * Copyright (c) 2015-2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/bitops.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/iopoll.h>
#include <linux/module.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/spi-nor.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/reset.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/nand.h>

#include "ak_sfcv2.h"

/*
 * The SDK's kernel adds this old-style OOB mode enum to linux/mtd/mtd.h for
 * this driver alone; kept here so the shared header stays mainline.
 */
typedef enum {
	MTD_OOB_PLACE,
	MTD_OOB_AUTO,
	MTD_OOB_RAW,
} mtd_oob_mode_t;
#include <mach/ak_l2.h>
#include <mach/map.h>

#define SFC_ERR  (1)
#define SFC_INFO (2)
#define SFC_DBG  (3)

static int trace_level = SFC_INFO;

#define sfc_trace(level, msg...) do { \
    if ((level) <= trace_level) { \
        pr_info(msg); \
    } \
} while (0)

#define SFC_COMMAND_BANKS (4)
#define AK_VA_SFC (AK_VA_SUBCTRL + 0x20120000 - AK_PA_SUBCTRL)
#define AK_VA_L2  (AK_VA_SUBCTRL + 0x40000)

#define NOR_OPCODE_WREN             0x06    /* Write Enable */
#define NOR_OPCODE_WRDI             0x04    /* Write Disable */
#define NOR_OPCODE_RDSR1            0x05    /* Read Status Register1 */
#define NOR_OPCODE_RDSR2            0x35    /* Read Status Register2 */
#define NOR_OPCODE_RDSR3            0x15    /* Read Status Register3 */
#define NOR_XMC_OPCODE_RDSR2        0x09    /* XMC Read Status Register2 */
#define NOR_XMC_OPCODE_RDSR3        0x95    /* XMC Read Status Register3 */

#define NOR_OPCODE_WRSR1            0x01    /* Write Status Register */
#define NOR_OPCODE_WRSR2            0x31    /* Write Status2 Register eg:gd25q128c*/
#define NOR_OPCODE_WRSR3            0x11    /* Write Status3 Register eg:gd25q128c*/
#define NOR_XMC_OPCODE_WRSR3        0xC0    /* XMC Write Status3 Reg eg:gd25q128c*/

#define NOR_OPCODE_NORM_READ        0x03    /* Read Data Bytes */
#define NOR_OPCODE_FAST_READ        0x0b    /* Read Data Bytes at Higher Speed */
#define NOR_OPCODE_FAST_D_READ      0x3b    /* Read Data Bytes at Dual output */
#define NOR_OPCODE_FAST_Q_READ      0x6b    /* Read Data Bytes at Quad output */
#define NOR_OPCODE_FAST_D_IO        0xbb    /* Read Data Bytes at Dual i/o */
#define NOR_OPCODE_FAST_Q_IO        0xeb    /* Read Data Bytes at Quad i/o */
#define NOR_OPCODE_DTR_FAST_Q_IO    0xed    /* Read Data Bytes at DTR Quad i/o */

#define NOR_OPCODE_PP               0x02    /* Page Program */
#define NOR_OPCODE_PP_DUAL          0x12    /* Dual Page Program*/
#define NOR_OPCODE_PP_QUAD          0x32    /* Quad Page Program*/
#define NOR_OPCODE_2IO_PP           0x18    /* 2I/O Page Program (tmp)*/
#define NOR_OPCODE_4IO_PP           0x38    /* 4I/O Page Program*/

#define NOR_OPCODE_SE               0x20    /* Sector (4K) Erase */
#define NOR_OPCODE_BE_32K           0x52    /* Block (32K) Erase */
#define NOR_OPCODE_BE_64K           0xd8    /* Block (64K) Erase */
#define NOR_OPCODE_CHIP_ERASE       0xc7    /* Chip Erase */
#define NOR_OPCODE_RDID             0x9f    /* Read JEDEC ID */
#define NOR_OPCODE_DP               0xb9    /* Deep Power-down */
#define NOR_OPCODE_RES              0xab    /* Release from DP,and Read Signature */
#define NOR_OPCODE_CMD_ADDR4        0xB7    /* Enter 4-Bytes Address Mode*/

#define NOR_OPCODE_SFDP             0x5a
/*.
* Used for SST flashes only..
*/
#define NOR_SST_OPCODE_BP               0x02    /* Byte program */
#define NOR_SST_OPCODE_WRDI             0x04    /* Write disable */
#define NOR_SST_OPCODE_AAI_WP           0xad    /* Auto address increment word program*/

#define NOR_OPCODE_IB_LOCK          0x36    /* Individual Block/Sector lock */
#define NOR_OPCODE_IB_UNLOCK        0x39    /* Individual Block/Sector Unlock */

#define NOR_OPCODE_VOLATILE_SR_WRITE_ENABLE 0x50

#define CONFIG_SPIFLASH_USE_FAST_READ

#ifdef CONFIG_SPIFLASH_USE_FAST_READ
#define NOR_OPCODE_READ             NOR_OPCODE_FAST_READ
#define FAST_READ_DUMMY_BYTE    1
#else
#define NOR_OPCODE_READ             NOR_OPCODE_NORM_READ
#define FAST_READ_DUMMY_BYTE    0
#endif

#define NOR_PAGESIZE     (256)
#define NOR_ID_MAX_LEN   (3)
#define NOR_SFDP_MAX_LEN (256)

#define BUS_WIDTH_1WIRE (1<<0)
#define BUS_WIDTH_2WIRE (1<<1)
#define BUS_WIDTH_4WIRE (1<<2)

#define SFLAG_UNDER_PROTECT         (1<<0)
#define SFLAG_FAST_READ             (1<<1)
#define SFLAG_AAAI                  (1<<2)
#define SFLAG_COM_STATUS2           (1<<3)
#define SFLAG_DUAL_IO_READ          (1<<4)
#define SFLAG_DUAL_READ             (1<<5)
#define SFLAG_QUAD_IO_READ          (1<<6)
#define SFLAG_QUAD_READ             (1<<7)
#define SFLAG_DUAL_IO_WRITE         (1<<8)
#define SFLAG_DUAL_WRITE            (1<<9)
#define SFLAG_QUAD_IO_WRITE         (1<<10)
#define SFLAG_QUAD_WRITE            (1<<11)
#define SFLAG_SECT_4K               (1<<12)
#define SFLAG_COM_STATUS3           (1<<13)
#define SFLAG_QUAD_NO_QE            (1<<14)
#define SFLAG_WR_STAT_MODE          (1<<15)
#define SFLAG_ADDR_4B               (1<<16)
#define SFLAG_FREQ_SEL_50M          (1<<17)
#define SFLAG_DTR_QUAD_READ         (1<<18)

#define SFC_TRANS_NO_ERROR    (0)
#define SFC_TRANS_ERROR_ABORT (-1)

#define SFC_MODE_DUAL (1 << 1)
#define SFC_MODE_QUAD (1 << 2)

#define SFC_DIR_READ (0 << 0)
#define SFC_DIR_WRITE (1 << 0)

#define SFC_TRANS_TXD_RXD (1 << 0)
#define SFC_TRANS_FIFO_DMA (1 << 1)
#define SFC_TRANS_FIFO_CPU (1 << 2)

#define SFC_SPI_MODE3 (1 << 0)
#define SFC_SPI_SCLK_FREE_AFTER_CS (1 << 1)

#define SFC_JEDEC_MFR(f) (((f)->jedec_id >> 16) & 0xff)

#define SFC_TIMEOUT (100000)

#define L2_DMA_MAX_SIZE (1 * 1024 * 1024)
//#define L2_DMA_MAX_SIZE (2088960)

#define NOR_MAX_WAIT_READY_JIFFIES  (16 * HZ)   /* 40s max chip erase */

/*
 * For full-chip erase, calibrated to a 2MB flash (M25P16); should be scaled up
 * for larger flash
 */
#define NOR_MAX_ERASE_2MB_READY_WAIT_JIFFIES (40UL * HZ)

#define NOR_MAX_WRITE_TIME (HZ/10)

#define SFC_NOR_MTD_NAME "spi-nor"

enum sfc_nor_ops {
    SFC_NOR_OPS_READ = 0,
    SFC_NOR_OPS_WRITE,
    SFC_NOR_OPS_ERASE,
    SFC_NOR_OPS_LOCK,
    SFC_NOR_OPS_UNLOCK,
};

#define NAND_OPCODE_RESET            0xff
#define NAND_OPCODE_WREN             0x06    /* Write Enable */ 
#define NAND_OPCODE_WRDI             0x04    /* Write Disable */ 

#define NAND_OPCODE_RDSR1            0x0f    /* Read Status Register1 */
#define NAND_OPCODE_RDSR2            0x35    /* Read Status Register2 */ 
#define NAND_OPCODE_RDSR3            0x15    /* Read Status Register3 */ 
#define NAND_OPCODE_WRSR1            0x1f    /* Write Status Register */ 
#define NAND_OPCODE_WRSR2            0x31    /* Write Status2 Register*/ 
#define NAND_OPCODE_WRSR3            0x11    /* Write Status3 Register*/ 

#define NAND_OPCODE_READ_TO_CACHE    0x13
#define NAND_OPCODE_NORM_READ        0x03    /* Read Data Bytes */ 
#define NAND_OPCODE_FAST_READ        0x0b    /* Read Data Bytes at Higher Speed */ 
#define NAND_OPCODE_FAST_D_READ      0x3b    /* Read Data Bytes at Dual output */ 
#define NAND_OPCODE_FAST_Q_READ      0x6b    /* Read Data Bytes at Quad output */ 
#define NAND_OPCODE_FAST_D_IO        0xbb    /* Read Data Bytes at Dual i/o */ 
#define NAND_OPCODE_FAST_Q_IO        0xeb    /* Read Data Bytes at Quad i/o */ 

#define NAND_OPCODE_PP               0x02    /* Page Program */
#define NAND_OPCODE_PP_DUAL          0x12    /* Dual Page Program*/
#define NAND_OPCODE_PP_QUAD          0x32    /* Quad Page Program*/
#define NAND_OPCODE_2IO_PP           0x18    /* 2I/O Page Program (tmp)*/
#define NAND_OPCODE_4IO_PP           0x38    /* 4I/O Page Program*/
#define NAND_OPCODE_PP_EXEC          0x10

#define NAND_OPCODE_BE_4K            0x20    /* Sector (4K) Erase */ 
#define NAND_OPCODE_BE_32K           0x52    /* Block (32K) Erase */
#define NAND_OPCODE_BE_64K           0xd8    /* Block (64K) Erase */ 
#define NAND_OPCODE_SE               0xd8    /* Sector erase (usually 64KiB) */
#define NAND_OPCODE_ERASE_BLOCK      0xd8    /* block Erase */ 
#define NAND_OPCODE_RDID             0x9f    /* Read JEDEC ID */
#define NAND_OPCODE_DP               0xb9    /* Deep Power-down */ 
#define NAND_OPCODE_RES              0xab    /* Release from DP, and Read Signature*/

#ifdef CONFIG_SPINAND_USE_FAST_READ
#define NAND_OPCODE_READ     NAND_OPCODE_FAST_READ
#define NAND_FAST_READ_DUMMY_BYTE 1
#else
#define NAND_OPCODE_READ     NAND_OPCODE_NORM_READ
#define NAND_FAST_READ_DUMMY_BYTE 1
#endif

#define NAND_ID_MAX_LEN   (4)

/* Define max times to check status register before we give up. */
#define NAND_MAX_READY_WAIT_JIFFIES  (40 * HZ)   /* 40s max chip erase */

#define NAND_FLASH_READ      1
#define NAND_FLASH_WRITE     2

#define NAND_FLASH_BUF_SIZE  (priv->info->page_size)

#define NAND_MAX_WRITE_TIME (HZ/50)

#define ALIGN_DOWN(a, b)  (((a) / (b)) * (b))

#define NAND_READ_ECC_ENABLE    (1)
#define NAND_READ_ECC_DISABLE   (0)

#define NAND_OTP_ECC_MASK            0x10

#define NAND_OOB_SIZE  8

#define NAND_OOB_LEN(info)   \
    ((info->oob_up_skiplen + info->oob_seglen + \
    info->oob_down_skiplen)*info->oob_seg_perpage)

#define BBT_BLOCK_GOOD      0x00
#define BBT_BLOCK_WORN      0x01
#define BBT_BLOCK_RESERVED  0x02
#define BBT_BLOCK_FACTORY_BAD   0x03

#define BBT_ENTRY_MASK      0x03
#define BBT_ENTRY_SHIFT     2

#define BADBLOCK_SCAN_MASK (~NAND_BBT_NO_OOB)

#define SFC_NAND_MTD_NAME "spi0.1"

/*
 * feature cmd list ,reference by spec.
 * */
static int nand_feature_cmd[3] = {0xC0, 0xB0, 0xA0};

static uint8_t nand_scan_0xff_pattern[] = { 0xff, 0xff };

struct ak_sfc {
    struct device *dev;
    struct reset_control* rstc;
    void __iomem *regbase;
    struct clk *gclk;
    struct clk *clk;
    u32 clk_rate;
    unsigned int irq;
    struct pinctrl *pctrl;
    struct completion trans_done;
    int trans_errno;
    struct spi_nor *nor;
    struct sfc_nand_priv *nand;
    u32 freq;
    u8 l2buf_id;
};

/*
 * SPI device driver setup and teardown
 */
struct ak_flash_info {
    const char      *name;

    /* JEDEC id zero means "no ID" (most older chips); otherwise it has
     * a high byte of zero plus three data bytes: the manufacturer id,
     * then a two byte device id.
     */
    u32         jedec_id;
    u16         ext_id;
    u8          id_ver_c;

    /*
     * Historily,
     * The size listed here is what works with NOR_OPCODE_SE, which isn't
     * necessarily called a "sector" by the vendor.
     *
     * Now,
     * sector_size  is only used to mutliply
     * with n_sectors to caculate whole size.
     */
    unsigned    sector_size;
    u16         n_sectors;

    /**
     *  chip character bits:
     *  bit 0: under_protect flag, the serial flash
     under protection or not when power on
     *  bit 1: fast read flag, the serial flash support
     fast read or not(command 0Bh)
     *  bit 2: AAI flag, the serial flash support
     auto address increment word programming
     *  bit 3: support dual write or no
     *  bit 4: support dual read or no
     *  bit 5: support quad write or no
     *  bit 6: support quad read or no
     *  bit 7: the second status command (35h) flag,
     if use 4-wire(quad) mode,the bit must be is enable
     */
    u32         flags;

    struct device_node *child;

    // status_reg info
    unsigned b_wip:4;       /*write in progress*/
    unsigned b_wel:4;       /*write ebable latch*/
    unsigned b_bp0:4;       /*block protected 0*/
    unsigned b_bp1:4;       /*block protected 1*/
    unsigned b_bp2:4;       /*block protected 2*/
    unsigned b_bp3:4;       /*block protected 3*/
    unsigned b_bp4:4;       /*block protected 4*/
    unsigned b_srp0:4;      /*status register protect 0*/

    unsigned b_srp1:4;      /*status register protect 1*/
    unsigned b_qe:4;        /*quad enable*/
    /*write protect control and status to the security reg.*/
    unsigned b_lb:4;
    /*
     *   unsigned b_reserved0:4;
     *   unsigned b_reserved1:4;
     *   unsigned b_reserved2:4;
     */
    unsigned b_cmp:4;       /*conjunction bp0-bp4 bit*/
    unsigned b_sus:4;       /*exec an erase/program suspend command*/
    u8  rd_status_cmd[4];
    u8  wr_status_cmd[4];
    u32 wr_status_mode;
    u32 wr_status_flag;
    u8  dtr_dummy;
};

struct ak_nand_flash_info {
    const char      *name;

    /* JEDEC id zero means "no ID" (most older chips); otherwise it has
     * a high byte of zero plus three data bytes: the manufacturer id,
     * then a two byte device id.
     */
    u32         jedec_id;
    u16         ext_id;
    u32         planecnt;
    u32         page_size;
    u32         page_per_block;

    /* The size listed here is what works with OPCODE_SE, which isn't
     * necessarily called a "sector" by the vendor.
     */
    unsigned    block_size;
    u16         n_blocks;

    /*|--------------------64bytes------------------------------|*/
    /*|---12---|-4--|---12----|-4--|---12---|-4--|---12----|-4--|*/
    /*|-seglen-|skip|-segllen-|skip|-seglen-|skip|-segllen-|skip|*/
    u32         oob_size;
    u16         oob_up_skiplen;
    u16         oob_seglen;
    u16         oob_down_skiplen;
    u16         oob_seg_perpage;
    u16         oob_vail_data_offset;

    /**
     *  chip character bits:
     *  bit 0: under_protect flag, the serial flash
     *      under protection or not when power on
     *  bit 1: fast read flag, the serial flash support
     *      fast read or not(command 0Bh)
     *  bit 2: AAI flag, the serial flash support
     *      auto address increment word programming
     *  bit 3: support dual write or no
     *  bit 4: support dual read or no
     *  bit 5: support quad write or no
     *  bit 6: support quad read or no
     *  bit 7: the second status command (35h) flag,
     *      if use 4-wire(quad) mode,the bit must be is enable
     */
    u16         flags;

    /* bad block flag ops */
    u16         badflag_offs;
    u16         badflag_len;
    u32         badflag_option;

    unsigned b_wip:4;       /*write in progress*/
    unsigned b_wel:4;       /*wrute ebabke latch*/
    unsigned b_bp0:4;       /*block protected 0*/
    unsigned b_bp1:4;       /*block protected 1*/
    unsigned b_bp2:4;       /*block protected 2*/
    unsigned b_bp3:4;       /*block protected 3*/
    unsigned b_bp4:4;       /*block protected 4*/
    unsigned b_srp0:4;      /*status register protect 0*/

    unsigned b_srp1:4;      /*status register protect 1*/
    unsigned b_qe:4;        /*quad enable*/
    /*write protect control and status to the security reg.*/
    unsigned b_lb:4;
/*
    unsigned b_reserved0:4;
    unsigned b_reserved1:4;
    unsigned b_reserved2:4;
*/
    unsigned b_cmp:4;       /*conjunction bp0-bp4 bit*/
    unsigned b_sus:4;       /*exec an erase/program suspend cmd_buf*/
    unsigned b_efail:4;     /**/
    unsigned b_pfail:4;     /**/

    struct device_node *child;
};

struct sfc_nor_priv {
    u32 chipselect;
    u32 bus_width;
    u32 readid_freq;
    struct ak_sfc *c;
    struct ak_flash_info *info;
    struct ak_flash_info finfo;
    struct device_node *np;
    u8 erase_opcode_small;
    uint32_t erasesize_small;
    u8 erase_opcode_large;
    uint32_t erasesize_large;
    unsigned int rxd_bus_width;
    unsigned int rxa_bus_width;
    u32 rx_mode;
    unsigned int rx_mode_bytes;
    unsigned int rx_dummy;
    unsigned int txd_bus_width;
    unsigned int txa_bus_width;
};

struct sfc_nand_priv {
    struct mtd_info mtd;
    struct ak_sfc *c;
    struct ak_nand_flash_info *info;
    struct ak_nand_flash_info finfo;
    struct device_node *np;
    struct mutex lock;
    u32 page_shift;
    unsigned char *buf;
    u8  erase_opcode;
    u8  tx_opcode;
    u8  rx_opcode;
    u8  txd_bus_width;
    u8  rxd_bus_width;
    u8  txa_bus_width;
    u8  rxa_bus_width;
    u8  rx_dummy;
    u32 chipselect;
    u32 bus_width;
};

static int sfc_nor_wait_till_ready(struct spi_nor *nor);
static void sfc_reset_hw(struct ak_sfc *c);

static struct spi_nor *ak_spi_nor;
static int g_spiflash_flag = 0;

static struct sfc_nand_priv *ak_nand_priv;
static int HeYangTek_Flag = 0;  //the mata data of HeYangTek's product different form others

#if 0
static void sfc_dump_ipinfo(int level)
{
    sfc_trace(level, "IP Megcell Name                = %x\n",
            __raw_readl(AK_VA_SFC + 0x0000));
    sfc_trace(level, "SFC IP Release Version         = %x\n",
            __raw_readl(AK_VA_SFC + 0x0004));
    sfc_trace(level, "SFC IP Project ID              = %x\n",
            __raw_readl(AK_VA_SFC + 0x0008));
    sfc_trace(level, "SFC IP Feature Information 0   = %x\n",
            __raw_readl(AK_VA_SFC + 0x0020));
    sfc_trace(level, "SFC IP Feature Information 1   = %x\n",
            __raw_readl(AK_VA_SFC + 0x0024));
}
#endif
#if 0
static void sfc_dump_regs(int level)
{
    int i;
    u32 inst, addr, mode, data, match_mask, match_data;
    u32 interface, length, interval;

    sfc_trace(level, "SOFT_REST_CTRL 0x%x PLL2_CLK_CTRL 0x%x"
            " PLL23_BANDWIDTH_CFG 0x%x SARADC_SFC_SDDAC_CLK_CTRL 0x%x"
            " CLK_GATE_CTRL 0x%x\n",
            __raw_readl(AK_VA_SYSCTRL + 0x20),
            __raw_readl(AK_VA_SYSCTRL + 0x8),
            __raw_readl(AK_VA_SYSCTRL + 0xe0),
            __raw_readl(AK_VA_SYSCTRL + 0xc),
            __raw_readl(AK_VA_SYSCTRL + 0x1c));
    sfc_trace(level, "FIQ_EN_CFG 0x%x IRQ_EN_CFG 0x%x TOP_CFG_INT_MASK 0x%x\n",
            __raw_readl(AK_VA_INT_TIMER + 0),
            __raw_readl(AK_VA_INT_TIMER + 4),
            __raw_readl(AK_VA_INT_TIMER + 0x2c));
    sfc_trace(level, "[08000188] 0x%x [080001ac] 0x%x [080001d4] 0x%x\n"
        "[080001c4] 0x%x [08000268] 0x%x [08000278] 0x%x [08000288] 0x%x\n",
        __raw_readl(AK_VA_SYSCTRL + 0x188),
        __raw_readl(AK_VA_SYSCTRL + 0x1ac),
        __raw_readl(AK_VA_SYSCTRL + 0x1d4),
        __raw_readl(AK_VA_SYSCTRL + 0x1c4),
        __raw_readl(AK_VA_SYSCTRL + 0x268),
        __raw_readl(AK_VA_SYSCTRL + 0x278),
        __raw_readl(AK_VA_SYSCTRL + 0x288));

    sfc_trace(level, "SFC_REG_FIFO_STATUS 0x%x\n",
            __raw_readl(AK_VA_SFC + SFC_REG_FIFO_STATUS));
    sfc_trace(level, "SFC_REG_STATUS 0x%x\n",
            __raw_readl(AK_VA_SFC + SFC_REG_STATUS));
    sfc_trace(level, "SFC_REG_INT_MASK 0x%x\n",
            __raw_readl(AK_VA_SFC + SFC_REG_INT_MASK));
    sfc_trace(level, "SFC_REG_CS_TIMING 0x%x\n",
            __raw_readl(AK_VA_SFC + SFC_REG_CS_TIMING));
    sfc_trace(level, "SFC_REG_WAIT_PHASE 0x%x\n",
            __raw_readl(AK_VA_SFC + SFC_REG_WAIT_PHASE));
    sfc_trace(level, "SFC_REG_TRAN_ENABLE 0x%x\n",
            __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE));
    sfc_trace(level, "SFC_REG_DMA_LEN 0x%x\n",
            __raw_readl(AK_VA_SFC + SFC_REG_DMA_LEN));

    for (i = 0; i < SFC_COMMAND_BANKS; i++) {
        inst = __raw_readl(AK_VA_SFC + SFC_REG_INST(i));
        addr = __raw_readl(AK_VA_SFC + SFC_REG_ADDR(i));
        mode = __raw_readl(AK_VA_SFC + SFC_REG_MODE(i));
        data = __raw_readl(AK_VA_SFC + SFC_REG_DATA(i));
        match_mask = __raw_readl(AK_VA_SFC + SFC_REG_MATCH_MASK(i));
        match_data = __raw_readl(AK_VA_SFC + SFC_REG_MATCH_DATA(i));
        interface  = __raw_readl(AK_VA_SFC + SFC_REG_INTERFACE(i));
        length     = __raw_readl(AK_VA_SFC + SFC_REG_LENGTH(i));
        interval   = __raw_readl(AK_VA_SFC + SFC_REG_INTERVAL(i));

        sfc_trace(level, "cmdbank[%d] - inst:0x%x\n", i, inst);
        sfc_trace(level, "cmdbank[%d] - addr:0x%x\n", i, addr);
        sfc_trace(level, "cmdbank[%d] - mode:0x%x\n", i, mode);
        sfc_trace(level, "cmdbank[%d] - data:0x%x\n", i, data);
        sfc_trace(level, "cmdbank[%d] - match_mask:0x%x\n", i, match_mask);
        sfc_trace(level, "cmdbank[%d] - match_data:0x%x\n", i, match_data);
        sfc_trace(level, "cmdbank[%d] - interface:0x%x\n", i, interface);
        sfc_trace(level, "cmdbank[%d] - length:0x%x\n", i, length);
        sfc_trace(level, "cmdbank[%d] - interval:0x%x\n", i, interval);
    }
#if 0
    sfc_trace(level, "---- L2 ----\n");
    sfc_trace(level, "[00] 0x%x [04] 0x%x [08] 0x%x [0c] 0x%x\n",
            __raw_readl(AK_VA_L2 + 0x0),
            __raw_readl(AK_VA_L2 + 0x4),
            __raw_readl(AK_VA_L2 + 0x8),
            __raw_readl(AK_VA_L2 + 0xc));
    printk("[40] 0x%x [44] 0x%x [48] 0x%x [4c] 0x%x\n",
            __raw_readl(AK_VA_L2 + 0x40),
            __raw_readl(AK_VA_L2 + 0x44),
            __raw_readl(AK_VA_L2 + 0x48),
            __raw_readl(AK_VA_L2 + 0x4c));
    printk("[80] 0x%x [84] 0x%x [88] 0x%x [90] 0x%x [94] 0x%x [98] 0x%x [9c] 0x%x [a0] 0x%x [a8]0x%x\n",
            __raw_readl(AK_VA_L2 + 0x80),
            __raw_readl(AK_VA_L2 + 0x84),
            __raw_readl(AK_VA_L2 + 0x88),
            __raw_readl(AK_VA_L2 + 0x90),
            __raw_readl(AK_VA_L2 + 0x94),
            __raw_readl(AK_VA_L2 + 0x98),
            __raw_readl(AK_VA_L2 + 0x9c),
            __raw_readl(AK_VA_L2 + 0xa0),
            __raw_readl(AK_VA_L2 + 0xa8));
#endif
}
#endif
static void sfc_clr_interrupt(unsigned int flag)
{
    u32 status;
    status = __raw_readl(AK_VA_SFC + SFC_REG_STATUS);
    status |= flag;
    __raw_writel(status, AK_VA_SFC + SFC_REG_STATUS);
}

/**
 * @brief sfc fifo config
 * @date 2023-9-27
 */
static void sfc_fifo_thres_cfg(unsigned int rx_thres, unsigned int tx_thres)
{
    unsigned int reg_val;

    reg_val = __raw_readl(AK_VA_SFC + SFC_REG_FIFO_STATUS);
    reg_val &= ~(0xffff);
    reg_val |= (tx_thres & 0xff) | ((rx_thres & 0xff) << 8);
    __raw_writel(reg_val, AK_VA_SFC + SFC_REG_FIFO_STATUS);
}

/**
 * @brief  sfc_fifo_clear
 * @param
 * @return
 * @note   复位sfc控制器的FIFO指针
 */
static void sfc_fifo_clear(void)
{
    int i;
    unsigned int reg_val;
    reg_val = __raw_readl(AK_VA_SFC + SFC_REG_WORK_MODE);
    reg_val |= (0x1 << 4);
    __raw_writel(reg_val ,AK_VA_SFC + SFC_REG_WORK_MODE);
    for (i = 0; i < SFC_TIMEOUT; i++) {
        if (!(__raw_readl(AK_VA_SFC + SFC_REG_WORK_MODE)&(0x1<<4)))
            break;
    }
    if (i >= SFC_TIMEOUT)
        sfc_trace(SFC_ERR, "%s: wait fifo clear timeout!\n", __func__);
}

static int sfc_cmd_addr_dummy_data(unsigned int bank_id, unsigned int dir,
        u32 opcode, unsigned int opcode_bytes, u32 addr,
        unsigned int addr_bytes, unsigned int addr_mode, u32 mode,
        unsigned int mode_bytes, unsigned int dummy_cycles, u8 *buf,
        unsigned int buf_len, unsigned int data_mode, unsigned int trans_mode)
{
    u32 reg_val = 0, cmd_num;

    if ((bank_id >= 4) || (addr_bytes > 4)
            || (mode_bytes > 4) || (opcode_bytes > 4)
            || ((trans_mode & SFC_TRANS_TXD_RXD) && (buf_len > 4))) {
        sfc_trace(SFC_ERR, "%s: invaild argument!\n", __func__);
        return -EINVAL;
    }

    sfc_trace(SFC_DBG, "%s: bank_id %d dir %s opcode(0x%x,%d) addr(0x%x,%d,0x%x)"
            " mode(0x%x,%d) dummy(%d) data(%d,0x%x) trans_mode 0x%x\n",
            __func__, bank_id, (dir & SFC_DIR_WRITE)? "WR": "RD", opcode, opcode_bytes,
            addr, addr_bytes, addr_mode, mode, mode_bytes, dummy_cycles,
            buf_len, data_mode, trans_mode);

    __raw_writel(opcode, AK_VA_SFC + SFC_REG_INST(bank_id));
    __raw_writel(addr, AK_VA_SFC + SFC_REG_ADDR(bank_id));
    __raw_writel(mode, AK_VA_SFC + SFC_REG_MODE(bank_id));
    __raw_writel((dummy_cycles > 0)? ((dummy_cycles - 1)&0xff) : 0,
            AK_VA_SFC + SFC_REG_WAIT_PHASE);

    reg_val = INST_PHASE;
    if (addr_bytes)
        reg_val |= ADDR_PHASE;
    if (mode_bytes)
        reg_val |= MODE_PHASE;
    if (dummy_cycles)
        reg_val |= WAIT_PHASE;
    if (buf_len)
        reg_val |= DATA_PHASE;
    if (addr_mode & SFC_MODE_QUAD) {
        reg_val |= (2 << 2);
    } else if (addr_mode & SFC_MODE_DUAL) {
        reg_val |= (1 << 2);
    }
    if (data_mode & SFC_MODE_QUAD) {
        reg_val |= (2 << 4);
    } else if (data_mode & SFC_MODE_DUAL) {
        reg_val |= (1 << 4);
    }
    if (opcode == NOR_OPCODE_DTR_FAST_Q_IO) {
        reg_val |= ((1 << 12) | (1 << 13));
    }
    if ((dir & SFC_DIR_WRITE) || (buf_len == 0)) {
        reg_val |= WRITE_SPI_FLASH;
    }
    if (trans_mode & SFC_TRANS_TXD_RXD) {
        reg_val |= SPI_TRANS_TXD_RXD;
    }
    __raw_writel(reg_val, AK_VA_SFC + SFC_REG_INTERFACE(bank_id));

    if ((trans_mode & SFC_TRANS_TXD_RXD) && (dir & SFC_DIR_WRITE)
            && buf_len) {
        int i;

        reg_val = 0;
        for (i = buf_len; i > 0; i--) {
            reg_val <<= 8;
            reg_val |= buf[i - 1];
        }
        __raw_writel(reg_val, AK_VA_SFC + SFC_REG_DATA(bank_id)); //data phase
    }

    __raw_writel((
            (((opcode_bytes > 0)? ((opcode_bytes-1)&0x3) : 0) << INST_PHASE_LEN)
          | (((addr_bytes > 0)? ((addr_bytes-1)&0x3) : 0) << ADDR_PHASE_LEN)
          | (((mode_bytes > 0)? ((mode_bytes-1)&0x3) : 0) << MODE_PHASE_LEN)
          | (((buf_len > 0)? ((buf_len-1)&0x3ffffff) : 0) << DATA_PHASE_LEN)),
        AK_VA_SFC + SFC_REG_LENGTH(bank_id));

    if (bank_id == 0) {
        reg_val = __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE);
        reg_val &= ~0xf;
        __raw_writel(reg_val, AK_VA_SFC + SFC_REG_TRAN_ENABLE);
    } else {
        reg_val = __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE);
        cmd_num = reg_val & 0xf;
        if (bank_id > cmd_num) {
            reg_val &= ~0xf;
            reg_val |= (bank_id & 0xf);
        }
        __raw_writel(reg_val, AK_VA_SFC + SFC_REG_TRAN_ENABLE);
    }

    __raw_writel(9, AK_VA_SFC + SFC_REG_INTERVAL(bank_id));

    if (buf_len && (trans_mode & SFC_TRANS_FIFO_DMA)) {
        __raw_writel(buf_len, AK_VA_SFC + SFC_REG_DMA_LEN);
        reg_val = __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE);
        reg_val &= ~(1 << CFG_SFC_DMA_DIR);
        if (dir & SFC_DIR_WRITE)
            reg_val |= DMA_UPLOAD;
        reg_val |= CFG_SFC_DMA_EN;
        __raw_writel(reg_val, AK_VA_SFC + SFC_REG_TRAN_ENABLE);
    } else if (trans_mode & SFC_TRANS_FIFO_CPU) {
        reg_val = __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE);
        reg_val &= ~CFG_SFC_DMA_EN;
        __raw_writel(reg_val, AK_VA_SFC + SFC_REG_TRAN_ENABLE);
    }

    return 0;
}

static void sfc_start_transfer(void)
{
    u32 reg_val;
    //start sfc transfer
    reg_val = __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE);
    reg_val |= CFG_SPI_TRANS_EN;
    __raw_writel(reg_val, AK_VA_SFC + SFC_REG_TRAN_ENABLE);
}

static void sfc_set_spi_baudrate(struct ak_sfc *c, unsigned long rate)
{
    u32 val;
    unsigned long parent_rate, factor;

    parent_rate = clk_get_rate(c->clk) * 2;
    factor = parent_rate / rate;
    if (factor * rate != parent_rate) {
        factor++;
    }

    if (factor <= 1) {
        factor = 2;
    }

    sfc_trace(SFC_DBG,
            "%s: baudrate %ld parent rate %ld, actual %ld\n",
            __func__, rate, parent_rate, parent_rate / factor);

    val = __raw_readl(AK_VA_SFC + SFC_REG_BAUD_RATE);
    val &= ~0x3ff;
    val |= ((factor - 1) & 0x3ff);
    __raw_writel(val, AK_VA_SFC + SFC_REG_BAUD_RATE);
    udelay(1);
}

static unsigned long sfc_get_spi_baudrate(struct ak_sfc *c)
{
    u32 val, factor;
    unsigned long parent_rate;

    parent_rate = clk_get_rate(c->clk) * 2;
    val = __raw_readl(AK_VA_SFC + SFC_REG_BAUD_RATE);
    factor = (val & 0x3ff) + 1;
    if (parent_rate % factor) {
        return (parent_rate / factor) + 1;
    } else {
        return parent_rate / factor;
    }
}

#if 0
static void sfc_set_spi_mode(struct spi_nor *nor,
        unsigned int enable_interval, unsigned int disable_interval,
        unsigned int mode)
{
    u32 val;

    __raw_writel(((enable_interval&0xf) | (disable_interval&0xf)<<4),
            AK_VA_SFC + SFC_REG_CS_TIMING);

    val = __raw_readl(AK_VA_SFC + SFC_REG_BAUD_RATE);
    val &= ~(3 << 30);
    if (mode & SFC_SPI_MODE3) {
        val |= (1 << 31);
        if (mode & SFC_SPI_SCLK_FREE_AFTER_CS)
            val |= (1 << 30);
    }
    __raw_writel(val, AK_VA_SFC + SFC_REG_BAUD_RATE);
}
#endif

static irqreturn_t sfc_irq_handler(int irq, void *dev_id)
{
    struct ak_sfc *c = dev_id;
    u32 status;

    status = __raw_readl(AK_VA_SFC + SFC_REG_STATUS);
    sfc_trace(SFC_DBG, "%s: status 0x%x\n", __func__, status);

    if(status & STA_SFC_TRANS_DONE_INT) {
        if (status & STA_SPI_ABORT_HAPPEN_INT) {
            sfc_clr_interrupt(
                    STA_SPI_ABORT_HAPPEN_INT | STA_SFC_TRANS_DONE_INT);
            c->trans_errno = SFC_TRANS_ERROR_ABORT;
            sfc_trace(SFC_DBG, "%s: spi abort!\n", __func__);
        } else {
            sfc_clr_interrupt(STA_SFC_TRANS_DONE_INT);
            c->trans_errno = SFC_TRANS_NO_ERROR;
        }
        complete(&c->trans_done);
    }

    return IRQ_HANDLED;
}

static int sfc_read_reg(struct ak_sfc *c, u32 opcode, unsigned int opcode_bytes, u8 *buf, int len)
{
    u32 reg_val = 0;

    sfc_trace(SFC_DBG, "%s: opcode (0x%x,%d) len 0x%x\n", __func__, opcode, opcode_bytes, len);

    if (!c || !buf || (len < 0)) {
        sfc_trace(SFC_ERR, "%s: invaild argument!\n", __func__);
        return -EINVAL;
    }
    if (len > 4) {
        sfc_trace(SFC_DBG, "%s: len(%d) no more than 4, use 4\n",
                __func__, len);
        len = 4;
    }

    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_READ, opcode, opcode_bytes, // dir, opcode, opcode_bytes
            0, 0, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            buf, len, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode

    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (!c->trans_errno) {
        reg_val = __raw_readl(AK_VA_SFC + SFC_REG_DATA(0));
        sfc_trace(SFC_DBG, "%s: data 0x%x\n", __func__, reg_val);
        memcpy(buf, &reg_val, len);
        return 0;
    }
    return -EFAULT;
}

static int sfc_nor_read_reg(struct spi_nor *nor, u8 opcode, u8 *buf, int len)
{
    struct sfc_nor_priv *priv;
    priv = nor->priv;
    if (!nor || !(nor->priv)) {
        sfc_trace(SFC_ERR, "%s: invaild argument!\n", __func__);
        return -EINVAL;
    }
    return sfc_read_reg(priv->c, opcode, 1, buf, len);
}

/**
 * @brief write data to the register
 * @author yang jianxiong
 * @date 2023-9-19
 */
static int sfc_write_reg(struct ak_sfc *c, u32 opcode, unsigned int opcode_bytes, u8 *buf, int len)
{
    //sfc_trace(SFC_DBG, "%s: opcode (0x%x,%d) len 0x%x\n", __func__, opcode, opcode_bytes, len);

    if (!c || (len < 0) || ((len > 0) && !buf)) {
        sfc_trace(SFC_ERR, "%s: invaild argument!\n", __func__);
        return -EINVAL;
    }
    if (len > 4) {
        sfc_trace(SFC_DBG, "%s: len(%d) no more than 4, use 4\n",
                __func__, len);
        len = 4;
    }

    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_WRITE, opcode, opcode_bytes, // dir, opcode, opcode_bytes,
            0, 0, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            buf, len, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode
    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (!c->trans_errno) {
        return 0;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

static int sfc_nor_write_reg(struct spi_nor *nor, u8 opcode,
        u8 *buf, int len)
{
    struct sfc_nor_priv *priv;
    priv = nor->priv;
    if (!nor || !(nor->priv)) {
        sfc_trace(SFC_ERR, "%s: invaild argument!\n", __func__);
        return -EINVAL;
    }
    return sfc_write_reg(priv->c, opcode, 1, buf, len);
}

static unsigned int bus_width_to_sfc_mode(int bus_width)
{
    switch (bus_width) {
        case BUS_WIDTH_1WIRE:
            return 0;
        case BUS_WIDTH_2WIRE:
            return SFC_MODE_DUAL;
        case BUS_WIDTH_4WIRE:
            return SFC_MODE_QUAD;
        default:
            return 0;
    }
}

static int sfc_nor_cpu_read(struct spi_nor *nor, loff_t from,
        unsigned char *buf, size_t len, size_t *retlen)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c = priv->c;
    struct ak_flash_info *info = priv->info;
    struct mtd_info *mtd = &nor->mtd;
    u32 val;
    unsigned int i, j;
    unsigned int thres_cnt, frac_thres_cnt;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    unsigned char *p_buf = buf;
    unsigned int addr_size;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n",
            __func__, from, len);

    if (from + len > mtd->size) {
        len = mtd->size - from;
    }

    addr_size = (info->flags & SFLAG_ADDR_4B) ? 4 : 3;
    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_READ, nor->read_opcode, 1, // dir, opcode, opcode_bytes
        from, addr_size, bus_width_to_sfc_mode(priv->rxa_bus_width), // addr, addr_bytes, addr_mode,
        priv->rx_mode, priv->rx_mode_bytes, priv->rx_dummy,// mode, mode_bytes, dummy_cycles,
        buf, len, bus_width_to_sfc_mode(priv->rxd_bus_width), // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_CPU); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    sfc_start_transfer();

    thres_cnt = len / (rx_thres * 4);
    frac_thres_cnt = len % (rx_thres * 4);

    for(i = 0; i < thres_cnt; i++) {
        while(!(__raw_readl(AK_VA_SFC + SFC_REG_STATUS) & (0x1<<3))); //等待FIFO阈值
        for(j = 0; j < rx_thres; j++) {
            val = __raw_readl(AK_VA_SFC + SFC_REG_FIFO_DATA);
            memcpy(p_buf, &val, sizeof(val));
            p_buf += sizeof(val);
        }
    }

    if (frac_thres_cnt) {
        wait_for_completion(&c->trans_done);
        if (!c->trans_errno) {
            for(i = 0; i < frac_thres_cnt / sizeof(val); i++) {
                val = __raw_readl(AK_VA_SFC + SFC_REG_FIFO_DATA);
                memcpy(p_buf, &val, sizeof(val));
                p_buf += sizeof(val);
            }

            val = __raw_readl(AK_VA_SFC+ SFC_REG_FIFO_DATA);
            for(i = 0; i < frac_thres_cnt % 4; i++) {
                *(p_buf) = (val >> (i * 8)) & 0xff;
                p_buf++;
            }
        }
    } else {
        wait_for_completion(&c->trans_done);
    }

    if (retlen)
        *retlen = len;

    if (!c->trans_errno) {
#if 0
        int i;
        for (i = 0; i < (len/8)*8; i+=8) {
            sfc_trace(SFC_DBG, "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                    buf[i], buf[i+1], buf[i+2], buf[i+3],
                    buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
        }
#endif
        return 0;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

static int sfc_nor_dma_read(struct spi_nor *nor, loff_t from,
        unsigned char *buf, size_t len, size_t *retlen)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct mtd_info *mtd = &nor->mtd;
    struct ak_sfc *c= priv->c;
    struct ak_flash_info *info = priv->info;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    dma_addr_t buf_dma;
    unsigned int addr_size;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n",  __func__, from, len);

    if (from + len > mtd->size) {
        len = mtd->size - from;
    }

    if (len >= L2_DMA_MAX_SIZE) {
        len = L2_DMA_MAX_SIZE;
    }
    if (len > 8 * 1024) {
        len = len - len % (64 * 8);
    } else {
        if (len % 64) {
            len = len - len % 64;
        }
    }

    if (len == 0) {
        if (retlen)
            *retlen = len;
        return 0;
    }

    buf_dma = dma_map_single(NULL, buf, len, DMA_FROM_DEVICE);
    if (dma_mapping_error(NULL, buf_dma)) {
        sfc_trace(SFC_DBG, "%s: failed to map buffer\n", __func__);
        return -ENOMEM;
    }

    addr_size = (info->flags & SFLAG_ADDR_4B) ? 4 : 3;
    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_READ, nor->read_opcode, 1, // dir, opcode, opcode_bytes
        from, addr_size, bus_width_to_sfc_mode(priv->rxa_bus_width), // addr, addr_bytes, addr_mode,
        priv->rx_mode, priv->rx_mode_bytes, priv->rx_dummy,// mode, mode_bytes, dummy_cycles,
        buf, len, bus_width_to_sfc_mode(priv->rxd_bus_width), // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_DMA); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    l2_clr_status(c->l2buf_id);
    l2_combuf_dma(buf_dma, c->l2buf_id, len, BUF2MEM, 0);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (!l2_combuf_wait_dma_finish(c->l2buf_id)) {
        sfc_trace(SFC_ERR,
                "%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
        return -EFAULT;
    }

    if (retlen)
        *retlen = len;

    dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
    if (!c->trans_errno) {
#if 0
        int i;
        for (i = 0; i < (len/8)*8; i+=8) {
            sfc_trace(SFC_DBG, "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                    buf[i], buf[i+1], buf[i+2], buf[i+3],
                    buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
        }
#endif
        return 0;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

static int sfc_nor_dma_read_sfdp(struct spi_nor *nor, loff_t from,
        unsigned char *buf, size_t len, size_t *retlen)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c= priv->c;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    dma_addr_t buf_dma;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n",  __func__, from, len);

    buf_dma = dma_map_single(NULL, buf, len, DMA_FROM_DEVICE);
    if (dma_mapping_error(NULL, buf_dma)) {
        sfc_trace(SFC_ERR, "%s: failed to map buffer\n", __func__);
        return -ENOMEM;
    }

    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_READ, NOR_OPCODE_SFDP, 1, // dir, opcode, opcode_bytes
        from, 3, 0, // addr, addr_bytes, addr_mode,
        0, 0, 8,// mode, mode_bytes, dummy_cycles,
        buf, len, 0, // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_DMA); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    l2_clr_status(c->l2buf_id);
    l2_combuf_dma(buf_dma, c->l2buf_id, len, BUF2MEM, 0);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (!l2_combuf_wait_dma_finish(c->l2buf_id)) {
        sfc_trace(SFC_ERR,
                "%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
        return -EFAULT;
    }

    if (retlen)
        *retlen = len;

    dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
    if (!c->trans_errno) {
#if 0
        int i;
        for (i = 0; i < (len/8)*8; i+=8) {
            sfc_trace(SFC_INFO, "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                    buf[i], buf[i+1], buf[i+2], buf[i+3],
                    buf[i+4], buf[i+5], buf[i+6], buf[i+7]);
        }
#endif
        return 0;
    }
    sfc_trace(SFC_INFO, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

static int sfc_nor_cpu_write(struct spi_nor *nor, loff_t from,
        unsigned char *buf, size_t len, size_t *retlen)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c = priv->c;
    struct ak_flash_info *info = priv->info;
    //struct mtd_info *mtd = &nor->mtd;
    u32 val;
    unsigned int i, j;
    unsigned int thres_cnt;
    unsigned int frac_thres_cnt;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    unsigned char *p_buf = buf;
    unsigned int addr_size;
    int ret;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n",
            __func__, from, len);

    len = min_t(unsigned int, NOR_PAGESIZE - from % NOR_PAGESIZE, len);

    if (len == 0) {
        if (retlen)
            *retlen = len;
        return 0;
    }

    addr_size = (info->flags & SFLAG_ADDR_4B) ? 4 : 3;

    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_WRITE, NOR_OPCODE_WREN, 1, // dir, opcode, opcode_bytes
        0, 0, 0, // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        NULL, 0, 0, // buf, buf_len, data_mode,
        SFC_TRANS_TXD_RXD); // trans_mode
    sfc_cmd_addr_dummy_data(
        1, // bank_id,
        SFC_DIR_WRITE, nor->program_opcode, 1, // dir, opcode, opcode_bytes
        from, addr_size, bus_width_to_sfc_mode(priv->txa_bus_width), // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        buf, len, bus_width_to_sfc_mode(priv->txd_bus_width), // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_CPU); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    sfc_start_transfer();

    thres_cnt = len / ((SFC_TXFIFO_SIZE - tx_thres) * 4);
    frac_thres_cnt = len % ((SFC_TXFIFO_SIZE - tx_thres) * 4);
    for (i = 0; i < thres_cnt; i++) {
        while(!(__raw_readl(AK_VA_SFC + SFC_REG_STATUS) & (0x1<<2))); //等待FIFO阈值
        for (j = 0; j < SFC_TXFIFO_SIZE - tx_thres; j++) {
            val = (p_buf[0] | p_buf[1]<<8 | p_buf[2]<<16 | p_buf[3]<<24);
            __raw_writel(val, AK_VA_SFC + SFC_REG_FIFO_DATA);
            p_buf += 4;
        }
    }

    if (frac_thres_cnt) {
        while(!(__raw_readl(AK_VA_SFC + SFC_REG_STATUS) & (0x1<<2))); //等待FIFO阈值
        for(i = 0; i < frac_thres_cnt / 4; i++) {
            val = (p_buf[0] | p_buf[1]<<8 | p_buf[2]<<16 | p_buf[3]<<24);
            __raw_writel(val, AK_VA_SFC + SFC_REG_FIFO_DATA);
            p_buf += 4;
        }
        val = 0;
        for(i = 0; i < frac_thres_cnt % 4; i++) {
            val |= *(p_buf) << (i * 8);
            p_buf++;
        }
        if (frac_thres_cnt % 4 != 0)
            __raw_writel(val, AK_VA_SFC + SFC_REG_FIFO_DATA);
    }

    ret = wait_for_completion_timeout(&c->trans_done, NOR_MAX_WRITE_TIME);
    if (ret == 0) {
        sfc_trace(SFC_INFO, "%s: from 0x%llx len %d timeout!"
                " SFC_REG_STATUS 0x%x\n SFC_REG_TRAN_ENABLE 0x%x",
                __func__, from, len,
                __raw_readl(AK_VA_SFC + SFC_REG_STATUS),
                __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE));

        sfc_reset_hw(c);
        *retlen = 0;
        return 0;
    }

    if (retlen)
        *retlen = len;

    if (!c->trans_errno) {
        return 0;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

static int sfc_nor_dma_write(struct spi_nor *nor, loff_t from,
        unsigned char *buf, size_t len, size_t *retlen)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c= priv->c;
    struct ak_flash_info *info = priv->info;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    dma_addr_t buf_dma;
    unsigned int addr_size;
    int ret;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n",
            __func__, from, len);

    if (len % 64) {
        len = len - len % 64;
    }

    len = min_t(unsigned int, NOR_PAGESIZE - from % NOR_PAGESIZE, len);

    if (len == 0) {
        if (retlen)
            *retlen = len;
        return 0;
    }

    buf_dma = dma_map_single(NULL, buf, len, DMA_TO_DEVICE);
    if (dma_mapping_error(NULL, buf_dma)) {
        sfc_trace(SFC_DBG, "%s: failed to map buffer\n", __func__);
        return -ENOMEM;
    }

    addr_size = (info->flags & SFLAG_ADDR_4B) ? 4 : 3;
    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_WRITE, NOR_OPCODE_WREN, 1, // dir, opcode, opcode_bytes
            0, 0, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            NULL, 0, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode
    sfc_cmd_addr_dummy_data(
        1, // bank_id,
        SFC_DIR_WRITE, nor->program_opcode, 1, // dir, opcode, opcode_bytes
        from, addr_size, bus_width_to_sfc_mode(priv->txa_bus_width), // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        buf, len, bus_width_to_sfc_mode(priv->txd_bus_width), // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_DMA); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    l2_clr_status(c->l2buf_id);
    l2_combuf_dma(buf_dma, c->l2buf_id, len, MEM2BUF, 0);
    sfc_start_transfer();
    ret = wait_for_completion_timeout(&c->trans_done, NOR_MAX_WRITE_TIME);
    if (!l2_combuf_wait_dma_finish(c->l2buf_id)) {
        sfc_trace(SFC_ERR,
                "%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        dma_unmap_single(NULL, buf_dma, len, DMA_TO_DEVICE);
        return -EFAULT;
    }

    if (ret == 0) {
        sfc_trace(SFC_INFO, "%s: from 0x%llx len %d timeout!"
                " SFC_REG_STATUS 0x%x SFC_REG_TRAN_ENABLE 0x%x\n",
                __func__, from, len,
                __raw_readl(AK_VA_SFC + SFC_REG_STATUS),
                __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE));

        sfc_reset_hw(c);
        *retlen = 0;
        return 0;
    }

    if (retlen)
        *retlen = len;

    dma_unmap_single(NULL, buf_dma, len, DMA_TO_DEVICE);
    if (!c->trans_errno) {
        return 0;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

static int sfc_alloc_l2buf(struct ak_sfc *c)
{
    if (c->l2buf_id == BUF_NULL) {
        c->l2buf_id = l2_alloc(ADDR_SFC);
        if (c->l2buf_id == BUF_NULL) {
            sfc_trace(SFC_ERR, "%s: l2_alloc failed!\n", __func__);
            return -EBUSY;
        }
        sfc_trace(SFC_DBG, "%s: l2buf_id %d\n", __func__, c->l2buf_id);
    }
    return 0;
}

static void sfc_free_l2buf(struct ak_sfc *c)
{
    if (c->l2buf_id != BUF_NULL) {
        l2_free(ADDR_SFC);
        c->l2buf_id = BUF_NULL;
    }
}

static int sfc_nor_lock_and_prep(struct spi_nor *nor, enum sfc_nor_ops ops)
{
    int ret = 0;

    mutex_lock(&nor->lock);

    if (nor->prepare) {
        ret = nor->prepare(nor, ops);
        if (ret) {
            sfc_trace(SFC_ERR, "%s: failed prepare for %d\n", __func__, ops);
            mutex_unlock(&nor->lock);
            return ret;
        }
    }
    return ret;
}

static void sfc_nor_unlock_and_unprep(struct spi_nor *nor, enum spi_nor_ops ops)
{
    if (nor->unprepare)
        nor->unprepare(nor, ops);
    mutex_unlock(&nor->lock);
}

static inline struct spi_nor *mtd_to_spi_nor(struct mtd_info *mtd)
{
    return mtd->priv;
}

/**
 * @brief read data from the SPI NOR
 * @param[in] nor   struct spi_nor *
 * @param[in] from  offset address in flash
 * @param[in] len   data length
 * @param[in] retlen    return length read actually
 * @param[in] read_buf  read data buffer
 * @param[out] int  success:0
 * @author  yang jianxiong
 * @date 2023-9-21
 */
static int sfc_nor_read(struct mtd_info *mtd, loff_t from, size_t len,
            size_t *retlen, u_char *buf)
{
    struct spi_nor *nor = mtd_to_spi_nor(mtd);
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c= priv->c;
    size_t offset, len1;
    int ret = 0, i;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n", __func__, from, len);

    ret = sfc_nor_lock_and_prep(nor, SPI_NOR_OPS_READ);
    if (ret)
        return ret;

    offset = 0;

    if ((unsigned long) buf >= VMALLOC_START) {
        goto _l_cpu_read;
    }

    for (i = 0; i < len / L2_DMA_MAX_SIZE; i++) {
        if (sfc_alloc_l2buf(c) || (sfc_nor_dma_read(
                nor, from + offset, buf + offset, L2_DMA_MAX_SIZE, NULL))) {
            sfc_trace(SFC_ERR,
                    "%s: sfc_nor_dma_read failed\n", __func__);
            goto err_dma_read;
        }
        offset += L2_DMA_MAX_SIZE;
    }

    if (offset < len) {
        len1 = len - offset;
        len1 = len1 - len1 % 512;
        if (len1) {
            if (sfc_alloc_l2buf(c) || (sfc_nor_dma_read(
                    nor, from + offset, buf + offset, len1, NULL))) {
                sfc_trace(SFC_ERR,
                        "%s: sfc_nor_dma_read failed\n", __func__);
                goto err_dma_read;
            }
            offset += len1;
        }
    }

    if (offset < len) {
        len1 = len - offset;
        len1 = len1 - len1 % 64;
        if (len1) {
            if (sfc_alloc_l2buf(c) || (sfc_nor_dma_read(
                    nor, from + offset, buf + offset, len1, NULL))) {
                sfc_trace(SFC_ERR,
                        "%s: sfc_nor_dma_read failed\n", __func__);
                goto err_dma_read;
            }
            offset += len1;
        }
    }
_l_cpu_read:
    if (offset < len) {
        len1 = len - offset;
        if (len1) {
            ret = sfc_nor_cpu_read(
                    nor, from + offset, buf + offset, len1, NULL);
            if (ret) {
                sfc_trace(SFC_ERR,
                        "%s: sfc_nor_cpu_read failed with %d\n", __func__, ret);
                /* Through the unlock below: returning here kept the lock. */
                goto err_dma_read;
            }
            offset += len1;
        }
    }

    if (retlen)
        *retlen = len;

    sfc_free_l2buf(c);
    sfc_nor_unlock_and_unprep(nor, SPI_NOR_OPS_READ);

    return 0;

err_dma_read:
    if (retlen)
        *retlen = offset;
    sfc_free_l2buf(c);
    sfc_nor_unlock_and_unprep(nor, SPI_NOR_OPS_READ);
    return -EFAULT;
}

static int sfc_nor_prep(struct spi_nor *nor, enum spi_nor_ops ops)
{
    return 0;
}

static void sfc_nor_unprep(struct spi_nor *nor, enum spi_nor_ops ops)
{
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int _nor_write_enable(struct spi_nor *nor)
{
    return nor->write_reg(nor, NOR_OPCODE_WREN, NULL, 0);
}

/*
 * Set write enable latch with Write Enable command.
 * Returns negative if error occurred.
 */
static inline int nor_write_enable(struct spi_nor *nor)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    u8 opcode, sr;
    int ret;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    _nor_write_enable(nor);
    opcode = info->rd_status_cmd[0];
    ret = nor->read_reg(nor, opcode, &sr, 1);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d read status reg 0x%x!\n",
                __func__, ret, opcode);
        return ret;
    }
    if (!(sr & (1 << info->b_wel))) {
        sfc_trace(SFC_ERR, "%s: WEL is not set after write enable!\n",
                __func__);
        return -EIO;
    }
    return 0;
}

/*
 * Send write disble instruction to the chip.
 */
static inline int nor_write_disable(struct spi_nor *nor)
{
    return nor->write_reg(nor, NOR_OPCODE_WRDI, NULL, 0);
}

/**
 * @brief spi write data to norflash
 * @param[in] nor           struct spi_nor *
 * @param[in] to            offset address in flash
 * @param[in] len           data length
 * @param[in] write_buf     data buffer address
 * @param[out] ssize_t      data length actually wrriten
 * @author  yang jianxiong
 * @date    2023-9-19
 */
static int _sfc_nor_write(struct spi_nor *nor, loff_t to,
        size_t len, size_t *retlen, u_char *buf)
{
    size_t offset, len1, actual, total = 0;
    int ret;

    // len must < NOR_PAGESIZE
    if (len > (NOR_PAGESIZE - to % NOR_PAGESIZE)) {
        sfc_trace(SFC_ERR,
                "%s: to %lld len %d invalid length!\n", __func__, to, len);
        return -EINVAL;
    }

    offset = 0;
    if ((unsigned long) buf >= VMALLOC_START) {
        goto _l_cpu_write;
    }

    len1 = len - len % 64;
    if (len1) {
        if (sfc_nor_dma_write(nor, to + offset, buf + offset, len1, &actual)) {
            sfc_trace(SFC_ERR,
                    "%s: sfc_nor_dma_write failed\n", __func__);
            return -EFAULT;
        }
        offset += len1;
        total += actual;

        ret = sfc_nor_wait_till_ready(nor);
        if (ret < 0) {
            sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                    __func__, ret);
            return ret;
        }
    }
_l_cpu_write:
    len1 = len - offset;
    if (len1) {
        ret = sfc_nor_cpu_write(
                nor, to + offset, buf + offset, len1, &actual);
        if (ret) {
            sfc_trace(SFC_ERR,
                    "%s: sfc_nor_cpu_write failed with %d\n", __func__, ret);
            return -EFAULT;
        }
        offset += len1;
        total += actual;
    }

    if (retlen)
        *retlen = total;

    return 0;
}

#if 0
/**
 * @brief  Wait for SPI flash ready.
 *
 *  Service routine to read status register until ready, or timeout occurs.
 * @author SheShaohua
 * @date 2012-03-20
 * @param[in] flash  spiflash handle.
 * @return int return write success or failed
 * @retval returns zero on success
 * @retval return a non-zero error code if failed
 */
static int sfc_nor_wait_till_ready(struct spi_nor *nor)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    unsigned long deadline;
    u8 opcode;
    u8 tmp;
    int ret;

    if (info->b_wip >= 8) {
        sfc_trace(SFC_ERR, "%s: b_wip %d >=8 is not supported!\n", __func__);
        return -EINVAL;
    }

    deadline = jiffies + MAX_READY_WAIT_JIFFIES;
    do {
        opcode = info->rd_status_cmd[0];
        ret = nor->read_reg(nor, opcode, &tmp, 1);
        if (ret < 0) {
            sfc_trace(SFC_ERR, "%s: error %d read status reg 0x%x!\n",
                    __func__, ret, opcode);
            return ret;
        }

        if (!(tmp & (1 << info->b_wip)))
            return 0;

        cond_resched();
    } while (!time_after_eq(jiffies, deadline));

    return -EFAULT;
}
#else
/**
 * @brief  Wait for SPI flash ready.
 *
 *  Service routine to read status register until ready, or timeout occurs.
 * @author SheShaohua
 * @date 2012-03-20
 * @param[in] flash  spiflash handle.
 * @return int return write success or failed
 * @retval returns zero on success
 * @retval return a non-zero error code if failed
 */
static int sfc_nor_wait_till_ready_timeout(struct spi_nor *nor,
        unsigned long timeout_jiffies)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c = priv->c;
    struct ak_flash_info *info = priv->info;
    u8 opcode, sr;
    u32 val;
    int ret;

    if (!nor) {
        sfc_trace(SFC_ERR, "%s: invaild argument!\n", __func__);
        return -EINVAL;
    }

    if (info->b_wip >= 8) {
        sfc_trace(SFC_ERR, "%s: b_wip %d >=8 is not supported!\n",
                __func__, info->b_wip);
        return -EINVAL;
    }

    opcode = info->rd_status_cmd[0];
    ret = nor->read_reg(nor, opcode, &sr, 1);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d read status reg 0x%x!\n",
                __func__, ret, opcode);
        return ret;
    }
    if ((sr & (1 << info->b_wip)) == 0)
        return 0;

    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_READ, opcode, 1, // dir, opcode, opcode_bytes
            0, 0, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            NULL, 1, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode
    val = __raw_readl(AK_VA_SFC + SFC_REG_INTERFACE(0));
    val |= (1 << CFG_SPI_CMD_TYPE); // polling cmd
    val &= ~(1 << CFG_POLLING_MATCH_MODE);
    __raw_writel(val, AK_VA_SFC + SFC_REG_INTERFACE(0));
    __raw_writel(~(1 << info->b_wip), AK_VA_SFC + SFC_REG_MATCH_MASK(0));
    __raw_writel(0, AK_VA_SFC + SFC_REG_MATCH_DATA(0));
    __raw_writel(0, AK_VA_SFC + SFC_REG_INTERVAL(0));

    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    ret = wait_for_completion_timeout(&c->trans_done, timeout_jiffies);

    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
    }
    if (ret > 0) {
        return c->trans_errno;
    } else {
        sfc_trace(SFC_DBG, "%s: transfer timeout\n", __func__);
        return -ETIMEDOUT;
    }
}

static int sfc_nor_wait_till_ready(struct spi_nor *nor)
{
    return sfc_nor_wait_till_ready_timeout(nor, NOR_MAX_WAIT_READY_JIFFIES);
}

#endif

/*
 * Write an address range to the nor chip.  Data must be written in
 * FLASH_PAGESIZE chunks.  The address range may be any size provided
 * it is within the physical boundaries.
 */
static int sfc_nor_write(struct mtd_info *mtd, loff_t to, size_t len,
        size_t *retlen, const u_char *buf)
{
    struct spi_nor *nor = mtd_to_spi_nor(mtd);
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c= priv->c;
    u32 page_offset, page_size, i;
    int ret;
    size_t tmp, total = 0;

    sfc_trace(SFC_DBG, "%s: to 0x%llx, len %d\n", __func__, to, len);

    if (to + len > mtd->size) {
        len = mtd->size - to;
    }

    if (retlen)
        *retlen = 0;

    ret = sfc_nor_lock_and_prep(nor, SFC_NOR_OPS_WRITE);
    if (ret)
        return ret;

    if (sfc_alloc_l2buf(c)) {
        ret = -ENOMEM;
        goto write_err;
    }

    page_offset = to & (nor->page_size - 1);

    /*
     * Every page write is checked, and only what it reports written is
     * counted: the vendor loop dropped the errors and added a length the
     * callee never set when it failed, so a failed write came back as done.
     */
    /* do all the bytes fit onto one page? */
    if (page_offset + len <= nor->page_size) {
        tmp = 0;
        ret = _sfc_nor_write(nor, to, len, &tmp, (u_char*)buf);
        if (ret)
            goto write_err;
        total += tmp;
    } else {
        /* the size of data remaining on the first page */
        page_size = nor->page_size - page_offset;
        tmp = 0;
        ret = _sfc_nor_write(nor, to, page_size, &tmp, (u_char*)buf);
        if (ret)
            goto write_err;
        total += tmp;

        /* write everything in nor->page_size chunks */
        for (i = page_size; i < len; i += page_size) {
            page_size = len - i;
            if (page_size > nor->page_size)
                page_size = nor->page_size;

            ret = sfc_nor_wait_till_ready(nor);
            if (ret)
                goto write_err;

            tmp = 0;
            ret = _sfc_nor_write(nor, to + i, page_size, &tmp, (u_char*)(buf + i));
            if (ret)
                goto write_err;
            total += tmp;
        }
    }

    ret = sfc_nor_wait_till_ready(nor);
write_err:
    sfc_free_l2buf(c);
    sfc_nor_unlock_and_unprep(nor, SPI_NOR_OPS_WRITE);
    if (retlen)
        *retlen = total;
    return ret;
}

static int sfc_nor_read_sr(struct spi_nor *nor, u32 *val)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    u8 opcode;
    u32 status;
    u8 st_tmp= 0;
    int ret;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    /*
     * NOR_OPCODE_RDSR1:the first read status cmd
     */
    opcode = info->rd_status_cmd[0];
    ret = nor->read_reg(nor, opcode, &st_tmp, 1);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d read status reg 0x%x!\n",
                __func__, ret, opcode);
        return ret;
    }
    status = st_tmp;

    if (info->flags & SFLAG_COM_STATUS2) {
        opcode = info->rd_status_cmd[1];
        ret = nor->read_reg(nor, opcode, &st_tmp, 1);
        if (ret < 0) {
            sfc_trace(SFC_ERR, "%s: error %d read status reg 0x%x!\n",
                    __func__, ret, opcode);
            return ret;
        }
        status = (status | (st_tmp << 8));
    }

    if (info->flags & SFLAG_COM_STATUS3) {
        opcode = info->rd_status_cmd[2];
        ret = nor->read_reg(nor, opcode, &st_tmp, 1);
        if (ret < 0) {
            sfc_trace(SFC_ERR, "%s: error %d read status reg 0x%x!\n",
                    __func__, ret, opcode);
            return ret;
        }
        status = (status | (st_tmp << 16));
    }

    *val = status;
    return 0;
}

static int sfc_nor_write_sr(struct spi_nor *nor, u32 val)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    int wr_cnt;
    int ret;
    u8 opcode;
    u8 buf[3];

    sfc_trace(SFC_DBG, "%s: val 0x%x\n", __func__, val);

    ret = sfc_nor_wait_till_ready(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                __func__, ret);
        return ret;
    }
    // write volatile status reg
    if ((info->wr_status_mode & 0x20)) {
        ret = nor->write_reg(nor,
                NOR_OPCODE_VOLATILE_SR_WRITE_ENABLE, NULL, 0);
    } else {
        ret = nor_write_enable(nor);
    }
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d write enable!\n",
                __func__, ret);
        return ret;
    }

    // one write status cmd can wirte all status regs?
    if ((info->wr_status_mode & 0xf)== 0) {
        opcode = info->wr_status_cmd[0];//NOR_OPCODE_WRSR1;
        buf[0] = val & 0xff;
        buf[1] = (val>>8) & 0xff;
        buf[2] = (val>>16) & 0xff;

        if (info->flags & SFLAG_COM_STATUS3) {
            wr_cnt = 3;
        } else if (info->flags & SFLAG_COM_STATUS2) {
            wr_cnt = 2;
        } else {
            wr_cnt = 1;
        }

        ret = nor->write_reg(nor, opcode, buf, wr_cnt);
        if (ret < 0) {
            sfc_trace(SFC_ERR, "%s: error %d write status reg 0x%x!\n",
                    __func__, ret, opcode);
            nor_write_disable(nor);
            return ret;
        }

        // write status reg then wait
        if (info->wr_status_mode & 0x10) {
            ret = sfc_nor_wait_till_ready(nor);
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                        __func__, ret);
                nor_write_disable(nor);
                return ret;
            }
        }

        return ret;
    } else {
        //NOR_OPCODE_WRSR1;
        opcode = info->wr_status_cmd[0];//NOR_OPCODE_WRSR1;
        buf[0] = val & 0xff;
        ret = nor->write_reg(nor, opcode, buf, 1);
        if (ret < 0) {
            sfc_trace(SFC_ERR, "%s: error %d write status reg 0x%x!\n",
                    __func__, ret, opcode);
            nor_write_disable(nor);
            return ret;
        }

        if (info->wr_status_flag & 0x2) {
            ret = sfc_nor_wait_till_ready(nor);
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                        __func__, ret);
                return ret;
            }
            // write volatile status reg
            if ((info->wr_status_mode & 0x20)) {
                ret = nor->write_reg(nor,
                        NOR_OPCODE_VOLATILE_SR_WRITE_ENABLE, NULL, 0);
            } else {
                ret = nor_write_enable(nor);
            }
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d write enable!\n",
                        __func__, ret);
                return ret;
            }
            //NOR_OPCODE_WRSR2;
            opcode = info->wr_status_cmd[1];
            buf[0] = (val>>8) & 0xff;
            ret = nor->write_reg(nor, opcode, buf, 1);
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d write status reg 0x%x!\n",
                        __func__, ret, opcode);
                nor_write_disable(nor);
                return ret;
            }
        }

        if (info->wr_status_flag & 0x4) {
            ret = sfc_nor_wait_till_ready(nor);
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                        __func__, ret);
                return ret;
            }
            // write volatile status reg
            if ((info->wr_status_mode & 0x20)) {
                ret = nor->write_reg(nor,
                        NOR_OPCODE_VOLATILE_SR_WRITE_ENABLE, NULL, 0);
            } else {
                ret = nor_write_enable(nor);
            }
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d write enable!\n",
                        __func__, ret);
                return ret;
            }
            //NOR_OPCODE_WRSR3
            opcode = info->wr_status_cmd[2];
            buf[0] = (val>>16) & 0xff;
            ret = nor->write_reg(nor, opcode, buf, 1);
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d write status reg 0x%x!\n",
                        __func__, ret, opcode);
                nor_write_disable(nor);
                return ret;
            }
        }
        // write status reg then wait
        if (info->wr_status_mode & 0x10) {
            ret = sfc_nor_wait_till_ready(nor);
            if (ret < 0) {
                sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                        __func__, ret);
                nor_write_disable(nor);
                return ret;
            }
        }
        return ret;
    }
}

/**
 * @brief  enable spi norflash quad 4 wires mode.
 *
 * enable spi norflash quad 4 wires mode can set  
 * spi norflash spi_wp and spi_hold share pin as data io
 * @author SheShaohua
 * @date 2012-03-20
 * @param[in] flash  spiflash handle.
 * @return int return write success or failed
 * @retval returns zero on success
 * @retval return a non-zero error code if failed
 */
static int sfc_nor_quad_mode_enable(struct spi_nor *nor)
{
    int ret;
    u32 status;
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    ret = sfc_nor_wait_till_ready(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                __func__, ret);
        return ret;
    }
    ret = sfc_nor_read_sr(nor, &status);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_read_sr!\n",
                __func__, ret);
        return ret;
    }
    if (status & (1<<info->b_qe)) {
        return 0;
    }

    status |= 1<<info->b_qe;
    ret = sfc_nor_write_sr(nor, status);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_write_sr!\n",
                __func__, ret);
        return ret;
    }

    ret = sfc_nor_wait_till_ready(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                __func__, ret);
        return ret;
    }

    ret = nor_write_disable(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d nor_write_disable!\n",
                __func__, ret);
        return ret;
    }

    return 0;
}

static int sfc_nor_addr4_mode_enable(struct spi_nor *nor)
{
    int ret;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    ret = sfc_nor_wait_till_ready(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                __func__, ret);
        return ret;
    }
    ret = nor_write_enable(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d nor_write_enable!\n",
                __func__, ret);
        return ret;
    }

    ret = nor->write_reg(nor, NOR_OPCODE_CMD_ADDR4, NULL, 0);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d opcode 0x%x!\n",
                __func__, ret, NOR_OPCODE_CMD_ADDR4);
        return ret;
    }

    ret = nor_write_disable(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d nor_write_disable!\n",
                __func__, ret);
        return ret;
    }

    return 0;
}

static struct ak_flash_info *sfc_nor_read_id(struct spi_nor *nor)
{
    int tmp;
    u8 id[NOR_ID_MAX_LEN];
    u32 readid = 0;
    u32 ext_readid = 0;
    u32 jedec;
    u16 ext_jedec = 0;
    u8 __attribute__((aligned(32))) sfdp[NOR_SFDP_MAX_LEN];
    struct device_node *child;
    struct ak_flash_info *info;
    u32 val;
    struct sfc_nor_priv *priv = nor->priv;

    tmp = nor->read_reg(nor, NOR_OPCODE_RDID, id, NOR_ID_MAX_LEN);
    if (tmp < 0) {
        sfc_trace(SFC_ERR, "%s: error %d reading JEDEC ID\n", __func__, tmp);
        return NULL;
    }

    jedec = id[0];
    jedec = jedec << 8;
    jedec |= id[1];
    jedec = jedec << 8;
    jedec |= id[2];
    sfc_trace(SFC_INFO, "%s: jedec 0x%x\n", __func__, jedec);

    priv->info = &priv->finfo;
    info = priv->info;

    if(jedec == 0x204017) {
        if(sfc_alloc_l2buf(priv->c) || sfc_nor_dma_read_sfdp(nor, 0, sfdp, NOR_SFDP_MAX_LEN, NULL)) {
            sfc_trace(SFC_ERR,"%s: sfc_nor_dma_read_sfdp failed\n", __func__);
            sfc_free_l2buf(priv->c);
            return NULL;
        }
        sfc_free_l2buf(priv->c);

        if(sfdp[50] & (1 << 3)) {
            ext_jedec = 0x6444;
            sfc_trace(SFC_DBG,"The flash DTR support,ext_jedec:%x\n",ext_jedec);
        } else {
            ext_jedec = 0x6443;
            sfc_trace(SFC_DBG,"The flash DTR not support,ext_jedec:%x\n",ext_jedec);
        }
    } else if(jedec == 0x204018) {
        if(sfc_alloc_l2buf(priv->c) || sfc_nor_dma_read_sfdp(nor, 0, sfdp, NOR_SFDP_MAX_LEN, NULL)) {
            sfc_trace(SFC_ERR,"%s: sfc_nor_dma_read_sfdp failed\n", __func__);
            sfc_free_l2buf(priv->c);
            return NULL;
        }
        sfc_free_l2buf(priv->c);

        if(sfdp[50] & (1 << 3)) {
            ext_jedec = 0x2844;
            sfc_trace(SFC_DBG,"The flash DTR support,ext_jedec:%x\n",ext_jedec);
        } else {
            ext_jedec = 0x2843;
            sfc_trace(SFC_DBG,"The flash DTR not support,ext_jedec:%x\n",ext_jedec);
        }
    }

    for_each_available_child_of_node(priv->np, child) {
        //sfc_trace(SFC_DBG, "%s: name %s type %s full_name %s\n",
        //        __func__, child->name, child->type, child->full_name);
        if (child->name && (of_node_cmp(child->name, "spi-norflash") == 0)) {
            of_property_read_u32(child, "norflash-jedec-id", &readid);
            if (readid == jedec) {
                of_property_read_u32(child, "norflash-ext-id", &ext_readid);
                if (ext_readid != 0 && ext_readid != ext_jedec) {
                    continue;
                } else {
                    info->child = child;
                    break;
                }
            }
        }
    }

    if (!info->child) {
        sfc_trace(SFC_ERR, "%s: no find match nor flash!\n", __func__);
        return NULL;
    }

    of_property_read_string(child, "norflash-name", &info->name);
    info->jedec_id = readid;
    info->ext_id = ext_readid;

    of_property_read_u32(child, "norflash-sector-size", &val);
    info->sector_size = val;
    of_property_read_u32(child, "norflash-n-sectors", &val);
    info->n_sectors = val;
    of_property_read_u32(child, "norflash-flags", &val);
    info->flags = val;

    sfc_trace(SFC_DBG, "%s: %s found, sector_size %d n-sectors %d flags 0x%x\n",
            __func__, info->name, info->sector_size,
            info->n_sectors, info->flags);
    return info;
}

/**
 * @brief  cfg spi norflash into address 4byte mode
 *.
 * @author qinlinyong
 * @date 2021-04-27
 * @param[in] flash      spiflash handle.
 * @return int return init success or failed
 * @retval returns zero on success
 * @retval return a non-zero error code if failed
 */
static int sfc_nor_cfg_addr4_mode(struct spi_nor *nor)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    int ret = 0;

    nor->addr_width = 3;
    if (info->flags & SFLAG_ADDR_4B) {
        ret = sfc_nor_addr4_mode_enable(nor);
        if (ret < 0) {
            sfc_trace(SFC_ERR, "config addr 4 byte enable fail. \
                    transfer use 3 byte.\n");
        } else {
            nor->addr_width = 4;
        }
    }
    return ret;
}

static int sfc_nor_check(struct spi_nor *nor)
{
    if (!nor->dev || !nor->read_reg || !nor->write_reg) {
        sfc_trace(SFC_ERR, "%s: invalid sfc nor!\n", __func__);
        return -EINVAL;
    }

    return 0;
}

static int sfc_nor_winbond_fix_erase_disturb(struct spi_nor *nor)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    int ret;
    unsigned drive_strength;
    u8 read_code;
    u8 write_code;
    u8 st_tmp= 0;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    if (info->jedec_id == 0xef4017 || info->jedec_id == 0xef4018) {
        drive_strength = 5;
        read_code = 0x15;
        write_code = 0x11;
    } else {
        return 0;
    }

    ret = sfc_nor_wait_till_ready(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                __func__, ret);
        return ret;
    }

    ret = nor->read_reg(nor, read_code, &st_tmp, 1);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d opcode 0x%x!\n",
                __func__, ret, read_code);
        return ret;
    }

    st_tmp &= ~(0x3<<drive_strength);
    st_tmp |= (0x3<<drive_strength);

    /*For winbond S19 bit.Recovery for Erase while power drop.*/
    if (info->jedec_id == 0xef4017 || info->jedec_id == 0xef4018) {
        st_tmp &= ~(0x1<<3);
        st_tmp |= (0x1<<3);
    }

    // write volatile status reg
    if ((info->wr_status_mode & 0x20)) {
        ret = nor->write_reg(nor,
                NOR_OPCODE_VOLATILE_SR_WRITE_ENABLE, NULL, 0);
    } else {
        ret = nor_write_enable(nor);
    }
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d write enable!\n",
                __func__, ret);
        return ret;
    }

    ret = nor->write_reg(nor, write_code, &st_tmp, 1);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d opcode 0x%x!\n",
                __func__, ret, read_code);
        return ret;
    }

    ret = sfc_nor_wait_till_ready(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d sfc_nor_wait_till_ready!\n",
                __func__, ret);
        return ret;
    }

    ret = nor_write_disable(nor);
    if (ret < 0) {
        sfc_trace(SFC_ERR, "%s: error %d nor_write_disable!\n",
                __func__, ret);
        return ret;
    }

    return 0;
}

static int sst_write(struct mtd_info *mtd, loff_t to, size_t len,
        size_t *retlen, const u_char *buf)
{
    struct spi_nor *nor = mtd_to_spi_nor(mtd);
    size_t actual;
    int ret;

    sfc_trace(SFC_DBG, "%s: to 0x%llx, len %d\n", __func__, to, len);

    if (!len)
        return 0;

    ret = sfc_nor_lock_and_prep(nor, SFC_NOR_OPS_WRITE);
    if (ret)
        return ret;

    nor_write_enable(nor);

    nor->sst_write_second = false;

    actual = to % 2;
    /* Start write from odd address. */
    if (actual) {
        nor->program_opcode = NOR_SST_OPCODE_BP;

        /* write one byte. */
        _sfc_nor_write(nor, to, 1, retlen, (u_char*)buf);
        ret = sfc_nor_wait_till_ready(nor);
        if (ret)
            goto time_out;
    }
    to += actual;

    /* Write out most of the data here. */
    for (; actual < len - 1; actual += 2) {
        nor->program_opcode = NOR_SST_OPCODE_AAI_WP;

        /* write two bytes. */
        _sfc_nor_write(nor, to, 2, retlen, (u_char*)(buf + actual));
        ret = sfc_nor_wait_till_ready(nor);
        if (ret)
            goto time_out;
        to += 2;
        nor->sst_write_second = true;
    }
    nor->sst_write_second = false;

    nor_write_disable(nor);
    ret = sfc_nor_wait_till_ready(nor);
    if (ret)
        goto time_out;

    /* Write out trailing byte if it exists. */
    if (actual != len) {
        nor_write_enable(nor);

        nor->program_opcode = NOR_SST_OPCODE_BP;
        _sfc_nor_write(nor, to, 1, retlen, (u_char*)(buf + actual));

        ret = sfc_nor_wait_till_ready(nor);
        if (ret)
            goto time_out;
        nor_write_disable(nor);
    }
time_out:
    sfc_nor_unlock_and_unprep(nor, SPI_NOR_OPS_WRITE);
    return ret;
}

/**
 * @brief  cfg spi norflash into quad mode
 *
 * @author SheShaohua
 * @date 2012-03-20
 * @param[in] flash      spiflash handle.
 * @return int return init success or failed
 * @retval returns zero on success
 * @retval return a non-zero error code if failed
 */
static int sfc_nor_cfg_quad_mode(struct spi_nor *nor)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    //struct ak_sfc *c = priv->c;
    int ret = 0;

    if((priv->bus_width & BUS_WIDTH_4WIRE) &&
            (info->flags & (SFLAG_QUAD_WRITE|SFLAG_QUAD_IO_WRITE|
                            SFLAG_QUAD_READ|SFLAG_QUAD_IO_READ))) {
        if((info->flags&SFLAG_QUAD_NO_QE) != SFLAG_QUAD_NO_QE){
            ret = sfc_nor_quad_mode_enable(nor);
            if (ret < 0) {
                priv->bus_width &= ~BUS_WIDTH_4WIRE;
                sfc_trace(SFC_ERR, "%s: config quad enable fail. \
                        use 1 wire.\n", __func__);
            }
        }
    } else {
        if (info->flags & (SFLAG_QUAD_WRITE|SFLAG_QUAD_IO_WRITE|
                    SFLAG_DUAL_READ|SFLAG_DUAL_IO_READ)){
            if((info->flags&SFLAG_QUAD_NO_QE) != SFLAG_QUAD_NO_QE){
                // cdh: test KH 1 WIRE for pin7 is not spi_hold but reset
                ret = sfc_nor_quad_mode_enable(nor);
                if (ret < 0) {
                    sfc_trace(SFC_ERR,
                            "%s: config quad enable fail.\n", __func__);
                }
            }
        }
    }

    return ret;
}

/**
 * @brief  init spiflash read and write cmd and data bus width info
 *
 * @author SheShaohua
 * @date 2012-03-20
 * @param[in] flash      spiflash handle.
 * @return int return init success or failed
 * @retval returns zero on success
 * @retval return a non-zero error code if failed
 */
static void sfc_nor_init_rw_info(struct spi_nor *nor)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_flash_info *info = priv->info;
    //struct ak_sfc *c = priv->c;

    /**default param.*/
    nor->read_opcode = NOR_OPCODE_READ;
    priv->rxd_bus_width = BUS_WIDTH_1WIRE;
    priv->rxa_bus_width = BUS_WIDTH_1WIRE;
    priv->rx_mode_bytes = 0;
    nor->flash_read = (NOR_OPCODE_READ == NOR_OPCODE_FAST_READ)?
        SPI_NOR_FAST: SPI_NOR_NORMAL;
    priv->rx_dummy = FAST_READ_DUMMY_BYTE * 8;
    nor->program_opcode = NOR_OPCODE_PP;
    priv->txd_bus_width = BUS_WIDTH_1WIRE;
    priv->txa_bus_width = BUS_WIDTH_1WIRE;

    if (priv->bus_width & BUS_WIDTH_2WIRE) {
        if(info->flags & SFLAG_DUAL_READ) {
            nor->read_opcode = NOR_OPCODE_FAST_D_READ;
            priv->rxd_bus_width = BUS_WIDTH_2WIRE;
            priv->rxa_bus_width = BUS_WIDTH_1WIRE;
            priv->rx_mode_bytes = 0;
            priv->rx_dummy = 8;
        } else if (info->flags & SFLAG_DUAL_IO_READ) {
            nor->read_opcode = NOR_OPCODE_FAST_D_IO;
            priv->rxd_bus_width = BUS_WIDTH_2WIRE;
            priv->rxa_bus_width = BUS_WIDTH_2WIRE;
            priv->rx_mode_bytes = 1;
            priv->rx_mode = 0xff;
            priv->rx_dummy = 0;
        }

        if(info->flags & SFLAG_DUAL_WRITE) {
            nor->program_opcode = NOR_OPCODE_PP_DUAL;
            priv->txd_bus_width = BUS_WIDTH_2WIRE;
            priv->txa_bus_width = BUS_WIDTH_1WIRE;
        } else if(info->flags & SFLAG_DUAL_IO_WRITE) {
            nor->program_opcode = NOR_OPCODE_2IO_PP;
            priv->txd_bus_width = BUS_WIDTH_2WIRE;
            priv->txa_bus_width = BUS_WIDTH_2WIRE;
        }
    }

    if(priv->bus_width & BUS_WIDTH_4WIRE){
        if(info->flags & SFLAG_DTR_QUAD_READ) {
            nor->read_opcode = NOR_OPCODE_DTR_FAST_Q_IO;
            priv->rxd_bus_width = BUS_WIDTH_4WIRE;
            priv->rxa_bus_width = BUS_WIDTH_4WIRE;
            priv->rx_mode_bytes = 1;
            priv->rx_mode = 0;
            priv->rx_dummy = info->dtr_dummy;
        } else if(info->flags & SFLAG_QUAD_READ) {
            nor->read_opcode = NOR_OPCODE_FAST_Q_READ;
            priv->rxd_bus_width = BUS_WIDTH_4WIRE;
            priv->rxa_bus_width = BUS_WIDTH_1WIRE;
            priv->rx_mode_bytes = 0;
            priv->rx_dummy = 8;
        }else if(info->flags & SFLAG_QUAD_IO_READ){
            nor->read_opcode = NOR_OPCODE_FAST_Q_IO;
            priv->rxd_bus_width = BUS_WIDTH_4WIRE;
            priv->rxa_bus_width = BUS_WIDTH_4WIRE;
            priv->rx_mode_bytes = 1;
            priv->rx_mode = 0xff;
            priv->rx_dummy = 4;
        }

        if(info->flags & SFLAG_QUAD_WRITE) {
            nor->program_opcode = NOR_OPCODE_PP_QUAD;
            priv->txd_bus_width = BUS_WIDTH_4WIRE;
            priv->txa_bus_width = BUS_WIDTH_1WIRE;
        }else if(info->flags & SFLAG_QUAD_IO_WRITE) {
            nor->program_opcode = NOR_OPCODE_4IO_PP;
            priv->txd_bus_width = BUS_WIDTH_4WIRE;
            priv->txa_bus_width = BUS_WIDTH_4WIRE;
        }
    }
}

/*
 * Erase the whole flash memory
 *
 * Returns 0 if successful, non-zero otherwise.
 */
static int nor_erase_chip(struct spi_nor *nor)
{
    sfc_trace(SFC_DBG, "%s: %lldKB\n", __func__, nor->mtd.size >> 10);

    return nor->write_reg(nor, NOR_OPCODE_CHIP_ERASE, NULL, 0);
}

static int _sfc_nor_erase(struct spi_nor *nor, loff_t from)
{
    struct sfc_nor_priv *priv = nor->priv;
    struct ak_sfc *c= priv->c;
    struct ak_flash_info *info = priv->info;
    unsigned int addr_size;

    sfc_trace(SFC_DBG, "%s: from 0x%llx\n", __func__, from);

    addr_size = (info->flags & SFLAG_ADDR_4B) ? 4 : 3;
    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_WRITE, nor->erase_opcode, 1, // dir, opcode, opcode_bytes
        from, addr_size, 0, // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        NULL, 0, 0, // buf, buf_len, data_mode,
        SFC_TRANS_TXD_RXD); // trans_mode

    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);

    if (!c->trans_errno) {
        return 0;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

/*
 * Erase an address range on the nor chip.  The address range may extend
 * one or more erase sectors.  Return an error is there is a problem erasing.
 */
static int sfc_nor_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct spi_nor *nor = mtd_to_spi_nor(mtd);
    struct sfc_nor_priv *priv = nor->priv;
    //struct ak_flash_info *info = priv->info;
    u32 addr, len, len1, align_to_large;
    uint32_t rem;
    int ret;

    sfc_trace(SFC_DBG, "%s: at 0x%llx, len %lld\n", __func__, instr->addr,
            instr->len);

    if (instr->addr + instr->len > mtd->size) {
        sfc_trace(SFC_ERR,
                "%s: addr %lld len %lld size %lld erase too large!\n",
                __func__, instr->addr, instr->len, mtd->size);
        return -EINVAL;
    }

    div_u64_rem(instr->len, mtd->erasesize, &rem);
    if (rem) {
        sfc_trace(SFC_ERR,
                "%s: len %lld erasesize %d len not align to erasesize!\n",
                __func__, instr->len, mtd->erasesize);
        return -EINVAL;
    }

    div_u64_rem(instr->addr, mtd->erasesize, &rem);
    if (rem) {
        sfc_trace(SFC_ERR,
                "%s: addr %lld erasesize %d addr not align to erasesize!\n",
                __func__, instr->addr, mtd->erasesize);
        return -EINVAL;
    }

    addr = instr->addr;
    len = instr->len;

    ret = sfc_nor_lock_and_prep(nor, SFC_NOR_OPS_ERASE);
    if (ret)
        return ret;

    /* whole-chip erase? */
    if (len == mtd->size) {
        unsigned long timeout;

        nor_write_enable(nor);

        if (nor_erase_chip(nor)) {
            ret = -EIO;
            goto erase_err;
        }

        /*
         * Scale the timeout linearly with the size of the flash, with
         * a minimum calibrated to an old 2MB flash. We could try to
         * pull these from CFI/SFDP, but these values should be good
         * enough for now.
         */
        timeout = max(NOR_MAX_ERASE_2MB_READY_WAIT_JIFFIES,
                NOR_MAX_ERASE_2MB_READY_WAIT_JIFFIES *
                (unsigned long)(mtd->size / SZ_2M));
        ret = sfc_nor_wait_till_ready_timeout(nor, timeout);
        if (ret)
            goto erase_err;

        /* REVISIT in some cases we could speed up erasing large regions
         * by using SPINOR_OP_SE instead of SPINOR_OP_BE_4K.  We may have set up
         * to use "small sector erase", but that's not always optimal.
         */

        /* "sector"-at-a-time erase */
    } else {
        u8 old_opcode = nor->erase_opcode;

        if (priv->erasesize_large > priv->erasesize_small) {
            align_to_large = addr % priv->erasesize_large;
            if (align_to_large)
                align_to_large = priv->erasesize_large - align_to_large;
            align_to_large = addr + align_to_large;
            while ((addr < align_to_large) && (len > 0)) {
                nor_write_enable(nor);
                if (_sfc_nor_erase(nor, addr)) {
                    ret = -EIO;
                    goto erase_err;
                }

                ret = sfc_nor_wait_till_ready(nor);
                if (ret)
                    goto erase_err;

                addr += mtd->erasesize;
                len -= mtd->erasesize;
            }

            nor->erase_opcode = priv->erase_opcode_large;
            len1 = len - len % priv->erasesize_large;
            while (len1 > 0) {
                nor_write_enable(nor);
                if (_sfc_nor_erase(nor, addr)) {
                    ret = -EIO;
                    goto erase_err;
                }

                ret = sfc_nor_wait_till_ready(nor);
                if (ret)
                    goto erase_err;

                addr += priv->erasesize_large;
                len -= priv->erasesize_large;
                len1 -= priv->erasesize_large;
            }

            nor->erase_opcode = old_opcode;
        }

        while (len > 0) {
            nor_write_enable(nor);
            if (_sfc_nor_erase(nor, addr)) {
                ret = -EIO;
                goto erase_err;
            }

            ret = sfc_nor_wait_till_ready(nor);
            if (ret)
                goto erase_err;

            addr += mtd->erasesize;
            len -= mtd->erasesize;
        }
    }

    nor_write_disable(nor);

    sfc_nor_unlock_and_unprep(nor, SPI_NOR_OPS_ERASE);

    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);

    return ret;

erase_err:
    sfc_nor_unlock_and_unprep(nor, SPI_NOR_OPS_ERASE);
    instr->state = MTD_ERASE_FAILED;
    return ret;
}

static int sfc_nor_scan(struct spi_nor *nor)
{
    struct ak_flash_info *info = NULL;
    struct sfc_nor_priv *priv = nor->priv;
    struct mtd_info *mtd = &nor->mtd;
    struct device *dev = nor->dev;
    struct ak_sfc *c = priv->c;
    int ret, number, i;
    u32 val;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    ret = sfc_nor_check(nor);
    if (ret)
        return ret;

    if (priv->readid_freq) {
        sfc_set_spi_baudrate(priv->c, priv->readid_freq);
        info = sfc_nor_read_id(nor);
        sfc_set_spi_baudrate(priv->c, c->freq);
    } else {
        sfc_set_spi_baudrate(priv->c, c->freq);
        info = sfc_nor_read_id(nor);
    }
    sfc_trace(SFC_INFO, "%s: sfc frequency is %ld\n",
            __func__, sfc_get_spi_baudrate(priv->c));

    if (!info)
        return -ENOENT;
    priv->info = info;

    mutex_init(&nor->lock);

    if (!mtd->name)
        mtd->name = dev_name(nor->dev);
    mtd->priv = nor;
    mtd->type = MTD_NORFLASH;
    mtd->writesize = NOR_PAGESIZE;
    mtd->flags = MTD_WRITEABLE;
    mtd->size = info->sector_size * info->n_sectors;
    mtd->_erase = sfc_nor_erase;
    mtd->_read = sfc_nor_read;

    /* sst nor chips use AAI word program */
    if (SFC_JEDEC_MFR(info) == SNOR_MFR_SST)
        mtd->_write = sst_write;
    else
        mtd->_write = sfc_nor_write;

    /*
     * if SFLAG_SECT_4K is set, mean support:
     * erase by section (4K)
     * erase by section (64K)
     */
    if (info->flags & SFLAG_SECT_4K) {
        nor->erase_opcode = NOR_OPCODE_SE;
        mtd->erasesize = 4*1024;
        priv->erase_opcode_small = nor->erase_opcode;
        priv->erasesize_small = mtd->erasesize;
        priv->erase_opcode_large = NOR_OPCODE_BE_64K;
        priv->erasesize_large = 64*1024;
    } else {
        /*
         * if SFLAG_SECT_4K is not set, mean support:
         * erase by section (64K)
         */
        nor->erase_opcode = NOR_OPCODE_BE_64K;
        mtd->erasesize = 64*1024;
        priv->erase_opcode_small = nor->erase_opcode;
        priv->erasesize_small = mtd->erasesize;
        priv->erase_opcode_large = priv->erase_opcode_small;
        priv->erasesize_large = priv->erasesize_small;
    }

#if 0
    /*
     *  ����gd25q64 ��B�汾��C �汾��Ȼ�������汾��״̬�Ĵ�����д��һ����������˸Ķ���
     */
    if (0xc84017 == info->jedec_id) {
        u8  id[2];
        u8  id_ver_c;

        sfc_cmd_addr_dummy_data(
                0, // bank_id,
                SFC_DIR_READ, 0x5a, 1, // dir, opcode, opcode_bytes
                0, 3, 0, // addr, addr_bytes, addr_mode,
                0, 0, 8,// mode, mode_bytes, dummy_cycles,
                id, 2, 0, // buf, buf_len, data_mode,
                SFC_TRANS_TXD_RXD); // trans_mode
        reinit_completion(&c->trans_done);
        sfc_start_transfer();
        wait_for_completion(&c->trans_done);
        if (!c->trans_errno) {
            val = __raw_readl(AK_VA_SFC + SFC_REG_DATA(0));
            sfc_trace(SFC_DBG, "%s,%d: data 0x%x\n", __func__, __LINE__,
                    val);
            id[0] = val & 0xff;
            id[1] = (val >> 8) & 0xff;
        }

        id_ver_c = id[0];
        if (0x53==id_ver_c) {
            info->id_ver_c = 1;
        }
    }
#endif

    /* get dts parameter
     * if has no this property, return -22 less 0, so must judge return value
     * get spi norflash read status reg cmd from dtsi
     */
    number = of_property_count_u32_elems(info->child, "rd_status_cmd");
    if (number < 0) {
        sfc_trace(SFC_ERR, "%s:no rd_status_cmd property found!\n", __func__);
        return -EINVAL;
    }

    if (number > ARRAY_SIZE(info->rd_status_cmd))
        number = ARRAY_SIZE(info->rd_status_cmd);

    for (i = 0; i < number; i++) {
        of_property_read_u32_index(info->child,
                "rd_status_cmd", i, &val);
        info->rd_status_cmd[i] = val;
    }

    /*
     * if has no this property, return -22 less 0, so must judge return value
     * get spi norflash write status reg cmd from dtsi
     */
    number = of_property_count_u32_elems(info->child, "wr_status_cmd");
    if (number < 0) {
        sfc_trace(SFC_ERR, "%s:no wr_status_cmd property found!\n", __func__);
        return -EINVAL;
    }

    if (number > ARRAY_SIZE(info->wr_status_cmd))
        number = ARRAY_SIZE(info->wr_status_cmd);

    for (i = 0; i < number; i++) {
        of_property_read_u32_index(info->child,
                "wr_status_cmd", i, &val);
        info->wr_status_cmd[i] = val;
    }

    /*
     * get write status reg wr_mode & norflash-wr_flags
     * config information from dtsi
     * wr_mode:
     *   bit0: 0:normal write status reg, 1:one cmd write one status reg
     *   bit4: write status reg then wait
     *   bit5: winbond write volatile status reg
     *   bit6: winbond clear status protect bits
     */
    ret = of_property_read_u32(info->child, "wr_mode",
            &info->wr_status_mode);
    if (ret) {
        pr_err("%s:no wr_mode property found!\n", __func__);
        return -EINVAL;
    }

    if (info->id_ver_c == 1) {
        info->wr_status_mode &= ~0xf;
        info->wr_status_mode |= 1;
    }

    /* norflash-wr_flags:
     *   bit1: write status2 reg
     *   bit2: write status3 reg
     */
    ret = of_property_read_u32(info->child, "norflash-wr_flags",
            &info->wr_status_flag);
    if (ret) {
        pr_err("%s:no norflash-wr_flags property found!\n", __func__);
        return -EINVAL;
    }

    /*
     * get status reg b_wip & b_qe bit map information from dtsi
     */
    ret = of_property_read_u32(info->child, "norflash-b-wip", &val);
    if (ret) {
        pr_err("%s:no norflash-b-wip property found!\n", __func__);
        return -EINVAL;
    }
    info->b_wip = val;

    ret = of_property_read_u32(info->child, "norflash-b-qe", &val);
    if (ret) {
        pr_err("%s:no norflash-b-qe property found!\n", __func__);
        return -EINVAL;
    }
    info->b_qe = val;

    ret = of_property_read_u32(info->child, "norflash-b-wel", &val);
    if (ret) {
        pr_err("%s:no norflash-b-wel property found!\n", __func__);
        return -EINVAL;
    }
    if (val >= 8) {
        pr_err("%s:norflash-b-wel is invalid!\n", __func__);
        return -EINVAL;
    }
    info->b_wel = val;

    ret = of_property_read_u32(info->child, "norflash-dtr-dummy", &val);
    if(ret && (info->flags & SFLAG_DTR_QUAD_READ)) {
        pr_err("%s:no norflash-dtr-dummy property found!\n", __func__);
        return -EINVAL;
    }
    info->dtr_dummy = val;

    /*
     * Atmel, SST, Intel/Numonyx, and others serial NOR tend to power up
     * with the software protection bits set
     */
    if (SFC_JEDEC_MFR(info) == SNOR_MFR_ATMEL ||
            SFC_JEDEC_MFR(info) == SNOR_MFR_INTEL ||
            SFC_JEDEC_MFR(info) == SNOR_MFR_SST) {
        nor_write_enable(nor);
        sfc_nor_write_sr(nor, 0);
    }

    mtd->dev.parent = dev;
    nor->page_size = NOR_PAGESIZE;
    mtd->writebufsize = nor->page_size;

    /* cfg norflash quad mode */
    ret = sfc_nor_cfg_quad_mode(nor);
    if (ret < 0) {
        return ret;
    }
    /* cfg norflash address 4byte mode */
    ret = sfc_nor_cfg_addr4_mode(nor);
    if (ret < 0) {
        return ret;
    }

    /*
     **Only For W25QxxJVxxIQ series chip.
     **For winbond S19 bit.Recovery for Erase while power drop.
     **/
    if (info->jedec_id == 0xef4017 || info->jedec_id == 0xef4018) {
        ret = sfc_nor_winbond_fix_erase_disturb(nor);
        if (ret < 0) {
            return ret;
        }
    }

    /* init norflash rw bus width cmd */
    sfc_nor_init_rw_info(nor);

    sfc_trace(SFC_INFO, "%s (%lld Kbytes)\n", info->name,
            (long long)mtd->size >> 10);

    sfc_trace(SFC_DBG,
            "mtd .name = %s, .size = 0x%llx (%lldMiB), "
            ".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
            mtd->name, (long long)mtd->size, (long long)(mtd->size >> 20),
            mtd->erasesize, mtd->erasesize / 1024, mtd->numeraseregions);

    if (mtd->numeraseregions)
        for (i = 0; i < mtd->numeraseregions; i++)
            sfc_trace(SFC_DBG,
                    "mtd.eraseregions[%d] = { .offset = 0x%llx, "
                    ".erasesize = 0x%.8x (%uKiB), "
                    ".numblocks = %d }\n",
                    i, (long long)mtd->eraseregions[i].offset,
                    mtd->eraseregions[i].erasesize,
                    mtd->eraseregions[i].erasesize / 1024,
                    mtd->eraseregions[i].numblocks);

    return 0;
}

static ssize_t sfc_nor_reset_store(struct device *dev,
            struct device_attribute *attr, const char *buf, size_t len)
{
    struct ak_sfc *c = dev_get_drvdata(dev);
    struct spi_nor *nor = c->nor;
    //struct sfc_nor_priv *priv = nor->priv;
    //struct ak_flash_info *info = priv->info;
    int err;
    u32 val;
    u8 code;

    if (!nor)
        return len;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    err = kstrtou32(buf, 10, &val);
    if (!err) {
        code = 0xab;
        err = nor->write_reg(nor, code, NULL, 0);
        if (err < 0) {
            sfc_trace(SFC_ERR, "%s: write_reg opcode 0x%x failed, ret %d\n",
                    __func__, code, err);
            return len;
        }
        mdelay(1);
        code = 0x66;
        err = nor->write_reg(nor, code, NULL, 0);
        if (err < 0) {
            sfc_trace(SFC_ERR, "%s: write_reg opcode 0x%x failed, ret %d\n",
                    __func__, code, err);
            return len;
        }
        code = 0x99;
        err = nor->write_reg(nor, code, NULL, 0);
        if (err < 0) {
            sfc_trace(SFC_ERR, "%s: write_reg opcode 0x%x failed, ret %d\n",
                    __func__, code, err);
            return len;
        }
        mdelay(1);
    }

    return len;
}

/**
*
*@brief: ak_spiflash_id
*@param[in] struct device *dev
*@param[in] struct device_attribute *attr
*@param[in] char *buf
*@return: ssize_t
*
**/
static ssize_t sfc_nor_id_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
    struct ak_sfc *c = dev_get_drvdata(dev);
    struct spi_nor *nor = c->nor;
    struct sfc_nor_priv *priv;
    struct ak_flash_info *info;

    if (nor) {
        priv = nor->priv;
        info = priv->info;
        sprintf(buf, "%x\n", info->jedec_id);
        return strlen(buf);
    } else {
        return 0;
    }
}

static DEVICE_ATTR_WO(sfc_nor_reset);

static DEVICE_ATTR_RO(sfc_nor_id);

static ssize_t sfc_trace_level_store(struct device *dev,
            struct device_attribute *attr, const char *buf, size_t len)
{
    int err;
    u32 val;

    sfc_trace(SFC_DBG, "%s\n", __func__);

    err = kstrtou32(buf, 10, &val);
    if (!err) {
        if ((val >= SFC_ERR) && (val <= SFC_DBG))
            trace_level = val;
    }

    return len;
}

static DEVICE_ATTR_WO(sfc_trace_level);

/**
 * Get spi flash device information and register it as a mtd device.
 */
static int sfc_nor_register(struct device_node *np, struct ak_sfc *c)
{
    struct device *dev = c->dev;
    struct spi_nor *nor;
    struct sfc_nor_priv *priv;
    struct mtd_info *mtd;
    int ret;

    nor = devm_kzalloc(dev, sizeof(*nor), GFP_KERNEL);
    if (!nor) {
        sfc_trace(SFC_ERR, "%s no memory!\n", __func__);
        return -ENOMEM;
    }

    nor->dev = dev;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        sfc_trace(SFC_ERR, "%s no memory!\n", __func__);
        return -ENOMEM;
    }

    ret = of_property_read_u32(np, "reg", &priv->chipselect);
    if (ret) {
        sfc_trace(SFC_ERR, "no reg property for %s\n", np->full_name);
        return -ENODEV;
    }

    ret = of_property_read_u32(np, "bus-width", &priv->bus_width);
    if (ret) {
        sfc_trace(SFC_ERR, "could not read bus-width property for %s\n",
                np->full_name);
        return -ENODEV;
    }

    ak_spi_nor = nor;

    priv->c = c;
    priv->np = np;
    priv->readid_freq = 50000000;
    nor->priv = priv;

    nor->prepare = sfc_nor_prep;
    nor->unprepare = sfc_nor_unprep;
    nor->read_reg = sfc_nor_read_reg;
    nor->write_reg = sfc_nor_write_reg;

    ret = sfc_nor_scan(nor);
    if (ret)
        return ret;

    mtd = &nor->mtd;
    mtd->name = SFC_NOR_MTD_NAME;
    ret = mtd_device_register(mtd, NULL, 0);
    if (ret) {
        sfc_trace(SFC_ERR, "add root MTD device failed, return %d\n", ret);
        return ret;
    }

    c->nor = nor;
    g_spiflash_flag = 1;

    ret = sysfs_create_file(&dev->kobj, &dev_attr_sfc_nor_id.attr);
    if (ret) {
        sfc_trace(SFC_ERR, "create sfc_nor_id sysfs file failed!\n");
        goto err_sysfs_id;
    }

    ret = sysfs_create_file(&dev->kobj, &dev_attr_sfc_nor_reset.attr);
    if (ret) {
        sfc_trace(SFC_ERR, "create sfc_nor_reset sysfs file failed!\n");
        goto err_sysfs_reset;
    }

    ret = sysfs_create_file(&dev->kobj, &dev_attr_sfc_trace_level.attr);
    if (ret) {
        sfc_trace(SFC_ERR, "create sfc_trace_level sysfs file failed!\n");
        goto err_sysfs_trace_level;
    }

    return 0;

err_sysfs_trace_level:
    sysfs_remove_file(&dev->kobj, &dev_attr_sfc_nor_reset.attr);
err_sysfs_reset:
    sysfs_remove_file(&dev->kobj, &dev_attr_sfc_nor_id.attr);
err_sysfs_id:
    mtd_device_unregister(mtd);

    return -EFAULT;
}

static void sfc_nor_unregister_all(struct ak_sfc *c)
{
    struct device *dev = c->dev;
    if (!c->nor)
        return;

    sysfs_remove_file(&dev->kobj, &dev_attr_sfc_trace_level.attr);
    sysfs_remove_file(&dev->kobj, &dev_attr_sfc_nor_reset.attr);
    sysfs_remove_file(&dev->kobj, &dev_attr_sfc_nor_id.attr);
    mtd_device_unregister(&c->nor->mtd);
}

static inline struct sfc_nand_priv *mtd_to_nandpriv(struct mtd_info *mtd)
{
    return container_of(mtd, struct sfc_nand_priv, mtd);
}

static struct ak_nand_flash_info *get_nand_flash_info(struct sfc_nand_priv *priv)
{
    struct ak_nand_flash_info *info;
    struct ak_sfc *c = priv->c;
    u8 id[5];
    u32 jedec;
    u16 ext_jedec = 0;
    //struct property *prop  = NULL;
    struct device_node *child;
    u32 readid = 0;
    u32 ext_readid = 0;
    u32 dataout;
    //int tmp;
    u32 reg_val;

    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_READ, NAND_OPCODE_RDID, 1, // dir, opcode, opcode_bytes
            0, 0, 0, // addr, addr_bytes, addr_mode,
            0, 0, 8,// mode, mode_bytes, dummy_cycles,
            id, NAND_ID_MAX_LEN, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode

    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
        return NULL;
    }

    reg_val = __raw_readl(AK_VA_SFC + SFC_REG_DATA(0));
    memcpy(id, &reg_val, NAND_ID_MAX_LEN);

    jedec = id[0];
    jedec = jedec << 8;
    jedec |= id[1];
    jedec = jedec << 8;
    jedec |= id[2];
    jedec = jedec << 8;
    jedec |= id[3];

    sfc_trace(SFC_INFO, "spi nandflash ID: 0x%08x\n", jedec);

    priv->info = &priv->finfo;
    info = priv->info;

    for_each_available_child_of_node(priv->np, child) {
        if (child->name && (of_node_cmp(child->name, "spi-nandflash") == 0)){
            of_property_read_u32(child, "nandflash-jedec-id", &readid);
            if (readid == jedec) {
                of_property_read_u32(child, "nandflash-ext-id", &ext_readid);
                if (ext_readid != 0 && ext_readid != ext_jedec) {
                    continue;
                } else {
                    info->child = child;
                    break;
                }
            }
        }
    }

    /*
    * judge search spi norflash device whether success or not
    */
    if (!info->child) {
        sfc_trace(SFC_ERR, "%s: no find match device!\n", __func__);
        priv->info = NULL;
        return NULL;
    }

    /*
    * get spi nandflash information
    */
    of_property_read_string(child, "nandflash-name", &info->name);
    info->jedec_id = readid;
    info->ext_id = ext_readid;
    of_property_read_u32(child, "nandflash-planecnt", &info->planecnt);
    of_property_read_u32(child, "nandflash-page_size", &info->page_size);
    of_property_read_u32(child, "nandflash-page_per_block",
                                                &info->page_per_block);
    of_property_read_u32(child, "nandflash-block_size", &info->block_size);
    of_property_read_u32(child, "nandflash-n_blocks", &dataout);
    info->n_blocks = (u16)dataout;
    of_property_read_u32(child, "nandflash-flags", &dataout);
    info->flags = (u16)dataout;

    of_property_read_u32(child, "nandflash-oob_size", &info->oob_size);
    of_property_read_u32(child, "nandflash-oob_seglen", &dataout);
    info->oob_seglen = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_seg_perpage", &dataout);
    info->oob_seg_perpage = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_up_skiplen", &dataout);
    info->oob_up_skiplen = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_down_skiplen", &dataout);
    info->oob_down_skiplen = (u16)dataout;
    of_property_read_u32(child, "nandflash-oob_vail_data_offset", &dataout);
    info->oob_vail_data_offset = (u16)dataout;

    of_property_read_u32(child, "nandflash-badflag_offs", &dataout);
    info->badflag_offs = (u16)dataout;
    of_property_read_u32(child, "nandflash-badflag_len", &dataout);
    info->badflag_len = (u16)dataout;
    of_property_read_u32(child, "nandflash-badflag_option", &dataout);
    info->badflag_option = (u32)dataout;

    if(!strcmp(info->name, "HYF1GQ4UDACAE"))
    {
        sfc_trace(SFC_INFO, "HYF1GQ4UDACAE especially\r\n");
        HeYangTek_Flag = 1;
    }else if(!strcmp(info->name, "HYF2GQ4UAACAE"))
    {
        sfc_trace(SFC_INFO, "HYF2GQ4UAACAE especially\r\n");
        HeYangTek_Flag = 1;
    }else if(!strcmp(info->name, "HYF4GQ4UAACAE"))
    {
        sfc_trace(SFC_INFO, "HYF4GQ4UAACAE especially\r\n");
        HeYangTek_Flag = 1;
    }

    return info;
} /*end of get_nand_flash_info*/

/**
* @brief Write status register
* 
*  Write status register 1 byte.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
* @param[in] val  register value to be write.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a negative error code if failed
*/
static int nand_write_sr(struct sfc_nand_priv *priv, u8 addr, u8 val)
{
    struct ak_sfc *c = priv->c;

#if 1
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_WRITE, NAND_OPCODE_WRSR1, 1, // dir, opcode, opcode_bytes,
            addr, 1, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            &val, 1, 0, // buf, buf_len, data_mode,
            SFC_TRANS_FIFO_CPU); // trans_mode
    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    __raw_writel(val, AK_VA_SFC + SFC_REG_FIFO_DATA);
#else
    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_WRITE, NAND_OPCODE_WRSR1, 1, // dir, opcode, opcode_bytes,
            (addr<<8)+val, 2, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            NULL, 0, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode
#endif
    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (!c->trans_errno) {
        return 0;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

/**
* @brief Read the status register.
* 
*  returning its value in the location
* @author lixinhai
* @date 2014-03-20
* @param[in] spiflash handle.
* @return int Return the status register value.
*/
static int nand_read_sr(struct sfc_nand_priv *priv, u8 addr)
{
    struct ak_sfc *c = priv->c;
    u8 val = 0;
    u32 reg_val;

    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_READ, NAND_OPCODE_RDSR1, 1, // dir, opcode, opcode_bytes,
            addr, 1, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            &val, 1, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode
    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (!c->trans_errno) {
        reg_val = __raw_readl(AK_VA_SFC + SFC_REG_DATA(0));
        return reg_val & 0xff;
    }
    sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
            __func__, c->trans_errno);
    return -EFAULT;
}

static inline int nand_flash_reset(struct sfc_nand_priv *priv)
{
    struct ak_sfc *c = priv->c;
    int ret;

    ret = sfc_write_reg(c, NAND_OPCODE_RESET, 1, NULL, 0);
    if (ret < 0)
        return ret;
    ret = nand_write_sr(priv, 0xa0, 0);
    return ret;
}

/**
* @brief  Wait for SPI flash ready.
* 
*  Service routine to read status register until ready, or timeout occurs.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int nand_wait_till_ready(struct sfc_nand_priv *priv)
{
    struct ak_sfc *c = priv->c;
    struct ak_nand_flash_info *info = priv->info;
    int shift;
    u8 addr;
    u32 val;
    int ret;

    shift = info->b_wip / 8;
    addr = nand_feature_cmd[shift];
    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_READ, NAND_OPCODE_RDSR1, 1, // dir, opcode, opcode_bytes
            addr, 1, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            NULL, 1, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode
    val = __raw_readl(AK_VA_SFC + SFC_REG_INTERFACE(0));
    val |= (1 << CFG_SPI_CMD_TYPE); // polling cmd
    val &= ~(1 << CFG_POLLING_MATCH_MODE);
    __raw_writel(val, AK_VA_SFC + SFC_REG_INTERFACE(0));
    __raw_writel(~(1 << info->b_wip), AK_VA_SFC + SFC_REG_MATCH_MASK(0));
    __raw_writel(0, AK_VA_SFC + SFC_REG_MATCH_DATA(0));
    __raw_writel(0, AK_VA_SFC + SFC_REG_INTERVAL(0));

    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    ret = wait_for_completion_timeout(&c->trans_done, NAND_MAX_READY_WAIT_JIFFIES);

    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
    }
    if (ret > 0) {
        return c->trans_errno;
    } else {
        sfc_trace(SFC_DBG, "%s: transfer timeout\n", __func__);
        return -ETIMEDOUT;
    }
}

/**
* @brief Set write enable latch.
* 
*  Set write enable latch with Write Enable command.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a negative error code if failed
*/
static int nand_write_enable(struct sfc_nand_priv *priv)
{
    struct ak_sfc *c = priv->c;
    int ret;

    ret = sfc_write_reg(c, NAND_OPCODE_WREN, 1, NULL, 0);
    return ret;
}

/**
* @brief Set write disble
* 
*  Set write disble instruction to the chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash  spiflash handle.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a negative error code if failed
*/
static int nand_write_disable(struct sfc_nand_priv *priv)
{
    struct ak_sfc *c = priv->c;
    int ret;

    ret = sfc_write_reg(c, NAND_OPCODE_WRDI, 1, NULL, 0);
    return ret;
}

/**
* @brief: enable 4 wire transfer mode.
* 
* @author lixinhai
* @date 2014-04-10
* @param[in] flash  spiflash handle.
*/
static int nand_quad_mode_enable(struct sfc_nand_priv *priv)
{
    int ret, idx, shift;
    u32 regval;
    u8 addr;
    struct ak_nand_flash_info *info = priv->info;

    if (info->flags & SFLAG_QUAD_NO_QE) {
        return 0;
    }

    sfc_trace(SFC_DBG, "%s b_qe %d\n", __func__, info->b_qe);

    shift = info->b_qe / 8;
    idx = info->b_qe % 8;

    addr = nand_feature_cmd[shift];
    ret = nand_wait_till_ready(priv);
    if (ret)
        return -EBUSY;

    nand_write_enable(priv);

    regval = nand_read_sr(priv, addr);
    regval |= 1<<(info->b_qe % 8);
    nand_write_sr(priv, addr, regval);

    regval = nand_read_sr(priv, addr);

    nand_write_disable(priv);
    sfc_trace(SFC_INFO, "%s: [0x%x] 0x%x\n", __func__, addr, regval);
    return 0;
}

/**
* @brief: disable 4 wire transfer mode.
* 
* @author lixinhai
* @date 2014-04-10
* @param[in] flash  spiflash handle.
*/
static int nand_quad_mode_disable(struct sfc_nand_priv *priv)
{
    int ret, idx, shift;
    u32 regval;
    u8 addr;
    struct ak_nand_flash_info *info = priv->info;

    if (info->flags & SFLAG_QUAD_NO_QE) {
        return 0;
    }

    sfc_trace(SFC_DBG, "%s\n", __func__);

    shift = info->b_qe / 8;
    idx = info->b_qe % 8;
    addr = nand_feature_cmd[shift];
    ret = nand_wait_till_ready(priv);
    if (ret)
        return -EBUSY;

    nand_write_enable(priv);

    regval = nand_read_sr(priv, addr);
    regval &= ~(1<<(info->b_qe%8));
    nand_write_sr(priv, addr, regval);

    regval = nand_read_sr(priv, addr);
    nand_write_disable(priv);
    sfc_trace(SFC_INFO, "%s: [0x%x] 0x%x\n", __func__, addr, regval);
    return 0;
}

/**
 * ak_spinand_enable_ecc- send command 0x1f to write the SPI Nand OTP register
 * Description:
 *   There is one bit( bit 0x10 ) to set or to clear the internal ECC.
 *   Enable chip internal ECC, set the bit to 1
 *   Disable chip internal ECC, clear the bit to 0
 */
static int sfc_nand_enable_ecc(struct sfc_nand_priv *priv)
{
    int retval;
    u8 otp = 0;

    otp=(u8)nand_read_sr(priv,0xb0);

    if ((otp & NAND_OTP_ECC_MASK) == NAND_OTP_ECC_MASK) {
        return 0;
    }
    else
    {
        otp |= NAND_OTP_ECC_MASK;
        retval=nand_write_sr(priv, 0xb0, (u16)otp);
        if (retval != 0)
            return retval;
    }

    return 0;
}

static int sfc_nand_disable_ecc(struct sfc_nand_priv *priv)
{
    int retval;
    u8 otp = 0;

    otp=(u8)nand_read_sr(priv,0xb0);

    if ((otp & NAND_OTP_ECC_MASK) == NAND_OTP_ECC_MASK) {
        otp &= ~NAND_OTP_ECC_MASK;
        retval=nand_write_sr(priv, 0xb0, (u16)otp);
        if (retval != 0)
            return retval;
    }
    return 0;
}

/**
* @brief  Erase sector
* 
*  Erase a sector specialed by user.
* @author lixinhai
* @date 2014-03-20
* @param[in] flash      spiflash handle.
* @param[in] offset    which is any address within the sector
*                      which should be erased.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int nand_erase_block(struct sfc_nand_priv *priv, u32 offset)
{
    u32 row;
    struct ak_sfc *c = priv->c;

    sfc_trace(SFC_DBG, "%s: %dKiB at 0x%08x\n",
            __func__, priv->mtd.erasesize / 1024, offset);

    row = ((offset>>priv->page_shift) & 0xffffff);

    /* Wait until finished previous write command. */
    if (nand_wait_till_ready(priv)) {
        sfc_trace(SFC_ERR, "%s: previous write command failed\n", __func__);
        return -EBUSY;
    }

    /* Send write enable, then erase commands. */
    nand_write_enable(priv);

    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_WRITE, priv->erase_opcode, 1, // dir, opcode, opcode_bytes,
            row, 3, 0, // addr, addr_bytes, addr_mode,
            0, 0, 0,// mode, mode_bytes, dummy_cycles,
            NULL, 0, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode
    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
        return -EFAULT;
    }

    if (nand_wait_till_ready(priv)) {
        sfc_trace(SFC_DBG, "%s: write command failed\n", __func__);
        /* REVISIT status return?? */
        return -EBUSY;
    }

    return 0;
}

/**
* @brief  MTD Erase
* 
* Erase an address range on the flash chip.
* @author luoyongchuang
* @date 2015-05-17
* @param[in] mtd    mtd info handle.
* @param[in] instr   erase info.
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int sfc_nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);
    u32 addr,len;
    uint32_t rem;

    sfc_trace(SFC_DBG, "%s: at 0x%llx, len %lld\n",
          __func__, instr->addr, instr->len);

    /* sanity checks */
    if (instr->addr + instr->len > mtd->size) {
        sfc_trace(SFC_DBG, "%s: instr->addr[0x%llx] + \
                        instr->len[%lld] > mtd->size[%lld]\n",
                        __func__, instr->addr, instr->len, mtd->size);
        return -EINVAL;
    }
    div_u64_rem(instr->len, mtd->erasesize, &rem);
    if (rem != 0) {
        sfc_trace(SFC_DBG, "%s: rem!=0 [%u]\n", __func__, rem);
        return -EINVAL;
    }

    addr = instr->addr;
    len = instr->len;

    mutex_lock(&priv->lock);

    while (len) {
        if (nand_erase_block(priv, addr)) {
            instr->state = MTD_ERASE_FAILED;
            mutex_unlock(&priv->lock);
            return -EIO;
        }

        addr += mtd->erasesize;
        len -= mtd->erasesize;
    }

    mutex_unlock(&priv->lock);

    instr->state = MTD_ERASE_DONE;
    mtd_erase_callback(instr);

    return 0;
}

//#ifdef SPINAND_USE_MTD_BLOCK_LAYER
#if 1
/**
* @brief: because of the _read() function call by mtd block layer, the buffer be
* allocate by vmalloc() in mtd layer, spi driver layer may use this buffer that 
* intents of use for DMA transfer, so, add this function to transition buffer.
* call this function at before real read/write data.
* 
* @author lixinhai
* @date 2013-04-10
* @param[in] flash  spiflash handle.
* @param[in] buffer.
* @param[in] buffer len
* @param[in] read/write
* @retval return the transition buffer
*/
static void *nand_flash_buf_bounce_pre(struct sfc_nand_priv *priv,
                void *buf, u32 len, int dir)
{
    if(!is_vmalloc_addr(buf)) {
        return buf;
    }

    if(dir == NAND_FLASH_WRITE) {
        memcpy(priv->buf, buf, len);
    }
    return priv->buf;
}

/**
* @brief: because of the _read() function call by mtd block layer, the buffer be
* allocate by vmalloc() in mtd layer, spi driver layer may use this buffer that 
* intents of use for DMA transfer, so, add this function to transition buffer.
* call this function at after real read/write data
* 
* @author lixinhai
* @date 2013-04-10
* @param[in] flash  spiflash handle.
* @param[in] buffer.
* @param[in] buffer len
* @param[in] read/write
* @retval return the transition buffer
*/
static void nand_flash_buf_bounce_post(struct sfc_nand_priv *priv,
                void *buf, u32 len, int dir)
{
    if (!is_vmalloc_addr(buf)) {
        return;
    }

    if (dir == NAND_FLASH_READ) {
        memcpy(buf, priv->buf, len);
    }
}
#else
static inline void *nand_flash_buf_bounce_pre(struct sfc_nand_priv *priv,
                void *buf, u32 len, int dir)
{
    return buf;
}

static inline void nand_flash_buf_bounce_post(struct sfc_nand_priv *priv,
                void *buf, u32 len, int dir)
{
}
#endif

static int sfc_nand_dma_write(struct sfc_nand_priv *priv, unsigned int row, unsigned int col,
        size_t len, size_t *retlen, u_char *buf)
{
    struct ak_sfc *c= priv->c;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    dma_addr_t buf_dma;
    int ret;
    void *bounce_buf;
    int block = row/priv->info->page_per_block;

    sfc_trace(SFC_DBG, "%s: row 0x%x col 0x%x len %d\n",
            __func__, row, col, len);

    if (len == 0) {
        if (retlen)
            *retlen = len;
        return 0;
    }

    if (retlen)
        *retlen = 0;

    bounce_buf = nand_flash_buf_bounce_pre(priv, buf, len, NAND_FLASH_WRITE);

    buf_dma = dma_map_single(NULL, bounce_buf, len, DMA_TO_DEVICE);
    if (dma_mapping_error(NULL, buf_dma)) {
        sfc_trace(SFC_DBG, "%s: failed to map buffer\n", __func__);
        return -ENOMEM;
    }

    /* Wait until finished previous write command. */
    if (nand_wait_till_ready(priv)) {
        sfc_trace(SFC_ERR, "%s: previous write cmd failed\n", __func__);
        return -EBUSY;
    }

    /* block number is beginning from 0.*/
    if(priv->info->planecnt == 2 && block%2 != 0) {
        col = col | (0x10 << 8);
    }

    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_WRITE, NAND_OPCODE_WREN, 1, // dir, opcode, opcode_bytes
        0, 0, 0, // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        NULL, 0, 0, // buf, buf_len, data_mode,
        SFC_TRANS_TXD_RXD); // trans_mode
    sfc_cmd_addr_dummy_data(
        1, // bank_id,
        SFC_DIR_WRITE, priv->tx_opcode, 1, // dir, opcode, opcode_bytes
        col, 2, bus_width_to_sfc_mode(priv->txa_bus_width), // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        bounce_buf, len, bus_width_to_sfc_mode(priv->txd_bus_width), // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_DMA); // trans_mode
    sfc_cmd_addr_dummy_data(
        2, // bank_id,
        SFC_DIR_WRITE, NAND_OPCODE_PP_EXEC, 1, // dir, opcode, opcode_bytes
        row, 3, 0, // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        NULL, 0, 0, // buf, buf_len, data_mode,
        SFC_TRANS_TXD_RXD); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    l2_clr_status(c->l2buf_id);
    l2_combuf_dma(buf_dma, c->l2buf_id, len, MEM2BUF, 0);
    sfc_start_transfer();
    ret = wait_for_completion_timeout(&c->trans_done, NAND_MAX_WRITE_TIME);
    if (!l2_combuf_wait_dma_finish(c->l2buf_id)) {
        sfc_trace(SFC_ERR,
                "%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        dma_unmap_single(NULL, buf_dma, len, DMA_TO_DEVICE);
        return -EFAULT;
    }

    if (ret == 0) {
        sfc_trace(SFC_INFO, "%s: row 0x%x col 0x%x len %d timeout!\n"
                " SFC_REG_STATUS 0x%x SFC_REG_TRAN_ENABLE 0x%x\n",
                __func__, row, col, len,
                __raw_readl(AK_VA_SFC + SFC_REG_STATUS),
                __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE));
        sfc_reset_hw(priv->c);
        dma_unmap_single(NULL, buf_dma, len, DMA_TO_DEVICE);
        return 0;
    }

    /* Wait until finished previous write command. */
    if (nand_wait_till_ready(priv)) {
        sfc_trace(SFC_ERR, "%s: write cmd failed\n", __func__);
        dma_unmap_single(NULL, buf_dma, len, DMA_TO_DEVICE);
        return -EBUSY;
    }

    if (retlen)
        *retlen = len;

    dma_unmap_single(NULL, buf_dma, len, DMA_TO_DEVICE);

    nand_flash_buf_bounce_post(priv, buf, len, NAND_FLASH_WRITE);

    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
        return -EFAULT;
    }
    return 0;
}

static int sfc_nand_cpu_write(struct sfc_nand_priv *priv, unsigned int row, unsigned int col,
        size_t len, size_t *retlen, u_char *buf)
{
    struct ak_sfc *c = priv->c;
    u32 val;
    unsigned int i, j;
    unsigned int thres_cnt;
    unsigned int frac_thres_cnt;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    unsigned char *p_buf = buf;
    //unsigned int addr_size;
    int ret;
    int block = row/priv->info->page_per_block;

    sfc_trace(SFC_DBG, "%s: row 0x%x col 0x%x len %d\n",
            __func__, row, col, len);

    if (len == 0) {
        if (retlen)
            *retlen = len;
        return 0;
    }

    if (retlen)
        *retlen = 0;

    /* Wait until finished previous write command. */
    if (nand_wait_till_ready(priv)) {
        sfc_trace(SFC_ERR, "%s: previous write cmd failed\n", __func__);
        return -EBUSY;
    }

    /* block number is beginning from 0.*/
    if(priv->info->planecnt == 2 && block%2 != 0) {
        col = col | (0x10 << 8);
    }

    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_WRITE, NAND_OPCODE_WREN, 1, // dir, opcode, opcode_bytes
        0, 0, 0, // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        NULL, 0, 0, // buf, buf_len, data_mode,
        SFC_TRANS_TXD_RXD); // trans_mode
    sfc_cmd_addr_dummy_data(
        1, // bank_id,
        SFC_DIR_WRITE, priv->tx_opcode, 1, // dir, opcode, opcode_bytes
        col, 2, bus_width_to_sfc_mode(priv->txa_bus_width), // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        buf, len, bus_width_to_sfc_mode(priv->txd_bus_width), // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_CPU); // trans_mode
    sfc_cmd_addr_dummy_data(
        2, // bank_id,
        SFC_DIR_WRITE, NAND_OPCODE_PP_EXEC, 1, // dir, opcode, opcode_bytes
        row, 3, 0, // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        NULL, 0, 0, // buf, buf_len, data_mode,
        SFC_TRANS_TXD_RXD); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    sfc_start_transfer();

    thres_cnt = len / ((SFC_TXFIFO_SIZE - tx_thres) * 4);
    frac_thres_cnt = len % ((SFC_TXFIFO_SIZE - tx_thres) * 4);
    for (i = 0; i < thres_cnt; i++) {
        while(!(__raw_readl(AK_VA_SFC + SFC_REG_STATUS) & (0x1<<2))); //等待FIFO阈值
        for (j = 0; j < SFC_TXFIFO_SIZE - tx_thres; j++) {
            val = (p_buf[0] | p_buf[1]<<8 | p_buf[2]<<16 | p_buf[3]<<24);
            __raw_writel(val, AK_VA_SFC + SFC_REG_FIFO_DATA);
            p_buf += 4;
        }
    }

    if (frac_thres_cnt) {
        while(!(__raw_readl(AK_VA_SFC + SFC_REG_STATUS) & (0x1<<2))); //等待FIFO阈值
        for(i = 0; i < frac_thres_cnt / 4; i++) {
            val = (p_buf[0] | p_buf[1]<<8 | p_buf[2]<<16 | p_buf[3]<<24);
            __raw_writel(val, AK_VA_SFC + SFC_REG_FIFO_DATA);
            p_buf += 4;
        }
        val = 0;
        for(i = 0; i < frac_thres_cnt % 4; i++) {
            val |= *(p_buf) << (i * 8);
            p_buf++;
        }
        if (frac_thres_cnt % 4 != 0)
            __raw_writel(val, AK_VA_SFC + SFC_REG_FIFO_DATA);
    }

    ret = wait_for_completion_timeout(&c->trans_done, NAND_MAX_WRITE_TIME);
    if (ret == 0) {
        sfc_trace(SFC_INFO, "%s: row 0x%x col 0x%x len %d timeout!"
                " SFC_REG_STATUS 0x%x\n SFC_REG_TRAN_ENABLE 0x%x",
                __func__, row, col, len,
                __raw_readl(AK_VA_SFC + SFC_REG_STATUS),
                __raw_readl(AK_VA_SFC + SFC_REG_TRAN_ENABLE));

        sfc_reset_hw(priv->c);
        return 0;
    }

    if (retlen)
        *retlen = len;

    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
        return -EFAULT;
    }

    return 0;
}

static int sfc_nand_write_page(struct sfc_nand_priv *priv, unsigned int row, unsigned int col,
        size_t len, size_t *retlen, u_char *buf)
{
    if (len < 64) {
        return sfc_nand_cpu_write(priv, row, col, len, retlen, buf);
    } else {
        return sfc_nand_dma_write(priv, row, col, len, retlen, buf);
    }
}

static int sfc_nand_write(struct mtd_info *mtd, loff_t to, size_t len,
        size_t *retlen, const u_char *buf)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len;
    u32 offset = 0;
    u32 count = len;
    int row, column;
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);
    //struct ak_nand_flash_info *info = priv->info;

    if (retlen)
        *retlen = 0;

    sfc_trace(SFC_DBG, "%s: to 0x%llx len %d\n", __func__, to, len);

    mutex_lock(&priv->lock);

    ret = sfc_alloc_l2buf(priv->c);
    if (ret) {
        mutex_unlock(&priv->lock);
        return ret;
    }

    /*decode row/column in address param*/
    row = ((to>>priv->page_shift) & 0xffffff);
    column = (to & 0x7ff);
    while(count > 0) {
        xfer_len = (count > NAND_FLASH_BUF_SIZE) ? NAND_FLASH_BUF_SIZE : count;

        /*transfer len not greater than page size*/
        if(xfer_len > priv->info->page_size)
            xfer_len = ALIGN_DOWN(xfer_len, priv->info->page_size);
        if(xfer_len+column >= priv->info->page_size)
            xfer_len = priv->info->page_size - column;

        ret = sfc_nand_write_page(priv, row, column, xfer_len, &rlen, (u_char *)(buf + offset));
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }

        row++;
        column = 0;
        if (retlen)
            *retlen += rlen;
        count -= rlen;
        offset += rlen;
    }
out:
    sfc_free_l2buf(priv->c);
    mutex_unlock(&priv->lock);
    return ret;
}

static int check_ecc_status(struct sfc_nand_priv *priv)
{
    int shift;
    u32 sr;
    u8 addr;
    struct ak_nand_flash_info *info = priv->info;

    shift = info->b_wip / 8;
    addr = nand_feature_cmd[shift];
    if ((sr = nand_read_sr(priv, addr)) < 0) {
        sfc_trace(SFC_ERR, "%s: nand_read_sr failed\n", __func__);
        return 1;
    }

    /*
     * XT26G01C和XT26G02C型号nand的ecc状态位需要特殊处理
     */
    if (0x0b110011 == info->jedec_id || 0x0b120012 == info->jedec_id) {
        if(((sr >> 4) & 0x0f) > 8){
            sfc_trace(SFC_ERR, "ecc error sr:0x%x\n", sr);
            return 1;
        }
    } else {
        if(((sr >> 4) & 0x3) == 2) {
            sfc_trace(SFC_ERR, "ecc error sr:0x%x\n", sr);
            return 1;
        }
    }

    return 0;
}

static int sfc_nand_dma_read(struct sfc_nand_priv *priv, unsigned int row, unsigned int col,
        size_t len, size_t *retlen, u_char *buf, int en_ecc)
{
    struct ak_sfc *c= priv->c;
    unsigned int rx_thres = 5;
    unsigned int tx_thres = 4;
    dma_addr_t buf_dma;
    //unsigned int addr_size;
    void *bounce_buf;
    int block = row/priv->info->page_per_block;

    sfc_trace(SFC_DBG, "%s: row 0x%x col 0x%x len %d en_ecc %d\n",
            __func__, row, col, len, en_ecc);

    if (len == 0) {
        if (retlen)
            *retlen = len;
        return 0;
    }

    if (retlen)
        *retlen = 0;

    bounce_buf = nand_flash_buf_bounce_pre(priv, buf, len, NAND_FLASH_READ);

    buf_dma = dma_map_single(NULL, bounce_buf, len, DMA_FROM_DEVICE);
    if (dma_mapping_error(NULL, buf_dma)) {
        sfc_trace(SFC_DBG, "%s: failed to map buffer\n", __func__);
        return -ENOMEM;
    }

    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_WRITE, NAND_OPCODE_READ_TO_CACHE, 1, // dir, opcode, opcode_bytes
        row, 3, 0, // addr, addr_bytes, addr_mode,
        0, 0, 0,// mode, mode_bytes, dummy_cycles,
        NULL, 0, 0, // buf, buf_len, data_mode,
        SFC_TRANS_TXD_RXD); // trans_mode

    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (c->trans_errno) {
        sfc_trace(SFC_ERR, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
        dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
        return -EFAULT;
    }

    /* Wait until finished previous write command. */
    if (nand_wait_till_ready(priv)) {
        sfc_trace(SFC_ERR, "%s: write cmd failed\n", __func__);
        dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
        return -EBUSY;
    }

    /* block number is beginning from 0.*/
    if(priv->info->planecnt == 2 && block%2 != 0) {
        col = col | (0x10 << 8);
    }

    sfc_cmd_addr_dummy_data(
        0, // bank_id,
        SFC_DIR_READ, priv->rx_opcode, 1, // dir, opcode, opcode_bytes
        col, 2, bus_width_to_sfc_mode(priv->rxa_bus_width), // addr, addr_bytes, addr_mode,
        0, 0, priv->rx_dummy,// mode, mode_bytes, dummy_cycles,
        bounce_buf, len, bus_width_to_sfc_mode(priv->rxd_bus_width), // buf, buf_len, data_mode,
        SFC_TRANS_FIFO_DMA); // trans_mode

    sfc_fifo_clear();
    sfc_fifo_clear();
    sfc_fifo_thres_cfg(rx_thres, tx_thres);
    reinit_completion(&c->trans_done);
    l2_clr_status(c->l2buf_id);
    l2_combuf_dma(buf_dma, c->l2buf_id, len, BUF2MEM, 0);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (!l2_combuf_wait_dma_finish(c->l2buf_id)) {
        sfc_trace(SFC_ERR,
                "%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
        return -EFAULT;
    }

    if (retlen)
        *retlen = len;

    dma_unmap_single(NULL, buf_dma, len, DMA_FROM_DEVICE);
    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
        return -EFAULT;
    }

#if 0
    {
        int i;
        char *p = bounce_buf;
        for (i = 0; i < (len/8)*8; i+=8) {
            sfc_trace(SFC_DBG, "%02x %02x %02x %02x %02x %02x %02x %02x\n",
                    p[i], p[i+1], p[i+2], p[i+3],
                    p[i+4], p[i+5], p[i+6], p[i+7]);
        }
    }
#endif

    if (en_ecc && check_ecc_status(priv)) {
        /* REVISIT status return?? */
        sfc_trace(SFC_DBG, "%s: check_ecc_status error, row:0x%x\n", __func__, row);
        return -EBUSY;
    }

    nand_flash_buf_bounce_post(priv, buf, len, NAND_FLASH_READ);
    return 0;
}

static int sfc_nand_read_page(struct sfc_nand_priv *priv, unsigned int row, unsigned int col,
        size_t len, size_t *retlen, u_char *buf, int en_ecc)
{
    return sfc_nand_dma_read(priv, row, col, len, retlen, buf, en_ecc);
}

/**
* @brief configure spi nandflash transfer mode according to flags. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] mtd info handle.
* @param[in] from: address.
* @param[in] transfer len.
* @param[out] transfer result len.
* @param[out] result buffer.
* @return int return config success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int sfc_nand_read(struct mtd_info *mtd, loff_t from, size_t len,
        size_t *retlen, u_char *buf)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len;
    u32 offset = 0;
    u32 count = len;
    int row, column;
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n", __func__, from, len);

    if (retlen)
        *retlen = 0;

    mutex_lock(&priv->lock);

    ret = sfc_alloc_l2buf(priv->c);
    if (ret) {
        mutex_unlock(&priv->lock);
        return ret;
    }

    /*decode row/column in address param*/
    row = ((from>>priv->page_shift) & 0xffffff);
    column = (from & 0x7ff);
    while(count > 0) {
        xfer_len = (count > NAND_FLASH_BUF_SIZE) ? NAND_FLASH_BUF_SIZE : count;

        /*transfer len not greater than page size*/
        if(xfer_len > priv->info->page_size)
            xfer_len = ALIGN_DOWN(xfer_len, priv->info->page_size);
        if(xfer_len+column >= priv->info->page_size)
            xfer_len = priv->info->page_size - column;

        ret = sfc_nand_read_page(priv, row, column, xfer_len,
                &rlen, buf + offset, NAND_READ_ECC_ENABLE);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }
        row++;
        column = 0;
        if (retlen)
            *retlen += rlen;
        count -= rlen;
        offset += rlen;

    }
out:
    sfc_free_l2buf(priv->c);
    mutex_unlock(&priv->lock);
    return ret;
}

/**
* @brief adjust transfer len according to readlen and column. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @param[in] column pos.
* @param[in] need read length.
* @retval return transfer len 
*/
static int adjust_xfer_len(struct sfc_nand_priv *priv, int column, int readlen)
{
    int seg_oob;
    int xfer_len;
    int ofs = priv->mtd.writesize;
    int start = column - ofs;
    int end;

    /*|--------------------64bytes------------------------------|*/
    /*|---12---|-4--|---12----|-4--|---12---|-4--|---12----|-4--|*/
    /*|-seglen-|skip|-segllen-|skip|-seglen-|skip|-segllen-|skip|*/

    xfer_len = (readlen > priv->info->oob_seglen)
                ? priv->info->oob_seglen : readlen;
    end = start + xfer_len;
    seg_oob = priv->info->oob_up_skiplen + priv->info->oob_seglen
            + priv->info->oob_down_skiplen;
    if(start/seg_oob != end/seg_oob)
        end = (start/seg_oob + 1)*seg_oob;

    xfer_len = end - start;

    return xfer_len;
}

/**
* @brief convert oob offset and addr pos to row/column coord. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @param[in] read from addr
* @param[in] offset to read from
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int nand_oobpos_to_coord(struct sfc_nand_priv *priv,
        loff_t addr, uint32_t ooboffs, int *row, int *column)
{
    *row = ((addr >> priv->page_shift) & 0xffffff);
    *column = (addr & (priv->mtd.writesize - 1));

    *row += ooboffs / priv->mtd.oobsize;
    *column += ooboffs % priv->mtd.oobsize;

    *column += priv->mtd.writesize;

    if(*column > (priv->mtd.writesize + NAND_OOB_SIZE))
        return -EINVAL;

    return 0;
}

static int sfc_nand_do_read_oob(struct mtd_info *mtd, loff_t from,
                struct mtd_oob_ops *ops)
{

    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len;
    u32 offset = 0;
    int row, column;
    uint8_t *oobrw_buf;
    int readlen;
    uint8_t *buf = NULL;
    ops->oobretlen = 0;
    readlen = ops->ooblen;
    buf = ops->oobbuf;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n",
            __func__, from, readlen);

    if (unlikely(ops->ooboffs >= mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt to start read "
                    "outside oob\n", __func__);
        return -EINVAL;
    }

    /* Do not allow reads past end of device */
    if (unlikely(from >= mtd->size ||
             ops->ooboffs + readlen > ((mtd->size >> priv->page_shift) -
                    (from >> priv->page_shift)) * mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt read beyond end "
                    "of device\n", __func__);
        return -EINVAL;
    }

    mutex_lock(&priv->lock);

    ret = sfc_alloc_l2buf(priv->c);
    if (ret) {
        mutex_unlock(&priv->lock);
        return ret;
    }

    if (HeYangTek_Flag) {
        oobrw_buf = (uint8_t *)kmalloc(priv->info->oob_size, GFP_KERNEL);
        if(!oobrw_buf){
            oobrw_buf = (uint8_t *)kmalloc(priv->info->oob_size, GFP_KERNEL);
            if(!oobrw_buf){
                sfc_trace(SFC_ERR, "allocate memory for pInit_info failed\n");
                ret = -ENOMEM;
                goto out;
            }
        }
        nand_oobpos_to_coord(priv, from, ops->ooboffs, &row, &column);

        memset(oobrw_buf, 0, priv->info->oob_size);
        ret = sfc_nand_read_page(priv, row, column, priv->info->oob_size, &rlen, oobrw_buf, NAND_READ_ECC_ENABLE);
        if(unlikely(ret)){
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        column += priv->info->oob_vail_data_offset;
        offset = 0;
        if(readlen > 4){
            memcpy(buf + offset, oobrw_buf + column, 4);
            column += 4 + priv->info->oob_down_skiplen;
            offset += 4;
            readlen -= 4;
            ops->oobretlen += 4;
            while(readlen > 0){
                xfer_len = (readlen > 8) ? 8 : readlen;
                memcpy(buf + offset, oobrw_buf + column, xfer_len);
                column += 8 + priv->info->oob_down_skiplen;
                offset += xfer_len;
                readlen -= xfer_len;
                ops->oobretlen += xfer_len;
            }
        }
        else if(readlen <= 4){
            memcpy(buf, oobrw_buf, readlen);
        }
    }else{
        oobrw_buf =(uint8_t *)kmalloc(NAND_OOB_LEN(priv->info), GFP_KERNEL);
        if (!oobrw_buf) {
            oobrw_buf =(uint8_t *)kmalloc(NAND_OOB_LEN(priv->info),
                GFP_KERNEL);
            if (!oobrw_buf) {
                printk("allocate memory for pInit_info failed\n");
                ret = -ENOMEM;
                goto out;
            }
        }
        nand_oobpos_to_coord(priv, from, ops->ooboffs, &row, &column);
        memset(oobrw_buf,0,NAND_OOB_LEN(priv->info));
        ret = sfc_nand_read_page(priv, row, column, NAND_OOB_LEN(priv->info),
                &rlen, oobrw_buf, NAND_READ_ECC_ENABLE);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        //column += priv->info->oob_up_skiplen;
        column += priv->info->oob_vail_data_offset;

        while(readlen > 0) {
            xfer_len = (readlen > priv->info->oob_seglen) ?
                            priv->info->oob_seglen : readlen;
            memcpy(buf + offset, oobrw_buf + column, xfer_len);
            column += (priv->info->oob_up_skiplen + priv->info->oob_seglen
                        + priv->info->oob_down_skiplen);

            readlen -= xfer_len;
            offset += xfer_len;
            ops->oobretlen += xfer_len;
        }
    }
out:
    sfc_free_l2buf(priv->c);
    mutex_unlock(&priv->lock);
    kfree(oobrw_buf);
    return ret;
} /*end of spinand_do_read_oob*/

static int sfc_nand_do_read_page(struct mtd_info *mtd, loff_t from, struct mtd_oob_ops *ops)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len,read_len,read_len_oob;
    u32 oob_size,oob_xfer_len,oob_add_len,oob_seglen_len;
    u32 offset = 0;
    u32 oob_offset = 0;
    u32 count = ops->len;
    int row, column;
    uint8_t *buf = NULL;
    uint8_t *oobbuf = ops->oobbuf;
    uint8_t *datbuf = ops->datbuf;
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);
    //u32 total_page = priv->info->page_size + priv->info->oob_size;
    uint8_t *r_buftmp = priv->buf;

    mutex_lock(&priv->lock);

    ret = sfc_alloc_l2buf(priv->c);
    if (ret) {
        mutex_unlock(&priv->lock);
        return ret;
    }

    /*decode row/column in address param*/
    row = ((from>>priv->page_shift) & 0xffffff);
    column = (from & 0x7ff);

    if (HeYangTek_Flag) {
        read_len_oob = ops->ooblen;
        oob_seglen_len = priv->info->oob_up_skiplen + priv->info->oob_seglen
            + priv->info->oob_down_skiplen;
        oob_size  = priv->info->oob_size;
        oob_xfer_len = priv->info->oob_seglen;

        while(count > 0) {
            xfer_len = (count > NAND_FLASH_BUF_SIZE) ? NAND_FLASH_BUF_SIZE : count;

            /*transfer len not greater than page size*/
            if(xfer_len > priv->info->page_size)
                xfer_len = ALIGN_DOWN(xfer_len, priv->info->page_size);
            if(xfer_len+column >= priv->info->page_size)
                xfer_len = priv->info->page_size - column;

            read_len = xfer_len + oob_size;

            ret = sfc_nand_read_page(priv, row, column, read_len,
                    &rlen, r_buftmp, NAND_READ_ECC_ENABLE);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }
            memcpy(datbuf + offset, r_buftmp, xfer_len);

            buf  = r_buftmp + rlen - oob_size + priv->info->oob_vail_data_offset;
            oob_add_len = 0;

            if(read_len_oob > 0){
                if(read_len_oob > 4){
                    memcpy(oobbuf, buf, 4);
                    read_len_oob -= 4;
                    oob_offset += 4;
                    oob_add_len += 4 + priv->info->oob_down_skiplen;
                    ops->oobretlen += 4;
                    while(read_len_oob){
                        oob_xfer_len = (read_len_oob > 8) ? 8 : read_len_oob;
                        memcpy(oobbuf + oob_offset, buf + oob_add_len, oob_xfer_len);
                        read_len_oob -= oob_xfer_len;
                        oob_offset += oob_xfer_len;
                        oob_add_len += 8 + priv->info->oob_down_skiplen;
                    }
                }else if(read_len_oob <= 4){
                    memcpy(oobbuf, buf, read_len_oob);
                }
            }else{
                oob_size = 0 ;
            }
            row++;
            column = 0;
            ops->retlen += (rlen - oob_size);
            count -= (rlen - oob_size);
            offset += (rlen-oob_size);
        }
    }else{
        read_len_oob = ops->ooblen;
        oob_seglen_len = priv->info->oob_up_skiplen + priv->info->oob_seglen
            + priv->info->oob_down_skiplen;
        oob_size  = priv->info->oob_size;
        oob_xfer_len = priv->info->oob_seglen;

        while(count > 0) {
            xfer_len = (count > NAND_FLASH_BUF_SIZE) ? NAND_FLASH_BUF_SIZE : count;

            /*transfer len not greater than page size*/
            if(xfer_len > priv->info->page_size)
                xfer_len = ALIGN_DOWN(xfer_len, priv->info->page_size);
            if(xfer_len+column >= priv->info->page_size)
                xfer_len = priv->info->page_size - column;

            read_len = xfer_len + oob_size;

            ret = sfc_nand_read_page(priv, row, column, read_len,
                    &rlen, r_buftmp, NAND_READ_ECC_ENABLE);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }
            memcpy(datbuf + offset, r_buftmp, xfer_len);

            buf  = r_buftmp + rlen - oob_size + priv->info->oob_vail_data_offset;
            oob_add_len = priv->info->oob_up_skiplen;

            if(read_len_oob > 0)
            {
                while(read_len_oob > 0)
                {
                    oob_xfer_len = (read_len_oob > priv->info->oob_seglen)
                        ? priv->info->oob_seglen : read_len_oob;
                    memcpy(oobbuf+oob_offset, buf+oob_add_len, oob_xfer_len);
                    oob_add_len += oob_seglen_len;
                    read_len_oob -= oob_xfer_len;
                    oob_offset += oob_xfer_len;
                    ops->oobretlen += oob_xfer_len;
                }
            }else{
                oob_size = 0 ;
            }
            row++;
            column = 0;
            ops->retlen += (rlen - oob_size);
            count -= (rlen - oob_size);
            offset += (rlen-oob_size);
        }
    }
out:
    sfc_free_l2buf(priv->c);
    mutex_unlock(&priv->lock);
    return ret;
} /*end of spinand_do_read_page*/

static int sfc_nand_read_oob(struct mtd_info *mtd, loff_t from,
             struct mtd_oob_ops *ops)
{
    int ret = -ENOTSUPP;
    sfc_trace(SFC_DBG, "%s: from:0x%llx\n", __func__, from);
    ops->retlen = 0;

    /* Do not allow reads past end of device */
    if (ops->datbuf && (from + ops->len) > mtd->size) {
        sfc_trace(SFC_ERR, "%s: Attempt read "
                "beyond end of device\n", __func__);
        return -EINVAL;
    }

    switch(ops->mode) {
    case MTD_OOB_PLACE:
    case MTD_OOB_AUTO:
    case MTD_OOB_RAW:
        break;

    default:
        goto out;
    }

    if (!ops->datbuf){
        ret = sfc_nand_do_read_oob(mtd, from, ops);
    } else {
        ret = sfc_nand_do_read_page(mtd, from, ops);
    }

 out:
    return ret;
}

static int sfc_nand_do_write_oob(struct mtd_info *mtd, loff_t to,
                struct mtd_oob_ops *ops)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len;
    u32 offset = 0;
    int row, column;
    int ofs = mtd->writesize;
    int writelen = ops->ooblen;
    uint8_t *buf = ops->oobbuf;
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);

    sfc_trace(SFC_DBG, "%s: to 0x%llx, len %d\n",
            __func__, to, writelen);

    if (unlikely(ops->ooboffs >= mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt to start read "
                    "outside oob\n", __func__);
        return -EINVAL;
    }

    /* Do not allow reads past end of device */
    if (unlikely(to >= mtd->size ||
             ops->ooboffs + writelen > ((mtd->size >> priv->page_shift) -
                    (to >> priv->page_shift)) * mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt write beyond end "
                    "of device\n", __func__);
        return -EINVAL;
    }

    mutex_lock(&priv->lock);

    ret = sfc_alloc_l2buf(priv->c);
    if (ret) {
        mutex_unlock(&priv->lock);
        return ret;
    }

    if (HeYangTek_Flag){
        ret = nand_oobpos_to_coord(priv, to, ops->ooboffs, &row, &column);

        column += priv->info->oob_vail_data_offset;
        if(writelen > 4){
            sfc_nand_write_page(priv, row, column, 4, &rlen, buf);
            column += 4 + priv->info->oob_down_skiplen;
            writelen -= rlen;
            offset += rlen;
            while(writelen > 0){
                xfer_len = (writelen > 8) ? 8 : writelen;
                sfc_nand_write_page(priv, row, column, xfer_len, &rlen, buf + offset);
                column += 8 + priv->info->oob_down_skiplen;
                writelen -= rlen;
                offset += rlen;
            }

        }else if(writelen <= 4){
            ret = sfc_nand_write_page(priv, row, column, writelen, &rlen, buf);
            if(unlikely(ret)){
                ret = -EBUSY;
                goto out;
            }
        }
    }else{
        ret = nand_oobpos_to_coord(priv, to, ops->ooboffs, &row, &column);
        //column += priv->info->oob_up_skiplen;
        column += priv->info->oob_vail_data_offset;
        while(writelen> 0) {
            xfer_len = adjust_xfer_len(priv, column, writelen);
            ret = sfc_nand_write_page(priv, row, column, xfer_len,
                        &rlen, buf + offset);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }

            column += (priv->info->oob_up_skiplen + rlen +
                        priv->info->oob_down_skiplen);
            if(column >= ofs + NAND_OOB_LEN(priv->info)) {
                column = ofs;
                row++;
            }
            writelen -= rlen;
            offset += rlen;
        }
    }
out:
    sfc_free_l2buf(priv->c);
    mutex_unlock(&priv->lock);
    return ret;
}

static int sfc_nand_do_write_page(struct mtd_info *mtd,
                    loff_t to, struct mtd_oob_ops *ops)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len,read_len_oob;
    u32 oob_size,oob_xfer_len,oob_seglen_len;// read_len,
    u16 spare_offset;
    u32 offset = 0;
    u32 oob_offset = 0;
    u32 oob_add_len = 0;
    u32 count = ops->len;
    int row, column;
    int row_oob, column_oob;
    uint8_t *p_buftmp_oob;
    u32 buftmp_len = 0;
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);
    //uint8_t *w_buftmp_oob =vmalloc(priv->info->oob_size);
    uint8_t *w_buftmp_oob;
    int i;
    //u32 total_page = priv->info->oob_size + priv->info->page_size;
    uint8_t *w_buftmp = priv->buf; // vmalloc(total_page);
    uint8_t *oobbuf = ops->oobbuf;
    uint8_t *datbuf = ops->datbuf;

    w_buftmp_oob = kmalloc(priv->info->oob_size, GFP_KERNEL);
    if (!w_buftmp_oob) {
        sfc_trace(SFC_DBG, "%s: alloc w_buftmp_oob failed!\n", __func__);
        return -ENOMEM;
    }

    mutex_lock(&priv->lock);

    ret = sfc_alloc_l2buf(priv->c);
    if (ret) {
        mutex_unlock(&priv->lock);
        return ret;
    }

    /*decode row/column in address param*/
    row = ((to>>priv->page_shift) & 0xffffff);
    column = (to & 0x7ff);

    if (HeYangTek_Flag){
        read_len_oob = ops->ooblen;
        oob_size = priv->info->oob_size;
        spare_offset = priv->info->oob_vail_data_offset;

        while(count > 0){
            xfer_len = (count > NAND_FLASH_BUF_SIZE) ? NAND_FLASH_BUF_SIZE : count;

            /*transfer len not greater than page size*/
            if(xfer_len > priv->info->page_size){
                xfer_len = ALIGN_DOWN(xfer_len, priv->info->page_size);
            }
            if(xfer_len + column >= priv->info->page_size){
                xfer_len = priv->info->page_size - column;
            }

            memcpy(w_buftmp, datbuf + offset, xfer_len);

            nand_oobpos_to_coord(priv, to, ops->ooboffs, &row_oob, &column_oob);
            ret = sfc_nand_read_page(priv, row_oob, column_oob, oob_size, &rlen, w_buftmp_oob, NAND_READ_ECC_ENABLE);

            p_buftmp_oob = w_buftmp_oob + spare_offset;
            if(read_len_oob > 4){
                memcpy(p_buftmp_oob, oobbuf, 4);
                read_len_oob -= 4;
                oob_add_len += 4 + priv->info->oob_down_skiplen;
                oob_offset += 4;
                // printk("line:%d:read_len_oob:%d, oob_add_len:%d, oob_offset:%d\r\n",
                //          __LINE__, read_len_oob, oob_add_len, oob_offset);
                while(read_len_oob > 0){
                    buftmp_len = (read_len_oob > 8) ? 8 : read_len_oob;
                    memcpy(p_buftmp_oob + oob_add_len, oobbuf + oob_offset, buftmp_len);
                    read_len_oob -= buftmp_len;
                    oob_add_len += 8 + priv->info->oob_down_skiplen;
                    oob_offset += buftmp_len;
                    // printk("line:%d:read_len_oob:%d, oob_add_len:%d, oob_offset:%d, buftmp_len:%d\r\n",
                    // __LINE__, read_len_oob, oob_add_len, oob_offset, buftmp_len);
                }

            }else if(read_len_oob <= 4){
                memcpy(p_buftmp_oob, oobbuf, read_len_oob);
            }
            memcpy(w_buftmp+xfer_len, w_buftmp_oob, oob_size);
            ret = sfc_nand_write_page(priv, row, column, xfer_len + oob_size, &rlen, w_buftmp);
            if(unlikely(ret)){
                ret = -EBUSY;
                goto out;
            }

            row++;
            column = 0;
            count -= xfer_len;
            offset += xfer_len;
        }
    }else{
        read_len_oob = ops->ooblen;
        oob_seglen_len = priv->info->oob_up_skiplen + priv->info->oob_seglen
                        + priv->info->oob_down_skiplen;
        oob_size  = priv->info->oob_size;
        oob_xfer_len = priv->info->oob_seglen;
        spare_offset = priv->info->oob_vail_data_offset;

        while(count > 0) {
            xfer_len = (count > NAND_FLASH_BUF_SIZE) ? NAND_FLASH_BUF_SIZE : count;

            /*transfer len not greater than page size*/
            if(xfer_len > priv->info->page_size)
                xfer_len = ALIGN_DOWN(xfer_len, priv->info->page_size);
            if(xfer_len+column >= priv->info->page_size)
                xfer_len = priv->info->page_size - column;

            memcpy(w_buftmp, datbuf+offset, xfer_len);

            nand_oobpos_to_coord(priv, to, ops->ooboffs, &row_oob, &column_oob);
            ret = sfc_nand_read_page(priv, row_oob, column_oob, oob_size,
                                &rlen, w_buftmp_oob, NAND_READ_ECC_ENABLE);

            sfc_trace(SFC_DBG, "count=%d, xfer_data_len=%d, xfer_oob_len=%d\n",
                        count, xfer_len, read_len_oob);
            sfc_trace(SFC_DBG, "row_oob=%d, column_oob=%d\n", row_oob, column_oob);

            for(i=0; i<4; i++)
                sfc_trace(SFC_DBG, "w_buftmp_oob[%d] = 0x%02x ", i, w_buftmp_oob[i]);
            sfc_trace(SFC_DBG, "\n");

            p_buftmp_oob = w_buftmp_oob + spare_offset; //offset to spare data

            while(read_len_oob> 0){
                buftmp_len = (read_len_oob > oob_xfer_len) ?
                            oob_xfer_len : read_len_oob;
                memcpy(p_buftmp_oob + oob_add_len, oobbuf+oob_offset, buftmp_len);
                read_len_oob -= buftmp_len;
                oob_offset += buftmp_len;
                oob_add_len +=oob_seglen_len;
            }
            memcpy(w_buftmp+xfer_len, w_buftmp_oob, oob_size);

            ret = sfc_nand_write_page(priv, row, column, xfer_len+oob_size,
                                &rlen, w_buftmp);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }

            row++;
            column = 0;
    //      *retlen += xfer_len;
            count -= xfer_len;
            offset += xfer_len;
        }
    }
out:
    sfc_free_l2buf(priv->c);
    mutex_unlock(&priv->lock);
    kfree(w_buftmp_oob);
    //vfree(w_buftmp);
    return ret;
} /*end of spinand_do_write_page*/

static int sfc_nand_write_oob(struct mtd_info *mtd, loff_t to,
             struct mtd_oob_ops *ops)
{
    int ret = -ENOTSUPP;
    //sfc_trace(SFC_DBG, "%s: to 0x%llx\n", __func__, to);
    ops->retlen = 0;

    /* Do not allow writes past end of device */
    if (ops->datbuf && (to + ops->len) > mtd->size) {
        sfc_trace(SFC_ERR, "%s: Attempt write beyond "
                "end of device\n", __func__);
        return -EINVAL;
    }

    switch(ops->mode) {
    case MTD_OOB_PLACE:
    case MTD_OOB_AUTO:
    case MTD_OOB_RAW:
        break;

    default:
        goto out;
    }

    if (!ops->datbuf)
        ret = sfc_nand_do_write_oob(mtd, to, ops);
    else
        ret = sfc_nand_do_write_page(mtd, to, ops);

 out:
    return ret;
}

static int sfc_nand_do_read_badflag(struct mtd_info *mtd, loff_t from,
                struct mtd_oob_ops *ops)
{
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);
    int ret = 0;
    size_t rlen = 0;
    /* u32 xfer_len; */
    /* u32 offset = 0; */
    int row, column;
    uint8_t *oobrw_buf;
    int readlen;
    uint8_t *buf = NULL;
    ops->oobretlen = 0;
    readlen = ops->ooblen;
    buf = ops->oobbuf;

    sfc_trace(SFC_DBG, "%s: from 0x%llx len %d\n",
            __func__, from, readlen);

    if (unlikely(ops->ooboffs >= mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt to start read "
                    "outside oob\n", __func__);
        return -EINVAL;
    }

    /* Do not allow reads past end of device */
    if (unlikely(from >= mtd->size ||
             ops->ooboffs + readlen > ((mtd->size >> priv->page_shift) -
                    (from >> priv->page_shift)) * mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt read beyond end "
                    "of device\n", __func__);
        return -EINVAL;
    }

    mutex_lock(&priv->lock);

    ret = sfc_alloc_l2buf(priv->c);
    if (ret) {
        mutex_unlock(&priv->lock);
        return ret;
    }

    if (HeYangTek_Flag) {
        oobrw_buf =(uint8_t *)kmalloc(priv->info->oob_size, GFP_KERNEL);
        if (!oobrw_buf) {
            oobrw_buf =(uint8_t *)kmalloc(priv->info->oob_size, GFP_KERNEL);
            if (!oobrw_buf) {
                printk("allocate memory for pInit_info failed\n");
                ret = -ENOMEM;
                goto out;
            }
        }
        nand_oobpos_to_coord(priv, from, ops->ooboffs, &row, &column);
        memset(oobrw_buf,0,priv->info->oob_size);
        ret = sfc_nand_read_page(priv, row, column, priv->info->oob_size, &rlen, oobrw_buf, NAND_READ_ECC_DISABLE);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        //column += priv->info->oob_up_skiplen;
        //column += priv->info->oob_vail_data_offset;
        memcpy(buf, oobrw_buf, 64);
    }else{
        oobrw_buf =(uint8_t *)kmalloc(NAND_OOB_LEN(priv->info), GFP_KERNEL);
        if (!oobrw_buf) {
            oobrw_buf =(uint8_t *)kmalloc(NAND_OOB_LEN(priv->info),
                    GFP_KERNEL);
            if (!oobrw_buf) {
                printk("allocate memory for pInit_info failed\n");
                ret =-ENOMEM;
                goto out;
            }
        }
        nand_oobpos_to_coord(priv, from, ops->ooboffs, &row, &column);
        memset(oobrw_buf,0,NAND_OOB_LEN(priv->info));
        ret = sfc_nand_read_page(priv, row, column, NAND_OOB_LEN(priv->info),
                &rlen, oobrw_buf, NAND_READ_ECC_DISABLE);
        if(unlikely(ret)) {
            ret = -EBUSY;
            goto out;
        }
        column = 0;
        //column += priv->info->oob_up_skiplen;
        //column += priv->info->oob_vail_data_offset;
        memcpy(buf, oobrw_buf, NAND_OOB_LEN(priv->info));
    }
out:
    sfc_free_l2buf(priv->c);
    mutex_unlock(&priv->lock);
    kfree(oobrw_buf);
    return ret;
}

static int sfc_nand_mtd_read_badflag(struct mtd_info *mtd,
            loff_t from, struct mtd_oob_ops *ops)
{
    int ret_code;
    ops->retlen = ops->oobretlen = 0;
    if (!mtd->_read_oob)
        return -EOPNOTSUPP;
    /*
     * In cases where ops->datbuf != NULL, mtd->_read_oob() has semantics
     * similar to mtd->_read(), returning a non-negative integer
     * representing max bitflips. In other cases, mtd->_read_oob() may
     * return -EUCLEAN. In all cases, perform similar logic to mtd_read().
     */
    /* ret_code = spinand_do_read_all_oob(mtd, from, ops); */
    ret_code = sfc_nand_do_read_badflag(mtd, from, ops);
    if (unlikely(ret_code < 0))
        return ret_code;
    if (mtd->ecc_strength == 0)
        return 0;   /* device lacks ecc */
    return ret_code >= mtd->bitflip_threshold ? -EUCLEAN : 0;
}

/**
 * ak_check_short_pattern - [GENERIC] check if a pattern is in the buffer
 * @buf: the buffer to search
 * @td: search pattern descriptor
 *
 * Check for a pattern at the given place. Used to search bad block tables and
 * good / bad block identifiers. Same as check_pattern, but no optional empty
 * check.
 */
static int sfc_nand_check_short_pattern(uint8_t *buf, struct nand_bbt_descr *td)
{
    /* Compare the pattern */
     /*printk("FILE:%s FUNCTION:%s LINE:%d buf + \
            td->offs:%x td->pattern:%x td->len:%d\n",
            __FILE__,__FUNCTION__,__LINE__,(buf + td->offs)[0],
            (td->pattern)[0], td->len); */
    if (memcmp(buf + td->offs, td->pattern, td->len))
        return -1;
    return 0;
}

/* Scan a given block partially */
static int sfc_nand_scan_block_fast(struct mtd_info *mtd,
    struct nand_bbt_descr *bd, loff_t offs, uint8_t *buf, int numpages)
{
    struct mtd_oob_ops ops;
    int j, ret;

    ops.ooblen = mtd->oobsize;
    ops.oobbuf = buf;
    ops.ooboffs = 0;
    ops.datbuf = NULL;
    ops.mode = MTD_OPS_PLACE_OOB;

    for (j = 0; j < numpages; j++) {
        /*
         * read oob data
         */
        ret = sfc_nand_mtd_read_badflag(mtd, offs, &ops);

        /* Ignore ECC errors when checking for BBM */
        if (ret && !mtd_is_bitflip_or_eccerr(ret))
            return ret;

        /* check the oob bad block flag */
        if (sfc_nand_check_short_pattern(buf, bd))
        {
            pr_err("%s:block %lld is a badblock",
                    __func__, offs >> (ffs(mtd->erasesize) - 1));
            return 1;
        }

        if(2 == numpages && (bd->options & NAND_BBT_FIRSTANDLAST))
        {
            /* scan first page and last page */
            offs += mtd->erasesize - (mtd->writesize * 1);
        }
        else
        {
            offs += mtd->writesize;   /* scan first 2 page or last 2 page */
        }
    }
    return 0;
}

static inline uint8_t sfc_nand_bbt_get_entry(struct nand_chip *chip, int block)
{
    uint8_t entry = chip->bbt[block >> BBT_ENTRY_SHIFT];
    entry >>= (block & BBT_ENTRY_MASK) * 2;
    return entry & BBT_ENTRY_MASK;
}

/**
 * ak_nand_isbad_bbt - [NAND Interface] Check if a block is bad
 * @mtd: MTD device structure
 * @offs: offset in the device
 * @allowbbt: allow access to bad block table region
 */
static int sfc_nand_isbad_bbt(struct mtd_info *mtd, loff_t offs,
    int allowbbt)
{
    struct nand_chip *this = mtd->priv;
    int block, res;

    block = (int)(offs >> this->bbt_erase_shift);
    /* 2bit per block, find the block from bbt */
    res = sfc_nand_bbt_get_entry(this, block);
    sfc_trace(SFC_DBG, "%s: bbt info for offs 0x%llx: (block %d) 0x%02x\n",
         __func__, offs, block, res);

    if(res)
        sfc_trace(SFC_INFO, "%s: block %d is a badblock", __func__, block);

    switch (res) {
    case BBT_BLOCK_GOOD:
        /* if res is 00B, it's a good block */
        return 0;
    case BBT_BLOCK_WORN:
        /* if res is 01B, it's a WORN block, WORN block is also a badlock.
            WORN block generate during operation(erase,write) */
        return 1;
    case BBT_BLOCK_RESERVED:
        /* if res is 10B, it's a RESERVED block.
            RESERVED block is use to save bbt.not use in mem_bbt */
        return allowbbt ? 0 : 1;
    }
    return 1;
}

/**
 * ak_nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 * @getchip: 0, if the chip is already selected
 * @allowbbt: 1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int sfc_nand_block_checkbad(struct mtd_info *mtd, loff_t ofs,
    int getchip, int allowbbt)
{
    struct nand_chip *chip = mtd->priv;
    int numpages = 0;
    loff_t from;

    /* if not bbt, direct to read badflag in oob */
    if (!chip->bbt)
    {
        if (chip->badblock_pattern->options & NAND_BBT_SCAN2NDPAGE)
        {
            numpages = 2;
        }
        else
        {
            numpages = 1;
        }

        /* Search first page and last page */
        if (chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST)
        {
            numpages = 2;
        }

        from = ofs;
        from = (from >> chip->bbt_erase_shift) << chip->bbt_erase_shift;
        if ((chip->bbt_options & NAND_BBT_SCANLASTPAGE)
            && !(chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST))
            from += mtd->erasesize - (mtd->writesize * numpages);

        return sfc_nand_scan_block_fast(mtd, chip->badblock_pattern, from,
                                    chip->buffers->databuf, numpages);
    }

    /* Return info from the table */
    return sfc_nand_isbad_bbt(mtd, ofs, allowbbt);
}

/**
 * ak_new_nand_block_bad - [anyka] Read bad block marker in anyka bbt
 * @mtd:    MTD device structure
 * @ofs:    offset from device start
 *
 * Check, if the block is bad.
 */
int sfc_nand_block_isbad(struct mtd_info *mtd, loff_t offs)
{
    sfc_trace(SFC_DBG, "%s: offs 0x%llx\n", __func__, offs);
    return sfc_nand_block_checkbad(mtd, offs, 1, 0);
}

static int sfc_nand_do_write_badflag(struct mtd_info *mtd, loff_t to,
                struct mtd_oob_ops *ops)
{
    int ret = 0;
    size_t rlen = 0;
    u32 xfer_len;
    u32 offset = 0;
    int row, column;
    int ofs = mtd->writesize;
    int writelen = ops->ooblen;
    uint8_t *buf = ops->oobbuf;
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);

    sfc_trace(SFC_DBG, "%s: from 0x%llx, len %d\n",
            __func__, to, writelen);

    if (unlikely(ops->ooboffs >= mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt to start read "
                    "outside oob\n", __func__);
        return -EINVAL;
    }

    /* Do not allow reads past end of device */
    if (unlikely(to >= mtd->size ||
             ops->ooboffs + writelen > ((mtd->size >> priv->page_shift) -
                    (to >> priv->page_shift)) * mtd->oobsize)) {
        sfc_trace(SFC_ERR, "%s: Attempt write beyond end "
                    "of device\n", __func__);
        return -EINVAL;
    }

    if (HeYangTek_Flag){
        ret = nand_oobpos_to_coord(priv, to, ops->ooboffs, &row, &column);

        while(writelen > 0){
                xfer_len = (writelen > 8) ? 8 : writelen;
                sfc_nand_write_page(priv, row, column, xfer_len, &rlen, buf + offset);
                column += 8 + priv->info->oob_down_skiplen;
                writelen -= rlen;
                offset += rlen;
        }
    }else{
        ret = nand_oobpos_to_coord(priv, to, ops->ooboffs, &row, &column);
        //column += priv->info->oob_up_skiplen;
        //column += priv->info->oob_vail_data_offset;
        while(writelen> 0) {
            xfer_len = adjust_xfer_len(priv, column, writelen);

            //printk("wr:to(%d)ofs(%d):%d,%d,%d,%p",
            //      (u32)to, ops->ooboffs, row, column, xfer_len, buf);
            ret = sfc_nand_write_page(priv, row, column, xfer_len,
                                &rlen, buf + offset);
            if(unlikely(ret)) {
                ret = -EBUSY;
                goto out;
            }

            column += (priv->info->oob_up_skiplen + rlen +
                        priv->info->oob_down_skiplen);
            if(column >= ofs + NAND_OOB_LEN(priv->info)) {
                column = ofs;
                row++;
            }
            writelen -= rlen;
            offset += rlen;
        }
    }
out:
    return ret;
}

/**
 * ak_nand_default_block_markbad - [DEFAULT] 
 *  mark a block bad via bad block marker
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This is the default implementation, which can be overridden by a hardware
 * specific driver. It provides the details for writing a bad block marker to a
 * block.
 */
static int sfc_nand_default_block_markbad(struct mtd_info *mtd,
    loff_t ofs)
{
    struct nand_chip *chip = mtd->priv;
    struct mtd_oob_ops ops;
    uint8_t buf[2] = { 0, 0 };
    int ret = 0, res, i = 0;

    memset(&ops, 0, sizeof(ops));
    ops.oobbuf = buf;
    ops.ooboffs = chip->badblockpos;
    /* ops.len = ops.ooblen = 1; */
    ops.len = ops.ooblen = chip->badblock_pattern->len;

    ops.mode = MTD_OPS_PLACE_OOB;

    /* Write to first/last page(s) if necessary */
    if (chip->bbt_options & NAND_BBT_SCANLASTPAGE)
        ofs += mtd->erasesize - mtd->writesize;
    do {
        /* res = spinand_do_write_all_oob(mtd, ofs, &ops); */
        res = sfc_nand_do_write_badflag(mtd, ofs, &ops);
        if (!ret)
            ret = res;

        i++;
        //ofs += mtd->writesize;

        if((chip->bbt_options & NAND_BBT_SCANLASTPAGE)
            && (chip->bbt_options & NAND_BBT_SCAN2NDPAGE))
        {
            ofs -= mtd->writesize;   /* mark last 2 page */
        }
        else if(chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST)
        {
            /* mark first and last page */
            ofs += mtd->erasesize - mtd->writesize;
        }
        else
        {
            ofs += mtd->writesize;   /* mark first two page */
        }

    } while (((chip->bbt_options & NAND_BBT_SCAN2NDPAGE)
            || (chip->badblock_pattern->options & NAND_BBT_FIRSTANDLAST))
            && i < 2);

    return ret;
}

static inline void sfc_nand_bbt_mark_entry(struct nand_chip *chip, int block,
        uint8_t mark)
{
    uint8_t msk = (mark & 0x03) << ((block & 0x03) * 2);
    chip->bbt[block >> 2] |= msk;
}

/**
 * ak_nand_markbad_bbt - [NAND Interface] Mark a block bad in the BBT
 * @mtd: MTD device structure
 * @offs: offset of the bad block
 */
static int sfc_nand_markbad_bbt(struct mtd_info *mtd, loff_t offs)
{
    struct nand_chip *this = mtd->priv;
    int block, ret = 0;

    block = (int)(offs >> this->bbt_erase_shift);

    /* Mark bad block in memory */
    /* if generate bad block durning operate(erase,write),mark 0x01 */
    sfc_nand_bbt_mark_entry(this, block, BBT_BLOCK_WORN);
    return ret;
}

/**
 * ak_nand_block_markbad_lowlevel - mark a block bad
 * @mtd: MTD device structure
 * @ofs: offset from device start
 *
 * This function performs the generic NAND bad block marking steps (i.e., bad
 * block table(s) and/or marker(s)). We only allow the hardware driver to
 * specify how to write bad block markers to OOB (chip->block_markbad).
 *
 * We try operations in the following order:
 *  (1) erase the affected block, to allow OOB marker to be written cleanly
 *  (2) write bad block marker to OOB area of affected block (unless flag
 *      NAND_BBT_NO_OOB_BBM is present)
 *  (3) update the BBT
 * Note that we retain the first error encountered in (2) or (3), finish the
 * procedures, and dump the error in the end.
*/
static int sfc_nand_block_markbad_lowlevel(struct mtd_info *mtd,
    loff_t ofs)
{
    struct nand_chip *chip = mtd->priv;
    int res, ret = 0;

    if (!(chip->bbt_options & NAND_BBT_NO_OOB_BBM)) {
        #if 1
        struct erase_info einfo;

        /* Attempt erase before marking OOB */
        memset(&einfo, 0, sizeof(einfo));
        einfo.mtd = mtd;
        einfo.addr = ofs;
        einfo.len = 1ULL << chip->phys_erase_shift;
        sfc_nand_erase(mtd, &einfo);   /* erase block */
        #endif

        /* Write bad block marker to OOB */
        ret = sfc_nand_default_block_markbad(mtd, ofs);

        if (!ret)
            sfc_trace(SFC_ERR, "%s:block %d has marked in oob",
                    __func__, (int)(ofs >> chip->bbt_erase_shift));
    }

    /* Mark block bad in BBT */
    if (chip->bbt) {
        res = sfc_nand_markbad_bbt(mtd, ofs);
        if (!ret)
            ret = res;

        if (!ret)
            sfc_trace(SFC_ERR, "%s:block %d has marked in bbt",
                    __func__, (int)(ofs >> chip->bbt_erase_shift));

    }

    if (!ret)
        mtd->ecc_stats.badblocks++;

    return ret;
}

int sfc_nand_block_markbad(struct mtd_info *mtd, loff_t offs)
{
    int ret;
    sfc_trace(SFC_DBG, "%s: offs 0x%llx\n", __func__, offs);
       /* check badblock before markblock */
    ret = sfc_nand_block_isbad(mtd, offs);
    if (ret) {
        /* If it was bad already, return success and do nothing */
        if (ret > 0)
            return 0;
        return ret;
    }

    return sfc_nand_block_markbad_lowlevel(mtd, offs);
}

/**
 * ak_create_bbt - [GENERIC] Create a bad block table by scanning the device
 * @mtd: MTD device structure
 * @buf: temporary buffer
 * @bd: descriptor for the good/bad block search pattern
 * @chip: create the table for a specific chip, -1 read all chips; applies only
 *        if NAND_BBT_PERCHIP option is set
 *
 * Create a bad block table by scanning the device for the given good/bad block
 * identify pattern.
 */
static int sfc_nand_create_bbt(struct mtd_info *mtd, uint8_t *buf,
    struct nand_bbt_descr *bd, int chip)
{
    struct nand_chip *this = mtd->priv;
    int i, numblocks, numpages;
    int startblock;
    loff_t from;

    pr_info("Scanning device for bad blocks\n");

    if (bd->options & NAND_BBT_SCAN2NDPAGE)
        numpages = 2;
    else
        numpages = 1;

    /* Search first page and last page */
    if (bd->options & NAND_BBT_FIRSTANDLAST)
    {
        numpages = 2;
    }

    if (chip == -1) {
        numblocks = mtd->size >> this->bbt_erase_shift;
        startblock = 0;
        from = 0;
    } else {
        if (chip >= this->numchips) {
            sfc_trace(SFC_ERR, "ak_create_bbt(): chipnr (%d) > available chips (%d)\n",
                   chip + 1, this->numchips);
            return -EINVAL;
        }
        numblocks = this->chipsize >> this->bbt_erase_shift;
        startblock = chip * numblocks;
        numblocks += startblock;
        from = (loff_t)startblock << this->bbt_erase_shift;
    }

    if ((this->bbt_options & NAND_BBT_SCANLASTPAGE) &&
            !(bd->options & NAND_BBT_FIRSTANDLAST))
        from += mtd->erasesize - (mtd->writesize * numpages);

    for (i = startblock; i < numblocks; i++) {
        int ret;
        BUG_ON(bd->options & NAND_BBT_NO_OOB);
        /* check the block is good or not */
        ret = sfc_nand_scan_block_fast(mtd, bd, from, buf, numpages);
        sfc_trace(SFC_DBG, "%s: ret %d\n", __func__, ret);
        if (ret < 0)
            return ret;

        if (ret) {
            /* factory bad block mark 0x3 in bbt */
            sfc_nand_bbt_mark_entry(this, i, BBT_BLOCK_FACTORY_BAD);
            sfc_trace(SFC_INFO, "Bad eraseblock %d at 0x%llx\n",
                i, from);
            mtd->ecc_stats.badblocks++;
        }

        from += (1 << this->bbt_erase_shift);
    }
    return 0;
}

/**
 * ak_nand_memory_bbt - [GENERIC] create a memory based bad block table
 * @mtd: MTD device structure
 * @bd: descriptor for the good/bad block search pattern
 *
 * The function creates a memory based bbt by scanning the device for
 * manufacturer / software marked good / bad blocks.
 */
static int sfc_nand_memory_bbt(struct mtd_info *mtd,
                            struct nand_bbt_descr *bd)
{
    struct nand_chip *this = mtd->priv;
    /* create bbt */
    return sfc_nand_create_bbt(mtd, this->buffers->databuf, bd, -1);
}

/**
 * ak_nand_default_bbt -
 * [NAND Interface] Select a default bad block table for the device
 * @mtd: MTD device structure
 *
 * This function selects the default bad block table support for the device and
 * calls the nand_scan_bbt function.
 */
int sfc_nand_default_bbt(struct mtd_info *mtd, struct nand_bbt_descr *bd)
{
    struct nand_chip *this = mtd->priv;
    int len, res;
    struct nand_bbt_descr *td = this->bbt_td;

    len = (mtd->size >> (this->bbt_erase_shift + 2)) ? : 1;
    /*
     * Allocate memory (2bit per block) and clear the memory bad block
     * table.
     */
    sfc_trace(SFC_DBG, "%s: len %d\n", __func__, len);
    this->bbt = kzalloc(len, GFP_KERNEL);
    if (!this->bbt)
        return -ENOMEM;

    /*
     * If no primary table decriptor is given, scan the device to build a
     * memory based bad block table.
     */
    if (!td) {
        sfc_trace(SFC_DBG, "%s use memory\n", __func__);
        if ((res = sfc_nand_memory_bbt(mtd, bd))) {
            sfc_trace(SFC_ERR, "nand_bbt: can't scan flash and build the RAM-based BBT\n");
            goto err;
        }
        return 0;
    }

err:
    kfree(this->bbt);
    this->bbt = NULL;
    return res;
}

int ak_set_chip(struct nand_chip *chip, struct ak_nand_flash_info *info)
{
    struct nand_bbt_descr *bd;
    struct nand_buffers *nbuf;

    if (!chip)
        return -ENOMEM;

    if (!info)
        return -ENODEV;

    chip->priv = info;
    /* if scan bbt in memory,chip->bbt_td need to be set NULL */
    chip->bbt_td = NULL;
    /* if scan bbt in memory,chip->bbt_md need to be set NULL */
    chip->bbt_md = NULL;
    /* scan bad flag in first page and last page */
    chip->bbt_options |= info->badflag_option;
    chip->bbt_erase_shift = ffs(info->block_size) - 1;
    chip->phys_erase_shift = chip->bbt_erase_shift;
    /* bad flag opsition in a page oob */
    chip->badblockpos = info->badflag_offs;

    /* bad block scan steup */
    bd = kzalloc(sizeof(*bd), GFP_KERNEL);
    if (!bd)
        return -ENOMEM;
    /* bad flag in nand oob */
    bd->options = chip->bbt_options & BADBLOCK_SCAN_MASK;
    bd->offs = chip->badblockpos;   /* bad flag opsition in a page oob */
    bd->len = info->badflag_len;   /* bad flag len in a page oob */
    bd->pattern = nand_scan_0xff_pattern;
    bd->options |= NAND_BBT_DYNAMICSTRUCT;
    chip->badblock_pattern = bd;
    sfc_trace(SFC_DBG, "%s: bd->options:%x, "
              "bd->offs:%d, bd->len:%d, bd->pattern:0x%x, "
              "chip->bbt_erase_shift:%d\n",
              __func__, bd->options,
              bd->offs,bd->len,bd->pattern[0],
              chip->bbt_erase_shift);
    sfc_trace(SFC_DBG, "chip->badblockpos:%d bd->offs:%d "
                "bd->len:%d chip->bbt_options:0x%x\n",
              chip->badblockpos,bd->offs,bd->len,chip->bbt_options);

    /* nand_buffers setting */
    nbuf = kzalloc(sizeof(*nbuf) + info->page_size
                + info->oob_size * 3, GFP_KERNEL);

    if (!nbuf) {
        kfree(bd);
        chip->badblock_pattern = NULL;
        return -ENOMEM;
    }
    nbuf->ecccalc = (uint8_t *)(nbuf + 1);
    nbuf->ecccode = nbuf->ecccalc + info->oob_size;
    nbuf->databuf = nbuf->ecccode + info->oob_size;

    chip->buffers = nbuf;

    return 0;
}

/**
* @brief    MTD get device ID
* 
* get the device ID of  the spi nand chip.
* @author lixinhai
* @date 2014-03-20
* @param[in] mtd     mtd info handle.
* @return int return device ID of  the spi nand chip.
*/
static int sfc_nand_get_devid(struct mtd_info *mtd)
{
    struct sfc_nand_priv *priv = mtd_to_nandpriv(mtd);
    //int         tmp;
    u8          id[5];
    u32         jedec;
    u32 reg_val;
    struct ak_sfc *c = priv->c;

    /* Wait until finished previous write command. */
    if (nand_wait_till_ready(priv))
        return -EBUSY;

    sfc_cmd_addr_dummy_data(
            0, // bank_id,
            SFC_DIR_READ, NAND_OPCODE_RDID, 1, // dir, opcode, opcode_bytes
            0, 0, 0, // addr, addr_bytes, addr_mode,
            0, 0, 8,// mode, mode_bytes, dummy_cycles,
            id, NAND_ID_MAX_LEN, 0, // buf, buf_len, data_mode,
            SFC_TRANS_TXD_RXD); // trans_mode

    reinit_completion(&c->trans_done);
    sfc_start_transfer();
    wait_for_completion(&c->trans_done);
    if (c->trans_errno) {
        sfc_trace(SFC_DBG, "%s: transfer failed with errno %d\n",
                __func__, c->trans_errno);
        return -EFAULT;
    }

    reg_val = __raw_readl(AK_VA_SFC + SFC_REG_DATA(0));
    memcpy(id, &reg_val, NAND_ID_MAX_LEN);

    jedec = id[0];
    jedec = jedec << 8;
    jedec |= id[1];
    jedec = jedec << 8;
    jedec |= id[2];
    jedec = jedec << 8;
    jedec |= id[3];

    sfc_trace(SFC_INFO, "spi nandflash ID: 0x%08x\n", jedec);
    return jedec;
}

static int sfc_nand_init_stat_reg(struct sfc_nand_priv *priv)
{
    struct ak_nand_flash_info *info = priv->info;
    u32 dataout;

    of_property_read_u32(info->child, "nandflash-b_wip", &dataout);
    info->b_wip = dataout;
    of_property_read_u32(info->child, "nandflash-b_qe", &dataout);
    info->b_qe = dataout;

    return 0;
}

/**
* @brief configure spi nandflash transfer mode according to flags. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @return int return config success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int sfc_nand_cfg_quad_mode(struct sfc_nand_priv *priv)
{
    int ret = 0;
    if((priv->bus_width & BUS_WIDTH_4WIRE) &&
        (priv->info->flags & (SFLAG_QUAD_WRITE|SFLAG_QUAD_IO_WRITE|
            SFLAG_DUAL_READ|SFLAG_DUAL_IO_READ))) {
        ret = nand_quad_mode_enable(priv);
        if(ret) {
            priv->bus_width &= ~BUS_WIDTH_4WIRE;
            sfc_trace(SFC_INFO, "config the nand flash quad enable failed. \
                        transfer use 1 wire.\n");
        }
    }
    else
        nand_quad_mode_disable(priv);

    return ret;
}

/**
* @brief initilize spi nand flash read/write param. 
* 
* @author lixinhai
* @date 2014-04-20
* @param[in] spiflash info handle.
* @return int return config success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int sfc_nand_init_rw_info(struct sfc_nand_priv *priv)
{
    /**default param.*/
    priv->rx_opcode = NAND_OPCODE_READ;
    priv->rxd_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
    priv->rxa_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
    priv->tx_opcode = NAND_OPCODE_PP;
    priv->txd_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
    priv->txa_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
    priv->rx_dummy = 1*8;

    if(priv->bus_width & BUS_WIDTH_2WIRE){
        if(priv->info->flags & SFLAG_DUAL_READ) {
            priv->rx_opcode = NAND_OPCODE_FAST_D_READ;
            priv->rxd_bus_width = BUS_WIDTH_2WIRE; //XFER_2DATAWIRE;
            priv->rxa_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
            priv->rx_dummy = 1*8;
        } else if (priv->info->flags & SFLAG_DUAL_IO_READ) {
            priv->rx_opcode = NAND_OPCODE_FAST_D_IO;
            priv->rxd_bus_width = BUS_WIDTH_2WIRE; //XFER_2DATAWIRE;
            priv->rxa_bus_width = BUS_WIDTH_2WIRE; //XFER_2DATAWIRE;
            priv->rx_dummy = 4;
        }

        if(priv->info->flags & SFLAG_DUAL_WRITE) {
            priv->tx_opcode = NAND_OPCODE_PP_DUAL;
            priv->txd_bus_width = BUS_WIDTH_2WIRE; //XFER_2DATAWIRE;
            priv->txa_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
        } else if(priv->info->flags & SFLAG_DUAL_IO_WRITE) {
            priv->tx_opcode = NAND_OPCODE_2IO_PP;
            priv->txd_bus_width = BUS_WIDTH_2WIRE; //XFER_2DATAWIRE;
            priv->txa_bus_width = BUS_WIDTH_2WIRE; //XFER_2DATAWIRE;
        }
    }

    if(priv->bus_width & BUS_WIDTH_4WIRE){
        if(priv->info->flags & SFLAG_QUAD_READ) {
            priv->rx_opcode = NAND_OPCODE_FAST_Q_READ;
            priv->rxd_bus_width = BUS_WIDTH_4WIRE; //XFER_4DATAWIRE;
            priv->rxa_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
            priv->rx_dummy = 1*8;
        }else if(priv->info->flags & SFLAG_QUAD_IO_READ){
            priv->rx_opcode = NAND_OPCODE_FAST_Q_IO;
            priv->rxd_bus_width = BUS_WIDTH_4WIRE; //XFER_4DATAWIRE;
            priv->rxa_bus_width = BUS_WIDTH_4WIRE; //XFER_4DATAWIRE;
            priv->rx_dummy = 4;
        }

        if(priv->info->flags & SFLAG_QUAD_WRITE) {
            priv->tx_opcode = NAND_OPCODE_PP_QUAD;
            priv->txd_bus_width = BUS_WIDTH_4WIRE; //XFER_4DATAWIRE;
            priv->txa_bus_width = BUS_WIDTH_1WIRE; //XFER_1DATAWIRE;
        }else if(priv->info->flags & SFLAG_QUAD_IO_WRITE) {
            priv->tx_opcode = NAND_OPCODE_4IO_PP;
            priv->txd_bus_width = BUS_WIDTH_4WIRE; //XFER_4DATAWIRE;
            priv->txa_bus_width = BUS_WIDTH_4WIRE; //XFER_4DATAWIRE;
        }

    }
    return 0;
}

int spinand_read_partition(unsigned long start_pos,
            unsigned long partition_size, unsigned char *buf,
            unsigned long buf_len)
{
    unsigned long read_len = buf_len;
    unsigned long write_idex = 0;
    unsigned long page_idex = 0;
    unsigned char *tmp_buf = NULL;
    unsigned long will_read_len = 0;
    unsigned long buf_idex = 0, b_idex = 0;
    struct mtd_info *mtd = &ak_nand_priv->mtd;
    unsigned long page_size = mtd->writesize;
    unsigned long erase_size = mtd->erasesize;
    loff_t offs = 0;
    size_t retlen = 0;

    while(1)
    {

        if(read_len == 0)
        {
            break;
        }
        if(page_idex*page_size > partition_size)
        {
            sfc_trace(SFC_ERR, "error: more than partition size, \
                    saved_size = %ld, write_size:%ld\n",
                    partition_size, page_idex*page_size);
            return -1;
        }

        offs = start_pos + b_idex*erase_size;
        if(sfc_nand_block_isbad(mtd, offs) == 1)
        {
            sfc_trace(SFC_ERR, "the block is bad block = %ld\n",
                    (start_pos + b_idex*erase_size)/erase_size);
            b_idex++;
            page_idex += (erase_size/page_size);
            continue;
        }

        tmp_buf = buf + write_idex*page_size;
        buf_idex = 0;
        while(1)
        {

            if(read_len > page_size)
            {
                will_read_len = page_size;
            }
            else
            {
                will_read_len = read_len;
            }

            if (sfc_nand_read(mtd, start_pos+page_idex*page_size,
                          will_read_len, &retlen, &tmp_buf[buf_idex]) != 0)
            {
                sfc_trace(SFC_ERR, "error: spinand_flash_read, pos:%ld\n",
                        start_pos+page_idex*page_size);
                return -1;
            }
            page_idex++;
            write_idex++;
            read_len -= will_read_len;
            buf_idex += will_read_len;

            if(read_len == 0)
            {
                break;
            }

            if((start_pos+page_idex*page_size)%erase_size == 0)
            {
                break;
            }

        }
        b_idex++;
    }
    return 0;
}


extern int parse_mtd_partitions(struct mtd_info *master,
                const char *const *types,struct mtd_partition **pparts,
                struct mtd_part_parser_data *data);

int spinand_get_partition_data(char *partition_name,
                char *data, unsigned long *date_len)
{
    int ret = 0, i = 0;
    struct mtd_partition *real_parts = NULL;
    struct mtd_partition *real_parts_saved = NULL;
    unsigned long partition_offset = 0;
    unsigned long partition_size = 0;
    struct mtd_info *mtd = &ak_nand_priv->mtd;

    if(data == NULL){
        sfc_trace(SFC_ERR, "data null\n");
        return -1;
    }

    if(partition_name == NULL){
        sfc_trace(SFC_ERR, "partition_name null\n");
        return -1;
    }

    ret = parse_mtd_partitions(mtd, NULL, &real_parts, NULL);
    /* Didn't come up with either parsed OR fallback partitions */
    if (ret <= 0) {
        sfc_trace(SFC_ERR, "failed to find partition %d\n",ret);
        /* Don't abort on errors; we can still use unpartitioned MTD */
        return -1;
    }
    real_parts_saved = real_parts;

    sfc_trace(SFC_ERR, "partition num:%d\n",ret);

    sfc_trace(SFC_ERR, "partition writesize:%d\n",mtd->writesize);
    sfc_trace(SFC_ERR, "partition erasesize:%d\n",mtd->erasesize);

    for(i = 0; i < ret; i++){
        if(strncmp(real_parts->name, partition_name,
                        strlen(partition_name)) == 0){

            partition_offset = real_parts->offset;
            partition_size = real_parts->size;
            break;
        }

        sfc_trace(SFC_INFO, "real_parts->name: %s\n",real_parts->name);
        sfc_trace(SFC_INFO, "real_parts->offset: %x\n",(uint32_t)real_parts->offset);
        sfc_trace(SFC_INFO, "real_parts->size: %x\n",(uint32_t)real_parts->size);
        real_parts++;
    }

    if(i == ret){
        sfc_trace(SFC_ERR, "no find partition %s\n",partition_name);
        return -1;
    }
    // avoid write too much
    if ((*date_len > 0) && (partition_size > *date_len)) {
        //sfc_trace(SFC_ERR, "partition_size %ld date_len %ld\n",
        //partition_size, *date_len);
        partition_size = *date_len;
    }

    if(spinand_read_partition(partition_offset, partition_size,
                                data, partition_size) == -1){
        sfc_trace(SFC_ERR, "read partition %s data fail\n",partition_name);
        return -1;
    }

    *date_len = partition_size;

    kfree(real_parts_saved);
    return 0;
}

/**
*
*@brief: /sys/devices/platform/soc/21100000.spi0/spi_master/spi0/spi0.1/ak_spiflash_id
*@param[in] struct device *dev
*@param[in] struct device_attribute *attr
*@param[in] char *buf
*@return: ssize_t
*
**/
static ssize_t nandflash_id_show(struct device *dev,
                struct device_attribute *attr,
                char *buf)
{
    struct ak_sfc *c = dev_get_drvdata(dev);
    struct sfc_nand_priv *priv = c->nand;

    if (c->nand) {
        sprintf(buf, "%x\n", priv->info->jedec_id);
        return strlen(buf);
    } else {
        return 0;
    }
}

static DEVICE_ATTR_RO(nandflash_id);

/**
 * Get spi flash device information and register it as a mtd device.
 */
static int sfc_nand_register(struct device_node *np, struct ak_sfc *c)
{
    struct device *dev = c->dev;
    struct sfc_nand_priv *priv;
    struct ak_nand_flash_info *info;
    struct mtd_info *mtd;
    struct nand_chip *chip;
    int ret, i;

    priv = devm_kzalloc(dev, sizeof(*priv), GFP_KERNEL);
    if (!priv) {
        sfc_trace(SFC_ERR, "%s no memory!\n", __func__);
        return -ENOMEM;
    }

    ret = of_property_read_u32(np, "reg", &priv->chipselect);
    if (ret) {
        sfc_trace(SFC_ERR, "no reg property for %s\n", np->full_name);
        return -ENODEV;
    }

    ret = of_property_read_u32(np, "bus-width", &priv->bus_width);
    if (ret) {
        sfc_trace(SFC_ERR, "could not read bus-width property for %s\n",
                np->full_name);
        return -ENODEV;
    }

    priv->c = c;
    priv->np = np;
    mtd = &priv->mtd;
    mutex_init(&priv->lock);
    ak_nand_priv = priv;
    c->nand = priv;

    sfc_set_spi_baudrate(c, c->freq);

    sfc_trace(SFC_INFO, "%s: sfc frequency is %ld\n",
            __func__, sfc_get_spi_baudrate(c));

    info = get_nand_flash_info(priv);
    if (!info)
        return -ENODEV;

    chip = devm_kzalloc(dev, sizeof(struct nand_chip), GFP_KERNEL);
    if (!chip) {
        sfc_trace(SFC_ERR, "alloc nand_chip failed!\n");
        return -ENOMEM;
    }

    ret = ak_set_chip(chip, priv->info);    /* use flash_info to setup nand_chip */
    if (ret != 0)
        return ret;

    nand_flash_reset(priv);

    mtd->name = SFC_NAND_MTD_NAME;
    priv->mtd.type = MTD_NANDFLASH;
    priv->mtd.writesize = info->page_size;
    priv->mtd.writebufsize = info->page_size;
    priv->mtd.flags = MTD_WRITEABLE;
    priv->mtd.size = info->block_size * info->n_blocks;
    priv->mtd._erase = sfc_nand_erase;
    priv->mtd._write = sfc_nand_write;
    priv->mtd._read = sfc_nand_read;
    priv->mtd._read_oob = sfc_nand_read_oob;
    priv->mtd._write_oob = sfc_nand_write_oob;

    priv->mtd._block_isbad = sfc_nand_block_isbad;
    priv->mtd._block_markbad = sfc_nand_block_markbad;
    priv->mtd.get_device_id = sfc_nand_get_devid;
    priv->erase_opcode = NAND_OPCODE_ERASE_BLOCK;
    priv->mtd.erasesize = info->block_size;
    priv->mtd.oobsize = info->oob_size;
    priv->mtd.oobavail = info->oob_seglen * info->oob_seg_perpage;
    priv->mtd.priv = chip;

    sfc_trace(SFC_INFO, "priv->bus_width:%d(note:1:1 wire, "
            "2:2 wire, 4:4 wire)\n", priv->bus_width);

    priv->page_shift = ffs(priv->mtd.writesize)-1;
//#ifdef SPINAND_USE_MTD_BLOCK_LAYER
#if 1
    /*pre-allocation buffer use for spi nand data transfer.*/
    priv->buf = kzalloc(NAND_FLASH_BUF_SIZE + info->oob_size, GFP_KERNEL);
    if (!priv->buf) {
        sfc_trace(SFC_ERR, "Allocate buf for spi page failed\n");
        ret = -ENOMEM;
        goto err_priv_buf;
    }
#endif

    sfc_nand_init_stat_reg(priv);
    sfc_nand_cfg_quad_mode(priv);
    sfc_nand_init_rw_info(priv);
    if (HeYangTek_Flag)
    {
        //YHY_Set_Meta_All_Protect(priv);
    }

    priv->mtd.dev.parent = dev;

    sfc_trace(SFC_INFO, "%s (%lld Kbytes)\n", info->name,
            (long long)priv->mtd.size >> 10);

    sfc_trace(SFC_INFO,
        "mtd .name = %s, .size = 0x%llx (%lldMiB) "
            ".erasesize = 0x%.8x (%uKiB) .numeraseregions = %d\n",
        priv->mtd.name,
        (long long)priv->mtd.size, (long long)(priv->mtd.size >> 20),
        priv->mtd.erasesize, priv->mtd.erasesize / 1024,
        priv->mtd.numeraseregions);

    if (priv->mtd.numeraseregions)
        for (i = 0; i < priv->mtd.numeraseregions; i++)
            sfc_trace(SFC_INFO,
                "mtd.eraseregions[%d] = { .offset = 0x%llx, "
                ".erasesize = 0x%.8x (%uKiB), "
                ".numblocks = %d }\n",
                i, (long long)priv->mtd.eraseregions[i].offset,
                priv->mtd.eraseregions[i].erasesize,
                priv->mtd.eraseregions[i].erasesize / 1024,
                priv->mtd.eraseregions[i].numblocks);

#if 0
    {
        struct erase_info einfo;
        struct mtd_oob_ops bops;
        static u8 buf[2048+128] = {0};
        int i;
        int retlen;

        sfc_trace(SFC_DBG, "test erase/write/read\n");

        sfc_trace(SFC_DBG, "[0xa0] 0x%x [0xb0] 0x%x [0xc0] 0x%x [0xd0] 0x%x [0xf0] 0x%x\n",
                nand_read_sr(priv, 0xa0),
                nand_read_sr(priv, 0xb0),
                nand_read_sr(priv, 0xc0),
                nand_read_sr(priv, 0xd0),
                nand_read_sr(priv, 0xf0));

        memset(&einfo, 0, sizeof(einfo));
        einfo.mtd = mtd;
        einfo.addr = 0;
        einfo.len = 1ULL << chip->phys_erase_shift;
        sfc_nand_erase(mtd, &einfo);   /* erase block */

        memset(buf, sizeof(buf), 0);
        sfc_alloc_l2buf(priv->c);
        sfc_nand_read_page(priv, 0, 0, 4, NULL, buf, 1);
        sfc_free_l2buf(priv->c);
        for (i = 0; i < 4; i++) {
            if (buf[i] != 0xff)
                sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
        }

        memset(buf, sizeof(buf), 0);
        sfc_alloc_l2buf(priv->c);
        sfc_nand_read_page(priv, 0, 0, 2048+64, &retlen, buf, 1);
        sfc_free_l2buf(priv->c);
        for (i = 0; i < 2048+64; i++) {
            if (buf[i] != 0xff)
                sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
        }

        memset(buf, sizeof(buf), 0);
        sfc_nand_read(mtd, 0, 2048, &retlen, buf);
        for (i = 0; i < 2048; i++) {
            if (buf[i] != 0xff)
                sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
        }

        memset(buf, sizeof(buf), 0);
        bops.mode = MTD_OOB_PLACE;
        bops.len = 2048;
        bops.ooboffs = 0;
        bops.datbuf = buf;
        bops.oobbuf = buf + 2048;
        bops.ooblen = 64-4;
        sfc_nand_read_oob(mtd, 0, &bops);
        for (i = 0; i < 2048+64-4; i++) {
            if (buf[i] != 0xff)
                sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
        }

        memset(buf, sizeof(buf), 0);
        bops.mode = MTD_OOB_PLACE;
        bops.len = 0;
        bops.ooboffs = 0;
        bops.datbuf = NULL;
        bops.oobbuf = buf;
        bops.ooblen = 64-4;
        sfc_nand_read_oob(mtd, 0, &bops);
        for (i = 0; i < 64-4; i++) {
            if (buf[i] != 0xff)
                sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
        }

        for (i = 0; i < 2048; i++)
            buf[i] = i & 0xff;
        sfc_nand_write(mtd, 0, 2048, NULL, buf);
        memset(buf, sizeof(buf), 0);
        sfc_nand_read(mtd, 0, 2048, NULL, buf);
        for (i = 0; i < 2048; i++) {
            if (buf[i] != (i & 0xff))
                sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
        }

        bops.mode = MTD_OOB_PLACE;
        bops.len = 0;
        bops.ooboffs = 0;
        bops.datbuf = NULL;
        bops.oobbuf = buf;
        bops.ooblen = 16;
        for (i = 0; i < 16; i++) {
            buf[i] = i & 0xff;
        }
        sfc_nand_write_oob(mtd, 0, &bops);

        for (i = 0; i < 2048; i++)
            buf[i] = i & 0xff;
        sfc_nand_write(mtd, 2048, 2048, NULL, buf);
        memset(buf, sizeof(buf), 0);
        sfc_nand_read(mtd, 2048, 2048, NULL, buf);
        for (i = 0; i < 2048; i++) {
            if (buf[i] != (i & 0xff))
                sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
        }

        memset(buf, sizeof(buf), 0);
        sfc_alloc_l2buf(priv->c);
        sfc_nand_read_page(priv, 0, 0, 2048/*2048+64*/, &retlen, buf, 1);
        sfc_free_l2buf(priv->c);
        for (i = 0; i < 2048/*2048+64*/; i++) {
            if (i < 2048) {
                if (buf[i] != (i & 0xff))
                    sfc_trace(SFC_DBG, "error at %d(0x%x)\n", i, buf[i]);
            } else
                sfc_trace(SFC_DBG, "0x%x ", buf[i]);
        }

#if 0
        memset(buf, sizeof(buf), 0);
        bops.mode = MTD_OOB_PLACE;
        bops.len = 0;
        bops.ooboffs = 0;
        bops.datbuf = NULL;
        bops.oobbuf = buf;
        bops.ooblen = 64-4;
        sfc_nand_read_oob(mtd, 0, &bops);
        for (i = 0; i < 64-4; i++) {
            sfc_trace(SFC_DBG, "0x%x ", buf[i]);
        }
#endif

        sfc_trace(SFC_DBG, "test erase/write/read end\n");
    }
#endif
    /* partitions should match sector boundaries; and it may be good to
     * use readonly partitions for writeprotected sectors (BP2..BP0).
     */
#ifdef CONFIG_MTD_CMDLINE_PARTS
    /* badblock scan and bbt creat in mem */
    if (0x1150115 != priv->info->jedec_id)  //GSS01GSAM0 must always enable ECC
    {
        sfc_nand_disable_ecc(priv);
    }
    ret = sfc_nand_default_bbt(&priv->mtd, chip->badblock_pattern);
    if (0x1150115 != priv->info->jedec_id)  //GSS01GSAM0 must always enable ECC
    {
        sfc_nand_enable_ecc(priv);
    }

    if(ret)
    {
        sfc_trace(SFC_ERR, "scan bbt failed in memory\n");
        goto scan_bbt_failed;
    }
#endif

    ret = mtd_device_parse_register(&priv->mtd, NULL, NULL, NULL, 0);
    if (ret) {
        sfc_trace(SFC_ERR, "Add root MTD device failed\n");
        goto err_mtd_register;
    }

    ret = sysfs_create_file(&dev->kobj, &dev_attr_nandflash_id.attr);
    if (ret) {
        sfc_trace(SFC_ERR, "%s: create nandflash_id sysfs file failed!\n",
                    __func__);
    }

    ret = sysfs_create_file(&dev->kobj, &dev_attr_sfc_trace_level.attr);
    if (ret) {
        sfc_trace(SFC_ERR, "create sfc_trace_level sysfs file failed!\n");
    }

    sfc_trace(SFC_INFO, "Init AK SPI NAND Flash finish\n");
    return 0;

err_mtd_register:
#ifdef CONFIG_MTD_CMDLINE_PARTS
scan_bbt_failed:
#endif
    if (priv->buf)
        kfree(priv->buf);
err_priv_buf:
    if (chip->badblock_pattern) {
        kfree(chip->badblock_pattern);
        chip->badblock_pattern = NULL;
    }
    if (chip->buffers) {
        kfree(chip->buffers);
        chip->buffers = NULL;
    }
    return -ENOMEM;
}

static void sfc_nand_unregister_all(struct ak_sfc *c)
{
    struct device *dev = c->dev;
    struct sfc_nand_priv *priv = c->nand;
    struct nand_chip *chip;

    if (!c->nand)
        return;

    chip = priv->mtd.priv;
    sysfs_remove_file(&dev->kobj, &dev_attr_nandflash_id.attr);
    sysfs_remove_file(&dev->kobj, &dev_attr_sfc_trace_level.attr);
    if (priv->buf) {
        kfree(priv->buf);
        priv->buf = NULL;
    }
    if (chip->badblock_pattern) {
        kfree(chip->badblock_pattern);
        chip->badblock_pattern = NULL;
    }
    if (chip->buffers) {
        kfree(chip->buffers);
        chip->buffers = NULL;
    }
    mtd_device_unregister(&c->nand->mtd);
}

static int sfc_register_all(struct ak_sfc *c)
{
    struct device *dev = c->dev;
    struct device_node *np;
    int ret = 0;
    const char *compat = NULL;

    for_each_available_child_of_node(dev->of_node, np) {
        of_property_read_string(np, "compatible", &compat);
        sfc_trace(SFC_DBG, "%s: compatible string is %s\n", __func__, compat);

        if (of_device_is_compatible(np, "anyka,ak-spiflash")) {
            sfc_trace(SFC_DBG, "%s: ak-spiflash found\n", __func__);
            ret = sfc_nor_register(np, c);
            if (ret)
                sfc_nor_unregister_all(c);
            break;
        }
        if (of_device_is_compatible(np, "anyka,ak-spinand")) {
            sfc_trace(SFC_DBG, "%s: ak-spinand found\n", __func__);
            ret = sfc_nand_register(np, c);
            if (ret)
                sfc_nand_unregister_all(c);
            break;
        }
    }

    return ret;
}

#ifdef CONFIG_SYS_FAST_LAUNCH

extern int parse_mtd_partitions(struct mtd_info *master,
                const char *const *types,struct mtd_partition **pparts,
                        struct mtd_part_parser_data *data);

static int sfc_nor_get_partition_data(char *partition_name,
        unsigned long offset, char *data, unsigned long *date_len, int op_dir)
{
    int ret = 0, i = 0;
    struct mtd_partition *real_parts = NULL;
    struct mtd_partition *real_parts_saved = NULL;

    unsigned long partition_offset = 0;
    unsigned long partition_size = 0;
    size_t retlen = 0;

    if(data == NULL){
        pr_err("data null\n");
        return -1;
    }

    if(partition_name == NULL){
        pr_err("partition_name null\n");
        return -1;
    }
    //pr_err("partition name:%s\n",ak_spi_nor->mtd.name);
    sfc_trace(SFC_DBG, "partition name:%s\n",ak_spi_nor->mtd.name);
    sfc_trace(SFC_DBG, "partition writesize:%d\n",ak_spi_nor->mtd.writesize);
    sfc_trace(SFC_DBG, "partition erasesize:%d\n",ak_spi_nor->mtd.erasesize);
    sfc_trace(SFC_DBG, "%s partition_name %s op_dir %d offset %lx\n", __func__, partition_name, op_dir, offset);

    ret = parse_mtd_partitions(&ak_spi_nor->mtd, NULL, &real_parts, NULL);
    /* Didn't come up with either parsed OR fallback partitions */
    if (ret <= 0) {
        pr_err("failed to find partition %d\n",ret);
        /* Don't abort on errors; we can still use unpartitioned MTD */
        return -1;
    }

    real_parts_saved = real_parts;

    for(i = 0; i < ret; i++){
        sfc_trace(SFC_DBG,"real_parts->name: %s\n",real_parts->name);
        sfc_trace(SFC_DBG,"real_parts->offset: %x\n",(uint32_t)real_parts->offset);
        sfc_trace(SFC_DBG,"real_parts->size: %x\n",(uint32_t)real_parts->size);
        if(strncmp(real_parts->name, partition_name,
                    strlen(partition_name)) == 0){

            partition_offset = real_parts->offset;
            partition_size = real_parts->size;
            break;
        }
        real_parts++;
    }

    if(i == ret){
        pr_err("no find partition %s\n",partition_name);
        return -1;
    }
    // avoid write too much
    if ((*date_len > 0) && ((partition_size + offset) > *date_len)) { // avoid write too much
        pr_err("offset=%lu,data_len=%lu,op:%d\n", offset, *date_len,op_dir);
        partition_size = *date_len;
    }

    if(op_dir==1) {
            struct erase_info einfo;
            memset(&einfo, 0, sizeof(struct erase_info));
            einfo.mtd = &ak_spi_nor->mtd;
            einfo.addr = partition_offset+offset;
            einfo.len = partition_size;
            sfc_nor_erase(&ak_spi_nor->mtd, &einfo);
            pr_err("erase 0x%lx [%lu] ok\n",partition_offset+offset,partition_size);
            sfc_nor_write(&ak_spi_nor->mtd, partition_offset+offset, partition_size, &retlen, data);
    } else {
        sfc_nor_read(&ak_spi_nor->mtd, partition_offset+offset, partition_size, &retlen, data);
    }
	*date_len = retlen;

    kfree(real_parts_saved);
    return 0;
}

int get_partition_data(char *partition_name, unsigned long offset, char *data, unsigned long *date_len, int op_dir)
{
    int ret = 0;

    if (g_spiflash_flag) {
        if(op_dir==0)
            g_spiflash_flag |= 0x10;
        else if( (g_spiflash_flag&0x10) ) //reading can't to write
        {
            printk("reading can't to write!\n");
            return -1;
        }
        ret = sfc_nor_get_partition_data(partition_name, offset, data, date_len,op_dir);
        g_spiflash_flag &= 0xf;
    }
    return ret;
}
EXPORT_SYMBOL(get_partition_data);

int is_fastfs(void)
{
   return ((g_spiflash_flag>>4)&0xf);
}
EXPORT_SYMBOL(is_fastfs);
#endif

static void sfc_reset_hw(struct ak_sfc *c)
{
    u32 reg;

    reg = __raw_readl(AK_VA_SYSCTRL + 0x20);
    reg |= ((1 << 31) | (1 << 5));
    __raw_writel(reg, AK_VA_SYSCTRL + 0x20);
    reg = __raw_readl(AK_VA_SYSCTRL);
    udelay(1);
    reg = __raw_readl(AK_VA_SYSCTRL + 0x20);
    reg &= ~((1 << 31) | (1 << 5));
    __raw_writel(reg, AK_VA_SYSCTRL + 0x20);
    reg = __raw_readl(AK_VA_SYSCTRL);

    reg = __raw_readl(AK_VA_SFC + SFC_REG_INT_MASK);
    //reg |= (STA_SFC_TRANS_DONE_INT_EN | STA_SPI_ABORT_HAPPEN_INT
    //    | STA_RXFIFOCNT_FULL_INT_EN | STA_TXFIFO_EMPTY_INT_EN
    //    | STA_XIP_ADDR_ERR_INT_EN | STA_XIP_MODE_ERR_INT_EN);
    reg |= STA_SFC_TRANS_DONE_INT_EN;
    __raw_writel(reg, AK_VA_SFC + SFC_REG_INT_MASK);

    reg = __raw_readl(AK_VA_SFC + SFC_REG_CS_TIMING);
    reg = (9 << 4) | (9 << 0);
    __raw_writel(reg, AK_VA_SFC + SFC_REG_CS_TIMING);

    reg = __raw_readl(AK_VA_SFC + SFC_REG_RXDS_CFG);
    reg = (1 << 28) | (0 << 0);
    __raw_writel(reg, AK_VA_SFC + SFC_REG_RXDS_CFG);

    sfc_set_spi_baudrate(c, c->freq);
}

static int ak_sfc_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct ak_sfc *c;
    u32 reg;
    int ret;

    sfc_trace(SFC_DBG, "%s enter\n", __func__);

    c = devm_kzalloc(dev, sizeof(*c), GFP_KERNEL);
    if (!c) {
        sfc_trace(SFC_ERR, "can not alloc memory!\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, c);
    c->dev = dev;
    c->regbase = AK_VA_SFC;

    c->gclk = devm_clk_get(dev, "spi0_gclk");
    if (IS_ERR_OR_NULL(c->gclk)) {
        sfc_trace(SFC_ERR,
                "can not get gclk, return %ld!\n", PTR_ERR(c->gclk));
        return -ENODEV;
    }

    c->clk = devm_clk_get(dev, "sfc_clk");
    if (IS_ERR_OR_NULL(c->clk)) {
        sfc_trace(SFC_ERR,
                "can not get clk, return %ld!\n", PTR_ERR(c->clk));
        return -ENODEV;
    }

    c->irq = platform_get_irq(pdev, 0);
    if(c->irq <= 0) {
        sfc_trace(SFC_ERR, "failed to get interrupt resouce\n");
        return -ENODEV;
    }

    c->rstc = devm_reset_control_get(dev, "sfc");
    if (IS_ERR_OR_NULL(c->rstc)) {
        sfc_trace(SFC_ERR, "No reset controller specified\n");
        return -ENODEV;
    }

    c->pctrl = devm_pinctrl_get_select(dev, "default");
    if (IS_ERR_OR_NULL(c->pctrl)) {
        sfc_trace(SFC_ERR, "select pinctrl default failed\n");
        return -ENODEV;
    }

    c->clk_rate = 100000000UL; // sfc port clk
    //c->clk_rate = 60000000UL; // sfc port clk, debug
    clk_set_rate(c->clk, c->clk_rate);
    clk_prepare_enable(c->gclk);

    ret = of_property_read_u32(dev->of_node, "sfc-frequency", &c->freq);
    if (ret)
        c->freq = clk_get_rate(c->clk);

    /*
    ret = reset_control_assert(c->rstc);
    if (ret) {
        sfc_trace(SFC_ERR, "unable to reset_control_assert\n");
        return -ENODEV;
    }
    udelay(1);
    ret = reset_control_deassert(c->rstc);
    if (ret) {
        sfc_trace(SFC_ERR, "unable to reset_control_deassert\n");
        return -ENODEV;
    }
    */
    reg = __raw_readl(AK_VA_SYSCTRL + 0x20);
    reg |= ((1 << 31) | (1 << 5));
    __raw_writel(reg, AK_VA_SYSCTRL + 0x20);
    reg = __raw_readl(AK_VA_SYSCTRL);
    udelay(1);
    reg = __raw_readl(AK_VA_SYSCTRL + 0x20);
    reg &= ~((1 << 31) | (1 << 5));
    __raw_writel(reg, AK_VA_SYSCTRL + 0x20);
    reg = __raw_readl(AK_VA_SYSCTRL);

    sfc_trace(SFC_DBG, "[0x08000008] 0x%x [0x0800000c] 0x%x [0x0800001c] 0x%x clk_rate %ld\n",
            __raw_readl(AK_VA_SYSCTRL + 0x8),
            __raw_readl(AK_VA_SYSCTRL + 0xc),
            __raw_readl(AK_VA_SYSCTRL + 0x1c),
            clk_get_rate(c->clk));

    reg = __raw_readl(AK_VA_SFC + SFC_REG_STATUS);
    __raw_writel(reg, AK_VA_SFC + SFC_REG_STATUS);

    if (devm_request_irq(dev, c->irq, sfc_irq_handler, 0, "sfc", c)) {
        sfc_trace(SFC_ERR, "failed to request sfc interrupt\n");
        return  -ENOENT;
    }

    init_completion(&c->trans_done);
    c->l2buf_id = BUF_NULL;

    reg = __raw_readl(AK_VA_SFC + SFC_REG_INT_MASK);
    //reg |= (STA_SFC_TRANS_DONE_INT_EN | STA_SPI_ABORT_HAPPEN_INT
    //    | STA_RXFIFOCNT_FULL_INT_EN | STA_TXFIFO_EMPTY_INT_EN
    //    | STA_XIP_ADDR_ERR_INT_EN | STA_XIP_MODE_ERR_INT_EN);
    reg |= STA_SFC_TRANS_DONE_INT_EN;
    __raw_writel(reg, AK_VA_SFC + SFC_REG_INT_MASK);

    reg = __raw_readl(AK_VA_SFC + SFC_REG_CS_TIMING);
    reg = (9 << 4) | (9 << 0);
    __raw_writel(reg, AK_VA_SFC + SFC_REG_CS_TIMING);

    reg = __raw_readl(AK_VA_SFC + SFC_REG_RXDS_CFG);
    reg = (1 << 28) | (0 << 0);
    __raw_writel(reg, AK_VA_SFC + SFC_REG_RXDS_CFG);

    sfc_fifo_clear();
    sfc_fifo_clear();

    ret = sfc_register_all(c);
    if (ret) {
        sfc_trace(SFC_ERR, "sfc_register_all failed! ret:%d\n", ret);
        return ret;
    }

    pr_info("Anyka SFCv2 driver register success\n");
    return 0;
}

static int ak_sfc_remove(struct platform_device *pdev)
{
    struct ak_sfc *c = platform_get_drvdata(pdev);
    //struct device *dev = &pdev->dev;

    //sysfs_remove_file(&dev->kobj, &dev_attr_sfc_nor_trace_level.attr);
    //sysfs_remove_file(&dev->kobj, &dev_attr_sfc_nor_reset.attr);
    //sysfs_remove_file(&dev->kobj, &dev_attr_sfc_nor_id.attr);
    if (c->nor)
        sfc_nor_unregister_all(c);
    if (c->nand)
        sfc_nand_unregister_all(c);
    clk_disable_unprepare(c->gclk);
    clk_disable_unprepare(c->clk);
    return 0;
}

#ifdef CONFIG_PM
static int ak_sfc_suspend(struct platform_device *pdev, pm_message_t state)
{
    struct ak_sfc *c = platform_get_drvdata(pdev);
    //struct spi_nor *nor = c->nor;

    sfc_dump_regs(SFC_DBG);

    //while (mutex_is_locked(&nor->lock)) {
    //    schedule();
    //}
    clk_set_rate(c->clk, 1000000);
    clk_disable_unprepare(c->gclk);

    pr_info("sfc suspend ok\n");
    return 0;
}
static int ak_sfc_resume(struct platform_device *pdev)
{
    struct ak_sfc *c = platform_get_drvdata(pdev);
    //struct spi_nor *nor = c->nor;
    u32 reg;

    clk_set_rate(c->clk, c->clk_rate);
    clk_prepare_enable(c->gclk);

    //reg = __raw_readl(AK_VA_SYSCTRL + 0x20);
    //reg |= ((1 << 31) | (1 << 5));
    //__raw_writel(reg, AK_VA_SYSCTRL + 0x20);
    //reg = __raw_readl(AK_VA_SYSCTRL);
    //udelay(1);
    reg = __raw_readl(AK_VA_SYSCTRL + 0x20);
    reg &= ~((1 << 31) | (1 << 5));
    __raw_writel(reg, AK_VA_SYSCTRL + 0x20);
    reg = __raw_readl(AK_VA_SYSCTRL);

    if (c->l2buf_id != BUF_NULL) {
        sfc_trace(SFC_ERR, "l2buf_id is not empty!\n");
    }

    reg = __raw_readl(AK_VA_SFC + SFC_REG_INT_MASK);
    //reg |= (STA_SFC_TRANS_DONE_INT_EN | STA_SPI_ABORT_HAPPEN_INT
    //    | STA_RXFIFOCNT_FULL_INT_EN | STA_TXFIFO_EMPTY_INT_EN
    //    | STA_XIP_ADDR_ERR_INT_EN | STA_XIP_MODE_ERR_INT_EN);
    reg |= STA_SFC_TRANS_DONE_INT_EN;
    __raw_writel(reg, AK_VA_SFC + SFC_REG_INT_MASK);

    reg = __raw_readl(AK_VA_SFC + SFC_REG_CS_TIMING);
    reg = (9 << 4) | (9 << 0);
    __raw_writel(reg, AK_VA_SFC + SFC_REG_CS_TIMING);

    reg = __raw_readl(AK_VA_SFC + SFC_REG_RXDS_CFG);
    reg = (1 << 28) | (0 << 0);
    __raw_writel(reg, AK_VA_SFC + SFC_REG_RXDS_CFG);

    sfc_set_spi_baudrate(c, c->freq);
    sfc_dump_regs(SFC_DBG);

    pr_info("sfc resume ok\n");
    return 0;
}
#else
#define ak_sfc_suspend NULL
#define ak_sfc_resume  NULL
#endif

static const struct of_device_id ak_sfc_match[] = {
    { .compatible = "anyka,km01a-sfc"},
    { .compatible = "anyka,ak3918av130-sfc"},
    { /* sentinel */ }
};
MODULE_DEVICE_TABLE(of, ak_sfc_match);

static struct platform_driver ak_sfc_driver = {
    .driver = {
        .name = "ak-sfc",
        .of_match_table = ak_sfc_match,
    },
    .probe  = ak_sfc_probe,
    .remove = ak_sfc_remove,
    .suspend = ak_sfc_suspend,
    .resume  = ak_sfc_resume,
};

module_platform_driver(ak_sfc_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anyka Microelectronic Ltd.");
MODULE_DESCRIPTION("Anyka SFCv2 Driver");
MODULE_VERSION("1.0.04");
