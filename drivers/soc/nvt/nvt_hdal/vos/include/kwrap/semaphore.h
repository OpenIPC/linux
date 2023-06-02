#ifndef _VOS_SEMAPHORE_H_
#define _VOS_SEMAPHORE_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                                                                                 */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

//! Init/Exit flag
void  rtos_sem_init(unsigned long max_sem_num);
void  rtos_sem_exit(void);

//! Common api
int   vos_sem_create(ID *p_semid, int init_cnt, char *name);
void  vos_sem_destroy(ID semid);
int   vos_sem_wait(ID semid); //non-interruptible and wait forever
int   vos_sem_wait_timeout(ID semid, int timeout_tick); //non-interruptible and wait with timeout, use vos_util_msec_to_tick to convert to ticks
int   vos_sem_wait_interruptible(ID semid); //interruptible or wait forever
void  vos_sem_sig(ID semid);
char *vos_sem_get_name(ID semid);
void  vos_sem_dump(int (*dump)(const char *fmt, ...), int level);

#if defined(__LINUX)
#define vos_sem_isig vos_sem_sig
#elif defined(__FREERTOS)
void  vos_sem_isig(ID semid); //sem signal for ISR (FreeRTOS Only)
#endif

#define SEM_HANDLE                           ID
#define SEM_SIGNAL(id)                       vos_sem_sig(id)
#define SEM_SIGNAL_ISR(id)                   vos_sem_isig(id)
#define SEM_WAIT(id)                         vos_sem_wait(id)
#define SEM_WAIT_TIMEOUT(id, timeout)        vos_sem_wait_timeout(id, timeout)
#define SEM_WAIT_INTERRUPTIBLE(id)           vos_sem_wait_interruptible(id)
#define SEM_CREATE(id, init_cnt)             vos_sem_create(&id, init_cnt, (char *)#id)

#define OS_CONFIG_SEMPHORE(id, attr, cnt, max_cnt) vos_sem_create(&id, max_cnt, (char *)#id)
#define SEM_DESTROY(id)                      vos_sem_destroy(id)

// Provide Linux-style semaphore for Linux only --------- begin
#if defined(__LINUX) && defined(__KERNEL__)
struct vk_semaphore {
	unsigned int init_tag;
	unsigned int count;
	unsigned int buf[14];
};

#define __VK_SEMAPHORE_INITIALIZER(name, n)	{ .init_tag = 0, .count = n, .buf = {0} }
#define VK_DEFINE_SEMAPHORE(name) struct vk_semaphore name = __VK_SEMAPHORE_INITIALIZER(name, 1)

void vk_sema_init(struct vk_semaphore *p_vksem, int val);
void vk_down(struct vk_semaphore *p_vksem);
int vk_down_interruptible(struct vk_semaphore *p_vksem);
int vk_down_killable(struct vk_semaphore *p_vksem);
int vk_down_trylock(struct vk_semaphore *p_vksem);
int vk_down_timeout(struct vk_semaphore *p_vksem, long jiffies);
void vk_up(struct vk_semaphore *p_vksem);
#endif
// Provide Linux-style semaphore for Linux only --------- end

#ifdef __cplusplus
}
#endif

#endif /* _VOS_SEMAPHORE_H_ */

