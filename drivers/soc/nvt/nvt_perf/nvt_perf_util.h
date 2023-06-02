#ifndef __NVT_PERF_UTIL_H__
#define __NVT_PERF_UTIL_H__
#include <linux/time.h>

static inline unsigned int time_interval(struct timeval *time_result, struct timeval *time_start, struct timeval *time_end)
{
	if (unlikely(time_end->tv_sec < time_start->tv_sec))
		return -EINVAL;

	time_result->tv_sec = (time_end->tv_sec - time_start->tv_sec);
	if (unlikely(time_end->tv_usec < time_start->tv_usec)) {
		if (unlikely(time_result->tv_sec == 0)) {
			return -EINVAL;
		}

		time_result->tv_sec--;
		time_result->tv_usec = (1000000 + time_end->tv_usec) - time_start->tv_usec;
	} else {
		time_result->tv_usec = time_end->tv_usec - time_start->tv_usec;
	}

	return 0;
}

#endif
