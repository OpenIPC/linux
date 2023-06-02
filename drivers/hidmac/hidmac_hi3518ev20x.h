#ifndef __HI_DMAC_HI3518EV200_H__
#define __HI_DMAC_HI3518EV200_H__

#define DDRAM_ADRS	0x80000000      /* fixed */
#define DDRAM_SIZE	0x3FFFFFFF      /* 1GB DDR. */

#define FLASH_BASE	0x10000000
#define FLASH_SIZE	0x04000000      /* (32MB) */


#define DMAC_INTSTATUS		0X00
#define DMAC_INTTCSTATUS	0X04
#define DMAC_INTTCCLEAR		0X08
#define DMAC_INTERRORSTATUS	0X0C

#define DMAC_INTERRCLR		0X10
#define DMAC_RAWINTTCSTATUS	0X14
#define DMAC_RAWINTERRORSTATUS	0X18
#define DMAC_ENBLDCHNS		0X1C
#define DMAC_CONFIG			0X30
#define DMAC_SYNC			0X34

#define DMAC_MAXTRANSFERSIZE	0x0fff /*the max length is denoted by 0-11bit*/
#define MAXTRANSFERSIZE		DMAC_MAXTRANSFERSIZE
#define DMAC_CxDISABLE		0x00
#define DMAC_CxENABLE		0x01

/*the definition for DMAC channel register*/
#define DMAC_CxBASE(i)		(0x100+i*0x20)
#define DMAC_CxSRCADDR(i)	DMAC_CxBASE(i)
#define DMAC_CxDESTADDR(i)	(DMAC_CxBASE(i)+0x04)
#define DMAC_CxLLI(i)		(DMAC_CxBASE(i)+0x08)
#define DMAC_CxCONTROL(i)	(DMAC_CxBASE(i)+0x0C)
#define DMAC_CxCONFIG(i)	(DMAC_CxBASE(i)+0x10)

/*the means the bit in the channel control register*/
#define DMAC_CxCONTROL_M2M	0x9d480000  /* Dwidth=32,burst size=4 */
#define DMAC_CxCONTROL_LLIM2M	0x0f480000  /* Dwidth=32,burst size=1 */
#define DMAC_CxCONTROL_LLIM2M_ISP	0x0b489000  /* Dwidth=32,burst size=1 */
#define DMAC_CxLLI_LM		0x01

#define DMAC_CxCONFIG_M2M	0xc000
#define DMAC_CxCONFIG_LLIM2M	0xc000

/*#define DMAC_CxCONFIG_M2M  0x4001*/
#define DMAC_CHANNEL_ENABLE	1
#define DMAC_CHANNEL_DISABLE	0xfffffffe

#define DMAC_CxCONTROL_P2M	0x89409000
#define DMAC_CxCONFIG_P2M	0xd000

#define DMAC_CxCONTROL_M2P	0x86089000
#define DMAC_CxCONFIG_M2P	0xc800

#define DMAC_CxCONFIG_SIO_P2M	0x0000d000
#define DMAC_CxCONFIG_SIO_M2P	0x0000c800

/*default the config and sync regsiter for DMAC controller*/
/*M1,M2 little endian, enable DMAC*/
#define DMAC_CONFIG_VAL		0x01
/*enable the sync logic for the 16 peripheral*/
#define DMAC_SYNC_VAL		0x0

#define DMAC_MAX_PERIPHERALS	16
#define MEM_MAX_NUM		2
#define CHANNEL_NUM		CONFIG_HI_DMAC_CHANNEL_NUM
#define DMAC_MAX_CHANNELS	CHANNEL_NUM

#define REG_BASE_UART0		0x20080000
#define UART0_DATA_REG		(REG_BASE_UART0 + 0x0)

#define REG_BASE_UART1		0x20090000
#define UART1_DATA_REG		(REG_BASE_UART1 + 0x0)

#define REG_BASE_UART2		0x200a0000
#define UART2_DATA_REG		(REG_BASE_UART2 + 0x0)

#define REG_BASE_SPI0		0x200c0000
#define SPI0_DATA_REG		(REG_BASE_SPI0 + 0x08)

#define REG_BASE_SPI1		0x200e0000
#define SPI1_DATA_REG		(REG_BASE_SPI1 + 0x08)

#define REG_BASE_I2C0		0x200d0000
#define I2C0_DATA_REG		(REG_BASE_I2C0 + 0x10)

#define REG_BASE_I2C1		0x20240000
#define I2C1_DATA_REG		(REG_BASE_I2C1 + 0x10)

#define REG_BASE_I2C2		0x20250000
#define I2C2_DATA_REG		(REG_BASE_I2C2 + 0x10)

/*the transfer control and configuration value for different peripheral*/

extern int g_channel_status[CHANNEL_NUM];


/*
 *	DMA config array!
 *	DREQ, FIFO, CONTROL, CONFIG, BITWIDTH
 */
dmac_peripheral  g_peripheral[DMAC_MAX_PERIPHERALS] = {
	/* periphal 0: I2C0 RX, 8bit width */
	{0, I2C0_DATA_REG, 0x99000000, 0x1000, 0},

	/* periphal 1: I2C0 TX, 8bit width */
	{1, I2C0_DATA_REG, 0x96000000, 0x0840, 0},

	/*periphal 2: I2C1 RX, 8bit width */
	{2, I2C1_DATA_REG, 0x99000000, 0x1004, 0},

	/*periphal 3: I2C1 TX, 8bit width */
	{3, I2C1_DATA_REG, 0x96000000, 0x08c0, 0},

	/*periphal 4: UART0 RX, 8bit width */
	{4, UART0_DATA_REG, 0x99000000, 0xd008, 0},

	/*periphal 5: UART0 TX, 8bit width */
	{5, UART0_DATA_REG, 0x96000000, 0xc940, 0},

	/*periphal 6: UART1 RX, 8bit width */
	{6, UART1_DATA_REG, 0x99000000, 0xd00c, 0},

	/*periphal 7: UART1 TX, 8bit width */
	{7, UART1_DATA_REG, 0x96000000, 0xc9c0, 0},

	/*periphal 8: UART2 RX, 8bit width */
	{8, UART2_DATA_REG, 0x99000000, 0xd010, 0},

	/*periphal 9: UART2 TX, 8bit width */
	{9, UART2_DATA_REG, 0x96000000, 0xca40, 0},

	/*periphal 10: I2C2 RX, 8bit width */
	{10, I2C2_DATA_REG, 0x99000000, 0x1014, 0},

	/*periphal 11: I2C2 TX, 8bit width */
	{11, I2C2_DATA_REG, 0x96000000, 0x0ac0, 0},

	/*periphal 12: SSP1 RX, 8bit width */
	{12, 0, 0x99000000, 0xd018, 0},

	/*periphal 13: SSP1 TX, 8bit width */
	{13, 0, 0x96000000, 0xcb40, 0},

	/*periphal 14: SSP0 RX, 8bit width */
	{14, 0, 0x99000000, 0xd01c, 0},

	/*periphal 15: SSP0 TX, 8bit width */
	{15, 0, 0x96000000, 0xcbc0, 0},
};
#endif
