
#include <string.h>
#include <stdlib.h>

#include "dpc_lib.h"
#include "vendor_isp.h"

#define MEASURE_TIME 0
#if (MEASURE_TIME)
#include "hd_common.h"
#include "hd_util.h"
#endif


#define PAUSE {printf("Press Enter key to continue..."); fgetc(stdin);}

#define CALI_MIN(x,y)    ((x) < (y) ? (x) : (y))
#define CALI_MAX(x,y)    ((x) > (y) ? (x) : (y))

static unsigned int get_choose_uint(void)
{
	char str_buf[32];
	unsigned int val, error;
	do {
		error = scanf(" %d", &val);
		if (error != 1) {
			printf("Invalid option. Try again.\n");
			clearerr(stdin);
			fgets(str_buf, sizeof(str_buf), stdin);
			printf(">> ");
		}
	} while(error != 1);

	return val;
}

static IQT_DPC_PARAM dpc_param;

int main(int argc, char *argv[])
{
	char *dp_buf = NULL;
	unsigned int option, infinite_loop = 1;

	ISPT_RAW_INFO raw_info = {0};

	CAL_ALG_DP_RST dp_rst;
	unsigned int sensor_id;
	AET_MANUAL ae;
	unsigned int dpc_th; //defual 20
	unsigned int dbg_en = 0;
	unsigned int dpc_en;
	int open_device;
	AET_CFG_INFO cfg_info = {0};
	//isp_dpc_path dpc_path;

	int ret = 0;

	open_device = vendor_isp_init();

	if (open_device < 0) {
		printf("open MCU device fail!\n");
		return E_GET_DEV_FAIL;
	}
/*
	if (dpc_init() < 0) {
		printf("failed to init cali lib!\n");

		return -1;
	}
*/
	// Enter sensor id
/*
	printf("----------------------------------------\n");
	printf("Available sensor id list: \r\n");
	get_sensor_id();
*/
	printf("Enter sensor id: {0,1,2}  \r\n");
	do {
		printf(">> ");
		sensor_id = get_choose_uint();
		printf("cali_sensor_id=%d \r\n", sensor_id);
		raw_info.id = sensor_id;
		cfg_info.id = sensor_id;
		dpc_param.id = sensor_id;
		ae.id = sensor_id;
	} while (0);

	// manual AE
	ae.manual.mode= MANUAL_MODE;
	ae.manual.totalgain= 8000;
	printf("----------------------------------------\n");
	printf("SET AE Exposure&Gain: \r\n");
	printf("Enter Exposure:  \r\n");
	do {
		printf(">> ");
		ae.manual.expotime = get_choose_uint();
		printf("exposure=%d \r\n", ae.manual.expotime);
	} while (0);

	printf("Enter Gain:  \r\n");
	do {
		printf(">> ");
		ae.manual.iso_gain = get_choose_uint();
		printf("gain=%d \r\n", ae.manual.iso_gain);
	} while (0);

	set_AE(&ae);

	while (infinite_loop) {
		printf("----------------------------------------\n");
		printf(" 1. Start dpc cali\n");
		printf(" 2. load dpc bin\n");
		printf(" 3. dpc on/off\n");
		printf(" 4. Dbg mode on/off\n");
		printf(" 5. Quit\n");
		printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_uint();
		} while (0);

		switch (option) {
		case 1:
			/* //coverity issue
			if(raw_info.raw_info.addr != 0) {
				free((UINT32*)(raw_info.raw_info.addr));
				raw_info.raw_info.addr = 0;
			}
			*/
			// get 8bit raw
			dpc_get_raw(&raw_info);

			if (dbg_en) {
				printf("addr = 0x%x, width=%d, height=%d, loff = %d, pxlfmt = 0x%x \r\n", raw_info.raw_info.addr, raw_info.raw_info.pw, raw_info.raw_info.ph, raw_info.raw_info.loff, raw_info.raw_info.pxlfmt);
			}

			// allocate dp buffer
			if ((dp_buf = (char *)malloc(raw_info.raw_info.pw * raw_info.raw_info.ph)) == NULL) {
				printf("fail to allocate memory for image buffer!\n");

				if(raw_info.raw_info.addr != 0) {
					free((UINT32*)(raw_info.raw_info.addr));
					raw_info.raw_info.addr = 0;
				}

				break;
		    }
			memset(dp_buf, 0, raw_info.raw_info.pw * raw_info.raw_info.ph);
			dp_rst.addr = (unsigned int)&dp_buf[0];

			// input threshold
		    printf("Enter threshold:  \r\n");
		    do {
		            printf(">> ");
		            dpc_th = get_choose_uint();
		            printf("dpc threshold = %d \r\n", dpc_th);
		    } while (0);

			// dpc process

			cal_dp_process(&raw_info, dpc_th, &dp_rst);

			if(raw_info.raw_info.addr != 0) {
				free((UINT32*)(raw_info.raw_info.addr));
				raw_info.raw_info.addr = 0;
			}

			if (dbg_en) {
				printf("cnt=%d,data_length=%d \r\n", dp_rst.pixel_cnt,dp_rst.data_length);
			}

			// save dp bin
			FILE *fp = NULL;
			fp = fopen("/mnt/sd/dp.bin", "wb");
			if (fp != NULL) {
				fwrite(dp_buf, sizeof(unsigned char), dp_rst.data_length, fp);

				fclose(fp);
			} else {
				printf("fp is NULL\r\n");
			}

			if (dp_buf != NULL) {
			    free(dp_buf);
			    dp_buf = NULL;
			}

			break;

		case 2:
			// set dp bin
			strncpy(cfg_info.path, "/mnt/app/isp/isp_imx290_0.cfg", CFG_NAME_LENGTH);
			ret = vendor_isp_set_iq(IQT_ITEM_RLD_CONFIG, &cfg_info);
			if (ret < 0) {
				printf("SET IQT_ITEM_RLD_CONFIG fail!\n");
				break;
			}

			// get dpc par
			ret = vendor_isp_get_iq(IQT_ITEM_DPC_PARAM, &dpc_param);
			if (ret < 0) {
				printf("GET IQT_ITEM_DPC_PARAM fail!\n");
				break;
			}
			//printf("dpc_param 0x%x 0x%x\e\n",dpc_param.dpc.table[0],dpc_param.dpc.table[1]);
			break;

		case 3:
			// set dpc enable
			printf("DPC enable: (0:off, 1:on)  \r\n");
			do {
				printf(">> ");
				dpc_en = get_choose_uint();
				printf("dpc_en=%d \r\n", dpc_en);
			} while (0);

			dpc_param.dpc.enable = dpc_en;
			// get dpc par
			ret = vendor_isp_set_iq(IQT_ITEM_DPC_PARAM, &dpc_param);
			if (ret < 0) {
				printf("SET IQT_ITEM_DPC_PARAM fail!\n");
				break;
			}

			break;

		case 4:
			// dbg msg on/off
			printf("dbg enable: (0:off, 1:on)  \r\n");
			do {
				printf(">> ");
				dbg_en = get_choose_uint();
				printf("dbg_en=%d \r\n", dbg_en);
			} while (0);
			cali_dpc_set_dbg_out(dbg_en);
			break;

		case 5:
		default:
			infinite_loop = 0;
			break;
		}
    }

/*
	dpc_exit();
*/
	ret = vendor_isp_uninit();
	if(ret != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", ret);
	}

	if(raw_info.raw_info.addr != 0) {
		free((UINT32*)(raw_info.raw_info.addr));
	}

	return 0;
}

