/*
 * Novatek IR remote receiver - BK platform internal header file
 *
 * Copyright Novatek Microelectronics Corp. 2020. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*************************
 *       Registers       *
 *************************/
/* 0x00 IR Control Register */
#define IR_CTRL_REG                      0x00
#define EN_BIT                           BIT(0)
#define NSCD_BIT                         BIT(1)
#define IIL_BIT                          BIT(2)
#define BI_PHASE_BIT                     BIT(3)
#define MSB_BIT                          BIT(4)
#define _NSCD_BIT_SHIFT                  1
#define _IIL_BIT_SHIFT                   2
#define _BI_PHASE_BIT_SHIFT              3
#define _MSB_BIT_SHIFT                   4

/* 0x04 IR Param0 Register */
#define IR_PARAM0_REG                    0x04
#define HIGH_TH_MASK                     GENMASK(14, 0)
#define TOUT_TH_MASK                     GENMASK(31, 16)
#define _TOUT_TH_MASK_SHIFT              16

/* 0x08 IR Param1 Register */
#define IR_PARAM1_REG                    0x08
#define LOW_TH0_MASK                     GENMASK(15, 0)
#define LOW_TH1_MASK                     GENMASK(31, 16)
#define _LOW_TH1_MASK_SHIFT              16

/* 0x0C IR Param2 Register */
#define IR_PARAM2_REG                    0x0C
#define CLK_DIV_MASK                     GENMASK(7, 0)
#define FIFO_SIZE_MASK                   GENMASK(13, 8)
#define CH_SEL_MASK                      GENMASK(20, 16)
#define _FIFO_SIZE_MASK_SHIFT            8
#define _CH_SEL_MASK_SHIFT               16

/* 0x10 IR Interrupt Status Register */
#define IR_STS_REG                       0x10
#define RD_STS_BIT                       BIT(0)
#define REP_STS_BIT                      BIT(1)
#define TOUT_STS_BIT                     BIT(2)
#define DATA_AMOUNT_MASK                 GENMASK(8, 3)

/* 0x14 IR Data Low Register */
#define IR_DATA_LO_REG                   0x14
#define DATA_LO_MASK                     GENMASK(31, 0)

/* 0x18 IR Data High Register */
#define IR_DATA_HI_REG                   0x18
#define DATA_HI_MASK                     GENMASK(31, 0)

/* 0x1C IR Interrupt Enable Register */
#define IR_INT_REG                       0x1C
#define RD_INT_BIT                       BIT(0)
#define REP_INT_BIT                      BIT(1)
#define TOUT_INT_BIT                     BIT(2)

static inline void nvt_ir_write(struct nvt_ir *ir, unsigned int off, unsigned int val)
{
	unsigned long flags = 0;

	spin_lock_irqsave(&ir->write_lock, flags);

	writel_relaxed(val, ir->reg + off);

	spin_unlock_irqrestore(&ir->write_lock, flags);
}

static inline unsigned int nvt_ir_read(struct nvt_ir *ir, unsigned int off)
{
	return readl_relaxed(ir->reg + off);
}

static inline void nvt_ir_masked_write(struct nvt_ir *ir, unsigned int off, unsigned int mask, unsigned int val)
{
	unsigned long flags = 0;
	u32 tmp = nvt_ir_read(ir, off);  /* be careful that some registers are cleared after reading */

	spin_lock_irqsave(&ir->write_lock, flags);

	tmp &= ~mask;
	tmp |= (val & mask);
	writel_relaxed(tmp, ir->reg + off);

	spin_unlock_irqrestore(&ir->write_lock, flags);
}

/**************************
 *       Parameters       *
 **************************/
typedef enum {
	IR_INT_RD    = 0x01,        /* Data ready status ID */
	IR_INT_REP   = 0x02,        /* Repeat code status ID */
	IR_INT_TOUT  = 0x04,        /* Timeout status ID */
	IR_INT_ALL   = 0x07,
} IR_INTERRUPT;

typedef enum {
	IR_CONFIG_ID_CLK_DIV,       /* Input clock divider. Use IR_CLK_DIV_SEL. */
	IR_CONFIG_ID_HIGH_TH,       /* HIGH_TH small than Pulse width, and lager than HIGH_THx2 regard as Header. Unit in Us. */
	IR_CONFIG_ID_LOW_TH0,       /* LOW_TH0 small than Space width of Header. Unit in Us. */
	IR_CONFIG_ID_LOW_TH1,       /* Space width of logic0 < LOW_TH1 < Space width of logic1, and LOW_TH1 small than Space width of Repeat. Unit in Us. */
	IR_CONFIG_ID_TIMEOUT_TH,    /* Unit in Us. Condition (1)Header-Pulse: Timeout_TH (2)Header-Space: Low_TH0 * 2 (3)Logic-Pulse: High_TH * 2 (4)Logic-Space: Low_TH1 * 1 */
	IR_CONFIG_ID_HEADER_DETECT, /* Detect header or not. Use IR_HEADER_DETECT */
	IR_CONFIG_ID_INPUT_INVERSE, /* Input signal inversion enable. Use true or false. */
	IR_CONFIG_ID_BI_PHASE_SEL,  /* Bi-phase mode enable. Use true or false. */
	IR_CONFIG_ID_DATA_ORDER,    /* Received data bit is MSB or LSB first. Use IR_DATA_ORDER. */
	IR_CONFIG_ID_DATA_LENGTH,   /* Data command bit length. Valid value from 1 to 64. */
	IR_CONFIG_ID_INPUT_CH_SEL,  /* Input channel. Valid value from 0 to 10. */
} IR_CONFIG_ID;

typedef enum {
	IR_CLK_DIV_120  = 120,      /* Clock divider is 120. But HW register will be filled with (120 - 2) */
} IR_CLK_DIV_SEL;

typedef enum {
	IR_FORCE_DETECT_HEADER,     /* Force detect header bit before each command. This can be used in the NEC/JVC like protocol. */
	IR_NO_DETECT_HEADER, 	    /* None of detect header bit. This can be used in the DENON/SHARP/Mitsubishi like protocol. */
} IR_HEADER_DETECT;

typedef enum {
	IR_DATA_LSB,                /* Data bit order is LSB first */
	IR_DATA_MSB,                /* Data bit order is MSB first */
} IR_DATA_ORDER;

typedef enum {
	IR_INPUT_CH_0 = 0,          /* Input channel 0    @ X_CPU_TDI      */
	IR_INPUT_CH_1,              /* Input channel 1    @ X_CPU_TMS      */
	IR_INPUT_CH_2,              /* Input channel 2    @ X_CPU_TCK      */
	IR_INPUT_CH_3,              /* Input channel 3    @ X_CPU_TDO      */
	IR_INPUT_CH_4,              /* Input channel 4    @ X_UART1_SOUT   */
	IR_INPUT_CH_5,              /* Input channel 5    @ X_UART1_SIN    */
	IR_INPUT_CH_6,              /* Input channel 6    @ X_GPIO_0       */
	IR_INPUT_CH_7,              /* Input channel 7    @ X_GPIO_1       */
	IR_INPUT_CH_8,              /* Input channel 8    @ X_GPIO_2       */
	IR_INPUT_CH_9,              /* Input channel 9    @ X_GPIO_3       */
	IR_INPUT_CH_10,             /* Input channel 10   @ X_GPIO_4       */
	IR_INPUT_CH_NUM,
} IR_INPUT_CH_SEL;