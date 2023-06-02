/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <linux/version.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
#include <uapi/linux/sched/types.h> // For struct sched_param
#include <linux/sched/debug.h> // For sched_show_task()
#endif
#include <linux/fs.h>
#include <linux/ioctl.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/memory.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>

#include <kwrap/cpu.h>
#include <kwrap/flag.h>
#include <kwrap/task.h>
#include <kwrap/semaphore.h>
#include <kwrap/spinlock.h>
#include <kwrap/util.h> //for obsolete vos_task_delay_xxx API
#include "vos_ioctl.h"

#define __MODULE__    rtos_task
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_TASK_INITED_TAG    MAKEFOURCC('R', 'T', 'S', 'K') ///< a key value
#define VOS_TASK_MIN_VALID_ID   1
#define VOS_TASK_USER_TSK_NAME  "user_tsk"

#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)

#define VOS_PRIO_TO_USER_NICE(prio) ((prio) - VK_TASK_HIGHEST_PRIORITY + MIN_NICE)
#define USER_NICE_TO_VOS_PRIO(nice) ((nice) - MIN_NICE + VK_TASK_HIGHEST_PRIORITY)

#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#if defined(__LINUX)
#define my_alloc    vmalloc
#define my_free     vfree
#define my_jiffies  jiffies_64
#define my_jiff_to_ms(j) ((j) * (1000 / HZ))

#elif defined(__FREERTOS)
#define my_alloc    malloc
#define my_free     free
#define current     xTaskGetCurrentTaskHandle()
#define my_jiffies  hwclock_get_longcounter()
#define my_jiff_to_ms(j) ((j) / 1000)

#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
#error Not supported OS
#endif

typedef struct {
	UINT                        st_used;
	struct task_struct          *task_hdl;
	UINT32                      wait_res;
	FLGPTN                      waiptn;
	UINT                        wfmode;
	ID                          wairesid;
	UINT64                      wait_time_jiffies;
	int                         vos_prio;   //vos priority
	unsigned long               pthread_id; //only used by user-space ioctl
	char                        pthread_name[VOS_TASK_NAME_SIZE];
	void                        *user_fp;       //user-space function pointer
	void                        *user_parm;     //user-space function parameters
	int                         user_stksize;   //user-space stack size
} TASK_CELL_T;

typedef struct {
	TASK_CELL_T            *p_tasks;
} TASK_CTRL_T;

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_task_debug_level = NVT_DBG_WRN;

module_param_named(rtos_task_debug_level, rtos_task_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rtos_task_debug_level, "Debug message level");

static  VK_DEFINE_SPINLOCK(my_lock);

static int            gtasknum;
static int            g_cur_task_id = VOS_TASK_MIN_VALID_ID;
static TASK_CTRL_T    g_st_task_ctrl;

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static char *policy2str(int policy)
{
	switch (policy) {
	case SCHED_NORMAL:
		return "SCHED_NORMAL";

	case SCHED_FIFO:
		return "SCHED_FIFO";

	case SCHED_RR:
		return "SCHED_RR";

	case SCHED_BATCH:
		return "SCHED_BATCH";

	case SCHED_IDLE:
		return "SCHED_IDLE";

	case SCHED_DEADLINE:
		return "SCHED_DEADLINE";

	default:
		return NULL;
	}
}

static char *get_task_state_str(int state)
{
	switch (state) {
	case TASK_RUNNING:
		return "Running";

	case TASK_INTERRUPTIBLE:
		return "Sleep-Interruptible";

	case TASK_UNINTERRUPTIBLE:
		return "Sleep-Un-Interruptible";

	case __TASK_STOPPED:
		return "STOPPED";

	case __TASK_TRACED:
		return "TRACED";

	case EXIT_DEAD:
		return "DEAD";

	case EXIT_ZOMBIE:
		return "ZOMBIE";

	default:
		return NULL;
	}
}

void rtos_task_init(unsigned long max_task_num)
{
	DBG_FUNC_BEGIN("\r\n");
	DBG_IND("max_task_num =%ld\r\n", max_task_num);

	// initialize value
	gtasknum = max_task_num;
	g_cur_task_id = VOS_TASK_MIN_VALID_ID;

	g_st_task_ctrl.p_tasks = (TASK_CELL_T *)my_alloc(sizeof(TASK_CELL_T) * gtasknum);
	if (g_st_task_ctrl.p_tasks == NULL) {
		DBG_ERR(" alloc!\n");
		goto END_INIT;
	}
	DBG_IND("p_tasks =0x%lx\r\n", (ULONG)g_st_task_ctrl.p_tasks);

	memset(g_st_task_ctrl.p_tasks, 0x00, sizeof(TASK_CELL_T) * gtasknum);

END_INIT:
	DBG_FUNC_END("\r\n");
	return;
}

/**
   rtos_exit_flag
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
void rtos_task_exit(void)
{
	my_free(g_st_task_ctrl.p_tasks);
	g_st_task_ctrl.p_tasks = NULL;
}

int vos_task_debug_dump(int argc, char** argv)
{
	TASK_CELL_T *p_tskcell;
	int i;
	unsigned long       flags = 0;

	loc_cpu(flags);

	for (i = 0; i < gtasknum; i++) {
		p_tskcell = g_st_task_ctrl.p_tasks + i;
		DBG_DUMP("[%02d] tag 0x%08X pth 0x%08X %-16s ktsk 0x%08lX %s\r\n",
			i,
			p_tskcell->st_used,
			(UINT)p_tskcell->pthread_id, p_tskcell->pthread_name,
			(ULONG)p_tskcell->task_hdl, p_tskcell->task_hdl->comm);
	}

	unl_cpu(flags);

	return 0;
}

static int vos_task_get_free_id_p(void)
{
	TASK_CELL_T        *p_tskcell;
	int                 cur_id, i;
	unsigned long       flags = 0;

	loc_cpu(flags);
	if (unlikely(g_cur_task_id >= gtasknum)) {
		// reset id
		g_cur_task_id = VOS_TASK_MIN_VALID_ID;
	}
	cur_id = g_cur_task_id;
	for (i = 0; i < gtasknum; i++) {
		if (unlikely(cur_id >= gtasknum)) {
			cur_id = VOS_TASK_MIN_VALID_ID;
		}
		p_tskcell = g_st_task_ctrl.p_tasks + cur_id;
		if (0 == p_tskcell->st_used) {
			p_tskcell->st_used = RTOS_TASK_INITED_TAG;
			g_cur_task_id = cur_id;
			g_cur_task_id++;
			unl_cpu(flags);
			DBG_IND("g_cur_task_id=%d\r\n", cur_id);
			return cur_id;
		}
		cur_id++;
	}
	unl_cpu(flags);
	return -1;
}

int vos_task_hdl2id(struct task_struct *task_hdl)
{
	//int                   task_id;
	int                         i;
	TASK_CELL_T        *p_tskcell;

	if (task_hdl == NULL) {
		return -1;
	}
	#if 0
	if (task_hdl->comm[0] >= 0x30 && task_hdl->comm[0] <= 0x39) {
		task_id = (task_hdl->comm[0]-0x30)*10 + (task_hdl->comm[1]-0x30);
		if (task_id > 0 && task_id < gtasknum) {
			p_tskcell = g_st_task_ctrl.p_tasks + task_id;
			if (RTOS_TASK_INITED_TAG == p_tskcell->st_used && task_hdl == p_tskcell->task_hdl) {
				return task_id;
			}
		}
	}
	#endif
	p_tskcell = g_st_task_ctrl.p_tasks + VOS_TASK_MIN_VALID_ID;
	for (i = VOS_TASK_MIN_VALID_ID; i < gtasknum; i++) {
		if (task_hdl == p_tskcell->task_hdl && RTOS_TASK_INITED_TAG == p_tskcell->st_used) {
			return i;
		}
		p_tskcell++;
	}
	return -1;
}

static TASK_CELL_T *vos_task_hdl2cell_p(struct task_struct *task_hdl)
{
	//int                 task_id;
	int                       i;
	TASK_CELL_T        *p_tskcell;

	if (task_hdl == NULL) {
		return NULL;
	}
	#if 0
	if (task_hdl->comm[0] >= 0x30 && task_hdl->comm[0] <= 0x39) {
		task_id = (task_hdl->comm[0]-0x30)*10 + (task_hdl->comm[1]-0x30);
		if (task_id > 0 && task_id < gtasknum) {
			p_tskcell = g_st_task_ctrl.p_tasks + task_id;
			if (RTOS_TASK_INITED_TAG == p_tskcell->st_used && task_hdl == p_tskcell->task_hdl) {
				return p_tskcell;
			}
		}
	}
	#endif
	p_tskcell = g_st_task_ctrl.p_tasks + VOS_TASK_MIN_VALID_ID;
	for (i = VOS_TASK_MIN_VALID_ID; i < gtasknum; i++) {
		if (RTOS_TASK_INITED_TAG == p_tskcell->st_used && task_hdl == p_tskcell->task_hdl) {
			return p_tskcell;
		}
		p_tskcell++;
	}
	return NULL;
}

static TASK_CELL_T* vos_task_pthreadid2cell_p(unsigned long pthread_id)
{
	int	i;
	TASK_CELL_T *p_tskcell;

	if (0 == pthread_id) {
		return NULL;
	}

	p_tskcell = g_st_task_ctrl.p_tasks + VOS_TASK_MIN_VALID_ID;
	for (i = VOS_TASK_MIN_VALID_ID; i < gtasknum; i++) {
		if (RTOS_TASK_INITED_TAG == p_tskcell->st_used && pthread_id == p_tskcell->pthread_id) {
			return p_tskcell;
		}
		p_tskcell++;
	}
	return NULL;
}

static int vos_task_pthreadid2id_p(unsigned long pthread_id)
{
	int	i;
	TASK_CELL_T *p_tskcell;

	if (0 == pthread_id) {
		return -1;
	}

	p_tskcell = g_st_task_ctrl.p_tasks + VOS_TASK_MIN_VALID_ID;
	for (i = VOS_TASK_MIN_VALID_ID; i < gtasknum; i++) {
		if (RTOS_TASK_INITED_TAG == p_tskcell->st_used && pthread_id == p_tskcell->pthread_id) {
			return i;
		}
		p_tskcell++;
	}
	return -1;
}

static struct task_struct * vos_task_tid2hdl(int tid)
{
	struct pid *p_pid;

	p_pid = find_vpid(tid);
	if (NULL == p_pid) {
		DBG_ERR("find_vpid fail, tid %d\r\n", tid);
		return NULL;
	}

	return pid_task(p_pid, PIDTYPE_PID);
}

static TASK_CELL_T* vos_task_id2cell(int drv_task_id)
{
	if (drv_task_id < VOS_TASK_MIN_VALID_ID || drv_task_id >= gtasknum) {
		DBG_ERR("Invalid drv_task_id %d\r\n", drv_task_id);
		return NULL;
	}

	return g_st_task_ctrl.p_tasks + drv_task_id;
}

static void vos_task_clear_info_p(TASK_CELL_T   *p_tskcell)
{
	if (RTOS_TASK_INITED_TAG == p_tskcell->st_used) {
		p_tskcell->task_hdl = 0;
		p_tskcell->waiptn = 0;
		p_tskcell->wfmode = 0;
		p_tskcell->wairesid = 0;
		p_tskcell->pthread_id = 0;
		p_tskcell->pthread_name[0] = '\0';
		p_tskcell->st_used = 0;
	}
}

static BOOL vos_task_chk_valid_p(TASK_CELL_T   *p_tskcell)
{
	char          *task_state_str;

	if (RTOS_TASK_INITED_TAG != p_tskcell->st_used) {
		return FALSE;
	}
	if (0 == p_tskcell->task_hdl) {
		return FALSE;
	}
	task_state_str = get_task_state_str(p_tskcell->task_hdl->state);
	if (NULL == task_state_str) {
		DBG_IND("\r\nInvalid state\r\n");
		return FALSE;
	}
	return TRUE;
}

VK_TASK_HANDLE vos_task_get_handle(void)
{
	return current;
}

int vos_task_get_name(VK_TASK_HANDLE task_hdl, char *name, unsigned int len)
{
	if (NULL == task_hdl) {
		return -1;
	}

	if (NULL == name) {
		return -1;
	}

	if (NULL == get_task_state_str(task_hdl->state)) {
		return -1;
	}

	if (virt_addr_valid(&task_hdl->comm[0])) {
		VOS_STRCPY(name, task_hdl->comm, len);
	} else {
		VOS_STRCPY(name, VOS_TASK_USER_TSK_NAME, len);
	}

	return 0;
}

int vos_task_get_tid(void)
{
	return current->pid;
}

static int vos_task_check_priority(int priority)
{
	if (priority > VK_TASK_LOWEST_PRIORITY || priority < VK_TASK_HIGHEST_PRIORITY) {
		DBG_ERR("Invalid %d, Lowest(%d) ~ Highest(%d)\r\n", priority, VK_TASK_LOWEST_PRIORITY, VK_TASK_HIGHEST_PRIORITY);
		return -1;
	}

	return 0;
}

void vos_task_set_priority(VK_TASK_HANDLE task_hdl, int priority)
{
	struct sched_param param = {0};
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		DBG_WRN("unknown task_hdl 0x%lx, skip\r\n", (ULONG)task_hdl);
		return;
	}

	if (0 != vos_task_check_priority(priority)) {
		DBG_ERR("skip, task_hdl 0x%lx\r\n", (ULONG)task_hdl);
		return;
	}

	param.sched_priority = 99 - priority;
	if (0 != sched_setscheduler(task_hdl, SCHED_RR, &param)) {
		DBG_ERR("sched_setscheduler failed\r\n");
	} else {
		//success
		p_tskcell->vos_prio = priority;
	}
}

BOOL vos_task_chk_hdl_valid(VK_TASK_HANDLE task_hdl)
{
	char          *task_state_str;

	if (task_hdl == NULL) {
		return FALSE;
	}
	task_state_str = get_task_state_str(((struct task_struct *)task_hdl)->state);
	if (NULL == task_state_str) {
		return FALSE;
	}
	return TRUE;
}

struct task_struct *vos_task_create(void *fp, void *parm, const char name[], int priority, int stksz)
{
	int                 task_id;
	TASK_CELL_T        *p_tskcell;

	// check priority if priority not zero
	if (0 != priority) {
		if (0 != vos_task_check_priority(priority)) {
			DBG_ERR("check priority %d fail\r\n", priority);
			return NULL;
		}
	}

	task_id = vos_task_get_free_id_p();
	if (task_id < 0) {
		DBG_ERR("exceed max task num %d\r\n", gtasknum);
		return NULL;
	}

	p_tskcell = g_st_task_ctrl.p_tasks + task_id;

	p_tskcell->task_hdl = kthread_create(fp, parm, name);
	if (IS_ERR(p_tskcell->task_hdl)) {
		p_tskcell->task_hdl = NULL;
		p_tskcell->st_used = 0;
		DBG_ERR("kthread_create fail\r\n");
		return NULL;
	}

	if (0 != priority) {
		p_tskcell->vos_prio = priority;
		vos_task_set_priority(p_tskcell->task_hdl, priority);
	}

	return p_tskcell->task_hdl;
}

int vos_task_resume(struct task_struct *task_hdl)
{
	#if 0
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		DBG_WRN("unknown task_hdl 0x%x, skip\r\n", (int)task_hdl);
		return -1;
	}
	#endif
	if (NULL == task_hdl) {
		DBG_WRN("task_hdl 0x%lx, skip\r\n", (ULONG)task_hdl);
		return -1;
	}
	wake_up_process(task_hdl);
	return 0;

}

void vos_task_destroy(struct task_struct *task_hdl)
{
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		DBG_WRN("unknown task_hdl 0x%lx, skip\r\n", (ULONG)task_hdl);
		return;
	}
	kthread_stop(task_hdl);
	vos_task_clear_info_p(p_tskcell);
}

void vos_task_enter(void)
{
}

int vos_task_return(int rtn_val)
{
	TASK_CELL_T        *p_tskcell;
	struct task_struct *task_hdl;

	task_hdl = current;
	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		//DBG_ERR("Invalid task_hdl 0x%lx\r\n", (ULONG)task_hdl);
		return rtn_val;
	}
	vos_task_clear_info_p(p_tskcell);
	return rtn_val;
}

void vos_task_delay_ms(int ms)
{
	//obsolete API, redirect to vos_util_delay_xxx API
	vos_util_delay_ms(ms);
}

void vos_task_delay_us(int us)
{
	//obsolete API, redirect to vos_util_delay_xxx API
	vos_util_delay_us(us);
}

void vos_task_delay_us_polling(int us)
{
	//obsolete API, redirect to vos_util_delay_xxx API
	vos_util_delay_us_polling(us);
}

void vos_task_update_info(int wait_res, struct task_struct *task_hdl, ID wairesid, FLGPTN waiptn, UINT wfmode)
{
	TASK_CELL_T        *p_tskcell;

	p_tskcell = vos_task_hdl2cell_p(task_hdl);
	if (NULL == p_tskcell) {
		//DBG_ERR("Invalid task_hdl 0x%x\r\n", (int)task_hdl);
		return;
	}
	p_tskcell->wait_res = wait_res;
	p_tskcell->waiptn = waiptn;            // wait flag pattern
	p_tskcell->wfmode = wfmode;            // wait mode (OR,AND)
	p_tskcell->wairesid = wairesid;
	p_tskcell->wait_time_jiffies = my_jiffies;
}

#if defined __UITRON || defined __ECOS
void vos_task_dump(void (*dump)(char *fmt, ...))
#else
void vos_task_dump(int (*dump)(const char *fmt, ...))
#endif
{
	int                 i, task_count = 0;
	char               *task_state_str;
	TASK_CELL_T        *p_tskcell;
	struct task_struct *task_hdl;
	UINT64              sleep_time_ms;
	char                task_name[TASK_COMM_LEN];
	struct thread_info *p_thread_info;

	DBG_DUMP("\r\n-------------------------TASK -------------------------------------------------------\r\n");
	DBG_DUMP(" thread_info size = %ld\r\n", (ULONG)sizeof(struct thread_info));
	for (i = 0; i < gtasknum; i++) {
		p_tskcell = g_st_task_ctrl.p_tasks + i;
		task_hdl = p_tskcell->task_hdl;
		if (FALSE == vos_task_chk_valid_p(p_tskcell)) {
			continue;
		}
		task_count++;
		task_state_str = get_task_state_str(task_hdl->state);
		if (NULL != task_state_str) {
			if (virt_addr_valid(&task_hdl->comm[0])) {
				VOS_STRCPY(task_name, task_hdl->comm, sizeof(task_name));
			} else {
				VOS_STRCPY(task_name, VOS_TASK_USER_TSK_NAME, sizeof(task_name));
			}
			DBG_DUMP("\r\nTask[%3d, %16s] -> VOS_PRI: %2d, OS_PRI: %3d (%s, NI:%3d, %s:%3d)\r\n",
				i, task_name, p_tskcell->vos_prio, task_hdl->normal_prio,
				(task_hdl->flags & PF_KTHREAD) ? " KER":"USER", task_nice(task_hdl), policy2str(task_hdl->policy), task_hdl->rt_priority);

			DBG_DUMP("Status : [%s] \r\n", task_state_str);
			if (0 != p_tskcell->wairesid && VOS_RES_TYPE_FLAG == p_tskcell->wait_res) {
				sleep_time_ms = my_jiff_to_ms(my_jiffies - p_tskcell->wait_time_jiffies);
				if (p_tskcell->wfmode & TWF_ORW) {
					DBG_DUMP("Wait for Flag [%3d, %32s], Pattern: 0x%08X OR, sleep_time = %lld ms\r\n",
							p_tskcell->wairesid, vos_flag_get_name(p_tskcell->wairesid), p_tskcell->waiptn, sleep_time_ms);
				} else {
				    DBG_DUMP("Wait for Flag [%3d, %32s], Pattern: 0x%08X AND, sleep_time = %lld ms\r\n",
							p_tskcell->wairesid, vos_flag_get_name(p_tskcell->wairesid), p_tskcell->waiptn, sleep_time_ms);
				}
			} else if (0 != p_tskcell->wairesid && VOS_RES_TYPE_SEM == p_tskcell->wait_res) {
				sleep_time_ms = my_jiff_to_ms(my_jiffies - p_tskcell->wait_time_jiffies);
				DBG_DUMP("Wait for Semaphore [%.3d, %32s], sleep_time = %lld ms \r\n", p_tskcell->wairesid,
					      vos_sem_get_name(p_tskcell->wairesid), sleep_time_ms);
			}

			p_thread_info = task_thread_info(task_hdl);
			DBG_DUMP("Stack Area: va[0x%.8lX ~ 0x%.8lX], pa[0x%.8lX ~ 0x%.8lX], saved_sp 0x%.8lX\r\n",
				(ULONG)p_thread_info,
				(ULONG)((VOS_ADDR)p_thread_info + THREAD_SIZE),
				(ULONG)vos_cpu_get_phy_addr((VOS_ADDR)p_thread_info),
				(ULONG)vos_cpu_get_phy_addr((VOS_ADDR)p_thread_info + THREAD_SIZE),
				(ULONG)thread_saved_sp(task_hdl));
		} else {
			continue;
		}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
		sched_show_task(task_hdl);
#else
		show_stack(task_hdl, NULL);
#endif
	}
	DBG_DUMP("\r\n Max task_count = %d , used = %d\r\n", gtasknum, task_count);
}

void vos_task_dump_by_tskhdl(int (*dump)(const char *fmt, ...), struct task_struct *task_hdl)
{
	char               *task_state_str;
	char                task_name[TASK_COMM_LEN];

	if (NULL == task_hdl) {
		return;
	}
	task_state_str = get_task_state_str(task_hdl->state);
	if (NULL != task_state_str) {
		if (virt_addr_valid(&task_hdl->comm[0])) {
			VOS_STRCPY(task_name, task_hdl->comm, sizeof(task_name));
		} else {
			VOS_STRCPY(task_name, VOS_TASK_USER_TSK_NAME, sizeof(task_name));
		}
		DBG_DUMP("\r\nTask[%s] -> VOS_PRI NA, OS_PRI %3d (%s, NI:%3d, %s:%3d)\r\n",
			task_name, task_hdl->normal_prio,
			(task_hdl->flags & PF_KTHREAD) ? " KER":"USER", task_nice(task_hdl), policy2str(task_hdl->policy), task_hdl->rt_priority);

		DBG_DUMP("Status : [%s] \r\n", task_state_str);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 11, 0))
		sched_show_task(task_hdl);
#else
		show_stack(task_hdl, NULL);
#endif
		DBG_DUMP("\r\n");
	}

}

int _IOFUNC_TASK_IOCMD_SET_UINFO(unsigned long arg)
{
	VOS_TASK_IOARG_UINFO ioarg = {0};
	TASK_CELL_T *p_tskcell;
	int new_id;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG_UINFO))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	//new and get the cell pointer
	new_id = vos_task_get_free_id_p();
	if (new_id < 0) {
		DBG_ERR("get_free_id failed\r\n");
		return -EFAULT;
	}
	p_tskcell = g_st_task_ctrl.p_tasks + new_id;

	//store data to the cell
	p_tskcell->vos_prio = ioarg.vos_prio;
	VOS_STRCPY(p_tskcell->pthread_name, ioarg.name, sizeof(p_tskcell->pthread_name));
	p_tskcell->user_fp = ioarg.user_fp;
	p_tskcell->user_parm = ioarg.user_parm;
	p_tskcell->user_stksize = ioarg.user_stksize;

	//return to users
	ioarg.drv_task_id = new_id;

	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_TASK_IOARG_UINFO))) {
		return -EFAULT;
	}

	return 0;
}

int _IOFUNC_TASK_IOCMD_GET_UINFO(unsigned long arg)
{
	VOS_TASK_IOARG_UINFO ioarg = {0};
	TASK_CELL_T *p_tskcell;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG_UINFO))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	//get the cell pointer
	p_tskcell = vos_task_id2cell(ioarg.drv_task_id);
	if (NULL == p_tskcell) {
		DBG_ERR("get cell failed, drv_task_id %d\r\n", ioarg.drv_task_id);
		return -EINVAL;
	}

	//return to users
	ioarg.vos_prio = p_tskcell->vos_prio;
	VOS_STRCPY(ioarg.name, p_tskcell->pthread_name, sizeof(ioarg.name));
	ioarg.user_fp = p_tskcell->user_fp;
	ioarg.user_parm = p_tskcell->user_parm;
	ioarg.user_stksize = p_tskcell->user_stksize;

	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_TASK_IOARG_UINFO))) {
		return -EFAULT;
	}

	return 0;
}

int _IOFUNC_TASK_IOCMD_REG_N_RUN(unsigned long arg)
{
	VOS_TASK_IOARG_REG_N_RUN ioarg = {0};
	TASK_CELL_T *p_tskcell;
	struct task_struct *p_hdl;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG_REG_N_RUN))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	p_hdl = vos_task_tid2hdl((int)ioarg.tid);
	if (NULL == p_hdl) {
		DBG_ERR("tid2hdl failed, tid %d\r\n", (int)ioarg.tid);
		return -EFAULT;
	}

	//get the cell pointer
	p_tskcell = vos_task_id2cell(ioarg.drv_task_id);
	if (NULL == p_tskcell) {
		DBG_ERR("get cell failed, drv_task_id %d\r\n", ioarg.drv_task_id);
		return -EINVAL;
	}

	//register
	p_tskcell->task_hdl = p_hdl;
	p_tskcell->pthread_id = ioarg.pthread_id;

	//set priority (nice value)
	set_user_nice(p_hdl, VOS_PRIO_TO_USER_NICE(p_tskcell->vos_prio));

	//return to users
	VOS_STRCPY(ioarg.name, p_tskcell->pthread_name, sizeof(ioarg.name));
	ioarg.user_fp = p_tskcell->user_fp;
	ioarg.user_parm = p_tskcell->user_parm;
	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_TASK_IOARG_REG_N_RUN))) {
		return -EFAULT;
	}

	return 0;
}

int _IOFUNC_TASK_IOCMD_SET_PRIORITY(unsigned long arg)
{
	VOS_TASK_IOARG_PRIORITY ioarg = {0};
	TASK_CELL_T *p_tskcell;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG_PRIORITY))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	//get the cell pointer
	p_tskcell = vos_task_id2cell(ioarg.drv_task_id);
	if (NULL == p_tskcell) {
		DBG_ERR("get cell failed, drv_task_id %d\r\n", ioarg.drv_task_id);
		return -EINVAL;
	}

	//change priority, skip if not created
	if (p_tskcell->task_hdl) {
		set_user_nice(p_tskcell->task_hdl, VOS_PRIO_TO_USER_NICE(p_tskcell->vos_prio));
	}

	//store the new priority
	p_tskcell->vos_prio = ioarg.vos_prio;

	return 0;
}

int _IOFUNC_TASK_IOCMD_DESTROY(unsigned long arg)
{
	VOS_TASK_IOARG ioarg = {0};
	TASK_CELL_T *p_tskcell;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	//get the cell pointer
	p_tskcell = vos_task_id2cell(ioarg.drv_task_id);
	if (NULL == p_tskcell) {
		DBG_ERR("get cell failed, drv_task_id %d\r\n", ioarg.drv_task_id);
		return -EINVAL;
	}

	//return to users
	ioarg.pthread_id = p_tskcell->pthread_id;
	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_TASK_IOARG))) {
		return -EFAULT;
	}

	//clean the internal list
	vos_task_clear_info_p(p_tskcell);

	return 0;
}

int _IOFUNC_TASK_IOCMD_RETURN(unsigned long arg)
{
	VOS_TASK_IOARG ioarg = {0};
	TASK_CELL_T *p_tskcell;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	p_tskcell = vos_task_pthreadid2cell_p(ioarg.pthread_id);
	if (NULL == p_tskcell) {
		DBG_ERR("pthreadid2cell failed, pthread_id %ld\r\n", ioarg.pthread_id);
		return -EFAULT;
	}

	//clean the internal list
	vos_task_clear_info_p(p_tskcell);

	return 0;
}

int _IOFUNC_TASK_IOCMD_RESUME(unsigned long arg)
{
	VOS_TASK_IOARG ioarg = {0};
	TASK_CELL_T *p_tskcell;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	//get the cell pointer
	p_tskcell = vos_task_id2cell(ioarg.drv_task_id);
	if (NULL == p_tskcell) {
		DBG_ERR("get cell failed, drv_task_id %d\r\n", ioarg.drv_task_id);
		return -EINVAL;
	}

	//register pthread
	p_tskcell->pthread_id = ioarg.pthread_id;

	return 0;
}

int _IOFUNC_TASK_IOCMD_CONVERT_HDL(unsigned long arg)
{
	VOS_TASK_IOARG ioarg = {0};

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_TASK_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	if (ioarg.pthread_id != 0) {
		//pthread_id => drv_task_id
		int drv_task_id = vos_task_pthreadid2id_p(ioarg.pthread_id);
		if (drv_task_id < VOS_TASK_MIN_VALID_ID) {
			DBG_ERR("pthreadid2id failed, pthread_id %ld\r\n", ioarg.pthread_id);
			return -EFAULT;
		}
		ioarg.drv_task_id = drv_task_id;
	} else if (ioarg.drv_task_id != 0) {
		//drv_task_id => pthread_id
		TASK_CELL_T *p_tskcell = vos_task_id2cell(ioarg.drv_task_id);
		if (NULL == p_tskcell) {
			DBG_ERR("get cell failed, drv_task_id %d\r\n", ioarg.drv_task_id);
			return -EINVAL;
		}
		if (0 == p_tskcell->pthread_id) {
			DBG_ERR("pthread_id not registered\r\n");
			return -EFAULT;
		}
		ioarg.pthread_id = p_tskcell->pthread_id;
	} else {
		DBG_ERR("Unknown path, pthread_id %ld, drv_task_id %d\r\n", ioarg.pthread_id, ioarg.drv_task_id);
		return -EFAULT;
	}

	//return to users
	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_TASK_IOARG))) {
		return -EFAULT;
	}

	return 0;
}

EXPORT_SYMBOL(vos_task_create);
EXPORT_SYMBOL(vos_task_resume);
EXPORT_SYMBOL(vos_task_destroy);
EXPORT_SYMBOL(vos_task_enter);
EXPORT_SYMBOL(vos_task_return);
EXPORT_SYMBOL(vos_task_delay_ms);
EXPORT_SYMBOL(vos_task_set_priority);
EXPORT_SYMBOL(vos_task_dump_by_tskhdl);
EXPORT_SYMBOL(vos_task_dump);
EXPORT_SYMBOL(vos_task_chk_hdl_valid);
EXPORT_SYMBOL(vos_task_get_handle);
EXPORT_SYMBOL(vos_task_get_name);
EXPORT_SYMBOL(vos_task_get_tid);
