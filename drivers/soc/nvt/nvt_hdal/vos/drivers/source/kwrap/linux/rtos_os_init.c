/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
//! Linux
#include <linux/init.h>
#include <linux/module.h>
#include <linux/moduleparam.h>

//! RTOS
#include <kwrap/task.h>
#include <kwrap/semaphore.h>
#include <kwrap/flag.h>
#include <kwrap/perf.h>
#include "rtos_os_proc.h"
#include "vos_ioctl.h"

/*-----------------------------------------------------------------------------*/
/* Local Constant Definitions                                                  */
/*-----------------------------------------------------------------------------*/
#define NVT_VOS_VERSION             "1.00.003"
// Total number of kernel ID sources
#define MAX_FLAG_COUNT              1000
#define MAX_TASK_COUNT              500
#define MAX_SEM_COUNT               2000
/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/


/*-----------------------------------------------------------------------------*/
/* Extern Global Variables                                                     */
/*-----------------------------------------------------------------------------*/
// GxSystem.a will start a idle task to detect dummy interrupt,
// and didn't provide API to turn this function off.
// We have to terminate the task via brute force
/*-----------------------------------------------------------------------------*/
/* Extern Function Prototype                                                   */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Function Protype                                                      */
/*-----------------------------------------------------------------------------*/

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
static int max_flag_count = 300;
static int max_task_count = 100;
static int max_sem_count  = 999;
module_param(max_flag_count, uint, 0644);
module_param(max_task_count, uint, 0644);
module_param(max_sem_count, uint, 0644);
MODULE_PARM_DESC(max_flag_count, "Set Max flag count");
MODULE_PARM_DESC(max_task_count, "Set Max task count");
MODULE_PARM_DESC(max_sem_count, "Set Max sem count");


/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
static int _rtos_init_(void)
{
	printk("nvt_vos: %s (%s %s)\n", NVT_VOS_VERSION, __DATE__, __TIME__);

	if (max_flag_count > MAX_FLAG_COUNT) {
		printk(KERN_ERR "rtos wrapper init max_flag_count = %d exceeds limit %d\n", max_flag_count, MAX_FLAG_COUNT);
		return -1;
	}
	if (max_task_count > MAX_TASK_COUNT) {
		printk(KERN_ERR "rtos wrapper init max_task_count = %d exceeds limit %d\n", max_task_count, MAX_TASK_COUNT);
		return -1;
	}
	if (max_sem_count > MAX_SEM_COUNT) {
		printk(KERN_ERR "rtos wrapper init max_sem_count = %d exceeds limit %d\n", max_sem_count, MAX_SEM_COUNT);
		return -1;
	}
	#if 0
	printk(KERN_INFO "rtos wrapper init max_flag_count = %d\n", max_flag_count);
	printk(KERN_INFO "rtos wrapper init max_task_count = %d\n", max_task_count);
	printk(KERN_INFO "rtos wrapper init max_sem_count = %d\n", max_sem_count);
	#endif
	vos_ioctl_init(NULL);
	rtos_sem_init(max_sem_count); // should before rtos_sem_init
	rtos_flag_init(max_flag_count);
	rtos_task_init(max_task_count);
	rtos_proc_init(NULL);
	vos_perf_init(NULL);
	return 0;
}

static void _rtos_exit_(void)
{
	rtos_sem_exit();
	rtos_task_exit();
	rtos_flag_exit();
	vos_perf_exit();
	vos_ioctl_exit();
	printk(KERN_INFO "rtos wrapper exit\n");
}

/*-----------------------------------------------------------------------------*/
/* Kernel Mode Definiton                                                       */
/*-----------------------------------------------------------------------------*/

#ifdef __NVT_VOS_KWRAP_USE_INITCALL
arch_initcall(_rtos_init_);
__exitcall(_rtos_exit_);
#else
module_init(_rtos_init_);
module_exit(_rtos_exit_);
#endif

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("rtos wrapper driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(NVT_VOS_VERSION);