#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/tty_driver.h>
#include <linux/console.h>
#include <linux/init.h>
#include <linux/jiffies.h>
#include <linux/nmi.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/delay.h>
#include <linux/smp.h>
#include <linux/security.h>
#include <linux/bootmem.h>
#include <linux/memblock.h>
#include <linux/syscalls.h>
#include <linux/crash_core.h>
#include <linux/kdb.h>
#include <linux/ratelimit.h>
#include <linux/kmsg_dump.h>
#include <linux/syslog.h>
#include <linux/cpu.h>
#include <linux/rculist.h>
#include <linux/poll.h>
#include <linux/irq_work.h>
#include <linux/ctype.h>
#include <linux/uio.h>
#include <linux/sched/clock.h>
#include <linux/sched/debug.h>
#include <linux/sched/task_stack.h>
#include <linux/uaccess.h>
#include <asm/sections.h>
#include <trace/events/initcall.h>
#include "../../fs/proc/axlogctl.h"
#include "internal.h"

static axlogctl_mem_cfg_ptr s_axlogctl_cfg_ptr = NULL;

static inline u32 ax_printk_caller_id(void)
{
	return in_task()? task_pid_nr(current) : raw_smp_processor_id();
}

static inline u8 ax_printk_cpu_id(void)
{
	return raw_smp_processor_id();
}

enum log_flags {
	LOG_NEWLINE	= 2,	/* text ended with a newline */
	LOG_PREFIX	= 4,	/* text started with a prefix */
	LOG_CONT	= 8,	/* text is a fragment of a continuation line */
};

struct ax_printk_log {
	u64 ts_nsec;		/* timestamp in nanoseconds */
	u16 len;		/* length of entire record */
	u16 text_len;		/* length of text buffer */
	u16 dict_len;		/* length of dictionary buffer */
	u8 mid;			/* module id */
	u8 flags:5;		/* internal record flags */
	u8 level:3;		/* syslog level */
	u8      cpu_id;         /* cpu id*/
	u32	caller_id;	/* thread id or processor id */
	char    tag[32];
}
#ifdef CONFIG_HAVE_EFFICIENT_UNALIGNED_ACCESS
__packed __aligned(4)
#endif
;

/*
 * The logbuf_lock protects kmsg buffer, indices, counters.  This can be taken
 * within the scheduler's rq lock. It must be released before calling
 * console_unlock() or anything else that might wake up a process.
 */
DEFINE_RAW_SPINLOCK(ax_logbuf_lock);

/*
 * Helper macros to lock/unlock logbuf_lock and switch between
 * printk-safe/unsafe modes.
 */
#define logbuf_lock_irq()				\
	do {						\
		printk_safe_enter_irq();		\
		raw_spin_lock(&ax_logbuf_lock);		\
	} while (0)

#define logbuf_unlock_irq()				\
	do {						\
		raw_spin_unlock(&ax_logbuf_lock);		\
		printk_safe_exit_irq();			\
	} while (0)

#define logbuf_lock_irqsave(flags)			\
	do {						\
		printk_safe_enter_irqsave(flags);	\
		raw_spin_lock(&ax_logbuf_lock);		\
	} while (0)

#define logbuf_unlock_irqrestore(flags)		\
	do {						\
		raw_spin_unlock(&ax_logbuf_lock);		\
		printk_safe_exit_irqrestore(flags);	\
	} while (0)

#ifdef CONFIG_PRINTK
DECLARE_WAIT_QUEUE_HEAD(ax_log_wait);
/* the next printk record to read by syslog(READ) or /proc/kmsg */
static u64 syslog_seq;
static u32 syslog_idx;
static size_t syslog_partial;

/* index and sequence number of the first record stored in the buffer */
static u64 log_first_seq;
static u32 log_first_idx;

/* index and sequence number of the next record to store in the buffer */
static u64 log_next_seq;
static u32 log_next_idx;

/* the next printk record to read after the last 'clear' command */
static u64 clear_seq;
static u32 clear_idx;

#define PREFIX_MAX		32
#define LOG_LINE_MAX		(1024 - PREFIX_MAX)

#define LOG_LEVEL(v)		((v) & 0x07)
#define LOG_FACILITY(v)		((v) >> 3 & 0xff)

/* record buffer */
#define LOG_ALIGN __alignof__(struct ax_printk_log)
#define __LOG_BUF_LEN (1 << CONFIG_AXLOG_BUF_SHIFT)
#define LOG_BUF_LEN_MAX (u32)(1 << 31)
static char __ax_log_buf[__LOG_BUF_LEN] __aligned(LOG_ALIGN);
static char *ax_log_buf = __ax_log_buf;
static u32 log_buf_len = __LOG_BUF_LEN;
static const char axlog_level_flags[]="MACEWNID";

/* Return log buffer address */
char *ax_log_buf_addr_get(void)
{
	return ax_log_buf;
}

/* Return log buffer size */
u32 ax_log_buf_len_get(void)
{
	return log_buf_len;
}

/* human readable text of the record */
static char *ax_log_text(const struct ax_printk_log *msg)
{
	return (char *)msg + sizeof(struct ax_printk_log);
}

/* optional key/value pair dictionary attached to the record */
static char *ax_log_dict(const struct ax_printk_log *msg)
{
	return (char *)msg + sizeof(struct ax_printk_log) + msg->text_len;
}

/* get record by index; idx must point to valid msg */
static struct ax_printk_log *ax_log_from_idx(u32 idx)
{
	struct ax_printk_log *msg = (struct ax_printk_log *)(ax_log_buf + idx);

	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer.
	 */
	if (!msg->len)
		return (struct ax_printk_log *)ax_log_buf;
	return msg;
}

/* get next record; idx must point to valid msg */
static u32 log_next(u32 idx)
{
	struct ax_printk_log *msg = (struct ax_printk_log *)(ax_log_buf + idx);

	/* length == 0 indicates the end of the buffer; wrap */
	/*
	 * A length == 0 record is the end of buffer marker. Wrap around and
	 * read the message at the start of the buffer as *this* one, and
	 * return the one after that.
	 */
	if (!msg->len) {
		msg = (struct ax_printk_log *)ax_log_buf;
		return msg->len;
	}
	return idx + msg->len;
}

/*
 * Check whether there is enough free space for the given message.
 *
 * The same values of first_idx and next_idx mean that the buffer
 * is either empty or full.
 *
 * If the buffer is empty, we must respect the position of the indexes.
 * They cannot be reset to the beginning of the buffer.
 */
static int logbuf_has_space(u32 msg_size, bool empty)
{
	u32 free;

	if (log_next_idx > log_first_idx || empty)
		free = max(log_buf_len - log_next_idx, log_first_idx);
	else
		free = log_first_idx - log_next_idx;

	/*
	 * We need space also for an empty header that signalizes wrapping
	 * of the buffer.
	 */
	return free >= msg_size + sizeof(struct ax_printk_log);
}

static int log_make_free_space(u32 msg_size)
{
	while (log_first_seq < log_next_seq &&
	       !logbuf_has_space(msg_size, false)) {
		/* drop old messages until we have enough contiguous space */
		log_first_idx = log_next(log_first_idx);
		log_first_seq++;
	}

	if (clear_seq < log_first_seq) {
		clear_seq = log_first_seq;
		clear_idx = log_first_idx;
	}

	/* sequence numbers are equal, so the log buffer is empty */
	if (logbuf_has_space(msg_size, log_first_seq == log_next_seq))
		return 0;

	return -ENOMEM;
}

/* compute the message size including the padding bytes */
static u32 msg_used_size(u16 text_len, u16 dict_len, u32 *pad_len)
{
	u32 size;

	size = sizeof(struct ax_printk_log) + text_len + dict_len;
	*pad_len = (-size) & (LOG_ALIGN - 1);
	size += *pad_len;

	return size;
}

/*
 * Define how much of the log buffer we could take at maximum. The value
 * must be greater than two. Note that only half of the buffer is available
 * when the index points to the middle.
 */
#define MAX_LOG_TAKE_PART 4
static const char trunc_msg[] = "<truncated>";

static u32 truncate_msg(u16 *text_len, u16 *trunc_msg_len,
			u16 *dict_len, u32 *pad_len)
{
	/*
	 * The message should not take the whole buffer. Otherwise, it might
	 * get removed too soon.
	 */
	u32 max_text_len = log_buf_len / MAX_LOG_TAKE_PART;
	if (*text_len > max_text_len)
		*text_len = max_text_len;
	/* enable the warning message */
	*trunc_msg_len = strlen(trunc_msg);
	/* disable the "dict" completely */
	*dict_len = 0;
	/* compute the size again, count also the warning message */
	return msg_used_size(*text_len + *trunc_msg_len, 0, pad_len);
}

/* insert record into the buffer, discard old ones, update heads */
static int log_store(const int id, const char* tag, int level,
		     enum log_flags flags, u64 ts_nsec,
		     const char *dict, u16 dict_len,
		     const char *text, u16 text_len)
{
	struct ax_printk_log *msg;
	u32 size, pad_len;
	u16 trunc_msg_len = 0;

	/* number of '\0' padding bytes to next message */
	size = msg_used_size(text_len, dict_len, &pad_len);

	if (log_make_free_space(size)) {
		/* truncate the message if it is too long for empty buffer */
		size = truncate_msg(&text_len, &trunc_msg_len,
				    &dict_len, &pad_len);
		/* survive when the log buffer is too small for trunc_msg */
		if (log_make_free_space(size))
			return 0;
	}

	if (log_next_idx + size + sizeof(struct ax_printk_log) > log_buf_len) {
		/*
		 * This message + an additional empty header does not fit
		 * at the end of the buffer. Add an empty header with len == 0
		 * to signify a wrap around.
		 */
		memset(ax_log_buf + log_next_idx, 0, sizeof(struct ax_printk_log));
		log_next_idx = 0;
	}

	/* fill message */
	msg = (struct ax_printk_log *)(ax_log_buf + log_next_idx);
	memcpy(ax_log_text(msg), text, text_len);
	msg->text_len = text_len;
	if (trunc_msg_len) {
		memcpy(ax_log_text(msg) + text_len, trunc_msg, trunc_msg_len);
		msg->text_len += trunc_msg_len;
	}
	if(dict)
		memcpy(ax_log_dict(msg), dict, dict_len);
	msg->dict_len = dict_len;
	msg->mid = id & 0xff;
	msg->level = level & 7;
	msg->flags = flags & 0x1f;
	msg->caller_id = ax_printk_caller_id();
	msg->cpu_id = ax_printk_cpu_id();
	if (tag)
		snprintf(msg->tag, sizeof(msg->tag), tag);
	if (ts_nsec > 0)
		msg->ts_nsec = ts_nsec;
	else
		msg->ts_nsec = local_clock();
	memset(ax_log_dict(msg) + dict_len, 0, pad_len);
	msg->len = size;

	/* insert message */
	log_next_idx += msg->len;
	log_next_seq++;

	return msg->text_len;
}

int ax_dmesg_restrict = IS_ENABLED(CONFIG_SECURITY_DMESG_RESTRICT);

static int syslog_action_restricted(int type)
{
	if (ax_dmesg_restrict)
		return 1;
	/*
	 * Unless restricted, we allow "read all" and "get buffer size"
	 * for everybody.
	 */
	return type != SYSLOG_ACTION_READ_ALL &&
	       type != SYSLOG_ACTION_SIZE_BUFFER;
}

static int check_syslog_permissions(int type, int source)
{
	/*
	 * If this is from /proc/kmsg and we've already opened it, then we've
	 * already done the capabilities checks at open time.
	 */
	if (source == SYSLOG_FROM_PROC && type != SYSLOG_ACTION_OPEN)
		goto ok;

	if (syslog_action_restricted(type)) {
		if (capable(CAP_SYSLOG))
			goto ok;
		/*
		 * For historical reasons, accept CAP_SYS_ADMIN too, with
		 * a warning.
		 */
		if (capable(CAP_SYS_ADMIN)) {
			pr_warn_once("%s (%d): Attempt to access syslog with "
				     "CAP_SYS_ADMIN but no CAP_SYSLOG "
				     "(deprecated).\n",
				 current->comm, task_pid_nr(current));
			goto ok;
		}
		return -EPERM;
	}
ok:
	return security_syslog(type);
}

/* requested log_buf_len from kernel cmdline */
static unsigned long __initdata new_log_buf_len;

/* we practice scaling the ring buffer by powers of 2 */
static void __init log_buf_len_update(u64 size)
{
	if (size > (u64)LOG_BUF_LEN_MAX) {
		size = (u64)LOG_BUF_LEN_MAX;
		pr_err("log_buf over 2G is not supported.\n");
	}

	if (size)
		size = roundup_pow_of_two(size);
	if (size > log_buf_len)
		new_log_buf_len = (unsigned long)size;
}

/* save requested log_buf_len since it's too early to process it */
static int __init log_buf_len_setup(char *str)
{
	u64 size;

	if (!str)
		return -EINVAL;

	size = memparse(str, &str);

	log_buf_len_update(size);

	return 0;
}
early_param("log_buf_len", log_buf_len_setup);

#ifdef CONFIG_SMP
#define __LOG_CPU_MAX_BUF_LEN (1 << CONFIG_LOG_CPU_MAX_BUF_SHIFT)

static void __init log_buf_add_cpu(void)
{
	unsigned int cpu_extra;

	/*
	 * archs should set up cpu_possible_bits properly with
	 * set_cpu_possible() after setup_arch() but just in
	 * case lets ensure this is valid.
	 */
	if (num_possible_cpus() == 1)
		return;

	cpu_extra = (num_possible_cpus() - 1) * __LOG_CPU_MAX_BUF_LEN;

	/* by default this will only continue through for large > 64 CPUs */
	if (cpu_extra <= __LOG_BUF_LEN / 2)
		return;

	pr_info("log_buf_len individual max cpu contribution: %d bytes\n",
		__LOG_CPU_MAX_BUF_LEN);
	pr_info("log_buf_len total cpu_extra contributions: %d bytes\n",
		cpu_extra);
	pr_info("log_buf_len min size: %d bytes\n", __LOG_BUF_LEN);

	log_buf_len_update(cpu_extra + __LOG_BUF_LEN);
}
#else /* !CONFIG_SMP */
static inline void log_buf_add_cpu(void) {}
#endif /* CONFIG_SMP */

void __init ax_setup_log_buf(int early)
{
	unsigned long flags;
	char *new_log_buf;
	unsigned int free;

	if (ax_log_buf != __ax_log_buf)
		return;

	if (!early && !new_log_buf_len)
		log_buf_add_cpu();

	if (!new_log_buf_len)
		return;

	if (early) {
		new_log_buf =
			memblock_virt_alloc(new_log_buf_len, LOG_ALIGN);
	} else {
		new_log_buf = memblock_virt_alloc_nopanic(new_log_buf_len,
							  LOG_ALIGN);
	}

	if (unlikely(!new_log_buf)) {
		pr_err("log_buf_len: %lu bytes not available\n",
			new_log_buf_len);
		return;
	}

	logbuf_lock_irqsave(flags);
	log_buf_len = new_log_buf_len;
	ax_log_buf = new_log_buf;
	new_log_buf_len = 0;
	free = __LOG_BUF_LEN - log_next_idx;
	memcpy(ax_log_buf, __ax_log_buf, __LOG_BUF_LEN);
	logbuf_unlock_irqrestore(flags);

	pr_info("log_buf_len: %u bytes\n", log_buf_len);
	pr_info("early log buf free: %u(%u%%)\n",
		free, (free * 100) / __LOG_BUF_LEN);
}

static bool __read_mostly ignore_loglevel;

static int __init ignore_loglevel_setup(char *str)
{
	ignore_loglevel = true;
	pr_info("debug: ignoring loglevel setting.\n");

	return 0;
}

early_param("ignore_loglevel", ignore_loglevel_setup);
module_param(ignore_loglevel, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ignore_loglevel,
		 "ignore loglevel setting (prints all kernel messages to the console)");

#ifdef CONFIG_BOOT_PRINTK_DELAY

static int boot_delay; /* msecs delay after each printk during bootup */
static unsigned long long loops_per_msec;	/* based on boot_delay */

static int __init boot_delay_setup(char *str)
{
	unsigned long lpj;

	lpj = preset_lpj ? preset_lpj : 1000000;	/* some guess */
	loops_per_msec = (unsigned long long)lpj / 1000 * HZ;

	get_option(&str, &boot_delay);
	if (boot_delay > 10 * 1000)
		boot_delay = 0;

	pr_debug("boot_delay: %u, preset_lpj: %ld, lpj: %lu, "
		"HZ: %d, loops_per_msec: %llu\n",
		boot_delay, preset_lpj, lpj, HZ, loops_per_msec);
	return 0;
}
early_param("boot_delay", boot_delay_setup);

static void boot_delay_msec(int level)
{
	unsigned long long k;
	unsigned long timeout;

	if ((boot_delay == 0 || system_state >= SYSTEM_RUNNING)
		|| suppress_message_printing(level)) {
		return;
	}

	k = (unsigned long long)loops_per_msec * boot_delay;

	timeout = jiffies + msecs_to_jiffies(boot_delay);
	while (k) {
		k--;
		cpu_relax();
		/*
		 * use (volatile) jiffies to prevent
		 * compiler reduction; loop termination via jiffies
		 * is secondary and may or may not happen.
		 */
		if (time_after(jiffies, timeout))
			break;
		touch_nmi_watchdog();
	}
}
#else
static inline void boot_delay_msec(int level)
{
}
#endif

static bool printk_time = IS_ENABLED(CONFIG_PRINTK_TIME);
module_param_named(time, printk_time, bool, S_IRUGO | S_IWUSR);

static size_t ax_print_time(u64 ts, char *buf)
{
	unsigned long rem_nsec;

	if (!printk_time)
		return 0;

	rem_nsec = do_div(ts, 1000000000);

	if (!buf)
		return snprintf(NULL, 0, "[%5lu.000000] ", (unsigned long)ts);

	return sprintf(buf, "[%5lu.%06lu] ",
		       (unsigned long)ts, rem_nsec / 1000);
}

static size_t ax_print_modinfo(const struct ax_printk_log *msg, char *buf)
{
	if (!buf)
		return snprintf(NULL, 0, " [C%u][%s][%c][%u] ", msg->cpu_id, msg->tag, axlog_level_flags[msg->level&7], msg->caller_id);

	return sprintf(buf, " [C%u][%s][%c][%u] ", msg->cpu_id, msg->tag, axlog_level_flags[msg->level&7], msg->caller_id);
}

static size_t ax_print_prefix(const struct ax_printk_log *msg, bool syslog, char *buf)
{
	size_t len = 0;
	unsigned int prefix = (msg->mid << 3) | msg->level;

	if (syslog) {
		if (buf) {
			len += sprintf(buf, "<%u>", prefix);
		} else {
			len += 3;
			if (prefix > 999)
				len += 3;
			else if (prefix > 99)
				len += 2;
			else if (prefix > 9)
				len++;
		}
	}

	len += ax_print_time(msg->ts_nsec, buf ? buf + len : NULL);


	/*[cpu][tag][level][tid]*/
	len += ax_print_modinfo(msg, buf ? buf + len : NULL);

	return len;
}

static size_t ax_msg_print_text(const struct ax_printk_log *msg, bool syslog, char *buf, size_t size)
{
	const char *text = ax_log_text(msg);
	size_t text_size = msg->text_len;
	size_t len = 0;

	do {
		const char *next = memchr(text, '\n', text_size);
		size_t text_len;

		if (next) {
			text_len = next - text;
			next++;
			text_size -= next - text;
		} else {
			text_len = text_size;
		}

		if (buf) {
			if (ax_print_prefix(msg, syslog, NULL) +
			    text_len + 1 >= size - len)
				break;

			len += ax_print_prefix(msg, syslog, buf + len);
			memcpy(buf + len, text, text_len);
			len += text_len;
			buf[len++] = '\n';
		} else {
			/* SYSLOG_ACTION_* buffer size only calculation */
			len += ax_print_prefix(msg, syslog, NULL);
			len += text_len;
			len++;
		}

		text = next;
	} while (text);

	return len;
}

static int syslog_print(char __user *buf, int size)
{
	char *text;
	struct ax_printk_log *msg;
	int len = 0;
	text = kmalloc(LOG_LINE_MAX + PREFIX_MAX, GFP_KERNEL);
	if (!text)
		return -ENOMEM;

	while (size > 0) {
		size_t n;
		size_t skip;

		logbuf_lock_irq();
		if (syslog_seq < log_first_seq) {
			/* messages are gone, move to first one */
			syslog_seq = log_first_seq;
			syslog_idx = log_first_idx;
			syslog_partial = 0;
		}
		if (syslog_seq == log_next_seq) {
			logbuf_unlock_irq();
			break;
		}

		skip = syslog_partial;
		msg = ax_log_from_idx(syslog_idx);
		n = ax_msg_print_text(msg, true, text, LOG_LINE_MAX + PREFIX_MAX);
		if (n - syslog_partial <= size) {
			/* message fits into buffer, move forward */
			syslog_idx = log_next(syslog_idx);
			syslog_seq++;
			n -= syslog_partial;
			syslog_partial = 0;
		} else if (!len){
			/* partial read(), remember position */
			n = size;
			syslog_partial += n;
		} else
			n = 0;
		logbuf_unlock_irq();

		if (!n)
			break;
		if (copy_to_user(buf, text + skip, n)) {
			if (!len)
				len = -EFAULT;
			break;
		}

		len += n;
		size -= n;
		buf += n;
	}

	kfree(text);
	return len;
}

static int syslog_print_all(char __user *buf, int size, bool clear)
{
	char *text;
	int len = 0;
	u64 next_seq;
	u64 seq;
	u32 idx;

	text = kmalloc(LOG_LINE_MAX + PREFIX_MAX, GFP_KERNEL);
	if (!text)
		return -ENOMEM;

	logbuf_lock_irq();
	/*
	 * Find first record that fits, including all following records,
	 * into the user-provided buffer for this dump.
	 */
	seq = clear_seq;
	idx = clear_idx;
	while (seq < log_next_seq) {
		struct ax_printk_log *msg = ax_log_from_idx(idx);

		len += ax_msg_print_text(msg, true, NULL, 0);
		idx = log_next(idx);
		seq++;
	}

	/* move first record forward until length fits into the buffer */
	seq = clear_seq;
	idx = clear_idx;
	while (len > size && seq < log_next_seq) {
		struct ax_printk_log *msg = ax_log_from_idx(idx);

		len -= ax_msg_print_text(msg, true, NULL, 0);
		idx = log_next(idx);
		seq++;
	}

	/* last message fitting into this dump */
	next_seq = log_next_seq;

	len = 0;
	while (len >= 0 && seq < next_seq) {
		struct ax_printk_log *msg = ax_log_from_idx(idx);
		int textlen;

		textlen = ax_msg_print_text(msg, true, text,
					 LOG_LINE_MAX + PREFIX_MAX);
		if (textlen < 0) {
			len = textlen;
			break;
		}
		idx = log_next(idx);
		seq++;

		logbuf_unlock_irq();
		if (copy_to_user(buf + len, text, textlen))
			len = -EFAULT;
		else
			len += textlen;
		logbuf_lock_irq();

		if (seq < log_first_seq) {
			/* messages are gone, move to next one */
			seq = log_first_seq;
			idx = log_first_idx;
		}
	}

	if (clear) {
		clear_seq = log_next_seq;
		clear_idx = log_next_idx;
	}
	logbuf_unlock_irq();

	kfree(text);
	return len;
}

static void syslog_clear(void)
{
	logbuf_lock_irq();
	clear_seq = log_next_seq;
	clear_idx = log_next_idx;
	logbuf_unlock_irq();
}

int do_axsyslog(int type, char __user *buf, int len, int source)
{
	bool clear = false;
	int error;
	error = check_syslog_permissions(type, source);
	if (error)
		return error;

	switch (type) {
	case SYSLOG_ACTION_CLOSE:	/* Close log */
		break;
	case SYSLOG_ACTION_OPEN:	/* Open log */
		break;
	case SYSLOG_ACTION_READ:	/* Read from log */
		if (!buf || len < 0)
			return -EINVAL;
		if (!len)
			return 0;
		if (!access_ok(VERIFY_WRITE, buf, len)){
			return -EFAULT;
		}
		error = wait_event_interruptible(ax_log_wait,
						 syslog_seq != log_next_seq);
		if (error)
			return error;
		error = syslog_print(buf, len);
		break;
	/* Read/clear last kernel messages */
	case SYSLOG_ACTION_READ_CLEAR:
		clear = true;
		/* FALL THRU */
	/* Read last kernel messages */
	case SYSLOG_ACTION_READ_ALL:
		if (!buf || len < 0)
			return -EINVAL;
		if (!len)
			return 0;
		if (!access_ok(VERIFY_WRITE, buf, len))
			return -EFAULT;
		error = syslog_print_all(buf, len, clear);
		break;
	/* Clear ring buffer */
	case SYSLOG_ACTION_CLEAR:
		syslog_clear();
		break;
	/* Number of chars in the log buffer */
	case SYSLOG_ACTION_SIZE_UNREAD:
		logbuf_lock_irq();
		if (syslog_seq < log_first_seq) {
			/* messages are gone, move to first one */
			syslog_seq = log_first_seq;
			syslog_idx = log_first_idx;
			syslog_partial = 0;
		}
		if (source == SYSLOG_FROM_PROC) {
			/*
			 * Short-cut for poll(/"proc/kmsg") which simply checks
			 * for pending data, not the size; return the count of
			 * records, not the length.
			 */
			error = log_next_seq - syslog_seq;
		} else {
			u64 seq = syslog_seq;
			u32 idx = syslog_idx;

			while (seq < log_next_seq) {
				struct ax_printk_log *msg = ax_log_from_idx(idx);

				error += ax_msg_print_text(msg, true, NULL, 0);
				idx = log_next(idx);
				seq++;
			}
			error -= syslog_partial;
		}
		logbuf_unlock_irq();
		break;
	/* Size of the log buffer */
	case SYSLOG_ACTION_SIZE_BUFFER:
		error = log_buf_len;
		break;
	default:
		error = -EINVAL;
		break;
	}

	return error;
}

SYSCALL_DEFINE3(axsyslog, int, type, char __user *, buf, int, len)
{
	return do_axsyslog(type, buf, len, SYSLOG_FROM_READER);
}

SYSCALL_DEFINE1(ax_local_clock, unsigned long long __user *, lclock)
{
	u64 tm = local_clock();

	if (copy_to_user(lclock, &tm, sizeof(unsigned long long)))
		return -EFAULT;

	return 0;
}

int ax_printk_delay_msec __read_mostly;

static inline void printk_delay(void)
{
	if (unlikely(ax_printk_delay_msec)) {
		int m = ax_printk_delay_msec;

		while (m--) {
			mdelay(1);
			touch_nmi_watchdog();
		}
	}
}

/*
 * Continuation lines are buffered, and not committed to the record buffer
 * until the line is complete, or a race forces it. The line fragments
 * though, are printed immediately to the consoles to ensure everything has
 * reached the console in case of a kernel crash.
 */
static struct cont {
	char buf[LOG_LINE_MAX];
	size_t len;			/* length == 0 means unused buffer */
	struct task_struct *owner;	/* task of first print*/
	u64 ts_nsec;			/* time of first print */
	u8 level;			/* log level of first message */
	u8 mid;				/* module id */
	enum log_flags flags;		/* prefix, newline flags */
	char    tag[32];
} cont;

static void cont_flush(void)
{
	if (cont.len == 0)
		return;

	log_store(cont.mid, cont.tag, cont.level, cont.flags, cont.ts_nsec,
		  NULL, 0, cont.buf, cont.len);
	cont.len = 0;
}

static bool cont_add(const int id, const char* tag, int level, enum log_flags flags, const char *text, size_t len)
{
	/*
	 * If ext consoles are present, flush and skip in-kernel
	 * continuation.  See nr_ext_console_drivers definition.  Also, if
	 * the line gets too long, split it up in separate records.
	 */
	if (cont.len + len > sizeof(cont.buf)) {
		cont_flush();
		return false;
	}

	if (!cont.len) {
		cont.mid = id;
		cont.level = level;
		cont.owner = current;
		cont.ts_nsec = local_clock();
		cont.flags = flags;
		if (tag)
			snprintf(cont.tag, sizeof(cont.tag), tag);
	}

	memcpy(cont.buf + cont.len, text, len);
	cont.len += len;

	// The original flags come from the first line,
	// but later continuations can add a newline.
	if (flags & LOG_NEWLINE) {
		cont.flags |= LOG_NEWLINE;
		cont_flush();
	}

	if (cont.len > (sizeof(cont.buf) * 80) / 100)
		cont_flush();

	return true;
}

static size_t log_output(const int id, const char* tag, int level, enum log_flags lflags, const char *dict, size_t dictlen, char *text, size_t text_len)
{
	/*
	 * If an earlier line was buffered, and we're a continuation
	 * write from the same process, try to add it to the buffer.
	 */
	if (cont.len) {
		if (cont.owner == current && (lflags & LOG_CONT)) {
			if (cont_add(id, tag, level, lflags, text, text_len))
				return text_len;
		}
		/* Otherwise, make sure it's flushed */
		cont_flush();
	}

	/* Skip empty continuation lines that couldn't be added - they just flush */
	if (!text_len && (lflags & LOG_CONT))
		return 0;

	/* If it doesn't end in a newline, try to buffer the current line */
	if (!(lflags & LOG_NEWLINE)) {
		if (cont_add(id, tag, level, lflags, text, text_len))
			return text_len;
	}

	/* Store it in the record log */
	return log_store(id, tag, level, lflags, 0, dict, dictlen, text, text_len);
}

/* Must be called under logbuf_lock. */
int ax_vprintk_store(const int id, const char* tag, int level,
		  const char *dict, size_t dictlen,
		  const char *fmt, va_list args)
{
	static char textbuf[LOG_LINE_MAX];
	char *text = textbuf;
	size_t text_len;
	enum log_flags lflags = 0;

	/*
	 * The printf needs to come first; we need the syslog
	 * prefix which might be passed-in as a parameter.
	 */
	text_len = vscnprintf(text, sizeof(textbuf), fmt, args);

	/* mark and strip a trailing newline */
	if (text_len && text[text_len-1] == '\n') {
		text_len--;
		lflags |= LOG_NEWLINE;
	}

	if(text_len <= 0)
		return 0;

	return log_output(id, tag, level, lflags,
			  NULL, 0, text, text_len);
}

asmlinkage int ax_vprintk_emit(const int id, const char* tag, int level,
			    const char *dict, size_t dictlen,
			    const char *fmt, va_list args)
{
	int printed_len;
	bool in_sched = false, pending_output;
	unsigned long flags;
	u64 curr_log_seq;

	if (level == LOGLEVEL_SCHED) {
		level = LOGLEVEL_DEFAULT;
		in_sched = true;
	}

	boot_delay_msec(level);
	printk_delay();

	/* This stops the holder of console_sem just where we want him */
	logbuf_lock_irqsave(flags);
	curr_log_seq = log_next_seq;
	printed_len = ax_vprintk_store(id, tag, level, dict, dictlen, fmt, args);
	pending_output = (curr_log_seq != log_next_seq);
	logbuf_unlock_irqrestore(flags);

	if (pending_output)
		wake_up_axklogd();
	return printed_len;
}
EXPORT_SYMBOL(ax_vprintk_emit);

asmlinkage int ax_vprintk(const int id, const char* tag, const int level, const char *fmt, va_list args)
{
	return ax_vprintk_func(id, tag, level, fmt, args);
}
EXPORT_SYMBOL(ax_vprintk);

asmlinkage int ax_printk_emit(const int id, const char* tag, int level,
			   const char *dict, size_t dictlen,
			   const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = ax_vprintk_emit(id, tag, level, dict, dictlen, fmt, args);
	va_end(args);

	return r;
}
EXPORT_SYMBOL(ax_printk_emit);

int ax_vprintk_default(const int id, const char* tag, const int level, const char *fmt, va_list args)
{
	int r;

#ifdef CONFIG_KGDB_KDB
	/* Allow to pass printk() to kdb but avoid a recursion. */
	if (unlikely(kdb_trap_printk && kdb_printf_cpu < 0)) {
		r = vkdb_printf(KDB_MSGSRC_PRINTK, fmt, args);
		return r;
	}
#endif
	r = ax_vprintk_emit(id, tag, level, NULL, 0, fmt, args);

	return r;
}
EXPORT_SYMBOL_GPL(ax_vprintk_default);

/**
 * printk - print a kernel message
 * @fmt: format string
 *
 * This is printk(). It can be called from any context. We want it to work.
 *
 * We try to grab the console_lock. If we succeed, it's easy - we log the
 * output and call the console drivers.  If we fail to get the semaphore, we
 * place the output into the log buffer and return. The current holder of
 * the console_sem will notice the new output in console_unlock(); and will
 * send it to the consoles before releasing the lock.
 *
 * One effect of this deferred printing is that code which calls printk() and
 * then changes console_loglevel may break. This is because console_loglevel
 * is inspected when the actual printing occurs.
 *
 * See also:
 * printf(3)
 *
 * See the vsnprintf() documentation for format string extensions over C99.
 */
asmlinkage __visible int ax_printk(const int id, const char* tag, const int level, const char *fmt, ...)
{
	va_list args;
	int r;
	axlog_lvl_info_ptr loglvlptr = NULL;
	unsigned char max_id;
	unsigned char klog_state;

	if (s_axlogctl_cfg_ptr != NULL){
		max_id = s_axlogctl_cfg_ptr->max_id;
		if (id < 0 || id >= max_id) return 0;
		klog_state = s_axlogctl_cfg_ptr->klog_state;
		loglvlptr = (axlog_lvl_info_ptr)s_axlogctl_cfg_ptr->klvladdr;
		if(!klog_state || level > loglvlptr[id].klevel)
			return 0;
	}

	va_start(args, fmt);
	r = ax_vprintk_func(id, tag, level, fmt, args);
	va_end(args);

	return r;
}
EXPORT_SYMBOL(ax_printk);
#endif /* CONFIG_PRINTK */

#if defined CONFIG_PRINTK
/*
 * Delayed printk version, for scheduler-internal messages:
 */
#define PRINTK_PENDING_WAKEUP	0x01
#define PRINTK_PENDING_OUTPUT	0x02

static DEFINE_PER_CPU(int, ax_printk_pending);

static void wake_up_klogd_work_func(struct irq_work *irq_work)
{
	int pending = __this_cpu_xchg(ax_printk_pending, 0);
	if (pending & PRINTK_PENDING_WAKEUP)
		wake_up_interruptible(&ax_log_wait);
}

static DEFINE_PER_CPU(struct irq_work, wake_up_axklogd_work) = {
	.func = wake_up_klogd_work_func,
	.flags = IRQ_WORK_LAZY,
};

void wake_up_axklogd(void)
{
	preempt_disable();
	if (waitqueue_active(&ax_log_wait)) {
		this_cpu_or(ax_printk_pending, PRINTK_PENDING_WAKEUP);
		irq_work_queue(this_cpu_ptr(&wake_up_axklogd_work));
	}
	preempt_enable();
}

void ax_defer_console_output(void)
{
	preempt_disable();
	__this_cpu_or(ax_printk_pending, PRINTK_PENDING_OUTPUT);
	irq_work_queue(this_cpu_ptr(&wake_up_axklogd_work));
	preempt_enable();
}

int ax_vprintk_deferred(const int id, const char* tag, const int level, const char *fmt, va_list args)
{
	int r;

	r = ax_vprintk_emit(id, tag, level, NULL, 0, fmt, args);
	ax_defer_console_output();

	return r;
}

int ax_printk_deferred(const int id, const char* tag, const int level,const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
	r = ax_vprintk_deferred(id, tag, level, fmt, args);
	va_end(args);

	return r;
}

void ax_set_kloglvl_info(unsigned long viraddr)
{
	s_axlogctl_cfg_ptr = (axlogctl_mem_cfg_ptr) viraddr;
}
EXPORT_SYMBOL(ax_set_kloglvl_info);

#endif

