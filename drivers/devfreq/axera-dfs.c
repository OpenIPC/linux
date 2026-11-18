#include <linux/clk.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/devfreq.h>

typedef struct axera_dfs_resource {
	struct clk *clk;
} dfs_resource_s;

static struct axera_dfs_resource *axera_get_resource(struct device *dev)
{
	struct platform_device *pdev;
	struct axera_dfs_resource *resource;

	pdev = to_platform_device(dev);
	resource = platform_get_drvdata(pdev);
	if (IS_ERR(resource)) {
		return NULL;
	}

	return resource;
}

static int axera_freq_target(struct device *dev, unsigned long *freq, u32 flags)
{
	struct dev_pm_opp *opp;
	unsigned long rate;
	struct axera_dfs_resource *resource;

	resource = axera_get_resource(dev);

	if (!resource) {
		return -ENODATA;
	}

	opp = devfreq_recommended_opp(dev, freq, flags);
	if (IS_ERR(opp)) {
		return PTR_ERR(opp);
	}

	rate = dev_pm_opp_get_freq(opp);

	if (resource->clk) {
		clk_set_rate(resource->clk, rate);
	}

	dev_pm_opp_put(opp);

	return 0;
}

static int axera_get_cur_freq(struct device *dev, unsigned long *freq)
{
	unsigned long rate;
	struct axera_dfs_resource *resource;

	resource = axera_get_resource(dev);

	if (!resource) {
		return -ENODATA;
	}

	if (resource->clk) {
		rate = clk_get_rate(resource->clk);
	}

	*freq = rate;
	return 0;
}

static int axera_dfs_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct clk *clk = NULL;
	struct devfreq *fp;
	struct axera_dfs_resource *resource;
	struct devfreq_dev_profile *axera_devfreq_profile;

	axera_devfreq_profile = devm_kzalloc(&pdev->dev,
		sizeof(struct devfreq_dev_profile), GFP_KERNEL);

	if (IS_ERR(axera_devfreq_profile)) {
		pr_err("get devfreq profile failed.\n");
		goto DFS_ERR0;
	}

	axera_devfreq_profile->target = axera_freq_target;
	axera_devfreq_profile->get_cur_freq = axera_get_cur_freq;

	resource = devm_kzalloc(&pdev->dev,
		sizeof(struct axera_dfs_resource), GFP_KERNEL);

	if (IS_ERR(resource)) {
		pr_err("get resource failed.\n");
		goto DFS_ERR1;
	}

	clk = devm_clk_get(&pdev->dev, "bus_clk");
	if (IS_ERR(clk)) {
		pr_err("get clk failed.\n");
		goto DFS_ERR2;
	}

	if (!IS_ERR(clk))
		resource->clk = clk;

	platform_set_drvdata(pdev, resource);

	ret = dev_pm_opp_of_add_table(&pdev->dev);
	if (ret) {
		pr_err("cannot add opp table!\n");
		goto DFS_ERR3;
	}

	if (!IS_ERR(clk)) {
		axera_devfreq_profile->initial_freq = clk_get_rate(clk);
	}

	fp = devm_devfreq_add_device(&pdev->dev, axera_devfreq_profile, "userspace", NULL);
	if (IS_ERR(fp)) {
		pr_err("cannot add device to devfreq!\n");
		goto DFS_ERR4;
	}

	return 0;

DFS_ERR4:
	devm_kfree(&pdev->dev, axera_devfreq_profile);
	devm_kfree(&pdev->dev, resource);
	devm_clk_put(&pdev->dev, clk);
	dev_pm_opp_of_remove_table(&pdev->dev);
	return PTR_ERR(fp);

DFS_ERR3:
	devm_kfree(&pdev->dev, axera_devfreq_profile);
	devm_kfree(&pdev->dev, resource);
	devm_clk_put(&pdev->dev, clk);
	return ret;

DFS_ERR2:
	devm_kfree(&pdev->dev, axera_devfreq_profile);
	devm_kfree(&pdev->dev, resource);
	return PTR_ERR(clk);

DFS_ERR1:
	devm_kfree(&pdev->dev, axera_devfreq_profile);
	return PTR_ERR(resource);

DFS_ERR0:
	return PTR_ERR(axera_devfreq_profile);

}

static int axera_dfs_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id axera_dfs_matches[] = {
	{.compatible = "axera,aclk_cpu_top"},
	{.compatible = "axera,aclk_isp_top"},
	{.compatible = "axera,aclk_vpu_top"},
	{.compatible = "axera,aclk_ocm_top"},
	{.compatible = "axera,aclk_nn_top"},
	{.compatible = "axera,aclk_mm_top"},
	{.compatible = "axera,clk_isp_mm"},
	{.compatible = "axera,pclk_top"},
	{},
};

static struct platform_driver axera_dfs_driver = {
	.driver = {
		.name = "axera-dfs",
		.of_match_table = axera_dfs_matches,
	},
	.probe = axera_dfs_probe,
	.remove = axera_dfs_remove,
};

module_platform_driver(axera_dfs_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Axera");
MODULE_ALIAS("platform:axera-dfs");