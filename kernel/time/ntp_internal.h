#ifndef _LINUX_NTP_INTERNAL_H
#define _LINUX_NTP_INTERNAL_H
#ifdef CONFIG_NTP
extern void ntp_init(void);
extern void ntp_clear(void);
/* Returns how long ticks are at present, in ns / 2^NTP_SCALE_SHIFT. */
extern u64 ntp_tick_length(void);
extern ktime_t ntp_get_next_leap(void);
extern int second_overflow(time64_t secs);
extern int ntp_validate_timex(struct timex *);
extern int __do_adjtimex(struct timex *, struct timespec64 *, s32 *);
extern void __hardpps(const struct timespec64 *, const struct timespec64 *);
#else
static inline void ntp_init(void)
{}
static inline void ntp_clear(void)
{}
static inline u64 ntp_tick_length(void)
{
	//10000000<<32 (NSEC_PER_SEC<<NTP_SCALED_SHIFT)
	return 42949672960000000ULL;
}
static inline int second_overflow(unsigned long secs)
{
	return 0;
}
static inline int ntp_validate_timex(struct timex *txc)
{
	return 0;
}
static inline int __do_adjtimex(struct timex *txc, struct timespec64 *ts,
				s32 *time_tai)
{
	return 0;
}
static inline void __hardpps(const struct timespec *ts1,
			     const struct timespec *ts2)
{}
static inline ktime_t ntp_get_next_leap(void)
{
	ktime_t ret;

	ret.tv64 = KTIME_MAX;
	return ret;
}
#endif
#endif /* _LINUX_NTP_INTERNAL_H */
