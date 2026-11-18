/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/printk.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include "osal_lib_ax.h"
#include "osal_logdebug_ax.h"

#define ax_vprintk vprintk

static AX_OSAL_LIST_HEAD(list);

int AX_OSAL_DBG_printk(const char * fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = ax_vprintk(fmt, args);
	va_end(args);
	return r;
}

EXPORT_SYMBOL(AX_OSAL_DBG_printk);

void AX_OSAL_DBG_panic(const char * fmt, const char * fun, int line, const char * cond)
{
	panic(fmt);
}

EXPORT_SYMBOL(AX_OSAL_DBG_panic);

void AX_OSAL_DBG_LogOutput(int target, int level, const char *tag, const char * fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	ax_vprintk(fmt, args);
	va_end(args);
	return;
}

EXPORT_SYMBOL(AX_OSAL_DBG_LogOutput);

void AX_OSAL_DBG_ISPLogoutput(int level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	ax_vprintk(fmt, args);
	va_end(args);

	return;
}

EXPORT_SYMBOL(AX_OSAL_DBG_ISPLogoutput);

void AX_OSAL_DBG_NPULogoutput(int level, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	ax_vprintk(fmt, args);
	va_end(args);

	return;
}

EXPORT_SYMBOL(AX_OSAL_DBG_NPULogoutput);

int AX_OSAL_DBG_SetLogLevel(int level)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DBG_SetLogLevel);

int AX_OSAL_DBG_SetLogTarget(int level)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DBG_SetLogTarget);

int AX_OSAL_DBG_EnableTimestamp(int enable)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DBG_EnableTimestamp);

int AX_OSAL_DBG_EnableTraceEvent(int module, int enable)
{
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DBG_EnableTraceEvent);

int AX_OSAL_DBG_seq_show(struct seq_file *s, void *p)
{
	AX_PROC_DIR_ENTRY_T *oldsentry = s->private;
	AX_PROC_DIR_ENTRY_T sentry;
	if (oldsentry == NULL) {
		printk("%s error oldsentry == NULL\n", __func__);
		return -1;
	}
	memset(&sentry, 0, sizeof(AX_PROC_DIR_ENTRY_T));

	sentry.seqfile = s;
	sentry.private_data = oldsentry->private_data;
	oldsentry->read(&sentry);
	return 0;
}

EXPORT_SYMBOL(AX_OSAL_DBG_seq_show);

ssize_t AX_OSAL_DBG_procwrite(struct file * file, const char __user * buf, size_t count, loff_t * ppos)
{
	AX_PROC_DIR_ENTRY_T *item = PDE_DATA(file_inode(file));

	if (item && item->write) {
		return item->write(item, buf, count, (long long *)ppos);
	}

	return -ENOSYS;
}

EXPORT_SYMBOL(AX_OSAL_DBG_procwrite);

int AX_OSAL_DBG_procopen(struct inode *inode, struct file *file)
{
	AX_PROC_DIR_ENTRY_T *sentry = PDE_DATA(inode);
	if (sentry != NULL && sentry->open != NULL) {
		sentry->open(sentry);
	}
	return single_open(file, AX_OSAL_DBG_seq_show, sentry);
}

EXPORT_SYMBOL(AX_OSAL_DBG_procopen);

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 15, 0))
static struct proc_ops osal_proc_ops = {

	.proc_open = AX_OSAL_DBG_procopen,
	.proc_read = seq_read,
	.proc_write = AX_OSAL_DBG_procwrite,
	.proc_lseek = seq_lseek,
	.proc_release = single_release
};
#else
static struct file_operations osal_proc_ops = {

	.open = AX_OSAL_DBG_procopen,
	.read = seq_read,
	.write = AX_OSAL_DBG_procwrite,
	.llseek = seq_lseek,
	.release = single_release
};
#endif

AX_PROC_DIR_ENTRY_T *AX_OSAL_DBG_create_proc_entry(const char * name, AX_PROC_DIR_ENTRY_T * parent)
{
	struct proc_dir_entry *entry = NULL;
	AX_PROC_DIR_ENTRY_T *sentry = NULL;
	sentry = kmalloc(sizeof(AX_PROC_DIR_ENTRY_T), GFP_KERNEL);
	if (!sentry) {
		printk("%s alloc AX_PROC_DIR_ENTRY_T failed\n", __FUNCTION__);
		return NULL;
	}

	AX_OSAL_LIB_memset(sentry, 0, sizeof(AX_PROC_DIR_ENTRY_T));

	AX_OSAL_LIB_strncpy(sentry->name, name, sizeof(sentry->name) - 1);

	if (parent == NULL) {
		entry = proc_create_data(name, 0, NULL, &osal_proc_ops, sentry);
	} else {
		entry = proc_create_data(name, 0, parent->proc_dir_entry, &osal_proc_ops, sentry);
	}
	if (entry == NULL) {
		printk("%s create data failed\n", __FUNCTION__);
		kfree(sentry);
		sentry = NULL;
		return NULL;
	}
	sentry->proc_dir_entry = entry;
	sentry->open = NULL;
	sentry->parent = parent;
	AX_OSAL_LIB_init_list_head(&(sentry->node));
	AX_OSAL_LIB_list_add_tail(&(sentry->node), &list);
	return sentry;
}

EXPORT_SYMBOL(AX_OSAL_DBG_create_proc_entry);

AX_PROC_DIR_ENTRY_T *AX_OSAL_DBG_proc_mkdir(const char * name, AX_PROC_DIR_ENTRY_T * parent)
{
	struct proc_dir_entry *proc = NULL;
	AX_PROC_DIR_ENTRY_T *sproc = NULL;
	sproc = kmalloc(sizeof(AX_PROC_DIR_ENTRY_T), GFP_KERNEL);
	if (!sproc) {
		printk("%s alloc AX_PROC_DIR_ENTRY_T failed\n", __func__);
		return NULL;
	}
	AX_OSAL_LIB_memset(sproc, 0, sizeof(AX_PROC_DIR_ENTRY_T));

	AX_OSAL_LIB_strncpy(sproc->name, name, sizeof(sproc->name) - 1);

	if (parent != NULL) {
		proc = proc_mkdir_data(name, 0, parent->proc_dir_entry, sproc);
	} else {
		proc = proc_mkdir_data(name, 0, NULL, sproc);
	}
	if (proc == NULL) {
		printk("%s proc error\n", __func__);
		kfree(sproc);
		sproc = NULL;
		return NULL;
	}
	sproc->proc_dir_entry = proc;
	sproc->parent = parent;
	AX_OSAL_LIB_init_list_head(&(sproc->node));
	AX_OSAL_LIB_list_add_tail(&(sproc->node), &list);
	return sproc;

}

EXPORT_SYMBOL(AX_OSAL_DBG_proc_mkdir);

void AX_OSAL_DBG_remove_proc_entry(const char * name, AX_PROC_DIR_ENTRY_T * parent)
{
	struct AX_PROC_DIR_ENTRY *sproc = NULL;

	if (name == NULL) {
		printk("%s - parameter invalid!\n", __func__);
		return;
	}
	if (parent != NULL)
		remove_proc_entry(name, parent->proc_dir_entry);
	else
		remove_proc_entry(name, NULL);
	AX_OSAL_LIB_list_for_each_entry(sproc, &list, node) {
		if ((AX_OSAL_LIB_strncmp(sproc->name, name, sizeof(sproc->name)) == 0) && (parent == sproc->parent)) {
			AX_OSAL_LIB_list_del(&(sproc->node));
			break;
		}
	}
	if (sproc != NULL)
		kfree(sproc);
}

EXPORT_SYMBOL(AX_OSAL_DBG_remove_proc_entry);

void AX_OSAL_DBG_seq_printf(AX_PROC_DIR_ENTRY_T * entry, const char * fmt, ...)
{
	struct seq_file *s = (struct seq_file *)(entry->seqfile);
	va_list args;

	va_start(args, fmt);
	seq_vprintf(s, fmt, args);
	va_end(args);
	return;
}

EXPORT_SYMBOL(AX_OSAL_DBG_seq_printf);
