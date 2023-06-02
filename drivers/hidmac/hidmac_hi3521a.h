#ifndef __HI_DMAC_HI3521A_H__
#define __HI_DMAC_HI3521A_H__

#define DDR_MEM_BASE    0x80000000
#define DDRAM_ADRS		DDR_MEM_BASE
#define DDRAM_SIZE		0x7FFFFFFF	/* 2GB DDR. */

#define FLASH_BASE		0x10000000
#define FLASH_SIZE		0x04000000	/* (32MB) */

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
#define DMAC_CxCONTROL_LLIM2M		0x0f480000  /* Dwidth=32,burst size=1 */
#define DMAC_CxCONTROL_LLIM2M_ISP	0x0b489000  /* Dwidth=32,burst size=1 */
#define DMAC_CxCONTROL_LLIP2M    0x0a000000
#define DMAC_CxCONTROL_LLIM2P    0x86089000
#define DMAC_CxLLI_LM		0x01

#define DMAC_CxCONFIG_M2M	0xc000
#define DMAC_CxCONFIG_LLIM2M	0xc000

/*#define DMAC_CxCONFIG_M2M 0x4001*/
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

/* Uart data register address */
#define REG_BASE_UART0              0x12080000
#define REG_BASE_UART1              0x12090000
#define REG_BASE_UART2              0x120a0000

#define REG_UART_DATA               0x0000

#define UART0_DATA_REG		(REG_BASE_UART0 + REG_UART_DATA)
#define UART1_DATA_REG		(REG_BASE_UART1 + REG_UART_DATA)
#define UART2_DATA_REG		(REG_BASE_UART2 + REG_UART_DATA)

/* SPI data register address */
#define SSP_REG_BASE                0x120d0000
#define REG_SSP_DATA                0x0008

#define SSP_DATA_REG		(SSP_REG_BASE + REG_SSP_DATA)

/* I2c data register address */
#define I2C_REG_BASE                0x120c0000
#define REG_I2C_DATA                0x0010

#define I2C_DATA_REG		(I2C_REG_BASE + REG_I2C_DATA)

/*the transfer control and configuration value for different peripheral*/

extern int g_channel_status[CHANNEL_NUM];
extern dmac_peripheral g_peripheral[DMAC_MAX_PERIPHERALS];

/*
 *	DMAC channel request default configure array
 * Request ID, peripheral data register address, Control, Config, width
 */
dmac_peripheral g_peripheral[DMAC_MAX_PERIPHERALS] = {
	/* Request 0: UART0 Rx 8bit width */
	{ 0, UART0_DATA_REG, 0x99000000, 0xd000, 0},

	/* Request 1: UART0 Tx 8bit width */
	{ 1, UART0_DATA_REG, 0x96000000, 0xc840, 0},

	/* Request 2: UART1 Rx 8bit width */
	{ 2, UART1_DATA_REG, 0x99000000, 0xd004, 0},

	/* Request 3: UART1 Tx 8bit width */
	{ 3, UART1_DATA_REG, 0x96000000, 0xc8c0, 0},

	/* Request 4: UART2 Rx 8bit width */
	{ 4, UART2_DATA_REG, 0x99000000, 0xd008, 0},

	/* Request 5: UART2 Tx 8bit width */
	{ 5, UART2_DATA_REG, 0x96000000, 0xc940, 0},

	/* Request 6: SSP Rx */
	{ 6, SSP_DATA_REG, 0x99000000, 0xd00c, 0},

	/* Request 7: SSP Tx */
	{ 7, SSP_DATA_REG, 0x96000000, 0xc9c0, 0},

	/* Request 8: I2C Rx 8bit width */
	{ 8, I2C_DATA_REG, 0x99000000, 0x1010, 0},

	/* Request 9: I2C Tx 8bit width */
	{ 9, I2C_DATA_REG, 0x96000000, 0x0a40, 0},
};
#endif /* End of __HI_DMAC_HI3521A_H__ */

