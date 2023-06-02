#ifndef _NVT_PROFILER_H_
#define _NVT_PROFILER_H_

#include <linux/seq_file.h>
#include <linux/irqdesc.h>

#ifdef CONFIG_NVT_PROFILER
void nvt_profiler_proc_intr_head(struct seq_file *p);
void nvt_profiler_proc_intr_val(struct seq_file *p, struct irq_desc *desc);
#else
#define nvt_profiler_proc_intr_head(...) do { } while (0)
#define nvt_profiler_proc_intr_val(...) do { } while (0)
#endif /* CONFIG_NVT_PROFILER */

#endif /* _NVT_PROFILER_H_ */
