/* SPDX-License-Identifier: GPL-2.0 */
#undef TRACE_SYSTEM
#define TRACE_SYSTEM testtracepoint

#if !defined(_TRACE_TESTTRACEPOINT_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_TESTTRACEPOINT_H

#include <linux/tracepoint.h>

TRACE_EVENT(test,

	TP_PROTO(int id),

	TP_ARGS(id),

	TP_STRUCT__entry(
		__field(int, id)
	),

	TP_fast_assign(
		__entry->id = id;
	),

	TP_printk("id=%d", __entry->id)
);

#endif /* _TRACE_TESTTRACEPOINT_H */

/* Specify the directory where the current file is located */
#define TRACE_INCLUDE_PATH .

/* This part must be outside protection */
#include <trace/define_trace.h>
