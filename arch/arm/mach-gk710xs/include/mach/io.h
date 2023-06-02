/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/io.h
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

#ifndef __MACH_IO_H
#define __MACH_IO_H

#include <asm/io.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE

#ifndef __ASSEMBLER__

#include <linux/spinlock.h>

extern spinlock_t gk_reg_lock;
extern unsigned long gk_reg_flags;
extern u32 gk_reglock_count;

#define gk_hal_readb(a)             g_hw->hw_readb((u32)(a))
#define gk_hal_readw(a)             g_hw->hw_readw((u32)(a))
#define gk_hal_writeb(a,v)          g_hw->hw_writeb((v),(u32)(a))
#define gk_hal_writew(a,v)          g_hw->hw_writew((v),(u32)(a))
#define gk_hal_readl(a)             g_hw->hw_readl((u32)(a))
#define gk_hal_writel(a,v)          g_hw->hw_writel((v),(u32)(a))
#if FLASH_HAL_MODE
#define gk_flash_read()         g_hw->flash_read()
#define gk_flash_write(v)       g_hw->flash_write((v))
#else
#define gk_flash_read()         (*((volatile unsigned int *)(REG_SFLASH_DATA)))
#define gk_flash_write(v)       (*((volatile unsigned int *)(REG_SFLASH_DATA)) = (v));
#endif

#if 0
#define GK_REG_LOCK()        spin_lock_irqsave(&gk_reg_lock, gk_reg_flags)
#define GK_REG_UNLOCK()        spin_unlock_irqrestore(&gk_reg_lock, gk_reg_flags)
#define GK_INC_REGLOCK_COUNT()    gk_reglock_count++
#else
#define GK_REG_LOCK()
#define GK_REG_UNLOCK()
#define GK_INC_REGLOCK_COUNT()
#endif

#define HAL_IO(mod) \
unsigned char gk_##mod##_readb(unsigned int addr) \
{\
	return g_hw->hw_readb((u32)(addr));\
}\
EXPORT_SYMBOL(gk_##mod##_readb);\
unsigned short gk_##mod##_readw(unsigned int addr) \
{\
	return  g_hw->hw_readw((u32)(addr));\
}\
EXPORT_SYMBOL(gk_##mod##_readw);\
unsigned int gk_##mod##_readl(unsigned int addr) \
{\
	return g_hw->hw_readl((u32)(addr)); \
}\
EXPORT_SYMBOL(gk_##mod##_readl);\
void gk_##mod##_writeb(unsigned int addr,unsigned char val) \
{\
	g_hw->hw_writeb((val),(u32)(addr)); \
}\
EXPORT_SYMBOL(gk_##mod##_writeb); \
void gk_##mod##_writew(unsigned int addr,unsigned short val) \
{\
	g_hw->hw_writew((val),(u32)(addr)); \
}\
EXPORT_SYMBOL(gk_##mod##_writew); \
void gk_##mod##_writel(unsigned int addr,unsigned int val) \
{\
	g_hw->hw_writel((val),(u32)(addr));\
}\
EXPORT_SYMBOL(gk_##mod##_writel);
#define DIR_IO(mod) \
unsigned char gk_##mod##_readb(unsigned int addr) \
{\
	return *((volatile unsigned char*)(addr));\
}\
EXPORT_SYMBOL(gk_##mod##_readb);\
unsigned short gk_##mod##_readw(unsigned int addr)\
{\
	return *((volatile unsigned short*)(addr));\
}\
EXPORT_SYMBOL(gk_##mod##_readw);\
unsigned int gk_##mod##_readl(unsigned int addr)\
{\
	return *((volatile unsigned int*)(addr));\
}\
EXPORT_SYMBOL(gk_##mod##_readl);\
void gk_##mod##_writeb(unsigned int addr,unsigned char val)\
{\
	*((volatile unsigned char*)(addr))= val;\
}\
EXPORT_SYMBOL(gk_##mod##_writeb);\
void gk_##mod##_writew(unsigned int addr,unsigned short val)\
{\
	*((volatile unsigned short*)(addr))= val;\
}\
EXPORT_SYMBOL(gk_##mod##_writew);\
void gk_##mod##_writel(unsigned int addr,unsigned int val)\
{\
	*((volatile unsigned int*)(addr))= val;\
}\
EXPORT_SYMBOL(gk_##mod##_writel);


#define IO_DEC(mod) \
extern unsigned char gk_##mod##_readb(unsigned int addr); \
extern unsigned short gk_##mod##_readw(unsigned int addr); \
extern unsigned int gk_##mod##_readl(unsigned int addr); \
extern void gk_##mod##_writeb(unsigned int addr,unsigned char val); \
extern void gk_##mod##_writew(unsigned int addr,unsigned short val); \
extern void gk_##mod##_writel(unsigned int addr,unsigned int val);

IO_DEC(hw)
IO_DEC(adc)
IO_DEC(aud)
IO_DEC(cry)
IO_DEC(ddr)
IO_DEC(dma)
IO_DEC(dsp)
IO_DEC(eth)
IO_DEC(greg)
IO_DEC(gpio)
IO_DEC(i2c)
IO_DEC(i2s)
IO_DEC(ir)
IO_DEC(irq)
IO_DEC(mcu)
IO_DEC(pmu)
IO_DEC(rct)
IO_DEC(sd)
IO_DEC(sf)
IO_DEC(ssi)
IO_DEC(timer)
IO_DEC(uart)
IO_DEC(usb)
IO_DEC(vout)
IO_DEC(wdt)
IO_DEC(pwm)
IO_DEC(efuse)

#define GK_HAL_OS_LOCK()    \
    do {    \
        GK_REG_LOCK();    \
        GK_INC_REGLOCK_COUNT();    \
    } while(0)


#define GK_HAL_OS_UNLOCK()    \
    do {    \
        GK_REG_UNLOCK();    \
    } while(0)

//--------------------------------------
static inline u8 __gk_readb(const volatile void __iomem *address)
{
    u8                 tmpval;

    GK_REG_LOCK();
    GK_INC_REGLOCK_COUNT();
    tmpval = gk_hw_readb((u32)address);
    GK_REG_UNLOCK();

    return tmpval;
}

static inline void __gk_writeb(const volatile void __iomem *address, u8 value)
{
    GK_REG_LOCK();
    GK_INC_REGLOCK_COUNT();
    gk_hw_writeb((u32)address,value);
    GK_REG_UNLOCK();
}

static inline u16 __gk_readw(const volatile void __iomem *address)
{
    u16                    tmpval;

    GK_REG_LOCK();
    GK_INC_REGLOCK_COUNT();
    tmpval = gk_hw_readw((u32)address);
    GK_REG_UNLOCK();

    return tmpval;
}

static inline void __gk_writew(const volatile void __iomem *address, u16 value)
{
    GK_REG_LOCK();
    GK_INC_REGLOCK_COUNT();
    gk_hw_writew((u32)address,value);
    GK_REG_UNLOCK();
}

static inline u32 __gk_readl(const volatile void __iomem *address)
{
    u32                    tmpval;

    GK_REG_LOCK();
    GK_INC_REGLOCK_COUNT();
    tmpval = gk_hw_readl((u32)address);
    GK_REG_UNLOCK();

    return tmpval;
}

static inline void __gk_writel(const volatile void __iomem *address, u32 value)
{
    GK_REG_LOCK();
    GK_INC_REGLOCK_COUNT();
    gk_hw_writel((u32)address,value);
    GK_REG_UNLOCK();
}

#define gk_adc_setbitsb(v, mask)    gk_adc_writeb((v),(gk_adc_readb((v)) | (mask)))
#define gk_adc_setbitsw(v, mask)    gk_adc_writew((v),(gk_adc_readw((v)) | (mask)))
#define gk_adc_setbitsl(v, mask)    gk_adc_writel((v),(gk_adc_readl((v)) | (mask)))
#define gk_adc_clrbitsb(v, mask)    gk_adc_writeb((v),(gk_adc_readb((v)) & ~(mask)))
#define gk_adc_clrbitsw(v, mask)    gk_adc_writew((v),(gk_adc_readw((v)) & ~(mask)))
#define gk_adc_clrbitsl(v, mask)    gk_adc_writel((v),(gk_adc_readl((v)) & ~(mask)))
#define gk_adc_tstbitsb(v, mask)    (gk_adc_readb((v)) & (mask))
#define gk_adc_tstbitsw(v, mask)    (gk_adc_readw((v)) & (mask))
#define gk_adc_tstbitsl(v, mask)    (gk_adc_readl((v)) & (mask))

#define gk_aud_setbitsb(v, mask)    gk_aud_writeb((v),(gk_aud_readb((v)) | (mask)))
#define gk_aud_setbitsw(v, mask)    gk_aud_writew((v),(gk_aud_readw((v)) | (mask)))
#define gk_aud_setbitsl(v, mask)    gk_aud_writel((v),(gk_aud_readl((v)) | (mask)))
#define gk_aud_clrbitsb(v, mask)    gk_aud_writeb((v),(gk_aud_readb((v)) & ~(mask)))
#define gk_aud_clrbitsw(v, mask)    gk_aud_writew((v),(gk_aud_readw((v)) & ~(mask)))
#define gk_aud_clrbitsl(v, mask)    gk_aud_writel((v),(gk_aud_readl((v)) & ~(mask)))
#define gk_aud_tstbitsb(v, mask)    (gk_aud_readb((v)) & (mask))
#define gk_aud_tstbitsw(v, mask)    (gk_aud_readw((v)) & (mask))
#define gk_aud_tstbitsl(v, mask)    (gk_aud_readl((v)) & (mask))

#define gk_cry_setbitsb(v, mask)    gk_cry_writeb((v),(gk_cry_readb((v)) | (mask)))
#define gk_cry_setbitsw(v, mask)    gk_cry_writew((v),(gk_cry_readw((v)) | (mask)))
#define gk_cry_setbitsl(v, mask)    gk_cry_writel((v),(gk_cry_readl((v)) | (mask)))
#define gk_cry_clrbitsb(v, mask)    gk_cry_writeb((v),(gk_cry_readb((v)) & ~(mask)))
#define gk_cry_clrbitsw(v, mask)    gk_cry_writew((v),(gk_cry_readw((v)) & ~(mask)))
#define gk_cry_clrbitsl(v, mask)    gk_cry_writel((v),(gk_cry_readl((v)) & ~(mask)))
#define gk_cry_tstbitsb(v, mask)    (gk_cry_readb((v)) & (mask))
#define gk_cry_tstbitsw(v, mask)    (gk_cry_readw((v)) & (mask))
#define gk_cry_tstbitsl(v, mask)    (gk_cry_readl((v)) & (mask))

#define gk_ddr_setbitsb(v, mask)    gk_ddr_writeb((v),(gk_ddr_readb((v)) | (mask)))
#define gk_ddr_setbitsw(v, mask)    gk_ddr_writew((v),(gk_ddr_readw((v)) | (mask)))
#define gk_ddr_setbitsl(v, mask)    gk_ddr_writel((v),(gk_ddr_readl((v)) | (mask)))
#define gk_ddr_clrbitsb(v, mask)    gk_ddr_writeb((v),(gk_ddr_readb((v)) & ~(mask)))
#define gk_ddr_clrbitsw(v, mask)    gk_ddr_writew((v),(gk_ddr_readw((v)) & ~(mask)))
#define gk_ddr_clrbitsl(v, mask)    gk_ddr_writel((v),(gk_ddr_readl((v)) & ~(mask)))
#define gk_ddr_tstbitsb(v, mask)    (gk_ddr_readb((v)) & (mask))
#define gk_ddr_tstbitsw(v, mask)    (gk_ddr_readw((v)) & (mask))
#define gk_ddr_tstbitsl(v, mask)    (gk_ddr_readl((v)) & (mask))

#define gk_dma_setbitsb(v, mask)    gk_dma_writeb((v),(gk_dma_readb((v)) | (mask)))
#define gk_dma_setbitsw(v, mask)    gk_dma_writew((v),(gk_dma_readw((v)) | (mask)))
#define gk_dma_setbitsl(v, mask)    gk_dma_writel((v),(gk_dma_readl((v)) | (mask)))
#define gk_dma_clrbitsb(v, mask)    gk_dma_writeb((v),(gk_dma_readb((v)) & ~(mask)))
#define gk_dma_clrbitsw(v, mask)    gk_dma_writew((v),(gk_dma_readw((v)) & ~(mask)))
#define gk_dma_clrbitsl(v, mask)    gk_dma_writel((v),(gk_dma_readl((v)) & ~(mask)))
#define gk_dma_tstbitsb(v, mask)    (gk_dma_readb((v)) & (mask))
#define gk_dma_tstbitsw(v, mask)    (gk_dma_readw((v)) & (mask))
#define gk_dma_tstbitsl(v, mask)    (gk_dma_readl((v)) & (mask))

#define gk_dsp_setbitsb(v, mask)    gk_dsp_writeb((v),(gk_dsp_readb((v)) | (mask)))
#define gk_dsp_setbitsw(v, mask)    gk_dsp_writew((v),(gk_dsp_readw((v)) | (mask)))
#define gk_dsp_setbitsl(v, mask)    gk_dsp_writel((v),(gk_dsp_readl((v)) | (mask)))
#define gk_dsp_clrbitsb(v, mask)    gk_dsp_writeb((v),(gk_dsp_readb((v)) & ~(mask)))
#define gk_dsp_clrbitsw(v, mask)    gk_dsp_writew((v),(gk_dsp_readw((v)) & ~(mask)))
#define gk_dsp_clrbitsl(v, mask)    gk_dsp_writel((v),(gk_dsp_readl((v)) & ~(mask)))
#define gk_dsp_tstbitsb(v, mask)    (gk_dsp_readb((v)) & (mask))
#define gk_dsp_tstbitsw(v, mask)    (gk_dsp_readw((v)) & (mask))
#define gk_dsp_tstbitsl(v, mask)    (gk_dsp_readl((v)) & (mask))

#define gk_eth_setbitsb(v, mask)    gk_eth_writeb((v),(gk_eth_readb((v)) | (mask)))
#define gk_eth_setbitsw(v, mask)    gk_eth_writew((v),(gk_eth_readw((v)) | (mask)))
#define gk_eth_setbitsl(v, mask)    gk_eth_writel((v),(gk_eth_readl((v)) | (mask)))
#define gk_eth_clrbitsb(v, mask)    gk_eth_writeb((v),(gk_eth_readb((v)) & ~(mask)))
#define gk_eth_clrbitsw(v, mask)    gk_eth_writew((v),(gk_eth_readw((v)) & ~(mask)))
#define gk_eth_clrbitsl(v, mask)    gk_eth_writel((v),(gk_eth_readl((v)) & ~(mask)))
#define gk_eth_tstbitsb(v, mask)    (gk_eth_readb((v)) & (mask))
#define gk_eth_tstbitsw(v, mask)    (gk_eth_readw((v)) & (mask))
#define gk_eth_tstbitsl(v, mask)    (gk_eth_readl((v)) & (mask))

#define gk_greg_setbitsb(v, mask)    gk_greg_writeb((v),(gk_greg_readb((v)) | (mask)))
#define gk_greg_setbitsw(v, mask)    gk_greg_writew((v),(gk_greg_readw((v)) | (mask)))
#define gk_greg_setbitsl(v, mask)    gk_greg_writel((v),(gk_greg_readl((v)) | (mask)))
#define gk_greg_clrbitsb(v, mask)    gk_greg_writeb((v),(gk_greg_readb((v)) & ~(mask)))
#define gk_greg_clrbitsw(v, mask)    gk_greg_writew((v),(gk_greg_readw((v)) & ~(mask)))
#define gk_greg_clrbitsl(v, mask)    gk_greg_writel((v),(gk_greg_readl((v)) & ~(mask)))
#define gk_greg_tstbitsb(v, mask)    (gk_greg_readb((v)) & (mask))
#define gk_greg_tstbitsw(v, mask)    (gk_greg_readw((v)) & (mask))
#define gk_greg_tstbitsl(v, mask)    (gk_greg_readl((v)) & (mask))

#define gk_gpio_setbitsb(v, mask)    gk_gpio_writeb((v),(gk_gpio_readb((v)) | (mask)))
#define gk_gpio_setbitsw(v, mask)    gk_gpio_writew((v),(gk_gpio_readw((v)) | (mask)))
#define gk_gpio_setbitsl(v, mask)    gk_gpio_writel((v),(gk_gpio_readl((v)) | (mask)))
#define gk_gpio_clrbitsb(v, mask)    gk_gpio_writeb((v),(gk_gpio_readb((v)) & ~(mask)))
#define gk_gpio_clrbitsw(v, mask)    gk_gpio_writew((v),(gk_gpio_readw((v)) & ~(mask)))
#define gk_gpio_clrbitsl(v, mask)    gk_gpio_writel((v),(gk_gpio_readl((v)) & ~(mask)))
#define gk_gpio_tstbitsb(v, mask)    (gk_gpio_readb((v)) & (mask))
#define gk_gpio_tstbitsw(v, mask)    (gk_gpio_readw((v)) & (mask))
#define gk_gpio_tstbitsl(v, mask)    (gk_gpio_readl((v)) & (mask))

#define gk_i2c_setbitsb(v, mask)    gk_i2c_writeb((v),(gk_i2c_readb((v)) | (mask)))
#define gk_i2c_setbitsw(v, mask)    gk_i2c_writew((v),(gk_i2c_readw((v)) | (mask)))
#define gk_i2c_setbitsl(v, mask)    gk_i2c_writel((v),(gk_i2c_readl((v)) | (mask)))
#define gk_i2c_clrbitsb(v, mask)    gk_i2c_writeb((v),(gk_i2c_readb((v)) & ~(mask)))
#define gk_i2c_clrbitsw(v, mask)    gk_i2c_writew((v),(gk_i2c_readw((v)) & ~(mask)))
#define gk_i2c_clrbitsl(v, mask)    gk_i2c_writel((v),(gk_i2c_readl((v)) & ~(mask)))
#define gk_i2c_tstbitsb(v, mask)    (gk_i2c_readb((v)) & (mask))
#define gk_i2c_tstbitsw(v, mask)    (gk_i2c_readw((v)) & (mask))
#define gk_i2c_tstbitsl(v, mask)    (gk_i2c_readl((v)) & (mask))

#define gk_i2s_setbitsb(v, mask)    gk_i2s_writeb((v),(gk_i2s_readb((v)) | (mask)))
#define gk_i2s_setbitsw(v, mask)    gk_i2s_writew((v),(gk_i2s_readw((v)) | (mask)))
#define gk_i2s_setbitsl(v, mask)    gk_i2s_writel((v),(gk_i2s_readl((v)) | (mask)))
#define gk_i2s_clrbitsb(v, mask)    gk_i2s_writeb((v),(gk_i2s_readb((v)) & ~(mask)))
#define gk_i2s_clrbitsw(v, mask)    gk_i2s_writew((v),(gk_i2s_readw((v)) & ~(mask)))
#define gk_i2s_clrbitsl(v, mask)    gk_i2s_writel((v),(gk_i2s_readl((v)) & ~(mask)))
#define gk_i2s_tstbitsb(v, mask)    (gk_i2s_readb((v)) & (mask))
#define gk_i2s_tstbitsw(v, mask)    (gk_i2s_readw((v)) & (mask))
#define gk_i2s_tstbitsl(v, mask)    (gk_i2s_readl((v)) & (mask))


#define gk_ir_setbitsb(v, mask)    gk_ir_writeb((v),(gk_ir_readb((v)) | (mask)))
#define gk_ir_setbitsw(v, mask)    gk_ir_writew((v),(gk_ir_readw((v)) | (mask)))
#define gk_ir_setbitsl(v, mask)    gk_ir_writel((v),(gk_ir_readl((v)) | (mask)))
#define gk_ir_clrbitsb(v, mask)    gk_ir_writeb((v),(gk_ir_readb((v)) & ~(mask)))
#define gk_ir_clrbitsw(v, mask)    gk_ir_writew((v),(gk_ir_readw((v)) & ~(mask)))
#define gk_ir_clrbitsl(v, mask)    gk_ir_writel((v),(gk_ir_readl((v)) & ~(mask)))
#define gk_ir_tstbitsb(v, mask)    (gk_ir_readb((v)) & (mask))
#define gk_ir_tstbitsw(v, mask)    (gk_ir_readw((v)) & (mask))
#define gk_ir_tstbitsl(v, mask)    (gk_ir_readl((v)) & (mask))

#define gk_irq_setbitsb(v, mask)    gk_irq_writeb((v),(gk_irq_readb((v)) | (mask)))
#define gk_irq_setbitsw(v, mask)    gk_irq_writew((v),(gk_irq_readw((v)) | (mask)))
#define gk_irq_setbitsl(v, mask)    gk_irq_writel((v),(gk_irq_readl((v)) | (mask)))
#define gk_irq_clrbitsb(v, mask)    gk_irq_writeb((v),(gk_irq_readb((v)) & ~(mask)))
#define gk_irq_clrbitsw(v, mask)    gk_irq_writew((v),(gk_irq_readw((v)) & ~(mask)))
#define gk_irq_clrbitsl(v, mask)    gk_irq_writel((v),(gk_irq_readl((v)) & ~(mask)))
#define gk_irq_tstbitsb(v, mask)    (gk_irq_readb((v)) & (mask))
#define gk_irq_tstbitsw(v, mask)    (gk_irq_readw((v)) & (mask))
#define gk_irq_tstbitsl(v, mask)    (gk_irq_readl((v)) & (mask))

#define gk_mcu_setbitsb(v, mask)    gk_mcu_writeb((v),(gk_mcu_readb((v)) | (mask)))
#define gk_mcu_setbitsw(v, mask)    gk_mcu_writew((v),(gk_mcu_readw((v)) | (mask)))
#define gk_mcu_setbitsl(v, mask)    gk_mcu_writel((v),(gk_mcu_readl((v)) | (mask)))
#define gk_mcu_clrbitsb(v, mask)    gk_mcu_writeb((v),(gk_mcu_readb((v)) & ~(mask)))
#define gk_mcu_clrbitsw(v, mask)    gk_mcu_writew((v),(gk_mcu_readw((v)) & ~(mask)))
#define gk_mcu_clrbitsl(v, mask)    gk_mcu_writel((v),(gk_mcu_readl((v)) & ~(mask)))
#define gk_mcu_tstbitsb(v, mask)    (gk_mcu_readb((v)) & (mask))
#define gk_mcu_tstbitsw(v, mask)    (gk_mcu_readw((v)) & (mask))
#define gk_mcu_tstbitsl(v, mask)    (gk_mcu_readl((v)) & (mask))

#define gk_pwm_setbitsb(v, mask)    gk_pwm_writeb((v),(gk_pwm_readb((v)) | (mask)))
#define gk_pwm_setbitsw(v, mask)    gk_pwm_writew((v),(gk_pwm_readw((v)) | (mask)))
#define gk_pwm_setbitsl(v, mask)    gk_pwm_writel((v),(gk_pwm_readl((v)) | (mask)))
#define gk_pwm_clrbitsb(v, mask)    gk_pwm_writeb((v),(gk_pwm_readb((v)) & ~(mask)))
#define gk_pwm_clrbitsw(v, mask)    gk_pwm_writew((v),(gk_pwm_readw((v)) & ~(mask)))
#define gk_pwm_clrbitsl(v, mask)    gk_pwm_writel((v),(gk_pwm_readl((v)) & ~(mask)))
#define gk_pwm_tstbitsb(v, mask)    (gk_pwm_readb((v)) & (mask))
#define gk_pwm_tstbitsw(v, mask)    (gk_pwm_readw((v)) & (mask))
#define gk_pwm_tstbitsl(v, mask)    (gk_pwm_readl((v)) & (mask))

#define gk_rct_setbitsb(v, mask)    gk_rct_writeb((v),(gk_rct_readb((v)) | (mask)))
#define gk_rct_setbitsw(v, mask)    gk_rct_writew((v),(gk_rct_readw((v)) | (mask)))
#define gk_rct_setbitsl(v, mask)    gk_rct_writel((v),(gk_rct_readl((v)) | (mask)))
#define gk_rct_clrbitsb(v, mask)    gk_rct_writeb((v),(gk_rct_readb((v)) & ~(mask)))
#define gk_rct_clrbitsw(v, mask)    gk_rct_writew((v),(gk_rct_readw((v)) & ~(mask)))
#define gk_rct_clrbitsl(v, mask)    gk_rct_writel((v),(gk_rct_readl((v)) & ~(mask)))
#define gk_rct_tstbitsb(v, mask)    (gk_rct_readb((v)) & (mask))
#define gk_rct_tstbitsw(v, mask)    (gk_rct_readw((v)) & (mask))
#define gk_rct_tstbitsl(v, mask)    (gk_rct_readl((v)) & (mask))

#define gk_sd_setbitsb(v, mask)    gk_sd_writeb((v),(gk_sd_readb((v)) | (mask)))
#define gk_sd_setbitsw(v, mask)    gk_sd_writew((v),(gk_sd_readw((v)) | (mask)))
#define gk_sd_setbitsl(v, mask)    gk_sd_writel((v),(gk_sd_readl((v)) | (mask)))
#define gk_sd_clrbitsb(v, mask)    gk_sd_writeb((v),(gk_sd_readb((v)) & ~(mask)))
#define gk_sd_clrbitsw(v, mask)    gk_sd_writew((v),(gk_sd_readw((v)) & ~(mask)))
#define gk_sd_clrbitsl(v, mask)    gk_sd_writel((v),(gk_sd_readl((v)) & ~(mask)))
#define gk_sd_tstbitsb(v, mask)    (gk_sd_readb((v)) & (mask))
#define gk_sd_tstbitsw(v, mask)    (gk_sd_readw((v)) & (mask))
#define gk_sd_tstbitsl(v, mask)    (gk_sd_readl((v)) & (mask))

#define gk_sf_setbitsb(v, mask)    gk_sf_writeb((v),(gk_sf_readb((v)) | (mask)))
#define gk_sf_setbitsw(v, mask)    gk_sf_writew((v),(gk_sf_readw((v)) | (mask)))
#define gk_sf_setbitsl(v, mask)    gk_sf_writel((v),(gk_sf_readl((v)) | (mask)))
#define gk_sf_clrbitsb(v, mask)    gk_sf_writeb((v),(gk_sf_readb((v)) & ~(mask)))
#define gk_sf_clrbitsw(v, mask)    gk_sf_writew((v),(gk_sf_readw((v)) & ~(mask)))
#define gk_sf_clrbitsl(v, mask)    gk_sf_writel((v),(gk_sf_readl((v)) & ~(mask)))
#define gk_sf_tstbitsb(v, mask)    (gk_sf_readb((v)) & (mask))
#define gk_sf_tstbitsw(v, mask)    (gk_sf_readw((v)) & (mask))
#define gk_sf_tstbitsl(v, mask)    (gk_sf_readl((v)) & (mask))

#define gk_ssi_setbitsb(v, mask)    gk_ssi_writeb((v),(gk_ssi_readb((v)) | (mask)))
#define gk_ssi_setbitsw(v, mask)    gk_ssi_writew((v),(gk_ssi_readw((v)) | (mask)))
#define gk_ssi_setbitsl(v, mask)    gk_ssi_writel((v),(gk_ssi_readl((v)) | (mask)))
#define gk_ssi_clrbitsb(v, mask)    gk_ssi_writeb((v),(gk_ssi_readb((v)) & ~(mask)))
#define gk_ssi_clrbitsw(v, mask)    gk_ssi_writew((v),(gk_ssi_readw((v)) & ~(mask)))
#define gk_ssi_clrbitsl(v, mask)    gk_ssi_writel((v),(gk_ssi_readl((v)) & ~(mask)))
#define gk_ssi_tstbitsb(v, mask)    (gk_ssi_readb((v)) & (mask))
#define gk_ssi_tstbitsw(v, mask)    (gk_ssi_readw((v)) & (mask))
#define gk_ssi_tstbitsl(v, mask)    (gk_ssi_readl((v)) & (mask))

#define gk_timer_setbitsb(v, mask)    gk_timer_writeb((v),(gk_timer_readb((v)) | (mask)))
#define gk_timer_setbitsw(v, mask)    gk_timer_writew((v),(gk_timer_readw((v)) | (mask)))
#define gk_timer_setbitsl(v, mask)    gk_timer_writel((v),(gk_timer_readl((v)) | (mask)))
#define gk_timer_clrbitsb(v, mask)    gk_timer_writeb((v),(gk_timer_readb((v)) & ~(mask)))
#define gk_timer_clrbitsw(v, mask)    gk_timer_writew((v),(gk_timer_readw((v)) & ~(mask)))
#define gk_timer_clrbitsl(v, mask)    gk_timer_writel((v),(gk_timer_readl((v)) & ~(mask)))
#define gk_timer_tstbitsb(v, mask)    (gk_timer_readb((v)) & (mask))
#define gk_timer_tstbitsw(v, mask)    (gk_timer_readw((v)) & (mask))
#define gk_timer_tstbitsl(v, mask)    (gk_timer_readl((v)) & (mask))

#define gk_uart_setbitsb(v, mask)    gk_uart_writeb((v),(gk_uart_readb((v)) | (mask)))
#define gk_uart_setbitsw(v, mask)    gk_uart_writew((v),(gk_uart_readw((v)) | (mask)))
#define gk_uart_setbitsl(v, mask)    gk_uart_writel((v),(gk_uart_readl((v)) | (mask)))
#define gk_uart_clrbitsb(v, mask)    gk_uart_writeb((v),(gk_uart_readb((v)) & ~(mask)))
#define gk_uart_clrbitsw(v, mask)    gk_uart_writew((v),(gk_uart_readw((v)) & ~(mask)))
#define gk_uart_clrbitsl(v, mask)    gk_uart_writel((v),(gk_uart_readl((v)) & ~(mask)))
#define gk_uart_tstbitsb(v, mask)    (gk_uart_readb((v)) & (mask))
#define gk_uart_tstbitsw(v, mask)    (gk_uart_readw((v)) & (mask))
#define gk_uart_tstbitsl(v, mask)    (gk_uart_readl((v)) & (mask))

#define gk_usb_setbitsb(v, mask)    gk_usb_writeb((v),(gk_usb_readb((v)) | (mask)))
#define gk_usb_setbitsw(v, mask)    gk_usb_writew((v),(gk_usb_readw((v)) | (mask)))
#define gk_usb_setbitsl(v, mask)    gk_usb_writel((v),(gk_usb_readl((v)) | (mask)))
#define gk_usb_clrbitsb(v, mask)    gk_usb_writeb((v),(gk_usb_readb((v)) & ~(mask)))
#define gk_usb_clrbitsw(v, mask)    gk_usb_writew((v),(gk_usb_readw((v)) & ~(mask)))
#define gk_usb_clrbitsl(v, mask)    gk_usb_writel((v),(gk_usb_readl((v)) & ~(mask)))
#define gk_usb_tstbitsb(v, mask)    (gk_usb_readb((v)) & (mask))
#define gk_usb_tstbitsw(v, mask)    (gk_usb_readw((v)) & (mask))
#define gk_usb_tstbitsl(v, mask)    (gk_usb_readl((v)) & (mask))

#define gk_vout_setbitsb(v, mask)    gk_vout_writeb((v),(gk_vout_readb((v)) | (mask)))
#define gk_vout_setbitsw(v, mask)    gk_vout_writew((v),(gk_vout_readw((v)) | (mask)))
#define gk_vout_setbitsl(v, mask)    gk_vout_writel((v),(gk_vout_readl((v)) | (mask)))
#define gk_vout_clrbitsb(v, mask)    gk_vout_writeb((v),(gk_vout_readb((v)) & ~(mask)))
#define gk_vout_clrbitsw(v, mask)    gk_vout_writew((v),(gk_vout_readw((v)) & ~(mask)))
#define gk_vout_clrbitsl(v, mask)    gk_vout_writel((v),(gk_vout_readl((v)) & ~(mask)))
#define gk_vout_tstbitsb(v, mask)    (gk_vout_readb((v)) & (mask))
#define gk_vout_tstbitsw(v, mask)    (gk_vout_readw((v)) & (mask))
#define gk_vout_tstbitsl(v, mask)    (gk_vout_readl((v)) & (mask))

#define gk_wdt_setbitsb(v, mask)    gk_wdt_writeb((v),(gk_wdt_readb((v)) | (mask)))
#define gk_wdt_setbitsw(v, mask)    gk_wdt_writew((v),(gk_wdt_readw((v)) | (mask)))
#define gk_wdt_setbitsl(v, mask)    gk_wdt_writel((v),(gk_wdt_readl((v)) | (mask)))
#define gk_wdt_clrbitsb(v, mask)    gk_wdt_writeb((v),(gk_wdt_readb((v)) & ~(mask)))
#define gk_wdt_clrbitsw(v, mask)    gk_wdt_writew((v),(gk_wdt_readw((v)) & ~(mask)))
#define gk_wdt_clrbitsl(v, mask)    gk_wdt_writel((v),(gk_wdt_readl((v)) & ~(mask)))
#define gk_wdt_tstbitsb(v, mask)    (gk_wdt_readb((v)) & (mask))
#define gk_wdt_tstbitsw(v, mask)    (gk_wdt_readw((v)) & (mask))
#define gk_wdt_tstbitsl(v, mask)    (gk_wdt_readl((v)) & (mask))


#define gk_musb_readb(a, o)      gk_usb_readb((a+o))
#define gk_musb_readw(a, o)      gk_usb_readw((a+o))
#define gk_musb_readl(a, o)      gk_usb_readl((a+o))
#define gk_musb_writeb(a, o, v)  gk_usb_writeb((a+o),(v))
#define gk_musb_writew(a, o, v)  gk_usb_writew((a+o),(v))
#define gk_musb_writel(a, o, v)  gk_usb_writel((a+o),(v))

#define gk_spi_readb(a)      gk_ssi_readb((a))
#define gk_spi_readw(a)      gk_ssi_readw((a))
#define gk_spi_readl(a)      gk_ssi_readl((a))
#define gk_spi_writeb(a, v)  gk_ssi_writeb((a),(v))
#define gk_spi_writew(a, v)  gk_ssi_writew((a),(v))
#define gk_spi_writel(a, v)  gk_ssi_writel((a),(v))




#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif /* __MACH_IO_H */
