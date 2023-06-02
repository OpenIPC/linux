#ifndef __KDRV_ISE_INT_API_H__
#define __KDRV_ISE_INT_API_H__

extern void nvt_kdrv_ise_install_cmd(void);
extern void nvt_kdrv_ise_uninstall_cmd(void);
#if defined(__LINUX)
int kdrv_ise_cmd_execute(unsigned char argc, char **argv);
#endif

#endif
