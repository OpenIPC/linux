#ifndef __ISPDRV_INTERFACE__
#define __ISPDRV_INTERFACE__

#include <plat-anyka/ak_isp_drv.h>

enum sensor_get_param {
	GET_MIPI_MHZ = 0,
	GET_VSYNC_ACTIVE_MS,
	GET_CUR_FPS,
	GET_MIPI_LANE,
	GET_INTERFACE,
	GET_SENSOR_IO_LEVEL
};

enum sensor_interface {
	DVP_INTERFACE = 0,
	MIPI_INTERFACE
};

enum sensor_io_level {
	SENSOR_IO_LEVEL_1V8 = 0,   // 1.8V IO
	SENSOR_IO_LEVEL_2V5,       // 2.5V IO
	SENSOR_IO_LEVEL_3V3        // 3.3V IO
};


int ispdrv_irq_work(void);

int ispdrv_awb_work(void);

int ispdrv_ae_work(void);

void ispdrv_vo_get_flip_mirror(int *flip_en, int *mirror_en, int *height_block_num);

int ispdrv_vo_get_using_frame_buf_id(void);

int ispdrv_vo_set_main_channel_scale(int width, int height);

int ispdrv_vo_set_sub_channel_scale(int width, int height);

int ispdrv_vi_set_crop(int sx, int sy, int width, int height);

int ispdrv_vi_apply_mode(enum isp_working_mode mode);

int ispdrv_vo_enable_buffer(enum buffer_id id);

int ispdrv_vo_disable_buffer(enum buffer_id id);

int ispdrv_vo_set_buffer_addr(enum buffer_id id,unsigned long yaddr_main_chan_addr,
		            unsigned long yaddr_sub_chan_addr);

int ispdrv_vo_enable_irq_status(int bit);

int ispdrv_vo_clear_irq_status(int bit);

int ispdrv_vo_check_irq_status(void);

int ispdrv_vi_start_capturing(void);

int ispdrv_vi_stop_capturing(void);

int ispdrv_is_continuous(void);

int ispdrv_register_sensor(void *sensor_info);

void *ispdrv_get_sensor(int *index);

void ispdrv_remove_all_sensors(void);

AK_ISP_PCLK_POLAR ispdrv_get_pclk_polar(void);

int ispdrv_set_isp_pause(void);
int ispdrv_set_isp_resume(void);

int ispdrv_set_td(void);
int ispdrv_reload_td(void);

int ispdrv_get_yuvaddr_and_mdinfo(int id, void **yuv, void **mdinfo);
int ispdrv_vo_get_inputdataw(void);

#endif
