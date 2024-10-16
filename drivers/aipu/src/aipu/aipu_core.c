// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/**
 * @file aipu_core.c
 * Implementation of the aipu_core create & destroy
 */

#include <linux/slab.h>
#include <linux/string.h>
#include "aipu_priv.h"
#include "config.h"

#if ((defined BUILD_ZHOUYI_V1) || (defined BUILD_ZHOUYI_COMPATIBLE))
#include "z1/z1.h"
extern struct aipu_core_operations zhouyi_v1_ops;
#endif
#if ((defined BUILD_ZHOUYI_V2) || (defined BUILD_ZHOUYI_COMPATIBLE))
#include "z2/z2.h"
extern struct aipu_core_operations zhouyi_v2_ops;
#endif

#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
#define MAX_CHAR_SYSFS 4096

static ssize_t aipu_core_ext_register_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int ret = 0;
	char tmp[1024];
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (unlikely(!core))
		return 0;

	if (core->ops->is_clk_gated(core) || !core->ops->is_power_on(core)) {
		return snprintf(buf, MAX_CHAR_SYSFS,
		    "AIPU is suspended and external registers cannot be read!\n");
	}

	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "   AIPU Core%d External Register Values\n", core->id);
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "%-*s%-*s%-*s\n", 8, "Offset", 22, "Name", 10, "Value");
	strcat(buf, tmp);
	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);
	ret += core->ops->sysfs_show(core, buf);
	ret += snprintf(tmp, 1024, "----------------------------------------\n");
	strcat(buf, tmp);

	return ret;
}

static ssize_t aipu_core_ext_register_sysfs_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	int i = 0;
	int ret = 0;
	char *token = NULL;
	char *buf_dup = NULL;
	int value[3] = { 0 };
	struct aipu_io_req io_req;
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if ((!core) || core->ops->is_clk_gated(core) || !core->ops->is_power_on(core))
		return 0;

	buf_dup = kzalloc(1024, GFP_KERNEL);
	if (!buf_dup)
		return -ENOMEM;
	snprintf(buf_dup, 1024, buf);

	dev_dbg(dev, "[SYSFS] user input str: %s", buf_dup);

	for (i = 0; i < 3; i++) {
		token = strsep(&buf_dup, "-");
		if (token == NULL) {
			dev_err(dev, "[SYSFS] please echo as this format: <reg_offset>-<write time>-<write value>");
			goto finish;
		}

		dev_dbg(dev, "[SYSFS] to convert str: %s", token);

		ret = kstrtouint(token, 0, &value[i]);
		if (ret) {
			dev_err(dev, "[SYSFS] convert str to int failed (%d): %s", ret, token);
			goto finish;
		}
	}

	dev_info(dev, "[SYSFS] offset 0x%x, time 0x%x, value 0x%x",
		value[0], value[1], value[2]);

	io_req.rw = AIPU_IO_WRITE;
	io_req.offset = value[0];
	io_req.value = value[2];
	for (i = 0; i < value[1]; i++) {
		dev_info(dev, "[SYSFS] writing register 0x%x with value 0x%x", value[0], value[2]);
		core->ops->io_rw(core, &io_req);
	}

finish:
	kfree(buf_dup);
	return count;
}

static ssize_t aipu_core_clock_gating_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (unlikely(!core))
		return 0;

	if (core->ops->is_clk_gated(core))
		return snprintf(buf, MAX_CHAR_SYSFS, "AIPU is in clock gating state and suspended.\n");
	else if (!core->ops->is_power_on(core))
		return snprintf(buf, MAX_CHAR_SYSFS, "AIPU is in power_off state.\n");
	else
		return snprintf(buf, MAX_CHAR_SYSFS, "AIPU is in normal working state.\n");
}

static ssize_t aipu_core_clock_gating_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int do_suspend = 0;
	int do_resume = 0;
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (unlikely(!core))
		return count;

	if ((strncmp(buf, "1", 1) == 0))
		do_suspend = 1;
	else if ((strncmp(buf, "0", 1) == 0))
		do_resume = 1;

	if ((!core->ops->is_clk_gated(core)) && core->ops->is_idle(core) && do_suspend) {
		dev_info(dev, "enable clock gating\n");
		core->ops->enable_clk_gating(core);
	} else if (core->ops->is_clk_gated(core) && do_resume) {
		dev_info(dev, "disable clock gating\n");
		core->ops->disable_clk_gating(core);
	} else {
		dev_err(dev, "operation cannot be completed!\n");
	}

	return count;
}

static ssize_t aipu_core_power_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return aipu_core_clock_gating_sysfs_show(dev, attr, buf);
}

static ssize_t aipu_core_power_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int do_power_off = 0;
	int do_power_on = 0;
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (unlikely(!core))
		return count;

	if ((strncmp(buf, "1", 1) == 0))
		do_power_on = 1;
	else if ((strncmp(buf, "0", 1) == 0))
		do_power_off = 1;

	if (core->ops->is_power_on(core) && core->ops->is_idle(core) && do_power_off) {
		dev_info(dev, "power off...\n");
		core->ops->power_off(core);
	} else if (!core->ops->is_power_on(core) && do_power_on) {
		dev_info(dev, "power on...\n");
		core->ops->power_on(core);
	} else {
		dev_info(dev, "operation cannot be completed\n");
	}

	return count;
}

static ssize_t aipu_core_disable_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if (atomic_read(&core->disable)) {
		return snprintf(buf, MAX_CHAR_SYSFS,
		    "AIPU core #%d is disabled (echo 0 > /sys/devices/platform/[dev]/disable to enable it).\n",
		    core->id);
	} else {
		return snprintf(buf, MAX_CHAR_SYSFS,
		    "AIPU core #%d is enabled (echo 1 > /sys/devices/platform/[dev]/disable to disable it).\n",
		    core->id);
	}
}

static ssize_t aipu_core_disable_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int do_disable = 0;
	struct platform_device *p_dev = container_of(dev, struct platform_device, dev);
	struct aipu_core *core = platform_get_drvdata(p_dev);

	if ((strncmp(buf, "1", 1) == 0))
		do_disable = 1;
	else if ((strncmp(buf, "0", 1) == 0))
		do_disable = 0;
	else
		do_disable = -1;

	if (atomic_read(&core->disable) && !do_disable) {
		dev_info(dev, "enable core...\n");
		atomic_set(&core->disable, 0);
	} else if (!atomic_read(&core->disable) && do_disable) {
		dev_info(dev, "disable core...\n");
		atomic_set(&core->disable, 1);
	}

	return count;
}

typedef ssize_t (*sysfs_show_t)(struct device *dev, struct device_attribute *attr, char *buf);
typedef ssize_t (*sysfs_store_t)(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static struct device_attribute *aipu_core_create_attr(struct device *dev, struct device_attribute **attr, const char *name,
	int mode, sysfs_show_t show, sysfs_store_t store)
{
	if ((!dev) || (!attr) || (!name))
		return ERR_PTR(-EINVAL);

	*attr = kzalloc(sizeof(*attr), GFP_KERNEL);
	if (!*attr)
		return ERR_PTR(-ENOMEM);

	(*attr)->attr.name = name;
	(*attr)->attr.mode = mode;
	(*attr)->show = show;
	(*attr)->store = store;
	device_create_file(dev, *attr);

	return *attr;
}

static void aipu_core_destroy_attr(struct device *dev, struct device_attribute **attr)
{
	if ((!dev) || (!attr) || (!*attr))
		return;

	device_remove_file(dev, *attr);
	kfree(*attr);
	*attr = NULL;
}
#endif

int init_aipu_core(struct aipu_core *core, int version, int id, void *job_manager,
	struct platform_device *p_dev)
{
	int ret = 0;
	int reg_iter = 0;
	struct resource *res = NULL;

	if ((!core) || (!p_dev) || !(job_manager))
		return -EINVAL;

	BUG_ON(core->is_init);
	BUG_ON((version != AIPU_ISA_VERSION_ZHOUYI_V1) && (version != AIPU_ISA_VERSION_ZHOUYI_V2));

	core->version = version;
	core->id = id;
	core->dev = &p_dev->dev;
	core->reg_cnt = 0;
	core->job_manager = job_manager;
	atomic_set(&core->disable, 0);
	snprintf(core->core_name, sizeof(core->core_name), "aipu%d", id);

#if ((defined BUILD_ZHOUYI_V1) || (defined BUILD_ZHOUYI_COMPATIBLE))
	if (version == AIPU_ISA_VERSION_ZHOUYI_V1) {
		core->max_sched_num = ZHOUYI_V1_MAX_SCHED_JOB_NUM;
		core->ops = &zhouyi_v1_ops;
	}
#endif
#if ((defined BUILD_ZHOUYI_V2) || (defined BUILD_ZHOUYI_COMPATIBLE))
	if (version == AIPU_ISA_VERSION_ZHOUYI_V2) {
		core->max_sched_num = ZHOUYI_V2_MAX_SCHED_JOB_NUM;
		core->ops = &zhouyi_v2_ops;
	}
#endif

	/* init AIPU core IO region(s) */
	for (reg_iter = 0; reg_iter < MAX_IO_REGION_CNT; reg_iter++) {
		u64 base = 0;
		u64 size = 0;

		res = platform_get_resource(p_dev, IORESOURCE_MEM, reg_iter);
		if (!res) {
			/* At least 1 IO region shall be provided in DTS for a single core */
			if (reg_iter > 0)
				break;
			dev_err(core->dev, "get aipu core #%d IO region failed\n", id);
			ret = -EINVAL;
			goto init_reg_fail;
		}
		base = res->start;
		size = res->end - res->start + 1;
		dev_dbg(core->dev, "get aipu core #%d IO region: [0x%llx, 0x%llx]\n",
			id, base, res->end);

		ret = init_aipu_ioregion(core->dev, &core->reg[reg_iter], base, size);
		if (ret) {
			dev_err(core->dev, "create aipu core #%d IO region failed: base 0x%llx, size 0x%llx\n",
				id, base, size);
			goto init_reg_fail;
		}
		core->reg_cnt++;
		dev_dbg(core->dev, "init aipu core #%d IO region done: [0x%llx, 0x%llx]\n",
		    id, base, res->end);
	}

	/* init AIPU core interrupt */
	res = platform_get_resource(p_dev, IORESOURCE_IRQ, 0);
	if (!res) {
		dev_err(core->dev, "get aipu core #%d IRQ number failed\n", id);
		ret = -EINVAL;
		goto init_irq_fail;
	}
	dev_dbg(core->dev, "get aipu core #%d IRQ number: 0x%x\n", id, (int)res->start);

	core->irq_obj = aipu_create_irq_object(res->start, core, core->core_name);
	if (!core->irq_obj) {
		dev_err(core->dev, "create IRQ object for core0 failed: IRQ 0x%x\n", (int)res->start);
		ret = -EFAULT;
		goto init_irq_fail;
	}
	dev_dbg(core->dev, "init aipu core #%d IRQ done\n", id);

#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
	/* external register */
	if (IS_ERR(aipu_core_create_attr(core->dev, &core->reg_attr, "ext_registers", 0644,
	    aipu_core_ext_register_sysfs_show, aipu_core_ext_register_sysfs_store))) {
		ret = -EFAULT;
		goto init_sysfs_fail;
	}

	/* clock gating */
	if (core->ops->has_clk_ctrl(core) &&
	    IS_ERR(aipu_core_create_attr(core->dev, &core->clk_attr, "core_clock_gating", 0644,
	    aipu_core_clock_gating_sysfs_show, aipu_core_clock_gating_sysfs_store))) {
		ret = -EFAULT;
		goto init_sysfs_fail;
	}

	/* power control */
	if (core->ops->has_power_ctrl(core) &&
	    IS_ERR(aipu_core_create_attr(core->dev, &core->pw_attr, "core_power", 0644,
		aipu_core_power_sysfs_show, aipu_core_power_sysfs_store))) {
		ret = -EFAULT;
		goto init_sysfs_fail;
	}

	/* disable */
	if (IS_ERR(aipu_core_create_attr(core->dev, &core->disable_attr, "disable", 0644,
		aipu_core_disable_sysfs_show, aipu_core_disable_sysfs_store))) {
		ret = -EFAULT;
		goto init_sysfs_fail;
	}
#else
	core->reg_attr = NULL;
	core->clk_attr = NULL;
	core->pw_attr = NULL;
	core->disable_attr = NULL;
#endif

	core->arch = AIPU_ARCH_ZHOUYI;
	core->config = core->ops->get_config(core);
	core->ops->enable_interrupt(core);
	core->ops->print_hw_id_info(core);

	/* success */
	core->is_init = 1;
	goto finish;

#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
init_sysfs_fail:
	aipu_core_destroy_attr(core->dev, &core->reg_attr);
	aipu_core_destroy_attr(core->dev, &core->clk_attr);
	aipu_core_destroy_attr(core->dev, &core->pw_attr);
	aipu_core_destroy_attr(core->dev, &core->disable_attr);
#endif
init_irq_fail:
init_reg_fail:
	for (reg_iter = 0; reg_iter < core->reg_cnt; reg_iter++) {
		deinit_aipu_ioregion(&core->reg[reg_iter]);
		core->reg_cnt--;
	}

finish:
	return ret;
}

void deinit_aipu_core(struct aipu_core *core)
{
	int reg_iter = 0;

	if (!core)
		return;

	core->ops->disable_interrupt(core);

	for (reg_iter = 0; reg_iter < core->reg_cnt; reg_iter++)
		deinit_aipu_ioregion(&core->reg[reg_iter]);
	core->reg_cnt = 0;

	if (core->irq_obj) {
		aipu_destroy_irq_object(core->irq_obj);
		core->irq_obj = NULL;
	}

#if (defined AIPU_ENABLE_SYSFS) && (AIPU_ENABLE_SYSFS == 1)
	aipu_core_destroy_attr(core->dev, &core->reg_attr);
	aipu_core_destroy_attr(core->dev, &core->clk_attr);
	aipu_core_destroy_attr(core->dev, &core->pw_attr);
	aipu_core_destroy_attr(core->dev, &core->disable_attr);
#endif

	core->is_init = 0;
}
