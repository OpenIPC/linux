/**
    Header file for Interrupt module

    This file is the header file that define the API for Interrupt module.

    @file       Interrupt.h
    @ingroup    mIDrvSys_Interrupt
    @note       Nothing.

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _INTERRUPT_H
#define _INTERRUPT_H

#include <kwrap/nvt_type.h>

// Interrupt number <= 32
//typedef UINT32          INT_PTN;
// Interrupt number > 32
typedef UINT64          INT_PTN;
/**
    Interrupt module ID

    Interrupt module ID for int_getIRQId() and int_getDummyId().
*/
#define INT_GIC_SPI_START_ID    32

typedef enum {
	INT_ID_TIMER = INT_GIC_SPI_START_ID,	// 0
	INT_ID_SIE,								// 1
	INT_ID_SIE2,							// 2
	INT_ID_SIE3,							// 3

	INT_ID_DRTC,							// 4
	INT_ID_IPE,								// 5
	INT_ID_IME,								// 6
	INT_ID_DCE,								// 7

	INT_ID_IFE,								// 8
	INT_ID_IFE2,							// 9
	INT_ID_DIS,								//10
	INT_ID_CNN,								//11

	INT_ID_SDP,             				//12
	INT_ID_RHE,								//13
	INT_ID_DRE,								//14
	INT_ID_DAI,             				//15

	INT_ID_H26X,							//16
	INT_ID_JPEG,							//17
	INT_ID_GRAPHIC,							//18
	INT_ID_GRAPHIC2,						//19

	INT_ID_RSA,								//20
	INT_ID_ISE,								//21
	INT_ID_TGE,								//22
	INT_ID_TSE,								//23

	INT_ID_GPIO,							//24
	INT_ID_REMOTE,							//25
	INT_ID_PWM,								//26
	INT_ID_USB,								//27

	INT_ID_HASH,							//28
	INT_ID_NAND,							//29
	INT_ID_SDIO,							//30
	INT_ID_SDIO2,           				//31

	INT_ID_SDIO3,							//32
	INT_ID_DMA,								//33
	INT_ID_ETHERNET,						//34
	INT_ID_SPI,								//35

	INT_ID_SPI2,							//36
	INT_ID_SPI3,							//37
	INT_ID_CRYPTO,							//38
	INT_ID_ETH_REV_MII,     				//39

	INT_ID_SIF,								//40
	INT_ID_I2C,								//41
	INT_ID_I2C2,							//42
	INT_ID_UART,            				//43

	INT_ID_UART2,							//44
	INT_ID_UART3,							//45
	INT_ID_MDBC,							//46
	INT_ID_ADC,								//47

	INT_ID_IDE,								//48
	INT_ID_CNN2,							//49
	INT_ID_DSI,								//50
	INT_ID_DMA2,							//51

	INT_ID_AFFINE,          				//52
	INT_ID_IVE,								//53 (o)
	INT_ID_LVDS,							//54
	INT_ID_LVDS2,           				//55

	INT_ID_RTC,								//56
	INT_ID_WDT,								//57
	INT_ID_CG,								//58
	INT_ID_UVCP,            				//59

	INT_ID_I2C3,			//60
	INT_ID_SIE5,			//61	@ NT98528
	INT_ID_VPE,				//62	@ NT98528
	INT_ID_UART4,			//63	@ NT98528
	INT_ID_UART5,			//64	@ NT98528
	INT_ID_UART6,			//65	@ NT98528
	INT_ID_SPI4,			//66	@ NT98528
	INT_ID_SPI5,			//67	@ NT98528
	INT_ID_I2C4,			//68	@ NT98528
	INT_ID_I2C5,			//69	@ NT98528
	INT_ID_TIMER2, 			//70	@ NT98528
	INT_ID_GPIO2,			//71	@ NT98528
	INT_ID_SIF2,			//72	@ NT98528
	INT_ID_PWM2,			//73	@ NT98528
	INT_ID_SDE,				//74	@ NT98528
	INT_ID_DUMMY_DGPIO,
	INT_ID_CNT,
	INT_ID_MAX = INT_ID_CNT - INT_GIC_SPI_START_ID, //94


	INT_ID_WFI = 229,
	INT_ID_SIE4,
	INT_ID_GIC_TOTAL = 256,
	ENUM_DUMMY4WORD(INT_ID)
} INT_ID;

typedef union {
	INT_PTN reg[4];
	struct {
        UINT32  bInt_ID_PPI:32;
		//INT0
		UINT32  bInt_ID_TIMER: 1;
		UINT32  bInt_ID_SIE: 1;
		UINT32  bInt_ID_SIE2: 1;
		UINT32  bInt_ID_SIE3: 1;

		UINT32  bInt_ID_DRTC: 1;
		UINT32  bInt_ID_IPE: 1;
		UINT32  bInt_ID_IME: 1;
		UINT32  bInt_ID_DCE: 1;

		UINT32  bInt_ID_IFE: 1;
		UINT32  bInt_ID_IFE2: 1;
		UINT32  bInt_ID_DIS: 1;
		UINT32  bInt_ID_FDE: 1;

//      UINT32  bInt_ID_RDE:1;
		UINT32  bInt_ID_Ethernet_LPI: 1;
		UINT32  bInt_ID_RHE: 1;
		UINT32  bInt_ID_DRE: 1;
		UINT32  bInt_ID_DAI: 1;


		//INT16
		UINT32  bInt_ID_H264: 1;
		UINT32  bInt_ID_JPEG: 1;
		UINT32  bInt_ID_GRAPHIC: 1;
		UINT32  bInt_ID_GRAPHIC2: 1;

		UINT32  bInt_ID_RSA: 1;
		UINT32  bInt_ID_ISE: 1;
		UINT32  bInt_ID_TGE: 1;
		UINT32  bInt_ID_TSMUX: 1;

		UINT32  bInt_ID_GPIO: 1;
		UINT32  bInt_ID_REMOTE: 1;
		UINT32  bInt_ID_PWM: 1;
		UINT32  bInt_ID_USB: 1;

		UINT32  bInt_ID_HASH: 1;
		UINT32  bInt_ID_NAND: 1;
		UINT32  bInt_ID_SDIO: 1;
		UINT32  bInt_ID_SDIO2: 1;

		//INT32
		UINT32  bInt_ID_SDIO3: 1;
		UINT32  bInt_ID_DMA: 1;
		UINT32  bInt_ID_ETHERNET: 1;
		UINT32  bInt_ID_SPI: 1;

		UINT32  bInt_ID_SPI2: 1;
		UINT32  bInt_ID_SPI3: 1;
		UINT32  bInt_ID_SCE: 1;
		UINT32  bInt_ID_Ehternet_RevMII: 1;
//      UINT32  bInt_ID_SPI5:1;


		UINT32  bInt_ID_SIF: 1;
		UINT32  bInt_ID_I2C: 1;
		UINT32  bInt_ID_I2C2: 1;
		UINT32  bInt_ID_UART: 1;

		UINT32  bInt_ID_UART2: 1;
		UINT32  bInt_ID_UART3: 1;
		UINT32  bInt_ID_MDBC: 1;
		UINT32  bInt_ID_ADC: 1;

		//INT48
		UINT32  bInt_ID_IDE: 1;
		UINT32  bInt_ID_CNN2: 1;
		UINT32  bInt_ID_DSI: 1;
		UINT32  bInt_ID_DMA2: 1;

		//INT52
		UINT32  bInt_ID_AFFINE: 1;
		UINT32  bInt_ID_IVE: 1;
		UINT32  bInt_ID_LVDS: 1;        // LVDS& HiSPI& CSI
		UINT32  bInt_ID_LVDS2: 1;       //LVDS2&HiSPI2&CSI2

		//INT56
		UINT32  bInt_ID_RTC: 1;
		UINT32  bInt_ID_WDT: 1;
		UINT32  bInt_ID_CG: 1;
		UINT32  bInt_ID_SIE4: 1;		//@NT98528

		//INT60
		UINT32  bInt_ID_I2C3: 1;
		UINT32  bInt_ID_SIE5: 1;		//@NT98528
		UINT32  bInt_ID_VPE: 1;			//@NT98528
		UINT32  bInt_ID_UART4: 1;		//@NT98528

		//INT64
		UINT32  bInt_ID_UART5: 1;		//@NT98528
		UINT32  bInt_ID_UART6: 1;		//@NT98528
		UINT32  bInt_ID_SPI4: 1; 		//@NT98528
		UINT32  bInt_ID_SPI5: 1; 		//@NT98528

		//INT68
		UINT32  bInt_ID_I2C4: 1;
		UINT32  bInt_ID_I2C5: 1;
		UINT32  bInt_ID_TIMER2: 1;
		UINT32  bInt_ID_GPIO2: 1;

		//INT72
		UINT32  bInt_ID_SIF2: 1;
		UINT32  bInt_ID_PWM2: 1;
		UINT32  bInt_ID_SDE: 1;
		UINT32  bInt_ID_RSV: 1;

		//INT76 => 4
		UINT32  bReserved76: 4;
		//INT80=> 4
		UINT32  bReserved80: 4;
		//INT84=> 4
		UINT32  bReserved84: 4;
		//INT88=> 4
		UINT32  bReserved88: 4;
		//INT92=> 4
		UINT32  bReserved92: 4;
	} Bit;
} INT_GIC_ID_ENABLE, *PINT_GIC_ID_ENABLE;

typedef struct {
	INT_GIC_ID_ENABLE  int_id_enable;
} INT_INTC_ENABLE, *PINT_INTC_ENABLE;

#define IRQF_TRIGGER_NONE		0x00000000
#define IRQF_TRIGGER_RISING		0x00000001
#define IRQF_TRIGGER_FALLING	0x00000002
#define IRQF_TRIGGER_HIGH		0x00000004
#define IRQF_TRIGGER_LOW		0x00000008


typedef enum {
	IRQF_BH_PRI_LOW		=	0x00000100,
	IRQF_BH_PRI_MIDDLE	=	0x00000200,
	IRQF_BH_PRI_HIGH	=	0x00000400,

	IRQF_BH_PRI_DEFAULT	=	IRQF_BH_PRI_MIDDLE,
	ENUM_DUMMY4WORD(IRQF_BH)
}IRQF_BH;



enum irqreturn {
	IRQ_NONE			= (0 << 0),
	IRQ_HANDLED			= (1 << 0),
	IRQ_WAKE_THREAD		= (1 << 1),
};

typedef enum irqreturn irqreturn_t;
typedef irqreturn_t (* irq_handler_t)(int irq, void *dev);
typedef irqreturn_t (* irq_bh_handler_t)(int irq, unsigned long event, void *data);


extern void	irq_init(void);
extern int	request_irq(unsigned int irq, irq_handler_t handler, unsigned long flags, const char *name, void *dev);
extern void	free_irq(unsigned int irq, void *dev);

extern int	request_irq_bh(unsigned int irq, irq_bh_handler_t bh_handler, IRQF_BH flags);
extern void	free_irq_bh(unsigned int irq, void *dev);
extern int	kick_bh(unsigned int irq, unsigned long event, void *data);

extern void int_enable_multi(INT_INTC_ENABLE gic_int_en);
extern void int_disable_multi(INT_INTC_ENABLE gic_int_en);
extern void int_get_gic_enable(PINT_INTC_ENABLE gic_int_en);
extern void int_gic_set_target(unsigned int number, unsigned int target);




//@}

#endif
