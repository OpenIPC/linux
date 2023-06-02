#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

extern int nvt_ctl_sen_module_init(void);
extern int nvt_ctl_sen_module_exit(void);
extern int nvt_ctl_sie_init(void);
extern int nvt_ctl_sie_exit(void);
extern int isf_vdocap_module_init(void);
extern int isf_vdocap_module_exit(void);

int __init kflow_videocapture_init(void)
{
	int ret;


	if ((ret=nvt_ctl_sen_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_ctl_sie_init()) !=0 ) {
		return ret;
	}
	if ((ret=isf_vdocap_module_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kflow_videocapture_exit(void)
{
	isf_vdocap_module_exit();
	nvt_ctl_sie_exit();
	nvt_ctl_sen_module_exit();
}

module_init(kflow_videocapture_init);
module_exit(kflow_videocapture_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kflow_videocapture driver");
MODULE_LICENSE("GPL");

#else
#endif