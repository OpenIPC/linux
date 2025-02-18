/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef XMEDIA_FMC100_REG_H
#define XMEDIA_FMC100_REG_H

#define XMEDIA_FMC100_CFG                                       0x00
#define XMEDIA_FMC100_CFG_SPI_NOR_ADDR_MODE_4B        0x400

#define XMEDIA_FMC100_CFG_NF_MODE_TOGGLE10            0x2000
#define XMEDIA_FMC100_CFG_NF_MODE_ONFI23              0x4000
#define XMEDIA_FMC100_CFG_NF_MODE_ONFI30              0x4000
#define XMEDIA_FMC100_CFG_NF_MODE_TOGGLE20            0x8000
#define XMEDIA_FMC100_CFG_NF_MODE_MASK                (7 << 13)

#define XMEDIA_FMC100_CFG_ECC_TYPE_MASK               0xE0
#define XMEDIA_FMC100_CFG_ECC_TYPE_NONE               0x00
#define XMEDIA_FMC100_CFG_ECC_TYPE_8BIT               0x20
#define XMEDIA_FMC100_CFG_ECC_TYPE_16BIT              0x40
#define XMEDIA_FMC100_CFG_ECC_TYPE_24BIT              0x60
#define XMEDIA_FMC100_CFG_ECC_TYPE_28BIT              0x80
#define XMEDIA_FMC100_CFG_ECC_TYPE_40BIT              0xA0
#define XMEDIA_FMC100_CFG_ECC_TYPE_64BIT              0xC0

#define XMEDIA_FMC100_CFG_PAGE_SIZE_MASK              0x18
#define XMEDIA_FMC100_CFG_PAGE_SIZE_2K                0x00
#define XMEDIA_FMC100_CFG_PAGE_SIZE_4K                0x08
#define XMEDIA_FMC100_CFG_PAGE_SIZE_8K                0x10
#define XMEDIA_FMC100_CFG_PAGE_SIZE_16K               0x18

#define XMEDIA_FMC100_CFG_FLASH_SEL_MASK              0x6
#define XMEDIA_FMC100_CFG_FLASH_SEL_SPINOR            0x0
#define XMEDIA_FMC100_CFG_FLASH_SEL_SPINAND           0x2
#define XMEDIA_FMC100_CFG_FLASH_SEL_NAND              0x4

#define XMEDIA_FMC100_CFG_OP_MODE_NORMAL              0x01

#define XMEDIA_FMC100_CFG_NFMODE_MASK                (0x7 << 13)
#define XMEDIA_FMC100_CFG_NFMODE_TOGGLE10            0x2000
#define XMEDIA_FMC100_CFG_NFMODE_ONFI23              0x4000
#define XMEDIA_FMC100_CFG_NFMODE_ONFI30              0x6000
#define XMEDIA_FMC100_CFG_NFMODE_TOGGLE20            0x8000

#define XMEDIA_FMC100_GLOBAL_CFG                                0x04
#define XMEDIA_FMC100_GLOBAL_CFG_RANDOMIZER_EN        (1 << 2)
#define XMEDIA_FMC100_GLOBAL_CFG_EDO_MODE             (1 << 9)

#define XMEDIA_FMC100_TIMING_SPI_CFG                            0x08
#define XMEDIA_FMC100_PND_PWIDTH_CFG                            0x0c

#define XMEDIA_FMC100_SYNC_TIMING                               0x14

#define XMEDIA_FMC100_INT                                       0x18
#define XMEDIA_FMC100_INT_CACHE_PAGE_DONE             BIT(8)
#define XMEDIA_FMC100_INT_AHB_OP                      BIT(7)
#define XMEDIA_FMC100_INT_WR_LOCK                     BIT(6)
#define XMEDIA_FMC100_INT_DMA_ERR                     BIT(5)
#define XMEDIA_FMC100_INT_ERR_ALARM                   BIT(4)
#define XMEDIA_FMC100_INT_ERR_INVALID                 BIT(3)
#define XMEDIA_FMC100_INT_ERR_VALID                   BIT(2)
#define XMEDIA_FMC100_INT_OP_FAIL                     BIT(1)
#define XMEDIA_FMC100_INT_OP_DONE                     BIT(0)

#define XMEDIA_FMC100_INT_EN                                    0x1c
#define XMEDIA_FMC100_INT_EN_CACHE_PAGE_DONE          BIT(8)
#define XMEDIA_FMC100_INT_EN_AHB_OP                   BIT(7)
#define XMEDIA_FMC100_INT_EN_WR_LOCK                  BIT(6)
#define XMEDIA_FMC100_INT_EN_DMA_ERR                  BIT(5)
#define XMEDIA_FMC100_INT_EN_ERR_ALARM                BIT(4)
#define XMEDIA_FMC100_INT_EN_ERR_INVALID              BIT(3)
#define XMEDIA_FMC100_INT_EN_ERR_VALID                BIT(2)
#define XMEDIA_FMC100_INT_EN_OP_FAIL                  BIT(1)
#define XMEDIA_FMC100_INT_EN_OP_DONE                  BIT(0)
#define XMEDIA_FMC100_INT_EN_ALL                      0x1FF

#define XMEDIA_FMC100_INT_CLR                                   0x20
#define XMEDIA_FMC100_INT_CLR_CACHE_PAGE_DONE         BIT(8)
#define XMEDIA_FMC100_INT_CLR_AHB_OP                  BIT(7)
#define XMEDIA_FMC100_INT_CLR_WR_LOCK                 BIT(6)
#define XMEDIA_FMC100_INT_CLR_DMA_ERR                 BIT(5)
#define XMEDIA_FMC100_INT_CLR_ERR_ALARM               BIT(4)
#define XMEDIA_FMC100_INT_CLR_ERR_INVALID             BIT(3)
#define XMEDIA_FMC100_INT_CLR_ERR_VALID               BIT(2)
#define XMEDIA_FMC100_INT_CLR_OP_FAIL                 BIT(1)
#define XMEDIA_FMC100_INT_CLR_OP_DONE                 BIT(0)
#define XMEDIA_FMC100_INT_CLR_ALL                     0x1FF

#define XMEDIA_FMC100_CMD                                       0x24

#define XMEDIA_FMC100_ADDRH                                     0x28
#define XMEDIA_FMC100_ADDRH_BYTE5(_addr)              ((unsigned int)(_addr) & 0xff)

#define XMEDIA_FMC100_ADDRL                                     0x2c

#define XMEDIA_FMC100_OP_CFG                                    0x30
#define XMEDIA_FMC100_OP_CFG_FM_CS                    0x800
#define XMEDIA_FMC100_OP_CFG_NUM_CS                   1
#define XMEDIA_FMC100_OP_CFG_FORCE_CS_EN              0x400
#define XMEDIA_FMC100_OP_CFG_MEM_IFTYPE_SHIFT         7
#define XMEDIA_FMC100_OP_CFG_MEM_IFTYPE_MASK          0x380
#define XMEDIA_FMC100_OP_CFG_MEM_IFTYPE_STD           0x0
#define XMEDIA_FMC100_OP_CFG_MEM_IFTYPE_DUAL          0x80
#define XMEDIA_FMC100_OP_CFG_MEM_IFTYPE_DUAL_ADDR     0x100
#define XMEDIA_FMC100_OP_CFG_MEM_IFTYPE_QUAD          0x180
#define XMEDIA_FMC100_OP_CFG_MEM_IFTYPE_QUAD_ADDR     0x200
#define XMEDIA_FMC100_OP_CFG_ADDR_NUM(_addr)          (((unsigned int)(_addr) & 0x7) << 4)
#define XMEDIA_FMC100_OP_CFG_ADDR_MASK                0x70
#define XMEDIA_FMC100_OP_CFG_DUMMY_NUM(_dummy)        ((unsigned int)(_dummy) & 0xf)
#define XMEDIA_FMC100_OP_CFG_DUMMY_MASK               0x0f

#define XMEDIA_FMC100_DATA_NUM                                  0x38
#define XMEDIA_FMC100_DATA_NUM_CNT(_n)                ((unsigned int)(_n) & 0x3FFF)

#define XMEDIA_FMC100_OP                                        0x3c
#define XMEDIA_FMC100_OP_RW_REG                       0x400
#define XMEDIA_FMC100_OP_READID                       0x200
#define XMEDIA_FMC100_OP_DUMMY_EN                     0x100
#define XMEDIA_FMC100_OP_CMD1_EN                      0x80
#define XMEDIA_FMC100_OP_ADDR_EN                      0x40
#define XMEDIA_FMC100_OP_WRITE_DATA_EN                0x20
#define XMEDIA_FMC100_OP_CMD2_EN                      0x10
#define XMEDIA_FMC100_OP_WAIT_READY_EN                0x8
#define XMEDIA_FMC100_OP_READ_DATA_EN                 0x4
#define XMEDIA_FMC100_OP_READ_STATUS_EN               0x2
#define XMEDIA_FMC100_OP_REG_OP_START                 0x1

#define XMEDIA_FMC100_DMA_LEN                                   0x40
#define XMEDIA_FMC100_DMA_LEN_MASK(_len)              ((unsigned int)(_len) & 0x0fffffff)

#define XMEDIA_FMC100_DMA_AHB_CTRL                              0x48
#define XMEDIA_FMC100_DMA_SADDR_D0                              0x4c
#define XMEDIA_FMC100_DMA_SADDR_D1                              0x50
#define XMEDIA_FMC100_DMA_SADDR_D2                              0x54
#define XMEDIA_FMC100_DMA_SADDR_D3                              0x58
#define XMEDIA_FMC100_DMA_SADDR_OOB                             0x5c

#define XMEDIA_FMC100_OP_CTRL                                   0x68
#define XMEDIA_FMC100_OP_CTRL_RD_OPCODE(_code)        (((unsigned int)(_code) & 0xff) << 16)
#define XMEDIA_FMC100_OP_CTRL_WR_OPCODE(_code)        (((unsigned int)(_code) & 0xff) << 8)
#define XMEDIA_FMC100_OP_CTRL_RD_OP_SEL_MASK          0x30
#define XMEDIA_FMC100_OP_CTRL_RD_OP_SEL_CACHE         0x30
#define XMEDIA_FMC100_OP_CTRL_RD_OP_SEL_BLK           0x20
#define XMEDIA_FMC100_OP_CTRL_RD_OP_SEL_OOB           0x10
#define XMEDIA_FMC100_OP_CTRL_RD_OP_SEL_PAGE          0x00
#define XMEDIA_FMC100_OP_CTRL_RW_OP_WRITE             0x02
#define XMEDIA_FMC100_OP_CTRL_DMA_OP_READY            0x01

#define XMEDIA_FMC100_OP_PARA                                   0x70
#define XMEDIA_FMC100_OP_PARA_RD_OOB_ONLY             BIT(1)

#define XMEDIA_FMC100_STATUS                                    0xac

#define XMEDIA_FMC100_VERSION                                   0xbc
#define XMEDIA_FMC100_VERSION_VALUE                   (0x100)

#define XMEDIA_FMC100_FMC_ERR_NUM0_BUF0                         0xc0

#define XMEDIA_FMC100_FMC_ECC_INVALID_ADDRH                     0xD8
#define XMEDIA_FMC100_FMC_ECC_INVALID_ADDRL                     0xDC

#define XMEDIA_FMC100_READ_TIMING_TUNE                          0xE0
#define XMEDIA_FMC100_READ_TIMING_TUNE_VALUE          (2)

#define XMEDIA_FMC100_FMC_CACHE_CTRL                            0x150
#define XMEDIA_FMC100_FMC_CACHE_EXIT_OP_EN            0x800
#define XMEDIA_FMC100_FMC_CACHE_TERMINATE             0x200
#define XMEDIA_FMC100_FMC_CACHE_PAGE_NUM_MASK         0x0F

#define XMEDIA_FMC100_FMC_DMA_SADDR_D0_P1                       0x154

#define DUMP_READ(_s)

#define xmedia_fmc_read(_host, _reg) ({\
	u32 _regval = readl(_host->regbase + (_reg)); \
	DUMP_READ(printk("%s:%d: readl(0x%08x)=0x%08x\n", __FILE__, __LINE__, (u32)_reg, _regval)); \
	_regval;})

#define DUMP_WRITE(_s)

#define xmedia_fmc_write(_host, _value, _reg) do {\
		DUMP_WRITE(printk("%s:%d: writel(0x%08x)=0x%08x\n", __FILE__, __LINE__, (u32)_reg, (u32)_value)); \
		writel((_value), _host->regbase + (_reg)); \
	} while (0)

#endif /* XMEDIA_FMC100_REG_H */
