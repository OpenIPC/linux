#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/string.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <asm/uaccess.h>
#include <linux/of_platform.h>
#include <linux/miscdevice.h>
#include <linux/poll.h>
#include <linux/ioctl.h>
#include <linux/list.h>
#include <linux/rbtree.h>
#include <linux/miscdevice.h>
#include <linux/timer.h>
#include <linux/freezer.h>
#include <linux/delay.h>
#include <linux/math64.h>
#include <linux/time64.h>
#include <uapi/linux/time.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>

#include "ax_os_mem.h"
#define DEVICE_NAME "ax_os_mem"
#define MODEL_DTS_NODE_MODEL "/reserved-memory/model_data_memreserved"
#define AX_OS_MEM_ROOT_NAME "ax_proc/os_mem"
#define AX_OS_MEM_STAT_NAME "ax_proc/os_mem/stat"
#define AX_OS_MEM_POLICY_NAME "ax_proc/os_mem/policy"
#define AX_OS_MEM_MAX_DISP_NAME "ax_proc/os_mem/max_display"
#define AX_OS_MEM_CTRL_NAME "ax_proc/os_mem/ctrl"
#define AX_OS_MOD_PROC_ALL 255
#define AX_OS_MEM_POLICY_CONFIGED (1 << 31)
#define AX_OS_DEFAULT_MAX_DISPC_NUM 512
#define AX_OS_MEM_GET_MOD_ID(id) (id & 0x000000ff)
#define AX_OS_MEM_HEADER_MAGIC                    (0x4F534D4D)
#define AX_OS_MEM_ID_MAX  ARRAY_SIZE(module_name)
#define AX_OSAL_GFP_ATOMIC  0
#define SEC_SIGN_HEADER_SIZE  0x400

char *module_name[] =
{
	"COMMON",
	"ISP",
	"CE",
	"VO",
	"VDSP",
	"EFUSE",
	"NPU",
	"VENC",
	"VDEC",
	"JENC",
	"JDEC",
	"SYS",
	"AENC",
	"IVPS",
	"MIPI",
	"ADEC",
	"DMA",
	"VIN",
	"USER",
	"IVES",
	"SKEL",
	"IVE",
	"RESERV",
	"RESERV",
	"RESERV",
	"RESERV",
	"AUDIO",
	"ALGO",
	"ENGINE",
	"RESERV",
	"RESERV",
	"RESERV",
	"AI",
	"AO",
	"SENSOR",
	"NT",
	"RESERV",
	"RESERV",
	"RESERV",
	"RESERV",
	"BASE",
	"THM",
	"3A-AE",
	"3A-AWB",
	"3A-AF",
	"RESERV",
};

typedef struct {
    u64 PhysAddr;
    u64 Size;
} AX_MODEL_INFO_T;

typedef struct {
	unsigned long addr;
	struct rb_node node;
	unsigned int size;
	unsigned int id;
} mem_node_t;

typedef struct {
	mem_proc_data_header_t proc_header;
	mem_proc_data_t proc_data[AX_OS_MEM_ID_MAX];
} shared_proc_data;

typedef struct {
	shared_proc_data *shared_data;
	struct list_head list_node;
	struct file *filp;
	unsigned long long share_data_phy;
} os_mem_proc_file;

typedef struct {
	struct rb_root rb_root_node;
	unsigned int total_size;
	unsigned int alloc_times;
	unsigned int max_total_size;
	unsigned int max_times;
	spinlock_t lock;
} os_mem_stat_t;

typedef struct {
	unsigned int id;
	unsigned int size;
} mem_proc_node_t;

typedef struct {
	u32 user_max_times[AX_OS_MEM_ID_MAX];
	u32 user_max_size[AX_OS_MEM_ID_MAX];
	u32 user_size[AX_OS_MEM_ID_MAX];
	u32 user_times[AX_OS_MEM_ID_MAX];
	u32 max_display_num;
	u64 reserved;
	struct mutex glb_lock;
	OS_MEM_POLICY_E os_mem_stat_policy;
	u32 proc_mod_id;
} os_mem_glb_data_t;

enum {
	INSERT_KMALLOC_ATMOIC = (1 << 16),
	INSERT_VMALLOC = (1 << 17),
} INSERT_FLAG_E;

enum {
	OS_MEM_CTRL_CLR_USER_STAT = 1,
} OS_MEM_CTRL_CMD_E;

static int model_release = 0;
static os_mem_stat_t *os_mem_stat_array = 0;
struct proc_dir_entry *os_mem_root;
struct proc_dir_entry *stat_file;
struct proc_dir_entry *policy_file;
struct proc_dir_entry *max_dispc_file;
struct proc_dir_entry *ctrl_file;
static struct list_head ax_os_mem_file_list = \
	LIST_HEAD_INIT(ax_os_mem_file_list);

static os_mem_glb_data_t os_mem_stat_glb;
static void ax_mem_glb_lock_init(void)
{
	mutex_init(&os_mem_stat_glb.glb_lock);
}
static void ax_mem_glb_lock(void)
{
	mutex_lock(&os_mem_stat_glb.glb_lock);
}
static void ax_mem_glb_unlock(void)
{
	mutex_unlock(&os_mem_stat_glb.glb_lock);
}
static u32 ax_mem_policy_get(void)
{
	return os_mem_stat_glb.os_mem_stat_policy & (~AX_OS_MEM_POLICY_CONFIGED);
}
static u32 ax_mem_policy_config_check(void)
{
	if(os_mem_stat_glb.os_mem_stat_policy & AX_OS_MEM_POLICY_CONFIGED)
		return 1;
	else
		return 0;
}
static u32 ax_mem_get_proc_mod_id(void)
{
	return os_mem_stat_glb.proc_mod_id;
}
static u32 ax_mem_set_proc_mod_id(u32 id)
{
	return os_mem_stat_glb.proc_mod_id = id;
}
static u32 ax_mem_get_max_dispc_num(void)
{
	return os_mem_stat_glb.max_display_num;
}
static u32 ax_mem_set_max_dispc_num(u32 num)
{
	return os_mem_stat_glb.max_display_num = num;
}
static int ax_mem_policy_set(int policy)
{
	if(ax_mem_policy_config_check())
		return 0;

	os_mem_stat_glb.os_mem_stat_policy = AX_OS_MEM_POLICY_CONFIGED | policy;
	return 0;
}
static int ax_mem_get_usr_policy(void)
{
	int policy;
	policy = ax_mem_policy_get();
	switch(policy) {
	case AX_OS_MEM_POLICY_1:
	case AX_OS_MEM_POLICY_2:
		policy = AX_OS_MEM_POLICY_1;
		break;
	default:
		policy = AX_OS_MEM_POLICY_0;
		break;
	}
	return policy;
}
static void ax_mem_usr_stat_update(void)
{
	struct list_head *itr;
	os_mem_proc_file *proc_file;
	mem_proc_data_t *proc_data;
	int i;
	void *ptr;
	u32* user_max_times;
	u32* user_max_size;
	u32* user_cur_size;
	u32* user_cur_times;
	ptr = kzalloc(AX_OS_MEM_ID_MAX * sizeof(u32) * 4, GFP_KERNEL);
	if(!ptr) {
		pr_err("%s malloc failed!\n", __func__);
		return;
	}
	user_max_times = ptr;
	user_max_size = (u32 *)((void *)user_max_times + AX_OS_MEM_ID_MAX * sizeof(u32));
	user_cur_size = (u32 *)((void *)user_max_size + AX_OS_MEM_ID_MAX * sizeof(u32));
	user_cur_times = (u32 *)((void *)user_cur_size + AX_OS_MEM_ID_MAX * sizeof(u32));
	ax_mem_glb_lock();
	list_for_each(itr, &ax_os_mem_file_list) {
		proc_file = list_entry(itr, os_mem_proc_file, list_node);
		if(proc_file) {
			proc_data = proc_file->shared_data->proc_data;
			for(i = 0 ;i  < AX_OS_MEM_ID_MAX; i++) {
				user_max_times[i] += proc_data[i].usr_max_times;
				user_cur_times[i] += proc_data[i].usr_times;
				if((ax_mem_policy_get() == AX_OS_MEM_POLICY_1) || \
					(ax_mem_policy_get() == AX_OS_MEM_POLICY_2)) {
					user_max_size[i] += proc_data[i].usr_max_total_size;
					user_cur_size[i] += proc_data[i].usr_size;
				}
			}
		}
	}
	if((ax_mem_policy_get() == AX_OS_MEM_POLICY_0)) {
		for(i = 0 ;i  < AX_OS_MEM_ID_MAX; i++) {
			os_mem_stat_glb.user_max_times[i] = user_max_times[i];
			os_mem_stat_glb.user_times[i] = user_cur_times[i];
			os_mem_stat_glb.user_max_size[i] = 0;
			os_mem_stat_glb.user_size[i] = 0;
		}
	}
	if(ax_mem_policy_get() == AX_OS_MEM_POLICY_1) {
		for(i = 0 ;i  < AX_OS_MEM_ID_MAX; i++) {
			os_mem_stat_glb.user_max_times[i] = user_max_times[i];
			os_mem_stat_glb.user_times[i] = user_cur_times[i];
			os_mem_stat_glb.user_max_size[i] = user_max_size[i];
			os_mem_stat_glb.user_size[i] = user_cur_size[i];
		}
	}
	if(ax_mem_policy_get() == AX_OS_MEM_POLICY_2) {
		for(i = 0; i  < AX_OS_MEM_ID_MAX; i++) {
			if(user_max_times[i] > os_mem_stat_glb.user_max_times[i])
				os_mem_stat_glb.user_max_times[i] = user_max_times[i];
			if(user_max_size[i] > os_mem_stat_glb.user_max_size[i])
				os_mem_stat_glb.user_max_size[i] = user_max_size[i];
			os_mem_stat_glb.user_size[i] = user_cur_size[i];
			os_mem_stat_glb.user_times[i] = user_cur_times[i];
		}
	}
	ax_mem_glb_unlock();
	kfree(ptr);
}
static void ax_mem_clr_glb_user_stat(void)
{
	ax_mem_glb_lock();
	memset(os_mem_stat_glb.user_max_times, 0, sizeof(os_mem_stat_glb.user_max_times));
	memset(os_mem_stat_glb.user_times, 0, sizeof(os_mem_stat_glb.user_times));
	memset(os_mem_stat_glb.user_max_size, 0, sizeof(os_mem_stat_glb.user_max_size));
	memset(os_mem_stat_glb.user_size, 0, sizeof(os_mem_stat_glb.user_size));
	ax_mem_glb_unlock();
};
static int ax_mem_node_insert(struct rb_root *root, mem_node_t *mem_node)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;
	mem_node_t *this;
	while (*new) {
		this = container_of(*new, mem_node_t, node);
		parent = *new;
		if (this->addr > mem_node->addr)
			new = &((*new)->rb_left);
		else if (this->addr < mem_node->addr)
			new = &((*new)->rb_right);
		else
			return -1;
	}
	rb_link_node(&mem_node->node, parent, new);
	rb_insert_color(&mem_node->node, root);
	return 0;
}

static mem_node_t *ax_os_mem_node_search(struct rb_root *root, unsigned long addr)
{
	struct rb_node **new = &(root->rb_node);
	mem_node_t *this;
	while (*new) {
		this = container_of(*new, mem_node_t, node);
		if (this->addr > addr)
			new = &((*new)->rb_left);
		else if (this->addr < addr)
			new = &((*new)->rb_right);
		else
			return this;
	}
	return NULL;
}

static int ax_os_mem_node_insert(int id, void *ptr, u32 size, u32 aligned)
{
	os_mem_stat_t *os_mem;
	mem_node_t *mem_node;
	unsigned long flags;
	unsigned int mod_id;
	mod_id = AX_OS_MEM_GET_MOD_ID(id);
	os_mem = &os_mem_stat_array[mod_id];
	if(ax_mem_policy_get() == AX_OS_MEM_POLICY_0) {
		spin_lock_irqsave(&os_mem->lock, flags);
		os_mem->alloc_times ++;
		if(os_mem->max_times < os_mem->alloc_times) {
			os_mem->max_times = os_mem->alloc_times;
		}
		spin_unlock_irqrestore(&os_mem->lock, flags);
		return 0;
	}
	if(id & INSERT_KMALLOC_ATMOIC) {
		mem_node = kmalloc(sizeof(mem_node_t), GFP_ATOMIC);
	}
	else {
		mem_node = kmalloc(sizeof(mem_node_t), GFP_KERNEL);
	}
	if(!mem_node) {
		return -1;
	}
	mem_node->addr = (unsigned long)ptr;
	size = (size + aligned - 1) & (~ (aligned - 1));
	mem_node->size = size;
	mem_node->id = id;
	spin_lock_irqsave(&os_mem->lock, flags);

	ax_mem_node_insert(&os_mem->rb_root_node, mem_node);
	os_mem->total_size += size;
	os_mem->alloc_times ++;
	if(os_mem->max_total_size < os_mem->total_size) {
		os_mem->max_total_size = os_mem->total_size;
	}
	if(os_mem->max_times < os_mem->alloc_times) {
		os_mem->max_times = os_mem->alloc_times;
	}
	spin_unlock_irqrestore(&os_mem->lock, flags);
	return 0;
}

static int ax_node_remove_from_mod(int mod_id, const void *ptr)
{
	os_mem_stat_t *os_mem;
	mem_node_t *mem_node;
	ulong flags;
	os_mem = &os_mem_stat_array[mod_id];
	if(ax_mem_policy_get() == AX_OS_MEM_POLICY_0) {
		spin_lock_irqsave(&os_mem->lock, flags);
		os_mem->alloc_times --;
		spin_unlock_irqrestore(&os_mem->lock, flags);
		return 0;
	}
	spin_lock_irqsave(&os_mem->lock, flags);
	mem_node = ax_os_mem_node_search(&os_mem->rb_root_node, (ulong)ptr);
	if(!mem_node) {
		spin_unlock_irqrestore(&os_mem->lock, flags);
		return -1;
	}
	rb_erase(&mem_node->node, &os_mem->rb_root_node);
	os_mem->total_size -= mem_node->size;
	os_mem->alloc_times --;
	spin_unlock_irqrestore(&os_mem->lock, flags);

	kfree(mem_node);
	return 0;
}

static int ax_node_remove(int mod_id, const void *ptr)
{
	int ret;
	int i;
	if(!ptr) {
		pr_warn("ax_node_remove: mod_id: %d, free address is zero\n", mod_id);
		return 0;
	}
	ret = ax_node_remove_from_mod(mod_id, ptr);
	if(ret == 0) {
		return 0;
	}
	for(i = 0; i < AX_OS_MEM_ID_MAX; i ++) {
		if(mod_id == i)
			continue;
		ret = ax_node_remove_from_mod(i, ptr);
		if(ret == 0)
			break;
	}
	return 0;
}
static u32 _get_aligned_size(size_t size)
{
	u32 aligned = 128;
	if(size <= ARCH_DMA_MINALIGN)
		return ARCH_DMA_MINALIGN;
	if(size > (1UL << KMALLOC_SHIFT_HIGH)) {
		aligned = PAGE_SIZE;
		return aligned;
	}
	if (size <=          8) return (1 << 3);
	if (size <=         16) return (1 << 4);
	if (size <=         32) return (1 << 5);
	if (size <=         64) return (1 << 6);
	if (size <=        128) return (1 << 7);
	if (size <=        256) return (1 << 8);
	if (size <=        512) return (1 << 9);
	if (size <=       1024) return (1 << 10);
	if (size <=   2 * 1024) return (1 << 11);
	if (size <=   4 * 1024) return (1 << 12);
	if (size <=   8 * 1024) return (1 << 13);
	return aligned;
}
static u32 get_aligned_size(size_t size)
{
	u32 ret;
	ret = _get_aligned_size(size);
	return ret;
}
void *ax_os_mem_kmalloc(int id, size_t size, u32 flag)
{
	void *addr;
	u32 mod_id;
	mod_id = AX_OS_MEM_GET_MOD_ID(id);
	if(mod_id >= AX_OS_MEM_ID_MAX) {
		return 0;
	}
	if (flag == AX_OSAL_GFP_ATOMIC) {
		addr = kmalloc(size, GFP_ATOMIC);
		id |= INSERT_KMALLOC_ATMOIC;
	}
	else {
		addr = kmalloc(size, GFP_KERNEL);
	}
	if(addr) {
		ax_os_mem_node_insert(id, addr, size, get_aligned_size(size));
	}
	return addr;
}
EXPORT_SYMBOL(ax_os_mem_kmalloc);

void *ax_os_mem_kzalloc(int id, size_t size, u32 flag)
{
	void *addr;
	u32 mod_id;
	mod_id = AX_OS_MEM_GET_MOD_ID(id);
	if(mod_id >= AX_OS_MEM_ID_MAX) {
		return 0;
	}
	if (flag == AX_OSAL_GFP_ATOMIC) {
		addr = kzalloc(size, GFP_ATOMIC);
		id |= INSERT_KMALLOC_ATMOIC;
	}
	else {
		addr = kzalloc(size, GFP_KERNEL);
	}

	if(addr) {
		ax_os_mem_node_insert(id, addr, size, get_aligned_size(size));
	}
	return addr;
}
EXPORT_SYMBOL(ax_os_mem_kzalloc);
void ax_os_mem_kfree(int id, const void *addr)
{
	u32 mod_id;
	mod_id = AX_OS_MEM_GET_MOD_ID(id);
	if(mod_id >= AX_OS_MEM_ID_MAX) {
		return;
	}
	ax_node_remove(mod_id, addr);
	kfree(addr);
}
EXPORT_SYMBOL(ax_os_mem_kfree);
void *ax_os_mem_vmalloc(int id, size_t size)
{
	void *addr;
	u32 mod_id;
	mod_id = AX_OS_MEM_GET_MOD_ID(id);
	if(mod_id >= AX_OS_MEM_ID_MAX) {
		return 0;
	}
	addr = vmalloc(size);
	if(addr) {
		ax_os_mem_node_insert(id | INSERT_VMALLOC, addr, size, PAGE_SIZE);
	}
	return addr;
}
EXPORT_SYMBOL(ax_os_mem_vmalloc);
void ax_os_mem_vfree(int id, const void *addr)
{
	u32 mod_id;
	mod_id = AX_OS_MEM_GET_MOD_ID(id);
	if(mod_id >= AX_OS_MEM_ID_MAX) {
		return;
	}
	ax_node_remove(mod_id, addr);
	vfree(addr);
}
EXPORT_SYMBOL(ax_os_mem_vfree);

s32 ax_os_release_reserved_mem(unsigned long phy_start, size_t size, char *s)
{
	void *vir_start = phys_to_virt(phy_start);
	void *vir_end = vir_start + size;
	free_reserved_area(vir_start, vir_end, POISON_FREE_INITMEM, s);
	return 0;
}
EXPORT_SYMBOL_GPL(ax_os_release_reserved_mem);

static s32 ax_os_mem_open(struct inode *inode, struct file *filp)
{
	os_mem_proc_file *proc_file;
	mem_proc_data_header_t *proc_header;
	shared_proc_data *shared_data;
	shared_data = (shared_proc_data *)get_zeroed_page(GFP_KERNEL | GFP_DMA);
	if (!shared_data)
		return -ENOMEM;
	proc_file = (void *)kzalloc(sizeof(os_mem_proc_file), GFP_KERNEL);
	if(!proc_file) {
		kfree(shared_data);
		return -ENOMEM;
	}
	proc_file->share_data_phy = virt_to_phys((void*)shared_data);
	proc_file->shared_data = shared_data;
	ax_mem_glb_lock();
	ax_mem_policy_set(ax_mem_policy_get());
	proc_header = &shared_data->proc_header;
	proc_header->mod_num = AX_OS_MEM_ID_MAX;
	proc_header->magic = AX_OS_MEM_HEADER_MAGIC;
	proc_header->policy = ax_mem_get_usr_policy();

	INIT_LIST_HEAD(&proc_file->list_node);
	list_add_tail(&proc_file->list_node, &ax_os_mem_file_list);
	filp->private_data = proc_file;
	ax_mem_glb_unlock();
	return 0;
}

int ax_model_utils_get_dts_reg(u64 *addr, u64 *size)
{
    struct device_node *dts_node;

    if (addr == NULL || size == NULL) {
        pr_err("params error\n");
        return -1;
    }

    dts_node = of_find_node_by_path(MODEL_DTS_NODE_MODEL);
    if (!dts_node) {
        return -2;
    }
    if (of_property_read_u64(dts_node, "reg", addr) ||
        of_property_read_u64_index(dts_node, "reg", 1, size)) {
        return -1;
    }

    return 0;
}

int ax_model_release_mem(void)
{
	int ret;
	u64 addr, size;

	if (model_release)
		return 0;

	ret = ax_model_utils_get_dts_reg(&addr, &size);
	if (ret < 0) {
		if (ret == -2) {
			pr_err("No model partition\n");
			return ret;
		}
		pr_err("get dts reg error add = %llx, size = %llx\n", addr, size);
		return ret;
	}

	ret = ax_os_release_reserved_mem(addr, size, "models area");
	if (ret != 0) {
	    pr_err("release models reserved memory fail\n");
	    return ret;
	}
	model_release = 1;

	return 0;
}

int get_models_info(AX_MODEL_INFO_T *ModelMsg)
{
	int ret;
	u64 addr, size;

	if (model_release) {
		pr_err("The model partition has been released.\n");
		return -2;
	}

	ret = ax_model_utils_get_dts_reg(&addr, &size);
	if (ret < 0) {
		return ret;
	}
	ModelMsg->PhysAddr = addr + SEC_SIGN_HEADER_SIZE;
	ModelMsg->Size = size - SEC_SIGN_HEADER_SIZE;
    return 0;
}

static s32 ax_os_mem_release(struct inode *inode, struct file *filp)
{
	os_mem_proc_file *proc_file = filp->private_data;
	/*only policy 2 Statistic history process*/
	if(ax_mem_policy_get() == AX_OS_MEM_POLICY_2) {
		ax_mem_usr_stat_update();
	}
	ax_mem_glb_lock();
	list_del(&proc_file->list_node);
	filp->private_data = NULL;
	ax_mem_glb_unlock();
	free_page((unsigned long)proc_file->shared_data);
	kfree(proc_file);
	return 0;
}

static long ax_os_mem_ioctl(struct file *filp, u32 cmd, unsigned long arg)
{
	void __user *argp = (void __user *)arg;
	int ret;
	AX_MODEL_INFO_T ModelMsg;
	os_mem_proc_file *proc_file = filp->private_data;
	if(!proc_file) {
		return -1;
	}
	switch (cmd) {
	case AX_OS_MEM_GET_SHARD_DATA_ADDR:
		if (copy_to_user(argp, &proc_file->share_data_phy, sizeof(proc_file->share_data_phy)))
			return -EFAULT;
		break;
	case AX_OS_MEM_RES_MODEL_IMG_MEM:
		if(ax_model_release_mem())
			return -EFAULT;
		break;
	case AX_OS_MEM_GET_MODEL_IMG_INFO:
		ret = get_models_info(&ModelMsg);
		if (ret < 0) {
			return ret;
		} else {
			if (copy_to_user(argp, (void *)&ModelMsg, sizeof(AX_MODEL_INFO_T)))
				return -EFAULT;
		}
		break;
	default:
		break;
	}
	return 0;
}
static const struct file_operations ax_os_mem_fops = {
	.owner = THIS_MODULE,
	.open = ax_os_mem_open,
	.release = ax_os_mem_release,
	.unlocked_ioctl = ax_os_mem_ioctl,
};

static struct miscdevice ax_os_mem_miscdev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_NAME,
	.fops = &ax_os_mem_fops
};
static s32 ax_os_module_stat_show(int mod_id, struct seq_file *m, void *v)
{
	mem_proc_node_t *node;
	unsigned int num;
	int i;
	u8 tmp_buffer[256];
	os_mem_stat_t *os_mem;
	struct rb_node *n;
	mem_node_t *this;
	unsigned long flags = 0;
	mem_proc_node_t *node_ptr;

	sprintf(tmp_buffer, "(%s)  mod_id: %2d, memory(kernel) stat\n", module_name[mod_id], mod_id);
	seq_printf(m, tmp_buffer);
	sprintf(tmp_buffer, "---------------------------------------------\n");
	seq_printf(m, tmp_buffer);
	sprintf(tmp_buffer, "%8s  %8s\n", "id", "size");
	seq_printf(m, tmp_buffer);
	if(ax_mem_policy_get() == AX_OS_MEM_POLICY_0)
		return 0;
	os_mem = &os_mem_stat_array[mod_id];
	num = min(ax_mem_get_max_dispc_num(), os_mem->alloc_times);
	node_ptr = kmalloc(sizeof(mem_proc_node_t) * num, GFP_KERNEL);
	if(!node_ptr)
		return -ENOMEM;

	spin_lock_irqsave(&os_mem->lock, flags);
	num = min(os_mem->alloc_times, num);
	n = rb_first(&os_mem->rb_root_node);
	i = 0;
	node = node_ptr;
	while(n && (i < num)) {
		this = container_of(n, mem_node_t, node);
		node->id = this->id;
		node->size = this->size;
		node++;
		i++;
		n = rb_next(n);
	}
	spin_unlock_irqrestore(&os_mem->lock, flags);
	node = node_ptr;
	num = i;
	for(i = 0; i < num; i++) {
		sprintf(tmp_buffer, "%8d    %10d\n", node->id, node->size);
		seq_printf(m, tmp_buffer);
		node++;
	}
	kfree(node_ptr);
	return 0;
}
#define STAT_ALL_HEAD_FMT "%8s %8s  %10s  %10s  %10s  %10s  %10s  %10s  %10s %10s\n", \
			"module", "mod_id", "usr_size", "ker_size", "usr_times", "ker_times", \
			"max_ker_size", "max_ker_times", "max_usr_size", "max_usr_times"

#define STAT_ALL_CONTENT_FMT "%8s%8d %10d %10d %10d %10d   %10d    %10d    %10d   %10d\n"
#define STAT_ALL_TAIL_FMT "%s: %8dK(usr) %8dK(ker) %8d(usr_times) %8d(ker_times)\n", "All"
#define STAT_ALL_SPIT_FMT "-----------------------------------------------------------"

static s32 ax_os_all_stat_show(struct seq_file *m, void *v)
{
	int i;
	os_mem_stat_t *os_mem = os_mem_stat_array;
	u8 tmp_buffer[256];
	u64 ker_size = 0;
	u32 ker_times = 0;
	u64 usr_size = 0;
	u32 usr_times = 0;
	sprintf(tmp_buffer, STAT_ALL_HEAD_FMT);
	seq_printf(m, tmp_buffer);
	ax_mem_usr_stat_update();
	for(i = 0; i < AX_OS_MEM_ID_MAX; i++) {
		ker_size += os_mem[i].total_size;
		ker_times += os_mem[i].alloc_times;
		usr_size += os_mem_stat_glb.user_size[i];
		usr_times += os_mem_stat_glb.user_times[i];
		sprintf(tmp_buffer, STAT_ALL_CONTENT_FMT, \
				module_name[i], i, os_mem_stat_glb.user_size[i], \
				os_mem[i].total_size, os_mem_stat_glb.user_times[i], \
				os_mem[i].alloc_times, os_mem[i].max_total_size, \
				os_mem[i].max_times, os_mem_stat_glb.user_max_size[i], \
				os_mem_stat_glb.user_max_times[i]);
		seq_printf(m, tmp_buffer);
	}
	sprintf(tmp_buffer, "%s%s\n", STAT_ALL_SPIT_FMT, STAT_ALL_SPIT_FMT);
	seq_printf(m, tmp_buffer);
	sprintf(tmp_buffer, STAT_ALL_TAIL_FMT, (u32)(usr_size / 1024), \
		(u32)(ker_size / 1024), usr_times, ker_times);
	seq_printf(m, tmp_buffer);
	return 0;
}
static s32 ax_os_mem_stat_show(struct seq_file *m, void *v)
{
	if(ax_mem_get_proc_mod_id() == AX_OS_MOD_PROC_ALL)
		ax_os_all_stat_show(m, v);
	else if(ax_mem_get_proc_mod_id() < AX_OS_MEM_ID_MAX) {
		ax_os_module_stat_show(ax_mem_get_proc_mod_id(), m, v);
	}
	return 0;
}

static s32 ax_os_mem_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_os_mem_stat_show, NULL);
}
static ssize_t ax_os_mem_stat_write(struct file *file, const char *buffer, size_t count, loff_t *f_pos)
{
	char tmp[5] = {0};
	long mod_id;
	int ret;
	if((!count) || (count > 4))
		return -EINVAL;
	if (copy_from_user(tmp, buffer, count)) {
		pr_err("%s copy_from_user failed!!!\n", __FUNCTION__);
		return -EFAULT;
	}
	ret = kstrtoul(tmp, 10, &mod_id);
 	if (ret)
 		return ret;
	if((mod_id < AX_OS_MEM_ID_MAX) || (mod_id == AX_OS_MOD_PROC_ALL))
		ax_mem_set_proc_mod_id(mod_id);
	else {
		return -EINVAL;
	}
	return count;
}

static const struct file_operations os_mem_stat_ops = {
	.open = ax_os_mem_stat_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = ax_os_mem_stat_write,
};

static s32 ax_os_mem_policy_show(struct seq_file *m, void *v)
{
	seq_printf(m, "policy: %d\n", ax_mem_policy_get());
	return 0;
}

static s32 ax_os_mem_policy_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_os_mem_policy_show, NULL);
}
static ssize_t ax_os_mem_policy_write(struct file *file, const char *buffer, \
			size_t count, loff_t *f_pos)
{
	char tmp[3] = {0};
	long val;
	int ret;
	if((count > 2))
		return -EINVAL;
	if (copy_from_user(tmp, buffer, count)) {
		pr_err("%s copy_from_user failed!!!\n", __FUNCTION__);
		return -EFAULT;
	}
	ret = kstrtoul(tmp, 10, &val);
 	if (ret)
 		return ret;
	/*only support policy 0-AX_OS_MEM_POLICY_MAX*/
	if((val < AX_OS_MEM_POLICY_MAX) &&  \
		!((ax_mem_policy_get() & AX_OS_MEM_POLICY_CONFIGED))) {
		ax_mem_policy_set(val);
	}
	else {
		return -EINVAL;
	}
	return count;
}

static const struct file_operations os_mem_policy_ops = {
	.open = ax_os_mem_policy_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = ax_os_mem_policy_write,
};
static s32 ax_os_mem_max_disp_show(struct seq_file *m, void *v)
{
	seq_printf(m, "max_disp: %d\n", ax_mem_get_max_dispc_num());
	return 0;
}

static s32 ax_os_mem_max_disp_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_os_mem_max_disp_show, NULL);
}
static ssize_t ax_os_mem_max_disp_write(struct file *file, const char *buffer, size_t count, loff_t *f_pos)
{
	char tmp[9] = {0};
	long val;
	int ret;
	if((count > 8))
		return -EINVAL;
	if (copy_from_user(tmp, buffer, count)) {
		pr_err("%s copy_from_user failed!!!\n", __FUNCTION__);
		return -EFAULT;
	}
	ret = kstrtoul(tmp, 10, &val);
 	if (ret)
 		return ret;
	ax_mem_set_max_dispc_num(val);
	return count;
}

static const struct file_operations os_mem_max_disp_ops = {
	.open = ax_os_mem_max_disp_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = ax_os_mem_max_disp_write,
};
static s32 ax_os_mem_ctrl_show(struct seq_file *m, void *v)
{
	seq_printf(m, "clear user os_mem stat command: echo 1 > /proc/%s\n", AX_OS_MEM_CTRL_NAME);
	return 0;
}

static s32 ax_os_mem_ctrl_open(struct inode *inode, struct file *file)
{
	return single_open(file, ax_os_mem_ctrl_show, NULL);
}
static s32 ax_mem_ctrl_command(int cmd)
{
	switch(cmd) {
	case OS_MEM_CTRL_CLR_USER_STAT:
		ax_mem_clr_glb_user_stat();
		break;
	default:
		break;
	}
	return 0;
}

static ssize_t ax_os_mem_ctrl_write(struct file *file, const char *buffer, size_t count, loff_t *f_pos)
{
	char tmp[9] = {0};
	long val;
	int ret;
	if((count > 8))
		return -EINVAL;
	if (copy_from_user(tmp, buffer, count)) {
		pr_err("%s copy_from_user failed!!!\n", __FUNCTION__);
		return -EFAULT;
	}
	ret = kstrtoul(tmp, 10, &val);
	if (ret)
		return ret;
	ax_mem_ctrl_command(val);
	return count;
}

static const struct file_operations os_mem_ctrl_ops = {
	.open = ax_os_mem_ctrl_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
	.write = ax_os_mem_ctrl_write,
};
static s32 ax_os_mem_proc_init(void)
{
	os_mem_root = proc_mkdir(AX_OS_MEM_ROOT_NAME, NULL);
	stat_file = proc_create(AX_OS_MEM_STAT_NAME, 0644, NULL, &os_mem_stat_ops);
	if (!stat_file) {
		ax_pr_err(0, "os_mem", "creat stat failed failed\n");
		return -1;
	}
	policy_file = proc_create(AX_OS_MEM_POLICY_NAME, 0644, NULL, &os_mem_policy_ops);
	if (!policy_file) {
		ax_pr_err(0, "os_mem", "creat policy failed failed\n");
		return -1;
	}
	max_dispc_file = proc_create(AX_OS_MEM_MAX_DISP_NAME, 0644, NULL, &os_mem_max_disp_ops);
	if (!policy_file) {
		ax_pr_err(0, "os_mem", "creat max_disp failed\n");
		return -1;
	}
	ctrl_file = proc_create(AX_OS_MEM_CTRL_NAME, 0644, NULL, &os_mem_ctrl_ops);
	if (!ctrl_file) {
		ax_pr_err(0, "os_mem", "creat ctrl failed\n");
		return -1;
	}
	return 0;
}
static s32 ax_os_mem_install(void)
{
	s32 ret;
	int i;
	os_mem_stat_array = kzalloc(sizeof(os_mem_stat_t) * AX_OS_MEM_ID_MAX, GFP_KERNEL);

	if(!os_mem_stat_array) {
		return -ENOMEM;
	}
	for(i = 0; i < AX_OS_MEM_ID_MAX; i++) {
		spin_lock_init(&os_mem_stat_array[i].lock);
	}
	ax_mem_glb_lock_init();
	ax_mem_set_max_dispc_num(AX_OS_DEFAULT_MAX_DISPC_NUM);
	ax_mem_set_proc_mod_id(AX_OS_MOD_PROC_ALL);
	ret = misc_register(&ax_os_mem_miscdev);
	if (ret) {
		kfree(os_mem_stat_array);
		pr_err("err: %s misc_register failed!\n", __func__);
		goto end;
	}
	ret = ax_os_mem_proc_init();
	if (ret) {
		pr_err("proc init fail\n");
		goto end;
	}
end:
	return ret;
}
static s32 ax_os_mem_remove(void)
{
	if (ctrl_file) {
		remove_proc_entry(AX_OS_MEM_CTRL_NAME, NULL);
	}
	if (policy_file) {
		remove_proc_entry(AX_OS_MEM_POLICY_NAME, NULL);
	}
	if (stat_file) {
		remove_proc_entry(AX_OS_MEM_STAT_NAME, NULL);
	}
	if (max_dispc_file) {
		remove_proc_entry(AX_OS_MEM_MAX_DISP_NAME, NULL);
	}
	if (os_mem_root) {
		remove_proc_entry(AX_OS_MEM_ROOT_NAME, NULL);
	}
	if(os_mem_stat_array) {
		kfree(os_mem_stat_array);
	}
	misc_deregister(&ax_os_mem_miscdev);
	return 0;
}

static s32 __init ax_os_mem_init(void)
{
	pr_info("ax_os_mem_init\n");
	ax_os_mem_install();
	return 0;
}

static void ax_os_mem_exit(void)
{
	ax_os_mem_remove();
}

module_init(ax_os_mem_init);
module_exit(ax_os_mem_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("Axera OSMEM driver");
MODULE_AUTHOR("Axera");
