/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_AX__H__
#define __OSAL_AX__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_type_ax.h"
#include <linux/timer.h>
#include <linux/uaccess.h>
#include <linux/ax_hrtimer.h>

#include <linux/version.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 16, 0))
#include <linux/iversion.h>
#endif

#define OSAL_VERSION_AX "0.50"

#define AX_THREAD_SHOULD_STOP 1
#define AX_TASK_RUNNING			0x0000
#define AX_TASK_INTERRUPTIBLE		0x0001
#define AX_TASK_UNINTERRUPTIBLE		0x0002


#define AX_VERIFY_READ VERIFY_READ
#define AX_VERIFY_WRITE VERIFY_WRITE

#define AX_OSAL_USER_access_ok(type, addr, size) access_ok(type, addr, size)
#define AX_OSAL_USER_put_user(x, ptr) __put_user(x, ptr)
#define AX_OSAL_USER_get_user(x, ptr) __get_user(x, ptr)

typedef int(*AX_THREAD_FUNC_T)(void *data);

typedef struct AX_TASK {
    void *task_struct;
} AX_TASK_T;

struct AX_CURRENT_TASK {
	pid_t pid;
	pid_t tgid;
	void *mm;
	char comm[16];
};

typedef struct AX_TASK_SCHED_PARAM {
    int sched_priority;
} AX_TASK_SCHED_PARAM_T;

AX_TASK_T *AX_OSAL_TASK_kthread_run(AX_THREAD_FUNC_T thread, void *data, char *name);
AX_TASK_T *AX_OSAL_TASK_kthread_create_ex(AX_THREAD_FUNC_T thread, void *data, char *name, int prioirty);
int AX_OSAL_TASK_kthread_stop(AX_TASK_T *task, unsigned int stop_flag);
int AX_OSAL_TASK_cond_resched(void);
bool AX_OSAL_TASK_kthread_should_stop(void);

void AX_OSAL_set_current_state(int state_value);
void AX_OSAL_get_current(struct AX_CURRENT_TASK *ax_task);
int AX_OSAL_sched_setscheduler(AX_TASK_T *task, int policy,
		       const struct AX_TASK_SCHED_PARAM *param);
void AX_OSAL_TASK_schedule(void);

//semaphore api
typedef struct AX_SEMAPHORE {
    void *sem;
} AX_SEMAPHORE_T;

int AX_OSAL_SYNC_sema_init(AX_SEMAPHORE_T *sem, int val);
//only for linux kernel
int AX_OSAL_SYNC_sema_down_interruptible(AX_SEMAPHORE_T *sem);
int AX_OSAL_SYNC_sema_down(AX_SEMAPHORE_T *sem);
int AX_OSAL_SYNC_sema_down_timeout(AX_SEMAPHORE_T *sem, long timeout);
int AX_OSAL_SYNC_sema_down_trylock(AX_SEMAPHORE_T *sem);
void AX_OSAL_SYNC_sema_up(AX_SEMAPHORE_T *sem);
void AX_OSAL_SYNC_sema_destroy(AX_SEMAPHORE_T *sem);

//mutex api
typedef struct AX_MUTEX {
    void *mutex;
} AX_MUTEX_T;


#ifndef CONFIG_DEBUG_MUTEXES
int AX_OSAL_SYNC_mutex_init(AX_MUTEX_T *mutex);
#else
void *AX_OSAL_DBG_mutex_init(AX_MUTEX_T * mutex);
#define AX_OSAL_SYNC_mutex_init(mutex)                     \
          ({                                               \
		int ret = 0;                               \
		void *p = AX_OSAL_DBG_mutex_init(mutex);   \
		if (p != NULL)                             \
			mutex_init(p);                     \
		else                                       \
			ret = -1;                          \
		ret;                                       \
         })
#endif

int AX_OSAL_SYNC_mutex_lock(AX_MUTEX_T *mutex);
//only for linux kernel
int AX_OSAL_SYNC_mutex_lock_interruptible(AX_MUTEX_T *mutex);
int AX_OSAL_SYNC_mutex_trylock(AX_MUTEX_T *mutex);
void AX_OSAL_SYNC_mutex_unlock(AX_MUTEX_T *mutex);
void AX_OSAL_SYNC_mutex_destroy(AX_MUTEX_T *mutex);

#define DEFINE_AX_OSAL_SYNC_MUTEX(mutex_name) \
	DEFINE_MUTEX(osal##mutex_name); \
	AX_MUTEX_T mutex_name = { \
			.mutex = &osal##mutex_name, \
		}

//spin lock api
typedef struct AX_SPINLOCK {
    void *lock;
} AX_SPINLOCK_T;

#ifndef CONFIG_DEBUG_SPINLOCK
int AX_OSAL_SYNC_spin_lock_init(AX_SPINLOCK_T *lock);
#else
spinlock_t *AX_OSAL_DBG_spin_lock_init(AX_SPINLOCK_T *lock);

#define AX_OSAL_SYNC_spin_lock_init(lock)                     \
	({                                                    \
		int ret = 0;                                  \
		spinlock_t *p = AX_OSAL_DBG_spin_lock_init(lock);   \
		if(p != NULL)                                 \
			spin_lock_init(p);                    \
		else                                          \
			ret = -1;                             \
 		ret;                                          \
	})
#endif

void AX_OSAL_SYNC_spin_lock(AX_SPINLOCK_T *lock);
int AX_OSAL_SYNC_spin_trylock(AX_SPINLOCK_T *lock);
void AX_OSAL_SYNC_spin_unlock(AX_SPINLOCK_T *lock);
void AX_OSAL_SYNC_spin_lock_irqsave(AX_SPINLOCK_T *lock, unsigned int *flags);
void AX_OSAL_SYNC_spin_unlock_irqrestore(AX_SPINLOCK_T *lock, unsigned int *flags);
void AX_OSAL_SYNC_spinLock_destory(AX_SPINLOCK_T *lock);

#define DEFINE_AX_OSAL_SYNC_SPINLOCK(lock_name) \
	DEFINE_SPINLOCK(osal##lock_name); \
	AX_SPINLOCK_T lock_name = { \
		   .lock = &osal##lock_name};




typedef struct AX_RT_MUTEX {
    void *rt_mutex;
} AX_RT_MUTEX_T;

#ifdef CONFIG_DEBUG_RT_MUTEXES
int AX_OSAL_DBG_rt_mutex_init(AX_RT_MUTEX_T * rt_mutex,struct lock_class_key *__key);
# define AX_OSAL_SYNC_rt_mutex_init(rt_mutex)                    \
({                                                               \
	int __ret = 0;                                           \
	static struct lock_class_key __key;                      \
	__ret = AX_OSAL_DBG_rt_mutex_init(rt_mutex,&__key);      \
	if(__ret < 0)                                            \
		__ret = __ret;                                   \
	__ret;                                                   \
})

#else
int AX_OSAL_SYNC_rt_mutex_init(AX_RT_MUTEX_T * mutex);
#endif

void AX_OSAL_SYNC_rt_mutex_lock(AX_RT_MUTEX_T * mutex);
void AX_OSAL_SYNC_rt_mutex_unlock(AX_RT_MUTEX_T * mutex);
void AX_OSAL_SYNC_rt_mutex_destroy(AX_RT_MUTEX_T * mutex);

//atomic api
typedef struct AX_ATOMIC {
    void *atomic;
} AX_ATOMIC_T;

int AX_OSAL_SYNC_atomic_init(AX_ATOMIC_T *atomic);
int AX_OSAL_SYNC_atomic_read(AX_ATOMIC_T *atomic);
void AX_OSAL_SYNC_atomic_set(AX_ATOMIC_T *atomic, int val);
int AX_OSAL_SYNC_atomic_inc_return(AX_ATOMIC_T *atomic);
int AX_OSAL_SYNC_atomic_dec_return(AX_ATOMIC_T *atomic);
int AX_OSAL_SYNC_atomic_cmpxchg(AX_ATOMIC_T *atomic, int old, int new);
bool AX_OSAL_SYNC_atomic_try_cmpxchg(AX_ATOMIC_T * atomic, int *old, int new);
void AX_OSAL_SYNC_atomic_and(int val,AX_ATOMIC_T * atomic);
void AX_OSAL_SYNC_atomic_or(int val,AX_ATOMIC_T * atomic);
int AX_OSAL_SYNC_atomic_fetch_add_ge(AX_ATOMIC_T * atomic, int add, int used);
void AX_OSAL_SYNC_atomic_destroy(AX_ATOMIC_T *atomic);


//barrier api
void AX_OSAL_SYNC_mb(void);
void AX_OSAL_SYNC_rmb(void);
void AX_OSAL_SYNC_wmb(void);
void AX_OSAL_SYNC_isb(void);
void AX_OSAL_SYNC_dsb(void);
void AX_OSAL_SYNC_dmb(void);


//workqueue api
typedef struct AX_WORK {
    void *work;
    void(*func)(struct AX_WORK *work);
} AX_WORK_T;
typedef void(*AX_WORK_FUNC_T)(AX_WORK_T *work);

int AX_OSAL_SYNC_init_work(AX_WORK_T *work, AX_WORK_FUNC_T func);
int AX_OSAL_SYNC_schedule_work(AX_WORK_T *work);
void AX_OSAL_SYNC_destroy_work(AX_WORK_T *work);

int AX_OSAL_SYNC_init_delayed_work(AX_WORK_T *work, AX_WORK_FUNC_T func);
int AX_OSAL_SYNC_schedule_delayed_work(AX_WORK_T *work, unsigned long delay);
void AX_OSAL_SYNC_destroy_delayed_work(AX_WORK_T *work);
int AX_OSAL_SYNC_cancel_delayed_work(AX_WORK_T *osal_work);
int AX_OSAL_SYNC_cancel_delayed_work_sync(AX_WORK_T *osal_work);

//waitqueue api
#define AX_OSAL_SYNC_INTERRUPTIBLE  0
#define AX_OSAL_SYNC_UNINTERRUPTIBLE    1

#define DEFINE_AX_OSAL_SYNC_WAIT_QUEUE_HEAD(wq_name) \
	DECLARE_WAIT_QUEUE_HEAD(osal##wq_name); \
	AX_WAIT_T wq_name = { \
		   .wait = &osal##wq_name};
typedef int(*AX_WAIT_COND_FUNC_T)(void *param);

typedef struct AX_WAIT {
    void *wait;
} AX_WAIT_T;
int AX_OSAL_SYNC_waitqueue_init(AX_WAIT_T *wait);
//only for linux kernel
void AX_OSAL_SYNC_wakeup(AX_WAIT_T *wait, void *key);
void AX_OSAL_SYNC_wake_up_interruptible(AX_WAIT_T *osal_wait, void *key);
void AX_OSAL_SYNC_wake_up_interruptible_all(AX_WAIT_T *wait, void *key);

void AX_OSAL_SYNC_wait_destroy(AX_WAIT_T *wait);

/*wait event interrupt*/
unsigned int __AX_OSAL_SYNC_wait_interruptible(AX_WAIT_T *wait, AX_WAIT_COND_FUNC_T func, void *param);
#define AX_OSAL_SYNC_wait_event_interruptible(wait, func, param) \
		({									\
			int __ret = 0;							\
			for (;;){						   \
				if(func(param)){					   \
					__ret = 0;					\
					break;					  \
				}\
				__ret = __AX_OSAL_SYNC_wait_interruptible(wait, (func), param);	\
				if(__ret < 0){				  \
					if(__ret == -2) {  \
						__ret = 0;   \
					}  \
					break;  \
				}   \
			}									 \
			__ret;									 \
		})



/*wait event interrupt*/
unsigned int __AX_OSAL_SYNC_wait_interruptible_timeout(AX_WAIT_T *wait, AX_WAIT_COND_FUNC_T func, void *param,unsigned long timeout);
#define AX_OSAL_SYNC_wait_event_interruptible_timeout(wait, func, param, timeout) \
	({									\
		int __ret = timeout;						  \
									   \
		if ((func(param)) && !timeout) \
		{ \
		__ret = 1; \
		} \
											  \
		for (;;) {							\
			if (func(param))					   \
			{\
				break;					  \
			}\
			__ret = __AX_OSAL_SYNC_wait_interruptible_timeout(wait, (func), param, __ret);	 \
			if(__ret < 0)	\
			{\
				break;  \
			}\
			if(!__ret && !func(param))  \
			{\
				__ret = -ETIMEDOUT;   \
				break;  \
			}\
		}									\
		__ret;									 \
	})

unsigned int __AX_OSAL_SYNC_wait_uninterruptible(AX_WAIT_T *wait, AX_WAIT_COND_FUNC_T func, void *param);
#define AX_OSAL_SYNC_wait_event(wait, func, param) \
({                                  \
    int __ret = 0;                          \
    for (;;){                          \
        if(func(param)){                       \
            __ret = 0;                  \
            break;                    \
        }\
        __ret = __AX_OSAL_SYNC_wait_uninterruptible(wait, (func), param);   \
        if(__ret < 0)                 \
            break;           \
    }                                    \
    __ret;                                   \
})

unsigned int __AX_OSAL_SYNC_wait_uninterruptible_timeout(AX_WAIT_T *wait, AX_WAIT_COND_FUNC_T func, void *param,unsigned long timeout);
#define AX_OSAL_SYNC_wait_event_timeout(wait, func, param, timeout) \
({                                  \
    int __ret = timeout;                          \
                                   \
    if ((func(param)) && !timeout) \
    { \
    __ret = 1; \
    } \
                                          \
    for (;;) {                          \
        if (func(param))                       \
        {\
            break;                    \
        }\
        __ret = __AX_OSAL_SYNC_wait_uninterruptible_timeout(wait, (func), param, __ret);   \
	if(!__ret && !func(param))  \
	{\
        	__ret = -ETIMEDOUT;   \
        	break;                \
	}\
    }                                   \
    __ret;                                   \
})


//timer api
typedef struct AX_TIMER {
    struct timer_list timer;
    void(*function)(void *);
    long data;
} AX_TIMER_T;

typedef struct AX_TIMERVAL {
    long tv_sec;
    long tv_usec;
} AX_TIMERVAL_T;

typedef struct AX_TIMERSPEC64 {
    long long int  tv_sec;
    long tv_nsec;
} AX_TIMERSPEC64_T;

typedef struct AX_RTC_TIMER {
    int tm_sec;
    int tm_min;
    int tm_hour;
    int tm_mday;
    int tm_mon;
    int tm_year;
    int tm_wday;
    int tm_yday;
    int tm_isdst;
} AX_RTC_TIMER_T;

#define AX_OSAL_HRTIMER_NORESTART  0      /* Timer is not restarted */
#define AX_OSAL_HRTIMER_RESTART   1        /* Timer must be restarted */

long AX_OSAL_DEV_usleep(unsigned long use);
void *AX_OSAL_DEV_hrtimer_alloc(unsigned long use,int (*function)(void *),void *private);
void AX_OSAL_DEV_hrtimer_destroy(void *timer);
int AX_OSAL_DEV_hrtimer_start(void *timer);
int AX_OSAL_DEV_hrtimer_stop(void *timer);

AX_TIMER_T *AX_OSAL_TMR_alloc_timers(void (*function)(void *), unsigned long data);
int AX_OSAL_TMR_init_timers(AX_TIMER_T *timer);
unsigned int AX_OSAL_TMR_mod_timer(AX_TIMER_T *timer, unsigned long interval);
unsigned int AX_OSAL_TMR_del_timer(AX_TIMER_T *timer);
int AX_OSAL_TMR_destory_timer(AX_TIMER_T *timer);
unsigned long AX_OSAL_TM_msleep(unsigned int msecs);
void AX_OSAL_TM_udelay(unsigned int usecs);
void AX_OSAL_TM_mdelay(unsigned int msecs);
long  AX_OSAL_TM_msecs_to_jiffies(long ax_msecs);
unsigned int AX_OSAL_TM_jiffies_to_msecs(void);
u64 AX_OSAL_TM_sched_clock(void);
u64 AX_OSAL_TM_get_microseconds(void);
//only for linux kernel
void AX_OSAL_TM_do_gettimeofday(AX_TIMERVAL_T *tm);
void AX_OSAL_TM_do_settimeofday(AX_TIMERVAL_T *tm);
//only for linux kernel
void AX_OSAL_TM_rtc_time_to_tm(unsigned long time, AX_RTC_TIMER_T *tm);
//only for linux kernel
void AX_OSAL_TM_rtc_tm_to_time(AX_RTC_TIMER_T *tm, unsigned long *time);
void AX_OSAL_TM_get_jiffies(u64 *pjiffies);
//only for linux kernel
int AX_OSAL_TM_rtc_valid_tm(AX_RTC_TIMER_T *tm);
u64 AX_OSAL_TM_get_microsecond(void);
void AX_OSAL_TM_hrtimer_mdelay(unsigned int msecs);
void AX_OSAL_TM_hrtimer_udelay(unsigned long usecs);
void AX_OSAL_TM_ktime_get_real_ts64( AX_TIMERSPEC64_T *tm);
unsigned long AX_OSAL_TMR_get_tmr64_clk(void);


//kmalloc , memory
#define AX_OSAL_GFP_ATOMIC  0
#define AX_OSAL_GFP_KERNEL  1

void *AX_OSAL_MEM_kmalloc(u32 size, unsigned int osal_gfp_flag);
void *AX_OSAL_MEM_kzalloc(u32 size, unsigned int osal_gfp_flag);
void AX_OSAL_MEM_kfree(const void *addr);
void *AX_OSAL_MEM_vmalloc(u32 size);
void AX_OSAL_MEM_vfree(const void *addr);
int AX_OSAL_MEM_VirtAddrIsValid(unsigned long vm_start, unsigned long vm_end);
int AX_OSAL_MEM_AddrMunmap(unsigned long start, u32 size);
unsigned long AX_OSAL_MEM_AddrMmap(void *file, unsigned long addr,
	unsigned long len, unsigned long prot, unsigned long flag, unsigned long offset);

void *ax_os_mem_kmalloc(int id, size_t size, u32 flag);
void *ax_os_mem_kzalloc(int id, size_t size, u32 flag);
void ax_os_mem_kfree(int id, const void *addr);
void *ax_os_mem_vmalloc(int id, size_t size);
void ax_os_mem_vfree(int id, const void *addr);
s32 ax_os_release_reserved_mem(unsigned long phy_start, size_t size, char *s);

//file system , only for linux kernel
#define AX_OSAL_O_RDONLY         00
#define AX_OSAL_O_WRONLY         01
#define AX_OSAL_O_RDWR           02

#define AX_OSAL_O_CREAT        0100
#define AX_OSAL_O_EXCL         0200
#define AX_OSAL_O_NOCTTY       0400
#define AX_OSAL_O_TRUNC       01000
#define AX_OSAL_O_APPEND      02000
#define AX_OSAL_O_NONBLOCK    04000
#define AX_OSAL_O_DSYNC      010000
#define AX_OSAL_O_SYNC     04010000
#define AX_OSAL_O_RSYNC    04010000
#define AX_OSAL_O_BINARY    0100000
#define AX_OSAL_O_DIRECTORY 0200000
#define AX_OSAL_O_NOFOLLOW  0400000
#define AX_OSAL_O_CLOEXEC  02000000

void *AX_OSAL_FS_filp_open(const char *filename, int flags, int mode);
void AX_OSAL_FS_filp_close(void * filp);
int AX_OSAL_FS_filp_write(char *buf, int len, void * filp);
int AX_OSAL_FS_filp_read(char *buf, int len, void * filp);

/*module init&exit API*/
/*IMPORTANT: 'module init&exit' not support to compat OS, later we will fixed, because we may change RTT sourcecode*/

/*Linux init module */
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/fcntl.h>
#include <linux/of.h>
#include <linux/irqreturn.h>

#define AX_OSAL_module_initcall(fn) module_init(fn);
#define AX_OSAL_module_exit(fn) module_exit(fn);



/*IO write/read*/
#define AX_OSAL_IO_writel(v, x) (*((volatile int *)(x)) = (v))
#define AX_OSAL_IO_readl(x) (*((volatile int *)(x)))

void AX_OSAL_IO_bit_set(unsigned int *bitmap, int pos);
void AX_OSAL_IO_bit_clean(unsigned int *bitmap, int pos);

/* srcu api */
typedef struct AX_OSAL_srcu_struct {
	void *ssp;
} AX_OSAL_srcu_struct_t;

#ifndef CONFIG_DEBUG_LOCK_ALLOC
int AX_OSAL_init_srcu_struct(AX_OSAL_srcu_struct_t *ssp);
#else
#include <linux/slab.h>
void *AX_OSAL_DBG_init_srcu_struct(AX_OSAL_srcu_struct_t *ssp);
#define AX_OSAL_init_srcu_struct(ssp)				\
	({							\
		int ret = 0;					\
		void *p = AX_OSAL_DBG_init_srcu_struct(ssp);	\
		if (p != NULL) {				\
			ret = init_srcu_struct(p);		\
			if (ret != 0) {				\
				kfree(p);			\
				ssp->ssp = NULL;		\
			}					\
		} else {					\
			ret = -1;				\
		}						\
		ret;						\
	})
#endif
void AX_OSAL_cleanup_srcu_struct(AX_OSAL_srcu_struct_t *ssp);
int AX_OSAL_srcu_read_lock(AX_OSAL_srcu_struct_t *ssp);
void AX_OSAL_srcu_read_unlock(AX_OSAL_srcu_struct_t *ssp, int idx);
void AX_OSAL_synchronize_srcu(AX_OSAL_srcu_struct_t *ssp);

/* percpu api */
#define AX_OSAL___percpu __percpu
#define AX_OSAL_for_each_possible_cpu for_each_possible_cpu

unsigned int AX_OSAL___percpu *AX_OSAL_alloc_percpu_u32(void);
unsigned int *AX_OSAL_this_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr);
unsigned int *AX_OSAL_get_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr);
void AX_OSAL_put_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr);
unsigned int *AX_OSAL_per_cpu_ptr_u32(unsigned int AX_OSAL___percpu *ptr, int cpu);
void AX_OSAL_free_percpu_u32(unsigned int AX_OSAL___percpu *ptr);

/* prefetch api */
void AX_OSAL_prefetch(void *addr);
void AX_OSAL_prefetchw(void *addr);

#ifdef __cplusplus
}
#endif

#endif /*__OSAL_AX__H__*/
