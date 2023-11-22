/*
 * mdrv_swtoe.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#if 0
#include <linux/module.h>
#include <linux/device.h>
#include <linux/major.h>
#include <linux/fs.h>
#include <linux/arm-smccc.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/semaphore.h>
#include <linux/atomic.h>
#include <asm/pgtable.h>
#include <asm/io.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#else
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/skbuff.h>
#include "drv_dualos.h"
#include "mdrv_swtoe.h"
#include "mdrv_swtoe_intl.h"
#endif

#define IPC_TEST_THREAD 0

#if IPC_TEST_THREAD
static int swtoe_ipc_test_thread(void *arg)
{
    int cnt = 0;
    int ret;

    while (1)
    {
        msleep(5000);
        {
            {
                char *p = (char *)0xc2e45000;
                printk("    %02x %02x %02x %02x %02x %02x %02x %02x\n", p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
            }
        }
        // printk("[%s][%d] ipc start\n", __FUNCTION__, __LINE__);
        ret = signal_rtos(0x11000000, cnt, cnt + 1, cnt + 2);
        // printk("[%s][%d] ipc end. %d\n", __FUNCTION__, __LINE__, ret);
        cnt++;
    }
    return 0;
}
#endif

static int sstar_swtoe_drv_probe(struct platform_device *pdev)
{
    drv_swtoe_cnx_rxq_create(0); // @FIXME : service id has to be refined
#if IPC_TEST_THREAD
    kthread_run(swtoe_ipc_test_thread, NULL, "ipc_test_thread");
#endif
    return 0;
}

static int sstar_swtoe_drv_remove(struct platform_device *pdev)
{
    platform_set_drvdata(pdev, NULL);
    return 0;
}

static int sstar_swtoe_drv_suspend(struct platform_device *dev, pm_message_t state)
{
    return 0;
}

static int sstar_swtoe_drv_resume(struct platform_device *dev)
{
    return 0;
}

#if defined(CONFIG_OF)
static struct of_device_id sstar_swtoe_of_device_ids[] = {
    {.compatible = "sstar-swtoe"},
    {},
};
#endif

static struct platform_driver Sstar_swtoe_driver = {.probe   = sstar_swtoe_drv_probe,
                                                    .remove  = sstar_swtoe_drv_remove,
                                                    .suspend = sstar_swtoe_drv_suspend,
                                                    .resume  = sstar_swtoe_drv_resume,

                                                    .driver = {
                                                        .name = "Sstar-swtoe",
#if defined(CONFIG_OF)
                                                        .of_match_table = sstar_swtoe_of_device_ids,
#endif
                                                        .owner = THIS_MODULE,
                                                    }};

static int __init sstar_swtoe_drv_init_module(void)
{
    int retval = 0;

    drv_swtoe_glue_init();

    retval = platform_driver_register(&Sstar_swtoe_driver);
    if (retval)
    {
        printk(KERN_INFO "Sstar_swtoe_driver register failed...\n");
        return retval;
    }

    return retval;
}

static void __exit sstar_swtoe_drv_exit_module(void)
{
    platform_driver_unregister(&Sstar_swtoe_driver);
}

module_init(sstar_swtoe_drv_init_module);
module_exit(sstar_swtoe_drv_exit_module);

MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SW TOE driver");
MODULE_LICENSE("GPL");
