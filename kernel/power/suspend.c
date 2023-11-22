// SPDX-License-Identifier: GPL-2.0-only
/*
 * kernel/power/suspend.c - Suspend to RAM and standby functionality.
 *
 * Copyright (c) 2003 Patrick Mochel
 * Copyright (c) 2003 Open Source Development Lab
 * Copyright (c) 2009 Rafael J. Wysocki <rjw@sisk.pl>, Novell Inc.
 */

#define pr_fmt(fmt) "PM: " fmt

#include <linux/string.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/console.h>
#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include <linux/gfp.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/export.h>
#include <linux/suspend.h>
#include <linux/syscore_ops.h>
#include <linux/swait.h>
#include <linux/ftrace.h>
#include <trace/events/power.h>
#include <linux/compiler.h>
#include <linux/moduleparam.h>
#include <linux/wakeup_reason.h>

#include "power.h"

const char * const pm_labels[] = {
	[PM_SUSPEND_TO_IDLE] = "freeze",
	[PM_SUSPEND_STANDBY] = "standby",
	[PM_SUSPEND_MEM] = "mem",
};
const char *pm_states[PM_SUSPEND_MAX];

#ifdef CONFIG_SS_PROFILING_TIME
extern void recode_timestamp_init(void);
extern void recode_timestamp(int mark, const char* name);
#endif

#ifdef CONFIG_MP_USB_STR_PATCH
typedef enum {
	E_STR_NONE,
	E_STR_IN_SUSPEND,
	E_STR_IN_RESUME
} EN_STR_STATUS;

static EN_STR_STATUS enStrStatus=E_STR_NONE;

bool is_suspending(void)
{
	return (enStrStatus == E_STR_IN_SUSPEND);
}
EXPORT_SYMBOL_GPL(is_suspending);
#endif

static const char * const mem_sleep_labels[] = {
	[PM_SUSPEND_TO_IDLE] = "s2idle",
	[PM_SUSPEND_STANDBY] = "shallow",
	[PM_SUSPEND_MEM] = "deep",
};
const char *mem_sleep_states[PM_SUSPEND_MAX];

suspend_state_t mem_sleep_current = PM_SUSPEND_TO_IDLE;
suspend_state_t mem_sleep_default = PM_SUSPEND_MAX;
suspend_state_t pm_suspend_target_state;
EXPORT_SYMBOL_GPL(pm_suspend_target_state);

unsigned int pm_suspend_global_flags;
EXPORT_SYMBOL_GPL(pm_suspend_global_flags);

static const struct platform_suspend_ops *suspend_ops;
static const struct platform_s2idle_ops *s2idle_ops;
static DECLARE_SWAIT_QUEUE_HEAD(s2idle_wait_head);

enum s2idle_states __read_mostly s2idle_state;
static DEFINE_RAW_SPINLOCK(s2idle_lock);

/**
 * pm_suspend_default_s2idle - Check if suspend-to-idle is the default suspend.
 *
 * Return 'true' if suspend-to-idle has been selected as the default system
 * suspend method.
 */
bool pm_suspend_default_s2idle(void)
{
	return mem_sleep_current == PM_SUSPEND_TO_IDLE;
}
EXPORT_SYMBOL_GPL(pm_suspend_default_s2idle);

void s2idle_set_ops(const struct platform_s2idle_ops *ops)
{
	lock_system_sleep();
	s2idle_ops = ops;
	unlock_system_sleep();
}

static void s2idle_begin(void)
{
	s2idle_state = S2IDLE_STATE_NONE;
}

static void s2idle_enter(void)
{
	trace_suspend_resume(TPS("machine_suspend"), PM_SUSPEND_TO_IDLE, true);

	raw_spin_lock_irq(&s2idle_lock);
	if (pm_wakeup_pending())
		goto out;

	s2idle_state = S2IDLE_STATE_ENTER;
	raw_spin_unlock_irq(&s2idle_lock);

	get_online_cpus();
	cpuidle_resume();

	/* Push all the CPUs into the idle loop. */
	wake_up_all_idle_cpus();
	/* Make the current CPU wait so it can enter the idle loop too. */
	swait_event_exclusive(s2idle_wait_head,
		    s2idle_state == S2IDLE_STATE_WAKE);

	cpuidle_pause();
	put_online_cpus();

	raw_spin_lock_irq(&s2idle_lock);

 out:
	s2idle_state = S2IDLE_STATE_NONE;
	raw_spin_unlock_irq(&s2idle_lock);

	trace_suspend_resume(TPS("machine_suspend"), PM_SUSPEND_TO_IDLE, false);
}

static void s2idle_loop(void)
{
	pm_pr_dbg("suspend-to-idle\n");

	/*
	 * Suspend-to-idle equals:
	 * frozen processes + suspended devices + idle processors.
	 * Thus s2idle_enter() should be called right after all devices have
	 * been suspended.
	 *
	 * Wakeups during the noirq suspend of devices may be spurious, so try
	 * to avoid them upfront.
	 */
	for (;;) {
		if (s2idle_ops && s2idle_ops->wake) {
			if (s2idle_ops->wake())
				break;
		} else if (pm_wakeup_pending()) {
			break;
		}

		pm_wakeup_clear(false);
		clear_wakeup_reasons();

		s2idle_enter();
	}

	pm_pr_dbg("resume from suspend-to-idle\n");
}

void s2idle_wake(void)
{
	unsigned long flags;

	raw_spin_lock_irqsave(&s2idle_lock, flags);
	if (s2idle_state > S2IDLE_STATE_NONE) {
		s2idle_state = S2IDLE_STATE_WAKE;
		swake_up_one(&s2idle_wait_head);
	}
	raw_spin_unlock_irqrestore(&s2idle_lock, flags);
}
EXPORT_SYMBOL_GPL(s2idle_wake);

static bool valid_state(suspend_state_t state)
{
	/*
	 * PM_SUSPEND_STANDBY and PM_SUSPEND_MEM states need low level
	 * support and need to be valid to the low level
	 * implementation, no valid callback implies that none are valid.
	 */
	return suspend_ops && suspend_ops->valid && suspend_ops->valid(state);
}

void __init pm_states_init(void)
{
	/* "mem" and "freeze" are always present in /sys/power/state. */
	pm_states[PM_SUSPEND_MEM] = pm_labels[PM_SUSPEND_MEM];
	pm_states[PM_SUSPEND_TO_IDLE] = pm_labels[PM_SUSPEND_TO_IDLE];
	/*
	 * Suspend-to-idle should be supported even without any suspend_ops,
	 * initialize mem_sleep_states[] accordingly here.
	 */
	mem_sleep_states[PM_SUSPEND_TO_IDLE] = mem_sleep_labels[PM_SUSPEND_TO_IDLE];
}

static int __init mem_sleep_default_setup(char *str)
{
	suspend_state_t state;

	for (state = PM_SUSPEND_TO_IDLE; state <= PM_SUSPEND_MEM; state++)
		if (mem_sleep_labels[state] &&
		    !strcmp(str, mem_sleep_labels[state])) {
			mem_sleep_default = state;
			break;
		}

	return 1;
}
__setup("mem_sleep_default=", mem_sleep_default_setup);

/**
 * suspend_set_ops - Set the global suspend method table.
 * @ops: Suspend operations to use.
 */
void suspend_set_ops(const struct platform_suspend_ops *ops)
{
	lock_system_sleep();

	suspend_ops = ops;

	if (valid_state(PM_SUSPEND_STANDBY)) {
		mem_sleep_states[PM_SUSPEND_STANDBY] = mem_sleep_labels[PM_SUSPEND_STANDBY];
		pm_states[PM_SUSPEND_STANDBY] = pm_labels[PM_SUSPEND_STANDBY];
		if (mem_sleep_default == PM_SUSPEND_STANDBY)
			mem_sleep_current = PM_SUSPEND_STANDBY;
	}
	if (valid_state(PM_SUSPEND_MEM)) {
		mem_sleep_states[PM_SUSPEND_MEM] = mem_sleep_labels[PM_SUSPEND_MEM];
		if (mem_sleep_default >= PM_SUSPEND_MEM)
			mem_sleep_current = PM_SUSPEND_MEM;
	}

	unlock_system_sleep();
}
EXPORT_SYMBOL_GPL(suspend_set_ops);

/**
 * suspend_valid_only_mem - Generic memory-only valid callback.
 *
 * Platform drivers that implement mem suspend only and only need to check for
 * that in their .valid() callback can use this instead of rolling their own
 * .valid() callback.
 */
int suspend_valid_only_mem(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM;
}
EXPORT_SYMBOL_GPL(suspend_valid_only_mem);

static bool sleep_state_supported(suspend_state_t state)
{
	return state == PM_SUSPEND_TO_IDLE || (suspend_ops && suspend_ops->enter);
}

static int platform_suspend_prepare(suspend_state_t state)
{
	return state != PM_SUSPEND_TO_IDLE && suspend_ops->prepare ?
		suspend_ops->prepare() : 0;
}

static int platform_suspend_prepare_late(suspend_state_t state)
{
	return state == PM_SUSPEND_TO_IDLE && s2idle_ops && s2idle_ops->prepare ?
		s2idle_ops->prepare() : 0;
}

static int platform_suspend_prepare_noirq(suspend_state_t state)
{
	if (state == PM_SUSPEND_TO_IDLE)
		return s2idle_ops && s2idle_ops->prepare_late ?
			s2idle_ops->prepare_late() : 0;

	return suspend_ops->prepare_late ? suspend_ops->prepare_late() : 0;
}

static void platform_resume_noirq(suspend_state_t state)
{
	if (state == PM_SUSPEND_TO_IDLE) {
		if (s2idle_ops && s2idle_ops->restore_early)
			s2idle_ops->restore_early();
	} else if (suspend_ops->wake) {
		suspend_ops->wake();
	}
}

static void platform_resume_early(suspend_state_t state)
{
	if (state == PM_SUSPEND_TO_IDLE && s2idle_ops && s2idle_ops->restore)
		s2idle_ops->restore();
}

static void platform_resume_finish(suspend_state_t state)
{
	if (state != PM_SUSPEND_TO_IDLE && suspend_ops->finish)
		suspend_ops->finish();
}

static int platform_suspend_begin(suspend_state_t state)
{
	if (state == PM_SUSPEND_TO_IDLE && s2idle_ops && s2idle_ops->begin)
		return s2idle_ops->begin();
	else if (suspend_ops && suspend_ops->begin)
		return suspend_ops->begin(state);
	else
		return 0;
}

static void platform_resume_end(suspend_state_t state)
{
	if (state == PM_SUSPEND_TO_IDLE && s2idle_ops && s2idle_ops->end)
		s2idle_ops->end();
	else if (suspend_ops && suspend_ops->end)
		suspend_ops->end();
}

static void platform_recover(suspend_state_t state)
{
	if (state != PM_SUSPEND_TO_IDLE && suspend_ops->recover)
		suspend_ops->recover();
}

static bool platform_suspend_again(suspend_state_t state)
{
	return state != PM_SUSPEND_TO_IDLE && suspend_ops->suspend_again ?
		suspend_ops->suspend_again() : false;
}

#ifdef CONFIG_PM_DEBUG
static unsigned int pm_test_delay = 5;
module_param(pm_test_delay, uint, 0644);
MODULE_PARM_DESC(pm_test_delay,
		 "Number of seconds to wait before resuming from suspend test");
#endif

static int suspend_test(int level)
{
#ifdef CONFIG_PM_DEBUG
	if (pm_test_level == level) {
		pr_info("suspend debug: Waiting for %d second(s).\n",
				pm_test_delay);
		mdelay(pm_test_delay * 1000);
		return 1;
	}
#endif /* !CONFIG_PM_DEBUG */
	return 0;
}

/**
 * suspend_prepare - Prepare for entering system sleep state.
 *
 * Common code run for every system sleep state that can be entered (except for
 * hibernation).  Run suspend notifiers, allocate the "suspend" console and
 * freeze processes.
 */
static int suspend_prepare(suspend_state_t state)
{
	int error;

	if (!sleep_state_supported(state))
		return -EPERM;

	pm_prepare_console();

	error = pm_notifier_call_chain_robust(PM_SUSPEND_PREPARE, PM_POST_SUSPEND);
	if (error)
		goto Restore;

	trace_suspend_resume(TPS("freeze_processes"), 0, true);
	error = suspend_freeze_processes();
	trace_suspend_resume(TPS("freeze_processes"), 0, false);
	if (!error)
		return 0;

	log_suspend_abort_reason("One or more tasks refusing to freeze");
	suspend_stats.failed_freeze++;
	dpm_save_failed_step(SUSPEND_FREEZE);
	pm_notifier_call_chain(PM_POST_SUSPEND);
 Restore:
	pm_restore_console();
	return error;
}

/* default implementation */
void __weak arch_suspend_disable_irqs(void)
{
	local_irq_disable();
}

/* default implementation */
void __weak arch_suspend_enable_irqs(void)
{
	local_irq_enable();
}
#ifdef CONFIG_SS_PM_POWER_SAVING_SCHEME

extern unsigned int pm_wakeup_irq;
extern struct irq_domain *ss_gpi_irq_domain;
extern struct irq_domain *ss_irq_domain;
extern unsigned int irq_find_mapping(struct irq_domain *domain, irq_hw_number_t hwirq);


typedef struct {
    char magic[8];
    unsigned int resume_entry;
    unsigned int count;
    unsigned int status;
    unsigned int password;
    unsigned int vadFrameAvg;
    unsigned int vadFrameResult;
    unsigned int vadCpuFreq;
    char vadLibVersion[32];
} suspend_keep_info;

extern suspend_keep_info *pStr_info;

int calc_checksum1(void *buf, int size)
{
    int i;
    int sum = 0;

    for (i = 0; size > i; i += 4)
        sum += *(volatile int*)(buf + i);
    return sum;
}
#endif

/**
 * suspend_enter - Make the system enter the given sleep state.
 * @state: System sleep state to enter.
 * @wakeup: Returns information that the sleep state should not be re-entered.
 *
 * This function should be called after devices have been suspended.
 */

static int suspend_enter(suspend_state_t state, bool *wakeup)
{
	int error, last_dev;
#ifdef CONFIG_SS_PM_POWER_SAVING_SCHEME
	unsigned int SIG1, SIG2;
#endif
	error = platform_suspend_prepare(state);
	if (error)
		goto Platform_finish;

	error = dpm_suspend_late(PMSG_SUSPEND);
	if (error) {
		last_dev = suspend_stats.last_failed_dev + REC_FAILED_NUM - 1;
		last_dev %= REC_FAILED_NUM;
		pr_err("late suspend of devices failed\n");
		log_suspend_abort_reason("late suspend of %s device failed",
					 suspend_stats.failed_devs[last_dev]);
		goto Platform_finish;
	}
	error = platform_suspend_prepare_late(state);
	if (error)
		goto Devices_early_resume;

	error = dpm_suspend_noirq(PMSG_SUSPEND);
	if (error) {
		last_dev = suspend_stats.last_failed_dev + REC_FAILED_NUM - 1;
		last_dev %= REC_FAILED_NUM;
		pr_err("noirq suspend of devices failed\n");
		log_suspend_abort_reason("noirq suspend of %s device failed",
					 suspend_stats.failed_devs[last_dev]);
		goto Platform_early_resume;
	}
	error = platform_suspend_prepare_noirq(state);
	if (error)
		goto Platform_wake;

	if (suspend_test(TEST_PLATFORM))
		goto Platform_wake;

	if (state == PM_SUSPEND_TO_IDLE) {
		s2idle_loop();
		goto Platform_wake;
	}

	error = suspend_disable_secondary_cpus();
	if (error || suspend_test(TEST_CPUS)) {
		log_suspend_abort_reason("Disabling non-boot cpus failed");
		goto Enable_cpus;
	}

	arch_suspend_disable_irqs();
	BUG_ON(!irqs_disabled());

	system_state = SYSTEM_SUSPEND;

	error = syscore_suspend();
	if (!error) {
		*wakeup = pm_wakeup_pending();
		if (!(suspend_test(TEST_CORE) || *wakeup)) {
			trace_suspend_resume(TPS("machine_suspend"),
				state, true);
			error = suspend_ops->enter(state);
			trace_suspend_resume(TPS("machine_suspend"),
				state, false);
		} else if (*wakeup) {
			error = -EBUSY;
		}
#ifdef CONFIG_MP_USB_STR_PATCH
		enStrStatus=E_STR_IN_RESUME;
#endif
#ifdef CONFIG_SS_PM_POWER_SAVING_SCHEME
        pStr_info = (suspend_keep_info *)(0xC0000000);

        SIG1= *((unsigned int *)&pStr_info->magic[0]);
        SIG2= *((unsigned int *)&pStr_info->magic[4]);

        if (!(SIG1 == 0x5F474953) && (SIG2 == 0x00525453)){
            pr_err("SSTAR-PM: Check Pattern fail ! while 1 !!!\n");
        }
        if(pStr_info->vadFrameResult==calc_checksum1(_text, __init_begin-1-_text))
        {
            pr_info("SSTAR-PM checksum PASS \r\n");
        }
        else{
            pr_err("SSTAR-PM: checksum fail !!!\r\n");
        }
    #endif
#ifdef CONFIG_SS_PROFILING_TIME
		recode_timestamp_init();
		recode_timestamp(__LINE__, "resume+");
#endif /* CONFIG_SS_PROFILING_TIME */
		syscore_resume();
	}

	system_state = SYSTEM_RUNNING;
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "en_irqs+");
#endif /* CONFIG_SS_PROFILING_TIME */
	arch_suspend_enable_irqs();
	BUG_ON(irqs_disabled());

 Enable_cpus:
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "en_cpus+");
#endif /* CONFIG_SS_PROFILING_TIME */
	suspend_enable_secondary_cpus();

 Platform_wake:
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "plat_noirq+");
#endif /* CONFIG_SS_PROFILING_TIME */
	platform_resume_noirq(state);
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "dpm_noirq+");
	pr_info("PM: dpm_noirq\n");
#endif /* CONFIG_SS_PROFILING_TIME */
	dpm_resume_noirq(PMSG_RESUME);

//#ifdef CONFIG_SS_PM_WAKEUP_PATCH
/*Record wakeup irq only gpio mode*/
    pm_wakeup_irq = irq_find_mapping(ss_gpi_irq_domain, *(unsigned short volatile *) 0xFD20080C); //hwirq

    if(!pm_wakeup_irq)
        pm_wakeup_irq = irq_find_mapping(ss_irq_domain, *(unsigned short volatile *) 0xFD20080C); //hwirq

    pr_info("pm_wakeup_irq %d vir %d\r\n", *(unsigned short volatile *) 0xFD20080C, pm_wakeup_irq);
//#endif

 Platform_early_resume:
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "plat_early+");
#endif /* CONFIG_SS_PROFILING_TIME */
	platform_resume_early(state);

 Devices_early_resume:
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "dev_early+");
#endif /* CONFIG_SS_PROFILING_TIME */
	dpm_resume_early(PMSG_RESUME);

 Platform_finish:
	platform_resume_finish(state);
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "plat_finish-");
#endif /* CONFIG_SS_PROFILING_TIME */
	return error;
}

/**
 * suspend_devices_and_enter - Suspend devices and enter system sleep state.
 * @state: System sleep state to enter.
 */
int suspend_devices_and_enter(suspend_state_t state)
{
	int error;
	bool wakeup = false;

	if (!sleep_state_supported(state))
		return -ENOSYS;

	pm_suspend_target_state = state;

	if (state == PM_SUSPEND_TO_IDLE)
		pm_set_suspend_no_platform();

	error = platform_suspend_begin(state);
	if (error)
		goto Close;

	suspend_console();
	suspend_test_start();
	error = dpm_suspend_start(PMSG_SUSPEND);
	if (error) {
		pr_err("Some devices failed to suspend, or early wake event detected\n");
		log_suspend_abort_reason(
				"Some devices failed to suspend, or early wake event detected");
		goto Recover_platform;
	}
	suspend_test_finish("suspend devices");
	if (suspend_test(TEST_DEVICES))
		goto Recover_platform;

	do {
		error = suspend_enter(state, &wakeup);
	} while (!error && !wakeup && platform_suspend_again(state));

 Resume_devices:
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "resume_end+");
	pr_info("PM: dpm_resume_end\n");
#endif /* CONFIG_SS_PROFILING_TIME */
	suspend_test_start();
	dpm_resume_end(PMSG_RESUME);
	suspend_test_finish("resume devices");
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "resume_end-");
#endif /* CONFIG_SS_PROFILING_TIME */
	trace_suspend_resume(TPS("resume_console"), state, true);
	resume_console();
	trace_suspend_resume(TPS("resume_console"), state, false);
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "console-");
#endif /* CONFIG_SS_PROFILING_TIME */

 Close:
#ifdef CONFIG_SS_PROFILING_TIME
	recode_timestamp(__LINE__, "plat_end+");
#endif /* CONFIG_SS_PROFILING_TIME */
	platform_resume_end(state);
	pm_suspend_target_state = PM_SUSPEND_ON;
	return error;

 Recover_platform:
	platform_recover(state);
	goto Resume_devices;
}

/**
 * suspend_finish - Clean up before finishing the suspend sequence.
 *
 * Call platform code to clean up, restart processes, and free the console that
 * we've allocated. This routine is not called for hibernation.
 */
static void suspend_finish(void)
{
	suspend_thaw_processes();
	pm_notifier_call_chain(PM_POST_SUSPEND);
	pm_restore_console();
}

/**
 * enter_state - Do common work needed to enter system sleep state.
 * @state: System sleep state to enter.
 *
 * Make sure that no one else is trying to put the system into a sleep state.
 * Fail if that's not the case.  Otherwise, prepare for system suspend, make the
 * system enter the given sleep state and clean up after wakeup.
 */
static int enter_state(suspend_state_t state)
{
	int error;

	trace_suspend_resume(TPS("suspend_enter"), state, true);
	if (state == PM_SUSPEND_TO_IDLE) {
#ifdef CONFIG_PM_DEBUG
		if (pm_test_level != TEST_NONE && pm_test_level <= TEST_CPUS) {
			pr_warn("Unsupported test mode for suspend to idle, please choose none/freezer/devices/platform.\n");
			return -EAGAIN;
		}
#endif
	} else if (!valid_state(state)) {
		return -EINVAL;
	}
	if (!mutex_trylock(&system_transition_mutex))
		return -EBUSY;
#ifdef CONFIG_MP_USB_STR_PATCH
        enStrStatus=E_STR_IN_SUSPEND;
#endif

	if (state == PM_SUSPEND_TO_IDLE)
		s2idle_begin();

	if (sync_on_suspend_enabled) {
		trace_suspend_resume(TPS("sync_filesystems"), 0, true);
		ksys_sync_helper();
		trace_suspend_resume(TPS("sync_filesystems"), 0, false);
	}

	pm_pr_dbg("Preparing system for sleep (%s)\n", mem_sleep_labels[state]);
	pm_suspend_clear_flags();
	error = suspend_prepare(state);
	if (error)
		goto Unlock;

	if (suspend_test(TEST_FREEZER))
		goto Finish;

	trace_suspend_resume(TPS("suspend_enter"), state, false);
	pm_pr_dbg("Suspending system (%s)\n", mem_sleep_labels[state]);
	pm_restrict_gfp_mask();
	error = suspend_devices_and_enter(state);
	pm_restore_gfp_mask();

 Finish:
	events_check_enabled = false;
	pm_pr_dbg("Finishing wakeup.\n");
	suspend_finish();
 Unlock:
#ifdef CONFIG_MP_USB_STR_PATCH
	enStrStatus=E_STR_NONE;
#endif
	mutex_unlock(&system_transition_mutex);
	return error;
}

/**
 * pm_suspend - Externally visible function for suspending the system.
 * @state: System sleep state to enter.
 *
 * Check if the value of @state represents one of the supported states,
 * execute enter_state() and update system suspend statistics.
 */
int pm_suspend(suspend_state_t state)
{
	int error;

	if (state <= PM_SUSPEND_ON || state >= PM_SUSPEND_MAX)
		return -EINVAL;

	pr_info("suspend entry (%s)\n", mem_sleep_labels[state]);
	error = enter_state(state);
	if (error) {
		suspend_stats.fail++;
		dpm_save_failed_errno(error);
	} else {
		suspend_stats.success++;
	}
	pr_info("suspend exit\n");
	return error;
}
EXPORT_SYMBOL(pm_suspend);
