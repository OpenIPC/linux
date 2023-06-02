#ifndef __HI_DMAC_HI3516CV300_H__
#define __HI_DMAC_HI3516CV300_H__

#define DDRAM_ADRS	0x80000000      /* fixed */
#define DDRAM_SIZE	0x1FFFFFFF      /* 512M DDR. */

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
#define DMAC_CxCONTROL_LLIM2M		0x0f480000  /* Dwidth=32,burst size=1 */
#define DMAC_CxCONTROL_LLIM2M_ISP	0x0b489000  /* Dwidth=32,burst size=1 */
#define DMAC_CxLLI_LM		0x01

#define NUM_HAL_INTERRUPT_DMAC         (14 + 16)

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

#define REG_BASE_I2C0		0x120c0000
#define I2C0_DATA_RXF		(REG_BASE_I2C0 + 0x24)
#define I2C0_DATA_TXF		(REG_BASE_I2C0 + 0x20)


#define REG_BASE_UART0		0x12080000
#define UART0_DATA_REG		(REG_BASE_UART0 + 0x0)

#define REG_BASE_UART1		0x12090000
#define UART1_DATA_REG		(REG_BASE_UART1 + 0x0)

#define REG_BASE_UART2		0x120a0000
#define UART2_DATA_REG		(REG_BASE_UART2 + 0x0)

/*the transfer control and configuration value for different peripheral*/

extern int g_channel_status[CHANNEL_NUM];

dmac_peripheral  g_peripheral[DMAC_MAX_PERIPHERALS] = {
	/*periphal 0: UART0 RX, 8bit width */
	{0, UART0_DATA_REG, 0x99000000, 0xd000, 0},

	/*periphal 1: UART0 TX, 8bit width */
	{1, UART0_DATA_REG, 0x96000000, 0xc840, 0},

	/*periphal 2: UART1 RX, 8bit width */
	{2, UART1_DATA_REG, 0x99000000, 0xd004, 0},

	/*periphal 3: UART1 TX, 8bit width */
	{3, UART1_DATA_REG, 0x96000000, 0xc8c0, 0},

	/*periphal 4: UART2 RX, 8bit width */
	{4, UART2_DATA_REG, 0x99000000, 0xd008, 0},

	/*periphal 5: UART2 TX, 8bit width */
	{5, UART2_DATA_REG, 0x96000000, 0xc940, 0},

	/*periphal 6: I2C0 RX, 8bit width */
	{6, I2C0_DATA_RXF, 0x99000000, 0x100c, 0},

	/*periphal 7: I2C0 TX, 8bit width */
	{7, I2C0_DATA_TXF, 0x96000000, 0x9c0, 0},
};

#endif
