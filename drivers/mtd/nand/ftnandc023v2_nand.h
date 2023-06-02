/* general register difinition */
#define ECC_CONTROL			0x8
#define	  ECC_ERR_MASK(x)		(x << 0)
#define	  ECC_EN(x)			(x << 8)
#define	  ECC_BASE			(1 << 16)
#define	  ECC_NO_PARITY			(1 << 17)
#define ECC_THRES_BITREG1		0x10
#define	ECC_THRES_BITREG2		0x14
#define ECC_CORRECT_BITREG1		0x18
#define	ECC_CORRECT_BITREG2		0x1C
#define	ECC_INTR_EN			0x20
#define	  ECC_INTR_THRES_HIT		(1 << 1)
#define	  ECC_INTR_CORRECT_FAIL		(1 << 0)
#define	ECC_INTR_STATUS			0x24
#define	  ECC_ERR_THRES_HIT(x)		(1 << (8 + x))
#define	  ECC_ERR_FAIL(x)		(1 << x)
#define	DEV_BUSY			0x100
#define GENERAL_SETTING			0x104
#define	  CE_NUM(x)			(x << 24)
#define	  BUSY_RDY_LOC(x)		(x << 12)
#define	  CMD_STS_LOC(x)		(x << 8)
#define	  REPORT_ADDR_EN		(1 << 4)
#define	  WRITE_PROTECT			(1 << 2)
#define	  DATA_INVERSE			(1 << 1)
#define	  DATA_SCRAMBLER		(1 << 0)
#define MEM_ATTR_SET			0x108
#define	  ATTR_COL_CYCLE(x)		(x << 12)
#define	  ATTR_ROW_CYCLE(x)		(x << 13)
#define	SPARE_REGION_ACCESS		0x10C
#define	  SPARE_PAGE_MODE(x)		(x << 0)
#define	  SPARE_PROT_EN(x)		(x << 8)

#define	FL_AC_TIMING0(x)		(0x110 + (x << 3))
#define	FL_AC_TIMING1(x)		(0x114 + (x << 3))
#define FL_AC_TIMING2(x)		(0x190 + (x << 3))
#define FL_AC_TIMING3(x)		(0x194 + (x << 3))
#define INTR_ENABLE             0x150
#define STS_FAIL_INT_EN(x)      (0x1 << x)
#define CRC_FAIL_INT_EN(x)      (0x1 << (x + 8))
#define INTR_STATUS			0x154
#define	  STATUS_CMD_COMPLETE(x)	(1 << (16 + x))
#define	  STATUS_CRC_FAIL(x)		(1 << (8 + x))
#define	  STATUS_FAIL(x)		(1 << x)
#define READ_STATUS0			0x178
#define CMDQUEUE_STATUS			0x200
#define	  CMDQUEUE_STATUS_FULL(x)	(1 << (8 + x))

#define	CMDQUEUE1(x)			(0x300 + (x << 5))
#define	CMDQUEUE2(x)			(0x304 + (x << 5))
#define	CMDQUEUE3(x)			(0x308 + (x << 5))
#define	CMDQUEUE4(x)			(0x30C + (x << 5))
#define	CMDQUEUE5(x)			(0x310 + (x << 5))
#define	  CMD_COUNT(x)			(x << 16)
#define	CMDQUEUE6(x)			(0x314 + (x << 5))
#define	  CMD_COMPLETE_EN		(1 << 0)
#define	  CMD_WAIT_EN			(1 << 1)
#define	  CMD_SCALE(x)			(x << 2)
#define	  CMD_INDEX(x)			(x << 8)
#define	  CMD_SPARE_NUM(x)		(x << 18)
#define	  CMD_BYTE_MODE			(1 << 26)
#define	  CMD_START_CE(x)		(x << 27)
#define	  CMD_FLASH_TYPE(x)		(x << 30)

#define	BMC_REGION_STATUS		0x400
#define	REGION_SW_RESET		0x428
#define AHB_SLAVEPORT_SIZE		0x508
#define	GLOBAL_RESET			0x50C
#define	AHB_RESET			0x510
#define DQS_DELAY			0x518
#define	SPARE_SRAM			0x1000

/* FIX_FLOW_INDEX for small page */
#define		SMALL_FIXFLOW_READSTATUS	0x29
#define		SMALL_FIXFLOW_READID		0x1B
#define		SMALL_FIXFLOW_RESET		0x21
#define		SMALL_FIXFLOW_READOOB		0x39
#define		SMALL_FIXFLOW_PAGEREAD		0x2E
#define		SMALL_FIXFLOW_PAGEWRITE		0x5C
#define		SMALL_FIXFLOW_WRITEOOB		0x67
#define		SMALL_FIXFLOW_ERASE		0xA9

/* FIX_FLOW_INDEX for large page */
#define		LARGE_FIXFLOW_READID		0x4F
#define		LARGE_FIXFLOW_RESET		0x55
#define		LARGE_FIXFLOW_READSTATUS	0x93
#define		LARGE_FIXFLOW_READOOB		0x3A
#define		LARGE_FIXFLOW_PAGEREAD		0x44
#define		LARGE_FIXFLOW_PAGEWRITE		0x25
#define		LARGE_FIXFLOW_WRITEOOB		0x30
#define		LARGE_FIXFLOW_ERASE		0x58

/* TBD */
#ifdef NAND023_V11
#undef LARGE_FIXFLOW_READID
#define LARGE_FIXFLOW_READID    0x51
#undef LARGE_FIXFLOW_RESET
#define LARGE_FIXFLOW_RESET     0x57
#undef LARGE_FIXFLOW_READSTATUS
#define LARGE_FIXFLOW_READSTATUS    0x63
#undef LARGE_FIXFLOW_READOOB
#define LARGE_FIXFLOW_READOOB   0x20
#undef LARGE_FIXFLOW_PAGEREAD
#define LARGE_FIXFLOW_PAGEREAD  0x00
#undef LARGE_FIXFLOW_WRITEOOB
#define LARGE_FIXFLOW_WRITEOOB  0x44
#undef LARGE_FIXFLOW_ERASE
#define LARGE_FIXFLOW_ERASE     0x82
#undef LARGE_FIXFLOW_PAGEWRITE
#define LARGE_FIXFLOW_PAGEWRITE 0x0F    //pagewrite with spare
#endif

/* FIX_FLOW_INDEX for ONFI change mode */
#define		ONFI_FIXFLOW_GETFEATURE		0x34B
#define		ONFI_FIXFLOW_SETFEATURE		0x352
#define		ONFI_FIXFLOW_SYNCRESET		0x33A

#define		MAX_CE				1   //4
#define		MAX_CHANNEL			1   //2
//#define		NAND_USE_AHBDMA_CHANNEL		0

#ifdef MODULE
#undef CONFIG_MTD_NAND_FTNANDC023
#define CONFIG_FTNANDC023_USE_DMA

/* ONFI 2.0 4K MLC */
#undef CONFIG_FTNANDC023_MICRON_29F32G08CBABB
/* Toggle 8K MLC*/
#undef CONFIG_FTNANDC023_SAMSUNG_K9HDGD8X5M
/* 2K SLC */
#undef CONFIG_FTNANDC023_SAMSUNG_K9F4G08U0A
/* 8K MLC */
#undef CONFIG_FTNANDC023_TOSHIBA_TH58NVG5D2ETA20
/* 4K MLC */
#undef CONFIG_FTNANDC023_MICRON_29F16G08MAA
/* 512B SLC */
#undef CONFIG_FTNANDC023_HYNIX_HY27US08561A
/* 4K MLC */
#undef CONFIG_FTNANDC023_SAMSUNG_K9LBG08U0M
#endif

#ifndef CONFIG_FTNANDC023_DEBUG
#define CONFIG_FTNANDC023_DEBUG			0
#endif
#if CONFIG_FTNANDC023_DEBUG > 0
#define ftnandc023_dbg(fmt, ...) printk(KERN_INFO fmt, ##__VA_ARGS__);
#else
#define ftnandc023_dbg(fmt, ...) do {} while (0);
#endif

#if CONFIG_FTNANDC023_DEBUG > 0
#define DBGLEVEL1(x)	x
#else
#define DBGLEVEL1(x)
#endif

#if CONFIG_FTNANDC023_DEBUG > 1
#define DBGLEVEL2(x)	x
#else
#define DBGLEVEL2(x)
#endif

typedef enum {
	LEGACY_SMALL = 0,
	LEGACY_LARGE,
	TOGGLE,
	ONFI,
} flashtype;

struct ftnandc023_nandchip_attr {
	char *name;
	int sparesize;
	int pagesize;
	int blocksize;	
	int flashsize;
	flashtype legacy;
};

struct ftnandc023_chip_timing {
	uint16_t		tWH;
	uint16_t		tWP;
	uint16_t		tREH;
	uint16_t		tREA;
	uint16_t		tRP;
	uint16_t		tWB;
	uint16_t		tRB;
	uint16_t		tWHR;
	uint16_t		tWHR2;
	uint16_t		tRHW;
	uint16_t		tRR;
	uint16_t		tAR;
	uint16_t		tADL;
	uint16_t		tRHZ;
	uint16_t		tCCS;
	uint16_t		tCS;
	uint16_t		tCLS;
	uint16_t		tCLR;
	uint16_t		tALS;
	uint16_t		tCALS2;
	uint16_t		tCWAW;
	uint16_t		tWPRE;
	uint16_t		tRPRE;
	uint16_t		tWPST;
	uint16_t		tRPST;
	uint16_t		tWPSTH;
	uint16_t		tRPSTH;
	uint16_t		tDQSHZ;
	uint16_t		tCAD;
	uint16_t		tDSL;
	uint16_t		tDSH;
	uint16_t		tDQSL;
	uint16_t		tDQSH;
	uint16_t		tCKWR;
	uint16_t		tWRCK;
};

