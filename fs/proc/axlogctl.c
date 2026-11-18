#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/seq_file.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/mm.h>
#include <linux/gfp.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include "axlogctl.h"

#define MEM_SIZE         PAGE_SIZE

static axlogctl_mem_cfg_ptr s_logctl_cfg = NULL;

typedef enum {
	SYS_LOG_TARGET_MIN = 0,
	SYS_LOG_TARGET_STDERR = 1,
	SYS_LOG_TARGET_SYSLOG = 2,
	SYS_LOG_TARGET_NULL   = 3,
	SYS_LOG_TARGET_MAX
} AX_LOG_TARGET_E;

void ax_set_kloglvl_info(unsigned long viraddr);

static void dump_header(void)
{
	if (s_logctl_cfg == NULL){
		pr_err("logctl not init!");
		return;
	}
	printk("=======================================\n");
	printk("phyaddr :   %lx\n", s_logctl_cfg->phyaddr);
	printk("kviraddr:   %lx\n", s_logctl_cfg->kviraddr);
	printk("klvladdr:   %lx\n", s_logctl_cfg->klvladdr);
	printk("uviraddr:   %lx\n", s_logctl_cfg->uviraddr);
	printk("ulvladdr:   %lx\n", s_logctl_cfg->ulvladdr);
	printk("mem_size:   %lx\n", s_logctl_cfg->mem_size);
	printk("ulog_state: %d\n",  s_logctl_cfg->ulog_state);
	printk("klog_state: %d\n",  s_logctl_cfg->klog_state);
	printk("level_min:  %d\n",  s_logctl_cfg->level_min);
	printk("level_max:  %d\n",  s_logctl_cfg->level_max);
	printk("max count:  %d\n",  s_logctl_cfg->max_id);
#ifdef CONFIG_PHYS_ADDR_T_64BIT
	printk("size:       %ld\n", sizeof(axlog_lvl_info_s));
#else
	printk("size:       %d\n", sizeof(axlog_lvl_info_s));
#endif
	printk("target:     %d\n",  s_logctl_cfg->ulog_target);
	printk("config:     %s\n",  s_logctl_cfg->config);
	printk("=======================================\n");
}

static long logctl_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch (cmd)
	{
		case AX_LOGCTL_GET_MEM_CFG:
			if (copy_to_user((void*)arg, s_logctl_cfg, sizeof(axlogctl_mem_cfg_s)))
				pr_err("@%s copy_to_user Failed!!!\n", __FUNCTION__);
			break;
		default:
			pr_warn("@%s unsupported cmd: %X\n", __FUNCTION__, cmd);
		break;
	}
	return 0;
}

static int __init axera_logctl_init(void)
{
	struct device_node *of_node;
	const char *config;
	unsigned long memaddr;
	unsigned char logstate[2] = {0};
	unsigned char loglevel[2] = {0};
	unsigned char ustate, kstate;
	unsigned char ulevel, klevel;
	axlog_lvl_info_ptr loglvlptr;
	int len = 0;
	int i = 0;
	int count;

	of_node = of_find_compatible_node(NULL, NULL, "axera,logctl");;
	if (!of_node) {
		pr_err("%s:can't find memory_dump tree node\n", __FUNCTION__);
		return -ENODEV;
	}
	pr_info("%s: of_find_compatible_node 'axera,logctl' success!!!", __FUNCTION__);

	if (of_property_read_string(of_node, "config", &config)) {
		pr_err("%s:can't find 'config' in logctl device tree node\n",__FUNCTION__);
		return -1;
	}
	pr_info("%s: of_find_compatible_node config: %s", __FUNCTION__, config);

	len = of_property_read_variable_u8_array(of_node, "logstate", logstate, 2, ARRAY_SIZE(logstate));
	pr_info("%s: logstate:%hhd %hhd len:%d\n", __FUNCTION__, logstate[0], logstate[1], len);
	if (len > 0){
		ustate =  logstate[0] & 0x1;
		kstate =  logstate[1] & 0x1;
	}else{
		ustate = 1;
		kstate = 1;
	}

	len = of_property_read_variable_u8_array(of_node, "loglevel", loglevel, 2, ARRAY_SIZE(loglevel));
	pr_info("%s: loglevel:%hhd %hhd len:%d\n", __FUNCTION__, loglevel[0], loglevel[1], len);
	if (len > 0){
		ulevel =  loglevel[0] & 0x7;
		klevel =  loglevel[1] & 0x7;
	}else{
		ulevel = 4;
		klevel = 4;
	}

	memaddr = get_zeroed_page(GFP_KERNEL|GFP_DMA);
	if (memaddr == 0){
		pr_err("%s: failed to alloc memory!!!\n", __FUNCTION__);
		return -ENOMEM;
	}

	s_logctl_cfg = (axlogctl_mem_cfg_ptr)memaddr;
	s_logctl_cfg->kviraddr = memaddr;
	s_logctl_cfg->phyaddr  = virt_to_phys((void*)memaddr);
	s_logctl_cfg->klvladdr = memaddr + sizeof(axlogctl_mem_cfg_s);
	s_logctl_cfg->mem_size = MEM_SIZE;
	s_logctl_cfg->ulog_state = ustate;
	s_logctl_cfg->klog_state = kstate;
	count = (MEM_SIZE - sizeof(axlogctl_mem_cfg_s))/sizeof(axlog_lvl_info_s);
	s_logctl_cfg->max_id = count > AX_MAX_ID ? AX_MAX_ID: count ;
	loglvlptr = (axlog_lvl_info_ptr)s_logctl_cfg->klvladdr;
	for (i=0; i <= s_logctl_cfg->max_id; i++ ){
		loglvlptr[i].level  = ulevel;
		loglvlptr[i].klevel = klevel;
	}
	strncpy(s_logctl_cfg->config, config, AX_MAX_LINE);
	ax_set_kloglvl_info(s_logctl_cfg->kviraddr);

	return 0;
}

static ssize_t logctl_proc_write(struct file *file, const char *buffer, size_t count, loff_t *f_pos)
{
	char *tmp = kzalloc((count + 1), GFP_KERNEL);
	unsigned char id, level;
	char logType[64] = {0};
	char param1[64]  = {0};
	int  ret, i;
	axlog_lvl_info_ptr pLevel = NULL;

	if (!tmp)
		return -ENOMEM;
	if (copy_from_user(tmp, buffer, count)) {
		pr_err("@%s copy_from_user failed!!!\n", __FUNCTION__);
		kfree(tmp);
		return -EFAULT;
	}
	pLevel = (axlog_lvl_info_ptr)s_logctl_cfg->klvladdr;
	do
	{
		ret = sscanf(tmp, "%s %s %hhd", logType, param1, &level);
		if (0 == strcmp(logType, LOG_TYPE_USER)){
			switch (ret)
			{
			case 2:
				if (0 == strcmp(param1, CMD_LOG_ON)){
					s_logctl_cfg->ulog_state = 1;
				}else if (0 == strcmp(param1, CMD_LOG_OFF)){
					s_logctl_cfg->ulog_state = 0;
				}else if (0 == strcmp(param1, TARGET_CONSOLE)){
					s_logctl_cfg->ulog_target = SYS_LOG_TARGET_STDERR;
				}else if (0 == strcmp(param1, TARGET_FILE)){
					s_logctl_cfg->ulog_target = SYS_LOG_TARGET_SYSLOG;
				}else if (0 == strcmp(param1, TARGET_NULL)){
					s_logctl_cfg->ulog_target = SYS_LOG_TARGET_NULL;
				}else{
					pr_warn("[axlogctl] unsupported cmd: %s", tmp);
				}
				break;
			case 3:
				if (0 == strcmp(param1, CMD_ALL)){
					if (level >= 0 && level <= 7) {
						for (i = 0; i <= s_logctl_cfg->max_id; i++ )
						{
							pLevel[i].level = level;
						}
					}else{
						pr_warn("[axlogctl] unsupported cmd: %s", tmp);
					}
				}else{
					ret = sscanf(tmp, "%s %hhd %hhd", logType, &id, &level);
					if ( ret == 3 
					&&(id >= 0 && id <= s_logctl_cfg->max_id)
					&& (level >= 0 && level <= 7) ){
						pLevel[id].level = level;
						printk("[axlogctl] %s's(id: %hd) user log level set to %hd\n", pLevel[id].name, id, level);
					}else{
						pr_warn("[axlogctl] unsupported cmd: %s", tmp);
					}
				}
				break;
			default:
				pr_warn("[axlogctl] unsupported cmd: %s", tmp);
				break;
			}

		}else if (0 == strcmp(logType, LOG_TYPE_KERNEL)){
			switch (ret)
			{
			case 2:
				if (0 == strcmp(param1, CMD_LOG_ON)){
					s_logctl_cfg->klog_state = 1;
				}else if (0 == strcmp(param1, CMD_LOG_OFF)){
					s_logctl_cfg->klog_state = 0;
				}else{
					pr_warn("[axlogctl] unsupported cmd: %s", tmp);
				}
				break;
			case 3:
				if (0 == strcmp(param1, CMD_ALL)){
					if (level >= 0 && level <= 7) {
						for (i = 0; i <= s_logctl_cfg->max_id; i++ )
						{
							pLevel[i].klevel = level;
						}
					}else{
						pr_warn("[axlogctl] unsupported cmd: %s", tmp);
					}
				}else{
					ret = sscanf(tmp, "%s %hhd %hhd", logType, &id, &level);
					if ( ret == 3 
					&&(id >= 0 && id <= s_logctl_cfg->max_id)
					&& (level >= 0 && level <= 7) ){
						pLevel[id].klevel = level;
						printk("[axlogctl] %s's(id: %hd) kernel log level set to %hd\n", pLevel[id].name, id, level);
					}else{
						pr_warn("[axlogctl] unsupported cmd: %s", tmp);
					}
				}
				break;
			default:
				pr_warn("[axlogctl] unsupported cmd: %s", tmp);
				break;
			}
		}else if(0 == strcmp(logType, "dumphead")){
			dump_header();
		}else {
			pr_warn("[axlogctl] unsupported cmd: %s", tmp);
		}
	} while(0);

	kfree(tmp);
	tmp = NULL;

	return count;
}

static int axlogctl_proc_show(struct seq_file *p, void *v)
{
	int i;
	axlog_lvl_info_ptr pLevel = NULL;
	char *pTarges[] = {TARGET_CONSOLE, TARGET_FILE, TARGET_NULL};
	unsigned char id_min  = 0;
	unsigned char id_max  = 0;
	unsigned char target  = s_logctl_cfg->ulog_target;
	unsigned char lvl_min = s_logctl_cfg->level_min;
	unsigned char lvl_max = s_logctl_cfg->level_max;
	unsigned char max_id   = s_logctl_cfg->max_id;

	pLevel = (axlog_lvl_info_ptr)s_logctl_cfg->klvladdr;

	for (i = 0; i <= max_id; i++){
		if (strlen(pLevel[i].name) > 0 ){
			id_min = i < id_min ? i : id_min;
			id_max = i > id_max ? i : id_max;
		}
	}
	seq_printf(p, "*************  Axera log control ************\n");
	seq_printf(p, "Usage:\n");
	seq_printf(p, "  echo [ulog/klog] [id] [level] > %s\n", LOGCTL_DEV);
	seq_printf(p, "  ulog: user log klog: kernel log\n");
	seq_printf(p, "       id: [%hhd-%hhd, all] level: [%hhd-%hhd])\n", id_min, id_max, lvl_min, lvl_max);
	seq_printf(p, "  echo ulog/klog/all [on, off] > %s\n", LOGCTL_DEV);
	seq_printf(p, "  echo ulog [%s, %s, %s] > %s(user log only)\n", TARGET_FILE, TARGET_CONSOLE, TARGET_NULL, LOGCTL_DEV);
	seq_printf(p, "--------------------------------------------\n");
	seq_printf(p, "  klog_state:  %s\n", s_logctl_cfg->klog_state==1?"on":"off");
	seq_printf(p, "  ulog_state:  %s\n", s_logctl_cfg->ulog_state==1?"on":"off");
	if (target > SYS_LOG_TARGET_MIN && target < SYS_LOG_TARGET_MAX)
		seq_printf(p, "  ulog_target: %s\n", pTarges[target-1]);

	seq_printf(p, "--------------------------------------------\n");
	seq_printf(p, "  %8s  %4s  %8s  %8s\n", "module", "id", "level(U)", "level(K)");
	for (i =0; i < s_logctl_cfg->max_id; i++)
	{
		if (strlen(pLevel[i].name) <= 0 )
			continue;
		seq_printf(p, "  %8s  %4u  %4u  %8u\n", pLevel[i].name, pLevel[i].id, pLevel[i].level, pLevel[i].klevel);
	}

	return 0;
}

static int logctl_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, axlogctl_proc_show, NULL);
}

static const struct file_operations proc_axlogctl_operations = {
	.open  = logctl_proc_open,
	.read  = seq_read,
	.write = logctl_proc_write,
	.unlocked_ioctl = logctl_ioctl,
	.release = single_release,
};

static int __init logctl_proc_init(void)
{
	axera_logctl_init();
	proc_create(PROC_NAME, S_IRUSR, NULL, &proc_axlogctl_operations);
	return 0;
}

module_init(logctl_proc_init);
