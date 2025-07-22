#include <linux/kernel.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/tty.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/kthread.h>
#include <linux/syscore_ops.h>
#include <linux/platform_device.h>
#include <linux/seq_file.h>
#include <asm/mmu_context.h>
#include <jz_proc.h>

static struct task_struct *trace_get_task_struct(pid_t pid)
{
	struct task_struct *child;

	rcu_read_lock();
	child = find_task_by_vpid(pid);
	if (child)
		get_task_struct(child);
	rcu_read_unlock();

	if (!child)
		return ERR_PTR(-ESRCH);
	return child;
}
char g_buffer[256];

static int watch_proc_show(struct seq_file *m, void *v)
{
	int len = 0;
	len = seq_printf(m, "%s\n",g_buffer);

	return len;
}

struct watch_struct
{
	struct task_struct *child;
	unsigned int watchhi;
	unsigned int watchlo;
};

static void local_set_watch(void *args) {
	struct watch_struct *watch = (struct watch_struct *)args;
	unsigned int cpu = smp_processor_id();
	unsigned int hi = 0;
	struct mips3264_watch_reg_state *watches =
		&watch->child->thread.watch.mips3264;
	if(watch->watchhi & 0x40000000)
		hi = watch->watchhi;
	else
		hi = (watch->watchhi & 0xffff) | (cpu_asid(cpu,watch->child->mm) << 16);

	if(watch->watchlo == 0)
		hi = 0;

	watches->watchlo[0] = watch->watchlo;
	watches->watchhi[0] = hi;
	write_c0_watchlo0(watch->watchlo);
	write_c0_watchhi0(hi);

}

static int watch_write_proc(struct file *file, const char __user *buffer,
			    size_t count, loff_t *data)
{
	char s[20];
	char *s1,*s2;
	pid_t pid;
	unsigned int hi;
	unsigned int lo;
	struct task_struct *child = NULL;
	struct watch_struct watch;
	struct mips3264_watch_reg_state *watches;
	copy_from_user(g_buffer,buffer,count);
	g_buffer[count] = 0;
	printk("%s\n",g_buffer);
	s1 = g_buffer;
	s2 = strchr(s1,':');
	printk("s1 = %p s2 = %p\n",s1,s2);
	if(!s2 || count == 0) {
		pid = simple_strtoul(s1,0,0);
		child = trace_get_task_struct(pid);
		watch.child = child;
		watch.watchlo = 0;
		watch.watchhi = 0;
		local_set_watch(&watch);
		watches = &child->thread.watch.mips3264;
		clear_tsk_thread_flag(child, TIF_LOAD_WATCH);
	}
	memcpy(s,s1,s2 - s1);
	s[s2 - s1]  = 0;
	pid = simple_strtoul(s,0,0);

	s1 = s2 + 1;
	s2 = strchr(s1,':');
	printk("s1 = %p s2 = %p\n",s1,s2);
	memcpy(s,s1,s2 - s1);
	s[s2 - s1]  = 0;
	lo = simple_strtoul(s,0,0);

	s1 = s2 + 1;
	hi = simple_strtoul(s1,0,0);
	printk("pid = %d lo = 0x%08x hi = %d\n",pid,lo,hi);
	child = trace_get_task_struct(pid);
	printk("child = %p\n",child);
	//child->thread.watch.mips3264.watchlo[0] = lo;
	//child->thread.watch.mips3264.watchhi[0] = hi;
	watch.child = child;
	watch.watchlo = lo;
	watch.watchhi = hi;
	local_set_watch(&watch);
	watches = &child->thread.watch.mips3264;

	return count;
}

static int watch_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, watch_proc_show, PDE_DATA(inode));
}

static const struct file_operations watch_proc_fops ={
	.read = seq_read,
	.open = watch_proc_open,
	.write = watch_write_proc,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init jz_trace_proc(void)
{
	struct proc_dir_entry *p;

	p = jz_proc_mkdir("debug");
	if (!p) {
		pr_warning("create_proc_entry for debug failed.\n");
		return -ENODEV;
	}

        proc_create_data("watch", 0444, p, &watch_proc_fops, NULL);

	return 0;
}
module_init(jz_trace_proc);
