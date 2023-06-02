#ifndef _NVTMPP_MAIN_H__
#define _NVTMPP_MAIN_H__
#include <linux/cdev.h>
#include <linux/types.h>


#define MODULE_MINOR_ID      0
#define MODULE_MINOR_COUNT   1
#define MODULE_NAME          "nvtmpp"
#define NONCACHE_FLAG        0x80000000

typedef struct nvtmpp_drv_info {
	struct class *pmodule_class;
	struct device *pdevice;
	struct resource *presource;
	void __iomem  *io_addr;
	struct cdev cdev;
	dev_t dev_id;
	void  *paddr[2]; // the physical address get from frammap
	void  *vaddr[2]; // the virtual address get from frammap

	// proc entries
	struct proc_dir_entry *pproc_hdal_root;
	struct proc_dir_entry *pproc_module_root;
	struct proc_dir_entry *pproc_info_entry;
	struct proc_dir_entry *pproc_task_entry;
	struct proc_dir_entry *pproc_sem_entry;
	struct proc_dir_entry *pproc_cmd_entry;
	struct proc_dir_entry *pproc_help_entry;
} NVTMPP_DRV_INFO, *PNVTMPP_DRV_INFO;


#endif

