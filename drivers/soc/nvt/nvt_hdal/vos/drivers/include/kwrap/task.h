#ifndef _VOS_TASK_H_
#define _VOS_TASK_H_

#include <kwrap/nvt_type.h>

#define VOS_RES_TYPE_INVALID 0
#define VOS_RES_TYPE_FLAG    1
#define VOS_RES_TYPE_SEM     2

#define VOS_DEF_STACK_SIZE   (8*1024)

// Note:
// FreeRTOS priorities, Highest: (configMAX_PRIORITIES(32)-1) ~ Lowest: (tskIDLE_PRIORITY(0) + 1)
// Linux kernel-space priorities, Highest: SCHED_RR(98) ~ SCHED_RR(1) ~ Lowest: Non-SCHED_RR(0)
// Linux user-space priorities, Highest: nice(-20) ~ nice(0) ~ Lowest: nice(+19)
// VOS priorities, Highest(1) ~ Lowest (30)
// VK_TASK_HIGHEST_PRIORITY is 1 to reserve 0 for debugging
// VK_TASK_LOWEST_PRIORITY is 30 to make sure FreeRTOS lowest priority > tskIDLE_PRIORITY(0)
#define VK_TASK_HIGHEST_PRIORITY    1
#define VK_TASK_LOWEST_PRIORITY     30

#if defined(__FREERTOS)
typedef void* VK_TASK_HANDLE;

#elif defined(__LINUX) && defined(__KERNEL__)
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/kernel.h>
typedef struct task_struct * VK_TASK_HANDLE;

#else //Linux user-space
typedef unsigned long int VK_TASK_HANDLE;
#endif

extern void                 rtos_task_init(unsigned long max_task_num);
extern void                 rtos_task_exit(void);
extern VK_TASK_HANDLE       vos_task_create(void *fp, void *parm, const char name[], int priority, int stksize);
extern void                 vos_task_destroy(VK_TASK_HANDLE task_hdl);
extern void                 vos_task_enter(void);
extern int                  vos_task_return(int rtn_val);
extern void                 vos_task_update_info(int wait_res, VK_TASK_HANDLE task_hdl, ID wairesid, FLGPTN waiptn, UINT wfmode);
extern void                 vos_task_dump(int (*dump)(const char *fmt, ...));
extern int                  vos_task_hdl2id(VK_TASK_HANDLE task_hdl);
extern VK_TASK_HANDLE       vos_task_id2hdl(int task_id);
extern void                 vos_task_dump_by_tskhdl(int (*dump)(const char *fmt, ...), VK_TASK_HANDLE task_hdl);
extern BOOL                 vos_task_chk_hdl_valid(VK_TASK_HANDLE task_hdl);
extern int                  vos_task_get_tid(void);
extern void                 vos_task_set_priority(VK_TASK_HANDLE task_hdl, int priority); //Range from VK_TASK_LOWEST_PRIORITY to VK_TASK_HIGHEST_PRIORITY
extern VK_TASK_HANDLE       vos_task_get_handle(void);
extern int                  vos_task_resume(VK_TASK_HANDLE task_hdl);
extern int                  vos_task_get_name(VK_TASK_HANDLE task_hdl, char *name, unsigned int len);

extern void                 vos_dump_stack_backtrace(UINT32 *info, UINT32 h_stack_base, INT32 h_stack_size, UINT32 level);
extern void                 vos_dump_stack_content(UINT32 h_stack_pointer, UINT32 h_stack_base, INT32 h_stack_size, UINT32 level);
extern void                 vos_dump_stack(void);
extern void                 vos_show_stack(VK_TASK_HANDLE takhdl, UINT32 level);

//===== obsolete API, please use vos_util_delay_xxx API =====
extern void                 vos_task_delay_ms(int ms); //VK_TASK_WAIT_INFINITELY to wait infinitely (Only supported by FreeRTOS)
extern void                 vos_task_delay_us(int us);
extern void                 vos_task_delay_us_polling(int us);
//===========================================================

#if defined(__FREERTOS)
// Note:
// 1. Do NOT call vTaskDelete directly. Use THREAD_RETURN() instead.
// 2. A FreeRTOS task should call THREAD_RETURN(0) or vTaskDelete(NULL) before exit,
//    otherwise an error message will be shown, e.g. vAssertCalled port.c, 328
extern void vTaskDelete(void *xTaskToDelete); //used by FreeRTOS THREAD_RETURN()
#endif

#if defined(__FREERTOS)
#define THREAD_HANDLE                           VK_TASK_HANDLE
#define THREAD_RETTYPE                          void
#define THREAD_DECLARE(name, arglist)           THREAD_RETTYPE name(void *arglist) //the same as FreeRTOS TaskFunction_t
#define THREAD_CREATE(handle, fp, parm, name)   handle = vos_task_create(fp, (void *)parm, name, 10, VOS_DEF_STACK_SIZE)
#define THREAD_RESUME(handle)                   vos_task_resume(handle)
#define THREAD_ENTRY()                          vos_task_enter()
#define THREAD_SHOULD_STOP                      0
#define THREAD_DESTROY(handle)                  vos_task_destroy(handle)
#define THREAD_RETURN(value)                    vos_task_return(value); vTaskDelete(NULL); return
#define THREAD_SELF(handle)                     handle = vos_task_get_handle()
#define THREAD_TID(thread_tid)                  thread_tid = vos_task_get_tid()
#define THREAD_SET_PRIORITY(handle, pri)        vos_task_set_priority(handle, pri)

#elif defined(__LINUX) && defined(__KERNEL__)
#define THREAD_HANDLE                           VK_TASK_HANDLE
#define THREAD_RETTYPE                          int
#define THREAD_DECLARE(name, arglist)           THREAD_RETTYPE name(void *arglist)
#define THREAD_CREATE(handle, fp, parm, name)   handle = vos_task_create(fp, (void *)parm, name, 0, VOS_DEF_STACK_SIZE)
#define THREAD_RESUME(handle)                   vos_task_resume(handle)
#define THREAD_ENTRY()                          vos_task_enter()
#define THREAD_SHOULD_STOP                      kthread_should_stop()
#define THREAD_DESTROY(handle)                  vos_task_destroy(handle)
#define THREAD_RETURN(value)                    return vos_task_return(value)
#define THREAD_SELF(handle)                     handle = vos_task_get_handle()
#define THREAD_TID(thread_tid)                  thread_tid = vos_task_get_tid()
#define THREAD_SET_PRIORITY(handle, pri)        vos_task_set_priority(handle, pri)

#else
#define THREAD_HANDLE                           VK_TASK_HANDLE
#if VOS_LEGACY_ARCH32
#define THREAD_RETTYPE                          void*
#else
#define THREAD_RETTYPE                          int
#endif
#define THREAD_DECLARE(name, arglist)           THREAD_RETTYPE name(void *arglist)
#define THREAD_CREATE(handle, fp, parm, name)   handle = vos_task_create(fp, (void *)parm, name, 0, VOS_DEF_STACK_SIZE)
#define THREAD_RESUME(handle)                   vos_task_resume(handle)
#define THREAD_ENTRY()                          vos_task_enter()
#define THREAD_SHOULD_STOP                      0
#define THREAD_DESTROY(handle)                  vos_task_destroy(handle)
#define THREAD_RETURN(value)                    return (THREAD_RETTYPE)vos_task_return(value)
#define THREAD_SELF(handle)                     handle = vos_task_get_handle()
#define THREAD_TID(thread_tid)                  thread_tid = vos_task_get_tid()
#define THREAD_SET_PRIORITY(handle, pri)        vos_task_set_priority(handle, pri)
#endif

#endif /* _VOS_TASK_H_ */

