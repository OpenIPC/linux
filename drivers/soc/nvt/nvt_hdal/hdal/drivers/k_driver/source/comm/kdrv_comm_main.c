#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <kwrap/verinfo.h>

extern int nvt_ddr_arb_module_init(void);
extern int nvt_ddr_arb_module_exit(void);
extern int nvt_timer_module_init(void);
extern int nvt_timer_module_exit(void);
extern int nvt_pwm_module_init(void);
extern int nvt_pwm_module_exit(void);
extern int nvt_sif_init(void);
extern int nvt_sif_exit(void);
extern int log_init(void);
extern int log_clearnup(void);

int __init kdrv_comm_init(void)
{
	int ret;

	if ((ret=nvt_pwm_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_timer_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_ddr_arb_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=log_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_sif_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kdrv_comm_exit(void)
{
	nvt_sif_exit();
	log_clearnup();
	nvt_ddr_arb_module_exit();
	nvt_timer_module_exit();
	nvt_pwm_module_exit();
}

module_init(kdrv_comm_init);
module_exit(kdrv_comm_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv_comm driver");
MODULE_LICENSE("GPL");

#else
#endif
