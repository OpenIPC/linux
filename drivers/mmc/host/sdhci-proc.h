/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

/*
 *  MCI connection table manager
 */
#ifndef __MCI_PROC_H__
#define __MCI_PROC_H__

#include <linux/proc_fs.h>

#define MAX_CARD_TYPE	4
#define MAX_SPEED_MODE	5



#if defined(CONFIG_ARCH_XM72050200) || defined(CONFIG_ARCH_XM72050300) || \
	defined(CONFIG_ARCH_XM72020300) || defined(CONFIG_ARCH_XM76050100) || \
	defined(CONFIG_ARCH_XM72050500) || defined(CONFIG_ARCH_XM720XXX)
	#define MCI_SLOT_NUM 3
#endif

extern unsigned int slot_index;
extern struct mmc_host *mci_host[MCI_SLOT_NUM];

int mci_proc_init(void);
int mci_proc_shutdown(void);

#endif /*  __MCI_PROC_H__ */
