#ifndef PINCTRL_OSDEP_H_
#define PINCTRL_OSDEP_H_

#include <linux/list.h>
#include <linux/kernel.h>
#include <mach/fh_predefined.h>
#include <linux/string.h>

#define OS_LIST_INIT LIST_HEAD_INIT
#define OS_LIST struct list_head
#define OS_PRINT printk
#define OS_LIST_EMPTY INIT_LIST_HEAD
#define OS_NULL NULL

#define PINCTRL_ADD_DEVICE(name)                    	\
        list_add(&pinctrl_dev_##name.list,		\
	list)

#define PAD_NUM             (77)

#define MUX_NUM             (6)

#endif /* PINCTRL_OSDEP_H_ */
