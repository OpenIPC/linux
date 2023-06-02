#ifndef __PINMUX_DRV_H__
#define __PINMUX_DRV_H__

#include "top_reg.h"
#include <top.h>
#include <kwrap/nvt_type.h>
#include <kwrap/flag.h>
#include <io_address.h>
#include <gpio.h>

typedef uint32_t u32;


/*
    BOOT_CARD_ENUM
*/
typedef enum {
	BOOT_CARD_SDIO,                     //< Boot card from SDIO
	BOOT_CARD_SDIO2,                    //< Boot card from SDIO2

	ENUM_DUMMY4WORD(BOOT_CARD_ENUM)
} BOOT_CARD_ENUM;

/*
	EJTAG_ENUM
*/
typedef enum
{
	EJTAG_GPIO,                         //< Disable EJTAG (ICE interface)
	EJTAG_EN,                           //< Enable EJTAG

	ENUM_DUMMY4WORD(EJTAG_ENUM)
} EJTAG_ENUM;

/*
	MPLL_CLK_ENUM
*/
typedef enum
{
	MPLL_CLK_NORMAL,                    //< PLL2~13 from internal MPLL
	MPLL_CLK_BYPASS,                    //< Bypass MPLL (From external pad)

	ENUM_DUMMY4WORD(MPLL_CLK_ENUM)
} MPLL_CLK_ENUM;

/*
	NAND_ACT_ENUM
*/
typedef enum
{
	NAND_ACT_DIS,                       //< De-activate NAND ACT
	NAND_ACT_EN,                        //< Activate NAND ACT

	ENUM_DUMMY4WORD(NAND_ACT_ENUM)
} NAND_ACT_ENUM;

/*
    SPI_ACT_ENUM
*/
typedef enum {
	SPI_ACT_DIS,                        //< De-activate SPI ACT
	SPI_ACT_EN,                         //< Activate SPI ACT

	ENUM_DUMMY4WORD(SPI_ACT_ENUM)
} SPI_ACT_ENUM;


/*
	SDIO2_ACT_ENUM
*/
typedef enum
{
	SDIO2_ACT_DIS,                      //< De-activate SDIO2 ACT
	SDIO2_ACT_EN,                       //< Activate SDIO2 ACT

	ENUM_DUMMY4WORD(SDIO2_ACT_ENUM)
} SDIO2_ACT_ENUM;

/*
    SPI_EXIST_ENUM
*/
typedef enum {
	SPI_EXIST_DIS,                      //< De-activate SPI EXIST
	SPI_EXIST_EN,                       //< Activate SPI EXIST

	ENUM_DUMMY4WORD(SPI_EXIST_ENUM)
} SPI_EXIST_ENUM;

/*
	SDIO_EXIST_ENUM
*/
typedef enum
{
	SDIO_EXIST_DIS,                     //< De-activate SDIO EXIST
	SDIO_EXIST_EN,                      //< Activate SDIO EXIST

	ENUM_DUMMY4WORD(SDIO_EXIST_ENUM)
} SDIO_EXIST_ENUM;

/*
	SDIO2_EXIST_ENUM
*/
typedef enum
{
	SDIO2_EXIST_DIS,                    //< De-activate SDIO2 EXIST
	SDIO2_EXIST_EN,                     //< Activate SDIO2 EXIST
	SDIO2_EXIST2_EN,                   //< Activate SDIO2_2 EXIST
	SDIO2_EXIST3_EN,                   //< Activate SDIO2_3 EXIST

	ENUM_DUMMY4WORD(SDIO2_EXIST_ENUM)
} SDIO2_EXIST_ENUM;

/*
	SDIO3_ENABLE_ENUM
*/
typedef enum {
	SDIO3_DIS,                         //< SDIO3 Disable
	SDIO3_EN,                          //< SDIO3 Enable

	ENUM_DUMMY4WORD(SDIO3_ENABLE_ENUM)
} SDIO3_ENABLE_ENUM;

/*
	EXTROM_EXIST_ENUM
*/
typedef enum
{
	EXTROM_EXIST_DIS,                   //< De-activate EXTROM ACT
	EXTROM_EXIST_EN,                    //< Activate EXTROM ACT

	ENUM_DUMMY4WORD(EXTROM_EXIST_ENUM)
} EXTROM_EXIST_ENUM;

/*
	SDIO2_MUX_ENUM
*/
typedef enum {
	SDIO2_MUX_1ST,                      //< 1st pinmux pad
	SDIO2_MUX_2ND,                      //< 2nd pinmux pad

	ENUM_DUMMY4WORD(SDIO2_MUX_ENUM)
} SDIO2_MUX_ENUM;


/*
	SPI_MUX_ENUM
*/
typedef enum
{
	SPI_MUX_1ST,                        //< 1st pinmux pad
	SPI_MUX_2ND,                        //< 2nd pinmux pad

	ENUM_DUMMY4WORD(SPI_MUX_ENUM)
} SPI_MUX_ENUM;

/*
	ROM_SRC_ENUM
*/
typedef enum
{
	ROM_SRC_INTERNAL,                   //< Internal ROM
	ROM_SRC_EXTERNAL,                   //< External ROM

	ENUM_DUMMY4WORD(ROM_SRC_ENUM)
} ROM_SRC_ENUM;

/*
	SPI_BUS_WIDTH_ENUM
*/
typedef enum {
	SPI_BUS_WIDTH_2BITS,                //< SPI is 2 bits
	SPI_BUS_WIDTH_4BITS,                //< SPI is 4 bits

	ENUM_DUMMY4WORD(SPI_BUS_WIDTH_ENUM)
} SPI_BUS_WIDTH_ENUM;

/*
	SDIO2_BUS_WIDTH_ENUM
*/
typedef enum
{
	SDIO2_BUS_WIDTH_4BITS,              //< SDIO2 is 4 bits
	SDIO2_BUS_WIDTH_8BITS,              //< SDIO2 is 8 bits (no use, Backward compatible)

	ENUM_DUMMY4WORD(SDIO2_BUS_WIDTH_ENUM)
} SDIO2_BUS_WIDTH_ENUM;

/*
	SDIO3_BUS_WIDTH_ENUM
*/
typedef enum
{
	SDIO3_BUS_WIDTH_4BITS,              //< SDIO3 is 4 bits
	SDIO3_BUS_WIDTH_8BITS,              //< SDIO3 is 8 bits

	ENUM_DUMMY4WORD(SDIO3_BUS_WIDTH_ENUM)
} SDIO3_BUS_WIDTH_ENUM;


/*
	LCDTYPE_ENUM
*/
typedef enum
{
	LCDTYPE_ENUM_GPIO,                  //< GPIO
	LCDTYPE_ENUM_CCIR656,               //< CCIR656
	LCDTYPE_ENUM_CCIR601,               //< CCIR601
	LCDTYPE_ENUM_PARALLEL_LCD,          //< Parallel LCD (RGB565)
	LCDTYPE_ENUM_SERIALRGB_8BITS,       //< Serial RGB 8 bits
	LCDTYPE_ENUM_SERIALRGB_6BITS,       //< Serial RGB 6 bits
	LCDTYPE_ENUM_SERIALYCBCR_8BITS,     //< Serial YCbCr 8 bits
	LCDTYPE_ENUM_RGB_16BITS,            //< RGB 16 bits
	LCDTYPE_ENUM_PARARGB666_LCD,        //< Parallel LCD (RGB666)
	LCDTYPE_ENUM_MIPI_DSI,              //< MIPI-DSI
	LCDTYPE_ENUM_PARARGB888_LCD,       //< Parallel LCD (RGB888) Only support @ 528
	ENUM_DUMMY4WORD(LCDTYPE_ENUM)
} LCDTYPE_ENUM;

/*
	PLCD_DATA_WIDTH_ENUM
*/
typedef enum
{
	PLCD_DATA_WIDTH_RGB888,             //< RGB888
	PLCD_DATA_WIDTH_RGB666,             //< RGB666
	PLCD_DATA_WIDTH_RGB565,             //< RGB565

	ENUM_DUMMY4WORD(PLCD_DATA_WIDTH_ENUM)
} PLCD_DATA_WIDTH_ENUM;

/*
	CCIR_DATA_WIDTH_ENUM
*/
typedef enum
{
	CCIR_DATA_WIDTH_8BITS,              //< CCIR 8 bits
	CCIR_DATA_WIDTH_16BITS,             //< CCIR 16 bits

	ENUM_DUMMY4WORD(CCIR_DATA_WIDTH_ENUM)
} CCIR_DATA_WIDTH_ENUM;

/*
	MEMIF_TYPE_ENUM
*/
typedef enum
{
	MEMIF_TYPE_GPIO,                    //< GPIO
	MEMIF_TYPE_SERIAL,                  //< Select serial MI
	MEMIF_TYPE_PARALLEL,                //< Select parallel MI

	ENUM_DUMMY4WORD(MEMIF_TYPE_ENUM)
} MEMIF_TYPE_ENUM;

/*
	PMEM_SEL_ENUM
*/
typedef enum
{
	PMEM_SEL_ENUM_1ST_PINMUX,           //< Primary Parallel MI pinmux (up to 18 bits)
	PMEM_SEL_ENUM_2ND_PINMUX,           //< Secondary Parallel MI pinmux (up to 9 bits)

	ENUM_DUMMY4WORD(PMEM_SEL_ENUM)
} PMEM_SEL_ENUM;

/*
	PMEMIF_DATA_WIDTH_ENUM
*/
typedef enum
{
	PMEMIF_DATA_WIDTH_8BITS,            //< Parallel MI bus width is 8 bits
	PMEMIF_DATA_WIDTH_9BITS,            //< Parallel MI bus width is 9 bits
	PMEMIF_DATA_WIDTH_16BITS,           //< Parallel MI bus width is 16 bits
	PMEMIF_DATA_WIDTH_18BITS,           //< Parallel MI bus width is 18 bits

	ENUM_DUMMY4WORD(PMEMIF_DATA_WIDTH_ENUM)
} PMEMIF_DATA_WIDTH_ENUM;

/*
	SMEMIF_DATA_WIDTH_ENUM
*/
typedef enum
{
	SMEMIF_DATA_WIDTH_1BITS,            //< Serial MI bus width is 1 bit
	SMEMIF_DATA_WIDTH_2BITS,            //< Serial MI bus width is 2 bits

	ENUM_DUMMY4WORD(SMEMIF_DATA_WIDTH_ENUM)
} SMEMIF_DATA_WIDTH_ENUM;

/*
	PLCD_DE_ENUM
*/
typedef enum
{
	PLCD_DE_ENUM_GPIO,                  //< GPIO
	PLCD_DE_ENUM_DE,                    //< DE

	ENUM_DUMMY4WORD(PLCD_DE_ENUM)
} PLCD_DE_ENUM;

/*
	CCIR_HVLD_VVLD_ENUM
*/
typedef enum
{
	CCIR_HVLD_VVLD_GPIO,                //< GPIO
	CCIR_HVLD_VVLD_EN,                  //< HVLD/VVLD enable

	ENUM_DUMMY4WORD(CCIR_HVLD_VVLD_ENUM)
} CCIR_HVLD_VVLD_ENUM;

/*
	CCIR_FIELD_ENUM
*/
typedef enum
{
	CCIR_FIELD_GPIO,                    //< GPIO
	CCIR_FIELD_EN,                      //< FIELD enable

	ENUM_DUMMY4WORD(CCIR_FIELD_ENUM)
} CCIR_FIELD_ENUM;

/*
	TE_SEL_ENUM
*/
typedef enum
{
	TE_SEL_GPIO,                        //< GPIO
	TE_SEL_EN,                          //< TE enable

	ENUM_DUMMY4WORD(TE_SEL_ENUM)
} TE_SEL_ENUM;

/*
	LCDTYPE2_ENUM
*/
typedef enum
{
	LCDTYPE2_ENUM_GPIO,                 //< GPIO
	LCDTYPE2_ENUM_CCIR656,              //< CCIR656
	LCDTYPE2_ENUM_CCIR601,              //< CCIR601
	LCDTYPE2_ENUM_SERIALRGB_8BITS = 4,  //< Serial RGB 8 bits
	LCDTYPE2_ENUM_SERIALRGB_6BITS,      //< Serial RGB 6 bits
	LCDTYPE2_ENUM_SERIALYCBCR_8BITS,    //< Serial YCbCr 8 bits
	LCDTYPE2_ENUM_PARALLELMI,           //< Parallel MI
	LCDTYPE2_ENUM_SERIALMI,             //< Serial MI

	ENUM_DUMMY4WORD(LCDTYPE2_ENUM)
} LCDTYPE2_ENUM;

/*
	CEC_ENUM
*/
typedef enum
{
	CEC_ENUM_GPIO,                      //< GPIO
	CEC_ENUM_CEC,                       //< CEC

	ENUM_DUMMY4WORD(CEC_ENUM)
} CEC_ENUM;

/*
	DDC_ENUM
*/
typedef enum
{
	DDC_ENUM_GPIO,                      //< GPIO
	DDC_ENUM_DDC,                       //< DDC

	ENUM_DUMMY4WORD(DDC_ENUM)
} DDC_ENUM;

/*
	HOTPLUG_ENUM
*/
typedef enum
{
	HOTPLUG_ENUM_GPIO,                  //< GPIO
	HOTPLUG_HOTPLUG,                    //< HOTPLUG

	ENUM_DUMMY4WORD(HOTPLUG_ENUM)
} HOTPLUG_ENUM;

/*
	SENSOR_ENUM
*/
typedef enum
{
	SENSOR_ENUM_GPIO,                   //< GPIO
	//SENSOR_ENUM_8BITS,                //< 8 bits sensor
	//SENSOR_ENUM_10BITS,               //< 10 bits sensor
	SENSOR_ENUM_12BITS_1ST,             //< 12 bits sensor 1st location
	SENSOR_ENUM_12BITS_2ND,             //< 12 bits sensor 2nd location
	SENSOR_ENUM_CCIR8BITS,              //< CCIR 8 bits sensor
	SENSOR_ENUM_CCIR16BITS,             //< CCIR 16 bits sensor
	SENSOR_ENUM_CSIMODE,                //< CSI mode

	ENUM_DUMMY4WORD(SENSOR_ENUM)
} SENSOR_ENUM;

/*
	SENSOR2_ENUM
*/
typedef enum
{
	SENSOR2_ENUM_GPIO,                   //< GPIO
	SENSOR2_ENUM_12BITS,                 //< 12 bits sensor
	SENSOR2_ENUM_CCIR8BITS,              //< CCIR 8 bits sensor

	ENUM_DUMMY4WORD(SENSOR2_ENUM)
} SENSOR2_ENUM;

/*
	PXCLK_SEL_ENUM
*/
typedef enum
{
	PXCLK_SEL_ENUM_GPIO,                //< Select as GPIO
	PXCLK_SEL_ENUM_PXCLK,               //< Select as PXCLK

	ENUM_DUMMY4WORD(PXCLK_SEL_ENUM)
} PXCLK_SEL_ENUM;

/*
	VD_HD_SEL_ENUM
*/
typedef enum
{
	VD_HD_SEL_ENUM_GPIO,                //< Select as GPIO
	VD_HD_SEL_ENUM_SIE_VDHD,         //< Mux VD/HD to SIE at 1st pinmux pad
	VD_HD_SEL_ENUM_SIE_VDHD2,       //< Mux VD/HD to SIE at 2nd pinmux pad

	ENUM_DUMMY4WORD(VD_HD_SEL_ENUM)
} VD_HD_SEL_ENUM;

/*
	SENMCLK_SEL_ENUM
*/
typedef enum
{
	SENMCLK_SEL_ENUM_GPIO,              //< Select as GPIO
	SENMCLK_SEL_ENUM_MCLK,              //< Select as MCLK at 1st pinmux pad
	SENMCLK_SEL_ENUM_MCLK_2ND,          //< Select as MCLK at 2nd pinmux pad
	SENMCLK_SEL_ENUM_MCLK_3RD,          //< Select as MCLK at 3rd pinmux pad

	ENUM_DUMMY4WORD(SENMCLK_SEL_ENUM)
} SENMCLK_SEL_ENUM;

/*
	MES_SEL_ENUM
*/
typedef enum
{
	MES_SEL_ENUM_GPIO,                  //< Select as GPIO
	MES_SEL_ENUM_1ST_PINMUX,            //< Select as MES at 1st pinmux pad
	MES_SEL_ENUM_2ND_PINMUX,            //< Select as MES at 2nd pinmux pad

	ENUM_DUMMY4WORD(MES_SEL_ENUM)
} MES_SEL_ENUM;

/*
	FLCTR_SEL_ENUM
*/
typedef enum
{
	FLCTR_SEL_ENUM_GPIO,                //< Select as GPIO
	FLCTR_SEL_ENUM_FLCTR,               //< Select as FLCTR

	ENUM_DUMMY4WORD(FLCTR_SEL_ENUM)
} FLCTR_SEL_ENUM;

/*
	SP_CLK_SEL_ENUM
*/
typedef enum
{
	SP_CLK_SEL_ENUM_GPIO,               //< Select as GPIO
	SP_CLK_SEL_ENUM_SPCLK,              //< Select as SP_CLK
	SP_CLK_SEL_ENUM_SP_2_CLK,           //< Select as SP_2_CLK
	SP_CLK_SEL_ENUM_SP_3_CLK,           //< Select as SP_3_CLK

	ENUM_DUMMY4WORD(SP_CLK_SEL_ENUM)
} SP_CLK_SEL_ENUM;

/*
	SP2_CLK_SEL_ENUM
*/
typedef enum
{
	SP2_CLK_SEL_ENUM_GPIO,              //< Select as GPIO
	SP2_CLK_SEL_ENUM_SP2CLK,            //< Select as SP2_CLK
	SP2_CLK_SEL_ENUM_SP2_2_CLK,         //< Select as SP2_2_CLK
	SP2_CLK_SEL_ENUM_SP2_3_CLK,         //< Select as SP2_3_CLK

	ENUM_DUMMY4WORD(SP2_CLK_SEL_ENUM)
} SP2_CLK_SEL_ENUM;

/*
	SHUTTER_ENUM
*/
typedef enum
{
	SHUTTER_ENUM_GPIO,                  //< Select as GPIO
	SHUTTER_ENUM_SHUTTER,               //< Select as SHUTTER

	ENUM_DUMMY4WORD(SHUTTER_ENUM)
} SHUTTER_ENUM;

/*
	STROBE_ENUM
*/
typedef enum
{
	STROBE_ENUM_GPIO,                   //< Select as GPIO
	STROBE_ENUM_STROBE,                 //< Select as STROBE

	ENUM_DUMMY4WORD(STROBE_ENUM)
} STROBE_ENUM;

/*
	MIPI_LVDS_CLK_ENUM
*/
typedef enum
{
	MIPI_LVDS_CLK_ENUM_GPI,             //< Select as GPI
	MIPI_LVDS_CLK_ENUM_CLK,             //< Select as Clock Lane

	ENUM_DUMMY4WORD(MIPI_LVDS_CLK_ENUM)
} MIPI_LVDS_CLK_ENUM;

/*
	MIPI_LVDS_DATA_ENUM
*/
typedef enum
{
	MIPI_LVDS_DATA_ENUM_GPI,            //< Select as GPI
	MIPI_LVDS_DATA_ENUM_DAT,            //< Select as Data Lane

	ENUM_DUMMY4WORD(MIPI_LVDS_DATA_ENUM)
} MIPI_LVDS_DATA_ENUM;

/*
	I2C_ENUM
*/
typedef enum
{
	I2C_ENUM_GPIO,                      //< Select as GPIO
	I2C_ENUM_I2C,                       //< Select as I2C on 1st pinmux pad
	I2C_ENUM_I2C_2ND,                //< Select as I2C on 2nd pinmux pad

	ENUM_DUMMY4WORD(I2C_ENUM)
} I2C_ENUM;

/*
	I2C_2_ENUM
*/
typedef enum
{
	I2C_2_ENUM_GPIO,                    //< Select as GPIO
	I2C_2_ENUM_I2C,                     //< Select as I2C

	ENUM_DUMMY4WORD(I2C_2_ENUM)
} I2C_2_ENUM;

/*
	I2C_3_ENUM
*/
typedef enum
{
	I2C_3_ENUM_GPIO,                      //< Select as GPIO
	I2C_3_ENUM_I2C,                       //< Select as I2C on 1st pinmux pad
	I2C_3_ENUM_I2C_2ND,                //< Select as I2C on 2nd pinmux pad
	I2C_3_ENUM_I2C_3RD,                //< Select as I2C on 3rd pinmux pad

	ENUM_DUMMY4WORD(I2C_3_ENUM)
} I2C_3_ENUM;

/*
	SPI_ENUM
*/
typedef enum
{
	SPI_DISABLE,                       //< Select as GPIO
	SPI_1ST_PINMUX,                    //< Select SPI on 1st pinmux pad
	SPI_2ND_PINMUX,                    //< Select SPI on 2nd pinmux pad
	SPI_3RD_PINMUX,                    //< Select SPI on 3rd pinmux pad

	ENUM_DUMMY4WORD(SPI_ENUM)
} SPI_ENUM;


/*
	SPI2_ENUM
*/
typedef enum
{
	SPI2_DISABLE,                       //< Select as GPIO
	SPI2_1ST_PINMUX,                    //< Select SPI2 on 1st pinmux pad
	SPI2_2ND_PINMUX,                    //< Select SPI2 on 2nd pinmux pad

	ENUM_DUMMY4WORD(SPI2_ENUM)
} SPI2_ENUM;

/*
	SPI3_ENUM
*/
typedef enum
{
	SPI3_DISABLE,                       //< Select as GPIO
	SPI3_1ST_PINMUX,                    //< Select SPI3 on 1st pinmux pad
	SPI3_2ND_PINMUX,                    //< Select SPI3 on 2nd pinmux pad
	SPI3_3RD_PINMUX,                    //< Select SPI3 on 3rd pinmux pad

	ENUM_DUMMY4WORD(SPI3_ENUM)
} SPI3_ENUM;

/*
	SPI3_RDY_ENUM
*/
typedef enum {
	SPI3_RDY_DISABLE,                   //< Select as GPIO
	SPI3_RDY_1ST_PINMUX,                //< Select SPI3 on 1st pinmux pad
	SPI3_RDY_2ND_PINMUX,                //< Select SPI3 on 2nd pinmux pad
	SPI3_RDY_3RD_PINMUX,                //< Select SPI3 on 3rd pinmux pad

	ENUM_DUMMY4WORD(SPI3_RDY_ENUM)
} SPI3_RDY_ENUM;

/*
	SPI4_ENUM
*/
typedef enum {
	SPI4_DISABLE,                       //< Select as GPIO
	SPI4_1ST_PINMUX,                    //< Select SPI4 on 1st pinmux pad
	SPI4_2ND_PINMUX,                    //< Select SPI4 on 2nd pinmux pad

	ENUM_DUMMY4WORD(SPI4_ENUM)
} SPI4_ENUM;


/*
    SPI4_RDY_ENUM
*/
typedef enum {
	SPI4_RDY_DISABLE,                   //< Select as GPIO
	SPI4_RDY_1ST_PINMUX,                //< Select SPI4 on 1st pinmux pad
	SPI4_RDY_2ND_PINMUX,                //< Select SPI4 on 2nd pinmux pad

	ENUM_DUMMY4WORD(SPI4_RDY_ENUM)
} SPI4_RDY_ENUM;

/*
	SPI5_ENUM
*/
typedef enum {
	SPI5_DISABLE,                       //< Select as GPIO
	SPI5_1ST_PINMUX,                    //< Select SPI5 on 1st pinmux pad
	SPI5_2ND_PINMUX,                    //< Select SPI5 on 2nd pinmux pad

	ENUM_DUMMY4WORD(SPI5_ENUM)
} SPI5_ENUM;

/*
	SPI_DAT_ENUM
*/
typedef enum {
	SPI_DAT_ENUM_1BIT,                  //< Select SPI data is 1 bit
	SPI_DAT_ENUM_2BIT,                  //< Select SPI data is 2 bits
	SPI_DAT_ENUM_4BIT,                  //< Select SPI data is 4 bits

	ENUM_DUMMY4WORD(SPI_DAT_ENUM)
} SPI_DAT_ENUM;

/*
	SDP_ENUM
*/
typedef enum {
	SDP_DISABLE,                       //< Select as GPIO
	SDP_1ST_PINMUX,                    //< Select SDP on 1st pinmux pad
	SDP_2ND_PINMUX,                    //< Select SDP on 2nd pinmux pad

	ENUM_DUMMY4WORD(SDP_ENUM)
} SDP_ENUM;

/*
	UART_ENUM
*/
typedef enum
{
	UART_ENUM_GPIO,                     //< Select as GPIO
	UART_ENUM_UART,                     //< Select as UART

	ENUM_DUMMY4WORD(UART_ENUM)
} UART_ENUM;

/*
	UART2_ENUM

	@note For UART2
*/
typedef enum
{
	UART2_ENUM_GPIO,                    //< Select as GPIO
	UART2_ENUM_1ST_PINMUX,              //< Select as UART2_1
	UART2_ENUM_2ND_PINMUX,              //< Select as UART2_2
	UART2_ENUM_3RD_PINMUX,              //< Select as UART2_3

	ENUM_DUMMY4WORD(UART2_ENUM)
} UART2_ENUM;

/*
	UART3_ENUM

	@note For UART3
*/
typedef enum
{
	UART3_ENUM_GPIO,                    //< Select as GPIO
	UART3_ENUM_1ST_PINMUX,              //< Select as UART3 1ST
	UART3_ENUM_2ND_PINMUX,              //< Select as UART3 2ND
	UART3_ENUM_3RD_PINMUX,              //< Select as UART3 3RD
	UART3_ENUM_4TH_PINMUX,              //< Select as UART3 4TH
	UART3_ENUM_5TH_PINMUX,              //< Select as UART3 5TH

	ENUM_DUMMY4WORD(UART3_ENUM)
} UART3_ENUM;

/*
	UART4_ENUM

	@note For UART4
*/
typedef enum
{
	UART4_ENUM_GPIO,                    //< Select as GPIO
	UART4_ENUM_1ST_PINMUX,              //< Select as UART4 1ST
	UART4_ENUM_2ND_PINMUX,              //< Select as UART4 2ND

	ENUM_DUMMY4WORD(UART4_ENUM)
} UART4_ENUM;

/*
	UART5_ENUM

	@note For UART5
*/
typedef enum
{
	UART5_ENUM_GPIO,                    //< Select as GPIO
	UART5_ENUM_1ST_PINMUX,              //< Select as UART5 1ST
	UART5_ENUM_2ND_PINMUX,              //< Select as UART5 2ND

	ENUM_DUMMY4WORD(UART5_ENUM)
} UART5_ENUM;


/*
	UART6_ENUM

	@note For UART6
*/
typedef enum
{
	UART6_ENUM_GPIO,                    //< Select as GPIO
	UART6_ENUM_1ST_PINMUX,              //< Select as UART6 1ST
	UART6_ENUM_2ND_PINMUX,              //< Select as UART6 2ND

	ENUM_DUMMY4WORD(UART6_ENUM)
} UART6_ENUM;



/*
	UART_CTSRTS_ENUM

	@note for UART2~3
*/
typedef enum
{
	UART_CTSRTS_GPIO,                   //< Select as UART without flow control
	UART_CTSRTS_PINMUX,                 //< Select as UART with flow control
	UART_CTSRTS_DIROE,                  //< Select as UART with flow control (DIR/OE)

	ENUM_DUMMY4WORD(UART_CTSRTS_ENUM)
} UART_CTSRTS_ENUM;

/*
	SIFCH_ENUM

	@note For SIF CH0, 1, 2~3
*/
typedef enum
{
	SIFCH_ENUM_GPIO,                    //< Select as GPIO
	SIFCH_ENUM_SIF,                     //< Select as SIF
	SIFCH_ENUM_SIF_2ND,                 //< Select as SIF 2nd pinmux
	SIFCH_ENUM_SIF_3RD,                 //< Select as SIF 3rd pinmux

	ENUM_DUMMY4WORD(SIFCH_ENUM)
} SIFCH_ENUM;

/*
	AUDIO_ENUM
*/
typedef enum
{
	AUDIO_ENUM_GPIO,                    //< Select as GPIO
	AUDIO_ENUM_I2S,                     //< Select as I2S_1 (on UART2_TX/UART2_RX/UART2_RTS/UART2_CTS)
	AUDIO_ENUM_I2S_2ND_PINMUX,          //< Select as I2S_2

	ENUM_DUMMY4WORD(AUDIO_ENUM)
} AUDIO_ENUM;

/*
	AUDIO_MCLK_ENUM
*/
typedef enum
{
	AUDIO_MCLK_GPIO,                    //< Select as GPIO
	AUDIO_MCLK_MCLK,                    //< Select as audio MCLK
	AUDIO_MCLK_MCLK_2ND_PINMUX,         //< Backward compatible

	ENUM_DUMMY4WORD(AUDIO_MCLK_ENUM)
} AUDIO_MCLK_ENUM;

/*
	DMIC_ENUM
*/
typedef enum {
	DMIC_ENUM_GPIO,                   //< Select as GPIO
	DMIC_ENUM_DMIC,                   //< Select as DMIC
	DMIC_ENUM_DMIC_2ND_PINMUX,        //< Select as DMIC 2ND
	DMIC_ENUM_DMIC_3RD_PINMUX,        //< Select as DMIC 3RD

	ENUM_DUMMY4WORD(DMIC_ENUM)
} DMIC_ENUM;

/*
	DMIC_DATA_ENUM
*/
typedef enum {
	DMIC_DATA_ENUM_GPIO,              //< Select as GPIO
	DMIC_DATA_ENUM_DMIC,              //< Select as DMIC DATA

	ENUM_DUMMY4WORD(DMIC_DATA_ENUM)
} DMIC_DATA_ENUM;

/*
	RTC_ENUM
*/
typedef enum {
	RTC_ENUM_GPIO,                    //< Select as GPIO
	RTC_ENUM_RTCCLK,                  //< Select as RTCCLK

	ENUM_DUMMY4WORD(RTC_ENUM)
} RTC_ENUM;


/*
	REMOTE_ENUM
*/
typedef enum
{
	REMOTE_ENUM_GPIO,                   //< Select as GPIO
	REMOTE_ENUM_REMOTE_1ST,             //< Select as REMOTE_1
	REMOTE_ENUM_REMOTE_2ND,             //< Select as REMOTE_2
	REMOTE_ENUM_REMOTE_3RD,             //< Select as REMOTE_3

	ENUM_DUMMY4WORD(REMOTE_ENUM)
} REMOTE_ENUM;


/*
	EJTAG_SEL_ENUM
*/
typedef enum
{
	EJTAG_SEL_ENUM_EJTAG_CPU1,          //< Select as EJTAG only CPU1
	EJTAG_SEL_ENUM_EJTAG_DAISYCHAIN,    //< Select as EJTAG Daisy chain

	ENUM_DUMMY4WORD(EJTAG_SEL_ENUM)
} EJTAG_SEL_ENUM;

/*
	PWM_ENUM
*/
typedef enum
{
	PWM_ENUM_GPIO,                      //< Select as GPIO
	PWM_ENUM_PWM,                       //< Select as PWM
	PWM_ENUM_PWM_2ND,                   //< Select as PWM 2ND
	PWM_ENUM_PWM_3RD,                   //< Select as PWM 3RD
	PWM_ENUM_PWM_4TH,                   //< Select as PWM 4TH
	PWM_ENUM_PWM_5TH,                   //< Select as PWM 5TH

	ENUM_DUMMY4WORD(PWM_ENUM)
} PWM_ENUM;

/*
	PICNT_ENUM
*/
typedef enum
{
	PICNT_ENUM_GPIO,                    //< Select as GPIO
	PICNT_ENUM_PICNT,                   //< Select as PI_CNT
	PICNT_ENUM_PICNT2,                  //< Select as PI_CNT2

	ENUM_DUMMY4WORD(PICNT_ENUM)
} PICNT_ENUM;

/*
	DAC_CLK_ENUM
*/
typedef enum
{
	DAC_CLK_ENUM_GPIO,                  //< Select as GPIO
	DAC_CLK_ENUM_DAC_CLK,               //< Select as DAC_CLK

	ENUM_DUMMY4WORD(DAC_CLK_ENUM)
} DAC_CLK_ENUM;

/*
	USB_VBUSI_ENUM
*/
typedef enum
{
	USB_VBUSI_ENUM_INACTIVE,            //< Inactive (VBUSI is low)
	USB_VBUSI_ENUM_ACTIVE,              //< Active (VBUSI is high)

	ENUM_DUMMY4WORD(USB_VBUSI_ENUM)
} USB_VBUSI_ENUM;

/*
	USB_ID_ENUM
*/
typedef enum
{
	USB_ID_ENUM_HOST,                   //< VBUS ID is host
	USB_ID_ENUM_DEVICE,                 //< VBUS ID is device

	ENUM_DUMMY4WORD(USB_ID_ENUM)
} USB_ID_ENUM;

/*
	ETH_ID_ENUM
*/
typedef enum
{
	ETH_ID_ENUM_GPIO,
	ETH_ID_ENUM_RMII,
	ETH_ID_ENUM_RMII_2,
	ETH_ID_ENUM_RGMII,

	ENUM_DUMMY4WORD(ETH_ID_ENUM)
} ETH_ID_ENUM;

/*
	ETH_EXTPHY_CLK_ENUM
*/
typedef enum {
	ETH_EXTPHY_CLK_GPIO,
	ETH_EXTPHY_CLK_FUNC,

	ENUM_DUMMY4WORD(ETH_EXTPHY_CLK_ENUM)
} ETH_EXTPHY_CLK_ENUM;

/*
	ETH_MDIO_ENUM
*/
typedef enum {
	ETH_MDIO_GPIO,
	ETH_MDIO_FUNC,
	ETH_MDIO_FUNC_2,

	ENUM_DUMMY4WORD(ETH_MDIO_ENUM)
} ETH_MDIO_ENUM;

/*
	ETH_LED_ENUM
*/
typedef enum {
	ETH_LED_ENUM_GPIO,
	ETH_LED_ENUM_LED1,
	ETH_LED_ENUM_LED2,

	ENUM_DUMMY4WORD(ETH_LED_ENUM)
} ETH_LED_ENUM;

/*
	LOCKN_ENUM
*/
typedef enum
{
	LOCKN_ENUM_GPIO,                   //< GPIO
	LOCKN_ENUM_LOCKN,                  //< LOCKN

	ENUM_DUMMY4WORD(LOCKN_ENUM)
} LOCKN_ENUM;

/*
	GPIO_ID_ENUM
*/
typedef enum
{
	GPIO_ID_EMUM_FUNC,                  //< pinmux is mapping to function
	GPIO_ID_EMUM_GPIO,                  //< pinmux is mapping to gpio

	ENUM_DUMMY4WORD(GPIO_ID_ENUM)
} GPIO_ID_ENUM;


#define TOP_SETREG(info, ofs,value)	   OUTW(info->top_base + ofs,(value))
#define TOP_GETREG(info, ofs)		   INW(info->top_base + ofs)

#define TGE_REG_ADDR(ofs)       (0xF0CC0000+(ofs))
#define TGE_GETREG(ofs)         INW(TGE_REG_ADDR(ofs))
#define TGE_SETREG(ofs,value)   OUTW(TGE_REG_ADDR(ofs), (value))
#define TGE_CONTROL_OFS 0x0

#define MAX_PAD_NUM  236

#define GPIO_SETREG(info, ofs,value)	   OUTW(info->gpio_base + ofs,(value))
#define GPIO_GETREG(info, ofs)		   INW(info->gpio_base + ofs)

struct nvt_pad_info {
	uint32_t pad_ds_pin;
	uint32_t driving;
	uint32_t pad_gpio_pin;
	uint32_t direction;
};

struct nvt_gpio_info {
	uint32_t gpio_pin;
	uint32_t direction;
};

struct nvt_pinctrl_info {
	u32 top_base;
	u32 pad_base;
	u32 gpio_base;
	PIN_GROUP_CONFIG top_pinmux[PIN_FUNC_MAX];
	struct nvt_pad_info pad[MAX_PAD_NUM];
};

#define MAX_MODULE_NAME 9

ER pinmux_init(struct nvt_pinctrl_info *info);
ER pad_init(struct nvt_pinctrl_info *info, uint32_t nr_pad);
void pinmux_parsing(struct nvt_pinctrl_info *info);
void pinmux_preset(struct nvt_pinctrl_info *info);
int pinmux_set_host(struct nvt_pinctrl_info *info, PINMUX_FUNC_ID id, uint32_t pinmux);
void pinmux_gpio_parsing(struct nvt_pinctrl_info *info);

#endif

