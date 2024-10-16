#include <linux/kthread.h>
#include "sitronix_ts.h"

#define SITRONIX_MT_DIS_LIMIT 10000
#define DELAY_MONITOR_THREAD_START_PROBE 300

static int i2cErrorCount = 0;
static struct task_struct *SitronixMonitorThread = NULL;
static int gMonitorThreadSleepInterval = 300; // 0.3 sec
static atomic_t iMonitorThreadPostpone = ATOMIC_INIT(0);
static int sitronix_ts_delay_monitor_thread_start =
	DELAY_MONITOR_THREAD_START_PROBE;
static uint8_t PreCheckData[4];
static int StatusCheckCount = 0;
static int DisCheckCount = 0;

static int sitronix_ts_monitor_thread(void *data)
{
	int ret = 0;
	int i;
	uint8_t buf[8] = { 0 };
	uint8_t disbuf[40 * 2 + 4] = { 0 };
	int result = 0;
	signed short disv;
	bool disOK;

	STX_INFO("%s start and delay %d ms", __func__,
		 sitronix_ts_delay_monitor_thread_start);
	msleep(DELAY_MONITOR_THREAD_START_PROBE);
	while (!kthread_should_stop()) {
		if (atomic_read(&iMonitorThreadPostpone)) {
			atomic_set(&iMonitorThreadPostpone, 0);
		} else if (stx_gpts.suspend_state || stx_gpts.is_upgrading ||
			   stx_gpts.is_testing || stx_gpts.fapp_in) {
			STX_ERROR(
				"%s suspend state = %d ,is_upgrading = %d ,is_testing = %d,fapp_in=%d ",
				__func__, stx_gpts.suspend_state,
				stx_gpts.is_upgrading, stx_gpts.is_testing,
				stx_gpts.fapp_in);
		} else {
			mutex_lock(&stx_gpts.dev_mutex);
			ret = stx_i2c_read_bytes(0x1, buf, 8);
			ret = stx_i2c_read_bytes(
				0x40, disbuf,
				4 + (stx_gpts.ts_dev_info.tx_chs * 2));
			mutex_unlock(&stx_gpts.dev_mutex);
			if (ret < 0) {
				STX_ERROR("read I2C fail (%d)", ret);
				result = 0;
				goto exit_i2c_invalid;
			}
			STX_INFO("monitor sensing counter: %02x %02x", buf[6],
				 buf[7]);

			if ((buf[0] & 0x0F) == 0x6) {
				result = 0;
				STX_ERROR("read Status: bootcode");
				goto exit_i2c_invalid;
			} else {
				result = 1;
				if (PreCheckData[0] == buf[6] &&
				    PreCheckData[1] == buf[7])
					StatusCheckCount++;
				else
					StatusCheckCount = 0;
				PreCheckData[0] = buf[6];
				PreCheckData[1] = buf[7];

				if (3 <= StatusCheckCount) {
					STX_ERROR("IC Status doesn't update! ");
					result = -1;
					StatusCheckCount = 0;
				}
			}

			if (disbuf[0] == 0x86) {
				disOK = true;
				for (i = 0; i < stx_gpts.ts_dev_info.tx_chs;
				     i++) {
					disv = (signed short)((disbuf[4 +
								      2 * i]) *
								      0x100 +
							      disbuf[5 + 2 * i]);

					if (disv > SITRONIX_MT_DIS_LIMIT ||
					    disv < -SITRONIX_MT_DIS_LIMIT) {
						STX_ERROR(
							"MT get error Distance for (%d,%d) , distance value = %d",
							disbuf[2], i, disv);
						disOK = false;
						break;
					}
				}

				if (!disOK)
					DisCheckCount++;
				else
					DisCheckCount = 0;

				if (3 <= DisCheckCount) {
					STX_ERROR(
						"Distance error for 3 times! ");
					result = -1;
					DisCheckCount = 0;
				}
			}
			if (-1 == result) {
				STX_ERROR("Chip abnormal, reset it!");
				st_reset_ic();
#ifdef ST_GLOVE_SWITCH_MODE
				if (stx_gpts.glove_mode)
					st_enter_glove_mode(&stx_gpts);
#endif
				i2cErrorCount = 0;
				StatusCheckCount = 0;
			}
		exit_i2c_invalid:
			if (0 == result) {
				i2cErrorCount++;
				if ((2 <= i2cErrorCount)) {
					STX_ERROR(
						"I2C abnormal or status bootcode, reset it!");
					st_reset_ic();
#ifdef ST_GLOVE_SWITCH_MODE
					if (stx_gpts.glove_mode)
						st_enter_glove_mode(&stx_gpts);
#endif
					i2cErrorCount = 0;
					StatusCheckCount = 0;
				}
			} else
				i2cErrorCount = 0;
		}
		msleep(gMonitorThreadSleepInterval);
	}
	STX_DEBUG("%s exit", __FUNCTION__);
	return 0;
}

int sitronix_monitor_start(void)
{
	STX_INFO("%s ENTER ", __func__);
	//== Add thread to monitor chip
	atomic_set(&iMonitorThreadPostpone, 1);

	if (!SitronixMonitorThread) {
		SitronixMonitorThread =
			kthread_run(sitronix_ts_monitor_thread, "Sitronix",
				    "Monitorthread");
		if (IS_ERR(SitronixMonitorThread)) {
			SitronixMonitorThread = NULL;
			STX_ERROR("sitronix_monitor_start err");
			return 1;
		}
		STX_INFO("%s success ", __func__);
	} else {
		STX_ERROR("%s already start ", __func__);
	}
	return 0;
}

int sitronix_monitor_stop(void)
{
	STX_INFO("%s ENTER ", __func__);
	if (SitronixMonitorThread) {
		kthread_stop(SitronixMonitorThread);
		SitronixMonitorThread = NULL;
		STX_INFO("%s success ", __func__);
	} else {
		STX_ERROR("sitronix_monitor_stop already stop");
	}
	return 0;
}

int sitronix_monitor_delay(void)
{
	atomic_set(&iMonitorThreadPostpone, 1);

	return 0;
}