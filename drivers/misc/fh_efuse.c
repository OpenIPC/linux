#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include <linux/fh_efuse.h>
#include <mach/fh_efuse_plat.h>
#include <linux/io.h>
#include <linux/of_platform.h>
#include <linux/of_address.h>
/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/

#define wrap_readl(wrap, name) \
	__raw_readl(&(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_writel(wrap, name, val) \
	__raw_writel((val), &(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_readw(wrap, name) \
	__raw_readw(&(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_writew(wrap, name, val) \
	__raw_writew((val), &(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_readb(wrap, name) \
	__raw_readb(&(((struct wrap_efuse_reg *)wrap->regs)->name))

#define wrap_writeb(wrap, name, val) \
	__raw_writeb((val), &(((struct wrap_efuse_reg *)wrap->regs)->name))

#define FH_EFUSE_PLAT_DEVICE_NAME			"fh_efuse"
#define FH_EFUSE_MISC_DEVICE_NAME			"fh_efuse"
#define FH_EFUSE_MISC_DEVICE_NODE_NAME		"fh_efuse_node"
#define EFUSE_HW_BUFFER_POS					(4)

#if defined CONFIG_ARCH_FH8632 || defined CONFIG_ARCH_FH8626V100
#define MATCH_KEY 0x53d485a7
#else
#define MATCH_KEY 0x92fc0025
#endif

#define SECURE_BOOT_ENTRY       (48)
#define SECURE_BOOT_ACTIVE_VAL  (1<<0)
#define SOC_BOOT_MODE_WITH_SECURE   0xaa5555aa
#define SECURE_PROTECT_EFUSE_ENTRY_MAX_INDEX    16
/****************************************************************************
 * ADT section
 *  add definition of user defined Data Type
 *  that only be used in this file  here
 ***************************************************************************/

struct wrap_efuse_reg {
	u32 efuse_cmd;				/*0x0*/
	u32 efuse_config;			/*0x4*/
	u32 efuse_match_key;		/*0x8*/
	u32 efuse_timing0;			/*0xc*/
	u32 efuse_timing1;			/*0x10*/
	u32 efuse_timing2;			/*0x14*/
	u32 efuse_timing3;			/*0x18*/
	u32 efuse_timing4;			/*0x1c*/
	u32 efuse_timing5;			/*0x20*/
	u32 efuse_timing6;			/*0x24*/
	u32 efuse_dout;				/*0x28*/
	u32 efuse_status0;			/*0x2c*/
	u32 efuse_status1;			/*0x30*/
	u32 efuse_status2;			/*0x34*/
	u32 efuse_status3;			/*0x38*/
	u32 efuse_status4;			/*0x3c*/
	u32 efuse_mem_info;
};




enum {
	CMD_TRANS_AESKEY = 4,
	CMD_WFLGA_AUTO = 8,
};

struct wrap_efuse_obj s_efuse_obj = {0};

#define EFUSE_MAX_ENTRY			60
#define EFUSE_DEBUG_ENTRY_NO_MIN			60
#define EFUSE_DEBUG_ENTRY_NO_MAX			62

/******************************************************************************
 * Function prototype section
 * add prototypes for all functions called by this file,execepting those
 * declared in header file
 *****************************************************************************/

/*****************************************************************************
 * Global variables section - Exported
 * add declaration of global variables that will be exported here
 * e.g.
 *  int8_t foo;
 ****************************************************************************/

/*****************************************************************************

 *  static fun;
 *****************************************************************************/
static int fh_efuse_open(struct inode *inode, struct file *file);
static int fh_efuse_release(struct inode *inode, struct file *filp);
static long fh_efuse_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg);
/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/

static const struct file_operations fh_efuse_fops = {
		.owner = THIS_MODULE,
		.open = fh_efuse_open,
		.release = fh_efuse_release,
		.unlocked_ioctl = fh_efuse_ioctl,
};

static struct miscdevice fh_efuse_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = FH_EFUSE_MISC_DEVICE_NAME,
	/*.nodename = FH_EFUSE_MISC_DEVICE_NODE_NAME,*/
	.fops = &fh_efuse_fops,
};

static int fh_efuse_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_efuse_release(struct inode *inode, struct file *filp)
{
	return 0;
}

void efuse_detect_complete(struct wrap_efuse_obj *obj, int pos)
{
	unsigned int rdata;
	unsigned int time = 0;
	/*printk("efuse wait pos %x...\n",pos);*/
	do {
		time++;
		rdata = wrap_readl(obj, efuse_status0);
		if (time > 1000) {
			printk("[efuse]:detect time out...pos: 0x%x\n", pos);
			return;
		}
		udelay(10);
	} while ((rdata&(1<<pos)) != 1<<pos);
    /*printk("efuse wait pos done...\n",pos);*/
	udelay(10);
}

void auto_check_efuse_pro_bits(struct wrap_efuse_obj *obj, u32 *buff)
{
	/*first set auto check cmd*/
	wrap_writel(obj, efuse_cmd, CMD_WFLGA_AUTO);
	efuse_detect_complete(obj, 8);
	/*
	mdelay(300);
	read status from hw status
	bit 1 means could be writen....
	*/
	buff[0] = wrap_readl(obj, efuse_status1);
	buff[1] = wrap_readl(obj, efuse_status2);
}

void open_efuse_power(struct wrap_efuse_obj *obj)
{
	u32 data = 0;

	data = wrap_readl(obj, efuse_config);
	data |= 1<<27;
	wrap_writel(obj, efuse_config, data);
}

void efuse_buffer_set(struct wrap_efuse_obj *obj, unsigned int value)
{
	u32 data;
	data = wrap_readl(obj, efuse_config);
	data &= ~(1 << EFUSE_HW_BUFFER_POS);
	data |= value << EFUSE_HW_BUFFER_POS;

	wrap_writel(obj, efuse_config, data);
}

void efuse_write_key_byte(struct wrap_efuse_obj *obj, u32 entry, u8 data)
{
	u32 temp = 0;
	temp = (u32)data;
	/*0~255*/
	temp &= ~0xffffff00;
	temp <<= 12;
	/*0~63*/
	entry &= 0x3f;
	temp |= (entry<<4) | (0x02);
	/*
	printk("efuse write entry: %x,data: %x\n",entry,data);

	printk("efuse write data :%x\n",temp);
	wrap_writel(obj,efuse_cmd,(data<<12) + (entry<<4) + (0x02));
	*/
	wrap_writel(obj, efuse_cmd, temp);
	efuse_detect_complete(obj, 2);
}

void efuse_load_usrcmd(struct wrap_efuse_obj *obj)
{
	wrap_writel(obj, efuse_cmd, 1);
	efuse_detect_complete(obj, 1);
}

void refresh_efuse(struct wrap_efuse_obj *obj)
{
	wrap_writel(obj, efuse_cmd, CMD_WFLGA_AUTO);
	efuse_detect_complete(obj, 8);
	efuse_load_usrcmd(obj);
}

void efuse_check_map_para(unsigned int size, struct ex_key_map *p_key_map)
{
	int loop;
	if (size > MAX_EX_KEY_MAP_SIZE) {
		printk(KERN_ERR "%s :: size : %d > max size : %d\n",
		__func__, size, MAX_EX_KEY_MAP_SIZE);
	}
	for (loop = 0; loop < size; loop++) {
		if ((p_key_map[loop].ex_mem_entry % 4 != 0)
			|| (p_key_map[loop].crypto_key_no != loop)) {
				printk(KERN_ERR \
				"map[%d]:entry[0x%x]:aes key[0x%x] para error..\n",
				loop,
				p_key_map[loop].ex_mem_entry,
				p_key_map[loop].crypto_key_no);
		}
	}
}



static int get_soc_crypto_load_mode_from_efuse(struct wrap_efuse_obj *p_efuse)
{
	unsigned char ret_buf;

	refresh_efuse(p_efuse);
	efuse_read_entry(p_efuse, MATCH_KEY,
	SECURE_BOOT_ENTRY, &ret_buf, 1);
    if (ret_buf & SECURE_BOOT_ACTIVE_VAL)
		return SOC_BOOT_MODE_WITH_SECURE;
    return -1;
}

/*
 * 0      :means do not use secure boot or decrypt. all efuse could be transfered to aes
 * others :means use secure boot mode. efuse entry 0~15 DO NOT USE
 */
int fh_efuse_secure_check(struct wrap_efuse_obj *obj,
u32 start_no, u32 size, struct af_alg_usr_def *p_alg)
{
	int ret;
	int i;
	int key_map_size;
	struct ex_key_map *p_key_map;

	ret = get_soc_crypto_load_mode_from_efuse(obj);
    if (ret != SOC_BOOT_MODE_WITH_SECURE)
		return 0;

	if (obj->open_flag & CRYPTO_EX_MEM_SWITCH_KEY) {
		if (p_alg->mode & CRYPTO_EX_MEM_SWITCH_KEY) {
			if (p_alg->mode & CRYPTO_EX_MEM_4_ENTRY_1_KEY) {
				key_map_size = p_alg->adv.ex_key_para.map_size;
				p_key_map = &p_alg->adv.ex_key_para.map[0];
				/* check para. */
				for (i = 0; i < key_map_size; i++, p_key_map++) {
					if (p_key_map->ex_mem_entry <
					SECURE_PROTECT_EFUSE_ENTRY_MAX_INDEX) {
						pr_err("Do not use secure area\n");
						return -1;
					}
				}
			}
		} else {
			/*efuse 0 ~ 15 must be used*/
			return -2;
		}
	} else {
		/* efuse 0 ~ 15 must be used*/
		return -3;
	}
	return 0;
}


void efuse_check_update_trans_flag(struct wrap_efuse_obj *obj,
u32 start_no, u32 size, struct af_alg_usr_def *p_alg){

	int i;
	struct ex_key_map *p_key_map;
	struct ex_key_map *old_p_key_map;
	struct af_alg_usr_def *p_old_usr_def;
	int key_map_size;

	p_old_usr_def = &obj->old_usr_def;
	/* first check if use cpu mode */
	if (p_old_usr_def->mode != p_alg->mode) {
		obj->efuse_trans_flag = EFUSE_NEED_TRANS;
		p_old_usr_def->mode = p_alg->mode;
		/* not return here..maybe para not the same..
		just update below. */
	}
	if (obj->open_flag & CRYPTO_EX_MEM_SWITCH_KEY) {
		if (p_alg->mode & CRYPTO_EX_MEM_SWITCH_KEY) {
			if (p_alg->mode & CRYPTO_EX_MEM_4_ENTRY_1_KEY) {
				key_map_size = p_alg->adv.ex_key_para.map_size;
				p_key_map = &p_alg->adv.ex_key_para.map[0];
				old_p_key_map = &p_old_usr_def->adv.ex_key_para.map[0];
				efuse_check_map_para(key_map_size, p_key_map);
				/* check map size */
				if (key_map_size != p_old_usr_def->adv.ex_key_para.map_size) {
					/* cpy new to old */
					memcpy(p_old_usr_def, p_alg,
						sizeof(struct af_alg_usr_def));
					obj->efuse_trans_flag = EFUSE_NEED_TRANS;
					return;
				}
				/* check para. */
				for (i = 0; i < key_map_size; i++,
					p_key_map++, old_p_key_map++) {
					if (memcmp(p_key_map, old_p_key_map,
						sizeof(struct ex_key_map))) {
						/* cmp error */
						memcpy(p_old_usr_def, p_alg,
						sizeof(struct af_alg_usr_def));
						obj->efuse_trans_flag = EFUSE_NEED_TRANS;
						return;
					}
				}
			}
		} else {
			/*chip need set the map...and usr not set */
			key_map_size = size;
			if (key_map_size != obj->old_size) {
				obj->old_size = key_map_size;
				obj->efuse_trans_flag = EFUSE_NEED_TRANS;
				return;
			}
		}
	} else {
		key_map_size = size;
		if (key_map_size != obj->old_size) {
			obj->old_size = key_map_size;
			obj->efuse_trans_flag = EFUSE_NEED_TRANS;
			return;
		}
	}
}

void efuse_trans_key(struct wrap_efuse_obj *obj,
u32 start_no, u32 size, struct af_alg_usr_def *p_alg)
{
	int i;
	struct ex_key_map *p_key_map;
	int key_map_size;
	unsigned int temp_reg;

	efuse_check_update_trans_flag(obj, start_no, size, p_alg);
	if (obj->efuse_trans_flag != EFUSE_NEED_TRANS) {
		/* printk("###DO NOT need to update efuse para to aes...\n"); */
		return;
	}
	obj->efuse_trans_flag = 0;

	if (obj->open_flag & CRYPTO_EX_MEM_SWITCH_KEY) {
		if (p_alg->mode & CRYPTO_EX_MEM_SWITCH_KEY) {
			if (p_alg->mode & CRYPTO_EX_MEM_4_ENTRY_1_KEY) {
				key_map_size = p_alg->adv.ex_key_para.map_size;
				p_key_map = &p_alg->adv.ex_key_para.map[0];
				efuse_check_map_para(key_map_size, p_key_map);
				for (i = 0; i < key_map_size;
				i++, p_key_map++) {
					refresh_efuse(obj);
					temp_reg =
					wrap_readl(obj, efuse_config);
					temp_reg &= ~(0xf<<28);
					temp_reg |=
					(p_key_map->ex_mem_entry / 4) << 28;
					wrap_writel(obj,
					efuse_config, temp_reg);
					wrap_writel(obj,
					efuse_cmd, (i << 20) + 0x04);
					efuse_detect_complete(obj, 4);
				}
		}
		} else {
			/*chip need set the map...and usr not set */
			key_map_size = size;
			for (i = 0; i < key_map_size; i++) {
				refresh_efuse(obj);
				temp_reg = wrap_readl(obj, efuse_config);
				temp_reg &= ~(0xf<<28);
				temp_reg |= i << 28;
				wrap_writel(obj, efuse_config, temp_reg);
				wrap_writel(obj, efuse_cmd, (i << 20) + 0x04);
				efuse_detect_complete(obj, 4);
			}
		}
	} else {
		key_map_size = size;
		for (i = 0; i < key_map_size; i++) {
			refresh_efuse(obj);
			wrap_writel(obj, efuse_cmd, (i << 20) + 0x04);
			efuse_detect_complete(obj, 4);
		}
	}

}

static void aes_biglittle_swap(u8 *buf)
{
	u8 tmp, tmp1;

	tmp = buf[0];
	tmp1 = buf[1];
	buf[0] = buf[3];
	buf[1] = buf[2];
	buf[2] = tmp1;
	buf[3] = tmp;
}
void efuse_get_lock_status(struct wrap_efuse_obj *obj,
struct efuse_status *status)
{
	status->efuse_apb_lock = (wrap_readl(obj, efuse_status0)>>20) & 0x0f;
	status->aes_ahb_lock = (wrap_readl(obj, efuse_status0)>>24) & 0x0f;
}

void efuse_read_entry(struct wrap_efuse_obj *obj,
u32 key, u32 start_entry, u8 *buff, u32 size)
{
	u32 data, i;

	for (i = 0; i < size; i++) {
		wrap_writel(obj, efuse_match_key, key);
		wrap_writel(obj, efuse_cmd, ((start_entry + i)<<4) + 0x03);
		efuse_detect_complete(obj, 3);
		data = wrap_readl(obj, efuse_dout);
		*buff++ = (u8)data;
	}
}

unsigned char efuse_read_debug_entry(u32 entry)
{
	unsigned char ret;

	if (entry < EFUSE_DEBUG_ENTRY_NO_MIN ||
	entry > EFUSE_DEBUG_ENTRY_NO_MAX) {
		pr_err("Error entry %d\n", entry);
		return 0;
	}
	efuse_read_entry(&s_efuse_obj, 0, entry, &ret, 1);
	return ret;
}
EXPORT_SYMBOL(efuse_read_debug_entry);

long fh_efuse_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg)
{
	int i;

	EFUSE_INFO efuse_info = {0};
	u32 *p_dst = NULL;
	u8 *p_dst_8 = NULL;
	unsigned int data;
	u32 temp_swap_data[32] = {0};

	if (copy_from_user((void *) &efuse_info, (void __user *) arg,
			sizeof(EFUSE_INFO))) {
		return -EFAULT;
	}

	refresh_efuse(&s_efuse_obj);
	switch (cmd) {
	case IOCTL_EFUSE_CHECK_PRO:
		auto_check_efuse_pro_bits(&s_efuse_obj,
		efuse_info.status.protect_bits);
		break;
	case IOCTL_EFUSE_WRITE_KEY:
		if (copy_from_user((void *) &temp_swap_data[0],
			(void __user *) efuse_info.key_buff,
			efuse_info.key_size)) {
			return -EFAULT;
		}

		p_dst = &temp_swap_data[0];

		for (i = 0; i < efuse_info.key_size / sizeof(u32); i++)
			aes_biglittle_swap((u8 *) (p_dst + i));
		p_dst_8 = (u8 *)&temp_swap_data[0];

		for (i = 0; i < efuse_info.key_size; i++) {
			efuse_write_key_byte(&s_efuse_obj,
			efuse_info.efuse_entry_no + i, *(p_dst_8 + i));
		}

		break;
	case IOCTL_EFUSE_CHECK_LOCK:

		efuse_get_lock_status(&s_efuse_obj, &efuse_info.status);
		break;
	case IOCTL_EFUSE_TRANS_KEY:
		printk(KERN_WARNING "please use efuse transkey with aes...\n");
		break;
	case IOCTL_EFUSE_SWITCH_CPU_KEY_MODE:
		printk(KERN_WARNING "please use cpu key with aes...\n");
		break;
	case IOCTL_EFUSE_SWITCH_EFUSE_KEY_MODE:
		printk(KERN_WARNING "please use efuse key with aes...\n");
		break;
	case IOCTL_EFUSE_CHECK_ERROR:
		printk(KERN_WARNING "not support...\n");
		break;
	case IOCTL_EFUSE_READ_KEY:
		/*printf("match is  : %x..\n",match_key);*/
		efuse_read_entry(&s_efuse_obj, efuse_info.status.error,
		efuse_info.efuse_entry_no,
		(u8 *)&temp_swap_data[0], efuse_info.key_size);
		p_dst = (u32 *)temp_swap_data;
		for (i = 0; i < efuse_info.key_size / sizeof(u32); i++) {
			aes_biglittle_swap((u8 *) (p_dst + i));
			/*printk("swap data is %x\n",*(p_dst + i));*/
		}
		if (copy_to_user((void __user *) (efuse_info.key_buff),
			(void *) &temp_swap_data[0],
			efuse_info.key_size)) {
			return -EFAULT;
		}
		/*memcpy(efuse_user_info,&efuse_info,sizeof(EFUSE_INFO));*/
		break;
	case IOCTL_EFUSE_SET_LOCK:
		/*parse lock data...*/
		data = efuse_info.status.aes_ahb_lock;
		data <<= 4;
		data &= 0xf0;
		efuse_info.status.efuse_apb_lock &= 0x0f;
		data |= efuse_info.status.efuse_apb_lock;
		efuse_write_key_byte(&s_efuse_obj, 63, (u8)data);
		break;
	case IOCTL_EFUSE_SET_MAP_PARA_4_TO_1:
		printk(KERN_WARNING "not support here..\
		pls check if chip support this func and set core para\n");
		break;
	case IOCTL_EFUSE_SET_MAP_PARA_1_TO_1:
		printk(KERN_WARNING "not support func here...\n");
		break;
	case IOCTL_EFUSE_CLR_MAP_PARA:
		printk(KERN_WARNING "not support here...\n");
		break;

	case IOCTL_EFUSE_WRITE_ENTRY:
		if (copy_from_user((void *) &temp_swap_data[0],
			(void __user *) efuse_info.key_buff,
			efuse_info.key_size)) {
			return -EFAULT;
		}
		p_dst_8 = (u8 *)&temp_swap_data[0];
		for (i = 0; i < efuse_info.key_size; i++) {
			efuse_write_key_byte(&s_efuse_obj,
			efuse_info.efuse_entry_no + i, *(p_dst_8 + i));
		}

		break;

	case IOCTL_EFUSE_READ_ENTRY:
		efuse_read_entry(&s_efuse_obj, efuse_info.status.error,
		efuse_info.efuse_entry_no,
		(u8 *)&temp_swap_data[0], efuse_info.key_size);
		if (copy_to_user((void __user *) (efuse_info.key_buff),
			(void *) &temp_swap_data[0],
			efuse_info.key_size)) {
			return -EFAULT;
		}

		break;

	default:
		break;
	}
	memcpy((void *)&s_efuse_obj.status, (void *)&efuse_info.status,
			sizeof(struct efuse_status));
	if (copy_to_user((void __user *) (&((EFUSE_INFO *)arg)->status),
			(void *) &efuse_info.status,
			sizeof(struct efuse_status))) {
			return -EFAULT;
	}
	return 0;
}

/*****************************************************************************
 *
 *
 *		function body
 *
 *
 *****************************************************************************/
static int fh_efuse_probe(struct platform_device *pdev)
{
	struct resource *res;
	int err = -ENODEV;
	struct device_node *np = pdev->dev.of_node;
	int temp;
	struct fh_efuse_platform_data *p_efuse_plat;

	if (np && !IS_ERR(np)) {
		s_efuse_obj.open_flag = CRYPTO_CPU_SET_KEY |
		CRYPTO_EX_MEM_SET_KEY;
		s_efuse_obj.regs = of_iomap(np, 0);
		if (!s_efuse_obj.regs) {
			dev_err(&pdev->dev, "efuse io map error..\n");
			return -EINVAL;
		}
		if (strcmp("enable",
		of_get_property(np, "key_switch", &temp)) == 0)
			s_efuse_obj.open_flag |=
			CRYPTO_EX_MEM_SWITCH_KEY | CRYPTO_EX_MEM_4_ENTRY_1_KEY;
		if (strcmp("enable",
		of_get_property(np, "indep_power", &temp)) == 0)
			s_efuse_obj.open_flag |=
			CRYPTO_EX_MEM_INDEP_POWER;
	} else {
		res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
		if (!res) {
			dev_err(&pdev->dev, "can't fetch device resource info\n");
			return -EINVAL;
		}

		s_efuse_obj.regs = ioremap(res->start, resource_size(res));
		if (s_efuse_obj.regs == NULL) {
			dev_err(&pdev->dev, "ioremap resource error\n");
			return -EINVAL;
		}

		p_efuse_plat =
		(struct fh_efuse_platform_data *) pdev->dev.platform_data;
		if (p_efuse_plat == NULL) {
			dev_err(&pdev->dev, "efuse get platform data error..\n");
			return -ENODEV;
		}
		s_efuse_obj.open_flag = p_efuse_plat->efuse_support_flag;
	}


	s_efuse_obj.clk = clk_get(&pdev->dev, "efuse_clk");

	if (IS_ERR(s_efuse_obj.clk)) {
		err = PTR_ERR(s_efuse_obj.clk);
		return err;
	}
	clk_prepare_enable(s_efuse_obj.clk);

	pr_info("efuse open mode is %x\n", s_efuse_obj.open_flag);
	if (s_efuse_obj.open_flag & CRYPTO_EX_MEM_INDEP_POWER) {
		/*efuse power up*/
		open_efuse_power(&s_efuse_obj);
	}
#ifdef EFUSE_BUFFER_OFF
	efuse_buffer_set(&s_efuse_obj, 0);
#endif
	err = misc_register(&fh_efuse_misc);
	if (err != 0) {
		dev_err(&pdev->dev, "efuse register misc error\n");
		return err;
	}

	platform_set_drvdata(pdev, &fh_efuse_misc);

	return 0;

}

static int fh_efuse_remove(struct platform_device *pdev)
{
	struct miscdevice *misc;

	misc = (struct miscdevice *)platform_get_drvdata(pdev);
	misc_deregister(misc);
	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id fh_efuse_of_match[] = {
	{.compatible = "fh,fh-efuse",},
	{},
};

MODULE_DEVICE_TABLE(of, fh_efuse_of_match);
#endif

static struct platform_driver fh_efuse_driver = {
	.driver = {
		.name = "fh_efuse",
#ifdef CONFIG_OF
		.of_match_table = fh_efuse_of_match,
#endif
	},
	.probe = fh_efuse_probe,
	.remove = fh_efuse_remove,
};

module_platform_driver(fh_efuse_driver);
MODULE_DESCRIPTION("fullhan EFUSE device driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("zhangy@fullhan.com");
MODULE_ALIAS("platform:FH_efuse");
