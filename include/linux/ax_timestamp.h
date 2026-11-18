#ifndef __AX_TIMESTAT_H_
#define __AX_TIMESTAT_H_
#define AX_ID_KERNEL	46
#define AX_SUB_ID_SUSPEND_END	1
#define AX_SUB_ID_RESUME_START	0
unsigned int ax_sys_sleeptimestamp(int modid, unsigned int subid);
void ax_sys_sleeptimestamp_print(void);
#endif
