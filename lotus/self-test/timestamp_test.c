// SPDX-License-Identifier: GPL-2.0

#include <linux/proc_fs.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <linux/hrtimer.h>
#include <linux/lotus/timestamp.h>


static struct task_struct *s_testd1;
static struct task_struct *s_testd2;
static struct task_struct *s_testd3;
static struct hrtimer s_test_hrtimer;

int timestamp_testd1(void *data)
{
	int ret = 0;

	while (1) {
		ret = stopwatch_trigger();
		if (ret == -ENOMEM)
			stopwatch_clear();

		msleep(1);
	}
}

int timestamp_testd2(void *data)
{
	int ret = 0;

	while (1) {
		msleep(1);
		ret = stopwatch_trigger();
		if (ret == -ENOMEM)
			stopwatch_clear();
	}
}

int timestamp_testd3(void *data)
{
	while (1) {
		stopwatch_print();
		msleep(5000);
	}
}

#define TEST_HRTIMER_MS 2

enum hrtimer_restart timestamp_test_hrtimer_fn(struct hrtimer *this)
{
	int ret = 0;

	stopwatch_trigger();
	if (ret == -ENOMEM)
		stopwatch_clear();

	hrtimer_forward_now(this, ms_to_ktime(TEST_HRTIMER_MS));
	return HRTIMER_RESTART;
}

void timestamp_test(void)
{
	int i;

	pr_info("Clear stopwatch\n");
	stopwatch_clear();
	mdelay(1);
	stopwatch_print();

	pr_info("Correction test\n");
	for (i = 0; i < 10; ++i) {
		stopwatch_trigger();
		mdelay(i);
	}
	pr_info("Correction checking:\n");
	stopwatch_print();

	pr_info("Multi thread and interrupt context test\n");
	s_testd1 = kthread_run(timestamp_testd1, NULL, "timestamp testd1");
	s_testd2 = kthread_run(timestamp_testd2, NULL, "timestamp testd2");
	s_testd3 = kthread_run(timestamp_testd3, NULL, "timestamp testd3");
	hrtimer_init(&s_test_hrtimer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	s_test_hrtimer.function = timestamp_test_hrtimer_fn;
	pr_info("hrtimer start...");
	hrtimer_start(&s_test_hrtimer, ms_to_ktime(TEST_HRTIMER_MS), HRTIMER_MODE_REL);
	pr_info("done\n");
}

