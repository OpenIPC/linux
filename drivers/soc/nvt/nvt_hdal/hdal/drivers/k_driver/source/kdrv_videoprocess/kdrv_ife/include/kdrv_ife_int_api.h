#ifndef __KDRV_IFE_INT_API_H_
#define __KDRV_IFE_INT_API_H_

extern void nvt_kdrv_ife_install_cmd(void);
extern void nvt_kdrv_ife_uninstall_cmd(void);
#if defined(__LINUX)
int kdrv_ife_cmd_execute(unsigned char argc, char **argv);
#endif

#endif

