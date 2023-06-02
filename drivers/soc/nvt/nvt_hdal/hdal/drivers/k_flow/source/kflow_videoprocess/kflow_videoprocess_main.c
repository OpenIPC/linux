#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

extern int nvt_ctl_ipp_init(void);
extern int nvt_ctl_ipp_exit(void);
extern int nvt_ctl_ise_init(void);
extern int nvt_ctl_ise_exit(void);
extern int isf_vdoprc_module_init(void);
extern int isf_vdoprc_module_exit(void);

int __init kflow_videoprocess_init(void)
{
	int ret;

	if ((ret=nvt_ctl_ipp_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_ctl_ise_init()) !=0 ) {
		return ret;
	}
	if ((ret=isf_vdoprc_module_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kflow_videoprocess_exit(void)
{
	isf_vdoprc_module_exit();
	nvt_ctl_ipp_exit();
	nvt_ctl_ise_exit();
}

module_init(kflow_videoprocess_init);
module_exit(kflow_videoprocess_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kflow_videocapture driver");
MODULE_LICENSE("GPL");

#else
#endif