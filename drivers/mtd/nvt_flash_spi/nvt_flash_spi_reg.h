#ifndef _NVT_FLASH_SPI_REG_H
#define _NVT_FLASH_SPI_REG_H

#include <mach/rcw_macro.h>
#include <mach/nvt_type.h>
/*
    SPI_LATCHDELAY_ENUM
*/
typedef enum
{
	SPI_LATCHDELAY_NORMAL,	//< Normal latch
	SPI_LATCHDELAY_1T,		//< Delay latch 1T
	SPI_LATCHDELAY_2T,		//< Delay latch 2T

	ENUM_DUMMY4WORD(SPI_LATCHDELAY_ENUM)
} SPI_LATCHDELAY_ENUM;

/*
    SPI_PKT_LEN_ENUM
*/
typedef enum
{
	SPI_PKT_LEN_ENUM_1BYTE,		//< 1 byte
	SPI_PKT_LEN_ENUM_2BYTES,	//< 2 byte

	ENUM_DUMMY4WORD(SPI_PKT_LEN_ENUM)
} SPI_PKT_LEN_ENUM;

/*
    SPI_PKT_CNT_ENUM
*/
typedef enum
{
	SPI_PKT_CNT_ENUM_1PKT,		//< 1 packet
	SPI_PKT_CNT_ENUM_2PKT,		//< 2 packet
	SPI_PKT_CNT_ENUM_3PKT,		//< 3 packet (reserved)
	SPI_PKT_CNT_ENUM_4PKT,		//< 4 packet

	ENUM_DUMMY4WORD(SPI_PKT_CNT_ENUM)
} SPI_PKT_CNT_ENUM;

#define NAND_MODULE0_REG_OFS	0x00
union NAND_MODULE0_REG {
	uint32_t reg;
	struct {
	unsigned int col_addr:2;
	unsigned int row_addr:2;
	unsigned int page_size:4;
	unsigned int pri_ecc_type:2;
	unsigned int sec_ecc_type:2;
	unsigned int reserved0:4;
	unsigned int latch_dly:3;
	unsigned int nand_type:1;
	unsigned int spi_flash_type:1;
	unsigned int spi_nor_flash_op_mode:1;
	unsigned int spi_nand_skip_column_address:1;
	unsigned int reserved1:9;
	} bit;
};

#define NAND_DLL_PHASE_DLY_REG1_OFS	0x08
union NAND_DLL_PHASE_DLY_REG1 {
	uint32_t reg;
	struct {
	unsigned int phy_sw_reset:1;
	unsigned int reserved0:1;
	unsigned int phy_sampclk_inv:1;
	unsigned int phy_src_clk_sel:1;
	unsigned int reserved1:15;
	unsigned int phy_clk_out_inv:1;
	unsigned int phy_pad_clk_sel:1;
	unsigned int reserved2:11;
	} bit;
};

#define NAND_DLL_PHASE_DLY_REG2_OFS	0x0C
union NAND_DLL_PHASE_DLY_REG2 {
	uint32_t reg;
	struct {
	unsigned int phy_data_dsw_sel:12;
	unsigned int phy_dat_read_dly:2;
	unsigned int phy_det_read_dly:2;
	unsigned int outdly_sel:6;
	unsigned int reserved0:2;
	unsigned int indly_sel:6;
	unsigned int reserved1:2;
	} bit;
};

#define NAND_SPI_CFG_REG_OFS	0x14
union NAND_SPI_CFG_REG {
	uint32_t reg;
	struct {
	unsigned int spi_pkt_lsb_mode:1;
	unsigned int spi_operation_mode:1;
	unsigned int reserved0:2;
	unsigned int spi_nand_cs:1;
	unsigned int reserved1:3;
	unsigned int spi_cs_pol:1;
	unsigned int spi_bs_width:2;
	unsigned int spi_io_order:1;
	unsigned int spi_pull_wphld:1;
	unsigned int reserved2:19;
	} bit;
};

#define NAND_DLL_PHASE_DLY_REG4_OFS	0x18
union NAND_DLL_PHASE_DLY_REG4 {
	uint32_t reg;
	struct {
	unsigned int output_dly_en:1;
	unsigned int reserved0:3;
	unsigned int data_out_delay_inv:1;
	unsigned int reserved1:3;
	unsigned int output_dly_sel:5;
	unsigned int reserved2:3;
	unsigned int clk_out_inv:1;
	unsigned int reserved3:15;
	} bit;
};

#define NAND_CTRL0_REG_OFS	0x20
union NAND_CTRL0_REG {
	uint32_t reg;
	struct {
	unsigned int oper_cmmd:6;
	unsigned int reserved0:2;
	unsigned int chip_en:1;
	unsigned int reserved1:3;
	unsigned int oper_en:1;
	unsigned int nand_wp:1;
	unsigned int reserved2:1;
	unsigned int soft_reset:1;
	unsigned int multipage_sel:2;
	unsigned int reserved3:1;
	unsigned int timeout_en:1;
	unsigned int protect_area1:1;
	unsigned int protect_area2:1;
	unsigned int reserved4:10;
	} bit;
};

#define NAND_TIME2_REG_OFS	0x24
union NAND_TIME2_REG {
	uint32_t reg;
	struct {
	unsigned int tslch:4;
	unsigned int tshch:4;
	unsigned int tshsl:8;
	unsigned int reserved0:16;
	} bit;
};

#define NAND_SRAM_ACCESS_REG_OFS	0x28
union NAND_SRAM_ACCESS_REG {
	uint32_t reg;
	struct {
	unsigned int spare_acc:1;
	unsigned int error_acc:1;
	unsigned int reserved0:30;
	} bit;
};

#define NAND_TIME0_REG_OFS	0x2C
union NAND_TIME0_REG {
	uint32_t reg;
	struct {
	unsigned int trp:4;
	unsigned int treh:4;
	unsigned int twp:4;
	unsigned int twh:4;
	unsigned int tcls:4;
	unsigned int tclh:4;
	unsigned int tadl:4;
	unsigned int tclch:4;
	} bit;
};

#define NAND_TIME1_REG_OFS	0x30
union NAND_TIME1_REG {
	uint32_t reg;
	struct {
	unsigned int tmpceh:8;
	unsigned int twb:4;
	unsigned int tmprb:4;
	unsigned int tals:4;
	unsigned int talh:4;
	unsigned int reserved0:8;
	} bit;
};

#define NAND_COMMAND_REG_OFS	0x34
union NAND_COMMAND_REG {
	uint32_t reg;
	struct {
	unsigned int cmd_cyc_1st:8;
	unsigned int cmd_cyc_2nd:8;
	unsigned int cmd_cyc_3rd:8;
	unsigned int cmd_cyc_4th:8;
	} bit;
};

#define NAND_COLADDR_REG_OFS	0x38
union NAND_COLADDR_REG {
	uint32_t reg;
	struct {
	unsigned int coladdr_1st:8;
	unsigned int coladdr_2nd:8;
	unsigned int coladdr_3rd:8;
	unsigned int reserved0:8;
	} bit;
};

#define NAND_ROWADDR_REG_OFS	0x3C
union NAND_ROWADDR_REG {
	uint32_t reg;
	struct {
	unsigned int rowaddr_1st:8;
	unsigned int rowaddr_2nd:8;
	unsigned int rowaddr_3rd:8;
	unsigned int rowaddr_4th:8;
	} bit;
};

#define NAND_TIMEOUT_REG_OFS	0x40
union NAND_TIMEOUT_REG {
	uint32_t reg;
	struct {
	unsigned int toutvalue:16;
	unsigned int reserved:16;
	} bit;
};

#define NAND_INTMASK_REG_OFS	0x44
union NAND_INTMASK_REG {
	uint32_t reg;
	struct {
	unsigned int reserved0:12;
	unsigned int comp_inten:1;
	unsigned int pri_ecc_inten:1;
	unsigned int stsfail_inten:1;
	unsigned int tout_inten:1;
	unsigned int sec_ecc_inten:1;
	unsigned int protect1_inten:1;
	unsigned int protect2_inten:1;
	unsigned int reserved1:13;
	} bit;
};

#define NAND_CTRL_STS_REG_OFS	0x48
union NAND_CTRL_STS_REG {
	uint32_t reg;
	struct {
	unsigned int sm_sts:8;
	unsigned int busy_sts:1;
	unsigned int reserved0:3;
	unsigned int comp_sts:1;
	unsigned int pri_ecc_sts:1;
	unsigned int stsfail_sts:1;
	unsigned int tout_sts:1;
	unsigned int sec_ecc_sts:1;
	unsigned int protect1_sts:1;
	unsigned int protect2_sts:1;
	unsigned int reserved:13;
	} bit;
};

#define NAND_PAGENUM_REG_OFS	0x4C
union NAND_PAGENUM_REG {
	uint32_t reg;
	struct {
	unsigned int pagenum:12;
	unsigned int reserved:20;
	} bit;
};

#define NAND_HAMERR_STS0_REG_OFS	0x50
union NAND_HAMERR_STS0_REG {
	uint32_t reg;
	struct {
	unsigned int sec0f1_err_sts:2;
	unsigned int sec0f2_err_sts:2;
	unsigned int sec1f1_err_sts:2;
	unsigned int sec1f2_err_sts:2;
	unsigned int sec2f1_err_sts:2;
	unsigned int sec2f2_err_sts:2;
	unsigned int sec3f1_err_sts:2;
	unsigned int sec3f2_err_sts:2;
	unsigned int sec4f1_err_sts:2;
	unsigned int sec4f2_err_sts:2;
	unsigned int sec5f1_err_sts:2;
	unsigned int sec5f2_err_sts:2;
	unsigned int sec6f1_err_sts:2;
	unsigned int sec6f2_err_sts:2;
	unsigned int sec7f1_err_sts:2;
	unsigned int sec7f2_err_sts:2;
	} bit;
};


#define NAND_RSERR_STS0_REG_OFS	0x70
union NAND_RSERR_STS0_REG {
	uint32_t reg;
	struct {
	unsigned int sec0_err_sts:4;
	unsigned int sec1_err_sts:4;
	unsigned int sec2_err_sts:4;
	unsigned int sec3_err_sts:4;
	unsigned int sec4_err_sts:4;
	unsigned int sec5_err_sts:4;
	unsigned int sec6_err_sts:4;
	unsigned int sec7_err_sts:4;
	} bit;
};

#define NAND_SECONDARY_ECC_STS_REG_OFS	0x80
union NAND_SECONDARY_ECC_STS_REG {
	uint32_t reg;
	struct {
	unsigned int sec_sec0_err_sts:2;
	unsigned int sec_sec1_err_sts:2;
	unsigned int sec_sec2_err_sts:2;
	unsigned int sec_sec3_err_sts:2;
	unsigned int sec_sec4_err_sts:2;
	unsigned int sec_sec5_err_sts:2;
	unsigned int sec_sec6_err_sts:2;
	unsigned int sec_sec7_err_sts:2;
	unsigned int reserved:16;
	} bit;
};

#define NAND_PROTECT_AREA1_ROW_START_ADDR_OFS	0x90

#define NAND_PROTECT_AREA1_ROW_END_ADDR_OFS	0x94

#define NAND_PROTECT_AREA2_ROW_START_ADDR_OFS	0x98

#define NAND_PROTECT_AREA2_ROW_END_ADDR_OFS	0x9C

#define NAND_MULTISPARE_INTERVAL_OFS	0xB0

#define NAND_COMMAND2_REG_OFS	0xB4
union NAND_COMMAND2_REG {
	uint32_t reg;
	struct {
	unsigned int cmd_cyc_5th:8;
	unsigned int cmd_cyc_6th:8;
	unsigned int cmd_cyc_7th:8;
	unsigned int cmd_cyc_8th:8;
	} bit;
};

#define NAND_STATUS_CHECK_REG_OFS	0xBC
union NAND_STATUS_CHECK_REG {
	uint32_t reg;
	struct {
	unsigned int status_value:8;
	unsigned int status_mask:8;
	unsigned int reserved:16;
	} bit;
};

#define NAND_DUMMY_CLOCK_NUM_REG_OFS	0xC0
union NAND_DUMMY_CLOCK_NUM_REG {
	uint32_t reg;
	struct {
	unsigned int dummy_clk:3;
	unsigned int reserved:29;
	} bit;
};


#define NAND_DATAPORT_REG_OFS	0x100
union NAND_DATAPORT_REG {
	uint32_t reg;
	struct {
	unsigned int data:32;
	} bit;
};

#define NAND_DATALEN_REG_OFS	0x104
union NAND_DATALEN_REG {
	uint32_t reg;
	struct {
	unsigned int length:26;
	unsigned int reserved:6;
	} bit;
};

#define NAND_FIFO_STS_REG_OFS	0x108
union NAND_FIFO_STS_REG {
	uint32_t reg;
	struct {
	unsigned int fifo_cnt:5;
	unsigned int reserved0:3;
	unsigned int fifo_empty:1;
	unsigned int fifo_full:1;
	unsigned int reserved1:22;
	} bit;
};

#define NAND_FIFO_CTRL_REG_OFS	0x10C
union NAND_FIFO_CTRL_REG {
	uint32_t reg;
	struct {
	unsigned int fifo_en:1;
	unsigned int fifo_mode:1;
	unsigned int fifo_dir:1;
	unsigned int reserved0:29;
	} bit;
};

#define NAND_DMASTART_REG_OFS	0x110
union NAND_DMASTART_REG {
	uint32_t reg;
	struct {
	unsigned int dmastadr:31;
	unsigned int reserved0:1;
	} bit;
};

#define NAND_DMACURRENT_REG_OFS	0x114
union NAND_DMACURRENT_REG {
	uint32_t reg;
	struct {
	unsigned int dmacuradr:31;
	unsigned int reserved0:1;
	} bit;
};

#define NAND_DEBUG_SEL_REG_OFS	0x140
union NAND_DEBUG_SEL_REG {
	uint32_t reg;
	struct {
	unsigned int reserved0:4;
	unsigned int fix_tclch:1;
	unsigned int debug_sel:27;
	} bit;
};

// for page 512/2K
#define NAND_SPARE00_REG_OFS	0x200
union NAND_SPARE00_REG {
	uint32_t reg;
	struct {
	unsigned int spare00:32;
	} bit;
};

#define NAND_SPARE01_REG_OFS	0x204
union NAND_SPARE01_REG {
	uint32_t reg;
	struct {
	unsigned int spare01:32;
	} bit;
};

#define NAND_SPARE02_REG_OFS	0x208
union NAND_SPARE02_REG {
	uint32_t reg;
	struct {
	unsigned int spare02:32;
	} bit;
};

#define NAND_SPARE03_REG_OFS	0x20C
union NAND_SPARE03_REG {
	uint32_t reg;
	struct {
	unsigned int spare03:32;
	} bit;
};

#define NAND_SPARE04_REG_OFS	0x210
union NAND_SPARE04_REG {
	uint32_t reg;
	struct {
	unsigned int spare04:32;
	} bit;
};

#define NAND_SPARE05_REG_OFS	0x214
union NAND_SPARE05_REG {
	uint32_t reg;
	struct {
	unsigned int spare05:32;
	} bit;
};

#define NAND_SPARE06_REG_OFS	0x218
union NAND_SPARE06_REG {
	uint32_t reg;
	struct {
	unsigned int spare06:32;
	} bit;
};

#define NAND_SPARE07_REG_OFS	0x21C
union NAND_SPARE07_REG {
	uint32_t reg;
	struct {
	unsigned int spare07:32;
	} bit;
};


#define NAND_SPARE08_REG_OFS	0x220
union NAND_SPARE08_REG {
	uint32_t reg;
	struct {
	unsigned int spare08:32;
	} bit;
};

#define NAND_SPARE09_REG_OFS	0x224
union NAND_SPARE09_REG {
	uint32_t reg;
	struct {
	unsigned int spare09:32;
	} bit;
};

#define NAND_SPARE10_REG_OFS	0x228
union NAND_SPARE10_REG {
	uint32_t reg;
	struct {
	unsigned int spare10:32;
	} bit;
};

#define NAND_SPARE11_REG_OFS	0x22C
union NAND_SPARE11_REG {
	uint32_t reg;
	struct {
	unsigned int spare11:32;
	} bit;
};

#define NAND_SPARE12_REG_OFS	0x230
union NAND_SPARE12_REG {
	uint32_t reg;
	struct {
	unsigned int spare12:32;
	} bit;
};

#define NAND_SPARE13_REG_OFS	0x234
union NAND_SPARE13_REG {
	uint32_t reg;
	struct {
	unsigned int spare13:32;
	} bit;
};

#define NAND_SPARE14_REG_OFS	0x238
union NAND_SPARE14_REG {
	uint32_t reg;
	struct {
	unsigned int spare14:32;
	} bit;
};

#define NAND_SPARE15_REG_OFS	0x23C
union NAND_SPARE15_REG {
	uint32_t reg;
	struct {
	unsigned int spare15:32;
	} bit;
};

#define NAND_SPARE16_REG_OFS	0x240
union NAND_SPARE16_REG {
	uint32_t reg;
	struct {
	unsigned int spare16:32;
	} bit;
};

#define NAND_SPARE17_REG_OFS	0x244
union NAND_SPARE17_REG {
	uint32_t reg;
	struct {
	unsigned int spare17:32;
	} bit;
};

#define NAND_SPARE18_REG_OFS	0x248
union NAND_SPARE18_REG {
	uint32_t reg;
	struct {
	unsigned int spare18:32;
	} bit;
};

#define NAND_SPARE19_REG_OFS	0x24C
union NAND_SPARE19_REG {
	uint32_t reg;
	struct {
	unsigned int spare19:32;
	} bit;
};

#define NAND_SPARE20_REG_OFS	0x250
union NAND_SPARE20_REG {
	uint32_t reg;
	struct {
	unsigned int spare20:32;
	} bit;
};

#define NAND_SPARE21_REG_OFS	0x254
union NAND_SPARE21_REG {
	uint32_t reg;
	struct {
	unsigned int spare21:32;
	} bit;
};

#define NAND_SPARE22_REG_OFS	0x258
union NAND_SPARE22_REG {
	uint32_t reg;
	struct {
	unsigned int spare22:32;
	} bit;
};

#define NAND_SPARE23_REG_OFS	0x25C
union NAND_SPARE23_REG {
	uint32_t reg;
	struct {
	unsigned int spare23:32;
	} bit;
};

#define NAND_SPARE24_REG_OFS	0x260
union NAND_SPARE24_REG {
	uint32_t reg;
	struct {
	unsigned int spare24:32;
	} bit;
};

#define NAND_SPARE25_REG_OFS	0x264
union NAND_SPARE25_REG {
	uint32_t reg;
	struct {
	unsigned int spare25:32;
	} bit;
};

#define NAND_SPARE26_REG_OFS	0x268
union NAND_SPARE26_REG {
	uint32_t reg;
	struct {
	unsigned int spare26:32;
	} bit;
};

#define NAND_SPARE27_REG_OFS	0x26C
union NAND_SPARE27_REG {
	uint32_t reg;
	struct {
	unsigned int spare27:32;
	} bit;
};

#define NAND_SPARE28_REG_OFS	0x270
union NAND_SPARE28_REG {
	uint32_t reg;
	struct {
	unsigned int spare28:32;
	} bit;
};

#define NAND_SPARE29_REG_OFS	0x274
union NAND_SPARE29_REG {
	uint32_t reg;
	struct {
	unsigned int spare29:32;
	} bit;
};

#define NAND_SPARE30_REG_OFS	0x278
union NAND_SPARE30_REG {
	uint32_t reg;
	struct {
	unsigned int spare30:32;
	} bit;
};

#define NAND_SPARE31_REG_OFS	0x27C
union NAND_SPARE31_REG {
	uint32_t reg;
	struct {
	unsigned int spare31:32;
	} bit;
};

#define NAND_SEC0_EADDR0_REG_OFS	0x400
union NAND_SEC0_EADDR0_REG {
	uint32_t reg;
	struct {
	unsigned int sec0_eaddr0:32;
	} bit;
};

#define NAND_SEC0_EADDR1_REG_OFS	0x404
union NAND_SEC0_EADDR1_REG {
	uint32_t reg;
	struct {
	unsigned int sec0_eaddr1:32;
	} bit;
};

#define NAND_SEC0_EADDR2_REG_OFS	0x408
union NAND_SEC0_EADDR2_REG {
	uint32_t reg;
	struct {
	unsigned int sec0_eaddr2:32;
	} bit;
};

#define NAND_SEC0_EADDR3_REG_OFS	0x40C
union NAND_SEC0_EADDR3_REG {
	uint32_t reg;
	struct {
	unsigned int sec0_eaddr3:32;
	} bit;
};

#endif
