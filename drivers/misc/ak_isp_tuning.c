#include <linux/errno.h>
#include <linux/device.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <asm/unaligned.h>

#include "ak_isp_tuning.h"

#define STYLE_HEADER_SIZE 0x200
#define MAX_TUNING_SIZE (1024 * 1024)
#define MAX_SENSOR_REGISTERS 4096

struct tuning_write {
	u16 offset;
	u16 size;
	int (*apply)(void *data);
};

#define TUNING_SETTER(name, type, function) \
	static int tune_##name(void *data) { return function((type *)data); }

TUNING_SETTER(blc, AK_ISP_BLC_ATTR, ak_isp_vp_set_blc_attr)
TUNING_SETTER(lsc, AK_ISP_LSC_ATTR, ak_isp_vp_set_lsc_attr)
TUNING_SETTER(raw_lut, AK_ISP_RAW_LUT_ATTR, ak_isp_vp_set_raw_lut_attr)
TUNING_SETTER(nr1, AK_ISP_NR1_ATTR, ak_isp_vp_set_nr1_attr)
TUNING_SETTER(nr2, AK_ISP_NR2_ATTR, ak_isp_vp_set_nr2_attr)
TUNING_SETTER(nr3d, AK_ISP_3D_NR_ATTR, ak_isp_vp_set_3d_nr_attr)
TUNING_SETTER(gb, AK_ISP_GB_ATTR, ak_isp_vp_set_gb_attr)
TUNING_SETTER(demo, AK_ISP_DEMO_ATTR, ak_isp_vp_set_demo_attr)
TUNING_SETTER(rgb_gamma, AK_ISP_RGB_GAMMA_ATTR, ak_isp_vp_set_rgb_gamma_attr)
TUNING_SETTER(ccm, AK_ISP_CCM_ATTR, ak_isp_vp_set_ccm_attr)
TUNING_SETTER(fcs, AK_ISP_FCS_ATTR, ak_isp_vp_set_fcs_attr)
TUNING_SETTER(wdr, AK_ISP_WDR_ATTR, ak_isp_vp_set_wdr_attr)
TUNING_SETTER(sharp_ex, AK_ISP_SHARP_EX_ATTR, ak_isp_vp_set_sharp_ex_attr)
TUNING_SETTER(sat, AK_ISP_SATURATION_ATTR, ak_isp_vp_set_saturation_attr)
TUNING_SETTER(contrast, AK_ISP_CONTRAST_ATTR, ak_isp_vp_set_contrast_attr)
TUNING_SETTER(rgb2yuv, AK_ISP_RGB2YUV_ATTR, ak_isp_vp_set_rgb2yuv_attr)
TUNING_SETTER(effect, AK_ISP_EFFECT_ATTR, ak_isp_vp_set_effect_attr)
TUNING_SETTER(dpc, AK_ISP_DDPC_ATTR, ak_isp_vp_set_dpc_attr)
TUNING_SETTER(weight, AK_ISP_WEIGHT_ATTR, ak_isp_vp_set_zone_weight)
TUNING_SETTER(af, AK_ISP_AF_ATTR, ak_isp_vp_set_af_attr)
TUNING_SETTER(wb, AK_ISP_WB_TYPE_ATTR, ak_isp_vp_set_wb_type)
TUNING_SETTER(mwb, AK_ISP_MWB_ATTR, ak_isp_vp_set_mwb_attr)
TUNING_SETTER(awb, AK_ISP_AWB_ATTR, ak_isp_vp_set_awb_attr)
TUNING_SETTER(awb_ex, AK_ISP_AWB_EX_ATTR, ak_isp_vp_set_awb_ex_attr)
TUNING_SETTER(raw_hist, AK_ISP_RAW_HIST_ATTR, ak_isp_vp_set_raw_hist_attr)
TUNING_SETTER(rgb_hist, AK_ISP_RGB_HIST_ATTR, ak_isp_vp_set_rgb_hist_attr)
TUNING_SETTER(y_hist, AK_ISP_YUV_HIST_ATTR, ak_isp_vp_set_yuv_hist_attr)
TUNING_SETTER(exposure, AK_ISP_EXP_TYPE, ak_isp_vp_set_exp_type)
TUNING_SETTER(frame_rate, AK_ISP_FRAME_RATE_ATTR, ak_isp_vp_set_frame_rate)
TUNING_SETTER(ae, AK_ISP_AE_ATTR, ak_isp_vp_set_ae_attr)
TUNING_SETTER(misc, AK_ISP_MISC_ATTR, ak_isp_vo_set_misc_attr)
TUNING_SETTER(y_gamma, AK_ISP_Y_GAMMA_ATTR, ak_isp_vp_set_y_gamma_attr)
TUNING_SETTER(hue, AK_ISP_HUE_ATTR, ak_isp_vp_set_hue_attr)

static int tune_sharp(void *data)
{
	AK_ISP_SHARP_ATTR *sharp = data;

	if (sharp->linkage_sharp_attr[0].mf_hpf_shift < 3)
		sharp->linkage_sharp_attr[0].mf_hpf_shift = 3;
	return ak_isp_vp_set_sharp_attr(sharp);
}

#define W(name, type, offset) { offset, sizeof(type), tune_##name }

static const struct tuning_module {
	u16 size;
	struct tuning_write writes[6];
} modules[] = {
	{ 186, { W(blc, AK_ISP_BLC_ATTR, 4) } },
	{ 192, { W(lsc, AK_ISP_LSC_ATTR, 4) } },
	{ 876, { W(raw_lut, AK_ISP_RAW_LUT_ATTR, 4) } },
	{ 2268, { W(nr1, AK_ISP_NR1_ATTR, 4), W(nr2, AK_ISP_NR2_ATTR, 0x6fa) } },
	{ 1088, { W(nr3d, AK_ISP_3D_NR_ATTR, 4) } },
	{ 86, { W(gb, AK_ISP_GB_ATTR, 4) } },
	{ 22, { W(demo, AK_ISP_DEMO_ATTR, 4) } },
	{ 876, { W(rgb_gamma, AK_ISP_RGB_GAMMA_ATTR, 4) } },
	{ 136, { W(ccm, AK_ISP_CCM_ATTR, 4) } },
	{ 106, { W(fcs, AK_ISP_FCS_ATTR, 4) } },
	{ 9966, { W(wdr, AK_ISP_WDR_ATTR, 4) } },
	{ 11096, { W(sharp, AK_ISP_SHARP_ATTR, 4), W(sharp_ex, AK_ISP_SHARP_EX_ATTR, 0x2b3a) } },
	{ 206, { W(sat, AK_ISP_SATURATION_ATTR, 4) } },
	{ 64, { W(contrast, AK_ISP_CONTRAST_ATTR, 4) } },
	{ 6, { W(rgb2yuv, AK_ISP_RGB2YUV_ATTR, 4) } },
	{ 14, { W(effect, AK_ISP_EFFECT_ATTR, 4) } },
	{ 4188, { W(dpc, AK_ISP_DDPC_ATTR, 4) } },
	{ 260, { W(weight, AK_ISP_WEIGHT_ATTR, 4) } },
	{ 46, { W(af, AK_ISP_AF_ATTR, 4) } },
	{ 528, { W(wb, AK_ISP_WB_TYPE_ATTR, 4), W(mwb, AK_ISP_MWB_ATTR, 6),
		 W(awb, AK_ISP_AWB_ATTR, 0x12), W(awb_ex, AK_ISP_AWB_EX_ATTR, 0xcc) } },
	{ 240, { W(raw_hist, AK_ISP_RAW_HIST_ATTR, 4), W(rgb_hist, AK_ISP_RGB_HIST_ATTR, 6),
		 W(y_hist, AK_ISP_YUV_HIST_ATTR, 8), W(exposure, AK_ISP_EXP_TYPE, 0xa),
		 W(frame_rate, AK_ISP_FRAME_RATE_ATTR, 0xc), W(ae, AK_ISP_AE_ATTR, 0x24) } },
	{ 30, { W(misc, AK_ISP_MISC_ATTR, 4) } },
	{ 306, { W(y_gamma, AK_ISP_Y_GAMMA_ATTR, 4) } },
	{ 996, { W(hue, AK_ISP_HUE_ATTR, 4) } },
};

static int validate_style(const u8 *data, unsigned int size,
			  unsigned int sensor_id, unsigned int count)
{
	unsigned int offset = STYLE_HEADER_SIZE;
	unsigned int i, w, payload;

	if (get_unaligned_le32(data) != count ||
	    get_unaligned_le32(data + 0x14) != sensor_id ||
	    get_unaligned_le32(data + 0x24) != size ||
	    memcmp(data + 4, "3.05_032401", sizeof("3.05_032401")))
		return -EINVAL;
	for (i = 0; i < ARRAY_SIZE(modules); i++) {
		if (size - offset < modules[i].size ||
		    get_unaligned_le16(data + offset) != i ||
		    get_unaligned_le16(data + offset + 2) != modules[i].size)
			return -EINVAL;
		for (w = 0; w < ARRAY_SIZE(modules[i].writes); w++) {
			const struct tuning_write *write = &modules[i].writes[w];
			if (write->apply && write->offset + write->size > modules[i].size)
				return -EINVAL;
		}
		offset += modules[i].size;
	}
	if (size - offset < 4 || get_unaligned_le16(data + offset) != 28)
		return -EINVAL;
	payload = get_unaligned_le16(data + offset + 2);
	if (!payload || payload % sizeof(AK_ISP_SENSOR_REG_INFO) ||
	    payload / sizeof(AK_ISP_SENSOR_REG_INFO) > MAX_SENSOR_REGISTERS ||
	    size - offset - 4 != payload)
		return -EINVAL;
	return 0;
}

/* Read a whole tuning file from the root filesystem into vmalloc memory. */
static int read_tuning_file(const char *path, void **buf, size_t *len)
{
	struct file *file;
	loff_t size;
	int ret;

	file = filp_open(path, O_RDONLY, 0);
	if (IS_ERR(file))
		return PTR_ERR(file);
	size = i_size_read(file_inode(file));
	if (size < STYLE_HEADER_SIZE || size > MAX_TUNING_SIZE) {
		ret = -EINVAL;
		goto out;
	}
	*buf = vmalloc(size);
	if (!*buf) {
		ret = -ENOMEM;
		goto out;
	}
	ret = kernel_read(file, 0, *buf, size);
	if (ret != size) {
		vfree(*buf);
		*buf = NULL;
		ret = ret < 0 ? ret : -EIO;
		goto out;
	}
	*len = size;
	ret = 0;
out:
	fput(file);
	return ret;
}

/*
 * The tuning comes from <dir>/<sensor name>.conf when the sensor driver
 * registered a name -- /etc/sensor/sc1345.conf, as the other OpenIPC SoCs
 * keep theirs -- and otherwise from the @fallback firmware file.
 */
int ak_isp_tuning_load(struct device *dev, const char *dir,
		      const char *sensor_name, const char *fallback,
		      unsigned int sensor_id, struct ak_isp_tuning *tuning)
{
	const struct firmware *firmware = NULL;
	unsigned int count, size, style;
	const char *name = fallback;
	char path[64];
	void *buf = NULL;
	const u8 *data;
	size_t len;
	int ret = -ENOENT;

	BUILD_BUG_ON(sizeof(AK_ISP_SENSOR_REG_INFO) != 4);
	if (sensor_name && dir && *dir) {
		snprintf(path, sizeof(path), "%s/%s.conf", dir, sensor_name);
		ret = read_tuning_file(path, &buf, &len);
		if (!ret) {
			name = path;
			data = buf;
		} else if (ret != -ENOENT) {
			dev_err(dev, "cannot read %s: %d\n", path, ret);
			return ret;
		}
	}
	if (ret == -ENOENT) {
		ret = request_firmware(&firmware, fallback, dev);
		if (ret) {
			if (sensor_name)
				dev_err(dev, "no ISP tuning: neither %s nor %s\n",
					path, fallback);
			return ret;
		}
		data = firmware->data;
		len = firmware->size;
		if (len < STYLE_HEADER_SIZE || len > MAX_TUNING_SIZE) {
			ret = -EINVAL;
			goto fail;
		}
	}
	count = get_unaligned_le32(data);
	size = get_unaligned_le32(data + 0x24);
	if (!count || size < STYLE_HEADER_SIZE || size > len ||
	    count != len / size || len % size) {
		ret = -EINVAL;
		goto fail;
	}
	for (style = 0; style < count; style++) {
		ret = validate_style(data + style * size, size, sensor_id, count);
		if (ret)
			goto fail;
	}
	tuning->firmware = firmware;
	tuning->buf = buf;
	tuning->data = data;
	tuning->style_size = size;
	tuning->style_count = count;
	dev_info(dev, "ISP tuning %s, %u styles\n", name, count);
	return 0;
fail:
	dev_err(dev, "invalid ISP tuning %s for sensor 0x%x\n", name, sensor_id);
	release_firmware(firmware);
	vfree(buf);
	return ret;
}

int ak_isp_tuning_apply(const struct ak_isp_tuning *tuning, unsigned int style,
		       AK_ISP_SENSOR_CB *sensor, bool initialize_sensor)
{
	const u8 *data;
	unsigned int i, w, offset = STYLE_HEADER_SIZE;
	unsigned int scratch_size = MAX_SENSOR_REGISTERS * sizeof(AK_ISP_SENSOR_REG_INFO);
	AK_ISP_SENSOR_INIT_PARA init;
	void *scratch;
	int ret = 0, fps = 0;

	if (!tuning->data || style >= tuning->style_count)
		return -EINVAL;
	if (!sensor || !sensor->sensor_init_func || !sensor->sensor_set_fps_func)
		return -ENODEV;
	scratch = kmalloc(scratch_size, GFP_KERNEL);
	if (!scratch)
		return -ENOMEM;
	data = tuning->data + style * tuning->style_size;
	for (i = 0; i < ARRAY_SIZE(modules); i++) {
		for (w = 0; w < ARRAY_SIZE(modules[i].writes); w++) {
			const struct tuning_write *write = &modules[i].writes[w];
			if (!write->apply)
				continue;
			if (WARN_ON(write->size > scratch_size)) {
				ret = -EINVAL;
				goto out;
			}
			/* Firmware fields are not necessarily naturally aligned. */
			memcpy(scratch, data + offset + write->offset, write->size);
			ret = write->apply(scratch);
			if (ret)
				goto out;
		}
		if (i == 20)
			fps = get_unaligned_le32(data + offset + 0xc);
		offset += modules[i].size;
	}
	if (initialize_sensor) {
		init.num = get_unaligned_le16(data + offset + 2) / sizeof(AK_ISP_SENSOR_REG_INFO);
		init.reg_info = scratch;
		memcpy(scratch, data + offset + 4, init.num * sizeof(AK_ISP_SENSOR_REG_INFO));
		ret = sensor->sensor_init_func(&init);
		if (ret)
			goto out;
	}
	ret = sensor->sensor_set_fps_func(fps);
	if (!ret && initialize_sensor && sensor->sensor_timer_func)
		ret = sensor->sensor_timer_func();
out:
	kfree(scratch);
	return ret > 0 || ret == -1 ? -EIO : ret;
}

void ak_isp_tuning_free(struct ak_isp_tuning *tuning)
{
	release_firmware(tuning->firmware);
	vfree(tuning->buf);
	memset(tuning, 0, sizeof(*tuning));
}
