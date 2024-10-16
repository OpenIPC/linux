/*
 * Some macro and struct of SUNXI.
 *
 * Copyright (C) 2018 Allwinner.
 *
 * Matteo <duanmintao@allwinnertech.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2.  This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef _RTC_SUNXI_COMMON_H_
#define _RTC_SUNXI_COMMON_H_

/*
 * Time unit conversions
 */
#define SEC_IN_MIN				60
#define SEC_IN_HOUR				(60 * SEC_IN_MIN)
#define SEC_IN_DAY				(24 * SEC_IN_HOUR)

/*
 * The year parameter passed to the driver is usually an offset relative to
 * the year 1900. This macro is used to convert this offset to another one
 * relative to the minimum year allowed by the hardware.
 */
#define SUNXI_YEAR_OFF(x)			((x)->min - 1900)

/*
 * min and max year are arbitrary set considering the limited range of the
 * hardware register field
 */
struct sunxi_rtc_data_year {
	unsigned int min;		/* min year allowed */
	unsigned int max;		/* max year allowed */
	unsigned int mask;		/* mask for the year field */
	unsigned int yshift;		/* bit shift to get the year */
	unsigned char leap_shift;	/* bit shift to get the leap year */
};

struct sunxi_rtc_dev {
	struct rtc_device *rtc;
	struct device *dev;
	struct sunxi_rtc_data_year *data_year;
	struct clk *clk;
	void __iomem *base;
	void __iomem *prcm_base;
	int irq;
};
#endif /* end of _RTC_SUNXI_COMMON_H_ */

