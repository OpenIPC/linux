#ifndef __UNFD_FCIE3_REG_H__
#define __UNFD_FCIE3_REG_H__

#ifndef BIT0
#define BIT0    (1<<0)
#endif
#ifndef BIT1
#define BIT1    (1<<1) 
#endif
#ifndef BIT2
#define BIT2    (1<<2)
#endif
#ifndef BIT3
#define BIT3    (1<<3)
#endif
#ifndef BIT4
#define BIT4    (1<<4)
#endif
#ifndef BIT5
#define BIT5    (1<<5)
#endif
#ifndef BIT6
#define BIT6    (1<<6)
#endif
#ifndef BIT7
#define BIT7    (1<<7)
#endif
#ifndef BIT8
#define BIT8    (1<<8)
#endif
#ifndef BIT9
#define BIT9    (1<<9)
#endif
#ifndef BIT10
#define BIT10   (1<<10)
#endif
#ifndef BIT11
#define BIT11   (1<<11)
#endif
#ifndef BIT12
#define BIT12   (1<<12)
#endif
#ifndef BIT13
#define BIT13   (1<<13)
#endif
#ifndef BIT14
#define BIT14   (1<<14)
#endif
#ifndef BIT15
#define BIT15   (1<<15)
#endif
//------------------------------------------------------------------
#define ECC_TYPE_RS                 1
#define ECC_TYPE_4BIT               2
#define ECC_TYPE_8BIT               3
#define ECC_TYPE_12BIT              4
#define ECC_TYPE_16BIT              5
#define ECC_TYPE_20BIT              6
#define ECC_TYPE_24BIT              7
#define ECC_TYPE_24BIT1KB           8
#define ECC_TYPE_32BIT1KB           9
#define ECC_TYPE_40BIT1KB           10

#define ECC_CODE_BYTECNT_RS         10
#if defined(FCIE4_DDR) && FCIE4_DDR
#define ECC_CODE_BYTECNT_4BIT        8
#define ECC_CODE_BYTECNT_8BIT       14
#else
#define ECC_CODE_BYTECNT_4BIT        7
#define ECC_CODE_BYTECNT_8BIT       13
#endif

#define ECC_CODE_BYTECNT_12BIT      20
#define ECC_CODE_BYTECNT_16BIT      26
#if defined(FCIE4_DDR) && FCIE4_DDR
#define ECC_CODE_BYTECNT_20BIT      34
#define ECC_CODE_BYTECNT_24BIT      40
#else
#define ECC_CODE_BYTECNT_20BIT      33
#define ECC_CODE_BYTECNT_24BIT      39
#endif

#define ECC_CODE_BYTECNT_24BIT1KB   42
#define ECC_CODE_BYTECNT_32BIT1KB   56
#define ECC_CODE_BYTECNT_40BIT1KB   70



//------------------------------------------------------------------
#define IF_SPARE_AREA_DMA          0 // [CAUTION] 


//------------------------------------------------------------------
#define NC_MIE_EVENT        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x00)
#define NC_MIE_INT_EN       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x01)
#define NC_MMA_PRI_REG      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x02)
#define NC_MIU_DMA_SEL      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x03)
#define NC_CARD_DET         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x07)
#define NC_FORCE_INT        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x08)
#define NC_PATH_CTL         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x0A)
#define NC_JOB_BL_CNT       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x0B)
#define NC_TR_BK_CNT        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x0C)
#define NC_SDIO_CTL         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1B)
#define NC_SDIO_ADDR0       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1C)
#define NC_SDIO_ADDR1       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1D)
#define NC_R2N_STAT         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x1F)
#define NC_R2N_CTRL         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x20)
#define NC_R2N_DATA_RD      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x21)
#define NC_R2N_DATA_WR      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x22)
#define NC_CIF_FIFO_CTL     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x25)
#define NC_SM_STS             GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x2C)
#define FCIE_NC_REORDER     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x2D)
#define NC_MIU_OFFSET       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x2E)
#define NC_REG_2Fh          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x2F)
#define NC_TEST_MODE        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x30)
#define NC_DEBUG_DBUS0      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x31)
#define NC_DEBUG_DBUS1      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x32)
#define NC_REG_33h            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x33)
#define NC_PWRSAVE_MASK     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x34)
#define NC_PWRSAVE_CTL      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x35)
#define NC_ECO_BUG_PATCH      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x36)
#define NC_CELL_DELAY       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x37)
#define NC_MIU_WPRT_L1      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x38)
#define NC_MIU_WPRT_L0      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x39)
#define NC_MIU_WPRT_H1      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3A)
#define NC_MIU_WPRT_H0      GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3B)
#define NC_MIU_WPRT_ER1     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3C)
#define NC_MIU_WPRT_ER0     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3D)
#define NC_FCIE_VERSION     GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x3F)
#define NC_SIGNAL           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x40)
#define NC_WIDTH            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x41)
#define NC_STAT_CHK         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x42)
#define NC_AUXREG_ADR       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x43)
#define NC_AUXREG_DAT       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x44)
#define NC_CTRL             GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x45)
#define NC_ST_READ          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x46)
#define NC_PART_MODE        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x47)
#define NC_SPARE            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x48)
#define NC_SPARE_SIZE       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x49)
#define NC_ADDEND           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4A)
#define NC_SIGN_DAT         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4B)
#define NC_SIGN_ADR         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4C)
#define NC_SIGN_CTRL        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4D)
#define NC_SPARE_DMA_ADR0   GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4E)
#define NC_SPARE_DMA_ADR1   GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x4F)
#define NC_ECC_CTRL         GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x50)
#define NC_ECC_STAT0        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x51)
#define NC_ECC_STAT1        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x52)
#define NC_ECC_STAT2        GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x53)
#define NC_ECC_LOC          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x54)
#define NC_RAND_R_CMD       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x55)
#define NC_RAND_W_CMD       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x56)
#define NC_ECO_FIX            GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x57) // B3,G1
#define NC_LATCH_DATA       GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x57)
#define NC_DDR_CTRL           GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x58)
#define NC_LFSR_CTRL          GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x59)
#define NC_NAND_TIMING			GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5A)
#define NC_SER_DIN_BYTECNT_LW GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5B)
#define NC_SER_DIN_BYTECNT_HW GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5C)
#define NC_NAND_TIMING1			GET_REG_ADDR(FCIE_REG_BASE_ADDR, 0x5D)

#define NC_CIFD_ADDR(u16_pos)   GET_REG_ADDR(FCIE_NC_CIFD_BASE, u16_pos) // 256 x 16 bits
#define NC_CIFD_BYTE_CNT        0x200 // 256 x 16 bits

#define NC_MAX_SECTOR_BYTE_CNT        (BIT12-1)
#define NC_MAX_SECTOR_SPARE_BYTE_CNT  (BIT8-1)
#define NC_MAX_TOTAL_SPARE_BYTE_CNT   (BIT11-1)

//------------------------------------------------------------------
/* NC_MIE_EVENT 0x00 */
/* NC_MIE_INT_EN 0x01 */
#define BIT_MMA_DATA_END           BIT0
#define BIT_MIU_WP_ERR       BIT8
#define BIT_NC_JOB_END             BIT9
#define BIT_NC_R2N_RDY             BIT10
#define BIT_NC_R2N_ECC             BIT12
#define BIT_NC_POWER_SAVE          BIT13
#if (defined(NAND_DRV_TV_LINUX)&&NAND_DRV_TV_LINUX)|| (defined(NAND_DRV_TV_MBOOT)&&NAND_DRV_TV_MBOOT) || (defined(NAND_DRV_CB2)&&NAND_DRV_CB2)
#define BIT_MIU_LAST_DONE      BIT14
#endif

/* NC_MMA_PRI_REG 0x02 */
#define BIT_NC_DMA_DIR_W           BIT2
#define BIT_MIU_REQUEST_RST        BIT4 
#define BIT_NC_FIFO_CLKRDY         BIT5 
#define BIT_NC_MIU_BURST_MASK      (BIT10|BIT9|BIT8)
#define BIT_NC_MIU_BURST_CTRL      BIT10
#define BIT_NC_MIU_BURST_8         BIT_NC_MIU_BURST_CTRL
#define BIT_NC_MIU_BURST_16        (BIT8|BIT_NC_MIU_BURST_CTRL)
#define BIT_NC_MIU_BURST_32        (BIT9|BIT_NC_MIU_BURST_CTRL)
/* NC_CARD_DET 0x7 */
#define BIT_NC_NF_RBZ_STS          BIT8
/* NC_MIU_DMA_SEL 0x03 */
#define BIT_MIU1_SELECT           BIT15

/* NC_PATH_CTL 0x0A */
#define BIT_MMA_EN                 BIT0
#define BIT_NC_EN                  BIT5
#if !((defined(NAND_DRV_TV_LINUX)&&NAND_DRV_TV_LINUX) || (defined(NAND_DRV_TV_MBOOT)&&NAND_DRV_TV_MBOOT)  || (defined(NAND_DRV_CB2)&&NAND_DRV_CB2))
#define BIT_MIU_LAST_DONE      BIT7  
#endif
/* NC_SDIO_CTL 0x1B */
#define MASK_SDIO_BLK_SIZE_MASK    (BIT12-1)
#define BIT_SDIO_BLK_MODE          BIT15
/* NC_R2N_CTRL_GET 0x1F */
#define BIT_RIU_RDY_MMA            BIT0
/* NC_R2N_CTRL_SET 0x20 */
#define BIT_R2N_MODE_EN            BIT0
#define BIT_R2N_DI_START           BIT1
#define BIT_R2N_DI_EN              BIT2
#define BIT_R2N_DI_END             BIT3
#define BIT_R2N_DO_START           BIT4
#define BIT_R2N_DO_EN              BIT5
#define BIT_R2N_DO_END             BIT6
/* NC_CIF_FIFO_CTL 0x25 */
#define BIT_CIFD_RD_REQ            BIT1
/* NC_SM_STS 0x2C*/
#define BIT_DQS_DELAY_CELL_MASK    (BIT8|BIT9|BIT10|BIT11)
#define BIT_DQS_DELAY_CELL_SHIFT   8
#define BIT_DQS_MODE_MASK          (BIT12|BIT13)
#define BIT_DQS_MDOE_SHIFT         12
#define BIT_DQS_MODE_2T            (0 << BIT_DQS_MDOE_SHIFT)
#define BIT_DQS_MODE_1_5T          (1 << BIT_DQS_MDOE_SHIFT)
#define BIT_DQS_MODE_2_5T          (2 << BIT_DQS_MDOE_SHIFT)
#define BIT_DQS_MODE_0T            (3 << BIT_DQS_MDOE_SHIFT)
/* NC_TEST_MODE 0x30 */
#define BIT_FCIE_BIST_FAIL         (BIT0|BIT1|BIT2|BIT3)
#define BIT_FCIE_DEBUG_MODE_SHIFT  8
#define BIT_FCIE_DEBUG_MODE        (BIT10|BIT9|BIT8)
#define BIT_FCIE_SOFT_RST          BIT12
#define BIT_FCIE_PPFIFO_CLK        BIT14

/* NC_POWER_SAVE_CTL 0x35 */
#define BIT_POWER_SAVE_MODE        BIT0      /* Power Save HW enable, high active */
#define BIT_SD_POWER_SAVE_RIU      BIT1      /* SW set register to emulate power lost event, high active */
#define BIT_POWER_SAVE_MODE_INT_EN BIT2      /* interrupt enable, high active */
#define BIT_SD_POWER_SAVE_RST      BIT3      /* software reset Power Save HW, default is '1', set '0' to reset HW */

#define BIT_RIU_SAVE_EVENT         BIT5      /* RO, RIU emulation power save event */
#define BIT_RST_SAVE_EVENT         BIT6      /* RO, Hardware reset power save event */
#define BIT_BAT_SAVE_EVENT         BIT7      /* RO, Battery lost power save event */
/* NC_ECO_BUG_PATCH 0x36 */
#define BIT_MIU_ECC_PATCH_DIS      BIT9 
#define BIT_NC_WIDTH_PATCH         BIT10
#define BIT_MIU_WRAP_PATCH         BIT11
/* NC_FCIE_VERSION 0x3F */
#define BIT_NFIE_INSIDE            BIT5
/* NC_SIGNAL 0x40 */
#define BIT_NC_CE_SEL_MASK         (BIT2-1)
#define BIT_NC_CE1Z                BIT0
#define BIT_NC_CE_H                BIT2
#define BIT_NC_CE_AUTO             BIT3
#define BIT_NC_WP_H                BIT4
#define BIT_NC_WP_AUTO             BIT5
#define BIT_NC_CHK_RB_HIGH         BIT6
#define BIT_NC_CHK_RB_EDGEn        BIT7
#define DEF_REG0x40_VAL            BIT_NC_CE_H
/* NC_WIDTH 0x41 */
#define BIT_W_H_SHIFT              0
#define BIT_W_L_SHIFT              3
#define BIT_R_H_SHIFT              6
#define BIT_R_L_SHIFT              9
#define BIT_CYCLE_CNT_MAX          7
#define BIT_NC_DEB_SEL_SHIFT       12
#define BIT_NC_DEB_SEL             (BIT14|BIT13|BIT12)
#define BIT_NC_BCH_DEB_SEL         BIT15
/* NC_CTRL 0x45 */
#define BIT_NC_JOB_START           BIT0
#define BIT_NC_CIFD_ACCESS         BIT1
#define BIT_NC_IF_DIR_W            BIT3
/* NC_ST_READ 0x46 */
#define BIT_ST_READ_FAIL           BIT0
#define BIT_ST_READ_BUSYn          BIT6
#define BIT_ST_READ_PROTECTn       BIT7
/* NC_PART_MODE 0x47*/
#define BIT_PARTIAL_MODE_EN        BIT0
#define BIT_START_SECTOR_CNT_MASK  (BIT6|BIT5|BIT4|BIT3|BIT2|BIT1)
#define BIT_START_SECTOR_CNT_SHIFT 1
#define BIT_START_SECTOR_IDX_MASK  (BIT12|BIT11|BIT10|BIT9|BIT8|BIT7)
#define BIT_START_SECTOR_IDX_SHIFT 7
/* NC_SPARE 0x48 */
#define BIT_NC_SECTOR_SPARE_SIZE_MASK     (BIT8-1)
#define BIT_NC_SPARE_DEST_MIU             BIT8
#define BIT_NC_SPARE_NOT_R_IN_CIFD        BIT9
#define BIT_NC_RANDOM_RW_OP_EN            BIT11
#define BIT_NC_HW_AUTO_RANDOM_CMD_DISABLE BIT12
#define BIT_NC_ONE_COL_ADDR               BIT13
/* NC_SIGN_CTRL 0x4D*/
#define BIT_NC_SIGN_CHECK_EN              BIT0
#define BIT_NC_SIGN_STOP_RUN              BIT1
#define BIT_NC_SIGN_CLR_STATUS            BIT2
#define BIT_NC_SIGN_DAT0_MISS             BIT3
#define BIT_NC_SIGN_DAT1_MISS             BIT4
/* NC_ECC_CTRL 0x50*/
#define BIT_NC_PAGE_SIZE_MASK      (BIT3-1)
#define BIT_NC_PAGE_SIZE_512Bn     BIT_NC_PAGE_SIZE_MASK
#define BIT_NC_PAGE_SIZE_2KB       BIT0
#define BIT_NC_PAGE_SIZE_4KB       BIT1
#define BIT_NC_PAGE_SIZE_8KB       (BIT1|BIT0)
#define BIT_NC_PAGE_SIZE_16KB      BIT2
#define BIT_NC_PAGE_SIZE_32KB      (BIT2|BIT0)
#define BIT_NC_ECC_TYPE_MASK       (BIT3|BIT4|BIT5|BIT6)
#define BIT_NC_ECC_TYPE_4b512Bn    BIT_NC_ECC_TYPE_MASK
#define BIT_NC_ECC_TYPE_8b512B     BIT3
#define BIT_NC_ECC_TYPE_12b512B    BIT4
#define BIT_NC_ECC_TYPE_16b512B    (BIT4|BIT3)
#define BIT_NC_ECC_TYPE_20b512B    BIT5
#define BIT_NC_ECC_TYPE_24b512B    (BIT5|BIT3)
#define BIT_NC_ECC_TYPE_24b1KB     (BIT5|BIT4)
#define BIT_NC_ECC_TYPE_32b1KB     (BIT5|BIT4|BIT3)
#define BIT_NC_ECC_TYPE_40b1KB     BIT6
#define BIT_NC_ECC_TYPE_RS         BIT6
#define BIT_NC_ECCERR_NSTOP        BIT7
#define BIT_NC_DYNAMIC_OFFCLK      BIT8
#define BIT_NC_ALL0xFF_ECC_CHECK   BIT9
#define BIT_NC_BYPASS_ECC          BIT10
#define BIT_NC_SHARE_PAD_EN        BIT14
#define BIT_NC_WORD_MODE           BIT15
/* NC_ECC_STAT0 0x51*/
#define BIT_NC_ECC_FAIL            BIT0
#define BIT_NC_ECC_MAX_BITS_MASK   (BIT6|BIT5|BIT4|BIT3|BIT2|BIT1)
#define BIT_NC_ECC_MAX_BITS_SHIFT  1
#define BIT_NC_ECC_SEC_CNT_MASK    (BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
/* NC_ECC_STAT2 0x53*/
#define BIT_NC_ECC_FLGA_MASK       (BIT1|BIT0)
#define BIT_NC_ECC_FLGA_NOERR      0
#define BIT_NC_ECC_FLGA_CORRECT    BIT0
#define BIT_NC_ECC_FLGA_FAIL       BIT1
#define BIT_NC_ECC_FLGA_CODE       BIT_NC_ECC_FLGA_MASK
#define BIT_NC_ECC_CNT_MASK        (BIT7|BIT6|BIT5|BIT4|BIT3|BIT2)
#define BIT_NC_ECC_CNT_SHIFT       2
#define BIT_NC_ECC_SEL_LOC_MASK    (BIT13|BIT12|BIT11|BIT10|BIT9|BIT8)
#define BIT_NC_ECC_SEL_LOC_SHIFT   8
/* NC_RAND_W_CMD 0x56*/ /* for fine tune timing for tADL & random R/W */
#define BIT_NC_HW_DELAY_CNT_MASK      (BIT11|BIT10|BIT9|BIT8)
#define BIT_NC_HW_DELAY_CNT_SHIFT     8
/* NC_LATCH_DATA*/
//#define BIT_NC_PATCH_WAIT_RB_EN    BIT0
//#define BIT_NC_PATCH_JOB_START_EN  BIT1
#define BIT_NC_LATCH_DATA_MASK        (BIT2|BIT1|BIT0)
#define BIT_NC_LATCH_DATA_NORMAL      0
#define BIT_NC_LATCH_DATA_0_5_T       BIT0
#define BIT_NC_LATCH_DATA_1_0_T       BIT1
#define BIT_NC_LATCH_DATA_1_5_T       (BIT1|BIT0)
#define BIT_NC_LATCH_DATA_2_0_T       BIT2
#define BIT_RE_DDR_TIMING_MASK     (BIT8|BIT9|BIT10|BIT11)
#define BIT_RE_DDR_TIMING_SHIFT    8
#define BIT_RE_SEC_TURN_CNT_MASK   (BIT12|BIT13|BIT14|BIT15)
#define BIT_RE_SEC_TURN_CNT_SHIFT  12
/* NC_DDR_CTRL 0x58 */
#define BIT_DDR_MASM               (BIT_DDR_ONFI|BIT_DDR_TOGGLE)
#define BIT_DDR_ONFI               BIT0
#define BIT_DDR_TOGGLE             BIT1
#define BIT_SDR_DIN_FROM_MACRO     BIT3
/* NC_LFSR_CTRL 0x59 */
#define BIT_TRR_CYCLE_CNT_MASK     (BIT0|BIT1|BIT2|BIT3)
#define BIT_TCS_CYCLE_CNT_MASK     (BIT4|BIT5|BIT6|BIT7)
#define BIT_TCS_CYCLE_CNT_SHIFT    4
#define BIT_SEL_PAGE_MASK          (BIT8|BIT9|BIT10|BIT11|BIT12|BIT13|BIT14)
#define BIT_SEL_PAGE_SHIFT         8
#define BIT_LFSR_ENABLE            BIT15

//------------------------------------------------------------------
/* AUX Reg Address */
#define AUXADR_CMDREG8    0x08
#define AUXADR_CMDREG9    0x09
#define AUXADR_CMDREGA    0x0A
#define AUXADR_ADRSET     0x0B
#define AUXADR_RPTCNT     0x18 // Pepeat Count
#define AUXADR_RAN_CNT    0x19
#define AUXADR_RAN_POS    0x1A // offset
#define AUXADR_ST_CHECK   0x1B
#define AUXADR_IDLE_CNT   0x1C
#define AUXADR_INSTQUE    0x20

/* OP Codes: Command */
#define CMD_0x00          0x00
#define CMD_0x30          0x01
#define CMD_0x35          0x02
#define CMD_0x90          0x03
#define CMD_0xFF          0x04
#define CMD_0x80          0x05
#define CMD_0x10          0x06
#define CMD_0x15          0x07
#define CMD_0x85          0x08
#define CMD_0x60          0x09
#define CMD_0xD0          0x0A
#define CMD_0x05          0x0B
#define CMD_0xE0          0x0C
#define CMD_0x70          0x0D
#define CMD_0x50          0x0E
#define CMD_0x01          0x0F
#define CMD_REG8L         0x10
#define CMD_REG8H         0x11
#define CMD_REG9L         0x12
#define CMD_REG9H         0x13
#define CMD_REGAL         0x14
#define CMD_REGAH         0x15

/* OP Code: Address */
#define OP_ADR_CYCLE_00   (0x4 << 4)
#define OP_ADR_CYCLE_01   (0x5 << 4)
#define OP_ADR_CYCLE_10   (0x6 << 4)
#define OP_ADR_CYCLE_11   (0x7 << 4)
#define OP_ADR_TYPE_COL   (0x0 << 2)
#define OP_ADR_TYPE_ROW   (0x1 << 2)
#define OP_ADR_TYPE_FUL   (0x2 << 2)
#define OP_ADR_TYPE_ONE   (0x3 << 2)
#define OP_ADR_SET_0      (0x0 << 0)
#define OP_ADR_SET_1      (0x1 << 0)
#define OP_ADR_SET_2      (0x2 << 0)
#define OP_ADR_SET_3      (0x3 << 0)

#define ADR_C2TRS0        (OP_ADR_CYCLE_00|OP_ADR_TYPE_ROW|OP_ADR_SET_0)
#define ADR_C2T1S0        (OP_ADR_CYCLE_00|OP_ADR_TYPE_ONE|OP_ADR_SET_0)
#define ADR_C3TRS0        (OP_ADR_CYCLE_01|OP_ADR_TYPE_ROW|OP_ADR_SET_0)
#define ADR_C3TFS0        (OP_ADR_CYCLE_00|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C4TFS0        (OP_ADR_CYCLE_01|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C4TFS1        (OP_ADR_CYCLE_01|OP_ADR_TYPE_FUL|OP_ADR_SET_1)
#define ADR_C5TFS0        (OP_ADR_CYCLE_10|OP_ADR_TYPE_FUL|OP_ADR_SET_0)
#define ADR_C6TFS0        (OP_ADR_CYCLE_11|OP_ADR_TYPE_FUL|OP_ADR_SET_0)

/* OP Code: Action */
#define ACT_WAITRB        0x80
#define ACT_CHKSTATUS     0x81
#define ACT_WAIT_IDLE     0x82
#define ACT_WAIT_MMA      0x83
#define ACT_BREAK         0x88
#define ACT_SER_DOUT      0x90 /* for column addr == 0 */
#define ACT_RAN_DOUT      0x91 /* for column addr != 0 */
//#define ACT_WR_REDU     0x92
//#define ACT_LUT_DWLOAD  0x93
//#define ACT_LUT_DWLOAD1 0x94
#define ACT_SER_DIN       0x98 /* for column addr == 0 */
#define ACT_RAN_DIN       0x99 /* for column addr != 0 */
//#define ACT_RD_REDU     0x9A
//#define ACT_LUT_UPLOAD  0x9B
#define ACT_PAGECOPY_US   0xA0
#define ACT_PAGECOPY_DS   0xA1
#define ACT_REPEAT        0xB0


/*
 * Power Save FIFO Cmd*
 */
#define PWR_BAT_CLASS    (0x1 << 13)  /* Battery lost class */
#define PWR_RST_CLASS    (0x1 << 12)  /* Reset Class */

/* Command Type */
#define PWR_CMD_WREG     (0x0 << 9)   /* Write data */
#define PWR_CMD_RDCP     (0x1 << 9)   /* Read and cmp data. If mismatch, HW retry */
#define PWR_CMD_WAIT     (0x2 << 9)   /* Wait idle, max. 128T */
#define PWR_CMD_WINT     (0x3 << 9)   /* Wait interrupt */
#define PWR_CMD_STOP     (0x7 << 9)   /* Stop */

/* RIU Bank */
#define PWR_CMD_BK0      (0x0 << 7)
#define PWR_CMD_BK1      (0x1 << 7)
#define PWR_CMD_BK2      (0x2 << 7)
#define PWR_CMD_BK3      (0x3 << 7)

#define PWR_RIU_ADDR     (0x0 << 0)

#endif /* __UNFD_FCIE3_REG_H__ */
