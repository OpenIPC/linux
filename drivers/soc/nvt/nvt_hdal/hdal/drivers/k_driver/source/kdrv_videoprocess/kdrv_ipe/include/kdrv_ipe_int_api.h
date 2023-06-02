#ifndef __KDRV_IPE_INT_API_H_
#define __KDRV_IPE_INT_API_H_

extern void nvt_kdrv_ipe_install_cmd(void);
extern void nvt_kdrv_ipe_uninstall_cmd(void);
#if defined(__LINUX)
int kdrv_ipe_cmd_execute(unsigned char argc, char **argv);
#endif

#endif //__KDRV_IPE_INT_API_H_

