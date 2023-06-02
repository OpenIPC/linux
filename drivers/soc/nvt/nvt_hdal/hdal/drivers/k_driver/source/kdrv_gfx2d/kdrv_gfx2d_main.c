#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

extern int nvt_graphic_module_init(void);
extern int nvt_graphic_module_exit(void);
extern int nvt_kdrv_ise_module_init(void);
extern int nvt_kdrv_ise_module_exit(void);

int __init kdrv_gfx2d_init(void)
{
	int ret;

	if ((ret=nvt_graphic_module_init()) !=0 ) {
		return ret;
	}
	if ((ret=nvt_kdrv_ise_module_init()) !=0 ) {
		return ret;
	}

	return 0;
}

void __exit kdrv_gfx2d_exit(void)
{
	nvt_kdrv_ise_module_exit();
	nvt_graphic_module_exit();
}

module_init(kdrv_gfx2d_init);
module_exit(kdrv_gfx2d_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("kdrv_gfx2d driver");
MODULE_LICENSE("GPL");

#else
#endif