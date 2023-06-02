/*
 *  MCI connection table manager
 */
#ifndef __MCI_PROC_H__
#define __MCI_PROC_H__

#include <linux/proc_fs.h>

#define MAX_CARD_TYPE	4
#define MAX_SPEED_MODE	5

#ifdef CONFIG_ARCH_HI3559AV100
	#define MCI_SLOT_NUM 3
#endif

extern unsigned int slot_index;
extern struct mmc_host *mci_host[MCI_SLOT_NUM];

int mci_proc_init(void);
int mci_proc_shutdown(void);

#endif /*  __MCI_PROC_H__ */
