#undef TRACE_SYSTEM
#define TRACE_SYSTEM nvt_profiler

#if !defined(_NVT_PROFILER_TRACE_H) || defined(TRACE_HEADER_MULTI_READ)
#define _NVT_PROFILER_TRACE_H

#include <linux/tracepoint.h>

TRACE_EVENT(irq_duration,

	TP_PROTO(int irq, s64 duration),

	TP_ARGS(irq, duration),

	TP_STRUCT__entry(
		__field(	int,	irq		)
		__field(	s64,	duration	)
	),

	TP_fast_assign(
		__entry->irq		= irq;
		__entry->duration	= duration;
	),

	TP_printk("irq=%d ns=%lld", __entry->irq, __entry->duration)
);

#endif /* _NVT_PROFILER_TRACE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
