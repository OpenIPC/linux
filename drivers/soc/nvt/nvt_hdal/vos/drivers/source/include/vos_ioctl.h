#ifndef _VOS_INTERNAL_IOCTL_H_
#define _VOS_INTERNAL_IOCTL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <kwrap/error_no.h>
#include <kwrap/ioctl.h>
#include <kwrap/nvt_type.h>

#define VOS_IOCTL_DEV_NAME      "nvt_vos"
#define VOS_IOCTL_MAGIC         'V'

#define VOS_FLAG_NAME_SIZE      32
#define VOS_SEM_NAME_SIZE       32
#define VOS_TASK_NAME_SIZE      16
#define VOS_PERF_NAME_SIZE      16

typedef struct {
	unsigned long id;
	unsigned long mode;
	unsigned long bits;
	T_CFLG init_pattern; //used for creation
	char name[VOS_FLAG_NAME_SIZE];
	int timeout_tick;
	int interruptible;
} VOS_FLAG_IOARG;

typedef struct {
	char name[VOS_SEM_NAME_SIZE];
	int semid;
	int init_cnt;
	int timeout_tick; //Note: For Linux user-space: tick is the same as msec.
	int interruptible;
} VOS_SEM_IOARG;

typedef struct {
	unsigned long pthread_id;   //user-space pthread id
	int drv_task_id;            //task id of vos driver layer
} VOS_TASK_IOARG;

typedef struct {
	char name[VOS_TASK_NAME_SIZE];
	int drv_task_id;    //task id of vos driver task
	int vos_prio;       //vos priority
	void *user_fp;      //user-space function pointer
	void *user_parm;    //user-space function parameters
	int user_stksize;   //user-space stack size
} VOS_TASK_IOARG_UINFO;

typedef struct {
	char name[VOS_TASK_NAME_SIZE];
	unsigned long pthread_id;   //user-space pthread id
	unsigned long tid;          //thread id of pthread
	int drv_task_id;            //task id of vos driver task
	int vos_prio;               //vos priority priority
	void *user_fp;              //user-space function pointer
	void *user_parm;            //user-space function parameters
} VOS_TASK_IOARG_REG_N_RUN;

typedef struct {
	int drv_task_id;            //task id of vos driver task
	int vos_prio;               //vos priority priority
} VOS_TASK_IOARG_PRIORITY;

typedef struct {
	char name[VOS_PERF_NAME_SIZE];
	unsigned long line_no;
	unsigned long cus_val;
	unsigned long tick;
} VOS_PERF_IOARG;

typedef struct {
	unsigned long vaddr;
	unsigned long len;
	unsigned int dir;
	unsigned int is_vb;
} VOS_CPU_IOARG;

typedef struct {
	unsigned long vaddr;
	unsigned long len;
	unsigned int dir;
	unsigned int cpu_id;
} VOS_CPU_IOARG_SYNC_CPU;

typedef struct {
	unsigned long vaddr;
	unsigned long paddr;
} VOS_CPU_IOARG_VA_PA;

typedef struct {
	unsigned long reserved;
} VOS_DEBUG_IOARG;

#define VOS_FLAG_IOCMD_CREATE           _VOS_IOW(VOS_IOCTL_MAGIC,  0, VOS_FLAG_IOARG)
#define VOS_FLAG_IOCMD_SET              _VOS_IOW(VOS_IOCTL_MAGIC,  1, VOS_FLAG_IOARG)
#define VOS_FLAG_IOCMD_CLR              _VOS_IOW(VOS_IOCTL_MAGIC,  2, VOS_FLAG_IOARG)
#define VOS_FLAG_IOCMD_WAIT             _VOS_IOWR(VOS_IOCTL_MAGIC, 3, VOS_FLAG_IOARG)
#define VOS_FLAG_IOCMD_CHK              _VOS_IOWR(VOS_IOCTL_MAGIC, 4, VOS_FLAG_IOARG)
#define VOS_FLAG_IOCMD_DESTROY          _VOS_IOWR(VOS_IOCTL_MAGIC, 5, VOS_FLAG_IOARG)

#define VOS_SEM_IOCMD_CREATE            _VOS_IOW(VOS_IOCTL_MAGIC, 10, VOS_SEM_IOARG)
#define VOS_SEM_IOCMD_WAIT              _VOS_IOW(VOS_IOCTL_MAGIC, 11, VOS_SEM_IOARG)
#define VOS_SEM_IOCMD_SIG               _VOS_IOW(VOS_IOCTL_MAGIC, 12, VOS_SEM_IOARG)
#define VOS_SEM_IOCMD_DESTROY           _VOS_IOW(VOS_IOCTL_MAGIC, 13, VOS_SEM_IOARG)

#define VOS_TASK_IOCMD_REG_N_RUN        _VOS_IOW(VOS_IOCTL_MAGIC, 20, VOS_TASK_IOARG_REG_N_RUN)
#define VOS_TASK_IOCMD_DESTROY          _VOS_IOW(VOS_IOCTL_MAGIC, 21, VOS_TASK_IOARG)
#define VOS_TASK_IOCMD_RETURN           _VOS_IOW(VOS_IOCTL_MAGIC, 22, VOS_TASK_IOARG)
#define VOS_TASK_IOCMD_RESUME           _VOS_IOW(VOS_IOCTL_MAGIC, 23, VOS_TASK_IOARG)
#define VOS_TASK_IOCMD_CONVERT_HDL      _VOS_IOW(VOS_IOCTL_MAGIC, 24, VOS_TASK_IOARG)
#define VOS_TASK_IOCMD_SET_UINFO        _VOS_IOW(VOS_IOCTL_MAGIC, 25, VOS_TASK_IOARG_UINFO)
#define VOS_TASK_IOCMD_GET_UINFO        _VOS_IOW(VOS_IOCTL_MAGIC, 26, VOS_TASK_IOARG_UINFO)
#define VOS_TASK_IOCMD_SET_PRIORITY     _VOS_IOW(VOS_IOCTL_MAGIC, 27, VOS_TASK_IOARG_PRIORITY)

#define VOS_PERF_IOCMD_MARK             _VOS_IOW(VOS_IOCTL_MAGIC, 30, VOS_PERF_IOARG)
#define VOS_PERF_IOCMD_LIST_MARK        _VOS_IOW(VOS_IOCTL_MAGIC, 31, VOS_PERF_IOARG)
#define VOS_PERF_IOCMD_LIST_DUMP        _VOS_IOW(VOS_IOCTL_MAGIC, 32, VOS_PERF_IOARG)
#define VOS_PERF_IOCMD_LIST_RESET       _VOS_IOW(VOS_IOCTL_MAGIC, 33, VOS_PERF_IOARG)

#define VOS_CPU_IOCMD_DCACHE_SYNC       _VOS_IOW(VOS_IOCTL_MAGIC, 40, VOS_CPU_IOARG)
#define VOS_CPU_IOCMD_GET_PHY_ADDR      _VOS_IOW(VOS_IOCTL_MAGIC, 41, VOS_CPU_IOARG_VA_PA)
#define VOS_CPU_IOCMD_SYNC_CPU          _VOS_IOW(VOS_IOCTL_MAGIC, 42, VOS_CPU_IOARG_SYNC_CPU)

#define VOS_DEBUG_IOCMD_HALT            _VOS_IOW(VOS_IOCTL_MAGIC, 50, VOS_DEBUG_IOARG)

void vos_ioctl_init(void *param);
void vos_ioctl_exit(void);

int _IOFUNC_FLAG_IOCMD_CREATE(unsigned long arg);
int _IOFUNC_FLAG_IOCMD_SET(unsigned long arg);
int _IOFUNC_FLAG_IOCMD_CLR(unsigned long arg);
int _IOFUNC_FLAG_IOCMD_WAIT(unsigned long arg);
int _IOFUNC_FLAG_IOCMD_CHK(unsigned long arg);
int _IOFUNC_FLAG_IOCMD_DESTROY(unsigned long arg);

int _IOFUNC_SEM_IOCMD_CREATE(unsigned long arg);
int _IOFUNC_SEM_IOCMD_WAIT(unsigned long arg);
int _IOFUNC_SEM_IOCMD_SIG(unsigned long arg);
int _IOFUNC_SEM_IOCMD_DESTROY(unsigned long arg);

int _IOFUNC_TASK_IOCMD_REG_N_RUN(unsigned long arg);
int _IOFUNC_TASK_IOCMD_DESTROY(unsigned long arg);
int _IOFUNC_TASK_IOCMD_RETURN(unsigned long arg);
int _IOFUNC_TASK_IOCMD_RESUME(unsigned long arg);
int _IOFUNC_TASK_IOCMD_ENTER(unsigned long arg);
int _IOFUNC_TASK_IOCMD_CONVERT_HDL(unsigned long arg);
int _IOFUNC_TASK_IOCMD_GET_UINFO(unsigned long arg);
int _IOFUNC_TASK_IOCMD_SET_UINFO(unsigned long arg);
int _IOFUNC_TASK_IOCMD_SET_PRIORITY(unsigned long arg);

int _IOFUNC_PERF_IOCMD_MARK(unsigned long arg);
int _IOFUNC_PERF_IOCMD_LIST_MARK(unsigned long arg);
int _IOFUNC_PERF_IOCMD_LIST_DUMP(unsigned long arg);
int _IOFUNC_PERF_IOCMD_LIST_RESET(unsigned long arg);

int _IOFUNC_CPU_IOCMD_DCACHE_SYNC(unsigned long arg);
int _IOFUNC_CPU_IOCMD_GET_PHY_ADDR(unsigned long arg);
int _IOFUNC_CPU_IOCMD_SYNC_CPU(unsigned long arg);

int _IOFUNC_DEBUG_IOCMD_HALT(unsigned long arg);

#ifdef __cplusplus
}
#endif

#endif /* _VOS_INTERNAL_IOCTL_H_ */

