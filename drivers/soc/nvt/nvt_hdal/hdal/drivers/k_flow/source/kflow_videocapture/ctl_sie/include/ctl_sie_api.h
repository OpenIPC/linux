#ifndef __module_api_h_
#define __module_api_h_
#include "ctl_sie_drv.h"
#include "kwrap/type.h"

#define CTL_SIE_TEST_CMD DISABLE //test cmd, for develop using
#if CTL_SIE_TEST_CMD
typedef enum {
	CTL_SIE_TEST_FUNC_NONE =   0x00000000,
	CTL_SIE_TEST_FUNC_CA =     0x00000001,  //680/520/528
	CTL_SIE_TEST_FUNC_LA =     0x00000002,  //680/520/528
	CTL_SIE_TEST_FUNC_ENCODE = 0x00000020,  //Raw only
} CTL_SIE_TEST_FUNC;

#define CTL_SIE_TEST_CCIR_SENSOR_FLOW   DISABLE // ENABLE: ccir sensor flow
#define CTL_SIE_TEST_PAT_GEN            ENABLE  // ENABLE: sie pattern gen mode, DISABLE: Sensor in
#define CTL_SIE_TEST_DIRECT_FLOW        DISABLE // ENABLE: SIE to IPP direct mode flow enable, DISABLE: Dram mode
#define CTL_SIE_TEST_PAT_GEN_SZ_W       1920//3840
#define CTL_SIE_TEST_PAT_GEN_SZ_H       1080//2160
#define CTL_SIE_TEST_DVS_SENSOR_SIM     DISABLE
#if CTL_SIE_TEST_CCIR_SENSOR_FLOW
#define CTL_SIE_TEST_FUNC_EN CTL_SIE_TEST_FUNC_NONE
#elif CTL_SIE_TEST_DVS_SENSOR_SIM
#define CTL_SIE_TEST_FUNC_EN CTL_SIE_TEST_FUNC_NONE
#else
#define CTL_SIE_TEST_FUNC_EN (CTL_SIE_TEST_FUNC_CA | CTL_SIE_TEST_FUNC_LA)
#endif
#endif

BOOL ctl_sie_cmd_set_dbg_type(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_set_dbg_level(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_save_mem(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_save_raw(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_set_isp_dbg_type(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_set_fb_dump(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_force_pat_gen(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_dump_signal(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_ccir_header_chk(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_manual(unsigned char argc, char **pargv);
#if CTL_SIE_TEST_CMD//Test Cmd
BOOL ctl_sie_cmd_dump_ca_rst(UINT32 id);
BOOL ctl_sie_cmd_dump_la_rst(UINT32 id);
BOOL ctl_sie_cmd_get_statis(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_on(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_off(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_open(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_close(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_start(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_stop(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_trig(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_set_iosize(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_test_cmd(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_isp_cfg_eth(unsigned char argc, char **pargv);
BOOL ctl_sie_cmd_test(unsigned char argc, char **pargv);

#endif

#if defined(__LINUX)
int ctl_sie_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
