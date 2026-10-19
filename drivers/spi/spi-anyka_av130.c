/**
*  /driver/spi/spi_anyka.c

*  Copyright (C) 2018 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
*  Author: Zhipeng Zhang <zhang_zhipeng@anyka.com>
*
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/spinlock.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/proc_fs.h>
#include <linux/reset.h>

#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <mach/anyka_types.h>
#include <mach/ak_l2.h>
#include <linux/dma-mapping.h>
#include "ak39_spi_av130.h"

/*usually use for spi keyboard or spi mouse.*/
//#define SPI_CPU_MODE_USE_INTERRUPT

#define MHz (1000*1000)

/**
 * ak_spi_devstate - per device data
 * @hz: Last frequency calculated for @sppre field.
 * @mode: Last mode setting for the @spcon field.
 * @spcon: Value to write to the SPCON register.
 * @sppre: Value to write to the SPIINT register.
 */
struct ak_spi_devstate {
    unsigned int    hz;
    u16     mode;
    u32     spcon;
    u8      spint;
    u8      initialed;
};

struct ak_spi {
    struct completion    done;
    struct spi_master   *master;
    struct spi_device   *curdev;
    struct device       *dev;

    void __iomem        *regs;
    struct clk          *clk;
    struct clk          *bus_clk;
    /* spi controller need a DMA memory.*/
    void *txbuffer;
    dma_addr_t txdma_buffer;

    void *rxbuffer;
    dma_addr_t rxdma_buffer;

    struct resource     *ioarea;
    int                 irq;
    int                 len;            /*need transfer len*/
    int                 count;          /*have transferred len*/
    unsigned char       cap_highspeed;  /*spi high-speed controller flag.*/
    
    u8                  l2buf_tid;
    u8                  l2buf_rid;

    /* data buffers */
    unsigned char       *tx;
    unsigned char       *rx;
    int                 xfer_dir;
    int                 xfer_mode; /*use for dma or cpu*/
    int                 bus_freq;

    struct proc_dir_entry* spi_proc;
    struct reset_control *rstc;

     unsigned int is_dma_mapped;
};

enum spi_xfer_dir {
    SPI_DIR_TX,
    SPI_DIR_RX,
    SPI_DIR_TXRX, 
    SPI_DIR_XFER_NUM,
};
#define TRANS_TIMEOUT           (10000)
#define MAX_XFER_LEN            SPI_DMA_MAX_LEN
#define SPI_TRANS_TIMEOUT       (5000)
#define SPI_DMA_MAX_LEN         (8192)

#define DFT_CON             (AK_SPICON_EN | AK_SPICON_MS)
#define DFT_DIV             (1) //5 /*127*/
#define DFT_BIT_PER_WORD    (8)
#define FORCE_CS            (1 << 5)
#if defined(CONFIG_MACH_AK37E) || defined(CONFIG_MACH_AK3918AV100) \
    || defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)
#define ENABLE_CS           (1 << 18)
#endif
#define SPPIN_DEFAULT       (0)

#define CS_ACTIVE   1   /* normally nCS, active low */
#define CS_INACTIVE 0


/**
*  @brief       print the value of registers related spi bus.
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   *hw
*  @return      void
*/
static void dbg_dumpregs(struct ak_spi *hw)
{
    pr_debug("\n");
    pr_debug("CON: \t0x%x\n", ioread32(hw->regs + AK_SPICON));
    pr_debug("STA: \t0x%x\n", ioread32(hw->regs + AK_SPISTA));
    pr_debug("INT: \t0x%x\n", ioread32(hw->regs + AK_SPIINT));
    pr_debug("CNT: \t0x%x\n", ioread32(hw->regs + AK_SPICNT));
    pr_debug("DOUT: \t0x%x\n", ioread32(hw->regs + AK_SPIOUT));
    pr_debug("DIN: \t0x%x\n", ioread32(hw->regs + AK_SPIIN));
}

static inline void dbg_dumpdata(struct ak_spi *hw,
        void *data, int size)
{
    int ii;
    int dsize = (size +3)/4;
    u32 *dptr = data;
    
    printk("xfer data (size:%d):", size);
    
    for(ii = 0; ii < dsize; ii++) {
        if((ii%10) == 0)
            printk("\n");
        
        printk("%08x ", *(dptr + ii));
    }
    printk("\n");
}


#define AKSPI_DATA_WIRE(mode)   \
    (((mode & SPI_NBITS_QUAD) == SPI_NBITS_QUAD) ?  \
    SPI_NBITS_QUAD:((mode & SPI_NBITS_DUAL) == SPI_NBITS_DUAL) ?  \
    SPI_NBITS_DUAL : SPI_NBITS_SINGLE)

#if !defined(CONFIG_MACH_KM01A) && !defined(CONFIG_MACH_AK3918AV130)
#define SPI_L2_TXADDR(m)   \
    ((m->bus_num == AKSPI_BUS_NUM0) ? ADDR_SPI0_TX : \
    ((m->bus_num == AKSPI_BUS_NUM1) ? ADDR_SPI1_TX : ADDR_SPI2_TX))

#define SPI_L2_RXADDR(m)   \
    ((m->bus_num == AKSPI_BUS_NUM0) ? ADDR_SPI0_RX : \
    ((m->bus_num == AKSPI_BUS_NUM1) ? ADDR_SPI1_RX : ADDR_SPI2_RX))
#else
#define SPI_L2_TXADDR(m)   \
    (((m->bus_num == AKSPI_BUS_NUM1) ? ADDR_SPI1_TX : ADDR_SPI2_TX))

#define SPI_L2_RXADDR(m)   \
    (((m->bus_num == AKSPI_BUS_NUM1) ? ADDR_SPI1_RX : ADDR_SPI2_RX))
#endif

static inline struct ak_spi *to_hw(struct spi_device *sdev)
{
    return spi_master_get_devdata(sdev->master);
}

/**
*  @brief       hw_txbyte
*  TODO: send one bytes
*  @author      lixinhai
*  @date        2013-03-19
*  @param[in]   *hw
*  @param[in]   count
*  @return      unsigned int
*/
static inline unsigned int hw_txbyte(struct ak_spi *hw, int len)
{
    u32 val = 0;
    int i = 0;
    
    while (i < len)
    {
        val |= (hw->tx[hw->count+i] << i*8);
        i++;
    }                       
    return val;
}

/**
*  @brief       hw_txdword
*  TODO: send double words
*  @author      lixinhai
*  @date        2013-03-19
*  @param[in]   *hw
*  @param[in]   count
*  @return      unsigned int
*/
static unsigned int hw_txdword(struct ak_spi *hw)
{
    u32 val = 0;
    int l = 0;
    
    l = (hw->len - hw->count) > 4 ? 4 : (hw->len - hw->count);

    val = hw_txbyte(hw, l);
                    
    hw->count += l;
    pr_debug("[%08x] ", val);
    return val;
}

/**
*  @brief       hw_rxbyte
*  TODO: recv one bytes
*  @author      lixinhai
*  @date        2013-03-19
*  @param[in]   count
*  @return      unsigned int
*/
static inline void hw_rxbyte(struct ak_spi *hw, unsigned int val, int len)
{
    int i = 0;
    
    while (i < len)
    {
        hw->rx[hw->count + i] = (val >> i*8) & 0xff;
        i++;
    }
}

/**
*  @brief       hw_rxbyte
*  TODO: double words
*  @author      lixinhai
*  @date        2013-03-19
*  @param[in]   count
*  @return      unsigned int
*/
static void hw_rxdword(struct ak_spi *hw, unsigned int val)
{
    int l = 0;
    
    l = (hw->len - hw->count) > 4 ? 4 : (hw->len - hw->count);
    
    hw_rxbyte(hw, val, l);
    hw->count += l; 
    pr_debug("[%08x] ", val);
}

static inline u32 hw_remain_datalen(struct ak_spi *hw)
{
    return (hw->len - hw->count);
}

static inline bool ak_spi_use_dma(struct ak_spi *hw)
{
    return (hw->xfer_mode == AKSPI_XFER_MODE_DMA);
}

static inline int wait_for_spi_cnt_to_zero(struct ak_spi *hw, u32 timeout)
{
    do {             
        if (readl(hw->regs + AK_SPICNT) == 0)
            break;
        udelay(1);
    }while(timeout--);
    
    return (timeout > 0) ? 0 : -EBUSY;
}

static void ak_spi_set_cs(struct ak_spi *hw, int cs, int pol)
{
#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330)
    BUG_ON(cs >= 1);
#endif
#if defined(CONFIG_MACH_AK37E) || defined(CONFIG_MACH_AK3918AV100) \
    || defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)
    BUG_ON(cs >= 2);
#endif
}

/**
* select one spi device by cs signal.
*/
static void ak_spi_chipsel(struct spi_device *spi, int value)
{
    struct ak_spi_devstate *cs = spi->controller_state;
    struct ak_spi *hw = to_hw(spi);
    unsigned int cspol = (spi->mode & SPI_CS_HIGH) ? 1 : 0;
    
    /* change the chipselect state and the state of the spi engine clock */ 
    switch (value) {
        case CS_INACTIVE:
            if(spi->chip_select == AKSPI_ONCHIP_CS) {
        #if defined(CONFIG_MACH_AK37E)
                /* enable cs0*/
                cs->spcon = ioread32(hw->regs + AK_SPICON);
                cs->spcon &= ~ENABLE_CS;
                iowrite32(cs->spcon, hw->regs + AK_SPICON);
        #elif defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A) || \
                defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130)
                if (spi->master->bus_num != AKSPI_BUS_NUM0) {
                    /* enable cs0*/
                    cs->spcon = ioread32(hw->regs + AK_SPICON);
                    cs->spcon &= ~ENABLE_CS;
                    iowrite32(cs->spcon, hw->regs + AK_SPICON);
                }
        #endif
            }
        #if defined(CONFIG_MACH_AK37E)
            else if(spi->chip_select == AKSPI_ONCHIP_CS1) {
                /* enable cs1*/
                cs->spcon = ioread32(hw->regs + AK_SPICON);
                cs->spcon |= ENABLE_CS;
                iowrite32(cs->spcon, hw->regs + AK_SPICON);
            }
        #elif defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A) || \
                defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130)
           else if ((spi->chip_select == AKSPI_ONCHIP_CS1) && 
                (spi->master->bus_num == AKSPI_BUS_NUM2)) {
               /* enable cs1*/
               cs->spcon = ioread32(hw->regs + AK_SPICON);
               cs->spcon |= ENABLE_CS;
               iowrite32(cs->spcon, hw->regs + AK_SPICON);
           }
        #endif
            else
                ak_spi_set_cs(hw, spi->chip_select, cspol^1);

            cs->spcon = ioread32(hw->regs + AK_SPICON);
            cs->spcon &= ~FORCE_CS;
            iowrite32(cs->spcon, hw->regs + AK_SPICON);

            udelay(2);
            break;
        case CS_ACTIVE:
            udelay(2);
            
            if(spi->chip_select == AKSPI_ONCHIP_CS) {
        #if defined(CONFIG_MACH_AK37E)
                /* enable cs0*/
                cs->spcon = ioread32(hw->regs + AK_SPICON);
                cs->spcon &= ~ENABLE_CS;
                iowrite32(cs->spcon, hw->regs + AK_SPICON);
        #elif defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A) || \
            defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130)
                if (spi->master->bus_num != AKSPI_BUS_NUM0) {
                    /* enable cs0*/
                    cs->spcon = ioread32(hw->regs + AK_SPICON);
                    cs->spcon &= ~ENABLE_CS;
                    iowrite32(cs->spcon, hw->regs + AK_SPICON);
                }
        #endif
            }
        #if defined(CONFIG_MACH_AK37E)
            else if(spi->chip_select == AKSPI_ONCHIP_CS1) {
                /* enable cs1*/
                cs->spcon = ioread32(hw->regs + AK_SPICON);
                cs->spcon |= ENABLE_CS;
                iowrite32(cs->spcon, hw->regs + AK_SPICON);
            }
        #elif defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_KM01A) || \
            defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130)
           else if ((spi->chip_select == AKSPI_ONCHIP_CS1) && 
                (spi->master->bus_num == AKSPI_BUS_NUM2)) {
               /* enable cs1*/
               cs->spcon = ioread32(hw->regs + AK_SPICON);
               cs->spcon |= ENABLE_CS;
               iowrite32(cs->spcon, hw->regs + AK_SPICON);
           }
        #endif
            else
                ak_spi_set_cs(hw, spi->chip_select, cspol);

            cs->spcon = ioread32(hw->regs + AK_SPICON);
            cs->spcon |= FORCE_CS;
            iowrite32(cs->spcon, hw->regs + AK_SPICON);

            break;
        default:
            break;
    }
}
/*end of ak_spi_chipsel*/

/**
*  @brief       update the device's state
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   *spi
*  @param[in]   *t
*  @return      int
*/
static int ak_spi_update_state(struct spi_device *spi,
                    struct spi_transfer *t)
{
    struct ak_spi_devstate *cs = spi->controller_state;
    unsigned int bpw;
    unsigned int hz, typical_hz;
    unsigned int div = 0;
    struct ak_spi *hw = to_hw(spi);
#ifdef CONFIG_MACH_AK3918AV100_FPGA
    //FPGA use default clock 60M
    unsigned long clk = 60000000;
    bpw = t ? t->bits_per_word : spi->bits_per_word;
    //FPGA use default clock 10M
    typical_hz  = 10000000;
#else
    unsigned long clk = clk_get_rate(hw->clk);
    bpw = t ? t->bits_per_word : spi->bits_per_word;
    if(spi->master->bus_num == 0){
        spi->max_speed_hz = spi->max_speed_hz > hw->bus_freq ?
                            hw->bus_freq : spi->max_speed_hz;
    }
    typical_hz  = t ? t->speed_hz : spi->max_speed_hz;
#endif
    
    if (!bpw)
        bpw = DFT_BIT_PER_WORD;

    if (bpw != DFT_BIT_PER_WORD) {
        dev_err(&spi->dev, "invalid bits-per-word (%d)\n", bpw);
        return -EINVAL;
    }

    /*spi mode change, config again*/
    if (spi->mode != cs->mode) {
        cs->spcon &= ~(AK_SPICON_CPHA|AK_SPICON_CPOL);
        
        if ((spi->mode & SPI_CPHA) == SPI_CPHA)
            cs->spcon |= AK_SPICON_CPHA;

        if ((spi->mode & SPI_CPOL) == SPI_CPOL)
            cs->spcon |= AK_SPICON_CPOL;

        cs->mode = spi->mode;
    }


    /*spi1 controller bus clock need this,but spi0 don't need. */
    if ((cs->hz != typical_hz)&& (!hw->cap_highspeed) ) {
        if (!typical_hz)
            typical_hz = spi->max_speed_hz;

        if (typical_hz > 50*MHz) {
            pr_info("%s, spi1/2 speed_hz %d is larger than max_speed_hz 50MHz, now using 50MHz.\n", __func__, typical_hz);
            typical_hz = 50*MHz;
        }

        div = clk / (typical_hz*2) - 1;

        if (div > 255)
              div = 255;

        if (div < 0)
              div = 0;

        hz = clk /((div+1)*2);

        /*when got clock greater than wanted clock, increase divider*/
        if((hz - typical_hz) > 0)
            div++;

        cs->hz = hz;
        cs->spcon &= ~AK_SPICON_CLKDIV;
        cs->spcon |= div << 8;

        pr_info("spi%d new hz is %u, div is %u(%s).\n",
                hw->master->bus_num, cs->hz, div, div ?"change":"not change");

    }

    if ( (cs->hz != typical_hz) && (hw->cap_highspeed) ){
        cs->hz = typical_hz;
        pr_info("spi%d hz is %u\n", hw->master->bus_num, cs->hz);
    }
    return 0;
}

/**
*  @brief       setup one transfer
*  setup_transfer() changes clock and/or wordsize to match settings
*  for this transfer; zeroes restore defaults from spi_device.
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   *spi
*  @param[in]   *t
*  @return      int
*/
static int ak_spi_setupxfer(struct spi_device *spi,
                 struct spi_transfer *t)
{
    struct ak_spi_devstate *cs = spi->controller_state;
    struct ak_spi *hw = to_hw(spi);
    int ret=0;

    /*spi device not change and has been initilize, return.*/
    if(likely((cs->initialed == 1) && (spi == hw->curdev)))
        return ret;

    cs->spcon = readl(hw->regs + AK_SPICON);

    ret = ak_spi_update_state(spi, t);
    if (!ret)
    {
        /*Add code to fix the issue that the SPICON register 
              was deranged. Shaohua @2012-3-23             
         */
        writel(cs->spcon, hw->regs + AK_SPICON);
        //printk("ak_spi_setupxfer,con:%08x.\n", cs->spcon);
        cs->initialed = 1;
        hw->curdev = spi;
    }

    return ret;
}

#define MODEBITS (SPI_CPOL | SPI_CPHA | SPI_CS_HIGH)

/**
*  @brief       setup and updates the device mode,state.
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   *spi
*  @return      int
*/
static int ak_spi_setup(struct spi_device *spi)
{
    struct ak_spi_devstate *cs = spi->controller_state;
    struct ak_spi *hw = to_hw(spi);
    int ret;

    /* allocate settings on the first call */
    if (!cs) {
        cs = kzalloc(sizeof(struct ak_spi_devstate), GFP_KERNEL);
        if (!cs) {
            dev_err(&spi->dev, "no memory for controller state\n");
            return -ENOMEM;
        }

        cs->spcon = DFT_CON;
        cs->hz = -1;
        spi->controller_state = cs;
        cs->initialed = 0;
    }

    if (spi->mode & ~(hw->master->mode_bits)) {
        dev_dbg(&spi->dev, "setup: unsupported mode bits %x\n",
            spi->mode & ~(hw->master->mode_bits));
        return -EINVAL;
    }
    /* initialise the state from the device,and spi bus clock. */
    ret = ak_spi_update_state(spi, NULL);

    if (!ret)
    {
        writel(cs->spcon, hw->regs + AK_SPICON);

        cs->initialed = 1;
        hw->curdev = spi;
    }
    return ret;
}

static void ak_spi_cleanup(struct spi_device *spi)
{
    kfree(spi->controller_state);
}

static inline u32 enable_imask(struct ak_spi *hw, u32 imask)
{
    u32 newmask;

    newmask = readl(hw->regs + AK_SPIINT);
    newmask |= imask;

    writel(newmask, hw->regs + AK_SPIINT);

    return newmask;
}

static inline u32 disable_imask(struct ak_spi *hw, u32 imask)
{
    u32 newmask;

    newmask = readl(hw->regs + AK_SPIINT);
    newmask &= ~imask;

    writel(newmask, hw->regs + AK_SPIINT);

    return newmask;
}


/**
*  @brief       configure the master register when start transfer data.
*  @author      lixinhai
*  @date        2013-03-15
*  @param[in]  *hw :akspi master dev.
*  @param[in]   t: a read/write buffer pair.
*  @return      void
*/
static void ak_spi_start_txrx(struct ak_spi *hw, struct spi_transfer *t)
{
    u32 reg_value;
    enum spi_xfer_dir dir = hw->xfer_dir;
        
    pr_debug("the spi transfer mode is %s.\n", (dir == SPI_DIR_TXRX) ? 
                "txrx" : (dir == SPI_DIR_RX)? "rx":"tx");
    
    reg_value = readl(hw->regs + AK_SPICON);

    switch(dir) {
        case SPI_DIR_TX:
            reg_value &= ~AK_SPICON_TGDM;
            reg_value |= AK_SPICON_ARRM;
            break;
        case SPI_DIR_RX:
            reg_value |= AK_SPICON_TGDM;
            reg_value &= ~AK_SPICON_ARRM;
            break;
        case SPI_DIR_TXRX:
            reg_value &= ~AK_SPICON_TGDM;
            reg_value &= ~AK_SPICON_ARRM;
            break;
        default:
            break;
    }

    /*configure the data wire*/
    reg_value &= ~AK_SPICON_WIRE;

    if(t->tx_nbits == SPI_NBITS_QUAD || t->rx_nbits == SPI_NBITS_QUAD)
    {
        reg_value |= 0x2<<16;
    }
    else if(t->tx_nbits == SPI_NBITS_DUAL || t->rx_nbits == SPI_NBITS_DUAL)
    {
        reg_value |= 0x1<<16;
    }
    else
    {
        reg_value |= 0x0<<16;
    }

    writel(reg_value, hw->regs + AK_SPICON);
}


/**
*  @brief       configure the master register when stop transfer data.
*  @author      lixinhai
*  @date        2013-03-15
*  @param[in]  *hw :akspi master dev.
*  @param[in]   t: a read/write buffer pair.
*  @return      void
*/
static void ak_spi_stop_txrx(struct ak_spi *hw, struct spi_transfer *t)
{
    u32 reg_value;

    reg_value = readl(hw->regs + AK_SPICON);
    reg_value &= ~AK_SPICON_WIRE;   
    writel(reg_value, hw->regs + AK_SPICON);
}

/* spi write data function by l2dma mode.
*  data from l2 buffer to DMA buffer.
*/
static int spi_dma_write(struct ak_spi *hw, unsigned char *buf, unsigned char *dma_buf, int count)
{
    int ret = 0;
    bool flags = false;
    u32 val;

    init_completion(&hw->done); 
    enable_imask(hw, AK_SPIINT_TRANSF);
    
    val = AK_SPIEXTX_BUFEN|AK_SPIEXTX_DMAEN;
    iowrite32(val, hw->regs + AK_SPIEXTX);
    iowrite32(count, hw->regs + AK_SPICNT);

    /*use for dma mode: greater than 256 bytes, align 4bytes of buf addr,
        align 64 bytes of data count*/
    if((count < 256) || ((unsigned long)buf & 0x3) || (count & (64 - 1))) {
        l2_combuf_cpu((unsigned long)buf, hw->l2buf_tid, count, MEM2BUF);
        
    } else {
        /*hw->is_dma_mapped == 1用于spi lcd*/
        if(hw->is_dma_mapped == 1 && dma_buf != NULL) {
            /*start l2 dma transmit*/
            l2_combuf_dma((unsigned long) dma_buf, hw->l2buf_tid,
                        count, MEM2BUF, AK_FALSE);
        }else{
            /*data from mtd cache to dma buffer */
            memcpy(hw->txbuffer, buf, count);

            /*start l2 dma transmit*/
            l2_combuf_dma(hw->txdma_buffer, hw->l2buf_tid,
                        count, MEM2BUF, AK_FALSE);
        }
        flags = true;
    }
    ret = wait_for_completion_timeout(&hw->done,
                    msecs_to_jiffies(SPI_TRANS_TIMEOUT));
    if(ret <= 0) {
        pr_err("wait for spi transfer interrupt timeout(%s).\n", __func__);
        dbg_dumpregs(hw);
        ret = -EINVAL;
        goto xfer_fail;
    }

    if (flags && (l2_combuf_wait_dma_finish(hw->l2buf_tid) == AK_FALSE))    {
        pr_err("%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        ret = -EINVAL;
        goto xfer_fail;
    }
    
    ret = wait_for_spi_cnt_to_zero(hw, TRANS_TIMEOUT);
    if(ret) {
        pr_err("%s: wait_for_spi_cnt_to_zero failed!\n", __func__);
        ret = -EINVAL;
        goto xfer_fail;
    }
        
    ret = count;
xfer_fail:

    //disable l2 dma
    iowrite32(0, hw->regs + AK_SPIEXTX);
    l2_clr_status(hw->l2buf_tid);
    return ret;
}

/* spi read data function by l2dma mode.
*  data from l2 buffer to DMA buffer.
*/
static int spi_dma_read(struct ak_spi *hw, unsigned char *buf, int count)
{
    int ret = 0;
    bool flags = false;
    u32 val;
    
    /*prepare spi read*/
    init_completion(&hw->done);
    enable_imask(hw, AK_SPIINT_TRANSF);
    
    val = AK_SPIEXRX_BUFEN|AK_SPIEXRX_DMAEN;
    iowrite32(val, hw->regs + AK_SPIEXRX);
    iowrite32(count, hw->regs + AK_SPICNT); 

    if(count < 256 || ((unsigned long)buf & 0x3) || (count & (64 - 1))) {
        l2_combuf_cpu((unsigned long)buf, hw->l2buf_rid, count, BUF2MEM);
    } 
    else {
        /* data from l2 buffer to dma buffer*/
        l2_combuf_dma(hw->rxdma_buffer, hw->l2buf_rid,
            count, BUF2MEM, AK_FALSE);
        flags = true;
    }

    ret = wait_for_completion_timeout(&hw->done,
            msecs_to_jiffies(SPI_TRANS_TIMEOUT));
    if(ret <= 0) {
        pr_err("wait for spi transfer interrupt timeout(%s).\n", __func__);
        dbg_dumpregs(hw);
        ret = -EINVAL;
        goto xfer_fail;
    }

    /*wait L2 dma finish, if need frac dma,start frac dma*/
    if (flags && l2_combuf_wait_dma_finish(hw->l2buf_rid) ==  AK_FALSE) {
        pr_err("%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        ret = -EINVAL;
        goto xfer_fail;
    }

    /*wait for spi count register value to zero.*/
    ret = wait_for_spi_cnt_to_zero(hw, TRANS_TIMEOUT);
    if(ret) {
        pr_err("%s: wait for spi count to zero failed!\n", __func__);
        ret = -EINVAL;
        goto xfer_fail;
    }

    if(flags == true){
        memcpy(buf, hw->rxbuffer, count);
    }
    ret = count;
xfer_fail:
    //disable l2 dma
    iowrite32(0, hw->regs + AK_SPIEXRX);
    l2_clr_status(hw->l2buf_rid);
    return ret;
}

/* spi read/write date function by l2dma mode*/
static int spi_dma_duplex(struct ak_spi *hw, 
            unsigned char *tx, unsigned char *rx, int count)
{
    int ret = 0;
    bool flags = false;
    u32 val;

    init_completion(&hw->done); 
    enable_imask(hw, AK_SPIINT_TRANSF);
    
    val = AK_SPIEXTX_BUFEN|AK_SPIEXTX_DMAEN;
    iowrite32(val, hw->regs + AK_SPIEXTX);

    val = AK_SPIEXRX_BUFEN|AK_SPIEXRX_DMAEN;
    iowrite32(val, hw->regs + AK_SPIEXRX);

    iowrite32(count, hw->regs + AK_SPICNT);

    if((count < 512) || ((unsigned long)tx & 0x3)) {
        
        l2_combuf_cpu((unsigned long)tx, hw->l2buf_tid, count, MEM2BUF);
        l2_combuf_cpu((unsigned long)rx, hw->l2buf_rid, count, BUF2MEM);
    }
    else {
        /* data from mtd cache to dma buffer.*/
        memcpy(hw->txbuffer, tx, count);
        /* data from dma buffer to l2 buffer.*/
        l2_combuf_dma(hw->txdma_buffer, hw->l2buf_tid,
                count, MEM2BUF, AK_FALSE);
        /* data from l2 buffer to dma buffer.*/
        l2_combuf_dma(hw->rxdma_buffer, hw->l2buf_rid,
                count, BUF2MEM, AK_FALSE);
        flags = true;
    }
    ret = wait_for_completion_timeout(&hw->done,
            msecs_to_jiffies(SPI_TRANS_TIMEOUT));
    if(ret <= 0) {
        pr_err("wait for spi transfer interrupt timeout(%s).\n", __func__);
        dbg_dumpregs(hw);
        ret = -EINVAL;
        goto xfer_fail;
    }

    if (flags && ((AK_FALSE == l2_combuf_wait_dma_finish(hw->l2buf_tid)) || 
        (AK_FALSE == l2_combuf_wait_dma_finish(hw->l2buf_rid))))
    {
        pr_err("%s: l2_combuf_wait_dma_finish failed!\n", __func__);
        ret = -EINVAL;
        goto xfer_fail;
    }
    
    ret = wait_for_spi_cnt_to_zero(hw, TRANS_TIMEOUT);
    if(ret) {
        pr_err("%s: wait for spi count to zero failed!\n", __func__);
        ret = -EINVAL;
        goto xfer_fail;
    }

    if(flags == true) {
        memcpy(rx, hw->rxbuffer, count);
    }
    ret = 0;

xfer_fail:
    //disable l2 dma
    iowrite32(0, hw->regs + AK_SPIEXTX);
    iowrite32(0, hw->regs + AK_SPIEXRX);
    l2_clr_status(hw->l2buf_tid);
    l2_clr_status(hw->l2buf_rid);
    return ret;
}


/**
*  @brief       spi transfer function by l2dma/l2cpu mode, actual worker 
*               spi_dma_read()/spi_dma_write() to be call.
*  @author      lixinhai
*  @date        2013-03-15
*  @param[in]  *hw :akspi master dev.
*  @param[in]   dir: transfer direction.
*  @return      transfer success result 0, otherwise result a negative value
*/
static int ak_spi_dma_txrx(struct ak_spi *hw, struct spi_transfer *t)
{
    int ret = 0;
    int retlen;
    u32 count = t->len;
    
    hw->tx = (unsigned char*)t->tx_buf;
    hw->rx = t->rx_buf;
    hw->l2buf_tid = hw->l2buf_rid = BUF_NULL;
    pr_debug("start the spi dma transfer.\n");

    switch(hw->xfer_dir) {
        case SPI_DIR_TXRX:
        {   
            /*alloc L2 buffer*/
            hw->l2buf_tid = l2_alloc(SPI_L2_TXADDR(hw->master));
            hw->l2buf_rid = l2_alloc(SPI_L2_RXADDR(hw->master));

            if ((BUF_NULL == hw->l2buf_tid) || (BUF_NULL == hw->l2buf_rid))
            {
                printk("%s: l2_alloc failed!\n", __func__);
                ret = -EBUSY;
                goto txrx_ret;
            }
            
            while(count > 0) {
                hw->count = 0;
                hw->len = (count > MAX_XFER_LEN) ?  MAX_XFER_LEN : count;
                
                retlen = spi_dma_duplex(hw, hw->tx, hw->rx, hw->len);
                if(unlikely(retlen < 0)) {
                    printk("spi master transfer data error!\n");
                    ret = -EBUSY;
                    goto txrx_ret;
                }
                hw->tx += retlen;
                hw->rx += retlen;
                count -= retlen;
            }
            break;
        }
        case SPI_DIR_TX:
        {
            /*alloc L2 buffer*/
            hw->l2buf_tid = l2_alloc(SPI_L2_TXADDR(hw->master));
            if (unlikely(BUF_NULL == hw->l2buf_tid)) {
                printk("%s: l2_alloc failed!\n", __func__);
                return -EBUSY;
            }
            hw->count = 0;
            while(count > 0) {
                hw->len = (count > MAX_XFER_LEN) ? MAX_XFER_LEN : count;
                
                retlen = spi_dma_write(hw, hw->tx + hw->count,
                                           (unsigned char*)t->tx_dma + hw->count,  hw->len);
                if(unlikely(retlen < 0)) {
                    printk("spi master read data error!\n");    
                    ret = -EBUSY;
                    goto txrx_ret;
                }
                hw->tx += retlen;
                count -= retlen;
                hw->count += retlen;
            }
            break;
        }
        case SPI_DIR_RX:
        {
            //alloc L2 buffer
            hw->l2buf_rid = l2_alloc(SPI_L2_RXADDR(hw->master));
            if (unlikely(BUF_NULL == hw->l2buf_rid))
            {
                return -EBUSY;
            }
            
            while(count > 0) {
                hw->count = 0;
                hw->len = (count > MAX_XFER_LEN) ? MAX_XFER_LEN : count;
                
                retlen = spi_dma_read(hw, hw->rx, hw->len);
                if(unlikely(retlen < 0)) {
                    printk("spi master read data error!\n");
                    ret = -EBUSY;
                    goto txrx_ret;
                }
                hw->rx += retlen;
                count -= retlen;
            }       
            break;
        }
    }
    pr_debug("finish the spi dma transfer.\n");
txrx_ret:
    if(hw->l2buf_tid != BUF_NULL)
        l2_free(SPI_L2_TXADDR(hw->master));
    if(hw->l2buf_rid != BUF_NULL)
        l2_free(SPI_L2_RXADDR(hw->master));

    return ret ? ret : t->len;
}
/*end of ak_spi_dma_txrx*/

/**
*  @brief       spi read/write data function by cpu mode, 
*  @author      lixinhai
*  @date        2013-03-15
*  @param[in]  *hw :akspi master dev.
*  @param[in]   t: a read/write buffer pair.
*  @return      transfer success result 0, otherwise result a negative value
*/
static int ak_cpu_duplex(struct spi_device *spi, struct spi_transfer *t)
{
    struct ak_spi *hw = to_hw(spi);
    u32 tran_4_nbr;
    u32 frac_nbr;
    u32 status, val;
    u32 off_set_read = 0, off_set_write = 0;
    const u8 *buff_tx;
    u8 *buff_rx;
    int i = 0, j;
    u32 to_read = 0, to_write = 0;

    hw->len = (t->len > MAX_XFER_LEN) ? MAX_XFER_LEN : t->len;
    tran_4_nbr = hw->len/4;
    frac_nbr = hw->len%4;

    hw->tx = (unsigned char*)t->tx_buf;
    hw->rx = t->rx_buf;

    buff_tx = hw->tx;
    buff_rx = hw->rx;

    pr_debug("duplex transfer,tran_4_nbr:%u  frac_nbr:%u\n",
                tran_4_nbr, frac_nbr);
    
    if (hw->len >= MAX_XFER_LEN)
    {
        printk("Too much to be read and send...\n");
        return -EINVAL;
    }

    /*set data count, and the the master will rise clk*/
    writel(hw->len, hw->regs + AK_SPICNT);
    
    while(1) {
        /*write 4 bytes first, and then read 4 bytes*/
        if (i<tran_4_nbr) {
            while(1) {
                status = ioread32(hw->regs + AK_SPISTA);
                if ((status & AK_SPISTA_TXHEMP) == AK_SPISTA_TXHEMP) {
                    pr_debug("TX HEMP...\n");
                    break;
                } else {
                    if(to_write++ < TRANS_TIMEOUT) 
                        udelay(10);
                    else {
                        pr_debug("master transfer timeout...\n");
                        goto SPI_TRANS_FAIL;
                    }
                }
            }           
            iowrite32(*(volatile u32 *)(buff_tx + off_set_write),
                        hw->regs + AK_SPIOUT);
            off_set_write += 4;
            i++;
        }
        //write not finished
        else if (off_set_write < hw->len) {         
            pr_debug("write frac...\n");
            to_write = 0;
            val = 0;
            if (frac_nbr != 0) {
                while(1) {
                    status = ioread32(hw->regs + AK_SPISTA);
                    if ((status & AK_SPISTA_TXHEMP) == AK_SPISTA_TXHEMP)
                        break;
                    if (to_write++ < TRANS_TIMEOUT) 
                        udelay(10);
                    else {
                        printk("SPI master write timeout...\n");                        
                        goto SPI_TRANS_FAIL;
                    }
                }
                    
                for (j=0; j<frac_nbr; j++)
                {
                    pr_debug("[%d]:%x ", off_set_write+j,
                                *(buff_tx+off_set_write+j));
                    val |= (*(buff_tx + off_set_write + j) << (j*8));
                }
                pr_debug("\nval: %x", val);
                pr_debug("\n\n");

                iowrite32(val, hw->regs + AK_SPIOUT); 
                off_set_write += frac_nbr;
            }
        }
        
        //read
        status = ioread32(hw->regs + AK_SPISTA);            
        
        if ((status & AK_SPISTA_TRANSF) == AK_SPISTA_TRANSF) {
            if (status & AK_SPISTA_RXFULL) {
                val = ioread32(hw->regs + AK_SPIIN);
                *(volatile u32 *)(buff_rx + off_set_read) = val;
                off_set_read += 4;

                val = ioread32(hw->regs + AK_SPIIN);
                *(volatile u32 *)(buff_rx + off_set_read) = val;
                off_set_read += 4;

            } else if (status & AK_SPISTA_RXHFULL) {

                val = ioread32(hw->regs + AK_SPIIN);
                *(volatile u32 *)(buff_rx + off_set_read) = val;
                off_set_read += 4;
            }
            if (frac_nbr != 0) {
                pr_debug("read frac...\n");
                val = ioread32(hw->regs + AK_SPIIN);

                for (j=0; j<frac_nbr; j++)
                {
                    *(buff_rx+off_set_read+j) = (val >> j*8) & 0xff;
                    pr_debug("%x ", *(buff_rx+off_set_read+j));
                }
            }
            break;
        } else {
            if ( (status & AK_SPISTA_RXHFULL) == AK_SPISTA_RXHFULL) {
                val = ioread32(hw->regs + AK_SPIIN);
                *(volatile u32 *)(buff_rx + off_set_read) = val;
                pr_debug("rx hfull .. %x\n", val);
                pr_debug("[0]%x [1]%x [2]%x [3]%x\n",
                        buff_rx[0], buff_rx[1], buff_rx[2], buff_rx[3]);
                off_set_read += 4;
            } else {
                if (to_read++ < TRANS_TIMEOUT) 
                    udelay(10);
                else {
                    pr_debug("master read timeout...\n");
                    goto SPI_READ_TIMEOUT;
                }
            }
        }   
    }
    
    return hw->len;
    
SPI_TRANS_FAIL:
SPI_READ_TIMEOUT:
    return off_set_read > off_set_write ? off_set_read: off_set_write;      
    
}
/*end of ak_cpu_duplex*/

#if defined(SPI_CPU_MODE_USE_INTERRUPT)
static int pio_imasks[SPI_DIR_XFER_NUM] = {
    AK_SPIINT_TRANSF | AK_SPIINT_TXHEMP,
    AK_SPIINT_TRANSF | AK_SPIINT_RXHFULL,
    AK_SPIINT_TXHEMP | AK_SPIINT_RXHFULL,
};


/**
*  @brief       spi transfer function by cpu mode.
*  @author      lixinhai
*  @date        2013-03-15
*  @param[in]  *hw :akspi master dev.
*  @param[in]   dir: transfer direction.
*  @return      transfer success result 0, otherwise result a negative value
*/
static int ak_spi_pio_txrx(struct ak_spi *hw, struct spi_transfer *t)
{
    int ret;

    init_completion(&hw->done);

    writel(hw->len, hw->regs + AK_SPICNT);
    enable_imask(hw, pio_imasks[hw->xfer_dir]);

    ret = wait_for_completion_timeout(&hw->done,
            msecs_to_jiffies(SPI_TRANS_TIMEOUT));
    if(ret <= 0) {
        printk("wait for spi transfer interrupt timeout(%s).\n", __func__);
        dbg_dumpregs(hw);
        return -EINVAL;
    }
    
    return hw->count;
}
#else


/**
*  @brief       ak37_spi_writeCPU
*  send message function with CPU mode and polling mode
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   *spi
*  @param[in]   *t
*  @return      int
*/
static int spi_pio_write(struct ak_spi *hw, unsigned char *buf, int count)
{
    u32 status;
    u32 to = 0;

    pr_debug("ak spi write by cpu mode\n");
    if (count > 64*1024)
    {
        printk("too much to be send...\n");
        return -EINVAL;
    }
    //set data count, and the the master will rise clk
    writel(count, hw->regs + AK_SPICNT);
    
    while(hw_remain_datalen(hw) > 0) {
        status = readl(hw->regs + AK_SPISTA);
        if ((status & AK_SPISTA_TXHEMP) == AK_SPISTA_TXHEMP) {
            writel(hw_txdword(hw), hw->regs + AK_SPIOUT);
               
        }else {
            if (to++ > 10 * 1000000) {
                printk("master write data timeout.\n"); 
                return hw->count;
            }
        }
    }
    //wait transfer finish
    while(1) {
        status = readl(hw->regs + AK_SPISTA);       
        if ((status & AK_SPISTA_TRANSF) == AK_SPISTA_TRANSF)
            break;
        
        if (to++ > 10 * 1000000) {
            printk("wait for write data finish timeout..\n");   
            return hw->count;
        }
    }

    
    if (hw_remain_datalen(hw) > 0)
        printk("write wasn't finished.\n");

    return hw->count;
}

/**
*  @brief       spi_pio_read
*  receiving message function with CPU mode and polling mode
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   *spi
*  @param[in]   *t
*  @return      int
*/
static int spi_pio_read(struct ak_spi *hw, unsigned char *buf, int count)
{
    u32 status;
    u32 to=0;
    
    pr_debug("ak spi read by cpu mode\n");
    if (count >= 64*1024)
    {
        printk("too much to be read...\n");
        return -EINVAL;
    }

    //set data count, and the the master will rise clk
    writel(count, hw->regs + AK_SPICNT);
    
    while(1) {
        status = readl(hw->regs + AK_SPISTA);           
        
        if((status & AK_SPISTA_TRANSF) == AK_SPISTA_TRANSF)
        {
            if(status & AK_SPISTA_RXFULL) {
                hw_rxdword(hw, readl(hw->regs + AK_SPIIN));
                hw_rxdword(hw, readl(hw->regs + AK_SPIIN));
            }else if (status & AK_SPISTA_RXHFULL) {
                hw_rxdword(hw, readl(hw->regs + AK_SPIIN));
            } 

            if (hw_remain_datalen(hw) > 0) {
                hw_rxdword(hw, readl(hw->regs + AK_SPIIN));
            }
            break;
        } else {
            if((status & AK_SPISTA_RXHFULL) == AK_SPISTA_RXHFULL) {
                hw_rxdword(hw, readl(hw->regs + AK_SPIIN));
            }
            else {
                if (to++ > 10 * 1000000) {
                    pr_debug("master read timeout.\n");
                    return hw->count;
                }
            }
        }   
    }
    if (hw_remain_datalen(hw) > 0)
        printk("read wasn't finished.\n");

    return hw->count;   
}


/**
*  @brief       spi transfer function by cpu mode.
*  @author      lixinhai
*  @date        2013-03-15
*  @param[in]  *hw :akspi master dev.
*  @param[in]   dir: transfer direction.
*  @return      transfer success result 0, otherwise result a negative value
*/
static int ak_spi_pio_txrx(struct ak_spi *hw, struct spi_transfer *t)
{
    int retlen = -1;
    u32 count = t->len;
            
    hw->tx = (unsigned char*)t->tx_buf;
    hw->rx = t->rx_buf;
    pr_debug("start the spi dma transfer.\n");

    switch(hw->xfer_dir) {
        case SPI_DIR_TX:
        {
            while(count > 0) {
                hw->count = 0;
                hw->len = (count > MAX_XFER_LEN) ? MAX_XFER_LEN : count;
                
                retlen = spi_pio_write(hw, hw->tx, hw->len);
                if(unlikely(retlen < 0)) {
                    printk("spi master transfer data error!\n");
                    goto txrx_ret;
                }
                hw->tx += retlen;
                count -= retlen;

            }
            break;
        }
        case SPI_DIR_RX:
        {
            while(count > 0) {
                hw->count = 0;
                hw->len = (count > MAX_XFER_LEN) ? MAX_XFER_LEN : count;

                retlen = spi_pio_read(hw, hw->rx, hw->len);
                if(unlikely(retlen < 0)) {
                    printk("spi master transfer data error!\n");
                    goto txrx_ret;
                }
                hw->rx += retlen;
                count -= retlen;
            }
            break;
        }
    }
    pr_debug("finish the spi dma transfer.\n");

txrx_ret:
    return (retlen<0) ? retlen : t->len;
}

#endif

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A) 

static void ak_spi_disable_xip_mode(struct ak_spi *hw)
{
    u32 regval;
    u32 try = 0;

    regval = __raw_readl(hw->regs + AK_SPI_WORK_MODE);
    regval |= 1; // disable XIP mode
    __raw_writel(regval, hw->regs + AK_SPI_WORK_MODE);

    while ((__raw_readl(hw->regs + AK_SPI_WORK_MODE) & 1) == 0) {
        try++;
        udelay(1);
        if (try > 3) {
            pr_err("disable XIP mode timeout!\n");
            return;
        }
    }
}

#endif

/**
*  @brief       transfer a message
*  call proper function to complete the transefer
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   *spi
*  @param[in]   *t
*  @return      int
*/
static int ak_spi_txrx(struct spi_device *spi, struct spi_transfer *t)
{
    int ret;
    struct ak_spi *hw = to_hw(spi);

    pr_debug("txrx: tx %p, rx %p, len %d\n",
        t->tx_buf, t->rx_buf, t->len);

    hw->xfer_mode = t->len < 256 ? AKSPI_XFER_MODE_CPU : AKSPI_XFER_MODE_DMA;

    hw->xfer_dir = (t->tx_buf && t->rx_buf) ? SPI_DIR_TXRX : 
                t->tx_buf ? SPI_DIR_TX : SPI_DIR_RX;

    ak_spi_start_txrx(hw, t);
    
    if(hw->xfer_dir == SPI_DIR_TXRX) {      
        ret = ak_cpu_duplex(spi, t);
        return ret;
    }   

    if(ak_spi_use_dma(hw)) {
        ret = ak_spi_dma_txrx(hw, t);
    } else {
        ret = ak_spi_pio_txrx(hw, t);
    }

    ak_spi_stop_txrx(hw, t);

    return ret;
}

/**
*  @brief       ak_spi_irq
*  TODO: used by interrupt mode
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[in]   irq
*  @param[out]  *dev
*  @return      irqreturn_t
*/
static irqreturn_t ak_spi_irq(int irq, void *dev)
{
    struct ak_spi *hw = dev;
    unsigned int status;

    status = readl(hw->regs + AK_SPISTA);

    pr_debug("spi interrupt: status register value is %08x\n", status);

    if(ak_spi_use_dma(hw)) {
        if((status & AK_SPISTA_TRANSF) == AK_SPISTA_TRANSF ) {
            pr_debug("spi transfer data have been finish.\n"); 
            disable_imask(hw, AK_SPIINT_TRANSF);
            complete(&hw->done);
        }
    } else {
        switch(hw->xfer_dir) {  
        case SPI_DIR_RX:
            if(status & (AK_SPISTA_RXHFULL | AK_SPISTA_TRANSF)) {
                pr_debug("spi recv buffer half full or transfer finish.\n");

                if(status & (AK_SPISTA_RXFULL|AK_SPISTA_TRANSF))
                    hw_rxdword(hw, readl(hw->regs + AK_SPIIN));                     
                hw_rxdword(hw, readl(hw->regs + AK_SPIIN));

                if (hw->count >= hw->len) {
                    disable_imask(hw, AK_SPIINT_RXHFULL|AK_SPIINT_TRANSF);
                    complete(&hw->done);
                }               
            }
            break;
        case SPI_DIR_TX:
            if(status & (AK_SPISTA_TXHEMP | AK_SPISTA_TRANSF)) {
                pr_debug("spi send buf half empty or transfer finish.\n");
                
                if (hw->count < hw->len) {
                    if((status & AK_SPISTA_TXEMP) && ((hw->count - hw->len)>4))
                        writel(hw_txdword(hw), hw->regs + AK_SPIOUT);
                    
                    writel(hw_txdword(hw), hw->regs + AK_SPIOUT);
                } else {
                    disable_imask(hw, AK_SPIINT_TXHEMP|AK_SPIINT_TRANSF);
                    complete(&hw->done);
                }
            }
            break;
        case SPI_DIR_TXRX :
        default:
            BUG();
            break;
        }
    }

    return IRQ_HANDLED;
}


/**
*  @brief       ak_spi_initialsetup
*  set up spi master
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[out]  *hw
*  @return      void
*/
static void ak_spi_initialsetup(struct ak_spi *hw)
{
    BUG_ON(hw->master && hw->master->bus_num >= AKSPI_MAX_BUS_NUM);

    /* program defaults into the registers */
    writel(DFT_DIV<<8 | DFT_CON | (1<<1),hw->regs + AK_SPICON);

    writel(0, hw->regs + AK_SPIINT);
}

/* 
* get rid of useless bitbang mode. 
* author: zhangzhipeng
* date:   2018-12-10
*/
static int ak_spi_transfer_one(struct spi_master *master,
                    struct spi_device *spi,
                    struct spi_transfer *transfer)
{   
    struct ak_spi *hw = to_hw(spi);
    struct ak_spi_devstate *cs = spi->controller_state;
    int status = 0;
    int rc;

    if(master->cur_msg){
         /*
         is_dma_mapped值取决于
         spi lcd 的框架kernel\drivers\staging\fbtft\fbtft-io.c
         */
        if(master->cur_msg->is_dma_mapped)
            hw->is_dma_mapped = 1;
        else
            hw->is_dma_mapped = 0;

        dev_dbg(&spi->dev, "new message %p submitted for %s\n",
                    master->cur_msg, dev_name(&spi->dev));
    }

    status = ak_spi_setupxfer(spi, transfer);
    if (status < 0)
        goto out;

    if(transfer->speed_hz < cs->hz){
        rc = clk_set_rate(hw->bus_clk, transfer->speed_hz);
        if (rc) {
            pr_err("%s: clk_set_rate returned %d\n",
            __func__, rc);
        }
    }
    
    
    if (transfer->len)
        status = ak_spi_txrx(spi, transfer);

    if (status == transfer->len)
        status = 0;
    else if (status >= 0)
        status = -EREMOTEIO;

    if(transfer->speed_hz < cs->hz){
        rc = clk_set_rate(hw->bus_clk, cs->hz);
        if (rc) {
            pr_err("%s: clk_set_rate returned %d\n",
            __func__, rc);
        }
    }
out:
    spi_finalize_current_transfer(master);

    return status;
}

/**
*
*@brief: spi_set_chipsel
*@param[in] struct spi_device *spi
*@param[in] bool enable
*@return: void
*
**/
static void ak_spi_set_chipsel(struct spi_device *spi, bool enable)
{
    enable = (!!(spi->mode & SPI_CS_HIGH) == enable);

    ndelay(100);
    ak_spi_chipsel(spi, enable ? CS_ACTIVE :
                CS_INACTIVE);
    ndelay(100);
}

static int spi_proc_show(struct seq_file *m, void *v)
{
    struct ak_spi *hw = m->private;
    struct spi_master *master = hw->master;
    unsigned long clk, clkdiv;
    u32 spicon;

    spicon = ioread32(hw->regs + AK_SPICON);

    if(master->bus_num == 0) {
        clk = clk_get_rate(hw->bus_clk);
    } else {
        clk = clk_get_rate(hw->clk);
        clkdiv = ((spicon>>8) & 0xff);
        clk = clk / (2*(clkdiv+1));
    }

    seq_printf(m, "bus-id: %d\n"
                  "work-mode: host\n"
                  "clockrate: %ldHZ\n",
                  master->bus_num,
                  clk);

    return 0;
}

static int spi_info_open(struct inode *inode, struct file *filp)
{
    return single_open(filp, spi_proc_show, PDE_DATA(inode));
}

static const struct file_operations proc_spi_operations = {
    .open       = spi_info_open,
    .read       = seq_read,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

/**
*  @brief       ak_spi_probe
*  @author      zhou wenyong
*  @date        2011-08-19
*  @param[out]  *pdev
*  @return      int __init
*/
static int  ak_spi_probe(struct platform_device *pdev)
{
    struct ak_spi *hw;
    struct spi_master *master;
    struct resource *res;
    int err = 0;
    struct device_node *np = pdev->dev.of_node;
    char proc_name[8] = {0};
    int ret;

#if (defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK37E) || \
        defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L) || \
        defined(CONFIG_MACH_AK3918AV130)) || defined(CONFIG_MACH_KM01A) \
        && !defined(CONFIG_MACH_AK3918AV100_FPGA)
    unsigned int spi_bus_freq;
    int rc;
#endif
    master = spi_alloc_master(&pdev->dev, sizeof(struct ak_spi));
    if (master == NULL) 
    {
        dev_err(&pdev->dev, "No memory for spi_master\n");
        err = -ENOMEM;
        goto err_nomem;
    }

    hw = spi_master_get_devdata(master);
    memset(hw, 0, sizeof(struct ak_spi));

    hw->master = spi_master_get(master);
    hw->dev = &pdev->dev;
    hw->xfer_mode = AKSPI_XFER_MODE_DMA;

    platform_set_drvdata(pdev, hw);

    init_completion(&hw->done);

    /* setup the master state. */
    /* the spi->mode bits understood by this driver: */
    master->mode_bits = SPI_CPOL | SPI_CPHA | SPI_CS_HIGH | SPI_LSB_FIRST;
    master->setup  = ak_spi_setup;
    master->cleanup = ak_spi_cleanup;
    master->set_cs = ak_spi_set_chipsel;
    master->transfer_one = ak_spi_transfer_one;
    master->dev.of_node = pdev->dev.of_node;
    master->bus_num = of_alias_get_id(master->dev.of_node, "spi");
#if defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK39EV330)
   master->num_chipselect = 1;
#endif
#if defined(CONFIG_MACH_AK37E)
   master->num_chipselect = 2;
#endif
#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)
    if (master->bus_num == AKSPI_BUS_NUM1) {
        master->num_chipselect = 1;
    } else {
        master->num_chipselect = 2;
    }
#endif

    /* find and map our resources */
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
    if (res == NULL) {
        dev_err(&pdev->dev, "Cannot get IORESOURCE_MEM\n");
        err = -ENOENT;
        goto err_no_iores;
    }

    hw->ioarea = request_mem_region(res->start, resource_size(res),
                    pdev->name);

    if (hw->ioarea == NULL) {
        dev_err(&pdev->dev, "Cannot reserve region\n");
        err = -ENXIO;
        goto err_no_iores;
    }

    hw->regs = ioremap(res->start, resource_size(res));
    if (hw->regs == NULL) {
        dev_err(&pdev->dev, "Cannot map IO\n");
        err = -ENXIO;
        goto err_no_iomap;
    }

    hw->irq = platform_get_irq(pdev, 0);
    if (hw->irq < 0) {
        dev_err(&pdev->dev, "No IRQ specified\n");
        err = -ENOENT;
        goto err_no_irq;
    }

    err = request_irq(hw->irq, ak_spi_irq, 0, pdev->name, hw);
    if (err) {
        dev_err(&pdev->dev, "Cannot claim IRQ\n");
        goto err_no_irq;
    }

    hw->rstc = devm_reset_control_get(&pdev->dev, np->name);
    if (IS_ERR(hw->rstc)) {
        dev_err(&pdev->dev, "No reset controller specified\n");
        ret = PTR_ERR(hw->rstc);
        goto err_no_rstc;
    }

    ret = reset_control_assert(hw->rstc);
    if (ret) {
        dev_err(&pdev->dev, "unable to reset_control_assert\n");
        goto err_no_rstc;
    }
    udelay(1);
    ret = reset_control_deassert(hw->rstc);
    if (ret) {
        dev_err(&pdev->dev, "unable to reset_control_deassert\n");
        goto err_no_rstc;
    }

#if (defined(CONFIG_MACH_AK37D) || defined(CONFIG_MACH_AK37E) || \
    defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L) || \
    defined(CONFIG_MACH_AK3918AV130) || defined(CONFIG_MACH_KM01A)) \
    && !defined(CONFIG_MACH_AK3918AV100_FPGA)
    /* spi0_bus CLOCK are generated by core_pll,
    spi1_bus CLOCK are generated by GCLK. */
    if(master->bus_num == 0){
        err = of_property_read_u32(pdev->dev.of_node, "spi-bus-frequency",
                   &spi_bus_freq);
        if (err < 0) {
             dev_warn(&pdev->dev,
                "Could not read spi-bus-frequency property\n");
        }
        
        hw->bus_clk = devm_clk_get(&pdev->dev, "spi0_bus");
        if (IS_ERR(hw->bus_clk)) {
            err = PTR_ERR(hw->bus_clk);
            goto  err_no_clk;
        }

        rc = clk_set_rate(hw->bus_clk, spi_bus_freq);
        if (rc) {
            dev_err(&pdev->dev, "%s: clk_set_rate returned %d\n",
            __func__, rc);
        }

        hw->bus_freq = spi_bus_freq;
        pr_info("SPI%d bus frequency is %lu HZ\n",
                master->bus_num, clk_get_rate(hw->bus_clk));
    } 

#endif
    hw->clk = clk_get(&pdev->dev, NULL);
    pr_info("enter %s..., hw=%p, hw->clk=%p\n", __func__,hw, hw->clk);
    if (IS_ERR(hw->clk)) {
        dev_err(&pdev->dev, "No clock for device\n");
        err = PTR_ERR(hw->clk);
        goto err_no_clk;
    }
    clk_prepare_enable(hw->clk);

#if defined(CONFIG_MACH_AK3918AV100) || defined(CONFIG_MACH_AK3918EV300L) || defined(CONFIG_MACH_AK3918AV130) \
    || defined(CONFIG_MACH_KM01A)
    ak_spi_disable_xip_mode(hw);
#endif

    /*spi high-speed controller cap */
    if (of_property_read_bool(np, "cap-spi-highspeed")){
        hw->cap_highspeed = 1;
    }else{
        hw->cap_highspeed = 0;}

    hw->is_dma_mapped = 0;

    /* spi dma transfer */
    err = dma_set_mask_and_coherent(hw->dev, DMA_BIT_MASK(32));
    if (err) {
        dev_warn(hw->dev, "Unable to set dma mask\n");
        return err;
    }
    hw->txbuffer = dmam_alloc_coherent(hw->dev, SPI_DMA_MAX_LEN,
            &hw->txdma_buffer, GFP_KERNEL);
    if (!hw->txbuffer)
        return -ENOMEM;

    hw->rxbuffer = dmam_alloc_coherent(hw->dev, SPI_DMA_MAX_LEN,
            &hw->rxdma_buffer, GFP_KERNEL);
    if (!hw->rxbuffer)
        return -ENOMEM;

    /* setup */ 
    ak_spi_initialsetup(hw);

    /* register our spi controller */ 
    err = devm_spi_register_master(&pdev->dev, master);
    if (err) {
        dev_err(&pdev->dev, "Failed to register SPI master\n");
        goto err_register;
    }

    pr_info("akspi master SPI%d initialize success, use for %s mode.\n", 
        master->bus_num, ak_spi_use_dma(hw)?"DMA":"PIO");

    /* create proc node to output state in runtime */
    sprintf(proc_name,"spi%d", master->bus_num);
    hw->spi_proc = proc_create_data(proc_name, S_IRUGO,
                NULL, &proc_spi_operations, hw);
    
    return 0;

 err_register:
    clk_disable(hw->clk);
    clk_put(hw->clk);

 err_no_rstc:
 err_no_clk:
    free_irq(hw->irq, hw);

 err_no_irq:
    iounmap(hw->regs);

 err_no_iomap:
    release_resource(hw->ioarea);

 err_no_iores:
    spi_master_put(hw->master);

 err_nomem:
    return err;
}


static int __exit ak_spi_remove(struct platform_device *dev)
{
    struct ak_spi *hw = platform_get_drvdata(dev);

    platform_set_drvdata(dev, NULL);

    spi_unregister_master(hw->master);

    clk_disable(hw->clk);
    clk_put(hw->clk);

    free_irq(hw->irq, hw);
    iounmap(hw->regs);

    release_resource(hw->ioarea);

    spi_master_put(hw->master);
    return 0;
}

#ifdef CONFIG_PM
static int ak_spi_suspend(struct device *dev)
{
    struct ak_spi *hw = platform_get_drvdata(to_platform_device(dev));
    pr_info("enter %s...,\n", __func__);
    if (hw->clk)
        clk_disable_unprepare(hw->clk);
    pinctrl_pm_select_sleep_state(dev);
    return 0;
}

static int ak_spi_resume(struct device *dev)
{
    struct ak_spi *hw = platform_get_drvdata(to_platform_device(dev));
    pr_info("enter %s...\n", __func__);
    pinctrl_pm_select_default_state(dev);
    if (hw->clk)
        clk_prepare_enable(hw->clk);
    ak_spi_initialsetup(hw);
    return 0;
}

static struct dev_pm_ops ak_spi_pmops = {
    SET_SYSTEM_SLEEP_PM_OPS(ak_spi_suspend,ak_spi_resume)
};

#define AK_SPI_PMOPS &ak_spi_pmops
#else
#define AK_SPI_PMOPS NULL
#endif /* CONFIG_PM */

static const struct of_device_id ak_spi_match[] = {
    { .compatible = "anyka,ak37d-spi0", },
    { .compatible = "anyka,ak37d-spi1", },
    { .compatible = "anyka,ak39ev330-spi0", },
    { .compatible = "anyka,ak39ev330-spi1", },
    { .compatible = "anyka,ak37e-spi0", },
    { .compatible = "anyka,ak37e-spi1", },
    { .compatible = "anyka,ak37e-spi2", },
    { .compatible = "anyka,ak3918av100-spi0", },
    { .compatible = "anyka,ak3918av100-spi1", },
    { .compatible = "anyka,ak3918av100-spi2", },
    { .compatible = "anyka,km01a-spi1", },
    { .compatible = "anyka,km01a-spi2", },
    { .compatible = "anyka,ak3918av130-spi1", },
    { .compatible = "anyka,ak3918av130-spi2", },
    { .compatible = "anyka,ak3918ev300l-spi0", },
    { .compatible = "anyka,ak3918ev300l-spi1", },
    { .compatible = "anyka,ak3918ev300l-spi2", },
    {}
};
MODULE_DEVICE_TABLE(of, ak_spi_match);

MODULE_ALIAS("platform:ak-spi");
static struct platform_driver ak_spi_driver = {
    .probe          = ak_spi_probe,
    .remove     = __exit_p(ak_spi_remove),
    .driver     = {
        .name   = "ak-spi",
        .of_match_table = of_match_ptr(ak_spi_match),
        .owner  = THIS_MODULE,
        .pm = AK_SPI_PMOPS,
    },
};

static int __init ak_spi_init(void)
{
        return platform_driver_register(&ak_spi_driver);
}

static void __exit ak_spi_exit(void)
{
        platform_driver_unregister(&ak_spi_driver);
}

module_init(ak_spi_init);
module_exit(ak_spi_exit);
MODULE_DESCRIPTION("Anyka On-Chip SPI controller driver");
MODULE_AUTHOR("Anyka Microelectronic");
MODULE_VERSION("2.0.00");
MODULE_LICENSE("GPL");
