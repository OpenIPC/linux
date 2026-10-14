// SPDX-License-Identifier: GPL-2.0

#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

#include <mach/platform.h>
#include <linux/lotus/timestamp.h>

/* stopwatch start */
#define STOPWATCH_BUF_SIZE 500


u32 *reg_ctrl1;
u32 *reg_ctrl2;
u32 *reg_result;
u32 *reg_result_h;
u32 *running_result;
u32 *running_result_h;
static struct proc_dir_entry *stopwatch_entry;
/* stopwatch end */

spinlock_t stopwatch_lock;

static int stopwatch_init(void);

/* stopwatch */
static ssize_t stopwatch_write(struct file *file, const char __user *ubuf, size_t count,  loff_t *ppos)
{
	int ret;
	unsigned long in;
	char *buf;

	if (*ppos > 0 || !ubuf || !count)
		return -EINVAL;

	buf = memdup_user_nul(ubuf, count);
	if (IS_ERR(buf))
		return PTR_ERR(buf);

	ret = kstrtoul(buf, 10, &in);
	if (ret < 0)
		goto out;

	switch (in) {
	case 1:
		ret = stopwatch_trigger();
		break;
	case 2:
		ret = stopwatch_clear();
		break;
#ifdef CONFIG_LOTUS_TIMESTAMP_TEST
	case 9:
		timestamp_test();
		break;
#endif
	default:
		ret = -EINVAL;
	}

out:
	kfree(buf);
	if (ret < 0)
		return ret;

	*ppos += count;
	return count;

}

static ssize_t stopwatch_read(struct file *file, char __user *ubuf, size_t count, loff_t *ppos)
{
	int i, len;
	u64 record[RECORD_NUM] = {0};
	char buf[STOPWATCH_BUF_SIZE] = {0};
	u64 current_record[2] = {0};
	unsigned long flags;

	if (*ppos > 0 || count < STOPWATCH_BUF_SIZE)
		return 0;

	if (reg_result == NULL)
		return -EBUSY;

	spin_lock_irqsave(&stopwatch_lock, flags);
	for (i = 0; i < RECORD_NUM; i++) {
		writel((i & (BIT(SELECT_BIT_NUM) - 1)) << SELECT_BIT, reg_ctrl1);
		record[i] = readl(reg_result);
		record[i] += (u64)readl(reg_result_h) << 32;
	}
	spin_unlock_irqrestore(&stopwatch_lock, flags);

	len = 0;
	memset(buf, 0, sizeof(buf));
	len += snprintf(buf, sizeof(buf) - len, "--------------------------\n");
	len += snprintf(buf + len, sizeof(buf) - len, "STOPWATCH[0-%d]:\n", RECORD_NUM - 1);
	for (i = 0; i < RECORD_NUM; i++) {
		len += snprintf(buf + len, sizeof(buf) - len, "%-3d  %-16lluus\n", i, record[i]);
	}

	spin_lock_irqsave(&stopwatch_lock, flags);
	writel(1 << RUNNING_TRIGGER_BIT, reg_ctrl2);
	udelay(4);
	current_record[0] = readl(running_result);
	current_record[1] = (u64)readl(running_result_h);
	spin_unlock_irqrestore(&stopwatch_lock, flags);
	len += snprintf(buf + len, sizeof(buf) - len, "CURRENT TIME: %lluus\n", current_record[0] + current_record[1]);
	len += snprintf(buf + len, sizeof(buf) - len, "--------------------------\n");

	if (copy_to_user(ubuf, buf, len))
		return -EFAULT;

	*ppos = len;

	return len;
}

static struct proc_ops stopwatch_ops = {
	.proc_read  = stopwatch_read,
	.proc_write = stopwatch_write,
};


static int stopwatch_init(void)
{
	reg_ctrl1 = ioremap(MISC_CTRL62, PAGE_SIZE);
	if (reg_ctrl1 == NULL) {
		pr_err("Fail to remap stopwatch reg\n");
		return -1;
	}
	reg_result   = reg_ctrl1 + 1;
	reg_result_h = reg_ctrl1 + 2;

	running_result = reg_ctrl1 + 3;
	running_result_h = reg_ctrl1 + 4;

	reg_ctrl2 = reg_ctrl1 + 5;

	spin_lock_init(&stopwatch_lock);

	stopwatch_entry = proc_create("stopwatch", 0644, NULL, &stopwatch_ops);
	if (stopwatch_entry == NULL) {
		pr_err("Fail to create stopwatch proc file\n");
		return -1;
	}

	return 0;
}


u64 stopwatch_get_running_timer(void)
{
	u64 value = 0;
	unsigned long flags;

	spin_lock_irqsave(&stopwatch_lock, flags);
	writel(1 << RUNNING_TRIGGER_BIT, reg_ctrl2);
	udelay(4);
	value = readl(running_result);
	value += (u64)readl(running_result_h) << 32;
	spin_unlock_irqrestore(&stopwatch_lock, flags);

	return value;
}

int stopwatch_trigger(void)
{
	u32 record, v;
	unsigned long flags;

	if (reg_ctrl1 == NULL || reg_ctrl2 == NULL || reg_result == NULL)
		return -EBUSY;

	spin_lock_irqsave(&stopwatch_lock, flags);

	v = readl(reg_ctrl1);
	v &= ~(BIT(SELECT_BIT_NUM) - 1) << SELECT_BIT;
	v |= ((RECORD_NUM - 1) & (BIT(SELECT_BIT_NUM) - 1)) << SELECT_BIT;
	writel(v, reg_ctrl1);
	record = readl(reg_result);
	if (record > 0) {
		pr_err_once("No more slot\n");
		goto err;
	}

	v = readl(reg_ctrl2);
	v |= 1 << TRIGGER_BIT;
	writel(v, reg_ctrl2);

	spin_unlock_irqrestore(&stopwatch_lock, flags);

	return 0;
err:
	spin_unlock_irqrestore(&stopwatch_lock, flags);
	return -ENOMEM;
}
EXPORT_SYMBOL(stopwatch_trigger);

int stopwatch_clear(void)
{
	unsigned long flags;
	u32 v;

	if (reg_ctrl2 == NULL)
		return -EBUSY;

	spin_lock_irqsave(&stopwatch_lock, flags);
	v = readl(reg_ctrl2);
	v |= 1 << CLEAR_BIT;
	writel(v, reg_ctrl2);
	spin_unlock_irqrestore(&stopwatch_lock, flags);

	return 0;
}
EXPORT_SYMBOL(stopwatch_clear);

void stopwatch_print(void)
{
	int i, len;
	u64 record[RECORD_NUM] = {0};
	char buf[STOPWATCH_BUF_SIZE] = {0};
	unsigned long flags;

	if (reg_result == NULL)
		return;

	spin_lock_irqsave(&stopwatch_lock, flags);
	for (i = 0; i < RECORD_NUM; i++) {
		writel((i & (BIT(SELECT_BIT_NUM) - 1)) << SELECT_BIT, reg_ctrl1);
		record[i] = readl(reg_result);
		record[i] += (u64)readl(reg_result_h) << 32;
	}
	spin_unlock_irqrestore(&stopwatch_lock, flags);

	len = 0;
	memset(buf, 0, sizeof(buf));
	len += snprintf(buf, sizeof(buf) - len, "--------------------------\n");
	len += snprintf(buf + len, sizeof(buf) - len, "STOPWATCH[0-%d]:\n", RECORD_NUM - 1);
	for (i = 0; i < RECORD_NUM; i++) {
		len += snprintf(buf + len, sizeof(buf) - len, "%-3d  %-16lluus\n", i, record[i]);
	}

	len += snprintf(buf + len, sizeof(buf) - len, "--------------------------\n");
	pr_info("%s\n", buf);

}
EXPORT_SYMBOL(stopwatch_print);

static void timestamps_exit(void)
{
	proc_remove(stopwatch_entry);
}


static int timestamps_init(void)
{
	int ret = -1;

	/* stopwatch初始化 */
	ret = stopwatch_init();
	if (ret != 0) {
		pr_err("stopwatch init failed\n");
		return ret;
	}
	return 0;
}

module_init(timestamps_init);
module_exit(timestamps_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Lotus");

