#undef TRACE_SYSTEM
#define TRACE_SYSTEM nvt

#if !defined(_TRACE_NVT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_NVT_H

#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(nvt1,

	TP_PROTO(int condition, const char *desc, u32 arg0),

	TP_ARGS(condition, desc, arg0),

	TP_STRUCT__entry(
		__string(	desc,	desc	)
		__field(	u32,	arg0	)
	),

	TP_fast_assign(
		__assign_str(desc, desc);
		__entry->arg0		= arg0;
	),

	TP_printk("%s 0x%x",
		__get_str(desc),
		__entry->arg0)
);

DEFINE_EVENT_CONDITION(nvt1, nvt1_event,
	TP_PROTO(int condition, const char *desc, u32 arg0),

	TP_ARGS(condition, desc, arg0),
	TP_CONDITION(condition)
);

DECLARE_EVENT_CLASS(nvt2,

	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1),

	TP_ARGS(condition, desc, arg0, arg1),

	TP_STRUCT__entry(
		__string(	desc,	desc	)
		__field(	u32,	arg0	)
		__field(	u32,	arg1	)
	),

	TP_fast_assign(
		__assign_str(desc, desc);
		__entry->arg0		= arg0;
		__entry->arg1		= arg1;
	),

	TP_printk("%s 0x%x 0x%x",
		__get_str(desc),
		__entry->arg0,
		__entry->arg1)
);

DEFINE_EVENT_CONDITION(nvt2, nvt2_event,
	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1),

	TP_ARGS(condition, desc, arg0, arg1),

	TP_CONDITION(condition)
);

DECLARE_EVENT_CLASS(nvt3,

	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2),

	TP_ARGS(condition, desc, arg0, arg1, arg2),

	TP_STRUCT__entry(
		__string(	desc,	desc	)
		__field(	u32,	arg0	)
		__field(	u32,	arg1	)
		__field(	u32,	arg2	)
	),

	TP_fast_assign(
		__assign_str(desc, desc);
		__entry->arg0		= arg0;
		__entry->arg1		= arg1;
		__entry->arg2		= arg2;
	),

	TP_printk("%s 0x%x 0x%x 0x%x",
		__get_str(desc),
		__entry->arg0,
		__entry->arg1,
		__entry->arg2)
);

DEFINE_EVENT_CONDITION(nvt3, nvt3_event,
	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2),

	TP_ARGS(condition, desc, arg0, arg1, arg2),

	TP_CONDITION(condition)
);

DECLARE_EVENT_CLASS(nvt4,

	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2, u32 arg3),

	TP_ARGS(condition, desc, arg0, arg1, arg2, arg3),

	TP_STRUCT__entry(
		__string(	desc,	desc	)
		__field(	u32,	arg0	)
		__field(	u32,	arg1	)
		__field(	u32,	arg2	)
		__field(	u32,	arg3	)
	),

	TP_fast_assign(
		__assign_str(desc, desc);
		__entry->arg0		= arg0;
		__entry->arg1		= arg1;
		__entry->arg2		= arg2;
		__entry->arg3		= arg3;
	),

	TP_printk("%s 0x%x 0x%x 0x%x 0x%x",
		__get_str(desc),
		__entry->arg0,
		__entry->arg1,
		__entry->arg2,
		__entry->arg3)
);

DEFINE_EVENT_CONDITION(nvt4, nvt4_event,
	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2, u32 arg3),

	TP_ARGS(condition, desc, arg0, arg1, arg2, arg3),

	TP_CONDITION(condition)
);

DECLARE_EVENT_CLASS(nvt5,

	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2, u32 arg3, u32 arg4),

	TP_ARGS(condition, desc, arg0, arg1, arg2, arg3, arg4),

	TP_STRUCT__entry(
		__string(	desc,	desc	)
		__field(	u32,	arg0	)
		__field(	u32,	arg1	)
		__field(	u32,	arg2	)
		__field(	u32,	arg3	)
		__field(	u32,	arg4	)
	),

	TP_fast_assign(
		__assign_str(desc, desc);
		__entry->arg0		= arg0;
		__entry->arg1		= arg1;
		__entry->arg2		= arg2;
		__entry->arg3		= arg3;
		__entry->arg4		= arg4;
	),

	TP_printk("%s 0x%x 0x%x 0x%x 0x%x 0x%x",
		__get_str(desc),
		__entry->arg0,
		__entry->arg1,
		__entry->arg2,
		__entry->arg3,
		__entry->arg4)
);

DEFINE_EVENT_CONDITION(nvt5, nvt5_event,
	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2, u32 arg3, u32 arg4),

	TP_ARGS(condition, desc, arg0, arg1, arg2, arg3, arg4),

	TP_CONDITION(condition)
);

DECLARE_EVENT_CLASS(nvt6,

	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2, u32 arg3, u32 arg4, u32 arg5),

	TP_ARGS(condition, desc, arg0, arg1, arg2, arg3, arg4, arg5),

	TP_STRUCT__entry(
		__string(	desc,	desc	)
		__field(	u32,	arg0	)
		__field(	u32,	arg1	)
		__field(	u32,	arg2	)
		__field(	u32,	arg3	)
		__field(	u32,	arg4	)
		__field(	u32,	arg5	)
	),

	TP_fast_assign(
		__assign_str(desc, desc);
		__entry->arg0		= arg0;
		__entry->arg1		= arg1;
		__entry->arg2		= arg2;
		__entry->arg3		= arg3;
		__entry->arg4		= arg4;
		__entry->arg5		= arg5;
	),

	TP_printk("%s 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
		__get_str(desc),
		__entry->arg0,
		__entry->arg1,
		__entry->arg2,
		__entry->arg3,
		__entry->arg4,
		__entry->arg5)
);

DEFINE_EVENT_CONDITION(nvt6, nvt6_event,
	TP_PROTO(int condition, const char *desc, u32 arg0, u32 arg1,
		 u32 arg2, u32 arg3, u32 arg4, u32 arg5),

	TP_ARGS(condition, desc, arg0, arg1, arg2, arg3, arg4, arg5),

	TP_CONDITION(condition)
);
#endif /* _TRACE_NVT_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
