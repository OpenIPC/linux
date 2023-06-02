#ifndef __KDRV_IPP_INT_API_H__
#define __KDRV_IPP_INT_API_H__

extern void nvt_kdrv_ipp_install_cmd(void);
extern void nvt_kdrv_ipp_uninstall_cmd(void);
#if defined(__LINUX)
int kdrv_ipp_cmd_execute(unsigned char argc, char **argv);
#endif

#endif

