#include <linux/of.h>
#include <linux/cpufreq.h>
#include <linux/cpu.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/platform_device.h>
#include <plat/top.h>
#define DRV_VERSION "1.0.0"

static const struct of_device_id machines[] __initconst = {
	{ .compatible = "nvt,ca9" },
	{ /* sentinel */ }
};

static struct cpufreq_frequency_table freq_table[] = {
	{ .frequency = 240000 },
	{ .frequency = 480000 },
	{ .frequency = 800000 },
	{ .frequency = 960000 },
	{ .frequency = CPUFREQ_TABLE_END },
};

struct nvt_cpufreq {
	struct device *dev;
	struct cpufreq_driver driver;
	struct clk *cpu_clk;
};

static int nvt_target(struct cpufreq_policy *policy, unsigned int index)
{
	struct nvt_cpufreq *cpufreq = cpufreq_get_driver_data();
	unsigned long rate = freq_table[index].frequency;

	if (nvt_get_chip_id() == CHIP_NA51055)
		return clk_set_rate(cpufreq->cpu_clk, rate * 1000);
	else
		return clk_set_rate(cpufreq->cpu_clk, rate * 1000 / 8);
}

static unsigned int nvt_cpufreq_get(unsigned int cpu)
{
	struct cpufreq_policy *policy = cpufreq_cpu_get_raw(cpu);

	if (!policy || IS_ERR(policy->clk)) {
		pr_err("%s: No %s associated to cpu: %d\n",
		       __func__, policy ? "clk" : "policy", cpu);
		return 0;
	}

	if (nvt_get_chip_id() == CHIP_NA51055)
		return clk_get_rate(policy->clk) / 1000;
	else
		return (clk_get_rate(policy->clk)*8) / 1000;
}

static int nvt_cpu_init(struct cpufreq_policy *policy)
{
	struct nvt_cpufreq *cpufreq = cpufreq_get_driver_data();
	int ret;

	if (!__clk_is_enabled(cpufreq->cpu_clk))
		clk_prepare_enable(cpufreq->cpu_clk);

	ret = cpufreq_generic_init(policy, freq_table, 300 * 1000);
	if (ret) {
		clk_disable_unprepare(cpufreq->cpu_clk);
		return ret;
	}

	policy->clk = cpufreq->cpu_clk;
	policy->suspend_freq = freq_table[0].frequency;
	return 0;
}

static int nvt_cpu_exit(struct cpufreq_policy *policy)
{
	struct nvt_cpufreq *cpufreq = cpufreq_get_driver_data();

	clk_disable_unprepare(cpufreq->cpu_clk);
	return 0;
}

static int __init nvt_cpufreq_init(void)
{
	struct nvt_cpufreq *cpufreq;
	struct device *cpu_dev = get_cpu_device(0);
	unsigned long max_freq;
	struct clk *cpu_clk;
	u32 cpu_freq_table[4] = {0};
	int err, i;

	if (!of_match_node(machines, of_root))
		return -ENODEV;

	of_property_read_u32_array(cpu_dev->of_node, "clock-frequency", cpu_freq_table, 4);

	if (cpu_freq_table[0]) {
		for (i = 0; i < 4; i++)
			freq_table[i].frequency = cpu_freq_table[i] / 1000;
	}

	cpufreq = devm_kzalloc(cpu_dev, sizeof(*cpufreq), GFP_KERNEL);
	if (!cpufreq)
		return -ENOMEM;

	cpufreq->cpu_clk = clk_get(NULL, "pll8");
	if (IS_ERR(cpu_clk))
		return -ENODEV;

	max_freq = clk_get_rate(cpu_clk);

	cpufreq->dev = cpu_dev;
	cpufreq->driver.get = nvt_cpufreq_get;
	cpufreq->driver.attr = cpufreq_generic_attr;
	cpufreq->driver.init = nvt_cpu_init;
	cpufreq->driver.exit = nvt_cpu_exit;
	cpufreq->driver.flags = CPUFREQ_NEED_INITIAL_FREQ_CHECK;
	cpufreq->driver.verify = cpufreq_generic_frequency_table_verify;
	cpufreq->driver.suspend = cpufreq_generic_suspend;
	cpufreq->driver.driver_data = cpufreq;
	cpufreq->driver.target_index = nvt_target;
	snprintf(cpufreq->driver.name, CPUFREQ_NAME_LEN, "novatek");

	err = cpufreq_register_driver(&cpufreq->driver);
	if (err)
		goto put_pll;

	return 0;

put_pll:
	clk_put(cpufreq->cpu_clk);

	return err;
}
device_initcall(nvt_cpufreq_init);
