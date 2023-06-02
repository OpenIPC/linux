#ifndef __GM_JIFFIES_H__
#define __GM_JIFFIES_H__

#define gm_jiffies      get_gm_jiffies()
#define gm_jiffies_u64  get_gm_jiffies_u64()

/* @unsigned long get_gm_jiffies(void)
 * @Purpose: This function provides jiffies with 1ms granularity. This value will count up to 0xFFFFFFFF
 *           and warp around again.
 * @Parameter:
 *   None
 * @Return:
 *      0 means the timer is not active. Othwise return non-zero value.
 */
unsigned long get_gm_jiffies(void);

/* @u64 get_gm_jiffies_u64(void)
 * @Purpose: This function provides jiffies with 1ms granularity. This value will count up to 0xFFFFFFFFFFFFFFFF
 *           and warp around again.
 * @Parameter:
 *   None
 * @Return:
 *      0 means the timer is not active. Othwise return non-zero value.
 */
u64 get_gm_jiffies_u64(void);

#endif /* __GM_JIFFIES_H__ */