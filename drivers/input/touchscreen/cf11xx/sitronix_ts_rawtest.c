#include "sitronix_ts.h"

#define MAX_RX_COUNT 12
#define MAX_TX_COUNT 21
#define MAX_KEY_COUNT 4
#define MAX_SENSOR_COUNT MAX_RX_COUNT + MAX_TX_COUNT + MAX_KEY_COUNT

#define MAX_RAW_LIMIT 7800
#define MIN_RAW_LIMIT 3700

#define ST_RAW_LOG_PATH "/sdcard/ST_RAW_LOG.txt"
#define ST_LOGFILE_MAX_LENGTH 80

//#define ST_RAWTEST_LOGFILE

static st_int st_drv_Get_2D_Length(st_int tMode[])
{
	if (tMode[0] == 0)
		return tMode[1];
	else
		return tMode[2];
}

static st_int st_drv_Get_2D_Count(st_int tMode[])
{
	if (tMode[0] == 0)
		return tMode[2];
	else
		return tMode[1];
}

static st_int st_drv_Get_2D_RAW(st_int tMode[], st_int rawJ[], st_int gsMode,
				st_u8 *rtbuf)
{
	st_int count = st_drv_Get_2D_Count(tMode);
	st_int length = st_drv_Get_2D_Length(tMode);
	st_int maxTimes = 40;
	st_int dataCount = 0;
	st_int times = maxTimes;
	st_int readLength = 8 + 2 * length;

	st_u8 raw[0x40];
	st_int i = 0;
	//	st_int j=0;
	st_int index;
	st_int keyAddCount = (tMode[3] > 0) ? 1 : 0;
	st_int rawI;
	st_int errorCount = 0;
	st_u8 isFillData[MAX_SENSOR_COUNT];

#ifdef ST_RAWTEST_LOGFILE
	struct file *filp;
	char data_buf[ST_LOGFILE_MAX_LENGTH];
	mm_segment_t fs;
	loff_t pos;
	filp = filp_open(ST_RAW_LOG_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (IS_ERR(filp)) {
		STX_ERROR("ST open %s error...", ST_RAW_LOG_PATH);
		return -1;
	}

	fs = get_fs();
	set_fs(KERNEL_DS);
	pos = 0;
#endif

	memset(raw, 0, 0x40);
	memset(isFillData, 0, count + keyAddCount);

	while (dataCount != (count + keyAddCount) && times-- > 0) {
		stx_i2c_read_bytes(0x40, raw, readLength);
		if (raw[0] == 6) {
			index = raw[2];
			if (isFillData[index] == 0) {
				isFillData[index] = 1;
				dataCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data_buf, ST_LOGFILE_MAX_LENGTH,
					 "index%d :", index);
				vfs_write(filp, data_buf, strlen(data_buf),
					  &pos);
#endif
				for (i = 0; i < length; i++) {
					rawI = raw[4 + 2 * i] * 0x100 +
					       raw[5 + 2 * i];
#ifdef ST_RAWTEST_LOGFILE
					snprintf(data_buf,
						 ST_LOGFILE_MAX_LENGTH, "%d,",
						 rawI);
					vfs_write(filp, data_buf,
						  strlen(data_buf), &pos);
#endif

					STX_INFO("Sensor RAW %d,%d = %d", index,
						 i, rawI);
					if (rawI > MAX_RAW_LIMIT ||
					    rawI < MIN_RAW_LIMIT) {
#ifdef ST_RAWTEST_LOGFILE
						snprintf(
							data_buf,
							ST_LOGFILE_MAX_LENGTH,
							"Error: Sensor RAW %d,%d = %d out of limity (%d,%d)\n",
							index, i, rawI,
							MIN_RAW_LIMIT,
							MAX_RAW_LIMIT);
						vfs_write(filp, data_buf,
							  strlen(data_buf),
							  &pos);
#endif
						STX_ERROR(
							"Error: Sensor RAW %d,%d = %d out of limity (%d,%d)",
							index, i, rawI,
							MIN_RAW_LIMIT,
							MAX_RAW_LIMIT);
						rtbuf[index * length + i] = 1;
						errorCount++;
					}
				}
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data_buf, ST_LOGFILE_MAX_LENGTH, "\n");
				vfs_write(filp, data_buf, strlen(data_buf),
					  &pos);
#endif
			}
		} else if (raw[0] == 7) {
			//key
			if (isFillData[count] == 0) {
				isFillData[count] = 1;
				dataCount++;
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data_buf, ST_LOGFILE_MAX_LENGTH,
					 "key :");
				vfs_write(filp, data_buf, strlen(data_buf),
					  &pos);
#endif
				for (i = 0; i < tMode[3]; i++) {
					rawI = raw[4 + 2 * i] * 0x100 +
					       raw[5 + 2 * i];
#ifdef ST_RAWTEST_LOGFILE
					snprintf(data_buf,
						 ST_LOGFILE_MAX_LENGTH, "%d,",
						 rawI);
					vfs_write(filp, data_buf,
						  strlen(data_buf), &pos);
#endif
					if (rawI > MAX_RAW_LIMIT ||
					    rawI < MIN_RAW_LIMIT) {
#ifdef ST_RAWTEST_LOGFILE
						snprintf(
							data_buf,
							ST_LOGFILE_MAX_LENGTH,
							"Error: Key RAW %d = %d out of limity (%d,%d)\n",
							i, rawI, MIN_RAW_LIMIT,
							MAX_RAW_LIMIT);
						vfs_write(filp, data_buf,
							  strlen(data_buf),
							  &pos);
#endif
						STX_ERROR(
							"Error: Key RAW %d = %d out of limity ",
							i, rawI);
						rtbuf[count * length + i] = 1;
						errorCount++;
					}
				}
#ifdef ST_RAWTEST_LOGFILE
				snprintf(data_buf, ST_LOGFILE_MAX_LENGTH, "\n");
				vfs_write(filp, data_buf, strlen(data_buf),
					  &pos);
#endif
			}
		}
	}
#ifdef ST_RAWTEST_LOGFILE
	if ((times > 0) && (errorCount == 0)) {
		snprintf(data_buf, ST_LOGFILE_MAX_LENGTH, "RAW TEST PASS");
		vfs_write(filp, data_buf, strlen(data_buf), &pos);
	} else {
		snprintf(data_buf, ST_LOGFILE_MAX_LENGTH, "RAW TEST FAILED");
		vfs_write(filp, data_buf, strlen(data_buf), &pos);
	}

	set_fs(fs);
	filp_close(filp, NULL);
	STX_INFO("Test result file	: %s", ST_RAW_LOG_PATH);
#endif

	if (times <= 0) {
		STX_ERROR("Get 2D RAW fail!");
		return -1;
	}
	return errorCount;
}
/*
	rtbuf is a u8 buf[X * Y + K]
	X:sensor count of X
	Y:sensor count of Y
	K:sensor count of K
	buf[?] = 0 means pass
	buf[?] = 1 means fail
	bug[20] means TX: 20/RX_count , RX: 20%RX_count
	RX_count : maybe X or Y , according layout..
*/
static int st_drv_test_raw(st_u8 *rtbuf)
{
	st_int result = 0;
	st_u8 buf[8];
	st_int sensorCount = 0;
	st_int raw_J[MAX_SENSOR_COUNT];
	st_int tMode[4];
	STX_INFO("start of st_drv_test_raw");
	//check status
	memset(buf, 0, 8);
	if (stx_i2c_read_bytes(1, buf, 8) < 0) {
		STX_ERROR("get status fail");
		return -1;
	}

	STX_INFO("status :0x%X", buf[0]);
	if (buf[0] != 0 && buf[0] != 4) {
		STX_ERROR("can't test in this status");
		result = -1;
		goto st_drv_notest;
	}
	//go develop page
	memset(buf, 0, 8);
	stx_i2c_read_bytes(0xFF, buf, 8);
	if (buf[1] != 0x53 || buf[2] != 0x54 || buf[3] != 0x50 ||
	    buf[4] != 0x41) {
		STX_ERROR("ic check fail , not sitronix protocol type");
		result = -1;
		goto st_drv_notest;
	}

	buf[0] = buf[6];
	buf[1] = buf[7];
	stx_i2c_write_bytes(buf, 2);

	st_msleep(5);
	stx_i2c_read_bytes(0xFF, buf, 2);
	STX_INFO("page 0x%X", buf[0]);
	if (buf[0] != 0xEF) {
		STX_ERROR("change page fail");
		result = -1;
		goto st_drv_notest;
	}

	stx_i2c_read_bytes(0xF0, buf, 8);
	tMode[0] = ((buf[0] & 0x04) >> 2); //tx flag

	tMode[1] = buf[5]; //x
	tMode[2] = buf[6]; //y
	tMode[3] = buf[7] & 0xf; // key

	sensorCount = tMode[1] + tMode[2] + tMode[3];

	STX_INFO("sensor count:%d %d %d", tMode[1], tMode[2], tMode[3]);

	memset(rtbuf, 0, tMode[1] * tMode[2] + tMode[3]);
	//get raw and judge
	result = st_drv_Get_2D_RAW(tMode, raw_J, 0, rtbuf);
	if (result != 0) {
		STX_ERROR("Error: Test fail with %d sensor", result);
		result = -1;
	} else {
		STX_INFO("Test successed!");
	}
	//reset
	buf[0] = 2;
	buf[1] = 1;
	stx_i2c_write_bytes(buf, 2);

	st_msleep(100);
st_drv_notest:
	return result;
}

int st_testraw_invoke(void)
{
	st_u8 fstatus;
	st_u8 fraw[MAX_RX_COUNT * MAX_TX_COUNT];

	stx_gpts.is_testing = true;
	fstatus = st_drv_test_raw(fraw);
	stx_gpts.is_testing = false;
	return fstatus;
}
