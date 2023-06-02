#ifndef _RULE_CHECK_H_
#define _RULE_CHECK_H_

#ifdef __KERNEL__
#define PLL_SET_CLOCK_RATE(x, y)
#define PLL_GET_CLOCK_RATE(x)
#else
#define PLL_SET_CLOCK_RATE(x, y)	pll_set_clock_rate((x), (y))
#define PLL_GET_CLOCK_RATE(x)		pll_get_clock_rate((x))
#endif








#endif
