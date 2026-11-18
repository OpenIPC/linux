// SPDX-License-Identifier: GPL-2.0
/*
 * Sipeed NanoAgent LT7911D HDMI module management driver.
 *
 * Copyright (c) 2025-2026 Sipeed Technology Co., Ltd.
 */

#include <linux/atomic.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/firmware.h>
#include <linux/gpio/consumer.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/math64.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/poll.h>
#include <linux/proc_fs.h>
#include <linux/regmap.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#define LT7911_PAGE_SELECT			0xff

#define LT7911D_SYS_OFFSET			0x80
#define LT7911D_SYS4_OFFSET			0xa0
#define LT7911D_HDMI_INFO_OFFSET	0xd2
#define LT7911D_AUDIO_INFO_OFFSET	0xd1
#define LT7911D_CSI_INFO_OFFSET		0xc2

#define LT7911EXC_INFO_OFFSET		0xe0
#define LT7911EXC_INT_TYPE			0x84
#define LT7911EXC_INT_VIDEO_OFF		0x00
#define LT7911EXC_INT_VIDEO_READY	0x01
#define LT7911EXC_INT_AUDIO_OFF		0x02
#define LT7911EXC_INT_AUDIO_READY	0x03
#define LT7911EXC_FW_NAME			"Lontium/lt7911exc_fw.bin"
#define LT7911EXC_FW_SIZE			(64 * 1024)
#define LT7911EXC_FW_DATA_SIZE		(LT7911EXC_FW_SIZE - 4)
#define LT7911EXC_FW_PAGE_SIZE		32
#define LT7911EXC_EDID_ADDR		0x60000
#define LT7911EXC_EDID_BLOCK_SIZE	128
#define LT7911EXC_EDID_MAX_SIZE	512

#define LT7911_REG(page, reg)		(((page) << 8) | (reg))

#define EDID_BUFFER_SIZE			LT7911EXC_EDID_MAX_SIZE
#define LT7911D_WR_SIZE				32

#define PROC_LT7911_DIR				"lt7911_info"
#define PROC_CHIP_ID				"chip_id"
#define PROC_VIDEO_STATUS			"status"
#define PROC_VIDEO_WIDTH			"width"
#define PROC_VIDEO_HEIGHT			"height"
#define PROC_VIDEO_PWR				"power"
#define PROC_VIDEO_FPS				"fps"
#define PROC_VIDEO_HDCP				"hdcp"
#define PROC_AUDIO_SAMPLE_RATE		"asr"
#define PROC_VIDEO_EDID				"edid"
#define PROC_VIDEO_EDID_SNAPSHOT	"edid_snapshot"
#define PROC_VERSION				"version"
#define PROC_FW_UPDATE				"fw_update"

enum lt7911_chip {
	LT7911_CHIP_UNKNOWN,
	LT7911_CHIP_LT7911D,
	LT7911_CHIP_LT7911EXC,
};

enum lt7911_res_type {
	NORMAL_RES,
	NEW_RES,
	UNSUPPORT_RES,
	UNKNOWN_RES,
	ERROR_RES,
};

enum lt7911_proc_buffer {
	LT7911_PROC_CHIP_ID,
	LT7911_PROC_STATUS,
	LT7911_PROC_WIDTH,
	LT7911_PROC_HEIGHT,
	LT7911_PROC_POWER,
	LT7911_PROC_FPS,
	LT7911_PROC_HDCP,
	LT7911_PROC_AUDIO_RATE,
	LT7911_PROC_EDID,
	LT7911_PROC_EDID_SNAPSHOT,
	LT7911_PROC_VERSION,
};

struct lt7911_data;

struct lt7911_chip_ops {
	const char *name;
	enum lt7911_chip chip;
	unsigned long irq_flags;
	int (*reset)(struct lt7911_data *lt7911);
	int (*enable)(struct lt7911_data *lt7911);
	int (*disable)(struct lt7911_data *lt7911);
	int (*check_chip)(struct lt7911_data *lt7911);
	void (*handle_irq)(struct lt7911_data *lt7911);
};

struct lt7911_data {
	struct device *dev;
	struct i2c_client *client;
	struct regmap *regmap;
	struct gpio_desc *irq_gpio;
	struct gpio_desc *reset_gpio;
	int irq;
	const struct lt7911_chip_ops *ops;

	bool init_done;
	enum lt7911_chip chip;

	struct mutex state_lock;
	struct mutex exc_lock;
	struct mutex fw_lock;
	wait_queue_head_t wait_queue;
	atomic_t event_seq;

	struct proc_dir_entry *proc_dir;

	char video_status[16];
	char video_width[16];
	char video_height[16];
	char video_power[16];
	char video_fps[16];
	char video_hdcp[16];
	char audio_sample_rate[16];
	u8 video_edid[EDID_BUFFER_SIZE];
	u8 video_edid_snapshot[EDID_BUFFER_SIZE];
	char version[LT7911D_WR_SIZE];
	bool fw_busy;
	int fw_result;
	size_t fw_size;
	u32 fw_crc_expected;
	u32 fw_crc_actual;
	char fw_status[160];
	size_t fw_status_len;

	size_t video_status_len;
	size_t video_width_len;
	size_t video_height_len;
	size_t video_power_len;
	size_t video_fps_len;
	size_t video_hdcp_len;
	size_t audio_sample_rate_len;
	size_t video_edid_len;
	size_t video_edid_snapshot_len;
	size_t version_len;

	u16 last_width;
	u16 last_height;
};

struct lt7911_video_info {
	bool present;
	enum lt7911_res_type res_type;
	u16 width;
	u16 height;
	u16 fps;
};

struct lt7911_status_file {
	struct lt7911_data *lt7911;
	int last_event_seq;
};

static int force_width = -1;
static int force_height = -1;
static int force_fps = -1;

static DEFINE_MUTEX(active_lock);
static struct lt7911_data *active_lt7911;

static const u16 hdmi_res_list[][2] = {
	{3840, 2400}, {3840, 2160}, {3440, 1440}, {2560, 1600},
	{2560, 1440}, {2560, 1080}, {2048, 1536}, {2048, 1152},
	{1920, 1440}, {1920, 1200}, {1920, 1080}, {1680, 1050},
	{1600, 1200}, {1600, 900}, {1440, 1080}, {1440, 900},
	{1440, 1050}, {1368, 768}, {1280, 1024}, {1280, 960},
	{1280, 800}, {1280, 720}, {1152, 864}, {1024, 768},
	{800, 600},
};

static const u16 hdmi_unsupported_res_list[][2] = {
	{1366, 768},
};

static const struct regmap_range_cfg lt7911_ranges[] = {
	{
		.name = "register_range",
		.range_min = 0,
		.range_max = 0xffff,
		.selector_reg = LT7911_PAGE_SELECT,
		.selector_mask = 0xff,
		.selector_shift = 0,
		.window_start = 0,
		.window_len = 0x100,
	},
};

static const struct regmap_config lt7911_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = 0xffff,
	.cache_type = REGCACHE_NONE,
	.ranges = lt7911_ranges,
	.num_ranges = ARRAY_SIZE(lt7911_ranges),
};

static void lt7911_notify(struct lt7911_data *lt7911)
{
	atomic_inc(&lt7911->event_seq);
	wake_up_interruptible(&lt7911->wait_queue);
}

static int lt7911_write(struct lt7911_data *lt7911, u8 page, u8 reg, u8 val)
{
	return regmap_write(lt7911->regmap, LT7911_REG(page, reg), val);
}

static int lt7911_read(struct lt7911_data *lt7911, u8 page, u8 reg, u8 *val)
{
	unsigned int tmp;
	int ret;

	ret = regmap_read(lt7911->regmap, LT7911_REG(page, reg), &tmp);
	if (ret)
		return ret;

	*val = tmp;
	return 0;
}

static int lt7911_bulk_read(struct lt7911_data *lt7911, u8 page, u8 reg,
			    void *val, size_t len)
{
	return regmap_bulk_read(lt7911->regmap, LT7911_REG(page, reg), val,
				len);
}

static int lt7911_power_set(struct lt7911_data *lt7911, bool on)
{
	gpiod_set_value_cansleep(lt7911->reset_gpio, on);
	msleep(on ? 400 : 20);

	mutex_lock(&lt7911->state_lock);
	lt7911->video_power_len = scnprintf(lt7911->video_power,
					    sizeof(lt7911->video_power),
					    on ? "on\n" : "off\n");
	mutex_unlock(&lt7911->state_lock);

	return 0;
}

static int lt7911_reset_sequence(struct lt7911_data *lt7911)
{
	gpiod_set_value_cansleep(lt7911->reset_gpio, 0);
	msleep(20);

	gpiod_set_value_cansleep(lt7911->reset_gpio, 1);
	msleep(400);

	mutex_lock(&lt7911->state_lock);
	lt7911->video_power_len = scnprintf(lt7911->video_power,
					    sizeof(lt7911->video_power),
					    "on\n");
	mutex_unlock(&lt7911->state_lock);

	return 0;
}

static int lt7911d_reset(struct lt7911_data *lt7911)
{
	return lt7911_reset_sequence(lt7911);
}

static int lt7911exc_reset(struct lt7911_data *lt7911)
{
	return lt7911_reset_sequence(lt7911);
}

static int lt7911d_enable(struct lt7911_data *lt7911)
{
	return lt7911_write(lt7911, LT7911D_SYS_OFFSET, 0xee, 0x01);
}

static int lt7911d_disable(struct lt7911_data *lt7911)
{
	return lt7911_write(lt7911, LT7911D_SYS_OFFSET, 0xee, 0x00);
}

static int lt7911_noop(struct lt7911_data *lt7911)
{
	return 0;
}

static int lt7911d_check_chip(struct lt7911_data *lt7911)
{
	u8 chip_id[2];
	int ret;

	ret = lt7911d_enable(lt7911);
	if (ret) {
		dev_err(lt7911->dev, "Failed to enable LT7911D: %d\n", ret);
		return ret;
	}

	ret = lt7911_bulk_read(lt7911, LT7911D_SYS4_OFFSET, 0x00, chip_id,
			       sizeof(chip_id));
	if (ret) {
		dev_err(lt7911->dev, "Failed to read chip id: %d\n", ret);
		return ret;
	}

	if (chip_id[0] == 0x16 && chip_id[1] == 0x05) {
		lt7911->chip = LT7911_CHIP_LT7911D;
		dev_info(lt7911->dev, "Chip: LT7911D\n");
		return 0;
	}

	lt7911->chip = LT7911_CHIP_UNKNOWN;
	return -ENODEV;
}

static int lt7911exc_check_chip(struct lt7911_data *lt7911)
{
	u8 fw_version[3];
	int ret;

	ret = lt7911_bulk_read(lt7911, 0xe0, 0x81, fw_version,
			       sizeof(fw_version));
	if (ret) {
		dev_err(lt7911->dev, "Failed to read LT7911EXC version: %d\n",
			ret);
		return ret;
	}

	lt7911->chip = LT7911_CHIP_LT7911EXC;
	dev_info(lt7911->dev, "Chip: LT7911EXC fw version %u.%u.%u\n",
		 fw_version[0], fw_version[1], fw_version[2]);

	return 0;
}

static void lt7911d_handle_irq(struct lt7911_data *lt7911);
static void lt7911exc_handle_irq(struct lt7911_data *lt7911);

static const struct lt7911_chip_ops lt7911d_ops = {
	.name = "LT7911D",
	.chip = LT7911_CHIP_LT7911D,
	.irq_flags = IRQF_TRIGGER_RISING,
	.reset = lt7911d_reset,
	.enable = lt7911d_enable,
	.disable = lt7911d_disable,
	.check_chip = lt7911d_check_chip,
	.handle_irq = lt7911d_handle_irq,
};

static const struct lt7911_chip_ops lt7911exc_ops = {
	.name = "LT7911EXC",
	.chip = LT7911_CHIP_LT7911EXC,
	.irq_flags = IRQF_TRIGGER_FALLING,
	.reset = lt7911exc_reset,
	.enable = lt7911_noop,
	.disable = lt7911_noop,
	.check_chip = lt7911exc_check_chip,
	.handle_irq = lt7911exc_handle_irq,
};

static int lt7911_reset(struct lt7911_data *lt7911)
{
	if (!lt7911->ops || !lt7911->ops->reset)
		return -ENODEV;

	return lt7911->ops->reset(lt7911);
}

static int lt7911_enable(struct lt7911_data *lt7911)
{
	if (!lt7911->ops || !lt7911->ops->enable)
		return -ENODEV;

	return lt7911->ops->enable(lt7911);
}

static int lt7911_disable(struct lt7911_data *lt7911)
{
	if (!lt7911->ops || !lt7911->ops->disable)
		return -ENODEV;

	return lt7911->ops->disable(lt7911);
}

static int lt7911_check_chip(struct lt7911_data *lt7911)
{
	if (!lt7911->ops || !lt7911->ops->check_chip)
		return -ENODEV;

	return lt7911->ops->check_chip(lt7911);
}

static void lt7911_fw_status_update_locked(struct lt7911_data *lt7911,
					   const char *state)
{
	lt7911->fw_status_len =
		scnprintf(lt7911->fw_status, sizeof(lt7911->fw_status),
			  "state=%s result=%d size=%zu crc_expected=0x%08x crc_actual=0x%08x firmware=%s\n",
			  state, lt7911->fw_result, lt7911->fw_size,
			  lt7911->fw_crc_expected, lt7911->fw_crc_actual,
			  LT7911EXC_FW_NAME);
}

static void lt7911_fw_status_update(struct lt7911_data *lt7911,
				    const char *state)
{
	mutex_lock(&lt7911->fw_lock);
	lt7911_fw_status_update_locked(lt7911, state);
	mutex_unlock(&lt7911->fw_lock);
}

static u32 lt7911_fw_crc32_be_byte(u32 crc, u8 data)
{
	int i;

	crc ^= (u32)data << 24;
	for (i = 0; i < 8; i++) {
		if (crc & 0x80000000)
			crc = (crc << 1) ^ 0x04c11db7;
		else
			crc <<= 1;
	}

	return crc;
}

static u32 lt7911_fw_crc32_custom(const u8 *data, size_t len)
{
	u32 crc = 0xffffffff;
	size_t i;

	for (i = 0; i < len; i += 4) {
		crc = lt7911_fw_crc32_be_byte(crc, data[i + 3]);
		crc = lt7911_fw_crc32_be_byte(crc, data[i + 2]);
		crc = lt7911_fw_crc32_be_byte(crc, data[i + 1]);
		crc = lt7911_fw_crc32_be_byte(crc, data[i + 0]);
	}

	return crc;
}

static int lt7911_fw_prepare_crc(const struct firmware *fw, u32 *crc)
{
	u8 *buf;

	buf = kvzalloc(LT7911EXC_FW_DATA_SIZE, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	memset(buf, 0xff, LT7911EXC_FW_DATA_SIZE);
	memcpy(buf, fw->data, fw->size);
	*crc = lt7911_fw_crc32_custom(buf, LT7911EXC_FW_DATA_SIZE);
	kvfree(buf);

	return 0;
}

static int lt7911exc_block_erase_fw(struct lt7911_data *lt7911)
{
	static const struct reg_sequence seq[] = {
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0xee), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x54), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x55), 0x06 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x00 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x54), 0x05 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x55), 0xd8 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5a), 0x00 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5b), 0x00 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5c), 0x00 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x00 },
	};
	int ret;

	ret = regmap_multi_reg_write(lt7911->regmap, seq, ARRAY_SIZE(seq));
	if (ret)
		return ret;

	msleep(200);
	return 0;
}

static int lt7911exc_block_erase_at(struct lt7911_data *lt7911, u32 addr)
{
	struct reg_sequence seq[] = {
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0xee), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x54), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x55), 0x06 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x00 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x54), 0x05 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x55), 0xd8 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5a),
		  (addr >> 16) & 0xff },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5b),
		  (addr >> 8) & 0xff },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5c), addr & 0xff },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x51), 0x00 },
	};
	int ret;

	ret = regmap_multi_reg_write(lt7911->regmap, seq, ARRAY_SIZE(seq));
	if (ret)
		return ret;

	msleep(200);
	return 0;
}

static int lt7911exc_prog_init_fw(struct lt7911_data *lt7911, u32 addr)
{
	struct reg_sequence seq[] = {
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0xee), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5f), 0x01 },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5a),
		  (addr >> 16) & 0xff },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5b),
		  (addr >> 8) & 0xff },
		{ LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5c), addr & 0xff },
	};

	return regmap_multi_reg_write(lt7911->regmap, seq, ARRAY_SIZE(seq));
}

static int lt7911exc_write_data_at(struct lt7911_data *lt7911, u32 addr,
				   const u8 *data, size_t len)
{
	size_t pages = (len + LT7911EXC_FW_PAGE_SIZE - 1) /
		       LT7911EXC_FW_PAGE_SIZE;
	size_t num;
	int ret;

	for (num = 0; num < pages; num++) {
		size_t offset = num * LT7911EXC_FW_PAGE_SIZE;
		size_t page_len = len - offset;

		if (page_len > LT7911EXC_FW_PAGE_SIZE)
			page_len = LT7911EXC_FW_PAGE_SIZE;

		ret = lt7911exc_prog_init_fw(lt7911, addr);
		if (ret)
			return ret;

		ret = regmap_raw_write(lt7911->regmap,
				       LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5d),
				       &data[offset], page_len);
		if (ret) {
			dev_err(lt7911->dev, "flash write error at page %zu\n",
				num);
			return ret;
		}

		if (page_len < LT7911EXC_FW_PAGE_SIZE) {
			ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET,
					   0x5f, 0x05);
			if (ret)
				return ret;
			ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET,
					   0x5f, 0x01);
			if (ret)
				return ret;
			usleep_range(1000, 2000);
		}

		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5f, 0x00);
		if (ret)
			return ret;

		addr += LT7911EXC_FW_PAGE_SIZE;
	}

	return 0;
}

static int lt7911exc_write_data_fw(struct lt7911_data *lt7911, const u8 *fw,
				   size_t fw_size)
{
	return lt7911exc_write_data_at(lt7911, 0, fw, fw_size);
}

static int lt7911exc_reset_fifo(struct lt7911_data *lt7911)
{
	int ret;

	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5f, 0x08);
	if (ret)
		return ret;
	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5f, 0x00);
	if (ret)
		return ret;

	msleep(1000);
	return 0;
}

static int lt7911exc_read_data_at(struct lt7911_data *lt7911, u32 addr,
				  u8 *data, size_t len)
{
	size_t offset;
	int ret;

	ret = lt7911exc_reset_fifo(lt7911);
	if (ret)
		return ret;

	for (offset = 0; offset < len; offset += LT7911EXC_FW_PAGE_SIZE) {
		size_t page_len = len - offset;
		u32 cur_addr = addr + offset;

		if (page_len > LT7911EXC_FW_PAGE_SIZE)
			page_len = LT7911EXC_FW_PAGE_SIZE;

		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x54, 0x45);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x55, 0x03);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x56,
				   page_len);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x57, 0x00);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x58, 0x00);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5a,
				   (cur_addr >> 16) & 0xff);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5b,
				   (cur_addr >> 8) & 0xff);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5c,
				   cur_addr & 0xff);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x51, 0x01);
		if (ret)
			return ret;
		ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x51, 0x00);
		if (ret)
			return ret;

		ret = lt7911_bulk_read(lt7911, LT7911EXC_INFO_OFFSET, 0x5e,
				       &data[offset], page_len);
		if (ret)
			return ret;
	}

	return 0;
}

static int lt7911exc_write_crc_fw(struct lt7911_data *lt7911, u32 crc32)
{
	const u32 addr = LT7911EXC_FW_SIZE - 4;
	u8 crc[4];
	int ret;

	crc[0] = crc32 & 0xff;
	crc[1] = (crc32 >> 8) & 0xff;
	crc[2] = (crc32 >> 16) & 0xff;
	crc[3] = (crc32 >> 24) & 0xff;

	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5f, 0x01);
	if (ret)
		return ret;
	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5a,
			   (addr >> 16) & 0xff);
	if (ret)
		return ret;
	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5b,
			   (addr >> 8) & 0xff);
	if (ret)
		return ret;
	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5c, addr & 0xff);
	if (ret)
		return ret;

	ret = regmap_raw_write(lt7911->regmap,
			       LT7911_REG(LT7911EXC_INFO_OFFSET, 0x5d),
			       crc, sizeof(crc));
	if (ret)
		return ret;

	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5f, 0x05);
	if (ret)
		return ret;
	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5f, 0x01);
	if (ret)
		return ret;
	usleep_range(1000, 2000);

	return lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x5f, 0x00);
}

static int lt7911exc_verify_crc_fw(struct lt7911_data *lt7911, u32 expected,
				   u32 *actual)
{
	u8 crc_tmp[4];
	int ret;

	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0xee, 0x01);
	if (ret)
		return ret;
	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x7b, 0x60);
	if (ret)
		return ret;
	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0x7b, 0x40);
	if (ret)
		return ret;

	msleep(150);

	ret = lt7911_bulk_read(lt7911, 0x00, 0x22, crc_tmp, sizeof(crc_tmp));
	if (ret)
		return ret;

	*actual = ((u32)crc_tmp[0] << 24) | ((u32)crc_tmp[1] << 16) |
		  ((u32)crc_tmp[2] << 8) | crc_tmp[3];

	return *actual == expected ? 0 : -EIO;
}

static int lt7911exc_read_edid(struct lt7911_data *lt7911, u8 *edid)
{
	int ret;
	int unlock_ret;

	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0xee, 0x01);
	if (ret)
		return ret;

	ret = lt7911exc_read_data_at(lt7911, LT7911EXC_EDID_ADDR, edid,
				     LT7911EXC_EDID_MAX_SIZE);

	unlock_ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0xee, 0x00);
	return ret ? ret : unlock_ret;
}

static int lt7911exc_write_edid(struct lt7911_data *lt7911, const u8 *edid,
				size_t len)
{
	u8 readback[LT7911EXC_EDID_MAX_SIZE];
	int ret;
	int unlock_ret;

	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0xee, 0x01);
	if (ret)
		return ret;

	ret = lt7911exc_block_erase_at(lt7911, LT7911EXC_EDID_ADDR);
	if (ret)
		goto out_unlock;

	ret = lt7911exc_write_data_at(lt7911, LT7911EXC_EDID_ADDR, edid, len);
	if (ret)
		goto out_unlock;

	ret = lt7911exc_read_data_at(lt7911, LT7911EXC_EDID_ADDR, readback,
				     len);
	if (ret)
		goto out_unlock;

	if (memcmp(readback, edid, len))
		ret = -EIO;

out_unlock:
	unlock_ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0xee, 0x00);
	if (ret)
		return ret;
	if (unlock_ret)
		return unlock_ret;

	return lt7911_reset(lt7911);
}

static int lt7911exc_run_fw_update(struct lt7911_data *lt7911,
				   const struct firmware *fw,
				   u32 expected_crc, u32 *actual_crc)
{
	int ret;
	int unlock_ret;

	ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0xee, 0x01);
	if (ret)
		goto out_unlock;

	ret = lt7911exc_block_erase_fw(lt7911);
	if (ret)
		goto out_unlock;

	ret = lt7911exc_write_data_fw(lt7911, fw->data, fw->size);
	if (ret)
		goto out_unlock;

	ret = lt7911exc_write_crc_fw(lt7911, expected_crc);
	if (ret)
		goto out_unlock;

	ret = lt7911_reset(lt7911);
	if (ret)
		goto out_unlock;

	ret = lt7911exc_verify_crc_fw(lt7911, expected_crc, actual_crc);

out_unlock:
	unlock_ret = lt7911_write(lt7911, LT7911EXC_INFO_OFFSET, 0xee, 0x00);
	return ret ? ret : unlock_ret;
}

static int lt7911_fw_update_run(struct lt7911_data *lt7911)
{
	const struct firmware *fw = NULL;
	size_t fw_size = 0;
	u32 expected_crc = 0;
	u32 actual_crc = 0;
	int ret;

	ret = request_firmware(&fw, LT7911EXC_FW_NAME, lt7911->dev);
	if (ret) {
		dev_err(lt7911->dev, "failed to request firmware %s: %d\n",
			LT7911EXC_FW_NAME, ret);
		goto out_status;
	}

	fw_size = fw->size;
	if (!fw->size || fw->size > LT7911EXC_FW_DATA_SIZE) {
		dev_err(lt7911->dev, "invalid firmware size: %zu\n", fw->size);
		ret = -EINVAL;
		goto out_release;
	}

	ret = lt7911_fw_prepare_crc(fw, &expected_crc);
	if (ret)
		goto out_release;

	mutex_lock(&lt7911->fw_lock);
	lt7911->fw_size = fw_size;
	lt7911->fw_crc_expected = expected_crc;
	lt7911->fw_crc_actual = 0;
	lt7911->fw_result = 0;
	lt7911_fw_status_update_locked(lt7911, "busy");
	mutex_unlock(&lt7911->fw_lock);

	disable_irq(lt7911->irq);
	mutex_lock(&lt7911->exc_lock);
	ret = lt7911exc_run_fw_update(lt7911, fw, expected_crc, &actual_crc);
	mutex_unlock(&lt7911->exc_lock);
	enable_irq(lt7911->irq);

out_release:
	release_firmware(fw);

out_status:
	mutex_lock(&lt7911->fw_lock);
	lt7911->fw_busy = false;
	lt7911->fw_result = ret;
	lt7911->fw_size = fw_size;
	lt7911->fw_crc_expected = expected_crc;
	lt7911->fw_crc_actual = actual_crc;
	lt7911_fw_status_update_locked(lt7911, ret ? "error" : "done");
	mutex_unlock(&lt7911->fw_lock);

	return ret;
}

static int lt7911_get_signal_state(struct lt7911_data *lt7911, u8 *state)
{
	u8 vactive[2];
	u8 hactive[2];
	u16 vactive_val;
	u16 hactive_val;
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911D)
		return -ENODEV;

	ret = lt7911_write(lt7911, LT7911D_HDMI_INFO_OFFSET, 0x83, 0x10);
	if (ret)
		return ret;

	msleep(5);

	ret = lt7911_bulk_read(lt7911, LT7911D_HDMI_INFO_OFFSET, 0x96,
			       vactive, sizeof(vactive));
	if (ret)
		return ret;

	ret = lt7911_bulk_read(lt7911, LT7911D_HDMI_INFO_OFFSET, 0x8b,
			       hactive, sizeof(hactive));
	if (ret)
		return ret;

	vactive_val = (vactive[0] << 8) | vactive[1];
	hactive_val = (hactive[0] << 8) | hactive[1];

	*state = 0;
	if (vactive_val && hactive_val)
		*state |= 0x01;

	*state |= 0x02;
	return 0;
}

static enum lt7911_res_type lt7911_check_res(struct lt7911_data *lt7911,
					     u16 width, u16 height)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(hdmi_res_list); i++) {
		if (width == hdmi_res_list[i][0] &&
		    height == hdmi_res_list[i][1]) {
			if (lt7911->last_width != width ||
			    lt7911->last_height != height) {
				lt7911->last_width = width;
				lt7911->last_height = height;
				return NEW_RES;
			}
			return NORMAL_RES;
		}
	}

	for (i = 0; i < ARRAY_SIZE(hdmi_unsupported_res_list); i++) {
		if (width == hdmi_unsupported_res_list[i][0] &&
		    height == hdmi_unsupported_res_list[i][1])
			return UNSUPPORT_RES;
	}

	return UNKNOWN_RES;
}

static int lt7911_get_csi_res(struct lt7911_data *lt7911, u16 *width,
			      u16 *height)
{
	u8 height_buf[2];
	u8 width_buf[2];
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911D)
		return -ENODEV;

	ret = lt7911_bulk_read(lt7911, LT7911D_CSI_INFO_OFFSET, 0x06,
			       height_buf, sizeof(height_buf));
	if (ret)
		return ret;

	ret = lt7911_bulk_read(lt7911, LT7911D_CSI_INFO_OFFSET, 0x38,
			       width_buf, sizeof(width_buf));
	if (ret)
		return ret;

	*height = (height_buf[0] << 8) | height_buf[1];
	*width = (width_buf[0] << 8) | width_buf[1];

	return lt7911_check_res(lt7911, *width, *height);
}

static int lt7911_get_csi_fps(struct lt7911_data *lt7911, u16 *fps)
{
	u8 htotal_buf[2];
	u8 vtotal_buf[2];
	u8 clk_buf[3];
	u32 htotal;
	u32 vtotal;
	u32 clk;
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911D)
		return -ENODEV;

	ret = lt7911_write(lt7911, LT7911D_HDMI_INFO_OFFSET, 0x83, 0x10);
	if (ret)
		return ret;

	ret = lt7911_bulk_read(lt7911, LT7911D_HDMI_INFO_OFFSET, 0x89,
			       htotal_buf, sizeof(htotal_buf));
	if (ret)
		return ret;

	ret = lt7911_bulk_read(lt7911, LT7911D_HDMI_INFO_OFFSET, 0x9e,
			       vtotal_buf, sizeof(vtotal_buf));
	if (ret)
		return ret;

	htotal = ((htotal_buf[0] << 8) | htotal_buf[1]) * 2;
	vtotal = (vtotal_buf[0] << 8) | vtotal_buf[1];
	if (!htotal || !vtotal)
		return -EINVAL;

	dev_info(lt7911->dev, "HDMI HTotal: %u, VTotal: %u\n", htotal,
		 vtotal);
	msleep(20);

	ret = lt7911_write(lt7911, LT7911D_SYS4_OFFSET, 0x34, 0x21);
	if (ret)
		return ret;

	msleep(10);

	ret = lt7911_bulk_read(lt7911, 0xb8, 0xb1, clk_buf, sizeof(clk_buf));
	if (ret)
		return ret;

	clk = ((clk_buf[0] & 0x07) << 16) | (clk_buf[1] << 8) | clk_buf[2];
	*fps = (u16)((clk * 2000) / (htotal * vtotal));

	return 0;
}

static int lt7911_get_audio_sample_rate(struct lt7911_data *lt7911,
					u8 *sample_rate)
{
	if (lt7911->chip != LT7911_CHIP_LT7911D)
		return -ENODEV;

	return lt7911_read(lt7911, LT7911D_AUDIO_INFO_OFFSET, 0x55,
			   sample_rate);
}

static int lt7911exc_get_int_type(struct lt7911_data *lt7911, u8 *int_type)
{
	if (lt7911->chip != LT7911_CHIP_LT7911EXC)
		return -ENODEV;

	return lt7911_read(lt7911, LT7911EXC_INFO_OFFSET, LT7911EXC_INT_TYPE,
			   int_type);
}

static int lt7911exc_get_video_info(struct lt7911_data *lt7911, u16 *width,
				    u16 *height, u16 *fps)
{
	u8 buf[11];
	u32 pixel_clock;
	u32 htotal;
	u32 vtotal;
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911EXC)
		return -ENODEV;

	ret = lt7911_bulk_read(lt7911, LT7911EXC_INFO_OFFSET, 0x85, buf,
			       sizeof(buf));
	if (ret)
		return ret;

	pixel_clock = (buf[0] << 16) | (buf[1] << 8) | buf[2];
	htotal = (buf[3] << 8) | buf[4];
	vtotal = (buf[5] << 8) | buf[6];
	*width = (buf[7] << 8) | buf[8];
	*height = (buf[9] << 8) | buf[10];

	if (pixel_clock && htotal && vtotal)
		*fps = (u16)div_u64((u64)pixel_clock * 1000,
				     htotal * vtotal);
	else
		*fps = 0;

	dev_info(lt7911->dev, "LT7911EXC HTotal: %u, VTotal: %u\n", htotal,
		 vtotal);

	return lt7911_check_res(lt7911, *width, *height);
}

static int lt7911exc_get_audio_sample_rate(struct lt7911_data *lt7911,
					   u8 *sample_rate)
{
	if (lt7911->chip != LT7911_CHIP_LT7911EXC)
		return -ENODEV;

	return lt7911_read(lt7911, LT7911EXC_INFO_OFFSET, 0xad, sample_rate);
}

static int lt7911_str_write(struct lt7911_data *lt7911, const u8 *str,
			    size_t len)
{
	if (len > LT7911D_WR_SIZE)
		return -EINVAL;

	if (lt7911->chip == LT7911_CHIP_LT7911D)
		return -EOPNOTSUPP;

	return -ENODEV;
}

static int lt7911_str_read(struct lt7911_data *lt7911, u8 *str)
{
	if (lt7911->chip == LT7911_CHIP_LT7911D)
		return -EOPNOTSUPP;

	return -ENODEV;
}

static void lt7911_proc_buffer_init(struct lt7911_data *lt7911)
{
	mutex_lock(&lt7911->state_lock);
	lt7911->video_status_len = scnprintf(lt7911->video_status,
					     sizeof(lt7911->video_status),
					     "disappear\n");
	lt7911->video_width_len = scnprintf(lt7911->video_width,
					    sizeof(lt7911->video_width),
					    "0\n");
	lt7911->video_height_len = scnprintf(lt7911->video_height,
					     sizeof(lt7911->video_height),
					     "0\n");
	lt7911->video_power_len = scnprintf(lt7911->video_power,
					    sizeof(lt7911->video_power),
					    "off\n");
	lt7911->video_fps_len = scnprintf(lt7911->video_fps,
					  sizeof(lt7911->video_fps), "0\n");
	lt7911->video_hdcp_len = scnprintf(lt7911->video_hdcp,
					   sizeof(lt7911->video_hdcp),
					   "unknown hdcp\n");
	lt7911->audio_sample_rate_len = scnprintf(lt7911->audio_sample_rate,
						  sizeof(lt7911->audio_sample_rate),
						  "disappear\n");
	lt7911->video_edid_len = 0;
	lt7911->video_edid_snapshot_len = 0;
	lt7911->version_len = 0;
	mutex_unlock(&lt7911->state_lock);
}

static void lt7911_force_resolution(struct lt7911_data *lt7911, u16 width,
				    u16 height, int fps)
{
	mutex_lock(&lt7911->state_lock);
	lt7911->video_status_len = scnprintf(lt7911->video_status,
					     sizeof(lt7911->video_status),
					     "new res\n");
	lt7911->video_width_len = scnprintf(lt7911->video_width,
					    sizeof(lt7911->video_width),
					    "%u\n", width);
	lt7911->video_height_len = scnprintf(lt7911->video_height,
					     sizeof(lt7911->video_height),
					     "%u\n", height);
	lt7911->video_fps_len = scnprintf(lt7911->video_fps,
					  sizeof(lt7911->video_fps), "%d\n",
					  fps > 0 ? fps : 30);
	lt7911->video_hdcp_len = scnprintf(lt7911->video_hdcp,
					   sizeof(lt7911->video_hdcp),
					   "unknown hdcp\n");
	mutex_unlock(&lt7911->state_lock);

	lt7911_notify(lt7911);
}

static void lt7911_update_status(struct lt7911_data *lt7911)
{
	bool notify = false;

	mutex_lock(&lt7911->state_lock);
	if (lt7911->video_status_len > 0 &&
	    (!strncmp(lt7911->video_status, "new res", 7) ||
	     !strncmp(lt7911->video_status, "normal res", 10) ||
	     !strncmp(lt7911->video_status, "unsupport res", 13) ||
	     !strncmp(lt7911->video_status, "unknown res", 11) ||
	     !strncmp(lt7911->video_status, "error res", 9))) {
		lt7911->video_status_len = scnprintf(lt7911->video_status,
						     sizeof(lt7911->video_status),
						     "stable\n");
		notify = true;
	}
	mutex_unlock(&lt7911->state_lock);

	if (notify)
		lt7911_notify(lt7911);
}

static ssize_t lt7911_read_proc_buffer(struct lt7911_data *lt7911,
				       enum lt7911_proc_buffer id,
				       char __user *user_buffer, size_t count,
				       loff_t *offset)
{
	u8 tmp[EDID_BUFFER_SIZE];
	size_t len = 0;

	mutex_lock(&lt7911->state_lock);
	switch (id) {
	case LT7911_PROC_CHIP_ID:
		switch (lt7911->chip) {
		case LT7911_CHIP_LT7911D:
			len = scnprintf((char *)tmp, sizeof(tmp), "lt7911d\n");
			break;
		case LT7911_CHIP_LT7911EXC:
			len = scnprintf((char *)tmp, sizeof(tmp),
					"lt7911exc\n");
			break;
		default:
			len = scnprintf((char *)tmp, sizeof(tmp), "unknown\n");
			break;
		}
		break;
	case LT7911_PROC_STATUS:
		len = lt7911->video_status_len;
		memcpy(tmp, lt7911->video_status, len);
		break;
	case LT7911_PROC_WIDTH:
		len = lt7911->video_width_len;
		memcpy(tmp, lt7911->video_width, len);
		break;
	case LT7911_PROC_HEIGHT:
		len = lt7911->video_height_len;
		memcpy(tmp, lt7911->video_height, len);
		break;
	case LT7911_PROC_POWER:
		len = lt7911->video_power_len;
		memcpy(tmp, lt7911->video_power, len);
		break;
	case LT7911_PROC_FPS:
		len = lt7911->video_fps_len;
		memcpy(tmp, lt7911->video_fps, len);
		break;
	case LT7911_PROC_HDCP:
		len = lt7911->video_hdcp_len;
		memcpy(tmp, lt7911->video_hdcp, len);
		break;
	case LT7911_PROC_AUDIO_RATE:
		len = lt7911->audio_sample_rate_len;
		memcpy(tmp, lt7911->audio_sample_rate, len);
		break;
	case LT7911_PROC_EDID:
		len = lt7911->video_edid_len;
		memcpy(tmp, lt7911->video_edid, len);
		break;
	case LT7911_PROC_EDID_SNAPSHOT:
		if (!lt7911->video_edid_snapshot_len)
			lt7911->video_edid_snapshot_len =
				scnprintf((char *)lt7911->video_edid_snapshot,
					  sizeof(lt7911->video_edid_snapshot),
					  "unknown\n");
		len = lt7911->video_edid_snapshot_len;
		memcpy(tmp, lt7911->video_edid_snapshot, len);
		break;
	case LT7911_PROC_VERSION:
		if (!lt7911->version_len)
			lt7911->version_len =
				scnprintf(lt7911->version,
					  sizeof(lt7911->version),
					  "unknown\n");
		len = lt7911->version_len;
		memcpy(tmp, lt7911->version, len);
		break;
	}
	mutex_unlock(&lt7911->state_lock);

	return simple_read_from_buffer(user_buffer, count, offset, tmp, len);
}

static ssize_t proc_video_power_read(struct file *file,
				     char __user *user_buffer, size_t count,
				     loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_POWER, user_buffer, count,
				       offset);
}

static ssize_t proc_chip_id_read(struct file *file, char __user *user_buffer,
				 size_t count, loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_CHIP_ID, user_buffer,
				       count, offset);
}

static ssize_t proc_video_power_write(struct file *file,
				      const char __user *user_buffer,
				      size_t count, loff_t *offset)
{
	struct lt7911_data *lt7911 = PDE_DATA(file_inode(file));
	char buf[16];

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	if (!strncmp(buf, "on", 2) || !strncmp(buf, "1", 1)) {
		lt7911_power_set(lt7911, true);
	} else if (!strncmp(buf, "off", 3) || !strncmp(buf, "0", 1)) {
		lt7911_power_set(lt7911, false);
		mutex_lock(&lt7911->state_lock);
		lt7911->video_status_len = scnprintf(lt7911->video_status,
						     sizeof(lt7911->video_status),
						     "disappear\n");
		mutex_unlock(&lt7911->state_lock);
		lt7911_notify(lt7911);
	} else {
		return -EINVAL;
	}

	return count;
}

static int proc_status_open(struct inode *inode, struct file *file)
{
	struct lt7911_status_file *status;
	struct lt7911_data *lt7911 = PDE_DATA(inode);

	status = kzalloc(sizeof(*status), GFP_KERNEL);
	if (!status)
		return -ENOMEM;

	status->lt7911 = lt7911;
	status->last_event_seq = atomic_read(&lt7911->event_seq);
	file->private_data = status;

	return 0;
}

static int proc_status_release(struct inode *inode, struct file *file)
{
	kfree(file->private_data);
	return 0;
}

static ssize_t proc_video_status_read(struct file *file,
				      char __user *user_buffer, size_t count,
				      loff_t *offset)
{
	struct lt7911_status_file *status = file->private_data;
	ssize_t ret;

	ret = lt7911_read_proc_buffer(status->lt7911, LT7911_PROC_STATUS,
				      user_buffer, count, offset);
	if (ret > 0)
		status->last_event_seq =
			atomic_read(&status->lt7911->event_seq);

	return ret;
}

static ssize_t proc_video_status_write(struct file *file,
				       const char __user *user_buffer,
				       size_t count, loff_t *offset)
{
	struct lt7911_status_file *status = file->private_data;
	char buf[16];

	if (count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buffer, count))
		return -EFAULT;
	buf[count] = '\0';

	if (!strncmp(buf, "ok", 2))
		lt7911_update_status(status->lt7911);

	return count;
}

static unsigned int proc_video_status_poll(struct file *file, poll_table *wait)
{
	struct lt7911_status_file *status = file->private_data;
	struct lt7911_data *lt7911 = status->lt7911;
	unsigned int mask = 0;

	poll_wait(file, &lt7911->wait_queue, wait);
	if (atomic_read(&lt7911->event_seq) != status->last_event_seq)
		mask |= POLLPRI;

	return mask;
}

static ssize_t proc_video_width_read(struct file *file,
				     char __user *user_buffer, size_t count,
				     loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_WIDTH, user_buffer, count,
				       offset);
}

static ssize_t proc_video_height_read(struct file *file,
				      char __user *user_buffer, size_t count,
				      loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_HEIGHT, user_buffer, count,
				       offset);
}

static ssize_t proc_video_fps_read(struct file *file,
				   char __user *user_buffer, size_t count,
				   loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_FPS, user_buffer, count,
				       offset);
}

static ssize_t proc_video_hdcp_read(struct file *file,
				    char __user *user_buffer, size_t count,
				    loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_HDCP, user_buffer, count,
				       offset);
}

static ssize_t proc_audio_sample_rate_read(struct file *file,
					   char __user *user_buffer,
					   size_t count, loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_AUDIO_RATE, user_buffer,
				       count, offset);
}

static bool lt7911_edid_checksum_valid(const u8 *edid, size_t len)
{
	size_t block;

	if (len < LT7911EXC_EDID_BLOCK_SIZE ||
	    len > LT7911EXC_EDID_MAX_SIZE ||
	    len % LT7911EXC_EDID_BLOCK_SIZE)
		return false;

	for (block = 0; block < len; block += LT7911EXC_EDID_BLOCK_SIZE) {
		u8 sum = 0;
		size_t i;

		for (i = 0; i < LT7911EXC_EDID_BLOCK_SIZE; i++)
			sum += edid[block + i];

		if (sum)
			return false;
	}

	return true;
}

static ssize_t proc_video_edid_read(struct file *file,
				    char __user *user_buffer, size_t count,
				    loff_t *offset)
{
	struct lt7911_data *lt7911 = PDE_DATA(file_inode(file));
	u8 edid[LT7911EXC_EDID_MAX_SIZE];
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911EXC)
		return -EOPNOTSUPP;

	disable_irq(lt7911->irq);
	mutex_lock(&lt7911->exc_lock);
	ret = lt7911exc_read_edid(lt7911, edid);
	mutex_unlock(&lt7911->exc_lock);
	enable_irq(lt7911->irq);
	if (ret)
		return ret;

	mutex_lock(&lt7911->state_lock);
	memcpy(lt7911->video_edid, edid, sizeof(edid));
	lt7911->video_edid_len = sizeof(edid);
	mutex_unlock(&lt7911->state_lock);

	return simple_read_from_buffer(user_buffer, count, offset, edid,
				       sizeof(edid));
}

static ssize_t proc_video_edid_write(struct file *file,
				     const char __user *user_buffer,
				     size_t count, loff_t *offset)
{
	struct lt7911_data *lt7911 = PDE_DATA(file_inode(file));
	u8 edid[LT7911EXC_EDID_MAX_SIZE];
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911EXC)
		return -EOPNOTSUPP;

	if (count < LT7911EXC_EDID_BLOCK_SIZE ||
	    count > LT7911EXC_EDID_MAX_SIZE ||
	    count % LT7911EXC_EDID_BLOCK_SIZE)
		return -EINVAL;

	if (copy_from_user(edid, user_buffer, count))
		return -EFAULT;

	if (!lt7911_edid_checksum_valid(edid, count))
		return -EINVAL;

	disable_irq(lt7911->irq);
	mutex_lock(&lt7911->exc_lock);
	ret = lt7911exc_write_edid(lt7911, edid, count);
	mutex_unlock(&lt7911->exc_lock);
	enable_irq(lt7911->irq);
	if (ret)
		return ret;

	mutex_lock(&lt7911->state_lock);
	memcpy(lt7911->video_edid, edid, count);
	lt7911->video_edid_len = count;
	mutex_unlock(&lt7911->state_lock);

	return count;
}

static ssize_t proc_video_edid_snapshot_read(struct file *file,
					     char __user *user_buffer,
					     size_t count, loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_EDID_SNAPSHOT, user_buffer,
				       count, offset);
}

static ssize_t proc_version_read(struct file *file, char __user *user_buffer,
				 size_t count, loff_t *offset)
{
	return lt7911_read_proc_buffer(PDE_DATA(file_inode(file)),
				       LT7911_PROC_VERSION, user_buffer,
				       count, offset);
}

static ssize_t proc_version_write(struct file *file,
				  const char __user *user_buffer,
				  size_t count, loff_t *offset)
{
	struct lt7911_data *lt7911 = PDE_DATA(file_inode(file));
	u8 version_write_buffer[LT7911D_WR_SIZE + 1];
	u8 version_read_buffer[LT7911D_WR_SIZE];
	int ret;

	if (!count || count > LT7911D_WR_SIZE)
		return -EINVAL;

	if (copy_from_user(version_write_buffer, user_buffer, count))
		return -EFAULT;
	version_write_buffer[count] = '\0';

	lt7911_power_set(lt7911, true);

	ret = lt7911_str_write(lt7911, version_write_buffer, count);
	if (ret)
		return -EIO;

	ret = lt7911_str_read(lt7911, version_read_buffer);
	if (ret)
		return -EIO;

	if (memcmp(version_write_buffer, version_read_buffer, count))
		return -EIO;

	mutex_lock(&lt7911->state_lock);
	memcpy(lt7911->version, version_read_buffer, count);
	lt7911->version_len = count;
	mutex_unlock(&lt7911->state_lock);

	dev_info(lt7911->dev, "Restarting LT7911D...\n");
	lt7911_power_set(lt7911, false);
	lt7911_power_set(lt7911, true);

	return count;
}

static ssize_t proc_fw_update_read(struct file *file,
				   char __user *user_buffer, size_t count,
				   loff_t *offset)
{
	struct lt7911_data *lt7911 = PDE_DATA(file_inode(file));
	char tmp[sizeof(lt7911->fw_status)];
	size_t len;

	mutex_lock(&lt7911->fw_lock);
	len = lt7911->fw_status_len;
	memcpy(tmp, lt7911->fw_status, len);
	mutex_unlock(&lt7911->fw_lock);

	return simple_read_from_buffer(user_buffer, count, offset, tmp, len);
}

static ssize_t proc_fw_update_write(struct file *file,
				    const char __user *user_buffer,
				    size_t count, loff_t *offset)
{
	struct lt7911_data *lt7911 = PDE_DATA(file_inode(file));
	char buf[16];
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911EXC)
		return -EOPNOTSUPP;

	if (!count || count >= sizeof(buf))
		return -EINVAL;

	if (copy_from_user(buf, user_buffer, count))
		return -EFAULT;
	buf[count] = '\0';
	if (buf[count - 1] == '\n')
		buf[count - 1] = '\0';

	if (strcmp(buf, "1") && strcmp(buf, "start"))
		return -EINVAL;

	mutex_lock(&lt7911->fw_lock);
	if (lt7911->fw_busy) {
		mutex_unlock(&lt7911->fw_lock);
		return -EBUSY;
	}

	lt7911->fw_busy = true;
	lt7911->fw_result = 0;
	lt7911->fw_size = 0;
	lt7911->fw_crc_expected = 0;
	lt7911->fw_crc_actual = 0;
	lt7911_fw_status_update_locked(lt7911, "busy");
	mutex_unlock(&lt7911->fw_lock);

	ret = lt7911_fw_update_run(lt7911);

	return ret ? ret : count;
}

static const struct file_operations proc_video_power_fops = {
	.owner = THIS_MODULE,
	.read = proc_video_power_read,
	.write = proc_video_power_write,
};

static const struct file_operations proc_chip_id_fops = {
	.owner = THIS_MODULE,
	.read = proc_chip_id_read,
};

static const struct file_operations proc_video_status_fops = {
	.owner = THIS_MODULE,
	.open = proc_status_open,
	.release = proc_status_release,
	.read = proc_video_status_read,
	.write = proc_video_status_write,
	.poll = proc_video_status_poll,
};

static const struct file_operations proc_video_width_fops = {
	.owner = THIS_MODULE,
	.read = proc_video_width_read,
};

static const struct file_operations proc_video_height_fops = {
	.owner = THIS_MODULE,
	.read = proc_video_height_read,
};

static const struct file_operations proc_video_fps_fops = {
	.owner = THIS_MODULE,
	.read = proc_video_fps_read,
};

static const struct file_operations proc_video_hdcp_fops = {
	.owner = THIS_MODULE,
	.read = proc_video_hdcp_read,
};

static const struct file_operations proc_audio_sample_rate_fops = {
	.owner = THIS_MODULE,
	.read = proc_audio_sample_rate_read,
};

static const struct file_operations proc_video_edid_fops = {
	.owner = THIS_MODULE,
	.read = proc_video_edid_read,
	.write = proc_video_edid_write,
};

static const struct file_operations proc_video_edid_snapshot_fops = {
	.owner = THIS_MODULE,
	.read = proc_video_edid_snapshot_read,
};

static const struct file_operations proc_version_fops = {
	.owner = THIS_MODULE,
	.read = proc_version_read,
	.write = proc_version_write,
};

static const struct file_operations proc_fw_update_fops = {
	.owner = THIS_MODULE,
	.read = proc_fw_update_read,
	.write = proc_fw_update_write,
};

static int lt7911_create_proc_file(struct lt7911_data *lt7911,
				   const char *name, umode_t mode,
				   const struct file_operations *fops)
{
	return proc_create_data(name, mode, lt7911->proc_dir, fops, lt7911) ?
		0 : -ENOMEM;
}

static int lt7911_proc_init(struct lt7911_data *lt7911)
{
	int ret;

	lt7911->proc_dir = proc_mkdir(PROC_LT7911_DIR, NULL);
	if (!lt7911->proc_dir)
		return -ENOMEM;

	ret = lt7911_create_proc_file(lt7911, PROC_CHIP_ID, 0444,
				      &proc_chip_id_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_PWR, 0666,
				      &proc_video_power_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_STATUS, 0666,
				      &proc_video_status_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_WIDTH, 0444,
				      &proc_video_width_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_HEIGHT, 0444,
				      &proc_video_height_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_FPS, 0444,
				      &proc_video_fps_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_HDCP, 0444,
				      &proc_video_hdcp_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_AUDIO_SAMPLE_RATE, 0444,
				      &proc_audio_sample_rate_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_EDID, 0666,
				      &proc_video_edid_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VIDEO_EDID_SNAPSHOT, 0444,
				      &proc_video_edid_snapshot_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_VERSION, 0666,
				      &proc_version_fops);
	if (ret)
		goto err;
	ret = lt7911_create_proc_file(lt7911, PROC_FW_UPDATE, 0644,
				      &proc_fw_update_fops);
	if (ret)
		goto err;

	return 0;

err:
	proc_remove(lt7911->proc_dir);
	lt7911->proc_dir = NULL;
	return ret;
}

static void lt7911_proc_remove(struct lt7911_data *lt7911)
{
	if (lt7911->proc_dir) {
		proc_remove(lt7911->proc_dir);
		lt7911->proc_dir = NULL;
	}
}

static void lt7911_update_video_status(struct lt7911_data *lt7911,
				       const struct lt7911_video_info *video)
{
	u16 width = video->width;
	u16 height = video->height;

	mutex_lock(&lt7911->state_lock);
	if (video->present) {
		if (video->res_type == ERROR_RES) {
			lt7911->video_status_len =
				scnprintf(lt7911->video_status,
					  sizeof(lt7911->video_status),
					  "error res\n");
			width = 0;
			height = 0;
		} else {
			lt7911->video_status_len =
				scnprintf(lt7911->video_status,
					  sizeof(lt7911->video_status),
					  "new res\n");
		}

		lt7911->video_width_len =
			scnprintf(lt7911->video_width,
				  sizeof(lt7911->video_width), "%u\n", width);
		lt7911->video_height_len =
			scnprintf(lt7911->video_height,
				  sizeof(lt7911->video_height), "%u\n",
				  height);
		lt7911->video_fps_len =
			scnprintf(lt7911->video_fps,
				  sizeof(lt7911->video_fps), "%u\n",
				  video->fps);
	} else {
		lt7911->video_status_len =
			scnprintf(lt7911->video_status,
				  sizeof(lt7911->video_status),
				  "disappear\n");
		lt7911->video_width_len =
			scnprintf(lt7911->video_width,
				  sizeof(lt7911->video_width), "0\n");
		lt7911->video_height_len =
			scnprintf(lt7911->video_height,
				  sizeof(lt7911->video_height), "0\n");
		lt7911->video_fps_len =
			scnprintf(lt7911->video_fps,
				  sizeof(lt7911->video_fps), "0\n");
	}
	lt7911->video_hdcp_len =
		scnprintf(lt7911->video_hdcp,
			  sizeof(lt7911->video_hdcp), "unknown hdcp\n");
	mutex_unlock(&lt7911->state_lock);

	lt7911_notify(lt7911);
}

static void lt7911_update_audio_status(struct lt7911_data *lt7911,
				       bool present, int sample_rate,
				       bool notify)
{
	mutex_lock(&lt7911->state_lock);
	if (!present) {
		lt7911->audio_sample_rate_len =
			scnprintf(lt7911->audio_sample_rate,
				  sizeof(lt7911->audio_sample_rate),
				  "disappear\n");
	} else if (sample_rate < 0) {
		lt7911->audio_sample_rate_len =
			scnprintf(lt7911->audio_sample_rate,
				  sizeof(lt7911->audio_sample_rate),
				  "unknown\n");
	} else {
		lt7911->audio_sample_rate_len =
			scnprintf(lt7911->audio_sample_rate,
				  sizeof(lt7911->audio_sample_rate), "%u\n",
				  (unsigned int)sample_rate);
	}
	mutex_unlock(&lt7911->state_lock);

	if (notify)
		lt7911_notify(lt7911);
}

static int lt7911d_read_video_info(struct lt7911_data *lt7911,
				   struct lt7911_video_info *video)
{
	int ret;

	video->present = true;
	video->res_type = NORMAL_RES;

	ret = lt7911_get_csi_res(lt7911, &video->width, &video->height);
	if (ret < 0)
		return ret;
	video->res_type = ret;

	ret = lt7911_get_csi_fps(lt7911, &video->fps);
	if (ret)
		video->fps = 0;

	return 0;
}

static void lt7911d_update_audio_status(struct lt7911_data *lt7911,
					bool present)
{
	u8 sample_rate = 0;
	int ret = 0;

	if (present) {
		ret = lt7911_get_audio_sample_rate(lt7911, &sample_rate);
		if (ret)
			dev_err(lt7911->dev,
				"Failed to get audio sample rate: %d\n", ret);
	}

	lt7911_update_audio_status(lt7911, present,
				   ret ? ret : sample_rate, false);
}

static void lt7911d_handle_irq(struct lt7911_data *lt7911)
{
	struct lt7911_video_info video = { 0 };
	u8 signal_state;
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911D)
		return;

	ret = lt7911_enable(lt7911);
	if (ret)
		return;

	ret = lt7911_get_signal_state(lt7911, &signal_state);
	if (ret) {
		dev_err(lt7911->dev, "Failed to get HDMI state: %d\n", ret);
		goto out_disable;
	}

	dev_info(lt7911->dev, "HDMI signal state: 0x%02x\n", signal_state);

	if (signal_state & 0x01) {
		dev_info(lt7911->dev, "HDMI signal is stable\n");
		if (force_height != -1 && force_width != -1) {
			lt7911_force_resolution(lt7911, force_width,
						force_height, force_fps);
		} else {
			ret = lt7911d_read_video_info(lt7911, &video);
			if (ret) {
				dev_err(lt7911->dev,
					"Failed to get HDMI video info: %d\n",
					ret);
			} else {
				lt7911_update_video_status(lt7911, &video);
				dev_info(lt7911->dev,
					 "HDMI resolution: %u x %u\n",
					 video.width, video.height);
				dev_info(lt7911->dev, "HDMI FPS: %u\n",
					 video.fps);
			}
		}
	} else {
		video.present = false;
		lt7911_update_video_status(lt7911, &video);
		dev_info(lt7911->dev, "HDMI signal has disappeared\n");
		dev_info(lt7911->dev, "HDMI resolution: 0 x 0\n");
		dev_info(lt7911->dev, "HDMI FPS: 0\n");
	}

	lt7911d_update_audio_status(lt7911, signal_state & 0x02);

out_disable:
	lt7911_disable(lt7911);
}

static int lt7911exc_read_video_info(struct lt7911_data *lt7911,
				     struct lt7911_video_info *video)
{
	int ret;

	video->present = true;
	video->res_type = NORMAL_RES;

	ret = lt7911exc_get_video_info(lt7911, &video->width, &video->height,
				       &video->fps);
	if (ret < 0)
		return ret;
	video->res_type = ret;

	return 0;
}

static void lt7911exc_update_video_status(struct lt7911_data *lt7911,
					  bool present)
{
	struct lt7911_video_info video = { 0 };
	int ret;

	video.present = present;
	if (!present) {
		dev_info(lt7911->dev, "LT7911EXC video is off\n");
		lt7911_update_video_status(lt7911, &video);
		dev_info(lt7911->dev, "LT7911EXC resolution: 0 x 0\n");
		dev_info(lt7911->dev, "LT7911EXC FPS: 0\n");
		return;
	}

	dev_info(lt7911->dev, "LT7911EXC video is ready\n");
	if (force_height != -1 && force_width != -1) {
		lt7911_force_resolution(lt7911, force_width, force_height,
					force_fps);
		return;
	}

	ret = lt7911exc_read_video_info(lt7911, &video);
	if (ret) {
		dev_err(lt7911->dev,
			"Failed to get LT7911EXC video info: %d\n", ret);
		return;
	}

	lt7911_update_video_status(lt7911, &video);
	dev_info(lt7911->dev, "LT7911EXC resolution: %u x %u\n",
		 video.width, video.height);
	dev_info(lt7911->dev, "LT7911EXC FPS: %u\n", video.fps);
}

static void lt7911exc_update_audio_status(struct lt7911_data *lt7911,
					  bool present)
{
	u8 sample_rate = 0;
	int ret = 0;

	if (present) {
		ret = lt7911exc_get_audio_sample_rate(lt7911, &sample_rate);
		if (ret)
			dev_err(lt7911->dev,
				"Failed to get LT7911EXC audio sample rate: %d\n",
				ret);
	} else {
		dev_info(lt7911->dev, "LT7911EXC audio is off\n");
	}

	lt7911_update_audio_status(lt7911, present,
				   ret ? ret : sample_rate, true);
	if (present && !ret)
		dev_info(lt7911->dev, "LT7911EXC audio sample rate: %u\n",
			 sample_rate);
}

static bool lt7911exc_refresh_ready_status(struct lt7911_data *lt7911)
{
	struct lt7911_video_info video = { 0 };
	u8 sample_rate = 0;
	int ret;

	if (lt7911->chip != LT7911_CHIP_LT7911EXC)
		return false;

	mutex_lock(&lt7911->exc_lock);

	lt7911->video_power_len = scnprintf(lt7911->video_power, sizeof(lt7911->video_power),
					    "on\n");

	ret = lt7911exc_read_video_info(lt7911, &video);
	if (ret) {
		dev_err(lt7911->dev,
			"Failed to refresh LT7911EXC video info: %d\n", ret);
		goto out_invalid;
	}

	ret = lt7911exc_get_audio_sample_rate(lt7911, &sample_rate);
	if (ret) {
		dev_err(lt7911->dev,
			"Failed to refresh LT7911EXC audio sample rate: %d\n",
			ret);
		goto out_invalid;
	}

	dev_info(lt7911->dev,
		 "LT7911EXC refresh: %u x %u, fps %u, asr %u\n",
		 video.width, video.height, video.fps, sample_rate);

	if (!video.width || !video.height || !video.fps || !sample_rate) {
		if (video.width || video.height || video.fps || sample_rate) {
			dev_info(lt7911->dev,
				 "LT7911EXC refresh data is incomplete\n");
			mutex_unlock(&lt7911->exc_lock);
			return true;
		}
		goto out_invalid;
	}

	if (force_height != -1 && force_width != -1)
		lt7911_force_resolution(lt7911, force_width, force_height,
					force_fps);
	else
		lt7911_update_video_status(lt7911, &video);

	lt7911_update_audio_status(lt7911, true, sample_rate, true);
	mutex_unlock(&lt7911->exc_lock);

	return true;

out_invalid:
	mutex_unlock(&lt7911->exc_lock);
	return false;
}

static void lt7911exc_handle_irq(struct lt7911_data *lt7911)
{
	u8 int_type;
	int ret;

	mutex_lock(&lt7911->exc_lock);

	ret = lt7911exc_get_int_type(lt7911, &int_type);
	if (ret) {
		dev_err(lt7911->dev, "Failed to get LT7911EXC INT type: %d\n",
			ret);
		goto out;
	}

	dev_info(lt7911->dev, "LT7911EXC INT type: 0x%02x\n", int_type);

	switch (int_type) {
	case LT7911EXC_INT_VIDEO_OFF:
		lt7911exc_update_video_status(lt7911, false);
		break;
	case LT7911EXC_INT_VIDEO_READY:
		lt7911exc_update_video_status(lt7911, true);
		break;
	case LT7911EXC_INT_AUDIO_OFF:
		lt7911exc_update_audio_status(lt7911, false);
		break;
	case LT7911EXC_INT_AUDIO_READY:
		lt7911exc_update_audio_status(lt7911, true);
		break;
	default:
		dev_warn(lt7911->dev, "Unknown LT7911EXC INT type: 0x%02x\n",
			 int_type);
		break;
	}

out:
	mutex_unlock(&lt7911->exc_lock);
}

static irqreturn_t lt7911_irq_thread(int irq, void *dev_id)
{
	struct lt7911_data *lt7911 = dev_id;

	if (!lt7911->init_done)
		return IRQ_HANDLED;

	if (lt7911->ops && lt7911->ops->handle_irq)
		lt7911->ops->handle_irq(lt7911);

	return IRQ_HANDLED;
}

static int force_res_param_set(const char *val, const struct kernel_param *kp)
{
	int old_val = *(int *)kp->arg;
	struct lt7911_data *lt7911;
	int ret;

	ret = param_set_int(val, kp);
	if (ret)
		return ret;

	mutex_lock(&active_lock);
	lt7911 = active_lt7911;
	if (!lt7911)
		goto out;

	if (old_val != -1 && *(int *)kp->arg == -1) {
		if (lt7911->chip == LT7911_CHIP_LT7911EXC) {
			ret = lt7911_reset(lt7911);
			if (ret)
				dev_err(lt7911->dev,
					"Failed to reset LT7911EXC: %d\n", ret);
		} else if (lt7911->ops && lt7911->ops->handle_irq) {
			lt7911->ops->handle_irq(lt7911);
		}
		goto out;
	}

	if (force_width > 0 && force_height > 0)
		lt7911_force_resolution(lt7911, force_width, force_height,
					force_fps);

out:
	mutex_unlock(&active_lock);
	return 0;
}

static const struct kernel_param_ops force_res_param_ops = {
	.set = force_res_param_set,
	.get = param_get_int,
};

module_param_cb(force_width, &force_res_param_ops, &force_width, 0644);
MODULE_PARM_DESC(force_width, "Force HDMI width");
module_param_cb(force_height, &force_res_param_ops, &force_height, 0644);
MODULE_PARM_DESC(force_height, "Force HDMI height");
module_param_cb(force_fps, &force_res_param_ops, &force_fps, 0644);
MODULE_PARM_DESC(force_fps, "Force HDMI fps");

static int lt7911_parse_dt(struct lt7911_data *lt7911)
{
	struct device *dev = lt7911->dev;

	lt7911->irq_gpio = devm_gpiod_get(dev, "irq", GPIOD_IN);
	if (IS_ERR(lt7911->irq_gpio)) {
		dev_err(dev, "failed to acquire irq GPIO: %ld\n",
			PTR_ERR(lt7911->irq_gpio));
		return PTR_ERR(lt7911->irq_gpio);
	}

	lt7911->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(lt7911->reset_gpio)) {
		dev_err(dev, "failed to acquire reset GPIO: %ld\n",
			PTR_ERR(lt7911->reset_gpio));
		return PTR_ERR(lt7911->reset_gpio);
	}

	lt7911->irq = gpiod_to_irq(lt7911->irq_gpio);
	if (lt7911->irq < 0) {
		dev_err(dev, "failed to map irq GPIO to IRQ: %d\n",
			lt7911->irq);
		return lt7911->irq;
	}

	return 0;
}

static int lt7911_manage_probe(struct i2c_client *client,
			       const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
	struct lt7911_data *lt7911;
	unsigned long irq_flags;
	int ret;

	lt7911 = devm_kzalloc(dev, sizeof(*lt7911), GFP_KERNEL);
	if (!lt7911)
		return -ENOMEM;

	lt7911->dev = dev;
	lt7911->client = client;
	lt7911->ops = of_device_get_match_data(dev);
	if (!lt7911->ops) {
		dev_err(dev, "unsupported LT7911 chip\n");
		return -ENODEV;
	}
	lt7911->chip = LT7911_CHIP_UNKNOWN;
	lt7911->last_width = 0xffff;
	lt7911->last_height = 0xffff;
	mutex_init(&lt7911->state_lock);
	mutex_init(&lt7911->exc_lock);
	mutex_init(&lt7911->fw_lock);
	init_waitqueue_head(&lt7911->wait_queue);
	atomic_set(&lt7911->event_seq, 0);
	i2c_set_clientdata(client, lt7911);
	lt7911_fw_status_update(lt7911, "idle");

	dev_info(dev, "Force HDMI width: %d, height: %d, fps: %d\n",
		 force_width, force_height, force_fps);

	lt7911->regmap = devm_regmap_init_i2c(client, &lt7911_regmap_config);
	if (IS_ERR(lt7911->regmap))
		return PTR_ERR(lt7911->regmap);

	ret = lt7911_parse_dt(lt7911);
	if (ret)
		return ret;

	lt7911_proc_buffer_init(lt7911);

	ret = lt7911_check_chip(lt7911);
	if (ret) {
		dev_err(dev, "unsupported LT7911 chip\n");
		return ret;
	}

	if (lt7911->chip == LT7911_CHIP_LT7911D) {
		ret = lt7911_reset(lt7911);
		if (ret)
			return ret;
	}

	ret = lt7911_proc_init(lt7911);
	if (ret)
		return ret;

	mutex_lock(&active_lock);
	active_lt7911 = lt7911;
	mutex_unlock(&active_lock);

	lt7911->init_done = true;

	if (lt7911->ops->handle_irq) {
		irq_flags = IRQF_ONESHOT | lt7911->ops->irq_flags;
		msleep(50);

		ret = devm_request_threaded_irq(dev, lt7911->irq,
						NULL, lt7911_irq_thread,
						irq_flags,
						"lt7911_manage", lt7911);
		if (ret) {
			lt7911->init_done = false;
			mutex_lock(&active_lock);
			if (active_lt7911 == lt7911)
				active_lt7911 = NULL;
			mutex_unlock(&active_lock);
			lt7911_proc_remove(lt7911);
			return ret;
		}
	}

	if (lt7911->chip == LT7911_CHIP_LT7911EXC)
		lt7911exc_refresh_ready_status(lt7911);

	dev_info(dev, "lt7911_manage module loaded\n");
	return 0;
}

static int lt7911_manage_remove(struct i2c_client *client)
{
	struct lt7911_data *lt7911 = i2c_get_clientdata(client);

	mutex_lock(&active_lock);
	if (active_lt7911 == lt7911)
		active_lt7911 = NULL;
	mutex_unlock(&active_lock);

	lt7911->init_done = false;
	lt7911_proc_remove(lt7911);

	dev_info(&client->dev, "lt7911_manage module unloaded\n");
	return 0;
}

static const struct of_device_id lt7911_of_match[] = {
	{ .compatible = "lontium,lt7911d", .data = &lt7911d_ops },
	{ .compatible = "lontium,lt7911exc", .data = &lt7911exc_ops },
	{ }
};
MODULE_DEVICE_TABLE(of, lt7911_of_match);

static const struct i2c_device_id lt7911_i2c_id[] = {
	{ "lt7911d", 0 },
	{ "lt7911exc", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, lt7911_i2c_id);

static struct i2c_driver lt7911_manage_driver = {
	.driver = {
		.name = "lt7911_manage",
		.of_match_table = of_match_ptr(lt7911_of_match),
	},
	.probe = lt7911_manage_probe,
	.remove = lt7911_manage_remove,
	.id_table = lt7911_i2c_id,
};

module_i2c_driver(lt7911_manage_driver);

MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.6");
MODULE_AUTHOR("Z2Z-BuGu");
MODULE_AUTHOR("916BGAI");
MODULE_DESCRIPTION("NanoAgent HDMI Module Management");
