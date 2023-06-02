#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

extern int nvtmpp_module_init(void);
extern int nvtmpp_module_exit(void);
extern int isf_flow_module_init(void);
extern int isf_flow_module_exit(void);
extern int nvt_gximage_module_init(void);
extern int nvt_gximage_module_exit(void);
extern int fastboot_module_init(void);
extern int fastboot_module_exit(void);

int __init kflow_common_init(void)
{
	int ret;


	if ((ret=nvtmpp_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=isf_flow_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_gximage_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=fastboot_module_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kflow_common_exit(void)
{
	fastboot_module_exit();
	nvt_gximage_module_exit();
	isf_flow_module_exit();
	nvtmpp_module_exit();
}

module_init(kflow_common_init);
module_exit(kflow_common_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kflow_common driver");
MODULE_LICENSE("GPL");

#else
#endif