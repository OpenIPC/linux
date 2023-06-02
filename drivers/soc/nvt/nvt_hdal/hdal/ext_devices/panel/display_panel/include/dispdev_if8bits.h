/*
    Display object object Golden Sample for driving RGB 8bits panel

    @file       dispdev_gsinf8bits.h
    @ingroup
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2011.  All rights reserved.
*/
#ifndef _DISPDEV_GSINF8BITS_H
#define _DISPDEV_GSINF8BITS_H
#include "dispdev_panlcomn.h"
#include "dispdev_platform.h"


/* Command code for adding delay between commands */
#define CMDDELAY_MS 0x80000000
#define CMDDELAY_US 0x40000000


//
//  GPIO SIF mode select
//
//@{
/* Check SIF of Settings */

#define USED_SIF_MODE     SIF_MODE_00

#if 0
#define OPTION_SIF_0000  0x0   ///< SIF bus mode 0000
#define OPTION_SIF_0001  0x1   ///< SIF bus mode 0001
#define OPTION_SIF_0010  0x2   ///< SIF bus mode 0010
#define OPTION_SIF_0011  0x3   ///< SIF bus mode 0011
#define OPTION_SIF_0100  0x4   ///< SIF bus mode 0100
#define OPTION_SIF_0101  0x5   ///< SIF bus mode 0101
#define OPTION_SIF_0110  0x6   ///< SIF bus mode 0110
#define OPTION_SIF_0111  0x7   ///< SIF bus mode 0111
#define OPTION_SIF_1000  0x8   ///< SIF bus mode 1000
#define OPTION_SIF_1001  0x9   ///< SIF bus mode 1001
#define OPTION_SIF_1010  0xA   ///< SIF bus mode 1010
#define OPTION_SIF_1011  0xB   ///< SIF bus mode 1011
#define OPTION_SIF_1100  0xC   ///< SIF bus mode 1100
#define OPTION_SIF_1101  0xD   ///< SIF bus mode 1101
#define OPTION_SIF_1110  0xE   ///< SIF bus mode 1110
#define OPTION_SIF_1111  0xF   ///< SIF bus mode 1111

#if (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_AUCN01)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TOPPOLY99000543_636)
#define SETTING_SIF_MODE                    OPTION_SIF_0100
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_AUA027DN01)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_AUA027DN03)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_AUCN02)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_PS20P00)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_RZWT27P02)
#define SETTING_SIF_MODE                    OPTION_SIF_0100
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TOPPOLY99000734)
#define SETTING_SIF_MODE                    OPTION_SIF_0100
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TOPPOLYA3AA)
#define SETTING_SIF_MODE                    OPTION_SIF_0100
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TOPPOLYTD025)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TOPPOLYTD030WHEA1)
#define SETTING_SIF_MODE                    OPTION_SIF_0100
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TXDT240C)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TXDT270M)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_WMF4823V47FLW)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_TXDT270CDR_2V7)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_ILI9341)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#elif (DISPDEV_IF8BITS_TYPE == DISPDEV_IF8BITS_D200N9309V3_ST7789V)
#define SETTING_SIF_MODE                    OPTION_SIF_0000
#else

#define SETTING_SIF_MODE                    OPTION_SIF_0000
#endif






#if (SETTING_SIF_MODE == OPTION_SIF_0000)     //Confirmed
#define USED_SIF_MODE     SIF_MODE_00

#define SIF_CLK_INIT()    gpio_setPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_clearPin(sif_clk)
#define SIF_CLK_END()     gpio_setPin(sif_clk)
#define SIF_SEN_INIT()    gpio_setPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_clearPin(sif_sen)
#define SIF_SEN_END()     gpio_setPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_0001)   //Not Confirmed
#define USED_SIF_MODE     SIF_MODE_01

#define SIF_CLK_INIT()    gpio_clearPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_setPin(sif_clk)
#define SIF_CLK_END()     gpio_clearPin(sif_clk)
#define SIF_SEN_INIT()    gpio_setPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_clearPin(sif_sen)
#define SIF_SEN_END()     gpio_setPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_0010)   //Not Confirmed
#define USED_SIF_MODE     SIF_MODE_02

#define SIF_CLK_INIT()    gpio_setPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_clearPin(sif_clk)
#define SIF_CLK_END()     gpio_setPin(sif_clk)
#define SIF_SEN_INIT()    gpio_clearPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_setPin(sif_sen)
#define SIF_SEN_END()     gpio_clearPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_0011)   //Not Confirmed
#define USED_SIF_MODE     SIF_MODE_03

#define SIF_CLK_INIT()    gpio_clearPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_setPin(sif_clk)
#define SIF_CLK_END()     gpio_clearPin(sif_clk)
#define SIF_SEN_INIT()    gpio_clearPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_setPin(sif_sen)
#define SIF_SEN_END()     gpio_clearPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_0100)   //Not Confirmed
#define USED_SIF_MODE     SIF_MODE_04

#define SIF_CLK_INIT()    gpio_clearPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_setPin(sif_clk)
#define SIF_CLK_END()     gpio_clearPin(sif_clk)
#define SIF_SEN_INIT()    gpio_setPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_clearPin(sif_sen)
#define SIF_SEN_END()     gpio_setPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_0101)   //Not Confirmed
#define USED_SIF_MODE     SIF_MODE_05

#define SIF_CLK_INIT()    gpio_setPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_clearPin(sif_clk)
#define SIF_CLK_END()     gpio_setPin(sif_clk)
#define SIF_SEN_INIT()    gpio_setPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_clearPin(sif_sen)
#define SIF_SEN_END()     gpio_setPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_0110)   //Not Confirmed
#define USED_SIF_MODE     SIF_MODE_06

#define SIF_CLK_INIT()    gpio_clearPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_setPin(sif_clk)
#define SIF_CLK_END()     gpio_clearPin(sif_clk)
#define SIF_SEN_INIT()    gpio_clearPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_setPin(sif_sen)
#define SIF_SEN_END()     gpio_clearPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_0111)   //Not Confirmed
#define USED_SIF_MODE     SIF_MODE_07

#define SIF_CLK_INIT()    gpio_setPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_clearPin(sif_clk)
#define SIF_CLK_END()     gpio_setPin(sif_clk)
#define SIF_SEN_INIT()    gpio_clearPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_setPin(sif_sen)
#define SIF_SEN_END()     gpio_clearPin(sif_sen)

#elif (SETTING_SIF_MODE == OPTION_SIF_1000)   //Confirmed
#define USED_SIF_MODE     SIF_MODE_08

#define SIF_CLK_INIT()    gpio_clearPin(sif_clk)
#define SIF_CLK_BEGIN()   gpio_setPin(sif_clk)
#define SIF_CLK_END()     gpio_clearPin(sif_clk)
#define SIF_SEN_INIT()    gpio_setPin(sif_sen)
#define SIF_SEN_BEGIN()   gpio_clearPin(sif_sen)
#define SIF_SEN_END()     gpio_setPin(sif_sen)

#else
#error("This SIF mode has not implement!");
#endif
//@}

#endif





T_LCD_PARAM     *dispdev_get_config_mode(UINT32 *mode_number);
T_PANEL_CMD     *dispdev_get_standby_cmd(UINT32 *cmd_number);

void            dispdev_write_to_lcd_sif(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value);
void            dispdev_write_to_lcd_gpio(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value);
void            dispdev_read_from_lcd_gpio(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 *value);

T_LCD_ROT       *dispdev_get_lcd_rotate_cmd(UINT32 *mode_number);
T_LCD_SIF_PARAM *dispdev_get_lcd_sif_param(void) __attribute__((weak));

T_LCD_PARAM     *dispdev_get_config_mode2(UINT32 *mode_number);
T_PANEL_CMD     *dispdev_get_standby_cmd2(UINT32 *cmd_number);

void            dispdev_write_to_lcd_sif2(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value);
void            dispdev_write_to_lcd_gpio2(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 value);
void            dispdev_read_from_lcd_gpio2(DISPDEV_IOCTRL p_disp_dev_control, UINT32 addr, UINT32 *value);

T_LCD_ROT       *dispdev_get_lcd_rotate_cmd2(UINT32 *mode_number);
T_LCD_SIF_PARAM *dispdev_get_lcd_sif_param2(void) __attribute__((weak));

void	dispdev_change_mode(UINT32 mode);

#endif
