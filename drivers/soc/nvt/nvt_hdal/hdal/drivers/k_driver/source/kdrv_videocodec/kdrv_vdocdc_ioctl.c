#include <linux/uaccess.h>

#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_ioctl.h"
#include "kdrv_vdocdc_emode.h"

#include "vemd_cfg.h"

static int get_ch_info(unsigned long arg)
{
	VENC_CH_INFO info;

	kdrv_vdocdc_emode_get_enc_info(&info);

	if (copy_to_user((void *)arg, (void *)(&info), sizeof(VENC_CH_INFO)) != 0) {
        DBG_ERR("copy_to_user error\n");
		return -1;
    }

	return 0;

}

static int set_enc_id(unsigned long arg)
{
	unsigned int enc_id;

	if (copy_from_user((void *)(&enc_id), (void *)arg, sizeof(unsigned int)) != 0) {
		DBG_ERR("copy_from_user error\n");
		return -1;
    }

	return kdrv_vdocdc_emode_set_enc_id(enc_id);
}

static int read_cfg(unsigned long arg)
{
	char file_name[64] = {0};

	if (copy_from_user((void *)file_name, (void *)arg, sizeof(char)*64) != 0) {
		DBG_ERR("copy_from_user error\n");
		return -1;
	}

	printk("read file_name = %s\r\n", file_name);

	return kdrv_vdocdc_emode_read_cfg(file_name);
}

static int write_cfg(unsigned long arg)
{
	char file_name[64] = {0};

	if (copy_from_user((void *)file_name, (void *)arg, sizeof(char)*64) != 0) {
		DBG_ERR("copy_from_user error\n");
		return -1;
	}

	printk("write file_name = %s\r\n", file_name);

	return kdrv_vdocdc_emode_write_cfg(file_name);
}

long kdrv_vdocdc_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	long ret = 0;

	if (filp == NULL) {
		DBG_ERR("invalid input\r\n");
		return -1;
	}

	switch(cmd) {
		case VENC_IOC_GET_CH_INFO:
			get_ch_info(arg);
			break;
		case VENC_IOC_SET_ENC_ID:
			set_enc_id(arg);
			break;
		case VENC_IOC_RD_CFG:
			read_cfg(arg);
			break;
		case VENC_IOC_WT_CFG:
			write_cfg(arg);
			break;
		default:
			break;
	}

	return ret;
}

