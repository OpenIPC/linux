/*!
*****************************************************************************
** \file        arch/arm/mach-gk/include/mach/gk.h
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

#ifndef __MACH_GK710X_H
#define __MACH_GK710X_H


#include <asm/mach/time.h>
#include <linux/init.h>

#define CHIP_ID(x)                  ((x / 1000))
#define CHIP_MAJOR(x)               ((x / 100) % 10)
#define CHIP_MINOR(x)               ((x / 10) % 10)

/* ==========================================================================*/
#define GK_DEBUG_NULL               (0)
#define GK_DEBUG_MEDIA              (1 << 0)
#define GK_DEBUG_VI                 (1 << 1)
#define GK_DEBUG_VO                 (1 << 2)
#define GK_DEBUG_AAA                (1 << 3)
#define GK_DEBUG_DSP                (1 << 4)

/* definition SOC types */
#define CONFIG_SOC_GK7101           0
#define CONFIG_SOC_GK7102           1

#define CONFIG_U2K_PHY_TYPE         0xC0000000
#define CONFIG_U2K_ARM_FREQ         0xC0000004
#define CONFIG_U2K_HAL_ADDR         0xC0000008

#define CONFIG_U2K_SOC_ADDR         0xC0000010        //0:, 1:7102
#define CONFIG_U2K_MEM_ADDR         0xC0000014
#define CONFIG_U2K_BSB_ADDR         0xC0000018
#define CONFIG_U2K_DSP_ADDR         0xC000001C

#define CONFIG_U2K_ENABLE           0xC0000020
#define CONFIG_U2K_USR_ADDR         0xC0000024   //for user's cmem module

#define CONFIG_U2K_TOTAL_MEM        0xC0000028

/* ==========================================================================*/
#ifndef __ASSEMBLER__

/* ==========================================================================*/
extern u32                gk_debug_level;
extern u32                gk_debug_info;

#define HW_HAL_MODE         1
#define USB_DMA_ADD_MODE    0   // 1: need update hal to 20151223
#define SPI_API_MODE        1   // 1: need update hal to 20160324
#define ADC_HAL_MODE        1
#define AUD_HAL_MODE        1
#define CRY_HAL_MODE        1
#define DDR_HAL_MODE        1
#define DSP_HAL_MODE        1
#define ETH_HAL_MODE        1
#define GREG_HAL_MODE       1
#define GPIO_HAL_MODE       1
#define I2C_HAL_MODE        1
#define I2S_HAL_MODE        1
#define IR_HAL_MODE         1
#define IRQ_HAL_MODE        1
#define MCU_HAL_MODE        1
#define PWM_HAL_MODE        1
#define RCT_HAL_MODE        1
#define SD_HAL_MODE         1
#define SF_HAL_MODE         1
#define SSI_HAL_MODE        1
#define TIMER_HAL_MODE      1
#define UART_HAL_MODE       1
#define USB_HAL_MODE        1
#define VOUT_HAL_MODE       1
#define WDT_HAL_MODE        1
#define EFUSE_HAL_MODE      0
struct hw_ops
{
    int (*get_version)(void);
    unsigned int (*reserved)(unsigned int );

    unsigned char (*hw_readb)(unsigned int );
    unsigned short (*hw_readw)(unsigned int );
    unsigned int (*hw_readl)(unsigned int );

    void (*hw_writeb)(unsigned char , unsigned int );
    void (*hw_writew)(unsigned short , unsigned int );
    void (*hw_writel)(unsigned int , unsigned int );

    unsigned int (*flash_read)(void);
    void (*flash_write)(unsigned int);

    unsigned char (*usb_readb)(unsigned int ptr, unsigned int offset);
    unsigned short (*usb_readw)(unsigned int ptr, unsigned int offset);
    unsigned int (*usb_readl)(unsigned int ptr, unsigned int offset);
    void (*usb_writeb)(unsigned int ptr, unsigned int offset, unsigned char value);
    void (*usb_writew)(unsigned int ptr, unsigned int offset, unsigned short value);
    void (*usb_writel)(unsigned int ptr, unsigned int offset, unsigned int value);

    unsigned int (*dma_readl)(unsigned int ptr);
    void (*dma_writel)(unsigned int ptr, unsigned int value);

#if SPI_API_MODE
    unsigned char (*spi_readb)(unsigned int ptr);
    unsigned short (*spi_readw)(unsigned int ptr);
    unsigned int (*spi_readl)(unsigned int ptr);
    void (*spi_writeb)(unsigned int ptr, unsigned char value);
    void (*spi_writew)(unsigned int ptr, unsigned short value);
    void (*spi_writel)(unsigned int ptr, unsigned int value);
#endif
};

extern struct hw_ops *g_hw;

extern unsigned long gk_debug_lookup_name(const char *name);
void sensor_init(u8 active_level);
void sensor_power(u8 power);
u32 get_audio_clk_freq(void);


#endif /* __ASSEMBLER__ */

extern u32 get_ppm_phys(void);
extern u32 get_ppm_virt(void);
extern u32 get_ppm_size(void);

extern u32 get_bsbmem_phys(void);
extern u32 get_bsbmem_virt(void);
extern u32 get_bsbmem_size(void);

extern u32 get_dspmem_phys(void);
extern u32 get_dspmem_virt(void);
extern u32 get_dspmem_size(void);

extern u32 gk_phys_to_virt(u32 paddr);
extern u32 gk_virt_to_phys(u32 vaddr);

extern u32 get_hal1_virt(void);
extern u32 get_hal2_virt(void);

extern u32 get_ahb_phys(void);
extern u32 get_ahb_virt(void);
extern u32 get_ahb_size(void);

extern u32 get_apb_phys(void);
extern u32 get_apb_virt(void);
extern u32 get_apb_size(void);

extern u32 get_osd_phys(void);
extern u32 get_osd_virt(void);
extern u32 get_osd_size(void);

extern u32 get_pri_phys(void);
extern u32 get_pri_virt(void);
extern u32 get_pri_size(void);

extern u32 get_qpm_phys(void);
extern u32 get_qpm_virt(void);
extern u32 get_qpm_size(void);

extern u32 get_bsbmem_virt(void);
extern u32 get_bsbmem_size(void);

extern u32 get_usrmem_phys(void);
extern u32 get_usrmem_virt(void);
extern u32 get_usrmem_size(void);

extern void get_stepping_info(int *chip, int *major, int *minor);
extern int gk_register_event_notifier(void *nb);

extern u32 gk_boot_splash_logo;

extern u64 gk_dmamask;

extern int gk_create_proc_dir(void);
extern struct proc_dir_entry *get_gk_proc_dir(void);

extern void get_stepping_info(int *chip, int *major, int *minor);

extern void gk_restart(char mode, const char *cmd);
extern void gk_power_off(void);

extern void gk_map_io(void) __init;

extern void gk_load_51mcu_code(u32 code);

#endif  /* __MACH_GK_H */

