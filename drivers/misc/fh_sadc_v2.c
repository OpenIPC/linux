/*
 * fh_sadc.c
 *
 *  Created on: Mar 13, 2015
 *      Author: duobao
 */
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

/*****************************************************************************
 *  Include Section
 *  add all #include here
 *****************************************************************************/
#include    <mach/fh_sadc.h>
#include    <mach/pmu.h>
#include    <linux/sched.h>


/*****************************************************************************
 * Define section
 * add all #define here
 *****************************************************************************/
#define wrap_readl(wrap, name) \
	__raw_readl(&(((struct wrap_sadc_reg *)wrap->regs)->name))

#define wrap_writel(wrap, name, val) \
	__raw_writel((val), &(((struct wrap_sadc_reg *)wrap->regs)->name))


#define IOCTL_GET_SINGLE_SADC_DATA	    0x1
#define IOCTL_CONTINUE_SADC_CFG	        0x3
#define IOCTL_CONTINUE_SADC_START	    0x4
#define IOCTL_CONTINUE_SADC_END	        0x5
#ifdef CONFIG_FH_SADC_V22
#define IOCTL_HIT_SADC_CFG	            0x6
#endif
#define IOCTL_SADC_POWER_DOWN	0xff




#define SADC_WRAP_BASE			(SADC_REG_BASE)
#define SADC_IRQn			(23)
#define SADC_MAX_CONTROLLER		(1)
#define SADC_STATUS_COLESD		(0)
#define SADC_STATUS_OPEN		(1)
#define FH_SADC_PLAT_DEVICE_NAME		"fh_sadc"
#define FH_SADC_MISC_DEVICE_NAME		"fh_sadc"
/*void (*g_sadc_get_continue_data)(u8 channel, u32 value) = NULL;*/
/*void (*g_sadc_get_hit_data)(u8 channel, u32 value) = NULL;*/

/****************************************************************************
* ADT section
*  add definition of user defined Data Type that only be used in this file  here
***************************************************************************/
struct sadc_info {
	int channel;
	int sadc_data;
};

struct sadc_continue_cfg {
	u32              channel_cfg;
	u32              continue_time;   /*ms*/
	u32              glitch_value;    /*mv*/
	u32              glitch_time;     /*count*/
	u32              eq_flag;
	u32              zero_value;      /*ms*/
	u32              precision;       /*0xfff*/
	u32              powerdown;
};

struct sadc_continue_ctrl {
	struct sadc_info usrdata;
	u32  flag;
};


#ifdef CONFIG_FH_SADC_V22
struct sadc_hit_data_cfg {
	u8   channel_cfg;
	u8   value_en;
	u16  hit_gap;/*mv*/
	u16  value[8];/*mv*/
};
#endif

struct sadc_continue_ctrl g_sadc_data[SADC_CHANNEL_NUM];
u32 g_sadc_debug;
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
static u32 fh_sadc_isr_read_data(struct wrap_sadc_obj *sadc,\
	 u32 channel, u16 *buf);
static int fh_sadc_open(struct inode *inode, struct file *file);
static int fh_sadc_release(struct inode *inode, struct file *filp);
static long fh_sadc_ioctl(struct file *filp, unsigned int cmd,\
	 unsigned long arg);
static int fh_sadc_read(struct file *filp, char __user *buf,
		size_t len, loff_t *off);

/*****************************************************************************
 * Global variables section - Local
 * define global variables(will be refered only in this file) here,
 * static keyword should be used to limit scope of local variable to this file
 * e.g.
 *  static uint8_t ufoo;
 *****************************************************************************/
static struct wrap_sadc_obj fh_sadc_obj;

static const struct file_operations fh_sadc_fops = {
	.owner = THIS_MODULE,
	.open = fh_sadc_open,
	.read = fh_sadc_read,
	.release = fh_sadc_release,
	.unlocked_ioctl = fh_sadc_ioctl,
};

static struct miscdevice fh_sadc_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = FH_SADC_MISC_DEVICE_NAME,
	.fops = &fh_sadc_fops,
};

/*****************************************************************************
 *
 *
 *		function body
 *
 *
 *****************************************************************************/
u32 fh_sadc_isr_read_data(struct wrap_sadc_obj *sadc, u32 channel,
		u16 *buf) {
	u32 xainsel = 1 << channel;
	u32 xversel = 0;
	u32 xpwdb = 1;
	/*cnt*/
	u32 sel2sam_pre_cnt = 2;
	u32 sam_cnt = 2;
	u32 sam2sel_pos_cnt = 2;
	/*time out*/
	u32 eoc_tos = 0xff;
	u32 eoc_toe = 0xff;
	u32 time_out = 0xffff;
	/*set isr en..*/
	u32 sadc_isr = 0x01;
	/*start*/
	u32 sadc_cmd = 0x01;
	/*get data*/
	u32 temp_data = 0;
	u32 ret_time;

	/*control...*/
	wrap_writel(sadc, sadc_control, xainsel | (xversel << 8) \
			| (xpwdb << 12));

	wrap_writel(sadc, sadc_cnt,
			sel2sam_pre_cnt | (sam_cnt << 8) | \
			(sam2sel_pos_cnt << 16));

	wrap_writel(sadc, sadc_timeout,
			eoc_tos | (eoc_toe << 8) | (time_out << 16));

	wrap_writel(sadc, sadc_ier, sadc_isr);

	wrap_writel(sadc, sadc_cmd, sadc_cmd);

	ret_time = wait_for_completion_timeout(&sadc->done, 5000);
	if (ret_time == 0) {
		printk(KERN_ERR "sadc timeout..\n");
		return SADC_TIMEOUT;
	}

	switch (channel) {
	case 0:
	case 1:
		/*read channel 0 1*/
		temp_data = wrap_readl(sadc, sadc_dout0);
		break;

	case 2:
	case 3:
		/*read channel 2 3*/
		temp_data = wrap_readl(sadc, sadc_dout1);
		break;

	case 4:
	case 5:
		/*read channel 4 5*/
		temp_data = wrap_readl(sadc, sadc_dout2);
		break;

	case 6:
	case 7:
		/*read channel 6 7*/
		temp_data = wrap_readl(sadc, sadc_dout3);
		break;
	default:
		break;
	}
	if (channel % 2) {
		/*read low 16bit*/
		*buf = (u16) (temp_data & 0xffff);
	} else {
		/*read high 16bit*/
		*buf = (u16) (temp_data >> 16);
	}
	return 0;

}
EXPORT_SYMBOL(fh_sadc_isr_read_data);

/* 0:single mode;1:continue mode*/
int fh_sadc_mode_set(u32 mode)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_model);
	if (mode)
		reg |= 1;
	else
		reg &= (~1);
	wrap_writel(sadc, sadc_model, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_mode_set);

int fh_sadc_lpc_en_set(u32 lpc_flag)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_chn_cfg);
	if (lpc_flag)
		reg |= (1 << 28);
	else
		reg &= (~(1 << 28));
	wrap_writel(sadc, sadc_chn_cfg, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_lpc_en_set);

u32 fh_sadc_lpc_en_get(void)
{
	u32 reg;
	u32 ret;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_chn_cfg);
	reg &= (1 << 28);
	ret = reg >> 28;
	return ret;
}
EXPORT_SYMBOL(fh_sadc_lpc_en_get);

int fh_sadc_scan_delta(u32 scan_delta)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32 value;

	value = ((u32)scan_delta*SADC_MAX_AD_VALUE)/SADC_REF;
	if (value > 0x3f)
		value = 0x3f;
	reg = wrap_readl(sadc, sadc_chn_cfg1);
	reg &= (~(0x3f<<0));
	reg |= value ;
	wrap_writel(sadc, sadc_chn_cfg1, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_scan_delta);


int fh_sadc_glitch_en_set(u32 glitch_flag)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_chn_cfg);
	if (glitch_flag)
		reg |= (1 << 27);
	else
		reg &= (~(1 << 27));
	wrap_writel(sadc, sadc_chn_cfg, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_glitch_en_set);

u32 fh_sadc_glitch_en_get(void)
{
	u32 reg;
	u32 ret;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_chn_cfg);
	reg &= (1 << 27);
	ret = reg >> 27;
	return ret;
}
EXPORT_SYMBOL(fh_sadc_glitch_en_get);

int fh_sadc_scan_start(void)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_scan_en);
	reg |= (1 << 0);
	wrap_writel(sadc, sadc_scan_en, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_scan_start);

int fh_sadc_scan_end(void)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_scan_en);
	reg |= (1 << 1);
	wrap_writel(sadc, sadc_scan_en, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_scan_end);

int fh_sadc_scan_power_cnt(u8 scan_power_cnt)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_chn_cfg1);
	reg &= (~(0xff<<8));
	reg |= (scan_power_cnt << 8);
	wrap_writel(sadc, sadc_chn_cfg1, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_scan_power_cnt);


int fh_sadc_cons_ch_eq_set(u32 chn_num)
{
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	reg = wrap_readl(sadc, sadc_glitch_cfg);
	reg |= (chn_num&0xff);
	wrap_writel(sadc, sadc_glitch_cfg, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_cons_ch_eq_set);

int fh_sadc_set_act_bit(u32 act_bit)
{
	u32 reg;
	u32 value;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	if (act_bit > 12)
		value = 12;
	else
		value = act_bit;
	value = (~(1<<value))&0xfff;
	reg = wrap_readl(sadc, sadc_glitch_cfg);
	reg &= (~(0xfff<<8));
	reg |= (value<<8);
	wrap_writel(sadc, sadc_glitch_cfg, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_set_act_bit);

int fh_sadc_set_continuous_time(u32 continuous_time)
{
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	struct clk  *sadc_clk;
	u32    value;

	sadc_clk = clk_get(NULL, "sadc_clk");
	if (IS_ERR(sadc_clk))
		return -EPERM;

	/*value = (glitch_time/1000)*sadc_clk->rate;FPGA 1000000HZ*/
	value = continuous_time*(1000000/1000);
	wrap_writel(sadc, sadc_continuous_time, value);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_set_continuous_time);

int fh_sadc_set_glitch_time(u32 glitch_time)
{
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	wrap_writel(sadc, sadc_glitch_time, glitch_time);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_set_glitch_time);

int fh_sadc_set_zero_value(u32 zero_value)
{
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32    value;
	u32    reg;

	value = (zero_value*SADC_MAX_AD_VALUE)/SADC_REF;
	value &= 0xfff;
	reg = wrap_readl(sadc, sadc_glitch_cfg);
	reg &= (~(0xfff<<20));
	reg |= (value<<20);
	wrap_writel(sadc, sadc_glitch_cfg, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_set_zero_value);

#ifdef CONFIG_FH_SADC_V22
int fh_sadc_hit_data_config(struct sadc_hit_data_cfg *hit_data)
{
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32    value;
	u32    value1;
	u32    reg;

	value = hit_data->hit_gap;
	value = (value*SADC_MAX_AD_VALUE)/SADC_REF;
	value &= 0xfff;
	reg = value|(hit_data->value_en << 16)|(hit_data->channel_cfg << 24);
	wrap_writel(sadc, sadc_hit_cfg, reg);

	value = hit_data->value[0];
	value = (value*SADC_MAX_AD_VALUE)/SADC_REF;
	value &= 0xfff;

	value1 = hit_data->value[1];
	value1 = (value1*SADC_MAX_AD_VALUE)/SADC_REF;
	value1 &= 0xfff;
	reg = value | (value1 << 16);

	wrap_writel(sadc, sadc_hit_value0, reg);

	value = hit_data->value[2];
	value = (value*SADC_MAX_AD_VALUE)/SADC_REF;
	value &= 0xfff;

	value1 = hit_data->value[3];
	value1 = (value1*SADC_MAX_AD_VALUE)/SADC_REF;
	value1 &= 0xfff;
	reg = value | (value1 << 16);

	wrap_writel(sadc, sadc_hit_value1, reg);

	value = hit_data->value[4];
	value = (value*SADC_MAX_AD_VALUE)/SADC_REF;
	value &= 0xfff;

	value1 = hit_data->value[5];
	value1 = (value1*SADC_MAX_AD_VALUE)/SADC_REF;
	value1 &= 0xfff;
	reg = value | (value1 << 16);

	wrap_writel(sadc, sadc_hit_value2, reg);

	value = hit_data->value[6];
	value = (value*SADC_MAX_AD_VALUE)/SADC_REF;
	value &= 0xfff;

	value1 = hit_data->value[7];
	value1 = (value1*SADC_MAX_AD_VALUE)/SADC_REF;
	value1 &= 0xfff;
	reg = value | (value1 << 16);

	wrap_writel(sadc, sadc_hit_value3, reg);

	reg = wrap_readl(sadc, sadc_ier);
	reg &= (~(0xff<<24));
	reg |= (hit_data->value_en << 24);
	wrap_writel(sadc, sadc_ier, reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_hit_data_config);
#endif

int fh_sadc_enable(void)
{
	u32 control_reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	control_reg = wrap_readl(sadc, sadc_control);
	control_reg |= 1 << 12;
	wrap_writel(sadc, sadc_control, control_reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_enable);

int fh_sadc_disable(void)
{
	u32 control_reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;

	control_reg = wrap_readl(sadc, sadc_control);
	control_reg &= ~(1 << 12);
	wrap_writel(sadc, sadc_control, control_reg);
	return 0;
}
EXPORT_SYMBOL(fh_sadc_disable);


u32 fh_sadc_default_config(void)
{
	u32 xversel = 0;
	u32 xpwdb = 1;
	/*cnt*/
	u32 sel2sam_pre_cnt = 2;
	u32 sam_cnt = 2;
	u32 sam2sel_pos_cnt = 2;
	/*time out*/
	u32 eoc_tos = 0xff;
	u32 eoc_toe = 0xff;
	u32 time_out = 0xffff;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	/*sadc act bit default 12bit*/
	fh_sadc_set_act_bit(12);
	/*control...*/
	wrap_writel(sadc, sadc_control, (xversel << 8) \
			| (xpwdb << 12));

	wrap_writel(sadc, sadc_cnt,
			sel2sam_pre_cnt | (sam_cnt << 8) | \
			(sam2sel_pos_cnt << 16));

	wrap_writel(sadc, sadc_timeout,
			eoc_tos | (eoc_toe << 8) | (time_out << 16));


	return 0;
}

static irqreturn_t fh_sadc_isr(int irq, void *dev_id)
{

	u32 isr_status;
	u32 temp_data =  0;
	u32 temp_data1 =  0;
	u32 channel = 0;
	u32 data = 0;
	u32 data1 = 0;
	struct wrap_sadc_obj *sadc = (struct wrap_sadc_obj *) dev_id;

	isr_status = wrap_readl(sadc, sadc_int_status);
	if (isr_status & 0x01) {

		u32 sadc_isr = 0x00;
		wrap_writel(sadc, sadc_ier, sadc_isr);

		wrap_writel(sadc, sadc_int_status, isr_status);
		complete(&(sadc->done));
		return IRQ_HANDLED;
	}
	if (isr_status & 0x10000) {
		channel = wrap_readl(sadc, sadc_status2);
		channel = ((channel>>12)&0x7);
		switch (channel/2) {
		case 0:
			/*read channel 0 1*/
			temp_data = wrap_readl(sadc, sadc_dout0);
			break;
		case 1:
			/*read channel 2 3*/
			temp_data = wrap_readl(sadc, sadc_dout1);
			break;
		case 2:
			/*read channel 4 5*/
			temp_data = wrap_readl(sadc, sadc_dout2);
			break;
		case 3:
			/*read channel 6 7*/
			temp_data = wrap_readl(sadc, sadc_dout3);
			break;
		default:
			break;
		}

		if (channel % 2)
			/*read low 16bit*/
			data = (temp_data & 0xfff);
		else
			/*read high 16bit*/
			data = (temp_data >> 16) & 0xfff;
		data = (data*SADC_REF)/SADC_MAX_AD_VALUE;
		if (g_sadc_debug)
			printk(KERN_ERR "channel:%x data:%dmv\n",
			channel, data);
		if (waitqueue_active(&fh_sadc_obj.readqueue)) {
			if (g_sadc_data[channel].flag) {
				g_sadc_data[channel].usrdata.sadc_data = data;
				g_sadc_data[channel].flag = 0;
				wake_up(&fh_sadc_obj.readqueue);
			}
		}
		wrap_writel(sadc, sadc_int_status, 0x10000);
	}
	if (isr_status & 0x100000) {
		channel = wrap_readl(sadc, sadc_status2);
		channel = ((channel>>12)&0x7);
		switch (channel/2) {
		case 0:
			/*read channel 0 1*/
			temp_data1 = wrap_readl(sadc, sadc_dout0_all);
			break;
		case 1:
			/*read channel 2 3*/
			temp_data1 = wrap_readl(sadc, sadc_dout1_all);
			break;
		case 2:
			/*read channel 4 5*/
			temp_data1 = wrap_readl(sadc, sadc_dout2_all);
			break;
		case 3:
			/*read channel 6 7*/
			temp_data1 = wrap_readl(sadc, sadc_dout3_all);
			break;
		default:
			break;
		}

		if (channel % 2)
			/*read low 16bit*/
			data1 = (temp_data1 & 0xfff);
		else
			/*read high 16bit*/
			data1 = (temp_data1 >> 16) & 0xfff;
		data1 = (data1*SADC_REF)/SADC_MAX_AD_VALUE;
		if (g_sadc_debug) {
			printk(KERN_ERR "glitch :%x data:%dmv\n",
				channel, data1);
			wrap_writel(sadc, sadc_int_status, 0x100000);
		}
	}
#ifdef CONFIG_FH_SADC_V22
	if (isr_status & 0xff000000) {
		channel = wrap_readl(sadc, sadc_status2);
		channel = ((channel>>12)&0x7);
		switch (channel) {
		case 0:
		case 1:
			/*read channel 0 1*/
			temp_data = wrap_readl(sadc, sadc_hit_data0);
			break;

		case 2:
		case 3:
			/*read channel 2 3*/
			temp_data = wrap_readl(sadc, sadc_hit_data1);
			break;

		case 4:
		case 5:
			/*read channel 4 5*/
			temp_data = wrap_readl(sadc, sadc_hit_data2);
			break;

		case 6:
		case 7:
			/*read channel 6 7*/
			temp_data = wrap_readl(sadc, sadc_hit_data3);
			break;
		default:
			break;
		}

		if (channel % 2) {
			/*read low 16bit*/
			data = (temp_data & 0xfff);
		} else {
			/*read high 16bit*/
			data = (temp_data >> 16) & 0xfff;
		}

		data = (data*SADC_REF)/SADC_MAX_AD_VALUE;
		if (g_sadc_debug)
			printk(KERN_ERR "hit value channel:%x data:%dmv isr_status:%x\n",
				channel, data, isr_status);
		if (waitqueue_active(&fh_sadc_obj.readqueue)) {
			if (g_sadc_data[channel].flag) {
				g_sadc_data[channel].usrdata.sadc_data = data;
				g_sadc_data[channel].flag = 0;
				wake_up(&fh_sadc_obj.readqueue);
			}
		}
		wrap_writel(sadc, sadc_int_status, (isr_status & 0xff000000));

	}
#endif
	return IRQ_HANDLED;
}

long fh_sadc_get_value(int channel)
{
	unsigned int ret;
	long w = 0;
	u16 ad_raw_data;
	if (channel < 0) {
		printk(KERN_ERR "ERROR: %s, sadc channel no %d is incorrect\n",
				__func__, channel);
		return 0;
	}

	fh_sadc_enable();

	ret = fh_sadc_isr_read_data(&fh_sadc_obj, channel, &ad_raw_data);

	if (ret != 0) {
		printk(KERN_INFO "sadc error code:0x%x\n", ret);
	} else {
		w = ad_raw_data * SADC_REF / SADC_MAX_AD_VALUE;
		printk(KERN_INFO "the value of sadc is: %ld\n", w);
	}

	return w;
}
EXPORT_SYMBOL(fh_sadc_get_value);

static void del_char(char *str, char ch)
{
	char *p = str;
	char *q = str;
	while (*q) {
		if (*q != ch)
			*p++ = *q;
		q++;
	}
	*p = '\0';
}

static ssize_t fh_sadc_proc_write(struct file *filp, const char *buf,
	 size_t len, loff_t *off)
{
	char message[32] = {0};
	char * const delim = ",";
	char *cur = message;
	int i;
	char *param_str[2];
	int reg;
	int ret;
	unsigned long param = 0;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	len = (len > 32) ? 32 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for (i = 0; i < 2; i++) {
		param_str[i] = strsep(&cur, delim);
		if (!param_str[i]) {
			pr_err("%s: ERROR: parameter[%d] is empty\n",
				__func__, i);
			pr_err("[debug],[channel],[pwr_down]\n");
			return -EINVAL;
		} else {
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
		}
	}
	if (!strcmp(param_str[0], "debug")) {
		ret = (u32)kstrtoul(param_str[1], 10, &param);
		spin_lock_irq(&fh_sadc_obj.lock);
		if (param) {
			g_sadc_debug = 1;
			reg = wrap_readl(sadc, sadc_ier);
			reg |= 1<<20;
			wrap_writel(sadc, sadc_ier, reg);
		} else {
			g_sadc_debug = 0;
			reg = wrap_readl(sadc, sadc_ier);
			reg &= (~(1<<20));
			wrap_writel(sadc, sadc_ier, reg);
		}
		spin_unlock_irq(&fh_sadc_obj.lock);
	}

	if (!strcmp(param_str[0], "pwr_down")) {
		ret = (u32)kstrtoul(param_str[1], 10, &param);
		param ? fh_sadc_enable() : fh_sadc_disable();
	}

	return len;
}

static void *v_seq_start(struct seq_file *s, loff_t *pos)
{
	static unsigned long counter;
	counter = 0;
	if (*pos == 0)
		return &counter;
	else {
		*pos = 0;
		return NULL;
	}
}

static void *v_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
}

static void v_seq_stop(struct seq_file *s, void *v)
{

}

static int v_seq_show(struct seq_file *sfile, void *v)
{
	int i;
	u32 reg;
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	u32 ret;

	reg = (wrap_readl(sadc, sadc_control) & 0x1000);

	seq_printf(sfile, "\nSADC Status:\n");
	seq_printf(sfile, "Power %s\n\n", reg ? "up" : "down");

	for (i = 0; i < 8; i++) {
		u16 ad_raw_data;
		ret = fh_sadc_isr_read_data(&fh_sadc_obj, i, &ad_raw_data);
		if (ret != 0)
			seq_printf(sfile, "sadc error code:0x%x\n", ret);
		else
			seq_printf(sfile, "channel: %d \tvalue: %u\n", i,
		ad_raw_data * SADC_REF / SADC_MAX_AD_VALUE);
	}
	return 0;
}

static const struct seq_operations isp_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_sadc_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &isp_seq_ops);
}

static const struct file_operations fh_sadc_proc_ops = {
	.owner = THIS_MODULE,
	.open = fh_sadc_proc_open,
	.read = seq_read,
	.write = fh_sadc_proc_write,
	.release = seq_release,
};

static int __devinit fh_sadc_probe(struct platform_device *pdev)
{
	int err;
	struct resource *res;
	struct clk  *sadc_clk;

	sadc_clk = clk_get(&pdev->dev, "sadc_clk");
	if (IS_ERR(sadc_clk)) {
		err = PTR_ERR(sadc_clk);
		return -EPERM;
	}
	clk_enable(sadc_clk);

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "sadc get platform source error..\n");
		return -ENODEV;
	}

	fh_sadc_obj.irq_no = platform_get_irq(pdev, 0);
	if (fh_sadc_obj.irq_no < 0) {
		dev_warn(&pdev->dev, "sadc interrupt is not available.\n");
		return fh_sadc_obj.irq_no;
	}

	res = request_mem_region(res->start, resource_size(res), pdev->name);
	if (res == NULL) {
		dev_err(&pdev->dev, "sadc region already claimed\n");
		return -EBUSY;
	}

	fh_sadc_obj.regs = ioremap(res->start, resource_size(res));
	if (fh_sadc_obj.regs == NULL) {
		err = -ENXIO;
		goto fail_no_ioremap;
	}

	init_completion(&fh_sadc_obj.done);
	init_waitqueue_head(&fh_sadc_obj.readqueue);
	mutex_init(&fh_sadc_obj.sadc_lock);
	fh_sadc_obj.active_channel_no = 0;

	err = request_irq(fh_sadc_obj.irq_no, fh_sadc_isr, 0,\
			dev_name(&pdev->dev),\
			&fh_sadc_obj);
	if (err) {
		dev_dbg(&pdev->dev, "request_irq failed, %d\n", err);
		err = -ENXIO;
		goto err_irq;
	}

	err = misc_register(&fh_sadc_misc);

	if (err < 0) {
		pr_err("%s: ERROR: %s registration failed", __func__,
				FH_SADC_MISC_DEVICE_NAME);
		err = -ENXIO;
		goto misc_error;
	}

	fh_sadc_obj.proc_file = create_proc_entry(FH_SADC_PROC_FILE,
		 0644, NULL);

	if (fh_sadc_obj.proc_file)
		fh_sadc_obj.proc_file->proc_fops = &fh_sadc_proc_ops;
	else
		pr_err("%s: ERROR: %s proc file create failed",
		__func__, "SADC");

	return 0;

misc_error:
	free_irq(fh_sadc_obj.irq_no, &fh_sadc_obj);

err_irq:
	iounmap(fh_sadc_obj.regs);

fail_no_ioremap:
	release_mem_region(res->start, resource_size(res));

	return err;
}

static int __exit fh_sadc_remove(struct platform_device *pdev)
{

	struct resource *res;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	misc_deregister(&fh_sadc_misc);
	free_irq(fh_sadc_obj.irq_no, &fh_sadc_obj);
	iounmap(fh_sadc_obj.regs);
	release_mem_region(res->start, resource_size(res));
	return 0;

}


static int fh_sadc_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_sadc_release(struct inode *inode, struct file *filp)
{
	return 0;
}
static int fh_sadc_read(struct file *filp, char __user *buf,
		size_t len, loff_t *off)
{
	int ret;
	struct sadc_info sadc_data;

	ret = copy_from_user((void *) &sadc_data,
			(void __user *) buf, sizeof(struct sadc_info));

	spin_lock_irq(&fh_sadc_obj.lock);
	g_sadc_data[sadc_data.channel].usrdata.channel = sadc_data.channel;
	g_sadc_data[sadc_data.channel].flag = 1;
	spin_unlock_irq(&fh_sadc_obj.lock);

	/* wait for enough data*/
	if (!wait_event_timeout(fh_sadc_obj.readqueue,
		!g_sadc_data[sadc_data.channel].flag, 100)) {
		pr_err("%s: channel:%d read time out",
		__func__, sadc_data.channel);
		return -1;
	}
	spin_lock_irq(&fh_sadc_obj.lock);
	ret = copy_to_user((void __user *)buf,
		(void *)&g_sadc_data[sadc_data.channel].usrdata,
		sizeof(struct sadc_info));
	spin_unlock_irq(&fh_sadc_obj.lock);
	return len;
}

static long fh_sadc_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg) {

	u32 ad_data;
	u32 control_reg;
	u16 ad_raw_data;
	struct sadc_info sadc_info;
	struct sadc_continue_cfg sadc_continue_cfg;
#ifdef CONFIG_FH_SADC_V22
	struct sadc_hit_data_cfg sadc_hit_data_cfg;
#endif
	struct wrap_sadc_obj *sadc = &fh_sadc_obj;
	mutex_lock(&sadc->sadc_lock);
	switch (cmd) {
	case IOCTL_GET_SINGLE_SADC_DATA:
	{
		if (copy_from_user((void *) &sadc_info, (void __user*) arg,
				sizeof(sadc_info))) {
			mutex_unlock(&sadc->sadc_lock);
			return -EFAULT;
		}
		if ((wrap_readl(sadc, sadc_status2) & 0x10)) {
				mutex_unlock(&sadc->sadc_lock);
				return -EBUSY;
			}
		fh_sadc_mode_set(0);

		fh_sadc_isr_read_data(&fh_sadc_obj, sadc_info.channel,\
			&ad_raw_data);
		ad_data = ad_raw_data * SADC_REF;
		ad_data /= SADC_MAX_AD_VALUE;
		sadc_info.sadc_data = ad_data;
		if (put_user(sadc_info.sadc_data,
				(int __user *)(&((struct sadc_info *)arg)\
				->sadc_data))) {
			mutex_unlock(&sadc->sadc_lock);
			return -EFAULT;
		}
		break;
	}
	case IOCTL_SADC_POWER_DOWN:
		control_reg = wrap_readl(sadc, sadc_control);
		control_reg &= ~(1 << 12);
		wrap_writel(sadc, sadc_control, control_reg);
		break;
	case IOCTL_CONTINUE_SADC_CFG:
	{
		if (copy_from_user((void *) &sadc_continue_cfg,
			(void __user *) arg, sizeof(sadc_continue_cfg))) {
			mutex_unlock(&sadc->sadc_lock);
			return -EFAULT;
		}
		/*config  continue model*/
		fh_sadc_mode_set(1);

		/*config  channel cfg */
		control_reg = wrap_readl(sadc, sadc_chn_cfg);
		control_reg |= (sadc_continue_cfg.channel_cfg & 0x7ffffff);
		wrap_writel(sadc, sadc_chn_cfg, control_reg);

		/*config continue time */
		fh_sadc_set_continuous_time(sadc_continue_cfg.continue_time);

		fh_sadc_glitch_en_set(1);

		/*config glitch time */
		fh_sadc_set_glitch_time(sadc_continue_cfg.glitch_time);

		/*config scan delta */
		fh_sadc_scan_delta(sadc_continue_cfg.glitch_value);

		/*config zero value */
		fh_sadc_set_zero_value(sadc_continue_cfg.zero_value);

		/*config precision*/
		if (sadc_continue_cfg.precision)
			fh_sadc_set_act_bit(sadc_continue_cfg.precision);

		/*config eq flag*/
		fh_sadc_cons_ch_eq_set(sadc_continue_cfg.eq_flag);

		/*default config*/
		fh_sadc_default_config();

		/*glitch isr enable*/
		wrap_writel(sadc, sadc_ier, 1<<16);
		break;
	}
	case IOCTL_CONTINUE_SADC_START:
		fh_sadc_scan_start();
		break;
	case IOCTL_CONTINUE_SADC_END:
		fh_sadc_scan_end();
		break;
#ifdef CONFIG_FH_SADC_V22
	case IOCTL_HIT_SADC_CFG:
		if (copy_from_user((void *) &sadc_hit_data_cfg,
			(void __user *) arg, sizeof(sadc_hit_data_cfg))) {
			mutex_unlock(&sadc->sadc_lock);
			return -EFAULT;
		}
		fh_sadc_hit_data_config(&sadc_hit_data_cfg);
		break;
#endif
	}
	mutex_unlock(&sadc->sadc_lock);
	return 0;
}


/*******************
 *
 *
 *add platform cause of i need the board info...
 *in the probe function. i will register the sadc
 *misc drive...then the app can open the sadc misc device..
 *
 ******************/
static struct platform_driver fh_sadc_driver = {
	.driver	= {
		.name	= FH_SADC_PLAT_DEVICE_NAME,
		.owner	= THIS_MODULE,
	},
	.probe	= fh_sadc_probe,
	.remove	= __exit_p(fh_sadc_remove),
};



static int __init fh_sadc_init(void)
{
	return platform_driver_register(&fh_sadc_driver);
}

static void __exit fh_sadc_exit(void)
{

	platform_driver_unregister(&fh_sadc_driver);

}

module_init(fh_sadc_init);
module_exit(fh_sadc_exit);

MODULE_DESCRIPTION("fh sadc driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("tangyh@fullhan.com");
MODULE_ALIAS("platform:FH_sadc");
