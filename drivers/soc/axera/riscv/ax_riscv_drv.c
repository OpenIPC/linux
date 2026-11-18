/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "ax_riscv_drv.h"
#include "ax_riscv_debug.h"
#include "ax_riscv_utils.h"
#include "osal_ax.h"
#include <asm/io.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/of_platform.h>

#define DRIVER_NAME    "ax_riscv"

#define COMMON_GLB_REG_BASE         0x2340000
#define AX_DUMMY_SW4_OFFSET         0x200
#define RISCV_RESET_STATUS          0x2dc
#define SW4_ENTRY_LOWPOWER_MAGIC    0x6c706d64
#define SW4_WAKEUP_FROM_WFI_MAGIC   0x77616b65
#define SYSIO_PAD_LPMD_B_DEEP_SLEEP 0x00
#define CLK_EB_1_SET_OFFSET         0x34
#define CLK_EB_1_CLR_OFFSET         0x38

static struct mutex riscv_mutex;
static int riscv_alive;
static int riscv_active;

static int riscv_clk_enable(void)
{
    u32 *comm_glb_reg = ioremap(COMMON_GLB_REG_BASE, 0x500);
    if (comm_glb_reg == NULL) {
        pr_err("ioremap comm_glb_reg fail\n");
        return -1;
    }
    *(comm_glb_reg + (CLK_EB_1_SET_OFFSET / 4)) = 0x0100;
    iounmap(comm_glb_reg);

    return 0;
}

static int riscv_clk_disable(void)
{
    u32 *comm_glb_reg = ioremap(COMMON_GLB_REG_BASE, 0x500);
    if (comm_glb_reg == NULL) {
        pr_err("ioremap comm_glb_reg fail\n");
        return -1;
    }
    *(comm_glb_reg + (CLK_EB_1_CLR_OFFSET / 4)) = 0x0100;
    iounmap(comm_glb_reg);

    return 0;
}

static int riscv_suspend(void)
{
    int ret;
    u32 *comm_glb_reg;
    u32 reg;
    u32 lpmd;
    u64 start, end;

    comm_glb_reg = ioremap(COMMON_GLB_REG_BASE, 0x500);
    if (comm_glb_reg == NULL) {
        pr_err("ioremap comm_glb_reg fail\n");
        return -1;
    }

    mutex_lock(&riscv_mutex);

    if (riscv_active == 0 || riscv_alive == 0) {
        iounmap(comm_glb_reg);
        mutex_unlock(&riscv_mutex);
        return 0;
    }

    ret = ax_riscv_utils_sw_int_trigger(RISCV_SW_INT_GROUP(3), RISCV_SW_INT_CHANNEL(28));
    if (ret != 0) {
        pr_err("low power trigger fail\n");
        iounmap(comm_glb_reg);
        mutex_unlock(&riscv_mutex);
        return -1;
    }

    start = ax_riscv_utils_get_microseconds();
    while (1) {
        reg = *(comm_glb_reg + (RISCV_RESET_STATUS / 4));
        lpmd = (reg & 0x30) > 4;
        if (lpmd == SYSIO_PAD_LPMD_B_DEEP_SLEEP) {
            break;
        }
        end = ax_riscv_utils_get_microseconds();
        if ((end - start) > 20000) {
            iounmap(comm_glb_reg);
            pr_err("riscv entry low power mode fail\n");
            mutex_unlock(&riscv_mutex);
            return -1;
        }
    }

    riscv_clk_disable();
    riscv_active = 0;
    mutex_unlock(&riscv_mutex);
    iounmap(comm_glb_reg);
    return 0;
}

static int riscv_resume(void)
{
    int ret;

    mutex_lock(&riscv_mutex);

    if (riscv_active != 0 || riscv_alive == 0) {
        mutex_unlock(&riscv_mutex);
        return 0;
    }

    riscv_clk_enable();
    ax_riscv_utils_interrupt_umask(INTERRUPT_VECTOR_SW_INT_GROUP_3);
    ret = ax_riscv_utils_sw_int_trigger(RISCV_SW_INT_GROUP(3), RISCV_SW_INT_CHANNEL(27));
    if (ret != 0) {
        pr_err("low power trigger fail\n");
        mutex_unlock(&riscv_mutex);
        return -1;
    }
    riscv_active = 1;

    mutex_unlock(&riscv_mutex);
    return 0;
}

int ax_riscv_release_mem(void)
{
    int ret;
    u64 addr, size;

    mutex_lock(&riscv_mutex);

    if (riscv_alive == 0) {
        mutex_unlock(&riscv_mutex);
        return 0;
    }

    if (riscv_active != 0) {
        pr_err("riscv is active, cannot release\n");
        mutex_unlock(&riscv_mutex);
        return -1;
    }

    ret = ax_riscv_utils_get_dts_reg(RISCV_DTS_NODE_RISCV, &addr, &size);
    if (ret != 0) {
        pr_err("get dts node %s fail\n", RISCV_DTS_NODE_RISCV);
        mutex_unlock(&riscv_mutex);
        return ret;
    }
    ret = ax_os_release_reserved_mem(addr, size, "riscv area");
    if (ret != 0) {
        pr_err("release riscv reserved memory fail\n");
        mutex_unlock(&riscv_mutex);
        return ret;
    }
    riscv_alive = 0;

    // ret = ax_riscv_utils_get_dts_reg(RISCV_DTS_NODE_RAMDISK_HEADER, &addr, &size);
    // if (ret != 0) {
    //     mutex_unlock(&riscv_mutex);
    //     return 0;
    // }
    // ret = ax_os_release_reserved_mem(addr, size, "ramdisk header area");
    // if (ret != 0) {
    //     pr_err("release ramdisk header reserved memory fail\n");
    //     mutex_unlock(&riscv_mutex);
    //     return ret;
    // }

    mutex_unlock(&riscv_mutex);
    return 0;
}
EXPORT_SYMBOL_GPL(ax_riscv_release_mem);

int ax_riscv_entry_sleep(void)
{
    return riscv_suspend();
}
EXPORT_SYMBOL_GPL(ax_riscv_entry_sleep);

static int riscv_probe(struct platform_device *pdev)
{
    pr_info("riscv enter probe\n");
    riscv_alive = 1;
    riscv_active = 1;
    ax_riscv_debug_init();
    return 0;
}

static int riscv_remove(struct platform_device *pdev)
{
    ax_riscv_debug_deinit();
    return 0;
}

static const struct of_device_id ax_riscv_of_match[] = {
    {.compatible = "axera,riscv"},
	{}
};

static int riscv_pm_suspend(struct device * dev)
{
    int ret;

    ret = riscv_suspend();
    if (ret != 0) {
        pr_err("riscv_suspend fail\n");
        return ret;
    }

    ret = ax_riscv_release_mem();
    if (ret != 0) {
        pr_err("ax_riscv_release_mem fail\n");
        return ret;
    }

    return 0;
}

static int riscv_pm_resume(struct device *dev)
{
    return riscv_resume();
}

static const struct dev_pm_ops riscv_pm_ops = {
    .suspend    = riscv_pm_suspend,
	.resume		= riscv_pm_resume,
};

static struct platform_driver ax_riscv_driver = {
    .probe = riscv_probe,
    .remove = riscv_remove,
	.driver = {
		.name = DRIVER_NAME,
		.of_match_table = ax_riscv_of_match,
		.pm = &riscv_pm_ops,
	},
};

static int __init riscv_driver_init(void)
{
    int ret;

    mutex_init(&riscv_mutex);
    riscv_alive = 0;
    riscv_active = 0;

    ret = platform_driver_register((void *)&ax_riscv_driver);
    if (ret != 0) {
        pr_err("register riscv fail\n");
        return ret;
    }

    return 0;
}

static void __exit riscv_driver_exit(void)
{
    return platform_driver_unregister((void *)&ax_riscv_driver);
}

module_init(riscv_driver_init);
module_exit(riscv_driver_exit);

MODULE_AUTHOR("axera");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");
MODULE_INFO(intree, "Y");
