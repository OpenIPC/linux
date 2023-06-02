/**
    Novatek TRNG driver

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 as
    published by the Free Software Foundation.
*/

#include <linux/err.h>
#include <linux/kernel.h>
#include <linux/hw_random.h>
#include <linux/io.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/random.h>
#include <linux/clk.h>

#define TRNG_VERION "1.00.000"

#define RNG_CTRL         0x0
#define RNG_CASR         0x100
#define RNG_POST         0x200
#define RNG_SEED_UPDATE  0x400
#define RNG_TDES_DONE    0x10000

#define RNG_RAND_NUM     0x0C

#define to_nvt_rng(p)    container_of(p, struct nvt_rng, rng)

struct nvt_rng {
	void __iomem *base;
	struct hwrng rng;
	struct clk   *clk;
	struct clk   *ro_clk; 
};

static inline void nvt_rng_write_reg(struct nvt_rng *nvt_rng, int reg, u32 val)
{
	__raw_writel(val, nvt_rng->base + reg);
}

static inline u32 nvt_rng_read_reg(struct nvt_rng *nvt_rng, int reg)
{
	return __raw_readl(nvt_rng->base + reg);
}

static int nvt_rng_init(struct hwrng *rng)
{
	struct nvt_rng *hrng = to_nvt_rng(rng);
	u32 data;

	//printk("nvt_rng_init\r\n");
	/* Seed init */
	//get_random_bytes(&seed, sizeof(seed));

	// Set config
	data = nvt_rng_read_reg(hrng, RNG_CTRL);
	nvt_rng_write_reg(hrng, RNG_CTRL, data|RNG_CASR|RNG_POST);
	return 0;
}

static void nvt_rng_cleanup(struct hwrng *rng)
{
	struct nvt_rng *hrng = to_nvt_rng(rng);

	clk_disable(hrng->clk);
	//printk("nvt_rng_cleanup\r\n");
	//writel_relaxed(0, hrng->base + RNG_CTRL);
}

static int nvt_rng_read(struct hwrng *rng, void *buf, size_t max, bool wait)
{
	struct nvt_rng *hrng = to_nvt_rng(rng);
	u32 *data = buf;
	u32 count = max;
	u32 ret = 0;
	
	while(count >= 4) {
		*(data++) = nvt_rng_read_reg(hrng, RNG_RAND_NUM);
		count -= 4;
		ret +=4;
	}
	return ret;
}

static int nvt_rng_probe(struct platform_device *pdev)
{
	struct nvt_rng *rng;
	struct resource *res;
	int ret;

	//printk("[nvt_rng_probe]\r\n");
	rng = devm_kzalloc(&pdev->dev, sizeof(*rng), GFP_KERNEL);
	if (!rng)
		return -ENOMEM;

	platform_set_drvdata(pdev, rng);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	rng->base = devm_ioremap_resource(&pdev->dev, res);
	if (IS_ERR(rng->base))
		return PTR_ERR(rng->base);

	rng->clk = devm_clk_get(&pdev->dev, dev_name(&pdev->dev));

	if (IS_ERR(rng->clk))
		return -ENODEV;
	else {
		rng->ro_clk = clk_get(NULL, "f0680000.rngro");
		if (IS_ERR(rng->ro_clk))
			return -ENODEV;
		else 
			clk_prepare_enable(rng->ro_clk);

		clk_prepare_enable(rng->clk);
		//clk_set_rate(rng->clk, 40000000);
		//printk("Clock Rate: %d\r\n",clk_get_rate(rng->clk));
	}

	rng->rng.name = pdev->name;
	rng->rng.init = nvt_rng_init;
	rng->rng.cleanup = nvt_rng_cleanup;
	rng->rng.read = nvt_rng_read;

	ret = devm_hwrng_register(&pdev->dev, &rng->rng);
	if (ret) {
		dev_err(&pdev->dev, "failed to register hwrng\n");
		return ret;
	}

	dev_info(&pdev->dev, "Register %s successfully\n", __func__);

	return 0;
}



static const struct of_device_id nvt_rng_dt_ids[] = {
	{ .compatible = "nvt,nvt_rng" },
	{},
};
MODULE_DEVICE_TABLE(of, nvt_rng_dt_ids);

static struct platform_driver nvt_rng_driver = {
	.probe		= nvt_rng_probe,
	.driver		= {
		.name	= "nvt_rng",
		.of_match_table = of_match_ptr(nvt_rng_dt_ids),
	},
};


module_platform_driver(nvt_rng_driver);

MODULE_AUTHOR("Novatek");
MODULE_DESCRIPTION("NA51055 random number generator driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(TRNG_VERION);
