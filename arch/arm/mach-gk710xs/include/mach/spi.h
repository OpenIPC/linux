/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/mach/spi.h
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

#ifndef __MACH_GK_SPI_H
#define __MACH_GK_SPI_H

#include <mach/hardware.h>
#include CONFIG_GK_CHIP_INCLUDE_FILE

/****************************************************/
/* Capabilities based on chip revision              */
/****************************************************/
#define SPI_MAX_SLAVE_ID                        7

#define SPI_SUPPORT_TISSP_NSM                   1

#define SPI_INSTANCES                           2
#define SPI_AHB_INSTANCES                       0
#define SPI_SUPPORT_TSSI_MODE                   1


#define SPI_EN2_EN3_ENABLED_BY_HOST_ENA_REG     0

#define SPI_EN2_ENABLED_BY_GPIO2_AFSEL_REG      0

#define SPI_EN4_7_ENABLED_BY_GPIO1_AFSEL_REG    1

#define SPI_SLAVE_INSTANCES                     0

#define SPI_SUPPORT_MASTER_CHANGE_ENA_POLARITY  0
#define SPI_SUPPORT_MASTER_DELAY_START_TIME     0
#define SPI_SUPPORT_NSM_SHAKE_START_BIT_CHSANGE 0

#define SPI_EN2_EN3_ENABLED_BY_GPIO2_AFSEL_REG  0

/* SPI_FIFO_SIZE */
#define SPI_DATA_FIFO_SIZE_16                   0x10
#define SPI_DATA_FIFO_SIZE_32                   0x20
#define SPI_DATA_FIFO_SIZE_64                   0x40
#define SPI_DATA_FIFO_SIZE_128                  0x80

/****************************************************/
/* Controller registers definitions                 */
/****************************************************/
#if SSI_HAL_MODE
#define SPI_CTRLR0_OFFSET                       0x0c
#define SPI_CTRLR1_OFFSET                       0x10
#define SPI_SSIENR_OFFSET                       0x08
#define SPI_MWCR_OFFSET                         0x14
#define SPI_SER_OFFSET                          0x60
#define SPI_BAUDR_OFFSET                        0x18
#define SPI_TXFTLR_OFFSET                       0x3c
#define SPI_RXFTLR_OFFSET                       0x48
#define SPI_TXFLR_OFFSET                        0x40
#define SPI_RXFLR_OFFSET                        0x4c
#define SPI_SR_OFFSET                           0x04
#define SPI_IMR_OFFSET                          0x1c
#define SPI_ISR_OFFSET                          0x30
#define SPI_RISR_OFFSET                         0x2c
#define SPI_TXOICR_OFFSET                       0x44
#define SPI_RXOICR_OFFSET                       0x50
#define SPI_RXUICR_OFFSET                       0x54
#define SPI_MSTICR_OFFSET                       0x24
#define SPI_ICR_OFFSET                          0x20
#if (SPI_AHB_INSTANCES >= 1)
#define SPI_DMAC_OFFSET                         0x28
#endif
#define SPI_IDR_OFFSET                          0x00
#define SPI_VERSION_ID_OFFSET                   0x5c
#define SPI_DR_OFFSET                           0x38

#if (SPI_SUPPORT_MASTER_CHANGE_ENA_POLARITY == 1)
#define SPI_SSIENPOLR_OFFSET                    0x260
#endif
#if (SPI_SUPPORT_MASTER_DELAY_START_TIME == 1)
#define SPI_SCLK_OUT_DLY_OFFSET                 0x264
#endif
#if (SPI_SUPPORT_NSM_SHAKE_START_BIT_CHSANGE == 1)
#define SPI_START_BIT_OFFSET                    0x268
#endif

#define TSSI_CTRL_OFFSET                        0x04
#define TSSI_SSR_OFFSET                         0x00
#define TSSI_INDEX_OFFSET                       0x0c
#define TSSI_DATA_OFFSET                        0x08
#define TSSI_POLARITY_INVERT                    0x10

#else
#define SPI_CTRLR0_OFFSET                       0x00
#define SPI_CTRLR1_OFFSET                       0x04
#define SPI_SSIENR_OFFSET                       0x08
#define SPI_MWCR_OFFSET                         0x0C
#define SPI_SER_OFFSET                          0x10
#define SPI_BAUDR_OFFSET                        0x14
#define SPI_TXFTLR_OFFSET                       0x18
#define SPI_RXFTLR_OFFSET                       0x1C
#define SPI_TXFLR_OFFSET                        0x20
#define SPI_RXFLR_OFFSET                        0x24
#define SPI_SR_OFFSET                           0x28
#define SPI_IMR_OFFSET                          0x2C
#define SPI_ISR_OFFSET                          0x30
#define SPI_RISR_OFFSET                         0x34
#define SPI_TXOICR_OFFSET                       0x38
#define SPI_RXOICR_OFFSET                       0x3C
#define SPI_RXUICR_OFFSET                       0x40
#define SPI_MSTICR_OFFSET                       0x44
#define SPI_ICR_OFFSET                          0x48
#if (SPI_AHB_INSTANCES >= 1)
#define SPI_DMAC_OFFSET                         0x4C
#endif
#define SPI_IDR_OFFSET                          0x58
#define SPI_VERSION_ID_OFFSET                   0x5C
#define SPI_DR_OFFSET                           0x60

#if (SPI_SUPPORT_MASTER_CHANGE_ENA_POLARITY == 1)
#define SPI_SSIENPOLR_OFFSET                    0x260
#endif
#if (SPI_SUPPORT_MASTER_DELAY_START_TIME == 1)
#define SPI_SCLK_OUT_DLY_OFFSET                 0x264
#endif
#if (SPI_SUPPORT_NSM_SHAKE_START_BIT_CHSANGE == 1)
#define SPI_START_BIT_OFFSET                    0x268
#endif

#define TSSI_CTRL_OFFSET                        0x00
#define TSSI_SSR_OFFSET                         0x04
#define TSSI_INDEX_OFFSET                       0x08
#define TSSI_DATA_OFFSET                        0x0C
#define TSSI_POLARITY_INVERT                    0x10
#endif

#define SPI_REG(x)                  (GK_VA_SSI1 + (x))
#define SPI2_REG(x)                 (GK_VA_SSI2 + (x))
#define TSSI_REG(x)                 (GK_VA_TSSI + (x))

#define SPI_CTRLR0_REG              SPI_REG(SPI_CTRLR0_OFFSET)
#define SPI_CTRLR1_REG              SPI_REG(SPI_CTRLR1_OFFSET)
#define SPI_SSIENR_REG              SPI_REG(SPI_SSIENR_OFFSET)
#define SPI_MWCR_REG                SPI_REG(SPI_MWCR_OFFSET)
#define SPI_SER_REG                 SPI_REG(SPI_SER_OFFSET)
#define SPI_BAUDR_REG               SPI_REG(SPI_BAUDR_OFFSET)
#define SPI_TXFTLR_REG              SPI_REG(SPI_TXFTLR_OFFSET)
#define SPI_RXFTLR_REG              SPI_REG(SPI_RXFTLR_OFFSET)
#define SPI_TXFLR_REG               SPI_REG(SPI_TXFLR_OFFSET)
#define SPI_RXFLR_REG               SPI_REG(SPI_RXFLR_OFFSET)
#define SPI_SR_REG                  SPI_REG(SPI_SR_OFFSET)
#define SPI_IMR_REG                 SPI_REG(SPI_IMR_OFFSET)
#define SPI_ISR_REG                 SPI_REG(SPI_ISR_OFFSET)
#define SPI_RISR_REG                SPI_REG(SPI_RISR_OFFSET)
#define SPI_TXOICR_REG              SPI_REG(SPI_TXOICR_OFFSET)
#define SPI_RXOICR_REG              SPI_REG(SPI_RXOICR_OFFSET)
#define SPI_RXUICR_REG              SPI_REG(SPI_RXUICR_OFFSET)
#define SPI_MSTICR_REG              SPI_REG(SPI_MSTICR_OFFSET)
#define SPI_ICR_REG                 SPI_REG(SPI_ICR_OFFSET)
#define SPI_IDR_REG                 SPI_REG(SPI_IDR_OFFSET)
#define SPI_VERSION_ID_REG          SPI_REG(SPI_VERSION_ID_OFFSET)
#define SPI_DR_REG                  SPI_REG(SPI_DR_OFFSET)

#if (SPI_INSTANCES >= 2)
#define SPI2_CTRLR0_REG             SPI2_REG(SPI_CTRLR0_OFFSET)
#define SPI2_CTRLR1_REG             SPI2_REG(SPI_CTRLR1_OFFSET)
#define SPI2_SSIENR_REG             SPI2_REG(SPI_SSIENR_OFFSET)
#define SPI2_MWCR_REG               SPI2_REG(SPI_MWCR_OFFSET)
#define SPI2_SER_REG                SPI2_REG(SPI_SER_OFFSET)
#define SPI2_BAUDR_REG              SPI2_REG(SPI_BAUDR_OFFSET)
#define SPI2_TXFTLR_REG             SPI2_REG(SPI_TXFTLR_OFFSET)
#define SPI2_RXFTLR_REG             SPI2_REG(SPI_RXFTLR_OFFSET)
#define SPI2_TXFLR_REG              SPI2_REG(SPI_TXFLR_OFFSET)
#define SPI2_RXFLR_REG              SPI2_REG(SPI_RXFLR_OFFSET)
#define SPI2_SR_REG                 SPI2_REG(SPI_SR_OFFSET)
#define SPI2_IMR_REG                SPI2_REG(SPI_IMR_OFFSET)
#define SPI2_ISR_REG                SPI2_REG(SPI_ISR_OFFSET)
#define SPI2_RISR_REG               SPI2_REG(SPI_RISR_OFFSET)
#define SPI2_TXOICR_REG             SPI2_REG(SPI_TXOICR_OFFSET)
#define SPI2_RXOICR_REG             SPI2_REG(SPI_RXOICR_OFFSET)
#define SPI2_RXUICR_REG             SPI2_REG(SPI_RXUICR_OFFSET)
#define SPI2_MSTICR_REG             SPI2_REG(SPI_MSTICR_OFFSET)
#define SPI2_ICR_REG                SPI2_REG(SPI_ICR_OFFSET)
#define SPI2_IDR_REG                SPI2_REG(SPI_IDR_OFFSET)
#define SPI2_VERSION_ID_REG         SPI2_REG(SPI_VERSION_ID_OFFSET)
#define SPI2_DR_REG                 SPI2_REG(SPI_DR_OFFSET)
#endif

#define TSSI_CTRL_REG               TSSI_REG(TSSI_CTRL_OFFSET)
#define TSSI_SSR_REG                TSSI_REG(TSSI_SSR_OFFSET)
#define TSSI_INDEX_REG              TSSI_REG(TSSI_INDEX_OFFSET)
#define TSSI_DATA_REG               TSSI_REG(TSSI_DATA_OFFSET)
#define TSSI_POLARITY_INVERT_REG    TSSI_REG(TSSI_POLARITY_INVERT)


#define SPI_MASTER_INSTANCES        (SPI_INSTANCES + SPI_AHB_INSTANCES)

/* ==========================================================================*/
/* SPI rw mode */
#define SPI_WRITE_READ      0
#define SPI_WRITE_ONLY      1
#define SPI_READ_ONLY       2

/* Tx FIFO empty interrupt mask */
#define SPI_TXEIS_MASK      0x00000001
#define SPI_TXOIS_MASK      0x00000002

/* SPI Parameters */
#define SPI_DUMMY_DATA      0xffff
#define MAX_QUERY_TIMES     10

/* Default SPI settings */
#define SPI_MODE            SPI_MODE_0
#define SPI_SCPOL           0
#define SPI_SCPH            0
#define SPI_FRF             0
#define SPI_CFS             0x0
#define SPI_DFS             0xf
#define SPI_BAUD_RATE       200000

/* ==========================================================================*/
#ifndef __ASSEMBLER__

struct gk_spi_hw_info {
    int bus_id;
    int cs_id;
};
typedef struct gk_spi_hw_info gk_spi_hw_t;

struct gk_spi_cfg_info {
    u8  spi_mode;
    u8  cfs_dfs;
    u8  cs_change;
    u32 baud_rate;
};
typedef struct gk_spi_cfg_info gk_spi_cfg_t;

typedef struct {
    u8  bus_id;
    u8  cs_id;
    u8  *buffer;
    u16 n_size;
} gk_spi_write_t;

typedef struct {
    u8  bus_id;
    u8  cs_id;
    u8  *buffer;
    u16 n_size;
} gk_spi_read_t;

typedef struct {
    u8  bus_id;
    u8  cs_id;
    u8  *w_buffer;
    u8  *r_buffer;
    u16 w_size;
    u16 r_size;
} gk_spi_write_then_read_t;

typedef struct {
    u8  bus_id;
    u8  cs_id;
    u8  *w_buffer;
    u8  *r_buffer;
    u16 n_size;
} gk_spi_write_and_read_t;

struct gk_spi_cs_config {
    u8                  bus_id;
    u8                  cs_id;
    u8                  cs_num;
    int                 *cs_pins;
};

struct gk_spi_platform_info {
    int                 support_dma;
    int                 fifo_entries;
    int                 cs_num;
    int                 *cs_pins;
    void                (*cs_activate)  (struct gk_spi_cs_config *);
    void                (*cs_deactivate)(struct gk_spi_cs_config *);
    void                (*rct_set_ssi_pll)(void);
    u32                 (*get_ssi_freq_hz)(void);
};
#define GK_SPI_PARAM_CALL(id, arg, perm) \
    module_param_cb(spi##id##_cs0, &param_ops_int, &(arg[0]), perm); \
    module_param_cb(spi##id##_cs1, &param_ops_int, &(arg[1]), perm); \
    module_param_cb(spi##id##_cs2, &param_ops_int, &(arg[2]), perm); \
    module_param_cb(spi##id##_cs3, &param_ops_int, &(arg[3]), perm); \
    module_param_cb(spi##id##_cs4, &param_ops_int, &(arg[4]), perm); \
    module_param_cb(spi##id##_cs5, &param_ops_int, &(arg[5]), perm); \
    module_param_cb(spi##id##_cs6, &param_ops_int, &(arg[6]), perm); \
    module_param_cb(spi##id##_cs7, &param_ops_int, &(arg[7]), perm)

/* ==========================================================================*/
extern struct platform_device           gk_spi0;
extern struct platform_device           gk_spi1;
extern struct platform_device           gk_spi2;
extern struct platform_device           gk_spi3;
extern struct platform_device           gk_spi4;
extern struct platform_device           gk_spi_slave;

/* ==========================================================================*/
extern int gk_spi_write(gk_spi_cfg_t *spi_cfg,
    gk_spi_write_t *spi_write);
extern int gk_spi_read(gk_spi_cfg_t *spi_cfg,
    gk_spi_read_t *spi_read);
extern int gk_spi_write_then_read(gk_spi_cfg_t *spi_cfg,
    gk_spi_write_then_read_t *spi_write_then_read);
extern int gk_spi_write_and_read(gk_spi_cfg_t *spi_cfg,
    gk_spi_write_and_read_t *spi_write_and_read);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

