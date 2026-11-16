/*
 * ingenic RTC register definition
 * Copyright (C) 2014 Ingenic Semiconductor Co., Ltd.
 *
 */
#ifndef __INGENIC_RTC_H__
#define __INGENIC_RTC_H__

/*Case option definition for the 'ingenic_rtc_ioctl' function*/
#define RTC_CMD_BASE 'R'
#define SET_PD_STEP _IOW(RTC_CMD_BASE, 1u, int)
#define SET_DDRBUFER_EN _IOW(RTC_CMD_BASE, 2u, int)
#define SET_DDRBUFER_DISEN _IOW(RTC_CMD_BASE, 3u, int)
#define PRINT_INFO _IOW(RTC_CMD_BASE, 4u, int)
#define SET_RTCCR_1HZIE_ON _IOW(RTC_CMD_BASE, 5u, int)
#define SET_RTCCR_1HZIE_OFF _IOW(RTC_CMD_BASE, 6u, int)
#define SET_WKUPPINCR_P_RST_EN _IOW(RTC_CMD_BASE, 7u, int)
#define SET_WKUPPINCR_P_RST_OFF _IOW(RTC_CMD_BASE, 8u, int)
#define CTRL_RTC_REG _IOW(RTC_CMD_BASE, 9u, int)

/*
 * RTC registers offset address definition
 */
#define RTC_RTCCR		(0x00)	/* rw, 32, 0x00000081 */
#define RTC_RTCSR		(0x04)	/* rw, 32, 0x???????? */
#define RTC_RTCSAR		(0x08)	/* rw, 32, 0x???????? */
#define RTC_RTCGR		(0x0c)	/* rw, 32, 0x0??????? */
#define RTC_HCR			(0x20)  /* rw, 32, 0x00000000 */
#define RTC_HWFCR		(0x24)  /* rw, 32, 0x0000???0 */
#define RTC_HRCR		(0x28)  /* rw, 32, 0x00000??0 */
#define RTC_HWCR		(0x2c)  /* rw, 32, 0x00000008 */
#define RTC_HWRSR		(0x30)  /* rw, 32, 0x00000000 */
#define RTC_HSPR		(0x34)  /* rw, 32, 0x???????? */
#define RTC_WENR		(0x3c)  /* rw, 32, 0x00000000 */
#define RTC_WKUPPINCR		(0x48)	/* rw, 32, 0x00af0064 */
/*
 * RTC registers common define
 */

/* RTC control register(RTCCR) */
#define RTCCR_WRDY		BIT(7)
#define RTCCR_1HZ		BIT(6)
#define RTCCR_1HZIE		BIT(5)
#define RTCCR_AF		BIT(4)
#define RTCCR_AIE		BIT(3)
#define RTCCR_AE		BIT(2)
#define RTCCR_SELEXC		BIT(1)
#define RTCCR_RTCE		BIT(0)

/* Generate the bit field mask from msb to lsb */
#define BITS_H2L(msb, lsb)  ((0xFFFFFFFF >> (32-((msb)-(lsb)+1))) << (lsb))

/* RTC regulator register(RTCGR) */
#define RTCGR_LOCK		BIT(31)
#define RTCGR_ADJC_LSB		16
#define RTCGR_ADJC_MASK		BITS_H2L(25, RTCGR_ADJC_LSB)
#define RTCGR_NC1HZ_LSB		0
#define RTCGR_NC1HZ_MASK	BITS_H2L(15, RTCGR_NC1HZ_LSB)

/* Hibernate control register(HCR) */
#define HCR_PD			BIT(0)

/* Hibernate wakeup filter counter register(HWFCR) */
#define HWFCR_LSB		5
#define HWFCR_MASK		BITS_H2L(15, HWFCR_LSB)
#define HWFCR_WAIT_TIME(ms)	(((ms) << HWFCR_LSB) > HWFCR_MASK ? HWFCR_MASK : ((ms) << HWFCR_LSB))

/* Hibernate reset counter register(HRCR) */
#define HRCR_LSB		11
#define HRCR_MASK		BITS_H2L(14, HRCR_LSB)
#define HRCR_WAIT_TIME(ms)     (ms < 62 ? 0 : (((ms / 62 - 1) << HRCR_LSB) > HRCR_MASK ? HRCR_MASK : ((ms / 62 - 1) << HRCR_LSB)))

/* Hibernate wakeup control register(HWCR) */

/* Power detect default value; this value means enable */
#define EPDET_DEFAULT           (0x5aa5a5a)
#define EPDET_ENABLE		(0x5aa5a5a)
#define EPDET_DISABLE		(0x1a55a5a5)
#define HWCR_EPDET		BIT(3)
#define HWCR_WKUPVL		BIT(2)
#define HWCR_EALM		BIT(0)

/* Hibernate wakeup status register(HWRSR) */
#define HWRSR_APD		BIT(8)
#define HWRSR_HR		BIT(5)
#define HWRSR_PPR		BIT(4)
#define HWRSR_PIN		BIT(1)
#define HWRSR_ALM		BIT(0)

/* write enable pattern register(WENR) */
#define WENR_WEN		BIT(31)
#define WENR_WENPAT_LSB		0
#define WENR_WENPAT_MASK	BITS_H2L(15, WENR_WENPAT_LSB)
#define WENR_WENPAT_WRITABLE	(0xa55a)

/* Hibernate scratch pattern register(HSPR) */
#define HSPR_RTCV               (0x52544356)      /* The value is 'RTCV', means rtc is valid */

/*WKUP_PIN_RST control register (WKUPPINCR)*/
#define WKUPPINCR_DEFAULT	(0x00af0064)
#define WKUPPINCR_DEFAULT_V1	(0x00a30064)

#define WKUPPINCR_P_RST_EN_DEF	(0x4)
#define WKUPPINCR_P_RST_EN	(0xb)
#define WKUPPINCR_P_RST_CLEAR	(0xf)
#define WKUPPINCR_OSC_EN	BIT(16)
#define WKUPPINCR_FASTBOOT	BIT(20)
#define WKUPPINCR_BUFFEREN	BIT(21)
#define WKUPPINCR_RAMSLEEP	BIT(22)
#define WKUPPINCR_RAMSHUT	BIT(23)

/* The divider is decided by the RTC clock frequency. */
#define RTC_FREQ_DIVIDER	(32768 - 1)
#define ms2clycle(x)  (((x) * RTC_FREQ_DIVIDER) / 1000)

struct rtc_private {
	int version_num;
};

/*ingenic rtc device struct*/
struct ingenic_rtc {
	int irq;
	struct clk *clk;
	spinlock_t lock;
	spinlock_t rd_lock;
	spinlock_t wr_lock;
	void __iomem *iomem;
	struct mutex	mutexlock;
	struct mutex	ioctl_mutexlock;
	struct mutex	mutex_wr_lock;
	struct resource *res;
	struct work_struct work;
	struct rtc_device *rtc;
	struct rtc_time rtc_alarm;
	struct rtc_private *priv;
#ifdef CONFIG_SUSPEND_TEST
	unsigned int sleep_count;
	unsigned int os_alarm_time;
	unsigned int save_rtccr;
#endif
};

struct set_reg{
    int ctrl;
    unsigned int reg;
    unsigned int val;
};

#endif /* __INGENIC_RTC_H__ */
