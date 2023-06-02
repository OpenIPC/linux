/*
 * Novatek IR remote receiver - FR platform internal header file
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
#define IR_CTRL_REG                           0x00
#define EN_BIT                                BIT(0)
#define TH_SEL_BIT                            BIT(4)
#define INV_BIT                               BIT(5)
#define MSB_BIT                               BIT(6)
#define REP_EN_BIT                            BIT(7)
#define HEADER_METHOD_MASK                    GENMASK(9, 8)
#define BI_PHASE_EN_BIT                       BIT(12)
#define BI_PHASE_DETECT_HEADER_TH_BIT         BIT(13)
#define LENGTH_MASK                           GENMASK(22, 16)
#define MATCH_LENGTH_MASK                     GENMASK(30, 24)
#define _TH_SEL_BIT_SHIFT                     4
#define _INV_BIT_SHIFT                        5
#define _MSB_BIT_SHIFT                        6
#define _REP_EN_BIT_SHIFT                     7
#define _HEADER_METHOD_MASK_SHIFT             8
#define _BI_PHASE_EN_BIT_SHIFT                12
#define _BI_PHASE_DETECT_HEADER_TH_BIT_SHIFT  13
#define _LENGTH_MASK_SHIFT                    16
#define _MATCH_LENGTH_MASK_SHIFT              24

/* 0x08 IR Interrupt Enable Register */
#define IR_INT_REG                            0x08
#define RD_INT_BIT                            BIT(0)
#define ERR_INT_BIT                           BIT(1)
#define MATCH_INT_BIT                         BIT(2)
#define REP_INT_BIT                           BIT(3)
#define OVERRUN_INT_BIT                       BIT(4)

/* 0x0C IR Interrupt Status Register */
#define IR_STS_REG                            0x0C
#define RD_STS_BIT                            BIT(0)
#define ERR_STS_BIT                           BIT(1)
#define MATCH_STS_BIT                         BIT(2)
#define REP_STS_BIT                           BIT(3)
#define OVERRUN_STS_BIT                       BIT(4)

/* 0x10 IR Data Low Register */
#define IR_DATA_LO_REG                        0x10
#define DATA_LO_MASK                          GENMASK(31, 0)

/* 0x14 IR Data High Register */
#define IR_DATA_HI_REG                        0x14
#define DATA_HI_MASK                          GENMASK(31, 0)

/* 0x18 IR Match Data Low Register */
#define IR_MATCH_DATA_LO_REG                  0x18
#define MATCH_DATA_LO_MASK                    GENMASK(31, 0)

/* 0x1C IR Match Data High Register */
#define IR_MATCH_DATA_HI_REG                  0x1C
#define MATCH_DATA_HI_MASK                    GENMASK(31, 0)

/* 0x20 IR Threshold Register 0 */
#define IR_TH0_REG                            0x20
#define HEADER_TH_MASK                        GENMASK(15, 0)
#define REP_TH_MASK                           GENMASK(31, 16)
#define _REP_TH_MASK_SHIFT                    16

/* 0x24 IR Threshold Register 1 */
#define IR_TH1_REG                            0x24
#define LOGIC_TH_MASK                         GENMASK(10, 0)
#define GSR_MASK                              GENMASK(23, 16)
#define _GSR_MASK_SHIFT                       16

/* 0x28 IR Threshold Register 2 */
#define IR_TH2_REG                            0x28
#define ERR_TH_MASK                           GENMASK(15, 0)

/* 0x2C IR RAW Data Low Register */
#define IR_RAW_DATA_LO_REG                    0x2C
#define RAW_DATA_LO_MASK                      GENMASK(31, 0)

/* 0x30 IR RAW Data High Register */
#define IR_RAW_DATA_HI_REG                    0x30
#define RAW_DATA_HI_MASK                      GENMASK(31, 0)

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
	IR_INT_RD       = 0x01,      /* Data ready status ID */
	IR_INT_ERR      = 0x02,      /* Timeout status ID */
	IR_INT_MATCH    = 0x04,      /* Match status ID */
	IR_INT_REP      = 0x08,      /* Repeat code status ID */
	IR_INT_OVERRUN  = 0x10,      /* Overrun status ID */
	IR_INT_ALL      = 0x1F,
} IR_INTERRUPT;

typedef enum {
	IR_CONFIG_ID_LOGIC_TH,       /* Configure the Logic-0/1 threshold to judge the data bit. Unit in Us. */
	IR_CONFIG_ID_GSR_TH,         /* Configure the glitch suppress threshold. Unit in Us. This value must be larger than or equal to 31. */
	IR_CONFIG_ID_REPEAT_TH,      /* Configure the repeat code judgement threshold. Unit in Us. */
	IR_CONFIG_ID_HEADER_TH,      /* Configure the header bit judgement threshold. Unit in Us. */
	IR_CONFIG_ID_ERROR_TH,       /* Configure the Error condition threshold. Unit in Us. */

	IR_CONFIG_ID_LOGIC_TH_HW,    /* Configure the Logic-0/1 threshold to judge the data bit. Unit is 1T depends on clk src. */
	IR_CONFIG_ID_GSR_TH_HW,      /* Configure the glitch suppress threshold. Unit is 1T depnds on clk src. */
	IR_CONFIG_ID_ERROR_TH_HW,    /* Configure the Error condition threshold. Unit is 1T depends on clk src. */

	IR_CONFIG_ID_THRESHOLD_SEL,  /* Configure the Data it is judged by the Space/Pulse period. Use IR_TH_SEL. */
	IR_CONFIG_ID_DATA_LENGTH,    /* Configure the Data Command Bit Length. Valid value from 1 to 64. */
	IR_CONFIG_ID_DATA_ORDER,     /* Configure the Received Data Bit is MSB or LSB first. Use IR_DATA_ORDER. */
	IR_CONFIG_ID_HEADER_DETECT,  /* Configure the Command Receive forcing detect header or not. Use IR_HEADER_DETECT */

	IR_CONFIG_ID_INPUT_INVERSE,  /* Configure the input signal inversion enable. Use true or false. */
	IR_CONFIG_ID_REPEAT_DET_EN,  /* Configure the NEC protocol repeat code detection enable. Use true or false. */
	IR_CONFIG_ID_MATCH_LOW,      /* Configure command comparision match LSB 32bits. */
	IR_CONFIG_ID_MATCH_HIGH,     /* Configure command comparision match MSB 32bits. */
	IR_CONFIG_ID_WAKEUP_ENABLE,  /* Enable the remote wakeup CPU condition after CPU power down mode. Use IR_INTERRUPT as input parameter. */
	IR_CONFIG_ID_WAKEUP_DISABLE, /* Disable the remote wakeup CPU condition after CPU power down mode. Use IR_INTERRUPT as input parameter. */
	IR_CONFIG_ID_CLK_SRC_SEL,    /* Configure remote input clock source. Use IR_CLK_SRC_SEL as input parameter. */

	IR_CONFIG_ID_BI_PHASE_EN,                /* Configure bi-phase mode enable. Use true or false */
	IR_CONFIG_ID_BI_PHASE_DETECT_HEADER_TH,  /* Configure bi-phase mode to detect the long header pulse, such as RC6 protocol. Use true or false */
	IR_CONFIG_ID_BI_PHASE_HEADER_LENGTH,     /* Configure bi-phase header length. Unit is half cycle. */
	IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_LOW,  /* Configure bi-phase header match data low. Each bit represents a level of half cycle, and the order is LSB fixed. */
	IR_CONFIG_ID_BI_PHASE_HEADER_MATCH_HIGH, /* Configure bi-phase header match data high. Each bit represents a level of half cycle, and the order is LSB fixed. */
	/*
     * EX: A 2T header with the following pattern should be described with match_length = 4 and match_data_l = 0xA (1010)
	 *       |------|      |------|
	 *       |      |      |      |
	 * ------|      |------|      |
	 *  1/2T   1/2T   1/2T   1/2T
     */

} IR_CONFIG_ID;

typedef enum {
	IR_SPACE_DET,               /* Detect the space cycle count as threshold */
	IR_PULSE_DET,               /* Detect the pulse cycle count as threshold */
} IR_TH_SEL;

typedef enum {
	IR_NO_DETECT_HEADER,        /* None of detect header bit. This can be used in the DENON/SHARP/Mitsubishi like protocol. */
	IR_FORCE_DETECT_HEADER,     /* Force detect header bit before each command. This can be used in the NEC/JVC like protocol. */
	IR_OPTIONAL_HEADER_DETECT,  /* Optinal detect header bit before each command. This can be used in the JVC like IR protocol. */
} IR_HEADER_DETECT;

typedef enum {
	IR_DATA_LSB,                /* Data bit order is LSB first */
	IR_DATA_MSB,                /* Data bit order is MSB first */
} IR_DATA_ORDER;

typedef enum {
	IR_CLK_SRC_RTC,             /* Clock frequence is 32768 Hz, and 1T = 1/32768 = 30.52 us */
	IR_CLK_SRC_OSC,             /* Clock frequence is 32000 HZ, and 1T = 1/32000 = 31.25 us */
	IR_CLK_SRC_EXT,             /* External clock */
	IR_CLK_SRC_3M,              /* Clock frequence is 3000000 Hz, and 1T = 1/3000000 = 0.333 us */
} IR_CLK_SRC_SEL;
