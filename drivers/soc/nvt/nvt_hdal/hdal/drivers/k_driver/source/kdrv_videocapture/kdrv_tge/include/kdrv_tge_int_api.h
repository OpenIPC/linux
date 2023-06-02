#ifndef _KDRV_TGE_INT_API_H_
#define _KDRV_TGE_INT_API_H_

extern void nvt_kdrv_tge_cmd_help(void);
#if defined(__LINUX)
int kdrv_tge_cmd_execute(unsigned char argc, char **argv);
#endif

#endif //_KDRV_TGE_INT_API_H_