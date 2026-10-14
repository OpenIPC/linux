#ifndef AK_ISP_TUNING_H
#define AK_ISP_TUNING_H

#include <linux/firmware.h>
#include <plat-anyka/ak_isp_drv.h>

struct ak_isp_tuning {
	const struct firmware *firmware;
	unsigned int style_size;
	unsigned int style_count;
};

int ak_isp_tuning_load(struct device *dev, const char *name,
		      unsigned int sensor_id, struct ak_isp_tuning *tuning);
int ak_isp_tuning_apply(const struct ak_isp_tuning *tuning, unsigned int style,
		       AK_ISP_SENSOR_CB *sensor, bool initialize_sensor);
void ak_isp_tuning_free(struct ak_isp_tuning *tuning);

#endif
