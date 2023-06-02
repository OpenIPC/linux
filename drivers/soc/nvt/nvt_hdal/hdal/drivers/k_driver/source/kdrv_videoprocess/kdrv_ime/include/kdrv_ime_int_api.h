#ifndef __KDRV_IME_INT_API_H_
#define __KDRV_IME_INT_API_H_

extern void nvt_kdrv_ime_install_cmd(void);
extern void nvt_kdrv_ime_uninstall_cmd(void);
#if defined(__LINUX)
int kdrv_ime_cmd_execute(unsigned char argc, char **argv);
#endif

#endif

