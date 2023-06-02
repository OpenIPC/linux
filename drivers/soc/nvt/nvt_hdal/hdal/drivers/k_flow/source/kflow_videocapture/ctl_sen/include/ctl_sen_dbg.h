#ifndef __MODULE_DBG_H_
#define __MODULE_DBG_H_

#define __MODULE__ ctl_sen
#define __DBGLVL__ 5          // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__ "*"      //*=All

#include "kflow_videocapture/ctl_sen.h"

#if defined(__FREERTOS) || defined(__ECOS) || defined(__UITRON)
#else
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>

#define debug_msg			printk
#endif

#include "kwrap/debug.h"

#endif

