#include <linux/module.h>

#include <mach-anyka/ispdrv_interface.h>

int ispdrv_irq_work(void)
{
	return ak_isp_irq_work();
}
EXPORT_SYMBOL(ispdrv_irq_work);

int ispdrv_awb_work(void)
{
	return ak_isp_awb_work();
}
EXPORT_SYMBOL(ispdrv_awb_work);

int ispdrv_ae_work(void)
{
	return ak_isp_ae_work();
}
EXPORT_SYMBOL(ispdrv_ae_work);

void ispdrv_vo_get_flip_mirror(int *flip_en, int *mirror_en, int *height_block_num)
{
	ak_isp_get_flip_mirror(flip_en, mirror_en, height_block_num);
}
EXPORT_SYMBOL(ispdrv_vo_get_flip_mirror);

int ispdrv_vo_get_using_frame_buf_id(void)
{
	return ak_isp_vo_get_using_frame_buf_id();
}
EXPORT_SYMBOL(ispdrv_vo_get_using_frame_buf_id);

int ispdrv_vo_set_main_channel_scale(int width, int height)
{
	return ak_isp_vo_set_main_channel_scale(width, height);
}
EXPORT_SYMBOL(ispdrv_vo_set_main_channel_scale);

int ispdrv_vo_set_sub_channel_scale(int width, int height)
{
	return ak_isp_vo_set_sub_channel_scale(width, height);
}
EXPORT_SYMBOL(ispdrv_vo_set_sub_channel_scale);

int ispdrv_vi_set_crop(int sx, int sy, int width, int height)
{
	return ak_isp_vi_set_crop(sx, sy, width, height);
}
EXPORT_SYMBOL(ispdrv_vi_set_crop);

int ispdrv_vi_apply_mode(enum isp_working_mode mode)
{
	return ak_isp_vi_apply_mode(mode);
}
EXPORT_SYMBOL(ispdrv_vi_apply_mode);

int ispdrv_vo_enable_buffer(enum buffer_id id)
{
	return ak_isp_vo_enable_buffer(id);
}
EXPORT_SYMBOL(ispdrv_vo_enable_buffer);

int ispdrv_vo_disable_buffer(enum buffer_id id)
{
	return ak_isp_vo_disable_buffer(id);
}
EXPORT_SYMBOL(ispdrv_vo_disable_buffer);

int ispdrv_vo_set_buffer_addr(enum buffer_id id,unsigned long yaddr_main_chan_addr,
		            unsigned long yaddr_sub_chan_addr)
{
	return ak_isp_vo_set_buffer_addr(id, yaddr_main_chan_addr, yaddr_sub_chan_addr);
}
EXPORT_SYMBOL(ispdrv_vo_set_buffer_addr);

int ispdrv_vo_enable_irq_status(int bit)
{
	return ak_isp_vo_enable_irq_status(bit);
}
EXPORT_SYMBOL(ispdrv_vo_enable_irq_status);

int ispdrv_vo_clear_irq_status(int bit)
{
	return ak_isp_vo_clear_irq_status(bit);
}
EXPORT_SYMBOL(ispdrv_vo_clear_irq_status);

int ispdrv_vo_check_irq_status(void)
{
	return ak_isp_vo_check_irq_status();
}
EXPORT_SYMBOL(ispdrv_vo_check_irq_status);

int ispdrv_vi_start_capturing(void)
{
	return ak_isp_vi_start_capturing();
}
EXPORT_SYMBOL(ispdrv_vi_start_capturing);

int ispdrv_vi_stop_capturing(void)
{
	return ak_isp_vi_stop_capturing();
}
EXPORT_SYMBOL(ispdrv_vi_stop_capturing);

int ispdrv_is_continuous(void)
{
	return ak_isp_is_continuous();
}
EXPORT_SYMBOL(ispdrv_is_continuous);

int ispdrv_register_sensor(void *sensor_info)
{
	return ak_isp_register_sensor(sensor_info);
}
EXPORT_SYMBOL(ispdrv_register_sensor);

void *ispdrv_get_sensor(int *index)
{
	return ak_isp_get_sensor(index);
}
EXPORT_SYMBOL(ispdrv_get_sensor);

void ispdrv_remove_all_sensors(void)
{
	ak_isp_remove_all_sensors();
}
EXPORT_SYMBOL(ispdrv_remove_all_sensors);

AK_ISP_PCLK_POLAR ispdrv_get_pclk_polar(void)
{
	return ak_isp_get_pclk_polar();
}
EXPORT_SYMBOL(ispdrv_get_pclk_polar);

int ispdrv_set_isp_pause(void)
{
	return ak_isp_set_isp_capturing(0);
}
EXPORT_SYMBOL(ispdrv_set_isp_pause);

int ispdrv_set_isp_resume(void)
{
	return ak_isp_set_isp_capturing(1);
}
EXPORT_SYMBOL(ispdrv_set_isp_resume);

int ispdrv_set_td(void)
{
    return ak_isp_set_td();
}
EXPORT_SYMBOL(ispdrv_set_td);

int ispdrv_reload_td(void)
{
    return ak_isp_reload_td();
}
EXPORT_SYMBOL(ispdrv_reload_td);

int ispdrv_get_yuvaddr_and_mdinfo(int id, void **yuv, void **mdinfo)
{
	return ak_isp_get_yuvaddr_and_mdinfo(id, yuv, mdinfo);
}
EXPORT_SYMBOL(ispdrv_get_yuvaddr_and_mdinfo);

int ispdrv_vo_get_inputdataw(void)
{
	return ak_isp_vo_get_inputdataw();
}
EXPORT_SYMBOL(ispdrv_vo_get_inputdataw);
