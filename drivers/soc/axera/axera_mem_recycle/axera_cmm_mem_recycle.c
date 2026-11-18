#include <linux/module.h>
#include <linux/sched.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

#define CMMREC_PROC_NAME "ax_proc/cmm_reserved"

static phys_addr_t s_cmm_addr_start = 0;
static phys_addr_t s_cmm_addr_end = 0;
static int s_cmm_freed = 0;

int ax_in_cmm_mem_range(const phys_addr_t paddr)
{
	if (s_cmm_freed)
		return 0;

	if (paddr >= s_cmm_addr_start && paddr <= s_cmm_addr_end)
		return 1;

	return 0;
}
EXPORT_SYMBOL(ax_in_cmm_mem_range);

static void ax_free_cmm_mem(void)
{
	unsigned long ret;
	void *vir_start;

	if (s_cmm_freed) {
		pr_warn("cmm memory already freed!");
		return;
	}

	s_cmm_freed = 1;
	vir_start = phys_to_virt(s_cmm_addr_start);
	ret = free_reserved_area(vir_start, vir_start + (s_cmm_addr_end - s_cmm_addr_start), 0xffff, "cmm");
	pr_info("cmm memory freed pages: %ld!!!\n", ret);

	return;
}

static int __init ax_cmm_mem_init(void)
{
	struct device_node *of_node;
	unsigned int addrs[4];
	int len;

	of_node = of_find_compatible_node(NULL, NULL, "axera, cmm_rec");
	if (!of_node) {
		pr_err("%s:can't find memory_dump tree node\n", __FUNCTION__);
		return -ENODEV;
	}
	pr_info("%s: of_find_compatible_node 'axera, cmm_rec' success!!!", __FUNCTION__);

	len = of_property_read_variable_u32_array(of_node, "reg", addrs, 4, ARRAY_SIZE(addrs));
	pr_debug("cmm: addr[0]:0x%x,addr[1]:0x%x,addr[2]:0x%x,addr[3]:0x%x", addrs[0], addrs[1], addrs[2], addrs[3]);
	if (len == 4){
		s_cmm_addr_start = addrs[1];
		s_cmm_addr_end   = addrs[1] + addrs[3];
	}else{
		pr_err("%s: invalid cmm address!", __FUNCTION__);
	}

	return 0;
}

static int cmmrec_proc_show(struct seq_file *p, void *v)
{
	seq_printf(p, "************* Axera cmm reserved memory info ************\n");
	seq_printf(p, "cmm memory  freed: %s\n", s_cmm_freed==0?"no":"yes");
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	seq_printf(p, "cmm start address: 0x%llx\n", s_cmm_addr_start);
	seq_printf(p, "cmm end   address: 0x%llx\n", s_cmm_addr_end);
#else
	seq_printf(p, "cmm start address: 0x%x\n", s_cmm_addr_start);
	seq_printf(p, "cmm end   address: 0x%x\n", s_cmm_addr_end);
#endif
	return 0;
}

static ssize_t cmmrec_proc_write(struct file *file, const char *buffer, size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count + 1), GFP_KERNEL);
	char cmd[64];

	if (!tmp)
		return -ENOMEM;

	if (copy_from_user(tmp, buffer, count)) {
		pr_err("@%s copy_from_user failed!!!\n", __FUNCTION__);
		kfree(tmp);
		return -EFAULT;
	}
	if (sscanf(tmp, "%s", cmd) == 1) {
		if(0 == strcmp(cmd, "freecmm")){
			pr_info("freeing cmm memory...");
			ax_free_cmm_mem();
		}
	}

	kfree(tmp);
	tmp = NULL;

	return count;
}

static int cmmrec_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, cmmrec_proc_show, NULL);
}

static const struct file_operations proc_cmmrec_operations = {
	.open  = cmmrec_proc_open,
	.read  = seq_read,
	.write = cmmrec_proc_write,
	.release = single_release,
};

static int __init cmmrec_proc_init(void)
{
	proc_create(CMMREC_PROC_NAME, S_IRUSR, NULL, &proc_cmmrec_operations);
	return 0;
}

early_initcall(ax_cmm_mem_init);
module_init(cmmrec_proc_init);
