/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <linux/mmzone.h>
#include <linux/memory.h>
#include <linux/vmalloc.h>
#include <linux/uaccess.h>

#include <kwrap/dev.h>
#include <kwrap/flag.h>
#include <kwrap/task.h>
#include <kwrap/spinlock.h>
#include <kwrap/util.h>
#include "vos_ioctl.h"

#define __MODULE__    rtos_flag
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_FLAG_INITED_TAG    MAKEFOURCC('R', 'F', 'L', 'G') ///< a key value
#define WAIT_TASK_NUM_MAX       3

#define VOS_STRCPY(dst, src, dst_size) do { \
	strncpy(dst, src, (dst_size)-1); \
	dst[(dst_size)-1] = '\0'; \
} while(0)

#define loc_cpu(flags) vk_spin_lock_irqsave(&my_lock, flags)
#define unl_cpu(flags) vk_spin_unlock_irqrestore(&my_lock, flags)

#if defined(__LINUX)
#define my_alloc            vmalloc
#define my_free             vfree
#define cur_task_name       current->comm
#define my_jiffies          jiffies_64
#define my_jiff_to_ms(j)    ((j) * (1000 / HZ))

#elif defined(__FREERTOS)
#define my_alloc            malloc
#define my_free             free
#define current             xTaskGetCurrentTaskHandle()
#define cur_task_name       pcTaskGetName(xTaskGetCurrentTaskHandle())
#define my_jiffies          hwclock_get_longcounter()
#define my_jiff_to_ms(j)    ((j) / 1000)

#ifndef unlikely
#define unlikely(x) (x)
#endif

#else
#error Not supported OS
#endif

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
typedef struct {
	FLGPTN                  waiptn;
	UINT                    wfmode;
	struct task_struct      *task_hdl;
	UINT64                  wait_time_jiffies;
} FLAG_WAIT_TSK_INFO;

typedef struct {
	T_CFLG                  st_flags_attr;
	wait_queue_head_t       st_waitq;
	unsigned long           st_bits;
	int                     st_used;
	CHAR                    name[VOS_FLAG_NAME_SIZE];
	FLAG_WAIT_TSK_INFO      waitsk_list[WAIT_TASK_NUM_MAX];
	int                     waitsk_count;
} FLAG_CELL_T;

typedef struct {
	FLAG_CELL_T             *p_flags;
} FLAG_CTRL_T;

/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Function Protype                                                      */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_flag_debug_level = NVT_DBG_WRN;

module_param_named(rtos_flag_debug_level, rtos_flag_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rtos_flag_debug_level, "Debug message level");

static  VK_DEFINE_SPINLOCK(my_lock);

static int            gflgnum;
static int            g_curflg_id = 1;
static FLAG_CTRL_T    g_st_flag_ctrl = {0};

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void rtos_flag_add_waitlist_p(FLAG_CELL_T *p_flags, struct task_struct *task_hdl, FLGPTN waiptn, UINT wfmode)
{
	int i;
	FLAG_WAIT_TSK_INFO *p_waitsk;

	p_waitsk = p_flags->waitsk_list;
	for (i = 0; i < WAIT_TASK_NUM_MAX; i++) {
		if (0 == p_waitsk->task_hdl) {
			p_waitsk->task_hdl = task_hdl;
			p_waitsk->waiptn = waiptn;
			p_waitsk->wfmode = wfmode;
			p_waitsk->wait_time_jiffies = my_jiffies;
			p_flags->waitsk_count++;
			return;
		}
		p_waitsk++;
	}
}

void rtos_flag_del_waitlist_p(FLAG_CELL_T *p_flags, struct task_struct *task_hdl)
{
	int i;
	FLAG_WAIT_TSK_INFO *p_waitsk;

	p_waitsk = p_flags->waitsk_list;
	for (i = 0; i < WAIT_TASK_NUM_MAX; i++) {
		if (task_hdl == p_waitsk->task_hdl) {
			p_waitsk->task_hdl = 0;
			p_flags->waitsk_count--;
			return;
		}
		p_waitsk++;
	}
}

/*-----------------------------------------------------------------------------*/
/* Export Functions                                                         */
/*-----------------------------------------------------------------------------*/
/**
   rtos_flag_init
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
void rtos_flag_init(unsigned long max_flag_num)
{
	DBG_FUNC_BEGIN("\r\n");
	DBG_IND("max_flag_num =%ld\r\n", max_flag_num);

	// initialize value
	gflgnum = max_flag_num;
	g_curflg_id = 1;

	g_st_flag_ctrl.p_flags = (FLAG_CELL_T *)my_alloc(sizeof(FLAG_CELL_T) * gflgnum);
	if (g_st_flag_ctrl.p_flags == NULL) {
		DBG_ERR(" alloc!\n");
		goto END_INIT;
	}

	DBG_IND("pflags =0x%lx\r\n", (ULONG)g_st_flag_ctrl.p_flags);
	memset(g_st_flag_ctrl.p_flags, 0x00, sizeof(FLAG_CELL_T) * gflgnum);

	//spin_lock_init(&g_st_flag_ctrl.st_flag_lock);

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
void rtos_flag_exit(void)
{
	my_free(g_st_flag_ctrl.p_flags);
	g_st_flag_ctrl.p_flags = NULL;
}

ER vos_flag_get_free_id(ID *p_flgid)
{
	FLAG_CELL_T *p_flagcell;
	int                  tmpid, i;
	unsigned long flags = 0;

	loc_cpu(flags);
	if (unlikely(g_curflg_id >= gflgnum)) {
		g_curflg_id = 1;
	}
	tmpid = g_curflg_id;
	for (i = 0; i < gflgnum; i++) {
		if (unlikely(tmpid >= gflgnum)) {
			tmpid = 1;
		}
		p_flagcell = g_st_flag_ctrl.p_flags + tmpid;
		if (0 == p_flagcell->st_used) {
			p_flagcell->st_used = RTOS_FLAG_INITED_TAG;
			g_curflg_id = tmpid;
			*p_flgid = g_curflg_id;
			g_curflg_id++;
			unl_cpu(flags);
			DBG_IND("g_curflg_id=%d\r\n", tmpid);
			return E_OK;
		}
		tmpid++;
	}
	*p_flgid = 0;
	unl_cpu(flags);
	DBG_ERR("exceed max flag num %d\r\n", gflgnum);
	return E_ID;
}
/**
   cre_flg
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
ER vos_flag_create(ID *p_flgid, T_CFLG *pk_cflg, char *name)
{
	ID flgid_new;
	ER st_reslut = E_OK;
	FLAG_CELL_T *p_flagcell;

	if (unlikely(NULL == p_flgid)) {
		DBG_ERR("p_flgid is NULL\n");
		return E_PAR;
	}

	if (unlikely(E_OK != vos_flag_get_free_id(&flgid_new))) {
		DBG_ERR("flag out of id\r\n");
		return E_ID;
	}

	if (unlikely(flgid_new == 0 || flgid_new >= gflgnum)) {
		DBG_ERR("Invalid flgid_new %d\r\n", flgid_new);
		return E_ID;
	}
	DBG_IND("flgid_new=%d\r\n", flgid_new);

	p_flagcell = g_st_flag_ctrl.p_flags + flgid_new;
	DBG_IND("p_flagcell=0x%lx\r\n", (ULONG)p_flagcell);

	init_waitqueue_head(&p_flagcell->st_waitq);
	VOS_STRCPY(p_flagcell->name, name, sizeof(p_flagcell->name));

	//setup the initial value if specified
	if (pk_cflg) {
		memcpy(&p_flagcell->st_flags_attr, pk_cflg, sizeof(T_CFLG));
		p_flagcell->st_bits |= (unsigned long)p_flagcell->st_flags_attr.iflgptn;
	}

	//restore the valid flag id
	*p_flgid = flgid_new;

	return st_reslut;
}

ER vos_flag_destroy(ID flgid)
{
	FLAG_CELL_T *p_flagcell;

	if (unlikely(flgid == 0 || flgid >= gflgnum)) {
		DBG_ERR("Invalid flgid %d, task [%s]\r\n", flgid, cur_task_name);
		return  E_ID;
	}
	DBG_IND("flgid=%d\r\n", flgid);
	p_flagcell = g_st_flag_ctrl.p_flags + flgid;
	memset(p_flagcell, 0x00, sizeof(FLAG_CELL_T));
	return E_OK;
}

/**
   set_flg
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
ER vos_flag_set(ID flgid, FLGPTN setptn)
{
	unsigned long flags = 0;
	FLAG_CELL_T *p_flagcell;

	if (unlikely(flgid == 0 || flgid >= gflgnum)) {
		DBG_ERR("Invalid flgid %d, task [%s]\r\n", flgid, cur_task_name);
		return  E_ID;
	}
	DBG_IND("flgid=%d, setptn=0x%x\r\n", flgid, setptn);

	p_flagcell = g_st_flag_ctrl.p_flags + flgid;
	DBG_IND("p_flagcell=0x%lx\r\n", (ULONG)p_flagcell);

	loc_cpu(flags);
	p_flagcell->st_bits |= (unsigned long)setptn;
	//wake_up_interruptible(&p_flagcell->st_waitq);
	wake_up(&p_flagcell->st_waitq);
	unl_cpu(flags);

	return E_OK;
}

/**
   clr_flg
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
ER vos_flag_clr(ID flgid, FLGPTN clrptn)
{
	unsigned long flags = 0;
	FLAG_CELL_T *p_flagcell;

	if (unlikely(flgid == 0 || flgid >= gflgnum)) {
		DBG_ERR("Invalid flgid %d, task [%s]\r\n", flgid, cur_task_name);
		return  E_ID;
	}
	p_flagcell = g_st_flag_ctrl.p_flags + flgid;

	loc_cpu(flags);
	p_flagcell->st_bits &= ~clrptn;
	unl_cpu(flags);

	return E_OK;
}

int _vos_flag_wait(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode, int timeout_tick, int interruptible)
{
	int api_ret = 0;
	int wait_ret = 0;
	FLAG_CELL_T *p_flagcell;
	unsigned long flags = 0;
	unsigned int keep = 0;
	FLGPTN flgwai;

	DBG_IND("flgid=%d\r\n", flgid);

	if (unlikely(flgid == 0 || flgid >= gflgnum)) {
		DBG_ERR("Invalid flgid %d, task [%s]\r\n", flgid, cur_task_name);
		return  E_ID;
	}
	if (unlikely(waiptn == 0)) {
		return E_PAR;
	}
	p_flagcell = g_st_flag_ctrl.p_flags + flgid;

	//printk("wai_flg b flgid = %d, waiptn = 0x%x, \r\n",flgid,waiptn);
	loc_cpu(flags);
	rtos_flag_add_waitlist_p(p_flagcell, current, waiptn, wfmode);
	vos_task_update_info(VOS_RES_TYPE_FLAG, current, flgid, waiptn, wfmode);
	unl_cpu(flags);

	do {
		if (interruptible) {
			/*
			 * Ref to include/linux/wait.h
			 * wait_event_interruptible - sleep until a condition gets true
			 * The function will return -ERESTARTSYS if it was interrupted by a
			 * signal and 0 if @condition evaluated to true.
			 */
			if (wfmode & TWF_ORW) {
				wait_ret = wait_event_interruptible(p_flagcell->st_waitq, p_flagcell->st_bits & waiptn);
			} else {
				wait_ret = wait_event_interruptible(p_flagcell->st_waitq, (p_flagcell->st_bits & waiptn) == waiptn);
			}
			if (0 == wait_ret) {
				wait_ret = 1; //set to 1 to represent the condition is met
			}
		} else if (-1 == timeout_tick) {
			if (wfmode & TWF_ORW) {
				wait_event(p_flagcell->st_waitq, p_flagcell->st_bits & waiptn);
			} else {
				wait_event(p_flagcell->st_waitq, (p_flagcell->st_bits & waiptn) == waiptn);
			}
			wait_ret = 1; //set to 1 to represent the condition is met
		} else {
			/*
			 * Ref to include/linux/wait.h
			 * wait_event_timeout
			 * Returns:
			 * 0 if the @condition evaluated to %false after the @timeout elapsed,
			 * 1 if the @condition evaluated to %true after the @timeout elapsed,
			 * or the remaining jiffies (at least 1) if the @condition evaluated
			 * to %true before the @timeout elapsed.
			 */
			if (wfmode & TWF_ORW) {
				wait_ret = wait_event_timeout(p_flagcell->st_waitq, p_flagcell->st_bits & waiptn, timeout_tick);
			} else {
				wait_ret = wait_event_timeout(p_flagcell->st_waitq, (p_flagcell->st_bits & waiptn) == waiptn, timeout_tick);
			}
		}

		keep = 0;
		loc_cpu(flags);

		if (wait_ret > 0) {
			flgwai = p_flagcell->st_bits & waiptn;
			// if the flag has been set, return
			if ((wfmode & TWF_ORW) ? flgwai != 0 : (flgwai == waiptn)) {
				if (wfmode & TWF_CLR) {
					p_flagcell->st_bits &= ~waiptn;
				}
				*p_flgptn = flgwai;
				api_ret = 0;
			} else {
				keep = 1;
			}
		} else if (-ERESTARTSYS == wait_ret){
			api_ret = E_RLWAI;
		} else {
			api_ret = E_TMOUT;
		}
		unl_cpu(flags);

	} while (keep);

	loc_cpu(flags);
	rtos_flag_del_waitlist_p(p_flagcell, current);
	vos_task_update_info(VOS_RES_TYPE_INVALID, current, 0, 0, 0);
	unl_cpu(flags);

	return api_ret;
}

ER vos_flag_wait(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode)
{
	return _vos_flag_wait(p_flgptn, flgid, waiptn, wfmode, -1, 0);
}

ER vos_flag_wait_timeout(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode, int timeout_tick)
{
	return _vos_flag_wait(p_flgptn, flgid, waiptn, wfmode, timeout_tick, 0);
}

ER vos_flag_wait_interruptible(PFLGPTN p_flgptn, ID flgid, FLGPTN waiptn, UINT wfmode)
{
	return _vos_flag_wait(p_flgptn, flgid, waiptn, wfmode, -1, 1);
}

/**
   kchk_flg
 * @remarks
 * @param none
 * @code

 * @endcode
 * @return
 * @see
*/
FLGPTN vos_flag_chk(ID flgid, FLGPTN chkptn)
{
	FLGPTN flg_status;
	FLAG_CELL_T *p_flagcell;

	if (unlikely(flgid == 0 || flgid >= gflgnum)) {
		DBG_ERR("Invalid flgid %d, task [%s]\r\n", flgid, cur_task_name);
		return  0;
	}
	p_flagcell = g_st_flag_ctrl.p_flags + flgid;

	flg_status = p_flagcell->st_bits & chkptn;
	return flg_status;
}

char *vos_flag_get_name(ID flgid)
{
	FLAG_CELL_T *p_flagcell;

	if (unlikely(flgid == 0 || flgid >= gflgnum)) {
		DBG_ERR("Invalid flgid %d\r\n", flgid);
		return NULL;
	}
	p_flagcell = g_st_flag_ctrl.p_flags + flgid;
	return p_flagcell->name;
}

#if defined __UITRON || defined __ECOS
void vos_flag_dump(void (*dump)(char *fmt, ...))
#else
void vos_flag_dump(int (*dump)(const char *fmt, ...))
#endif
{
	int                 i, tsk_i, curlen, task_id, flag_cnt = 0;
	FLAG_CELL_T       *p_flagcell;
	FLAG_WAIT_TSK_INFO *p_waitsk;
	char                buf[30];
	char               *p_curr;
	UINT64              sleep_time_ms;
	char                task_name[TASK_COMM_LEN];

	DBG_DUMP("\r\n-------------------------FLAG -------------------------------------------------------\r\n");
	p_flagcell = g_st_flag_ctrl.p_flags;
	for (i = 0; i < gflgnum; i++) {
		if (RTOS_FLAG_INITED_TAG == p_flagcell->st_used) {
			flag_cnt++;
			DBG_DUMP("Flag[%.3d, %32s] ->   Flags: 0x%.8lX\r\n", i, p_flagcell->name, p_flagcell->st_bits);
			if (p_flagcell->waitsk_count > 0) {
				DBG_DUMP("Waiting Task Queue : \r\n");
				p_waitsk = p_flagcell->waitsk_list;
				for (tsk_i = 0; tsk_i < WAIT_TASK_NUM_MAX; tsk_i++, p_waitsk++) {
					if (FALSE == vos_task_chk_hdl_valid(p_waitsk->task_hdl))
						continue;
					p_curr = (char *)&buf[0];
					if (p_waitsk->wfmode & TWF_ORW) {
						snprintf(p_curr, sizeof(buf), "%s", "OR ");
					} else {
						snprintf(p_curr, sizeof(buf), "%s", "AND ");
					}
					curlen = strlen(p_curr);
					p_curr += curlen;
					if (p_waitsk->wfmode & TWF_CLR) {
						snprintf(p_curr, sizeof(buf)-curlen, "%s", "CLEAR ");
					}
					task_id = vos_task_hdl2id(p_waitsk->task_hdl);
					sleep_time_ms = my_jiff_to_ms(my_jiffies - p_waitsk->wait_time_jiffies);
					if (virt_addr_valid(&p_waitsk->task_hdl->comm[0])) {
						VOS_STRCPY(task_name, p_waitsk->task_hdl->comm, sizeof(task_name));
					} else {
						VOS_STRCPY(task_name, "user_tsk", sizeof(task_name));
					}
					DBG_DUMP("  Task[%.3d, %s]\r\n            Pattern: 0x%.8X, Mode = %s, sleep_time = %lld ms \r\n",
							     task_id, task_name, p_waitsk->waiptn, buf, sleep_time_ms);
					if (task_id < 0) {
						vos_task_dump_by_tskhdl(dump, p_waitsk->task_hdl);
					}
				}
			}
		}
		p_flagcell++;
	}
	DBG_DUMP("\r\n Max flag_cnt = %d , used = %d\r\n", gflgnum, flag_cnt);
}

int _IOFUNC_FLAG_IOCMD_CREATE(unsigned long arg)
{
	VOS_FLAG_IOARG ioarg = {0};
	ID newid = 0;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_FLAG_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	if (E_OK != vos_flag_create(&newid, &ioarg.init_pattern, ioarg.name)) {
		DBG_ERR("vos_sem_create fail\r\n");
		return -EFAULT;
	}

	ioarg.id = (unsigned long)newid;
	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_FLAG_IOARG))) {
		DBG_ERR("copy_to_user failed\n");
		return -EFAULT;
	}

	return 0;
}

int _IOFUNC_FLAG_IOCMD_SET(unsigned long arg)
{
	VOS_FLAG_IOARG ioarg = {0};

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_FLAG_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	if (E_OK != vos_flag_set((ID)ioarg.id, (FLGPTN)ioarg.bits)) {
		return -EFAULT;
	}

	return 0;
}

int _IOFUNC_FLAG_IOCMD_CLR(unsigned long arg)
{
	VOS_FLAG_IOARG ioarg = {0};

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_FLAG_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	if (E_OK != vos_flag_clr((ID)ioarg.id, (FLGPTN)ioarg.bits)) {
		return -EFAULT;
	}

	return 0;
}


int _IOFUNC_FLAG_IOCMD_WAIT(unsigned long arg)
{
	VOS_FLAG_IOARG ioarg = {0};
	FLGPTN st_flg_ptn = 0;
	int tick = 0;
	int ret;

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_FLAG_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	tick = vos_util_msec_to_tick(ioarg.timeout_tick); //user-space tick is msec

	ret = _vos_flag_wait(&st_flg_ptn, (ID)ioarg.id, (FLGPTN)ioarg.bits, (UINT)ioarg.mode, tick, ioarg.interruptible);
	if (ret == E_RLWAI) {
		//Note: return -ERESTARTSYS so that user-space can restart syscall or return EINTR if terminated
		//And we should not destroy user arg by "copy_to_user"
		return -ERESTARTSYS;
	}

	ioarg.bits = (unsigned long)st_flg_ptn;
	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_FLAG_IOARG))) {
		return -EFAULT;
	}

	return ret;
}

int _IOFUNC_FLAG_IOCMD_CHK(unsigned long arg)
{
	VOS_FLAG_IOARG ioarg = {0};

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_FLAG_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	ioarg.bits = vos_flag_chk((ID)ioarg.id, (FLGPTN)ioarg.bits);
	if (copy_to_user((void *)arg, (void *)&ioarg, sizeof(VOS_FLAG_IOARG))) {
		return -EFAULT;
	}

	return 0;
}

int _IOFUNC_FLAG_IOCMD_DESTROY(unsigned long arg)
{
	VOS_FLAG_IOARG ioarg = {0};

	if (copy_from_user((void *)&ioarg, (void *)arg, sizeof(VOS_FLAG_IOARG))) {
		DBG_ERR("copy_from_user failed\n");
		return -EFAULT;
	}

	if (E_OK != vos_flag_destroy((ID)ioarg.id)) {
		return -EFAULT;
	}

	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Kernel Mode Definiton                                                       */
/*-----------------------------------------------------------------------------*/
EXPORT_SYMBOL(vos_flag_create);
EXPORT_SYMBOL(vos_flag_destroy);
EXPORT_SYMBOL(vos_flag_set);
EXPORT_SYMBOL(vos_flag_clr);
EXPORT_SYMBOL(vos_flag_wait);
EXPORT_SYMBOL(vos_flag_wait_interruptible);
EXPORT_SYMBOL(vos_flag_wait_timeout);
EXPORT_SYMBOL(vos_flag_chk);
EXPORT_SYMBOL(vos_flag_get_name);
EXPORT_SYMBOL(vos_flag_dump);
