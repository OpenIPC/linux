
#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/of_address.h>
#include <linux/of_platform.h>
#include <linux/cpuidle.h>
#include <linux/module.h>
#include <asm/cacheflush.h>
#include <asm/smp_plat.h>
#include <asm/system_misc.h>
#include "core.h"

enum {
	CPU_OP_NONE = 0,
	CPU_OP_DOWN = 1,
	CPU_OP_UP = 2,
};

static int wait_for_cpu_up(unsigned int cpu);
static void cpu_op_task(struct work_struct *work);

static void __iomem *pmu_base;
static void __iomem *intc_base;
static DECLARE_WORK(cpu_op_wq, cpu_op_task);
static atomic_t fh_cpu_op = ATOMIC_INIT(CPU_OP_NONE);

void fhca7_wakeup_cpu(unsigned int cpu)
{
	if (!pmu_base)
		return;

	if (atomic_read(&fh_cpu_op) == CPU_OP_NONE && cpu_is_offline(cpu)) {
		pr_debug("cpu%u up\n", cpu);
		atomic_set(&fh_cpu_op, CPU_OP_UP);
		schedule_work_on(0, &cpu_op_wq);
	}
}

void fhca7_shutdown_cpu(unsigned int cpu)
{
	if (!pmu_base)
		return;

	if (atomic_read(&fh_cpu_op) == CPU_OP_NONE && cpu_active(cpu)) {
		pr_debug("cpu%u down\n", cpu);
		atomic_set(&fh_cpu_op, CPU_OP_DOWN);
		schedule_work_on(0, &cpu_op_wq);
	}
}

int fhca7_force_cpu_poweron(unsigned int cpu)
{
	int ret = 0;

	if (fhca7_get_cpu_jump(cpu) == 0)
		return -EBUSY;

	/* force up */
	writel(0x00079444, pmu_base + OFFSET_LOWPOWER_CTRL);

	/* wait for cpu poweron and init done */
	ret = wait_for_cpu_up(cpu);

	if (ret)
		pr_err("cpu%u poweron fail, timeout %d\n", cpu, ret);

	writel(0, pmu_base + OFFSET_LOWPOWER_CTRL);
	return ret;
}

void fhca7_wakeup_cpu_force(unsigned int cpu)
{
	if (pmu_base && cpu_is_offline(cpu)) {
		if (!fhca7_force_cpu_poweron(cpu))
			fhca7_wakeup_cpu(cpu);
	}
}

static inline void fhca7_sencondary_cpu_do_idle(unsigned int cpu)
{
	fhca7_shutdown_cpu(cpu);
	cpu_do_idle();
}

static inline void fhca7_cpu0_do_idle(unsigned int cpu)
{
	cpu_do_idle();
}

static void fhca7_cpu_idle(void)
{
	unsigned int cpu = smp_processor_id();
	if (cpu == 0)
		fhca7_cpu0_do_idle(cpu);
	else
		fhca7_sencondary_cpu_do_idle(cpu);
}

static int wait_for_cpu_up(unsigned int cpu)
{
	unsigned long timeout = jiffies + msecs_to_jiffies(50);

	while (fhca7_get_cpu_jump(cpu)) {
		if (time_after(jiffies, timeout))
			return -1;
		msleep(1);
	}

	return 0;
}

static void cpu_op_task(struct work_struct *work)
{
	int cpu = 1;
	int ret = 0;
	int cpu_op = atomic_read(&fh_cpu_op);

	if (cpu_op == CPU_OP_DOWN && cpu_active(cpu)) {
		ret = cpu_down(cpu);
		if (!ret) {
			atomic_set(&fh_cpu_op, CPU_OP_NONE);
			/* enable core 1 int wakeup */
			writel(0x00078444, pmu_base + OFFSET_LOWPOWER_CTRL);
		} else {
			pr_err("cpu%d down error %d\n", cpu, ret);
			return;
		}
	}
	else if (cpu_op == CPU_OP_UP && cpu_is_offline(cpu)) {
		if (wait_for_cpu_up(cpu)) {
			pr_err("wait for cpu%u up timeout\n", cpu);
			return;
		}
		/* disable core 1 int wakeup */
		writel(0, pmu_base + OFFSET_LOWPOWER_CTRL);
		ret = cpu_up(cpu);
		if (!ret) {
			atomic_set(&fh_cpu_op, CPU_OP_NONE);
		}
		else {
			pr_err("cpu%d up error %d\n", cpu, ret);
			writel(0x00078444, pmu_base + OFFSET_LOWPOWER_CTRL);
			return;
		}
	}
}

static int fhca7_cpuidle_enter(struct cpuidle_device *dev,
			      struct cpuidle_driver *drv, int index)
{
	fhca7_cpu_idle();
	return index;
}

static struct cpuidle_driver fhca7_cpuidle_driver = {
	.name             = "fhca7_cpuidle",
	.owner            = THIS_MODULE,
	.states[0] = {
		.enter            = fhca7_cpuidle_enter,
		.exit_latency     = 2,
		.target_residency = 1,
		.name             = "fhca7 SRPG",
		.desc             = "CPU powered off",
	},
	.state_count = 1,
};

static int __init fhca7_cpuidle_init(void)
{
	struct device_node *node, *idle_node;
	struct clk *clk;

	node = of_find_compatible_node(NULL, NULL, "fh,fh-pmu");
	if (!node) {
		return -ENOENT;
	}

	pmu_base = of_iomap(node, 0);
	of_node_put(node);
	if (!pmu_base) {
		return -ENOMEM;
	}

	idle_node = of_find_compatible_node(NULL, NULL, "fh,fh-cpuidle");
	if (!idle_node) {
		return -ENOENT;
	}

	clk = of_clk_get_by_name(idle_node, "intc");
	if (IS_ERR(clk)) {
		pr_err("failed to get intc clk\n");
		return PTR_ERR(clk);
	}
	else
		clk_prepare_enable(clk);

	intc_base = of_iomap(idle_node, 0);
	of_node_put(idle_node);
	if (!intc_base) {
		return -ENOMEM;
	}

	/* enable timer, wdt int */
	writel_relaxed(0xc, intc_base);

	arm_pm_idle = fhca7_cpu_idle;

	return cpuidle_register(&fhca7_cpuidle_driver, NULL);
}

late_initcall(fhca7_cpuidle_init);
