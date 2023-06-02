#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

extern int nvt_kdrv_sie_init(void);
extern int nvt_kdrv_sie_exit(void);
extern int nvt_tge_module_init(void);
extern int nvt_tge_module_exit(void);
extern int nvt_ssenif_module_init(void);
extern int nvt_ssenif_module_exit(void);
extern int nvt_senphy_module_init(void);
extern int nvt_senphy_module_exit(void);

extern int nvt_csi_module_init(void);
extern int nvt_csi_module_exit(void);
extern int nvt_lvds_module_init(void);
extern int nvt_lvds_module_exit(void);

int __init kdrv_videocapture_init(void)
{
	int ret;

	if ((ret=nvt_kdrv_sie_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_tge_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_csi_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_lvds_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_ssenif_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_senphy_module_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kdrv_videocapture_exit(void)
{
	nvt_senphy_module_exit();
	nvt_ssenif_module_exit();
	nvt_lvds_module_exit();
	nvt_csi_module_exit();
	nvt_tge_module_exit();
	nvt_kdrv_sie_exit();
}

module_init(kdrv_videocapture_init);
module_exit(kdrv_videocapture_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv_videocapture driver");
MODULE_LICENSE("GPL");

#else
#endif