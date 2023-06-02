/*
 * drivers/mtd/ambarella_spinand.c
 *
 * History:
 *    2015/10/26 - [Ken He] created file
 *
 * Copyright (C) 2014-2018, Ambarella, Inc.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include "ambarella_spinand.h"

#define	MAX_WAIT_JIFFIES               (40 * HZ)
#define MAX_WAIT_ERASE_JIFFIES         ((HZ * 400)/1000)
#define AVERAGE_WAIT_JIFFIES           ((HZ * 20)/1000)

#define CACHE_BUF	(2176)

static struct nand_ecclayout ecc_layout_2KB_8bit = {
       .eccbytes = 64,
       .eccpos = {
               64, 65, 66, 67, 68, 69, 70, 71,
               72, 73, 74, 75, 76, 77, 78, 79,
			   80, 81, 82, 83, 84, 85, 86, 87,
			   88, 89, 90, 91, 92, 93, 94, 95,
			   96, 97, 98, 99, 100, 101, 102, 103,
			   104, 105, 106, 107, 108, 109, 110, 111,
			   112, 113, 114, 115, 116, 117, 118, 119,
			   120, 121, 122, 123, 124, 125, 126, 127
       },
       .oobfree = { {1, 63} }
};

/****************************************************************************/
static inline struct amb_spinand *mtd_to_amb(struct mtd_info *mtd)
{
    return container_of(mtd, struct amb_spinand, mtd);
}

/* mtd_to_state - obtain the spinand state from the mtd info provided */
static inline struct spinand_state *mtd_to_state(struct mtd_info *mtd)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct amb_spinand *flash = (struct amb_spinand *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)flash->priv;

	return state;
}
/****************************************************************************/

static int ambspinand_send_cmd(struct amb_spinand *flash, u32 cmd, u32 dummy_len, u32 data, u32 len)
{
    u32 tmp = 0;

    tmp = amba_readl(flash->regbase + REG00);
    REGPREP(tmp, REG00_DATALEN_MASK, REG00_DATALEN_SHIFT, 0);
    REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, dummy_len);
    REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, len);
    REGPREP(tmp, REG00_CMDLEN_MASK, REG00_CMDLEN_SHIFT, 1);
    amba_writel(flash->regbase + REG00, tmp);

    tmp = amba_readl(flash->regbase + REG04);
    REGPREP(tmp, REG04_WRITEEN_MASK, REG04_WRITEEN_SHIFT, 1);
    REGPREP(tmp, REG04_READEN_MASK, REG04_READEN_SHIFT, 0);
    REGPREP(tmp, REG04_DATALANE_MASK, REG04_DATALANE_SHIFT, 0);
    REGPREP(tmp, REG04_ADDRLANE_MASK, REG04_ADDRLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDLANE_MASK, REG04_CMDLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDDTR_MASK, REG04_CMDDTR_SHIFT, 0);
    REGPREP(tmp, REG04_LSBFRT_MASK, REG04_LSBFRT_SHIFT, 0);
    amba_writel(flash->regbase + REG04, tmp);

    tmp = amba_readl(flash->regbase + REG0C);
    REGPREP(tmp, REG0C_CMD0_MASK, REG0C_CMD0_SHIFT, cmd);
    amba_writel(flash->regbase + REG0C, tmp);

    if(len){
        tmp = data;
        amba_writel(flash->regbase + REG14, tmp);
    }

    tmp = 0x0;
    amba_writel(flash->regbase + REG30, tmp);

    tmp = 0x20;
    amba_writel(flash->regbase + REG3C, tmp);

    tmp = amba_readl(flash->regbase + REG50);
    REGPREP(tmp, REG50_STRTRX_MASK, REG50_STRTRX_SHIFT, 1);
    amba_writel(flash->regbase + REG50, tmp);

    for (;;){
        tmp = amba_readl(flash->regbase + REG38);
        if (REGDUMP(tmp, REG38_DATALENREACHINTR_MASK, REG38_DATALENREACHINTR_SHIFT) == 1){
            return 0;
        }
    }
    return -1;
}

static int ambspinand_read_reg(struct amb_spinand *flash, u32 datalen, u32 reg, u8 *value)
{
    u32 tmp = 0;
    int i;

    tmp = amba_readl(flash->regbase + REG28);
    for (;REGDUMP(tmp, REG28_RXFIFOLV_MASK, REG28_RXFIFOLV_SHIFT)!= 0;){
        amba_readb(flash->regbase + REG200);
        tmp = amba_readl(flash->regbase + REG28);
    }

    tmp = amba_readl(flash->regbase + REG00);
    REGPREP(tmp, REG00_CMDLEN_MASK, REG00_CMDLEN_SHIFT, 1);
    REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, 0);
    REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, 0);
    REGPREP(tmp, REG00_DATALEN_MASK, REG00_DATALEN_SHIFT, datalen);
    amba_writel(flash->regbase + REG00, tmp);

    tmp = amba_readl(flash->regbase + REG04);
    REGPREP(tmp, REG04_READEN_MASK, REG04_READEN_SHIFT, 1);
    REGPREP(tmp, REG04_WRITEEN_MASK, REG04_WRITEEN_SHIFT, 0);
    REGPREP(tmp, REG04_RXLANE_MASK, REG04_RXLANE_SHIFT, 1);
    REGPREP(tmp, REG04_DATALANE_MASK, REG04_DATALANE_SHIFT, 1);
    REGPREP(tmp, REG04_ADDRLANE_MASK, REG04_ADDRLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDLANE_MASK, REG04_CMDLANE_SHIFT, 0);
    REGPREP(tmp, REG04_LSBFRT_MASK, REG04_LSBFRT_SHIFT, 0);
    REGPREP(tmp, REG04_CMDDTR_MASK, REG04_CMDDTR_SHIFT, 0);
    amba_writel(flash->regbase + REG04, tmp);

    tmp = reg;
    amba_writel(flash->regbase + REG0C, tmp);

    tmp = 0x0;
    amba_writel(flash->regbase + REG10, tmp);
    tmp = 0x0;
    amba_writel(flash->regbase + REG14, tmp);

    tmp = 0;
    REGPREP(tmp, REG50_STRTRX_MASK, REG50_STRTRX_SHIFT, 1);
    amba_writel(flash->regbase + REG50, tmp);

    for (;;){
        tmp = amba_readl(flash->regbase + REG28);
        if(REGDUMP(tmp, REG28_RXFIFOLV_MASK, REG28_RXFIFOLV_SHIFT) == datalen){
            break;
        }
    }

    for (i = 0; i < datalen; i++){
        *(value+i) = amba_readb(flash->regbase + REG200);
    }
    return 0;
}

static int ambspinand_read_feature(struct amb_spinand *flash, u32 cmd, u32 datalen, u32 reg, u8 *value)
{
    u32 tmp = 0;
    int i;

    tmp = amba_readl(flash->regbase + REG28);
    for (;REGDUMP(tmp, REG28_RXFIFOLV_MASK, REG28_RXFIFOLV_SHIFT)!= 0;){
        amba_readb(flash->regbase + REG200);
        tmp = amba_readl(flash->regbase + REG28);
    }

    tmp = amba_readl(flash->regbase + REG00);
    REGPREP(tmp, REG00_CMDLEN_MASK, REG00_CMDLEN_SHIFT, 1);
    REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, 0);
    REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, 1);
    REGPREP(tmp, REG00_DATALEN_MASK, REG00_DATALEN_SHIFT, datalen);
    amba_writel(flash->regbase + REG00, tmp);

    tmp = amba_readl(flash->regbase + REG04);
    REGPREP(tmp, REG04_READEN_MASK, REG04_READEN_SHIFT, 1);
    REGPREP(tmp, REG04_WRITEEN_MASK, REG04_WRITEEN_SHIFT, 0);
    REGPREP(tmp, REG04_RXLANE_MASK, REG04_RXLANE_SHIFT, 1);
    REGPREP(tmp, REG04_DATALANE_MASK, REG04_DATALANE_SHIFT, 1);
    REGPREP(tmp, REG04_ADDRLANE_MASK, REG04_ADDRLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDLANE_MASK, REG04_CMDLANE_SHIFT, 0);
    REGPREP(tmp, REG04_LSBFRT_MASK, REG04_LSBFRT_SHIFT, 0);
    REGPREP(tmp, REG04_CMDDTR_MASK, REG04_CMDDTR_SHIFT, 0);
    amba_writel(flash->regbase + REG04, tmp);

    tmp = cmd;
    amba_writel(flash->regbase + REG0C, tmp);

    tmp = 0x0;
    amba_writel(flash->regbase + REG10, tmp);
    tmp = reg;
    amba_writel(flash->regbase + REG14, tmp);

    tmp = 0;
    REGPREP(tmp, REG50_STRTRX_MASK, REG50_STRTRX_SHIFT, 1);
    amba_writel(flash->regbase + REG50, tmp);

    for (;;){
        tmp = amba_readl(flash->regbase + REG28);
        if(REGDUMP(tmp, REG28_RXFIFOLV_MASK, REG28_RXFIFOLV_SHIFT) == datalen){
            break;
        }
    }

    for (i = 0; i < datalen; i++){
        *(value+i) = amba_readb(flash->regbase + REG200);
    }
    return 0;
}

static int ambspinand_set_feature(struct amb_spinand *flash, u32 cmd, u32 datalen, u32 reg, u8 value)
{
    u32 tmp = 0;
/*
    tmp = amba_readl(flash->regbase + REG28);
    for (;REGDUMP(tmp, REG28_RXFIFOLV_MASK, REG28_RXFIFOLV_SHIFT)!= 0;){
        amba_readb(flash->regbase + REG200);
        tmp = amba_readl(flash->regbase + REG28);
    }
*/
    tmp = amba_readl(flash->regbase + REG00);
    REGPREP(tmp, REG00_CMDLEN_MASK, REG00_CMDLEN_SHIFT, 1);
    REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, 0);
    REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, 1);
    REGPREP(tmp, REG00_DATALEN_MASK, REG00_DATALEN_SHIFT, datalen);
    amba_writel(flash->regbase + REG00, tmp);

    tmp = amba_readl(flash->regbase + REG04);
    REGPREP(tmp, REG04_READEN_MASK, REG04_READEN_SHIFT, 0);
    REGPREP(tmp, REG04_WRITEEN_MASK, REG04_WRITEEN_SHIFT, 1);
    REGPREP(tmp, REG04_RXLANE_MASK, REG04_RXLANE_SHIFT, 0);
    REGPREP(tmp, REG04_DATALANE_MASK, REG04_DATALANE_SHIFT, 0);
    REGPREP(tmp, REG04_ADDRLANE_MASK, REG04_ADDRLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDLANE_MASK, REG04_CMDLANE_SHIFT, 0);
    REGPREP(tmp, REG04_LSBFRT_MASK, REG04_LSBFRT_SHIFT, 0);
    REGPREP(tmp, REG04_CMDDTR_MASK, REG04_CMDDTR_SHIFT, 0);
    amba_writel(flash->regbase + REG04, tmp);

    tmp = cmd;
    amba_writel(flash->regbase + REG0C, tmp);

    tmp = 0x0;
    amba_writel(flash->regbase + REG10, tmp);
    tmp = reg;
    amba_writel(flash->regbase + REG14, tmp);

	amba_writeb(flash->regbase + REG100, value);
    tmp = 0;
    REGPREP(tmp, REG50_STRTRX_MASK, REG50_STRTRX_SHIFT, 1);
    amba_writel(flash->regbase + REG50, tmp);

	for (;;){
            tmp = amba_readl(flash->regbase + REG24);
            if(REGDUMP(tmp, REG24_TXFIFOLV_MASK, REG24_TXFIFOLV_SHIFT) == 0){
                return 0;
            }
            udelay(100);//must delay
    }

    return 0;
}

static int ambspi_dma_config(struct amb_spinand *flash)
{
     flash->dmabuf = dma_alloc_coherent(flash->dev, AMBA_SPINOR_DMA_BUFF_SIZE,
                     &flash->dmaaddr, GFP_KERNEL);
     if (flash->dmabuf == NULL){
        dev_err(flash->dev,  "dma_alloc_coherent failed!\n");
        return -ENOMEM;
     }
     return 0;
}

static int ambspinand_prog_page(struct amb_spinand *flash, u16 byte_id, u8 *buf, u32 len)
{
    int done;
    u32 tmp = 0;

    amba_writel(flash->dmaregbase + 0x00,  0x1a800000);// DMA_ch0_control
    amba_writel(flash->dmaregbase + 0x0c,  0x0);
    amba_writel(flash->dmaregbase + 0x04,  flash->dmaaddr + byte_id);// DMA_ch0_src_addr
    amba_writel(flash->dmaregbase + 0x08,  (u32)(flash->regbase + REG100));// DMA_ch0_dest_addr
    amba_writel(flash->dmaregbase + 0x00,  0x9a800000|len);// DMA_ch0_control

    tmp = amba_readl(flash->regbase + REG00);
    REGPREP(tmp, REG00_DATALEN_MASK, REG00_DATALEN_SHIFT, len);
    //REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, flash->dummy);
    //REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, flash->addr_width);
	REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, 0);
    REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, 2);
    REGPREP(tmp, REG00_CMDLEN_MASK, REG00_CMDLEN_SHIFT, 1);
    amba_writel(flash->regbase + REG00, tmp);

    tmp = amba_readl(flash->regbase + REG04);
    REGPREP(tmp, REG04_WRITEEN_MASK, REG04_WRITEEN_SHIFT, 1);
    REGPREP(tmp, REG04_READEN_MASK, REG04_READEN_SHIFT, 0);
    REGPREP(tmp, REG04_DATALANE_MASK, REG04_DATALANE_SHIFT, 0);
    REGPREP(tmp, REG04_ADDRLANE_MASK, REG04_ADDRLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDLANE_MASK, REG04_CMDLANE_SHIFT, 0);
    REGPREP(tmp, REG04_LSBFRT_MASK, REG04_LSBFRT_SHIFT, 0);
    REGPREP(tmp, REG04_CMDDTR_MASK, REG04_CMDDTR_SHIFT, 0);
    amba_writel(flash->regbase + REG04, tmp);

    tmp = flash->command[0];
    amba_writel(flash->regbase + REG0C, tmp);

    tmp = 0;
    REGPREP(tmp, REG30_DATAREACH_MASK, REG30_DATAREACH_SHIFT, 1);
    amba_writel(flash->regbase + REG30, tmp);

    tmp = byte_id;
    amba_writel(flash->regbase + REG14, tmp);

    tmp = 0;
    REGPREP(tmp, REG1C_TXFIFOLV_MASK, REG1C_TXFIFOLV_SHIFT, (256-32));
    amba_writel(flash->regbase + REG1C, tmp);

    tmp = amba_readl(flash->regbase + REG18);
    REGPREP(tmp, REG18_TXDMAEN_MASK, REG18_TXDMAEN_SHIFT, 1);
    REGPREP(tmp, REG18_RXDMAEN_MASK, REG18_RXDMAEN_SHIFT, 0);
    amba_writel(flash->regbase + REG18, tmp);

    do {
        tmp = amba_readl(flash->regbase + REG2C);
        done = REGDUMP(tmp, REG2C_TXFIFOEMP_MASK, REG2C_TXFIFOEMP_SHIFT);
    }while (done != 0x0);

    tmp = 0x20;
    amba_writel(flash->regbase + REG3C, tmp);

    tmp = amba_readl(flash->regbase + REG50);
    REGPREP(tmp, REG50_STRTRX_MASK, REG50_STRTRX_SHIFT, 1);
    amba_writel(flash->regbase + REG50, tmp);
    for (;;){
        tmp = amba_readl(flash->regbase + REG38);
        if (REGDUMP(tmp,
                REG38_DATALENREACHINTR_MASK,
                REG38_DATALENREACHINTR_SHIFT) == 1){
            return 0;
        }
    }
    return -1;
}

static int ambspinand_read_page(struct amb_spinand *flash, u32 offset, u8 *rbuf,
                            u32 len)
{
    u32 tmp = 0;

    amba_writel(flash->dmaregbase + 0x10,  0x2a800000);// DMA_ch1_control
    amba_writel(flash->dmaregbase + 0x1c,  0x0);
    amba_writel(flash->dmaregbase + 0x14,  (u32)(flash->regbase + REG200));// DMA_ch1_src_addr
    amba_writel(flash->dmaregbase + 0x18,  flash->dmaaddr);// DMA_ch1_dest_addr
    amba_writel(flash->dmaregbase + 0x10,  0xaa800000 | len);// DMA_ch1_control

    tmp = amba_readl(flash->regbase + REG00);
    REGPREP(tmp, REG00_DATALEN_MASK, REG00_DATALEN_SHIFT, len);
    //REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, flash->dummy);
    //REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, flash->addr_width);
	REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, 0);
    REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, 3);
    REGPREP(tmp, REG00_CMDLEN_MASK, REG00_CMDLEN_SHIFT, 1);
    amba_writel(flash->regbase + REG00, tmp);

    tmp = amba_readl(flash->regbase + REG04);
    REGPREP(tmp, REG04_READEN_MASK, REG04_READEN_SHIFT, 1);
    REGPREP(tmp, REG04_WRITEEN_MASK, REG04_WRITEEN_SHIFT, 0);
    REGPREP(tmp, REG04_DATALANE_MASK, REG04_DATALANE_SHIFT, 1);
    REGPREP(tmp, REG04_ADDRLANE_MASK, REG04_ADDRLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDLANE_MASK, REG04_CMDLANE_SHIFT, 0);
    REGPREP(tmp, REG04_LSBFRT_MASK, REG04_LSBFRT_SHIFT, 0);
    REGPREP(tmp, REG04_CMDDTR_MASK, REG04_CMDDTR_SHIFT, 0);
    amba_writel(flash->regbase + REG04, tmp);

    tmp = flash->command[0];
    amba_writel(flash->regbase + REG0C, tmp);

    tmp = offset;
    amba_writel(flash->regbase + REG14, tmp);

    tmp = 0x20;
    amba_writel(flash->regbase + REG3C, tmp);

    tmp = (32-1); // must use word.can't use rxfifothlv
    amba_writel(flash->regbase + REG20, tmp);

    tmp = amba_readl(flash->regbase + REG18);
    REGPREP(tmp, REG18_RXDMAEN_MASK, REG18_RXDMAEN_SHIFT, 1);
    REGPREP(tmp, REG18_TXDMAEN_MASK, REG18_TXDMAEN_SHIFT, 0);
    amba_writel(flash->regbase + REG18, tmp);

    tmp = amba_readl(flash->regbase + REG50);
    REGPREP(tmp, REG50_STRTRX_MASK, REG50_STRTRX_SHIFT, 1);
    amba_writel(flash->regbase + REG50, tmp);

    for (;;){
        tmp = amba_readl(flash->regbase + REG38);
        if(REGDUMP(tmp,
                REG38_DATALENREACHINTR_MASK,
                REG38_DATALENREACHINTR_SHIFT) == 1){
            return 0;
        }
        udelay(10);
    }
    return -1;
}
#if 0
static u32 get_ssi3_freq_hz(void)
{
	#define PLL_OUT_ENET   300000000
	u32 val;

	val = amba_rct_readl(CG_SSI3_REG);
	if (val & 0x01000000)
		return 0;

	if (val == 0)
		val = 1;

	//return (clk_get_rate(clk_get(NULL, "gclk_core")) << 1) / val;
	return (PLL_OUT_ENET) / val;
}
#endif
static int ambspinand_init(struct amb_spinand *flash)
{
    u32 tmp = 0;
#if 0
	u32 divider;
	divider = get_ssi3_freq_hz() / flash->clk;
    tmp = amba_readl(flash->regbase + REG08);
    REGPREP(tmp, REG08_CHIPSEL_MASK, REG08_CHIPSEL_SHIFT, ~(1 << SPINOR_DEV));
    REGPREP(tmp, REG08_CLKDIV_MASK, REG08_CLKDIV_SHIFT, divider);
    REGPREP(tmp, REG08_FLOWCON_MASK, REG08_FLOWCON_SHIFT, 1);
    REGPREP(tmp, REG08_HOLDPIN_MASK, REG08_HOLDPIN_SHIFT, 3);
    amba_writel(flash->regbase + REG08, tmp);

    tmp = amba_readl(flash->regbase + REG00);
    REGPREP(tmp, REG00_DATALEN_MASK, REG00_DATALEN_SHIFT, 0);
    REGPREP(tmp, REG00_DUMMYLEN_MASK, REG00_DUMMYLEN_SHIFT, 0);
    REGPREP(tmp, REG00_ADDRLEN_MASK, REG00_ADDRLEN_SHIFT, 0);
    REGPREP(tmp, REG00_CMDLEN_MASK, REG00_CMDLEN_SHIFT, 1);
    amba_writel(flash->regbase + REG00, tmp);

    tmp = amba_readl(flash->regbase + REG04);
    REGPREP(tmp, REG04_WRITEEN_MASK, REG04_WRITEEN_SHIFT, 0);
    REGPREP(tmp, REG04_READEN_MASK, REG04_READEN_SHIFT, 0);
    REGPREP(tmp, REG04_DATALANE_MASK, REG04_DATALANE_SHIFT, 0);
    REGPREP(tmp, REG04_ADDRLANE_MASK, REG04_ADDRLANE_SHIFT, 0);
    REGPREP(tmp, REG04_CMDLANE_MASK, REG04_CMDLANE_SHIFT, 0);
    REGPREP(tmp, REG04_LSBFRT_MASK, REG04_LSBFRT_SHIFT, 0);
    REGPREP(tmp, REG04_CMDDTR_MASK, REG04_CMDDTR_SHIFT, 0);
    amba_writel(flash->regbase + REG04, tmp);
#endif
    tmp = 0x20;
    amba_writel(flash->regbase + REG30, tmp);

    tmp = 0;
    REGPREP(tmp, REG40_TXFIFORESET_MASK, REG40_TXFIFORESET_SHIFT, 1);
    amba_writel(flash->regbase + REG40, tmp);
    tmp = 0;
    REGPREP(tmp, REG44_RXFIFORESET_MASK, REG44_RXFIFORESET_SHIFT, 1);
    amba_writel(flash->regbase + REG44, tmp);

    /* after reset fifo, the 0x28 will become 0x10,
    *so , read REG200 times to clear the 0x28,  this is a bug in hardware
    */
	while (amba_readl(flash->regbase + REG28) != 0) {
        tmp = amba_readl(flash->regbase + REG200);
    }

    tmp = 0;
    REGPREP(tmp, REG1C_TXFIFOLV_MASK, REG1C_TXFIFOLV_SHIFT, 0x7f);
    amba_writel(flash->regbase + REG1C, tmp);

    tmp = 0;
    REGPREP(tmp, REG20_RXFIFOLV_MASK, REG20_RXFIFOLV_SHIFT, 0x7f);
    amba_writel(flash->regbase + REG20, tmp);

    return 0;
}
/* spi nand function end */

/****************************************************************************/
/*
 * spinand_write_enable - send command to enable write or erase of
 * the nand cells.
 * Before one can write or erase the nand cells, the write enable
 * has to be set. After write or erase, the write enable bit is
 * automatically cleared.
 */
static int spinand_write_enable(struct amb_spinand *flash)
{
    int ret;

	flash->command[0] = SPI_NAND_WRITE_ENABLE;
	ret = ambspinand_send_cmd(flash, flash->command[0], 0, 0, 0);
	if (ret < 0) {
		   dev_err(flash->dev, "Write Enable SPI NAND failed!\n");
	}
	return ret;
}

/*
 * spinand_get_feature - send command to get feature register
 * spinand_set_feature - send command to set feature register
 *
 * The GET FEATURES (0Fh) and SET FEATURES (1Fh) commands are used to
 * alter the device behavior from the default power-on behavior.
 * These commands use a 1-byte feature address to determine which feature
 * is to be read or modified
 */
static int spinand_get_feature(struct amb_spinand *flash, u8 feature_reg,
                                                               u8 *value)
{
	u32 cmd;
	int ret;

	cmd = SPI_NAND_GET_FEATURE_INS;

	/* Check the register address */
	if (feature_reg != SPI_NAND_PROTECTION_REG_ADDR &&
		feature_reg != SPI_NAND_FEATURE_EN_REG_ADDR &&
		feature_reg != SPI_NAND_STATUS_REG_ADDR &&
		feature_reg != SPI_NAND_DS_REG_ADDR)
			return -1;

	ret = ambspinand_read_feature(flash, cmd, 1, feature_reg, value);
	if (ret < 0)
		dev_err(flash->dev, "Error %d read feature reg.\n", ret);
	return ret;
}

static int spinand_set_feature(struct amb_spinand *flash, u8 feature_reg,
                                                               u8 value)
{
	int ret;
	u32 cmd;

	cmd = SPI_NAND_SET_FEATURE;

	/* Check the register address */
	if (feature_reg != SPI_NAND_PROTECTION_REG_ADDR &&
		feature_reg != SPI_NAND_FEATURE_EN_REG_ADDR &&
		feature_reg != SPI_NAND_STATUS_REG_ADDR &&
		feature_reg != SPI_NAND_DS_REG_ADDR)
			return -1;

	ret = ambspinand_set_feature(flash, cmd, 1, feature_reg, value);
	if (ret < 0)
		dev_err(flash->dev, "Error %d set feture reg.\n", ret);

	return ret;
}
/*
 * spinand_get_status
 * spinand_get_protection
 * spinand_get_feature_en
 * spinand_get_driver_strength
 *
 * Read the specific feature register using spinand_get_feature
 */
static inline int
spinand_get_status(struct amb_spinand *flash, u8 *value)
{
	return
	spinand_get_feature(flash, SPI_NAND_STATUS_REG_ADDR, value);
}
static inline int
spinand_get_protection(struct amb_spinand *flash, u8 *value)
{
	return
	spinand_get_feature(flash, SPI_NAND_PROTECTION_REG_ADDR, value);
}
static inline int
spinand_get_feature_en(struct amb_spinand *flash, u8 *value)
{
	return
	spinand_get_feature(flash, SPI_NAND_FEATURE_EN_REG_ADDR, value);
}
static inline int
spinand_get_driver_strength(struct amb_spinand *flash, u8 *value)
{
	return
	spinand_get_feature(flash, SPI_NAND_DS_REG_ADDR, value);
}
/*
 * spinand_set_status
 * spinand_set_protection
 * spinand_set_feature_en
 *
 * Set the specific feature register using spinand_set_feature
 */
static inline int
spinand_set_status(struct amb_spinand *flash, u8 value)
{
	return
	spinand_set_feature(flash, SPI_NAND_STATUS_REG_ADDR, value);
}
static inline int
spinand_set_protection(struct amb_spinand *flash, u8 value)
{
	return
	spinand_set_feature(flash, SPI_NAND_PROTECTION_REG_ADDR, value);
}
static inline int
spinand_set_feature_en(struct amb_spinand *flash, u8 value)
{
	return
	spinand_set_feature(flash, SPI_NAND_FEATURE_EN_REG_ADDR, value);
}
static inline int
spinand_set_driver_strength(struct amb_spinand *flash, u8 value)
{
	return
	spinand_set_feature(flash, SPI_NAND_DS_REG_ADDR, value);
}
/*
 * is_spinand_busy - check the operation in progress bit and return
 * if NAND chip is busy or not.
 * This function checks the Operation In Progress (OIP) bit to
 * determine whether the NAND memory is busy with a program execute,
 * page read, block erase or reset command.
 */
static inline int is_spinand_busy(struct amb_spinand *flash)
{
	u8 status;
	int ret;

	/* Read the status register and check the OIP bit */
	ret = spinand_get_status(flash, &status);
	if (ret)
		return ret;

	if (status & SPI_NAND_OIP)
		return 1;
	else
		return 0;
}

/* wait_execution_complete - wait for the current operation to finish */
static inline int wait_execution_complete(struct amb_spinand *flash,
                                                       u32 timeout)
{
	int ret;
	unsigned long deadline = jiffies + timeout;

	do {
		ret = is_spinand_busy(flash);
		if (!ret)
			return 0;
		if (ret < 0)
			return ret;
	} while (!time_after_eq(jiffies, deadline));

	return -1;
}

static int spinand_enable_ecc(struct amb_spinand *flash)
{
	uint8_t feature_reg = 0;
	int ret = 0;

	ret = spinand_get_feature_en(flash, &feature_reg);
	if (ret < 0)
		return ret;
	return spinand_set_feature_en(flash, feature_reg | SPI_NAND_ECC_EN);
}
#if 0
static int spinand_disable_ecc(struct amb_spinand *flash)
{
	uint8_t feature_reg = 0;
	int ret = 0;

	ret = spinand_get_feature_en(flash, &feature_reg);
	if (ret < 0)
		return ret;
	return spinand_set_feature_en(flash, feature_reg & (~SPI_NAND_ECC_EN));
}
#endif
/*
 * spinand_read_id - Read SPI nand ID
 * Byte 0: Manufacture ID
 * Byte 1: Device ID 1
 * Byte 2: Device ID 2
 */
static int spinand_read_id(struct amb_spinand *flash, u8 *id)
{
	int ret;
	u8 nand_id[3];
	u32 cmd;

	cmd = SPI_NAND_READ_ID;
	ret = ambspinand_read_reg(flash,3, cmd, nand_id);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d reading id\n", ret);
		return ret;
	}
	id[0] = nand_id[0];
	id[1] = nand_id[1];
	id[2] = nand_id[2];

	return ret;
}

/* spinand_reset - send RESET command to NAND device */
static void spinand_reset(struct amb_spinand *flash)
{
	int ret;

	flash->command[0] = SPI_NAND_RESET;
	ret = ambspinand_send_cmd(flash, flash->command[0], 0, 0, 0);
	if (ret < 0) {
		   dev_err(flash->dev, "Reset SPI NAND failed!\n");
		   return;
	}

	/* OIP status can be read from 300ns after reset*/
	udelay(1);
	/* Wait for execution to complete */
	ret = wait_execution_complete(flash, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0)
			dev_err(flash->dev, "%s: Wait execution complete failed!\n",
				__func__);
		else
			dev_err(flash->dev, "%s: Wait execution complete timedout!\n",
				__func__);
	}
}

/*
 * spinand_erase_block - erase a block
 * The erase can specify the block to be erased
 * (block_id >= 0, block_id < no_blocks)
 * no_blocks depends on the size of the flash
 * Command sequence: WRITE ENBALE, BLOCK ERASE,
 * GET FEATURES command to read the status register
 */
static int spinand_erase_block(struct amb_spinand *flash, u32 page)
{
	int ret;
	u8 status;

	/* Enable capability of erasing NAND cells */
	ret = spinand_write_enable(flash);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d on write enable.\n",
					(int) ret);
		return ret;
	}

	/* Set up command buffer. */
	flash->command[0] = SPI_NAND_BLOCK_ERASE_INS;
	ambspinand_send_cmd(flash, flash->command[0], 0, page, 3);

	if (ret < 0) {
		dev_err(flash->dev, "Error %d when erasing block.\n",
					(int) ret);
		return ret;
	}

	ret = wait_execution_complete(flash, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			dev_err(flash->dev, "%s: Wait execution complete failed!\n",
					__func__);
			return ret;
		} else {
			dev_err(flash->dev, "%s: Wait execution complete timedout!\n",
					__func__);
			return -1;
		}
	}

	/* Check status register for erase fail bit */
	ret = spinand_get_status(flash, &status);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d reading status register.\n",
					(int) ret);
		return ret;
	}
	if (status & SPI_NAND_EF) {
		dev_err(flash->dev, "Erase fail on block %d\n", (page/64));
		return -1;
	}
	return 0;
}

/*
 * spinand_read_page_to_cache - send command to read data from the device and
 * into the internal cache
 * The read can specify the page to be read into cache
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 */
static int spinand_read_page_to_cache(struct amb_spinand *flash, u32 page_id)
{
	int ret = 0;
	/* Set up command buffer. */
	flash->command[0] = SPI_NAND_PAGE_READ_INS;
	ret = ambspinand_send_cmd(flash, flash->command[0], 0, page_id, 3);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d when read page 0x%x to cache.\n",
					page_id, (int) ret);
    }
	return ret;
}

/*
 * spinand_read_from_cache - send command to read out the data from the
 * cache register
 * The read can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The read can specify a length to be read (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 */
static int spinand_read_from_cache(struct amb_spinand *flash,
                                       u32 byte_id, int len, u8 *rbuf)
{
	int ret;

	flash->command[0] = SPI_NAND_READ_CACHE_INS;
	ret = ambspinand_read_page(flash, byte_id, rbuf, len);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d when read from cache.\n",
			   (int) ret);
	}
	return ret;
}

/*
 * spinand_read_page - read data from the flash by first reading the
 * corresponding page into the internal cache and after reading out the
 * data from it.
 * The read can specify the page to be read into cache
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 * The read can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The read can specify a length to be read (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 */
static int spinand_read_page(struct amb_spinand *flash, u32 page_id,
                               u32 offset, int len, u8 *rbuf)
{
	int ret;
	u8 status;

	/* Read page from device to internal cache */
	ret = spinand_read_page_to_cache(flash, page_id);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d reading page to cache.\n",
				ret);
		return ret;
	}

	/* Wait until the operation completes or a timeout occurs. */
	ret = wait_execution_complete(flash, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			dev_err(flash->dev, "%s: Wait execution complete failed!\n",
						__func__);
			return ret;
		} else {
			dev_err(flash->dev, "%s: Wait execution complete timedout!\n",
						__func__);
			return -1;
		}
	}

	/* Check status register for uncorrectable errors */
	ret = spinand_get_status(flash, &status);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d reading status register.\n",
					(int) ret);
		return ret;
	}
	status &= SPI_NAND_ECC_UNABLE_TO_CORRECT;
	if (status  == SPI_NAND_ECC_UNABLE_TO_CORRECT) {
		dev_err(flash->dev, "ECC error reading page %d.\n",
					page_id);
		return -1;
	}

	/* Read page from internal cache to our buffers */
	ret = spinand_read_from_cache(flash, offset, len, rbuf);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d reading from cache.\n",
			(int) ret);
		return ret;
	}

	return ret;
}

/*
 * spinand_program_data_to_cache - send command to program data to cache
 * The write can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The write can specify a length to be written
 * (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 */
static int spinand_program_data_to_cache(struct amb_spinand *flash,
                                               u16 byte_id, int len, u8 *wbuf)
{
	int ret = 0;

	flash->command[0] = SPI_NAND_PROGRAM_LOAD_INS;
	ret = ambspinand_prog_page(flash, byte_id, wbuf, len);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d when program to cache.\n",
			   (int) ret);
	}
	return ret;
}

/*
 * spinand_program_execute - writes a page from cache to NAND array
 * The write can specify the page to be programmed
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 */
static int spinand_program_execute(struct amb_spinand *flash, u32 page_id)
{
	int ret;

	flash->command[0] = SPI_NAND_PROGRAM_EXEC_INS;
	ret = ambspinand_send_cmd(flash, flash->command[0], 0, page_id, 3);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d when read page 0x%x to cache.\n",
				   page_id, (int) ret);
	}
	return ret;
}

/*
 * spinand_program_page - secquence to program a page
 * The write can specify the page to be programmed
 * (page_id >= 0, page_id < no_pages, no_pages=no_blocks*no_pages_per_block)
 * no_blocks and no_pages_per_block depend on the size of the flash
 * The write can specify a byte offset within the page
 * (byte_id >= 0, byte_id < size_of_page)
 * The write can specify a length to be written
 * (len > 0 && len < size_of_page)
 * size_of_page depends on the size of the flash
 * Command sequence: WRITE ENABLE, PROGRAM LOAD, PROGRAM EXECUTE,
 * GET FEATURE command to read the status
 */
static int spinand_program_page(struct amb_spinand *flash,
               u32 page_id, u16 offset, int len, u8 *buf)
{
	int ret;
	u8 status;
	uint8_t *wbuf;

	wbuf = buf;

	/* Issue program cache command */
	ret = spinand_program_data_to_cache(flash, offset, len, wbuf);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d when programming cache.\n",
					(int) ret);
		return ret;
	}

	/* Enable capability of programming NAND cells */
	ret = spinand_write_enable(flash);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d on write enable.\n",
					(int) ret);
		return ret;
	}

	/* Issue program execute command */
	ret = spinand_program_execute(flash, page_id);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d when programming NAND cells.\n",
					(int) ret);
		return ret;
	}

	/* Wait until the operation completes or a timeout occurs. */
	ret = wait_execution_complete(flash, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			dev_err(flash->dev, "%s: Wait execution complete failed!\n",
						__func__);
			return ret;
		} else {
			dev_err(flash->dev, "%s Wait execution complete timedout!\n",
						__func__);
			return -1;
		}
	}

	/* Check status register for program fail bit */
	ret = spinand_get_status(flash, &status);
	if (ret < 0) {
		dev_err(flash->dev, "Error %d reading status register.\n",
					(int) ret);
		return ret;
	}
	if (status & SPI_NAND_PF) {
		dev_err(flash->dev, "Program failed on page %d\n", page_id);
		return -1;
	}

	return 0;
}

static int spinand_write_page_hwecc(struct mtd_info *mtd,
               struct nand_chip *chip, const uint8_t *buf, int oob_required)
{
	chip->write_buf(mtd, buf, chip->ecc.size * chip->ecc.steps);
	return 0;
}

static int spinand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
               uint8_t *buf, int oob_required, int page)
{
	u8 status;
	int ret;
	struct amb_spinand *flash = (struct amb_spinand *)chip->priv;

	/* Read data and OOB area */
	chip->read_buf(mtd, buf, chip->ecc.size * chip->ecc.steps);
	if (oob_required)
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);

	/* Wait until the operation completes or a timeout occurs. */
	ret = wait_execution_complete(flash, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			pr_err("%s: Wait execution complete failed!\n",
					__func__);
			return ret;
		} else {
			pr_err("%s: Wait execution complete timedout!\n",
					__func__);
			return -1;
		}
	}

	/* Check status register for uncorrectable errors */
	ret = spinand_get_status(flash, &status);
	if (ret < 0) {
		pr_err("Error %d reading status register.\n", ret);
		return ret;
	}
	status &= SPI_NAND_ECC_UNABLE_TO_CORRECT;
	if (status  == SPI_NAND_ECC_UNABLE_TO_CORRECT) {
		pr_info("ECC error reading page.\n");
		mtd->ecc_stats.failed++;
	}
	if (status && (status != SPI_NAND_ECC_UNABLE_TO_CORRECT))
		mtd->ecc_stats.corrected++;
	return 0;
}

static void amb_spinand_cmdfunc(struct mtd_info *mtd, unsigned int command,
                               int column, int page)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct amb_spinand *flash = (struct amb_spinand *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)flash->priv;

	switch (command) {
		case NAND_CMD_READ1:
		case NAND_CMD_READ0:
			state->buf_ptr = 0;
			spinand_read_page(flash, page, 0, mtd->writesize,
								flash->dmabuf);
			break;
		case NAND_CMD_READOOB:
			state->buf_ptr = 0;
			spinand_read_page(flash, page, mtd->writesize, mtd->oobsize,
								flash->dmabuf);
			break;
		case NAND_CMD_RNDOUT:
			state->buf_ptr = column;
			break;
		case NAND_CMD_READID:
			state->buf_ptr = 0;
			spinand_read_id(flash, (u8 *)flash->dmabuf);
			break;
		case NAND_CMD_PARAM:
			state->buf_ptr = 0;
			break;
		/* ERASE1 performs the entire erase operation*/
		case NAND_CMD_ERASE1:
			//printk("spinand erase 1 command page 0x%x \n", page);
			spinand_erase_block(flash, page);
			break;
		case NAND_CMD_ERASE2:
			//printk("spinand erase 2 command page 0x%x \n", page);
			break;
		/* SEQIN sets up the addr buffer and all registers except the length */
		case NAND_CMD_SEQIN:
			state->col = column;
			state->row = page;
			state->buf_ptr = 0;
			break;
		/* PAGEPROG reuses all of the setup from SEQIN and adds the length */
		case NAND_CMD_PAGEPROG:
			spinand_program_page(flash, state->row, state->col,
									state->buf_ptr, flash->dmabuf);
			break;
		case NAND_CMD_STATUS:
			spinand_get_status(flash, flash->dmabuf);
			if (!(flash->dmabuf[0] & 0x80))
				flash->dmabuf[0] = 0x80;
			state->buf_ptr = 0;
			break;
		/* RESET command */
		case NAND_CMD_RESET:
			spinand_reset(flash);
			break;
		default:
			dev_err(flash->dev, "Command 0x%x not implementd or unknown.\n",
					command);
		}
}

static int ambarella_spinand_get_resource(struct amb_spinand *flash,
				struct platform_device *pdev)
{
    struct resource *res;
    int errorCode = 0;

    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (!res) {
        dev_err(&pdev->dev, "No mem resource for spinor_reg!\n");
        errorCode = -ENXIO;
        goto spinand_get_resource_err_exit;
    }

    flash->regbase =
        devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!flash->regbase) {
        dev_err(&pdev->dev, "devm_ioremap() failed\n");
        errorCode = -ENOMEM;
        goto spinand_get_resource_err_exit;
    }

    res = platform_get_resource(pdev, IORESOURCE_MEM, 1);
    if (!res) {
        dev_err(&pdev->dev, "No mem resource for spinor_reg!\n");
        errorCode = -ENXIO;
        goto spinand_get_resource_err_exit;
    }

    flash->dmaregbase =
        devm_ioremap(&pdev->dev, res->start, resource_size(res));
    if (!flash->dmaregbase) {
        dev_err(&pdev->dev, "devm_ioremap() failed\n");
        errorCode = -ENOMEM;
        goto spinand_get_resource_err_exit;
    }

    return 0;

spinand_get_resource_err_exit:
    return errorCode;
}

static int amb_spinand_waitfunc(struct mtd_info *mtd, struct nand_chip *chip)
{
	struct amb_spinand *flash = (struct amb_spinand *)chip->priv;
	int state = chip->state;
	u32 timeout;

	if (state == FL_ERASING)
		timeout = MAX_WAIT_ERASE_JIFFIES;
	else
		timeout = AVERAGE_WAIT_JIFFIES;
	return wait_execution_complete(flash, timeout);
}

static void amb_spinand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct amb_spinand *flash = (struct amb_spinand *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)flash->priv;

	memcpy(flash->dmabuf + state->buf_ptr, buf, len);
	state->buf_ptr += len;
}

static void amb_spinand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct amb_spinand *flash = (struct amb_spinand *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)flash->priv;

	memcpy(buf, flash->dmabuf + state->buf_ptr, len);
	state->buf_ptr += len;
}

static uint8_t amb_spinand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct amb_spinand *flash = (struct amb_spinand *)chip->priv;
	struct spinand_state *state = (struct spinand_state *)flash->priv;
	u8 data;

	data = flash->dmabuf[state->buf_ptr];
	state->buf_ptr++;
	return data;
}

static void amb_spinand_select_chip(struct mtd_info *mtd, int dev)
{
}

static int ambarella_spinand_init_chip(struct amb_spinand *flash,
		struct device_node *np)
{
	struct nand_chip *chip = &flash->chip;

	chip->read_byte = amb_spinand_read_byte;
	chip->write_buf = amb_spinand_write_buf;
	chip->read_buf = amb_spinand_read_buf;
	chip->select_chip = amb_spinand_select_chip;
	chip->waitfunc = amb_spinand_waitfunc;
	chip->cmdfunc = amb_spinand_cmdfunc;
	chip->options |= (NAND_CACHEPRG | NAND_NO_SUBPAGE_WRITE);
	//chip->options |= NAND_CACHEPRG;
	chip->priv = flash;
	chip->bbt_options |= NAND_BBT_USE_FLASH | NAND_BBT_NO_OOB;

	flash->mtd.priv = chip;
	flash->mtd.name = "amba_spinand";
	flash->mtd.owner = THIS_MODULE;
	flash->mtd.writebufsize = flash->mtd.writesize;
	flash->mtd.type = MTD_NANDFLASH;

	return 0;
}

static int ambarella_spinand_init_chipecc(struct amb_spinand *flash)
{
	int ret = 0;

	struct nand_chip *chip = &flash->chip;

	chip->ecc.mode	= NAND_ECC_HW;
	chip->ecc.size	= 0x200;
	chip->ecc.bytes	= 16;
	chip->ecc.steps	= 0x4;

	chip->ecc.strength = 8;
	chip->ecc.total	= chip->ecc.steps * chip->ecc.bytes;
	chip->ecc.layout = &ecc_layout_2KB_8bit;
	chip->ecc.read_page = spinand_read_page_hwecc;
	chip->ecc.write_page = spinand_write_page_hwecc;
	ret = spinand_enable_ecc(flash);

	return ret;
}

static int ambarella_spinand_probe(struct platform_device *pdev)
{
	struct mtd_info	*mtd;
	struct mtd_part_parser_data	ppdata;
	struct amb_spinand	*flash;
	struct spinand_state *state;
	int	errCode = 0;

	flash = kzalloc(sizeof(struct amb_spinand), GFP_KERNEL);
	if (!flash) {
		errCode = -ENOMEM;
		goto ambarella_spinand_probe_exit;
	}

	state = kzalloc(sizeof(struct spinand_state), GFP_KERNEL);
	if (!state) {
		errCode = -ENOMEM;
		goto ambarella_spinand_probe_free_flash;
	}
	flash->priv = state;
	state->buf_ptr = 0;

    mutex_init(&flash->lock);
    platform_set_drvdata(pdev, flash);
    flash->dev = &pdev->dev;

	/* set 50Mhz as default spi clock */
	flash->clk = 50000000;
    ambarella_spinand_get_resource(flash, pdev);
    ambspinand_init(flash);

	ambarella_spinand_init_chipecc(flash);
	ambarella_spinand_init_chip(flash, pdev->dev.of_node);

	mtd = &flash->mtd;

	flash->command = kzalloc(5, GFP_KERNEL);
    if(!flash->command) {
        dev_err((const struct device *)&flash->dev,
            "SPI NAND driver malloc command error\r\n");
        errCode = -ENOMEM;
        goto ambarella_spinand_probe_free_state;
    }
	//flash->clk = info->max_clk_hz;
    ambspinand_init(flash);
    ambspi_dma_config(flash);

	spinand_set_protection(flash, SPI_NAND_PROTECTED_ALL_UNLOCKED);

	if (nand_scan(mtd, 1))
		return -ENXIO;

	ppdata.of_node = pdev->dev.of_node;
	errCode = mtd_device_parse_register(&flash->mtd, NULL, &ppdata, NULL, 0);
	if (errCode < 0)
		goto ambarella_spinand_probe_free_command;

    printk("AMBARELLA SPINAND probed \n");
    return 0;

ambarella_spinand_probe_free_command:
    kfree(flash->command);
ambarella_spinand_probe_free_state:
	kfree(state);
ambarella_spinand_probe_free_flash:
    kfree(flash);
ambarella_spinand_probe_exit:
    return errCode;
}


static int ambarella_spinand_remove(struct platform_device *pdev)
{
	struct amb_spinand    *flash = platform_get_drvdata(pdev);
	int        status;

	if (flash) {
		/* Clean up MTD stuff. */
		status = mtd_device_unregister(&flash->mtd);
		if (status == 0) {
			kfree(flash->command);
			dma_free_coherent(flash->dev,
				AMBA_SPINOR_DMA_BUFF_SIZE,
				flash->dmabuf, flash->dmaaddr);
			kfree(flash);
		}
	}
    return 0;
}

static void ambarella_spinand_shutdown(struct platform_device *pdev)
{
	int ret;
	struct amb_spinand    *flash = platform_get_drvdata(pdev);

	/* Wait until finished previous write command. */
	ret = wait_execution_complete(flash, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0) {
			dev_err(flash->dev, "%s: Wait execution complete failed!\n",
					__func__);
			return;
		} else
			dev_err(flash->dev, "%s: Wait execution complete timedout!\n",
					__func__);
	}
	/* Workaround for the spinand software reboot */
	spinand_read_page_to_cache(flash, 0);
	ret = wait_execution_complete(flash, MAX_WAIT_JIFFIES);
	if (ret) {
		if (ret < 0)
			dev_err(flash->dev, "%s: Wait execution complete failed!\n",
					__func__);
		else
			dev_err(flash->dev, "%s: Wait execution complete timedout!\n",
					__func__);
	}
	return;
}

static const struct of_device_id ambarella_spinand_of_match[] = {
    {.compatible = "ambarella,spinand", },
    {},
};
MODULE_DEVICE_TABLE(of, ambarella_spinand_of_match);

static struct platform_driver amb_spinand_driver = {
	.probe = ambarella_spinand_probe,
	.remove = ambarella_spinand_remove,
	.driver = {
		.name = "ambarella-spinand",
		.owner = THIS_MODULE,
		.of_match_table = ambarella_spinand_of_match,
	},
	.shutdown = ambarella_spinand_shutdown,
};

module_platform_driver(amb_spinand_driver);

MODULE_AUTHOR("Ken He");
MODULE_DESCRIPTION("Ambarella Media processor SPI NAND Flash Controller Driver");
MODULE_LICENSE("GPL");

