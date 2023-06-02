/*
* Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
*
* This program is free software; you can redistribute  it and/or modify it
* under  the terms of  the GNU General Public License as published by the
* Free Software Foundation;  either version 2 of the  License, or (at your
* option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include "core.h"

#define DWC3_PROC_ROOT "dwc3"
#define DWC3_PROC_CONNECTED_STATUS "csts"

static struct proc_dir_entry *proc_dwc3_dir = NULL;
static int proc_dwc3_dir_cnt = 0;

static void dwc3_stats_seq_printout(struct seq_file *s)
{
	struct dwc3 *dwc = s->private;

	switch(dwc->udc_connect_status) {
		case UDC_CONNECT_HOST:
			seq_puts(s, "cnt2host\n");
			break;

		case UDC_CONNECT_CHARGER:
			seq_puts(s, "cnt2charger\n");
			break;

		default:
			seq_puts(s, "disconnected\n");
			break;
	}
}

/* define parameters where showed in proc file */
static int dwc3_stats_seq_show(struct seq_file *s, void *v)
{
	dwc3_stats_seq_printout(s);
	return 0;
}

/* proc file open*/
static int dwc3_stats_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, dwc3_stats_seq_show, PDE_DATA(inode));
};

/* proc file operation */
static const struct file_operations dwc3_stats_proc_ops = {
	.owner = THIS_MODULE,
	.open = dwc3_stats_proc_open,
	.read = seq_read,
	.release = single_release,
};

int dwc3_proc_init(struct dwc3 *dwc)
{
	struct proc_dir_entry *proc_entry;

	if (proc_dwc3_dir == NULL) {
		proc_entry = proc_mkdir(DWC3_PROC_ROOT, NULL);
		if (!proc_entry) {
			pr_err("%s: failed to create proc file %s\n",
					__func__, DWC3_PROC_ROOT);
			return 1;
		}
		proc_dwc3_dir = proc_entry;
	}
	proc_dwc3_dir_cnt++;

	proc_entry = proc_mkdir(to_platform_device(dwc->dev)->name, proc_dwc3_dir);
	if (!proc_entry) {
		pr_err("%s: failed to create proc file %s\n",
				__func__, to_platform_device(dwc->dev)->name);
		return -1;
	}
	dwc->parent_entry = proc_entry;

	proc_entry = proc_create_data(DWC3_PROC_CONNECTED_STATUS,
			0, dwc->parent_entry, &dwc3_stats_proc_ops, dwc);
	if (!proc_entry) {
		pr_err("%s: failed to create proc file %s\n",
				__func__, DWC3_PROC_CONNECTED_STATUS);
		return -1;
	}
	dwc->csts_entry = proc_entry;

	/*
	 * add here if more proc information need.
	 */
	return 0;
}

int dwc3_proc_shutdown(struct dwc3 *dwc)
{
	if (proc_dwc3_dir) {
		remove_proc_entry(DWC3_PROC_CONNECTED_STATUS, dwc->parent_entry);
		remove_proc_entry(to_platform_device(dwc->dev)->name, proc_dwc3_dir);
		remove_proc_entry(DWC3_PROC_ROOT, NULL);
	}

	if (proc_dwc3_dir_cnt)
		proc_dwc3_dir_cnt--;

	if (proc_dwc3_dir_cnt == 0)
		proc_dwc3_dir = NULL;

	return 0;
}
