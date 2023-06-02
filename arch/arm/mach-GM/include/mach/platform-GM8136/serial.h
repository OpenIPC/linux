/*
 *  arch/arm/mach-GM/include/mach/platform-GM/serial.h
 *
 *  Serial port definition
 *
 *  Copyright (C) 2005 Faraday Corp. (http://www.faraday-tech.com)
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Note
 *
 *  There are 4 UARTs (FTUART010) in GM platform
 *
 * ChangeLog
 *
 *  Luke Lee  09/15/2005  Created.
 *  Luke Lee  11/16/2005  Add conditional compilation.
 */

#ifdef CONFIG_FPGA
#define CONFIG_UART_CLK			12000000
#else
#define CONFIG_UART_CLK			25000000
#endif

     /* UART CLK        PORT                      IRQ                   FLAGS       hub6 io_type iomem_base reg_shift*/
#ifdef CONFIG_SERIAL_UART1_IP
#define EXTENDED_UART_1 \
     { 0, BASE_BAUD, UART_FTUART010_1_VA_BASE, UART_FTUART010_1_IRQ, STD_COM_FLAGS,   0,    0,     NULL,       2 }, /* ttyS1 */
#else
#define EXTENDED_UART_1 \
     { 0, 0, 0, 0, 0, 0, 0, NULL, 0 },
#endif

#ifdef CONFIG_SERIAL_UART2_IP
#define EXTENDED_UART_2 \
     { 0, BASE_BAUD, UART_FTUART010_2_VA_BASE, UART_FTUART010_2_IRQ, STD_COM_FLAGS,   0,    0,     NULL,       2 }, /* ttyS2 */
#else
#define EXTENDED_UART_2 \
     { 0, 0, 0, 0, 0, 0, 0, NULL, 0 },
#endif

#define PLATFORM_MORE_SERIAL_PORTS \
        EXTENDED_UART_1            \
        EXTENDED_UART_2

/* set uart clock source select to PLL2 divider output / 2 and clock divided value */
static inline void ft_set_uartclk(void)
{
    unsigned int val, pval;

#if 0
    /* default is already set to pll2out_div2 */
    val = inl(PMU_FTPMU010_VA_BASE + 0x28);
    val &= ~(1 << 3);
    outl(val, PMU_FTPMU010_VA_BASE + 0x28);
#endif
    pval = 0;
    val = inl(PMU_FTPMU010_VA_BASE + 0x70);
    val = (val & ~(0x3F << 16)) | (pval << 16);
    outl(val, PMU_FTPMU010_VA_BASE + 0x70);
}

#define OSCH_FREQ   30000000

/* get UCLK at run time, CONFIG_UART_CLK is omitted */
static inline unsigned int ft_get_uartclk(void)
{
    unsigned int pval, mul, div;
    static int set_done = 0;

    if (set_done == 0) {
        ft_set_uartclk();
        set_done = 1;
    }

    pval = (inl(PMU_FTPMU010_VA_BASE + 0x70) >> 16) & 0x3F;

    /* uart clock source = PLL2 divider output / 2 */
    mul = (inl(PMU_FTPMU010_VA_BASE + 0x34) >> 4) & 0x7F;
    div = (inl(PMU_FTPMU010_VA_BASE + 0x34) >> 11) & 0x1F;

    return (OSCH_FREQ / div * mul) / 2 / (pval + 1);
}
