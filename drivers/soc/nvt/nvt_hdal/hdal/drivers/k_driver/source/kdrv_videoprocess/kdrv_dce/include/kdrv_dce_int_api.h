#ifndef __KDRV_DCE_INT_API_H_
#define __KDRV_DCE_INT_API_H_

extern void nvt_kdrv_dce_install_cmd(void);
extern void nvt_kdrv_dce_uninstall_cmd(void);
#if defined(__LINUX)
int kdrv_dce_cmd_execute(unsigned char argc, char **argv);
#endif

#endif

