#ifndef __NVT_JIFFIES_H__
#define __NVT_JIFFIES_H__

#define nvt_jiffies      get_nvt_jiffies()
#define nvt_jiffies_u64  get_nvt_jiffies_u64()

/* @unsigned long get_nvt_jiffies(void)
 * @Purpose: This function provides jiffies with 1ms granularity. This value will count up to 0xFFFFFFFF
 *           and warp around again.
 * @Parameter:
 *   None
 * @Return:
 *      0 means the timer is not active. Othwise return non-zero value.
 */
unsigned long get_nvt_jiffies(void);

/* @u64 get_nvt_jiffies_u64(void)
 * @Purpose: This function provides jiffies with 1ms granularity. This value will count up to 0xFFFFFFFFFFFFFFFF
 *           and warp around again.
 * @Parameter:
 *   None
 * @Return:
 *      0 means the timer is not active. Othwise return non-zero value.
 */
u64 get_nvt_jiffies_u64(void);

#endif /* __NVT_JIFFIES_H__ */