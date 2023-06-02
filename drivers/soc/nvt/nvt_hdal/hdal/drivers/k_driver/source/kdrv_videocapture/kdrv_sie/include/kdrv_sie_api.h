#ifndef __KDRV_SIE_API_H_
#define __KDRV_SIE_API_H_

#define KDRV_SIE_TEST_CMD DISABLE //test cmd, for develop using

#if KDRV_SIE_TEST_CMD
typedef enum {
	EXAM_KDRV_SIE_FUNC_NONE =   0x00000000,
	EXAM_KDRV_SIE_FUNC_CA =     0x00000001,
	EXAM_KDRV_SIE_FUNC_LA =     0x00000002,
	EXAM_KDRV_SIE_FUNC_ENCODE = 0x00000004,	//not support in ccir sensor
} EXAM_KDRV_SIE_FUNC;

#define KDRV_SIE_TEST_CMD_PAT_GEN_EN ENABLE	//disable for sensor in
#define KDRV_SIE_TEST_CMD_FUNC_EN (EXAM_KDRV_SIE_FUNC_CA|EXAM_KDRV_SIE_FUNC_LA)

#define SIE_OUT_PUB_BUF_W 2880
#define SIE_OUT_PUB_BUF_H 1080

BOOL kdrv_sie_test_cmd_open(unsigned char argc, char **pargv);
BOOL kdrv_sie_test_cmd_close(unsigned char argc, char **pargv);
BOOL kdrv_sie_test_cmd_start(unsigned char argc, char **pargv);
BOOL kdrv_sie_test_cmd_stop(unsigned char argc, char **pargv);
BOOL kdrv_sie_test_cmd_on(unsigned char argc, char **pargv);
BOOL kdrv_sie_test_cmd_off(unsigned char argc, char **pargv);
#endif

BOOL kdrv_sie_cmd_set_dbg_type(unsigned char argc, char **pargv);
BOOL kdrv_sie_cmd_set_dbg_level(unsigned char argc, char **pargv);
BOOL kdrv_sie_cmd_save_raw(unsigned char argc, char **pargv);
BOOL kdrv_sie_cmd_dbg_func( unsigned char argc, char **pargv);

#if defined(__LINUX)
int kdrv_sie_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
