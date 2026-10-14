// SPDX-License-Identifier: GPL-2.0

#include <linux/module.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/export.h>
#include <linux/types.h>

void calc_bandwidth_util(int numerator, int denominator, int *integer, int *decimal)
{
	float v = (float)numerator * 100 / (float)denominator;

	int integer_part = (int)v;

	if (integer)
		*integer = integer_part;

	if (decimal)
		*decimal = (int)((v - integer_part) * 100);
}
