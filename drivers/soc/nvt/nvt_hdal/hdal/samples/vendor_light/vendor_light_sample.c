
#include <string.h>
#include <stdlib.h>

#include "light_lib.h"
#include "vendor_isp.h"

#define MEASURE_TIME 0
#if (MEASURE_TIME)
#include "hd_common.h"
#include "hd_util.h"
#endif

//============================================================================
// PLATFORM define
//============================================================================
#define IPC_DEMO2

//============================================================================
// Macro
//============================================================================
#define MIN(x,y) ((x)<(y) ? (x) : (y))
#define MAX(x,y) ((x)>(y) ? (x) : (y))

//============================================================================
// global
//============================================================================

typedef enum _LIGHT_MODE {
	E_DAY_MODE = 0,
	E_NIGHT_MODE = 1,
}LIGHT_MODE;

static LIGHT_MODE g_curr_mode = E_DAY_MODE;
static UINT32 id = 0;

static UINT32 while_loop = TRUE;

//============================================================================
// DEMO board platform control
//============================================================================
int init_demo2_led(void)
{
	/*
	char pbuffer[64];
	FILE *pfile = fopen("/sys/class/gpio/export", "w");

	if (!pfile) {
		printf("ERROR: failed to open gpio export proc node\n");
		return -1;
	}
	sprintf(pbuffer, "42\n");
	fwrite(pbuffer, sizeof(char), strlen(pbuffer), pfile);
	fclose(pfile);
	pfile = 0;

	pfile = fopen("/sys/class/gpio/gpio42/direction", "w");

	if (!pfile) {
		printf("ERROR: failed to open gpio42 direction proc node\n");
		return -1;
	}
	sprintf(pbuffer, "out\n");
	fwrite(pbuffer, sizeof(char), strlen(pbuffer), pfile);
	fclose(pfile);
	*/
	return 0;
}

int set_demo2_led(int on)
{
	/*
	char pbuffer[64];
	FILE *pfile;
	pfile = fopen("/sys/class/gpio/gpio42/value", "w");

	if (!pfile) {
		printf("ERROR: failed to open gpio42 proc node\n");
		return -1;
	}

	if (on) {
		sprintf(pbuffer, "1\n");
	} else {
		sprintf(pbuffer, "0\n");
	}
	fwrite(pbuffer, sizeof(char), strlen(pbuffer), pfile);
	fclose(pfile);
	*/
	return 0;

}

int init_demo2_ircut(void)
{
	/*
	char pbuffer[64];
	FILE *pfile = fopen("/sys/class/gpio/export", "w");

	if (!pfile) {
		printf("ERROR: failed to open gpio export proc node\n");
		return -1;
	}
	sprintf(pbuffer, "32\n");
	fwrite(pbuffer, sizeof(char), strlen(pbuffer), pfile);
	fclose(pfile);
	pfile = 0;

	pfile = fopen("/sys/class/gpio/gpio32/direction", "w");

	if (!pfile) {
		printf("ERROR: failed to open gpio32 direction proc node\n");
		return -1;
	}
	sprintf(pbuffer, "out\n");
	fwrite(pbuffer, sizeof(char), strlen(pbuffer), pfile);
	fclose(pfile);

	pfile = fopen("/sys/class/gpio/export", "w");

	if (!pfile) {
		printf("ERROR: failed to open gpio export proc node\n");
		return -1;
	}
	sprintf(pbuffer, "34\n");
	fwrite(pbuffer, sizeof(char), strlen(pbuffer), pfile);
	fclose(pfile);
	pfile = 0;

	pfile = fopen("/sys/class/gpio/gpio34/direction", "w");

	if (!pfile) {
		printf("ERROR: failed to open gpio34 direction proc node\n");
		return -1;
	}
	sprintf(pbuffer, "out\n");
	fwrite(pbuffer, sizeof(char), strlen(pbuffer), pfile);
	fclose(pfile);
	*/
	return 0;
}

int set_demo2_ircut(int on)
{
	/*
	char pbuffer0[64], pbuffer1[64];
	FILE *pfile0, *pfile1;

	pfile0 = fopen("/sys/class/gpio/gpio32/value", "w");
	if (!pfile0) {
		printf("ERROR: failed to open gpio32 proc node\n");
		return -1;
	}
	pfile1 = fopen("/sys/class/gpio/gpio34/value", "w");
	if (!pfile1) {
		printf("ERROR: failed to open gpio34 proc node\n");
		fclose(pfile1);
		return -1;
	}
	sprintf(pbuffer1, "1\n");
	sprintf(pbuffer0, "0\n");

	if (on) {
		fwrite(pbuffer0, sizeof(char), strlen(pbuffer0), pfile0);
		fwrite(pbuffer1, sizeof(char), strlen(pbuffer0), pfile1);
	} else {
		fwrite(pbuffer1, sizeof(char), strlen(pbuffer1), pfile0);
		fwrite(pbuffer0, sizeof(char), strlen(pbuffer0), pfile1);
	}
	fclose(pfile0);
	fclose(pfile1);
	*/

	return 0;

}

int init_LED(void)
{
	int ret = 0;
	// NOTE: mark
	#if 0
	ret = init_demo2_led();
	#endif
	return ret;
}
int set_LED(int on)
{
	int ret = 0;

	// NOTE: mark
	#if 0
	ret = set_demo2_led(on);
	#endif

	return ret;
}
int init_IRcut(void)
{
	int ret = 0;

	// NOTE: mark
	#if 0
	ret = init_demo2_ircut();
	#endif

	return ret;
}

int set_IRcut(int on)
{
	int ret = 0;

	// NOTE: mark
	#if 0
	ret = set_demo2_ircut(on);
	#endif

	return ret;
}

static int set_isp_daynight_mode(LIGHT_MODE mode)
{
	IQT_NIGHT_MODE night_mode = {0};
	AWBT_SCENE_MODE awb_scene = {0};
	int ret = 0;

	night_mode.id = id;
	awb_scene.id = id;

	if (mode == E_NIGHT_MODE) {
		night_mode.mode = 1;
		awb_scene.mode = 10;
		ret = vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
		if (ret < 0) {
			printf("set IQT_ITEM_NIGHT_MODE fail!\n");
		}

		ret = vendor_isp_set_awb(AWBT_ITEM_SCENE, &awb_scene);
		if (ret < 0) {
			printf("set AWBT_ITEM_SCENE fail!\n");
		}

	} else {
		night_mode.mode = 0;
		awb_scene.mode = 0;
		ret = vendor_isp_set_iq(IQT_ITEM_NIGHT_MODE, &night_mode);
		if (ret < 0) {
			printf("set IQT_ITEM_NIGHT_MODE fail!\n");
		}
		ret = vendor_isp_set_awb(AWBT_ITEM_SCENE, &awb_scene);
		if (ret < 0) {
			printf("set AWBT_ITEM_SCENE fail!\n");
		}
	}
	g_curr_mode = mode;
	return ret;
}

static int set_daynight_mode(LIGHT_MODE mode)
{
	if (mode == E_NIGHT_MODE) {
		printf("---------------------\n");
		printf("set mono mode\n");
		set_isp_daynight_mode(1);
		sleep(1);
		printf("set IRcut off\n");
		set_IRcut(0);
		printf("set LED on\n");
		set_LED(1);
	} else {
		printf("---------------------\n");
		printf("set LED off\n");
		set_LED(0);

		printf("set IRcut on\n");
		set_IRcut(1);

		printf("set color mode\n");
		set_isp_daynight_mode(0);
	}
	g_curr_mode = mode;
	return 0;
}

static int get_choose_int(void)
{
	CHAR buf[256];
	int val = 0, error;

	do {
		error = scanf("%d", &val);
		if (error != 1) {
			printf("Invalid option. Try again.\n");
			clearerr(stdin);
			fgets(buf, sizeof(buf), stdin);
			printf(">> ");
		}
	} while(error != 1);

	return val;
}

int run_test(void)
{
	int option;
	int curr_ev;
	int night_ev, day_ev, sensitive;

	while (1) {
		// Test device
		printf("----------------------------------------\n");
		printf(" 1. Set ISP mono mode.\n");
		printf(" 2. Set ISP color mode.\n");
		printf(" 3. Set IR-CUT on.\n");
		printf(" 4. Set IR-CUT off.\n");
		printf(" 5. Set LED on.\n");
		printf(" 6. Set LED off.\n");
		printf(" 7. Set night mode.\n");
		printf(" 8. Set day mode.\n");
		printf(" 9. Get current EV.\n");
		printf(" 10.Get parameter.\n");
		printf(" 11.Set parameter.\n");
		printf(" 0. Quit\n");
		printf("----------------------------------------\n");

		do {
			printf(">> ");
			option = get_choose_int();
		} while (0);

		switch (option) {
			case 1:
				set_isp_daynight_mode(1);
				printf("> Set ISP mono mode.\n");
				break;
			case 2:
				set_isp_daynight_mode(0);
				printf("> Set ISP color mode.\n");
				break;
			case 3:
				set_IRcut(1);
				printf("> Set IR-CUT on.\n");
				break;
			case 4:
				set_IRcut(0);
				printf("> Set IR-CUT off.\n");
				break;
			case 5:
				set_LED(1);
				printf(">Set LED on.\n");
				break;
			case 6:
				set_LED(0);
				printf("> Set LED off.\n");
				break;
			case 7:
				set_daynight_mode(1);
				printf("> Set night mode.\n");
				break;
			case 8:
				set_daynight_mode(0);
				printf("> Set day mode.\n");
				break;
			case 9:
				curr_ev = sw_cds_get_curr_ev();
				if (curr_ev < 0)
					printf("> Get current EV fail!\n");
				else
					printf("> Current EV = %d.\n", curr_ev);
				break;
			case 10:
				if (sw_cds_get_parameter(&night_ev, &day_ev, &sensitive) == 0) {
					printf("> Get parameter night_ev = %d, day_ev = %d, sensitive = %d\n", night_ev, day_ev, sensitive);
				} else {
					printf("Get parameter fail\n");
				}
				break;
			case 11:
					printf("night_ev >> ");
					night_ev = get_choose_int();
					printf("day_ev >> ");
					day_ev = get_choose_int();
					printf("sensitive >> ");
					sensitive = get_choose_int();
				if (sw_cds_set_parameter(night_ev, day_ev, sensitive) == 0) {
					printf("> Set parameter night_ev = %d, day_ev = %d, sensitive = %d\n", night_ev, day_ev, sensitive);
				} else {
					printf("Set parameter fail\n");
				}
				break;
			case 0:
				return 0;
			default:
				break;
		}
	}
}

int run_sw_cds(void)
{
	int ret = 0;
	while (while_loop == TRUE) {
		if (g_curr_mode == E_DAY_MODE) {
			ret = sw_cds_check_env_night();
			if (ret == E_NOT_READY) {
				printf("Check night not ready.\n");
			} else if (ret == 1) {
				//Calll to library for data collection before set mode to night.
				sw_cds_night_pre_cb();
				//Set LED on, IR-cut off, mono mode.
				set_daynight_mode(E_NIGHT_MODE);
				//Calll to library for data collection after set mode to night.
				sw_cds_night_post_cb();
			}
		} else { // current mode = NIGHT_MODE
			ret = sw_cds_check_env_day();
			if (ret == E_NOT_READY){
				printf("Check day not ready.\n");
			}
			else if (ret == 1){
				//Set LED off, IR-cut on, color mode.
				set_daynight_mode(E_DAY_MODE);
			}
		}
		sleep(1);
	}
	return 0;
}

int main(int argc, char *argv[])
{
	int night_ev_th = 200;
	int day_ev_th = 250;
	int sensitive = 5;
	int test_mode = 0;
	int open_device;
	INT rt = 0;

	if (argc > 6) {
		printf("Wrong parameter no.\n");
		return 1;
	}
	if (argc == 2 && strcmp(argv[1], "test") == 0) {
		test_mode = 1;
	} else {
		if (argc >= 3) {
			id = atoi(argv[2]);
			if (id > 2){
				printf("arg[0] id : 0 1 2\n");
				return 1;
			}

		}
		if (argc >= 4) {
			night_ev_th = atoi(argv[3]);
			if (night_ev_th <= 1){
				printf("arg[1] EV_TH >= 1 is must!\n");
				return 1;
			}

		}
		if (argc >= 5) {
			day_ev_th = atoi(argv[4]);
			if (day_ev_th < night_ev_th){
				printf("arg[2] day_ev_th must > night_ev_th\n");
				return 1;
			}
		}
		if (argc >= 6) {
			sensitive = atoi(argv[5]);
			if (sensitive > 10 || sensitive < 0) {
				printf("arg[3] Sensitivity range : 0 ~ 10\n");
				return 1;
			}
		}
	}

	open_device = vendor_isp_init();

	if (open_device < 0) {
		printf("open MCU device fail!\n");
		return E_GET_DEV_FAIL;
	}

	if (init_LED() < 0) {
		printf("init LED fail\n");
		goto _end;
	}

	if (init_IRcut() < 0) {
		printf("init IR cut fail\n");
		goto _end;
	}

	// Init light_sensing
	if(sw_cds_init(id, night_ev_th, day_ev_th, sensitive, open_device) < 0) {
		printf("light_sense_init fail!\n");
		goto _end;
	}
	// Set the max. EV value of your IR light.
	sw_cds_set_max_ir_ev(1200);

	set_daynight_mode(E_DAY_MODE);

	if (test_mode) {
		run_test();
	} else {
		printf("************************************************\n");
		printf("Nova SW CDS start.\n");
		printf("NIGHT_EV_TH=%d, DAY_EV_TH=%d, Sensitive=%d\n", night_ev_th, day_ev_th, sensitive);
		printf("************************************************\n");
		// Turn on light_sensing debug print
		sw_cds_set_dbg_out(1);
		run_sw_cds();
	}
_end:
	sw_cds_exit();
	rt = vendor_isp_uninit();
	if(rt != HD_OK) {
		printf("vendor_isp_uninit fail=%d\n", rt);
	}

	return 0;
}

