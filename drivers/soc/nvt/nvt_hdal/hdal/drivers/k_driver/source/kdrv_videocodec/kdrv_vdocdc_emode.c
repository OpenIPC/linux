#if defined(__LINUX)
#include <linux/vmalloc.h>
#elif defined(__FREERTOS)
#include <malloc.h>
#include <string.h>
#endif
#if defined(VDOCDC_EMU)
#include "kdrv_vdocdc_emode.h"
int kdrv_vdocdc_emode_get_enc_info(VENC_CH_INFO *p_info){return 0;}
int kdrv_vdocdc_emode_set_enc_id(unsigned int enc_id){return 0;}
int kdrv_vdocdc_emode_read_cfg(char file_name[64]){return 0;}
int kdrv_vdocdc_emode_write_cfg(char file_name[64]){return 0;}
#else
#include "kwrap/file.h"

#include "kdrv_videoenc/kdrv_videoenc.h"

#include "kdrv_vdocdc_comn.h"
#include "kdrv_vdocdc_dbg.h"
#include "kdrv_vdocdc_emode.h"
#include "vemd_api.h"
#include "vemd_cfg.h"

int kdrv_vdocdc_emode_get_enc_info(VENC_CH_INFO *p_info)
{
	int i;

	memset(p_info, 0, sizeof(VENC_CH_INFO));

	p_info->drv_ver = ((KDRV_VDOCDC_VERSION_0<<28) |
						((KDRV_VDOCDC_VERSION_1/10)<<24) | ((KDRV_VDOCDC_VERSION_1%10)<<20) |
						((KDRV_VDOCDC_VERSION_2/100)<<16) | ((((KDRV_VDOCDC_VERSION_2%100)/10))<<12) | (((KDRV_VDOCDC_VERSION_2%10))<<8) |
						((KDRV_VDOCDC_VERSION_3/10)<<4) | (KDRV_VDOCDC_VERSION_3%10));

	p_info->emode_ver = KDRV_VDOCDC_EMODE_VERSION;

	for (i = 0; i < KDRV_VDOENC_ID_MAX; i++) {
		if (g_enc_info[i].b_enable == TRUE)
			p_info->id[p_info->num++] = i;
	}

	return 0;
}

int kdrv_vdocdc_emode_set_enc_id(unsigned int enc_id)
{
	g_vemd_info.enc_id = (int)enc_id;

	return 0;
}

int kdrv_vdocdc_emode_read_cfg(char file_name[64])
{
	VOS_FILE c_file = 0;
	struct vos_stat f_stat;

	char *buf = NULL;
	int ret = 0;

	if ((c_file = vos_file_open(file_name, O_RDONLY, 0)) == -1) {
		DBG_ERR("open file (%s) error\r\n", file_name);
		ret = -1;
		goto exit;
	}

	if (vos_file_fstat(c_file, &f_stat) != 0) {
		DBG_ERR("fstat error\r\n");
		ret = -1;
		goto free_file;
	}

#if defined(__LINUX)
	buf = (char *)vmalloc(sizeof(char)*f_stat.st_size);
#elif defined(__FREERTOS)
	buf = (char *)malloc(sizeof(char)*f_stat.st_size);
#endif

	if (buf == NULL) {
		DBG_ERR("malloc buffer error\r\n");
		ret = -1;
		goto free_file;
	}

	if (vos_file_read(c_file, buf, f_stat.st_size) != (int)f_stat.st_size) {
		DBG_ERR("read file error\r\n");
		ret = -1;
		goto free_buf;
	}

	buf[f_stat.st_size - 1] = '\n';

	if (vemd_parse_cfg(buf, f_stat.st_size) != 0) {
		DBG_ERR("parsing cfg error\r\n");
		ret = -1;
	}

free_buf:
#if defined(__LINUX)
		vfree(buf);
#elif defined(__FREERTOS)
		free(buf);
#endif

free_file:
	vos_file_close(c_file);
exit:
	return ret;
}

int kdrv_vdocdc_emode_write_cfg(char file_name[64])
{
	VOS_FILE c_file = 0;

	char *buf = NULL;
	int ret = 0;
	size_t buf_size = 1024*5;

	if (g_enc_info[g_vemd_info.enc_id].b_enable != TRUE) {
		DBG_ERR("select enc_id(%d) not enable already\r\n", g_vemd_info.enc_id);
		return -1;
	}

	if ((c_file = vos_file_open(file_name, O_CREAT | O_WRONLY | O_SYNC, 0)) == -1) {
		DBG_ERR("open file (%s) error\r\n", file_name);
		ret = -1;
		goto exit;
	}

#if defined(__LINUX)
	buf = (char *)vmalloc(sizeof(char)*buf_size);
#elif defined(__FREERTOS)
	buf = (char *)malloc(sizeof(char)*buf_size);
#endif

	if (vemd_gen_cfg(buf, buf_size) != 0) {
		DBG_ERR("generate cfg error\r\n");
		ret = -1;
		goto exit;
	}

	vos_file_write(c_file, buf, strlen(buf));

exit:
	if (buf != NULL) {
#if defined(__LINUX)
		vfree(buf);
#elif defined(__FREERTOS)
		free(buf);
#endif
	}

	if (c_file != -1)
		vos_file_close(c_file);

	return ret;
}
#endif
