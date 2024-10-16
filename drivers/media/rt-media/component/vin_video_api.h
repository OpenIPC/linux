
#ifndef _VIN_VIDEO_API_H_
#define _VIN_VIDEO_API_H_

#include <linux/videodev2.h>
#include "vin_video.h"


enum {
	/*encpp_ctrl*/
	RT_CTRL_ENCPP_EN = 0,
	RT_CTRL_ENCPP_STATIC_CFG,
	RT_CTRL_ENCPP_DYNAMIC_CFG,
	RT_CTRL_ENCODER_3DNR_CFG,
	RT_CTRL_ENCODER_2DNR_CFG,
};

int vin_open_special(int id);
int vin_s_input_special(int id, int i);
int vin_s_parm_special(int id, void *priv, struct v4l2_streamparm *parms);
int vin_close_special(int id);
int vin_s_fmt_special(int id, struct v4l2_format *f);
int vin_g_fmt_special(int id, struct v4l2_format *f);
int vin_dqbuffer_special(int id, struct vin_buffer **buf);
int vin_qbuffer_special(int id, struct vin_buffer *buf);
int vin_streamon_special(int id, enum v4l2_buf_type i);
int vin_streamoff_special(int id, enum v4l2_buf_type i);
int vin_get_encpp_cfg(int id, unsigned char ctrl_id, void *value);
void vin_register_buffer_done_callback(int id, void *func);
struct device *vin_get_dev(int id);
int vin_isp_get_lv_special(int id, int mode_flag);
void vin_server_reset_special(int id, int mode_flag, int ir_on, int ir_flash_on);
int vin_s_ctrl_special(int id, unsigned int ctrl_id, int val);
int vin_isp_get_exp_gain_special(int id, struct sensor_exp_gain *exp_gain);
int vin_isp_get_hist_special(int id, unsigned int *hist);
int vin_isp_set_attr_cfg(int id, unsigned int ctrl_id, int value);

#endif

