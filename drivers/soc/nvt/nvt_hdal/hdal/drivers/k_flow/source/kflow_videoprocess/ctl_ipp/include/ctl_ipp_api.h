#ifndef __module_api_h_
#define __module_api_h_
#include "ctl_ipp_drv.h"

/* test command */
BOOL nvt_ctl_ipp_api_kdf_testcmd(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_ctrl_testcmd(unsigned char argc, char **pargv);

/* common command */
BOOL nvt_ctl_ipp_api_set_out_path(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_in_crop(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_func_en(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_ispid(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_isp_cbtime_check(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_isp_dbg_mode(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_flip(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_bp(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_eng_hang_handle(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_strp_rule(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_dma_abort(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_path_order(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_set_path_region(unsigned char argc, char **pargv);

BOOL nvt_ctl_ipp_api_dump_ts(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_dump_hdl_all(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_dump_proc_info_all(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_saveyuv(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_dump_isp_item(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_dump_dtsi(unsigned char argc, char **pargv);

BOOL nvt_ctl_ipp_api_dbg_level(unsigned char argc, char **pargv);
BOOL nvt_ctl_ipp_api_dbg_primask(unsigned char argc, char **pargv);

extern void nvt_ctl_ipp_install_cmd(void);
extern void nvt_ctl_ipp_uninstall_cmd(void);
#if defined(__LINUX)
int ctl_ipp_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
