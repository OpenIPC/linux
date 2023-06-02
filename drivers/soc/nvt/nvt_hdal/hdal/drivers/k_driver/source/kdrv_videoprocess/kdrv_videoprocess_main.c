#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

extern int nvt_kdrv_dce_module_init(void);
extern int nvt_kdrv_dce_module_exit(void);
extern int nvt_kdrv_ife_module_init(void);
extern int nvt_kdrv_ife_module_exit(void);
extern int nvt_kdrv_ife2_module_init(void);
extern int nvt_kdrv_ife2_module_exit(void);
extern int nvt_kdrv_ime_module_init(void);
extern int nvt_kdrv_ime_module_exit(void);
extern int nvt_kdrv_ipe_module_init(void);
extern int nvt_kdrv_ipe_module_exit(void);
extern int nvt_kdrv_ipp_module_init(void);
extern int nvt_kdrv_ipp_module_exit(void);


int __init kdrv_videoprocess_init(void)
{
	int ret;


	if ((ret=nvt_kdrv_dce_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_kdrv_ife_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_kdrv_ipe_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_kdrv_ime_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_kdrv_ipp_module_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kdrv_videoprocess_exit(void)
{
	nvt_kdrv_ipp_module_exit();
	nvt_kdrv_ime_module_exit();
	nvt_kdrv_ipe_module_exit();
	//nvt_kdrv_ife2_module_exit();
	nvt_kdrv_ife_module_exit();
	nvt_kdrv_dce_module_exit();

}

module_init(kdrv_videoprocess_init);
module_exit(kdrv_videoprocess_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv_videoprocess driver");
MODULE_LICENSE("GPL");

#else
#endif