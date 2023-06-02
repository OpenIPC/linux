/*
 * Copyright (c) 2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include "ufs_proc.h"
#include "ufshcd.h"

#define UFS_PARENT	"ufs"
#define UFS_STATS_PROC	"ufs_info"
#define MAX_MANUF_SIZE	9
#define MAX_PROD_SIZE	17
#define MAX_CID_SIZE	127

unsigned int slot_index = 0;
static struct proc_dir_entry *proc_ufs_dir;
struct ufs_hba *hba_list[UFS_SLOT_NUM] = {NULL};

struct device_info {
	char manufacturer_name[MAX_MANUF_SIZE];
	char product_name[MAX_PROD_SIZE];
	char serial_number[MAX_CID_SIZE];
};

static char *card_type[MAX_CARD_TYPE + 1] = {
	"UFS card",
	"unknown"
};

static char *ufs_get_card_type(unsigned int type)
{
	if (MAX_CARD_TYPE <= type)
		return card_type[MAX_CARD_TYPE];
	else
		return card_type[type];
}

static int ufs_get_card_info(struct ufs_hba *hba, struct device_info *card_info)
{
	int err;
	u8 desc_buf[QUERY_DESC_DEVICE_MAX_SIZE];
	u8 str_desc_buf[QUERY_DESC_STRING_MAX_SIZE];
	u8 imanufacturername, iproductname, iserialnumber;

	err = ufshcd_read_device_desc(hba, desc_buf, QUERY_DESC_DEVICE_MAX_SIZE);
	if (err)
		return err;

	imanufacturername = desc_buf[DEVICE_DESC_PARAM_MANF_NAME];
	iproductname = desc_buf[DEVICE_DESC_PARAM_PRDCT_NAME];
	iserialnumber = desc_buf[DEVICE_DESC_PARAM_SN];

	err = ufshcd_read_string_desc(hba, imanufacturername, str_desc_buf,
					QUERY_DESC_STRING_MAX_SIZE, ASCII_STD);
	if (err)
		return err;
	strlcpy(card_info->manufacturer_name, (str_desc_buf + 2), str_desc_buf[0] - 2);
	card_info->manufacturer_name[MAX_MANUF_SIZE - 1] = '\0';

	err = ufshcd_read_string_desc(hba, iproductname, str_desc_buf,
					QUERY_DESC_STRING_MAX_SIZE, ASCII_STD);
	if (err)
		return err;
	strlcpy(card_info->product_name, (str_desc_buf + 2), str_desc_buf[0] - 2);
	card_info->product_name[MAX_PROD_SIZE - 1] = '\0';

	err = ufshcd_read_string_desc(hba, iserialnumber, str_desc_buf,
					QUERY_DESC_STRING_MAX_SIZE, ASCII_STD);
	if (err)
		return err;
	strlcpy(card_info->serial_number, (str_desc_buf + 2), str_desc_buf[0] - 2);
	card_info->serial_number[MAX_CID_SIZE - 1] = '\0';

	return 0;
}

static void ufs_stats_seq_printout(struct seq_file *s)
{
	int err;
	int present = 0;
	bool status;
	unsigned int index_ufs;
	static struct ufs_hba *hba;
	struct device_info card_info;
	u32 pwrmode, gear, rate, lane;

	for (index_ufs = 0; index_ufs < UFS_SLOT_NUM; index_ufs++) {
		hba = hba_list[index_ufs];
		if (NULL == hba) {
			seq_printf(s, "UFS%d: invalid\n", index_ufs);
			continue;
		} else {
			seq_printf(s, "UFS%d: ", index_ufs);
		}

		if (gpio_is_valid(hba->cd_gpio))
			present = gpio_get_value(hba->cd_gpio) ?  D_NO_DETECT : D_DETECT;

		if (present) {
			seq_puts(s, "pluged");
		} else {
			seq_puts(s, "unplugged");
		}

		status = hba->latest_card_status;
		if (status) {
			seq_puts(s, "_connected\n");
		} else {
			seq_puts(s, "_disconnected\n");
			continue;
		}

 		seq_printf(s, "\tType: %s\n", ufs_get_card_type(0));

		pwrmode = hba->pwr_info.pwr_rx;
		gear = hba->pwr_info.gear_rx;
		rate = hba->pwr_info.hs_rate;
		lane = hba->pwr_info.lane_rx;
		seq_printf(s, "\tMode: %s Gear-%d Rate-%c Lanes-%d\n",
				 ((FAST_MODE == pwrmode) ? "Fast" :
				 ((FASTAUTO_MODE == pwrmode) ? "FastAuto" :
				 ((SLOW_MODE == pwrmode) ? "Slow" : "SlowAuto"))),
				 gear, (PA_HS_MODE_A == rate) ? 'A' : 'B', lane);

		err = ufs_get_card_info(hba, &card_info);
		if (err)
			continue;

		seq_printf(s, "\tManufacturer Name: %s\n", card_info.manufacturer_name);
		seq_printf(s, "\tProduct Name: %s\n", card_info.product_name);
		seq_printf(s, "\tSerial Number: %s\n", card_info.serial_number);
		seq_printf(s, "\tCard Error Count: %u\n", hba->error_count);
	}
}

/* proc interface setup */
static void *ufs_seq_start(struct seq_file *s, loff_t *pos)
{
	/* counter is used to tracking multi proc interfaces
	 *  We have only one interface so return zero
	 *  pointer to start the sequence.
	 */
	static unsigned long counter;

	if (*pos == 0)
		return &counter;

	*pos = 0;
	return NULL;
}

/* proc interface next */
static void *ufs_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >= UFS_SLOT_NUM)
		return NULL;

	return NULL;
}

/* define parameters where showed in proc file */
static int ufs_stats_seq_show(struct seq_file *s, void *v)
{
	ufs_stats_seq_printout(s);
	return 0;
}

/* proc interface stop */
static void ufs_seq_stop(struct seq_file *s, void *v)
{
}

/* proc interface operation */
static const struct seq_operations ufs_stats_seq_ops = {
	.start = ufs_seq_start,
	.next = ufs_seq_next,
	.stop = ufs_seq_stop,
	.show = ufs_stats_seq_show
};

/* proc file open*/
static int ufs_stats_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &ufs_stats_seq_ops);
};

/* proc file operation */
static const struct file_operations ufs_stats_proc_ops = {
	.owner = THIS_MODULE,
	.open = ufs_stats_proc_open,
	.read = seq_read,
	.release = seq_release
};

int ufs_proc_init(void)
{
	struct proc_dir_entry *proc_stats_entry;

	proc_ufs_dir = proc_mkdir(UFS_PARENT, NULL);
	if (!proc_ufs_dir) {
		pr_err("%s: failed to create proc file %s\n",
				__func__, UFS_PARENT);
		return 1;
	}

	proc_stats_entry = proc_create(UFS_STATS_PROC,
			0, proc_ufs_dir, &ufs_stats_proc_ops);
	if (!proc_stats_entry) {
		pr_err("%s: failed to create proc file %s\n",
				__func__, UFS_STATS_PROC);
		return 1;
	}

	return 0;
}

int ufs_proc_shutdown(void)
{
	if (proc_ufs_dir) {
		remove_proc_entry(UFS_STATS_PROC, proc_ufs_dir);
		remove_proc_entry(UFS_PARENT, NULL);
		proc_ufs_dir = NULL;
	}

	return 0;
}
