#ifndef __TSE_PROC_H_
#define __TSE_PROC_H_
#include "tse_main.h"

struct proc_file_info_t {
	char *name;
	struct proc_dir_entry *entry;
	struct file_operations *op;
};

int nvt_tse_proc_init(PTSE_DRV_INFO pdrv_info);
int nvt_tse_proc_remove(PTSE_DRV_INFO pdrv_info);
int nvt_tse_proc_register_cmd(struct proc_dir_entry *root, struct proc_file_info_t *pfile);
int nvt_tse_proc_unregister_cmd(struct proc_file_info_t *pfile);

#endif //__TSE_PROC_H_
