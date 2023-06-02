#include <linux/irq.h>
#include <linux/seq_file.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kprobes.h>
#include <linux/interrupt.h>
#include <linux/irqdesc.h>

#define CREATE_TRACE_POINTS
#include <trace/events/nvt_profiler.h>

struct nvt_profiler_params {
	bool full_feature;
};

struct nvt_irq_kretprobe_data {
	struct irq_desc *desc;
	ktime_t entry_stamp;
};

struct nvt_profiler_params params = {
	.full_feature = true,
};

DEFINE_STATIC_KEY_FALSE(full_feature_key);

static int __init early_param_nprofile_irq_duration(char *p)
{
	if (!strncmp(p, "on", 2))
		params.full_feature = true;
	else
		params.full_feature = false;
	return 0;
}
early_param("nprofile_irq_duration", early_param_nprofile_irq_duration);

static int __init set_nprofile_irqsoff(char *str)
{
	int nprofile_irqsoff_enable(char *str) __init;

	return nprofile_irqsoff_enable(str);
}
__setup("nprofile_irqsoff=", set_nprofile_irqsoff);

static int nvt_irq_kret_probe_entry(struct kretprobe_instance *ri,
				    struct pt_regs *regs)
{
	struct nvt_irq_kretprobe_data *data;
#if defined(CONFIG_ARM)
	struct irq_desc *desc = (struct irq_desc*)(regs->ARM_r0);
#elif defined(CONFIG_ARM64)
	struct irq_desc *desc = (struct irq_desc*)(regs->regs[0]);
#else
#error "CPU architecture not supported"
#endif
	ktime_t current_stamp = ktime_get();

	data = (struct nvt_irq_kretprobe_data *)ri->data;
	data->entry_stamp = current_stamp;
	data->desc = desc;

	if (static_branch_unlikely(&full_feature_key)) {
		/* write out last calculation */
		if (ktime_ms_delta(current_stamp, desc->start_stamp) > 1000) {
			desc->last_stamp = desc->start_stamp;
			desc->last_sum = desc->cur_sum;
			desc->last_count = desc->cur_count;
			desc->start_stamp = current_stamp;
			desc->cur_sum = 0;
			desc->cur_count = 0;
		}
	}

	return 0;
}

#define ns_to_ms(ns) ((ns) / 1000000)
static int nvt_irq_kret_probe_ret(struct kretprobe_instance *ri,
				  struct pt_regs *regs)
{
	struct nvt_irq_kretprobe_data *data =
				(struct nvt_irq_kretprobe_data *)ri->data;
	struct irq_desc *desc = data->desc;

	ktime_t cur = ktime_get();
	ktime_t start = desc->start_stamp;
	ktime_t entry = data->entry_stamp;
	s64 duration = ktime_to_ns(ktime_sub(cur, entry));

	/* for last_ns and max_in in /proc/interrupts */
	desc->last_duration = duration;
	if (unlikely(duration > desc->max_duration))
		desc->max_duration = duration;

	/* for irq_duration trace_event */
	trace_irq_duration(desc->irq_data.irq, duration);

	/* for cnt/sec and ns/sec in /proc/interrupts */
	if (static_branch_unlikely(&full_feature_key)) {

		/*
		 * Case 1:
		 *    start                 start+1s               start+2s
		 *      |-------+--------+-----|----------------------|
		 *            entry     cur
		 * Case 2:
		 *    start                 start+1s               start+2s
		 *      |-------+--------------|----------+-----------|
		 *            entry                      cur
		 * Case 3:
		 *    start                 start+1s               start+2s
		 *      |-------+--------------|----------------------|
		 *            entry           cur
		 */
		if (ktime_to_ms(ktime_sub(cur, start)) < 1000) {
			/* case 1 */
			desc->cur_count++;
			desc->cur_sum += duration;
		} else {
			/* case 2,3 */
			ktime_t start_1 = ktime_add_ms(start, 1000);

			/* write out last calculation */
			desc->last_stamp = start;
			desc->last_sum = desc->cur_sum +
					 ktime_to_ns(ktime_sub(start_1, entry));
			desc->last_count = desc->cur_count + 1;

			/* start new calculation */
			desc->start_stamp = cur;
			/* check for case 2 or case 3 */
			desc->cur_sum = ktime_to_ns(ktime_sub(cur, start_1));
			desc->cur_count = desc->cur_sum ? 1: 0;
		}
	}

	return 0;
}

static struct kretprobe nvt_irq_kretprobe = {
	.handler		= nvt_irq_kret_probe_ret,
	.entry_handler		= nvt_irq_kret_probe_entry,
	.data_size		= sizeof(struct nvt_irq_kretprobe_data),
	/* Probe up to 20 instances concurrently. */
	.maxactive		= 20,
};

static int __init nvt_irq_kretprobe_init(void)
{
	static const char func_name[] = "__irq_action_handler";
	int ret;

	nvt_irq_kretprobe.kp.symbol_name = func_name;
	ret = register_kretprobe(&nvt_irq_kretprobe);
	if (ret < 0) {
		pr_err("Novatek IRQ probe init failed: %d\n", ret);
		return -1;
	}

	if (params.full_feature)
		static_branch_enable(&full_feature_key);

	pr_info("Novatek IRQ probe at %s %s\n",
		nvt_irq_kretprobe.kp.symbol_name,
		!params.full_feature ? "" : "(lite)");

	return 0;
}
module_init(nvt_irq_kretprobe_init)

static void __exit nvt_irq_kretprobe_exit(void)
{
	unregister_kretprobe(&nvt_irq_kretprobe);
	/* nmissed > 0 suggests that maxactive was set too low. */
	pr_info("Novatek IRQ probe missed probing %d instances of %s\n",
		nvt_irq_kretprobe.nmissed, nvt_irq_kretprobe.kp.symbol_name);
}
module_exit(nvt_irq_kretprobe_exit)

void nvt_profiler_proc_intr_head(struct seq_file *p)
{
	seq_printf(p, "%7s %10s %10s %10s %10s", "last_ns", "max_ns", "cnt/sec",
					    "ns/sec", "timestamp");
}

void nvt_profiler_proc_intr_val(struct seq_file *p, struct irq_desc *desc)
{
	u64 ts = ktime_to_ms(desc->last_stamp);
	u32 ms = do_div(ts, 1000);

	seq_printf(p, "   %10lld %10lld %10lld %10lld %6llu.%03u",
							desc->last_duration,
							desc->max_duration,
							desc->last_count,
							desc->last_sum,
							ts, ms);
}
