#ifndef __PM_H
#define	__PM_H

struct ak_wakeup_gpio {
	int pin;
	char wakeup_pol;	
};

struct ak_pm_pdata {
	struct ak_wakeup_gpio *gpios;
	int nr_gpios;
};

/* ak39_pm_init
 *
 * called from board at initialisation time to setup the power
 * management
*/

#ifdef CONFIG_PM
//extern int __init ak39_pm_init(void);
#else
static inline int ak39_pm_init(void)
{
	return 0;
}
#endif

#endif /* __PM_H */


