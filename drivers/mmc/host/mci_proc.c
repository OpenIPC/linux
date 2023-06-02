/*****************************************************************************
 *  This is the driver for the host mci SOC.
 *
 *  Copyright (C) Hisilicon. All rights reserved.
 *
 ******************************************************************************/

#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/device.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include "sdhci.h"
#include "mci_proc.h"

#define MCI_PARENT       "mci"
#define MCI_STATS_PROC   "mci_info"
#define MAX_CLOCK_SCALE	(4)
#define UNSTUFF_BITS(resp,start,size)                   \
	({                              \
	 const int __size = size;                \
	 const u32 __mask = (__size < 32 ? 1 << __size : 0) - 1; \
	 const int __off = 3 - ((start) / 32);           \
	 const int __shft = (start) & 31;            \
	 u32 __res;                      \
	 \
	 __res = resp[__off] >> __shft;              \
	 if (__size + __shft > 32)               \
	 __res |= resp[__off-1] << ((32 - __shft) % 32); \
	 __res & __mask;                     \
	 })

struct card_info {
	unsigned int type;
	unsigned int state;
	unsigned int sd_bus_speed;
	unsigned int raw_ssr[16];
};

unsigned int slot_index = 0;
struct mmc_host *mci_host[MCI_SLOT_NUM] = {NULL};
static struct proc_dir_entry *proc_mci_dir;

static char *card_type[MAX_CARD_TYPE + 1] = {
	"MMC card",
	"SD card",
	"SDIO card",
	"SD combo (IO+mem) card",
	"unknown"
};
static char *clock_unit[4] = {
	"Hz",
	"KHz",
	"MHz",
	"GHz"
};

static char *mci_get_card_type(unsigned int sd_type)
{
	if (MAX_CARD_TYPE <= sd_type)
		return card_type[MAX_CARD_TYPE];
	else
		return card_type[sd_type];
}

static unsigned int analyze_clock_scale(unsigned int clock,
		unsigned int *clock_val)
{
	unsigned int scale = 0;
	unsigned int tmp = clock;

	while (1) {
		tmp = tmp / 1000;
		if (0 < tmp) {
			*clock_val = tmp;
			scale++;
		} else {
			break;
		}
	}
	return scale;
}

static inline int mmc_in_uhs(struct mmc_host *host)
{
	return host->ios.timing >= MMC_TIMING_UHS_SDR12 &&
		host->ios.timing <= MMC_TIMING_UHS_DDR50;
};

static inline int mmc_in_hs(struct mmc_host *host)
{
	return host->ios.timing == MMC_TIMING_SD_HS ||
		host->ios.timing == MMC_TIMING_MMC_HS;
};

static inline int mmc_in_hs400(struct mmc_host *host)
{
	return host->ios.timing == MMC_TIMING_MMC_HS400;
};

static inline int mmc_in_hs200(struct mmc_host *host)
{
	return host->ios.timing == MMC_TIMING_MMC_HS200;
};

static inline int mmc_in_ddr52(struct mmc_host *host)
{
	return host->ios.timing == MMC_TIMING_MMC_DDR52;
};

static void mci_stats_seq_printout(struct seq_file *s)
{
	unsigned int index_mci;
	unsigned int clock;
	unsigned int clock_scale;
	unsigned int clock_value = 0;
	const char *type;
	static struct mmc_host *mmc;
	static struct mmc_card	*card;
	const char *uhs_bus_speed_mode = "";
	static const char *const uhs_speeds[] = {
		[UHS_SDR12_BUS_SPEED] = "SDR12 ",
		[UHS_SDR25_BUS_SPEED] = "SDR25 ",
		[UHS_SDR50_BUS_SPEED] = "SDR50 ",
		[UHS_SDR104_BUS_SPEED] = "SDR104 ",
		[UHS_DDR50_BUS_SPEED] = "DDR50 ",
	};
	unsigned int speed_class, grade_speed_uhs;
	struct card_info info = {0};
	unsigned int present;
	struct sdhci_host *host;

	for (index_mci = 0; index_mci < MCI_SLOT_NUM; index_mci++) {
		mmc = mci_host[index_mci];
		if (NULL == mmc) {
			seq_printf(s, "MCI%d invalid\n", index_mci);
			continue;
		} else {
			seq_printf(s, "MCI%d", index_mci);
		}

		present = mmc->ops->get_cd(mmc);
		if (present) {
			seq_puts(s, ": pluged");
		} else {
			seq_puts(s, ": unplugged");
		}

		card = mmc->card;
		if (NULL == card) {
			seq_puts(s, "_disconnected\n");
		} else {
			mmc_get_card(card);
			if (mmc_card_removed(card)) {
				mmc_put_card(card);
				seq_puts(s, "_disconnected\n");
				continue;
			}
			info.type = card->type;
			info.state = card->state;
			info.sd_bus_speed = card->sd_bus_speed;
			memcpy(info.raw_ssr, card->raw_ssr, 64);
			mmc_put_card(card);

			seq_puts(s, "_connected\n");

			seq_printf(s,
					"\tType: %s",
					mci_get_card_type(info.type)
				  );

			if (mmc_card_blockaddr(&info)) {
				if (mmc_card_ext_capacity(&info))
					type = "SDXC";
				else
					type = "SDHC";
				seq_printf(s, "(%s)\n", type);
			}

			if (mmc_in_uhs(mmc) &&
					(info.sd_bus_speed < ARRAY_SIZE(uhs_speeds)))
				uhs_bus_speed_mode = uhs_speeds[info.sd_bus_speed];
			else
				uhs_bus_speed_mode = "";

			seq_printf(s, "\tMode: %s%s%s%s\n",
					mmc_in_uhs(mmc) ? "UHS " :
					(mmc_in_hs(mmc) ? "HS " : ""),
					mmc_in_hs400(mmc) ? "HS400 " :
					(mmc_in_hs200(mmc) ? "HS200 " : ""),
					mmc_in_ddr52(mmc) ? "DDR " : "",
					uhs_bus_speed_mode);

			speed_class = UNSTUFF_BITS(info.raw_ssr, 440 - 384, 8);
			grade_speed_uhs = UNSTUFF_BITS(info.raw_ssr, 396 - 384, 4);
			seq_printf(s, "\tSpeed Class: Class %s\n",
					(0x00 == speed_class) ? "0":
					(0x01 == speed_class) ? "2":
					(0x02 == speed_class) ? "4":
					(0x03 == speed_class) ? "6":
					(0x04 == speed_class) ? "10":
					"Reserved");
			seq_printf(s, "\tUhs Speed Grade: %s\n",
					(0x00 == grade_speed_uhs)?
					"Less than 10MB/sec(0h)" :
					(0x01 == grade_speed_uhs)?
					"10MB/sec and above(1h)":
					"Reserved");

			clock = mmc->ios.clock;
			clock_scale = analyze_clock_scale(clock, &clock_value);
			seq_printf(s, "\tHost work clock: %d%s\n",
					clock_value, clock_unit[clock_scale]);

			clock = mmc->ios.clock;
			clock_scale = analyze_clock_scale(clock, &clock_value);
			seq_printf(s, "\tCard support clock: %d%s\n",
					clock_value, clock_unit[clock_scale]);
			clock = mmc->actual_clock;
			clock_scale = analyze_clock_scale(clock, &clock_value);
			seq_printf(s, "\tCard work clock: %d%s\n",
					clock_value, clock_unit[clock_scale]);

			host = mmc_priv(mmc);
			/* add card read/write error count */
			seq_printf(s, "\tCard error count: %d\n",
					        host->error_count);
		}
	}
}

/* proc interface setup */
static void *mci_seq_start(struct seq_file *s, loff_t *pos)
{
	/*   counter is used to tracking multi proc interfaces
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
static void *mci_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	if (*pos >= MCI_SLOT_NUM)
		return NULL;

	return NULL;
}

/* define parameters where showed in proc file */
static int mci_stats_seq_show(struct seq_file *s, void *v)
{
	mci_stats_seq_printout(s);
	return 0;
}

/* proc interface stop */
static void mci_seq_stop(struct seq_file *s, void *v)
{
}

/* proc interface operation */
static const struct seq_operations mci_stats_seq_ops = {
	.start = mci_seq_start,
	.next = mci_seq_next,
	.stop = mci_seq_stop,
	.show = mci_stats_seq_show
};

/* proc file open*/
static int mci_stats_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &mci_stats_seq_ops);
};

/* proc file operation */
static const struct file_operations mci_stats_proc_ops = {
	.owner = THIS_MODULE,
	.open = mci_stats_proc_open,
	.read = seq_read,
	.release = seq_release
};

int mci_proc_init(void)
{
	struct proc_dir_entry *proc_stats_entry;

	proc_mci_dir = proc_mkdir(MCI_PARENT, NULL);
	if (!proc_mci_dir) {
		pr_err("%s: failed to create proc file %s\n",
				__func__, MCI_PARENT);
		return 1;
	}

	proc_stats_entry = proc_create(MCI_STATS_PROC,
			0, proc_mci_dir, &mci_stats_proc_ops);
	if (!proc_stats_entry) {
		pr_err("%s: failed to create proc file %s\n",
				__func__, MCI_STATS_PROC);
		return 1;
	}

	return 0;
}

int mci_proc_shutdown(void)
{
	if (proc_mci_dir) {
		remove_proc_entry(MCI_STATS_PROC, proc_mci_dir);
		remove_proc_entry(MCI_PARENT, NULL);
		proc_mci_dir = NULL;
	}

	return 0;
}
