#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/remoteproc.h>
#include <linux/interrupt.h>
#include <linux/delay.h>

#include <plat/remoteproc.h>
#include <plat/rct.h>

#include "../../remoteproc_internal.h"

static void rpmsg_send_irq(int irq)
{
	if (irq > 0) {
		amba_writel(AHB_SCRATCHPAD_REG(0x10), 0x1 << (irq - AXI_SOFT_IRQ(0)));
	}
	else {
		irq = -irq;
		amba_writel(VIC3_REG(0x18), 0x1 << (irq - 96));
	}
}

static void rpmsg_ack_irq(int irq)
{
	amba_writel(AHB_SCRATCHPAD_REG(0x14), 0x1 << (irq - AXI_SOFT_IRQ(0)));
}

static void ambarella_rproc_kick(struct rproc *rproc, int vqid)
{
	struct ambarella_rproc_pdata *pdata = rproc->priv;

	if (vqid == 0)
		rpmsg_send_irq(pdata->rvq_tx_irq);
	else
		rpmsg_send_irq(pdata->svq_tx_irq);
}

static void rproc_svq_worker(struct work_struct *work)
{
	struct ambarella_rproc_pdata *pdata;
	struct rproc *rproc;

	pdata = container_of(work, struct ambarella_rproc_pdata, svq_work);
	rproc = pdata->rproc;

	rproc_vq_interrupt(rproc, 1);
}

static void rproc_rvq_worker(struct work_struct *work)
{
	struct ambarella_rproc_pdata *pdata;
	struct rproc *rproc;
	struct rproc_vring *rvring;

	pdata = container_of(work, struct ambarella_rproc_pdata, rvq_work);
	rproc = pdata->rproc;
	rvring = idr_find(&pdata->rproc->notifyids, 0);

	while (1) {

		if (rproc_vq_interrupt(rproc, 0) == IRQ_HANDLED)
			continue;

		virtqueue_enable_cb(rvring->vq);

		if (rproc_vq_interrupt(rproc, 0) == IRQ_HANDLED) {

			virtqueue_disable_cb(rvring->vq);
			continue;
		}

		break;
	}
}

static irqreturn_t rproc_ambarella_isr(int irq, void *dev_id)
{
	struct ambarella_rproc_pdata *pdata = dev_id;
	struct rproc_vring *rvring;

	if (irq == pdata->rvq_rx_irq) {
		rvring = idr_find(&pdata->rproc->notifyids, 0);
		virtqueue_disable_cb(rvring->vq);
		schedule_work(&pdata->rvq_work);
	}
	else if (irq == pdata->svq_rx_irq) {
		schedule_work(&pdata->svq_work);
	}

	rpmsg_ack_irq(irq);

	return IRQ_HANDLED;
}

static int ambarella_rproc_start(struct rproc *rproc)
{
	return 0;
}

static int ambarella_rproc_stop(struct rproc *rproc)
{
	return 0;
}

static struct rproc_ops ambarella_rproc_ops = {
	.start		= ambarella_rproc_start,
	.stop		= ambarella_rproc_stop,
	.kick		= ambarella_rproc_kick,
};

extern const struct rproc_fw_ops rproc_dummy_fw_ops;

static int ambarella_rproc_probe(struct platform_device *pdev)
{
	struct ambarella_rproc_pdata *pdata = pdev->dev.platform_data;
	struct rproc *rproc;
	int ret;

	ret = dma_set_coherent_mask(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(pdev->dev.parent, "dma_set_coherent_mask: %d\n", ret);
		return ret;
	}

	rproc = rproc_alloc(&pdev->dev, pdata->name, &ambarella_rproc_ops,
			    pdata->firmware, 8192);
	if (!rproc) {
		ret = -ENOMEM;
		goto free_rproc;
	}

	ret = request_irq(pdata->svq_rx_irq, rproc_ambarella_isr, IRQF_SHARED,
			"rproc-svq_rx", pdata);
	if (ret)
		goto free_rproc;

	ret = request_irq(pdata->rvq_rx_irq, rproc_ambarella_isr, IRQF_SHARED,
			"rproc-rvq_rx", pdata);
	if (ret)
		goto free_rproc;

	INIT_WORK(&pdata->svq_work, rproc_svq_worker);
	INIT_WORK(&pdata->rvq_work, rproc_rvq_worker);

	mutex_init(&rproc->lock);

	rproc->priv = pdata;
	pdata->rproc = rproc;

	platform_set_drvdata(pdev, rproc);

	rproc->fw_ops = &rproc_dummy_fw_ops;
	ret = rproc_add(rproc);
	if (ret)
		goto free_rproc;

	return 0;

free_rproc:
	rproc_put(rproc);
	return ret;
}

static int ambarella_rproc_remove(struct platform_device *pdev)
{
	struct rproc *rproc = platform_get_drvdata(pdev);

	rproc_del(rproc);
	rproc_put(rproc);
	return 0;
}

static struct platform_device_id id_table[] = {
	{ "ca9_a_and_b", 0},
	{ "ca9_a_and_arm11", 0},
	{ "ca9_b_and_arm11", 0},
    { "c0_and_c1", 0},
	{ /* sentinel */ }
};

static struct platform_driver ambarella_rproc_driver = {
	.probe = ambarella_rproc_probe,
	.remove = ambarella_rproc_remove,
	.driver = {
		.name = "ambarella-rproc",
		.owner = THIS_MODULE,
	},
	.id_table = id_table,
};

static int ambarella_rproc_driver_init(void)
{
	platform_driver_register(&ambarella_rproc_driver);
	return 0;
}

static void ambarella_rproc_driver_fini(void)
{
	platform_driver_unregister(&ambarella_rproc_driver);
}

module_init(ambarella_rproc_driver_init);
module_exit(ambarella_rproc_driver_fini);

MODULE_DESCRIPTION("Ambarella Remote Processor control driver");
