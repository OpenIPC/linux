// SPDX-License-Identifier: GPL-2.0
/* Copyright (c) 2018-2021 Arm Technology (China) Co., Ltd. All rights reserved. */

/*
 * @file aipu_priv.c
 * Implementation of the aipu private struct init & deinit
 */

#include <linux/slab.h>
#include <linux/of_address.h>
#include "aipu_priv.h"
#include "aipu_mm.h"
#include "aipu_job_manager.h"
#include "config.h"

static int init_misc_dev(struct aipu_priv *aipu)
{
	/* init misc */
	aipu->misc.minor = MISC_DYNAMIC_MINOR;
	aipu->misc.name = "aipu";
	aipu->misc.fops = aipu->aipu_fops;
	aipu->misc.mode = 0666;
	return misc_register(&aipu->misc);
}

static void deinit_misc_dev(struct aipu_priv *aipu)
{
	if (aipu && aipu->misc.fops) {
		misc_deregister(&aipu->misc);
		memset(&aipu->misc, 0, sizeof(aipu->misc));
	}
}

int init_aipu_priv(struct aipu_priv *aipu, struct platform_device *p_dev,
	const struct file_operations *fops)
{
	int ret = 0;
	u64 int_reg[2];

	if ((!aipu) || (!p_dev) || (!fops))
		return -EINVAL;

	aipu->version = 0;
	aipu->core_cnt = 0;
	aipu->cores = NULL;
	aipu->core0_dev = &p_dev->dev;
	aipu->aipu_fops = fops;

	if (!of_property_read_u64_array(aipu->core0_dev->of_node, "interrupts-reg", int_reg, 2)) {
		if (init_aipu_ioregion(aipu->core0_dev, &aipu->interrupts, int_reg[0], int_reg[1]))
			return -EINVAL;
	} else
		dev_info(aipu->core0_dev, "no interrupts-reg specified\n");

	ret = init_misc_dev(aipu);
	if (ret)
		goto err_handle;

	ret = init_aipu_job_manager(&aipu->job_manager);
	if (ret)
		goto err_handle;

	ret = aipu_init_mm(&aipu->mm, p_dev);
	if (ret)
		goto err_handle;

	/* success */
	aipu->is_init = 1;
	goto finish;

err_handle:
	deinit_aipu_priv(aipu);

finish:
	return ret;
}

int aipu_priv_add_core(struct aipu_priv *aipu, struct aipu_core *core,
	int version, int id, struct platform_device *p_dev)
{
	int ret = 0;
	struct aipu_core **new_core_arr = NULL;

	if ((!aipu) || (!core) || (!p_dev))
		return -EINVAL;

	BUG_ON(!aipu->is_init);

	if (aipu->interrupts.kern)
		core->interrupts = &aipu->interrupts;
	else
		core->interrupts = NULL;

	ret = init_aipu_core(core, version, id, &aipu->job_manager, p_dev);
	if (ret)
		return ret;

	/* register new AIPU core */
	new_core_arr = kcalloc(aipu->core_cnt + 1, sizeof(*new_core_arr), GFP_KERNEL);
	if (!new_core_arr)
		return -ENOMEM;

	if (aipu->core_cnt) {
		BUG_ON(!aipu->cores);
		memcpy(new_core_arr, aipu->cores, aipu->core_cnt * sizeof(*new_core_arr));
		kfree(aipu->cores);
		aipu->cores = NULL;
	}

	new_core_arr[aipu->core_cnt] = core;
	aipu->cores = new_core_arr;
	aipu->core_cnt++;

	aipu_job_manager_set_cores_info(&aipu->job_manager, aipu->core_cnt, aipu->cores);
	return ret;
}

int aipu_priv_get_version(struct aipu_priv *aipu)
{
	if (likely(aipu))
		return aipu->version;
	return 0;
}

int aipu_priv_get_core_cnt(struct aipu_priv *aipu)
{
	if (likely(aipu))
		return aipu->core_cnt;
	return 0;
}

int deinit_aipu_priv(struct aipu_priv *aipu)
{
	int core_iter = 0;

	if (!aipu)
		return 0;

	for (core_iter = 0; core_iter < aipu->core_cnt; core_iter++)
		deinit_aipu_core(aipu->cores[core_iter]);

	kfree(aipu->cores);
	aipu->core_cnt = 0;

	if (aipu->interrupts.kern)
		deinit_aipu_ioregion(&aipu->interrupts);

	aipu_deinit_mm(&aipu->mm);
	deinit_aipu_job_manager(&aipu->job_manager);
	deinit_misc_dev(aipu);
	aipu->is_init = 0;

	return 0;
}

int aipu_priv_query_core_capability(struct aipu_priv *aipu, struct aipu_core_cap *cap)
{
	int id = 0;
	struct aipu_core *core = NULL;

	if (unlikely((!aipu) && (!cap)))
		return -EINVAL;

	for (id = 0; id < aipu->core_cnt; id++) {
		core = aipu->cores[id];
		cap[id].core_id = id;
		cap[id].arch = core->arch;
		cap[id].version = core->version;
		cap[id].config = core->config;
		cap[id].info.reg_base = core->reg[0].phys;
	}

	return 0;
}

int aipu_priv_query_capability(struct aipu_priv *aipu, struct aipu_cap *cap)
{
	int id = 0;
	struct aipu_core_cap *core_cap = NULL;

	if (unlikely((!aipu) && (!cap)))
		return -EINVAL;

	cap->core_cnt = aipu->core_cnt;
	cap->host_to_aipu_offset = aipu->mm.host_aipu_offset;
	cap->is_homogeneous = 1;

	core_cap = kcalloc(aipu->core_cnt, sizeof(*core_cap), GFP_KERNEL);
	if (!core_cap)
		return -ENOMEM;

	aipu_priv_query_core_capability(aipu, core_cap);
	for (id = 1; id < aipu->core_cnt; id++) {
		if (((core_cap[id].arch != core_cap[id - 1].arch) ||
		     (core_cap[id].version != core_cap[id - 1].version) ||
		     (core_cap[id].config != core_cap[id - 1].config))) {
			cap->is_homogeneous = 0;
			break;
		}
	}

	if (cap->is_homogeneous)
		cap->core_cap = core_cap[0];

	kfree(core_cap);
	return 0;
}

int aipu_priv_io_rw(struct aipu_priv *aipu, struct aipu_io_req *io_req)
{
	int ret = -EINVAL;
	int id = 0;

	if ((!aipu) || (!io_req) || (io_req->core_id >= aipu->core_cnt))
		return ret;

	id = io_req->core_id;
	return aipu->cores[id]->ops->io_rw(aipu->cores[id], io_req);
}

int aipu_priv_check_status(struct aipu_priv *aipu)
{
	if (aipu && aipu->is_init)
		return 0;
	return -EINVAL;
}
