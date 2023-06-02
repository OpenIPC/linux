/*
 * fh_sadc.c
 *
 *	  Created on: Jan 7, 2020
 *	  Author: fullhan
 */
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/irqdomain.h>

#include	<mach/pmu.h>
#include	<linux/sched.h>
#include	<mach/clock.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_platform.h>

#include	"fh_perf_mon.h"

#define AXI_PORT_MAX 5


struct fh_perf_mon_obj_t fh_perf_mon_obj;
struct fh_perf_param_output lastres;
struct clk  *fh_perf_clk;

static int fh_perf_mon_open(struct inode *inode, struct file *file);
static int fh_perf_mon_release(struct inode *inode, struct file *filp);
static long fh_perf_mon_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
static void ctl_monitor(int on);

static const struct file_operations fh_perf_mon_fops = {
	.owner = THIS_MODULE,
	.open = fh_perf_mon_open,
	.read = NULL,
	.release = fh_perf_mon_release,
	.unlocked_ioctl = fh_perf_mon_ioctl,
};

static struct miscdevice fh_perf_mon_misc = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = FH_PERF_MON_DEVNAME,
	.fops = &fh_perf_mon_fops,
};

enum fh_perf_state_e
{
	FH_PERF_STOP,
	FH_PERF_RUNING,
} fh_perf_state;

struct fh_perf_param_input last_input_param;
struct fh_perf_data_buff fh_databuff; 

static void update_lastresult(void)
{
	int i = 0;
	unsigned int regval = 0;

	lastres.serial_cnt++;
	// ddr_bw
	lastres.hw_cnt = readl(REG_PERF_MONI_REG(59))&0xff;

	for (i = 0; i< AXI_PORT_MAX; i++) {
		lastres.wr_cmd_cnt[i] = readl(REG_PERF_MONI_REG(17+i));
		lastres.rd_cmd_cnt[i] = readl(REG_PERF_MONI_REG(22+i));
		lastres.wr_cmd_byte[i] = readl(REG_PERF_MONI_REG(27+i));
		lastres.rd_cmd_byte[i] = readl(REG_PERF_MONI_REG(32+i));
		lastres.wr_sum_lat[i] = readl(REG_PERF_MONI_REG(37+i));
		lastres.rd_sum_lat[i] = readl(REG_PERF_MONI_REG(42+i));
		lastres.wr_cmd_cnt_lat[i] = readl(REG_PERF_MONI_REG(47+i));
		lastres.rd_cmd_cnt_lat[i] = readl(REG_PERF_MONI_REG(52+i));
		switch(i) {
		default:
			break;
		case 0:
			lastres.wr_ot[i] = (readl(REG_PERF_MONI_REG(15))>>0)&0x3f;
			lastres.rd_ot[i] = (readl(REG_PERF_MONI_REG(15))>>8)&0x3f;
			break;
		case 1:
			lastres.wr_ot[i] = (readl(REG_PERF_MONI_REG(15))>>16)&0x3f;
			lastres.rd_ot[i] = (readl(REG_PERF_MONI_REG(15))>>24)&0x3f;
			break;
		case 2:
			lastres.wr_ot[i] = (readl(REG_PERF_MONI_REG(16))>>0)&0x3f;
			lastres.rd_ot[i] = (readl(REG_PERF_MONI_REG(16))>>8)&0x3f;
			break;
		case 3:
			lastres.wr_ot[i] = (readl(REG_PERF_MONI_REG(16))>>16)&0x3f;
			lastres.rd_ot[i] = (readl(REG_PERF_MONI_REG(16))>>24)&0x3f;
			break;
		case 4:
			regval = readl(REG_PERF_MONI_REG(15));
			lastres.wr_ot[i] = ((regval>>6)&0x3)|((regval>>12)&(0x3<<2))|
					((regval>>18)&(0x3<<4));
			regval = readl(REG_PERF_MONI_REG(16));
			lastres.rd_ot[i] = ((regval>>6)&0x3)|((regval>>12)&(0x3<<2))|
					((regval>>18)&(0x3<<4));
			break;


		}
	}

	lastres.ddr_wr_bw = readl(REG_PERF_MONI_REG(57));
	lastres.ddr_rd_bw = readl(REG_PERF_MONI_REG(58));




	if (fh_databuff.addr != 0 && fh_databuff.cnt>0) {
		u32 idx = (lastres.serial_cnt - 1 ) % fh_databuff.cnt;
		memcpy((void *)(fh_databuff.addr+idx*sizeof(lastres)),&lastres,sizeof(lastres));
	}
}

static irqreturn_t fh_perf_mon_isr(int irq, void *dev_id)
{

	u32 reg;
	update_lastresult();
	if (last_input_param.mode == FH_PERF_SINGLE) {
		complete(&fh_perf_mon_obj.done);
		ctl_monitor(0);
	}
	reg = readl(REG_PERF_MONI_REG(0));
	reg |= BIT(30);
	writel(reg, REG_PERF_MONI_REG(0));
	reg &= ~BIT(30);
	writel(reg, REG_PERF_MONI_REG(0));


	return IRQ_HANDLED;
}
static void set_fh_perf_hw_param(struct fh_perf_param_input* input);
static void del_char(char* str, char ch)
{
	char *p = str;
	char *q = str;
	while (*q) {
		if (*q != ch) {
			*p++ = *q;
		}
		q++;
	}
	*p = '\0';
}

static ssize_t fh_perf_mon_proc_write(struct file *filp, const char *buf,
	 size_t len, loff_t *off)
{
	int i;
	char message[64] = {0};
	char * const delim = ",";
	char *cur = message;
	char *param_str[4];

	len = (len > 64) ? 64 : len;

	if (copy_from_user(message, buf, len))
		return -EFAULT;

	for (i = 0; i < 3; i++) {
		param_str[i] = strsep(&cur, delim);
		if (!param_str[i]) {
			pr_err("%s: ERROR: parameter[%d] is empty\n", __func__, i);
			pr_err("[window_time], [mode], [max_databuffcnt]\n");
			return -EINVAL;
		} else {
			del_char(param_str[i], ' ');
			del_char(param_str[i], '\n');
		}
	}



	
	
	last_input_param.window_time = (u32)simple_strtoul(param_str[0], NULL, 10);
	last_input_param.mode = (u32)simple_strtoul(param_str[1], NULL, 10);
	pr_err("last_input_param.window_timet %x\r\n",last_input_param.window_time);
	pr_err("last_input_param.mode %x\r\n",last_input_param.mode);

	//last_input_param.mode = FH_PERF_CONTINUOUS;
	last_input_param.ddr_bw = 1;
	for (i = 0; i < AXI_PORT_MAX; i++) {
		last_input_param.axi_bw[i]  = 1;
	}
	set_fh_perf_hw_param(&last_input_param);

	fh_databuff.cnt =  (u32)simple_strtoul(param_str[2], NULL, 10);
	pr_err("fh_databuff.cnt %x\r\n",fh_databuff.cnt);
	if (fh_databuff.addr != 0) {
		kfree((void*)fh_databuff.addr);
		fh_databuff.addr = 0;
	}
	if (fh_databuff.cnt !=0) {
		fh_databuff.addr = (u32)kzalloc(sizeof(lastres)*fh_databuff.cnt,GFP_KERNEL);
		
	} else {
		fh_databuff.addr = (u32)NULL;
	}
	pr_err("fh_databuff.addr %x\r\n",fh_databuff.addr);
	ctl_monitor(1);
	return len;
}

static void *v_seq_next(struct seq_file *s, void *v, loff_t *pos)
{
	(*pos)++;
	return NULL;
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

static void v_seq_stop(struct seq_file *s, void *v)
{
}

static void fh_perf_printdata(struct seq_file *sfile, struct fh_perf_param_output* printres)
{
	int i = 0;

	seq_printf(sfile, "last serial idx 0x%x\n", printres->serial_cnt);
	seq_printf(sfile, "hw idx 0x%x\n", printres->hw_cnt);
	seq_printf(sfile, "DDR WR BW %u (MB/s)\n", printres->ddr_wr_bw/(last_input_param.window_time*1000));
	seq_printf(sfile, "DDR RD BW %u (MB/s) \n", printres->ddr_rd_bw/(last_input_param.window_time*1000));

	seq_printf(sfile,"                ");
	for(i = 0; i< AXI_PORT_MAX; i++) {
		seq_printf(sfile, " AXI_PORT_%02d ", i);
	}
	seq_printf(sfile, "\n");
	
	seq_printf(sfile, "wr_ot         ");
	for(i = 0; i< AXI_PORT_MAX; i++) {
		seq_printf(sfile, "   %09u  ", printres->wr_ot[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "rd_ot         ");
	for(i = 0; i< AXI_PORT_MAX; i++) {
		seq_printf(sfile, "   %09u  ", printres->rd_ot[i]);
	}
	seq_printf(sfile, "\n");
	seq_printf(sfile, "wr_cmd_cnt    ");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->wr_cmd_cnt[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "rd_cmd_cnt    ");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->rd_cmd_cnt[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "wr_cmd_byte   ");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->wr_cmd_byte[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "rd_cmd_byte   ");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->rd_cmd_byte[i]);
	}
	seq_printf(sfile, "\n");
	
	seq_printf(sfile, "wr_cmd_bw(MB/s)");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ",
			printres->wr_cmd_byte[i]/(last_input_param.window_time*1000));
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "rd_cmd_bw(MB/s)");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ",
			printres->rd_cmd_byte[i]/(last_input_param.window_time*1000));
	}

	seq_printf(sfile, "\n");

	seq_printf(sfile, "wr_sum_lat    ");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->wr_sum_lat[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "rd_sum_lat    ");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->rd_sum_lat[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "wr_cmd_cnt_lat");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->wr_cmd_cnt_lat[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "rd_cmd_cnt_lat");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ", printres->rd_cmd_cnt_lat[i]);
	}
	seq_printf(sfile, "\n");
	
	seq_printf(sfile, "wr_av_lat(cycle)");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ",
			printres->wr_cmd_cnt_lat[i] == 0 ? 0 :
			printres->wr_sum_lat[i]/printres->wr_cmd_cnt_lat[i]);
	}
	seq_printf(sfile, "\n");

	seq_printf(sfile, "rd_av_lat(cycle)");
	for(i =0; i< AXI_PORT_MAX;i++) {
		seq_printf(sfile, "   %09u  ",
			printres->rd_cmd_cnt_lat[i] == 0 ? 0 :
			printres->rd_sum_lat[i]/printres->rd_cmd_cnt_lat[i]);
	}
	
	
	seq_printf(sfile, "\n");
}


static int v_seq_show(struct seq_file *sfile, void *v)
{
	int i = 0;

	ctl_monitor(0);
	if (last_input_param.window_time == 0) {
		seq_printf(sfile, "\nPlease run fh perf first!\n");
		return 0;
	}

	msleep(last_input_param.window_time>100?last_input_param.window_time:100);
	seq_printf(sfile, "\nLast FH Perf Monitor Status:\n");
	seq_printf(sfile, "window time %u ms\n", last_input_param.window_time);

	if (fh_databuff.addr == (u32)NULL) {
		fh_perf_printdata(sfile,&lastres);
	} else {
		struct fh_perf_param_output* cur =
				(struct fh_perf_param_output *)fh_databuff.addr;

		for (i = 0; i<fh_databuff.cnt;i++){
			fh_perf_printdata(sfile,cur);
			cur++;
		}
	}
	return 0;
}

static const struct seq_operations fh_perf_mon_seq_ops = {
	.start = v_seq_start,
	.next = v_seq_next,
	.stop = v_seq_stop,
	.show = v_seq_show
};

static int fh_perf_mon_proc_open(struct inode *inode, struct file *file)
{
	return seq_open(file, &fh_perf_mon_seq_ops);
}

static const struct file_operations fh_perf_mon_proc_ops = {
	.owner = THIS_MODULE,
	.open = fh_perf_mon_proc_open,
	.read = seq_read,
	.write = fh_perf_mon_proc_write,
	.release = seq_release,
};

static int __init fh_perf_mon_probe(struct platform_device *pdev)
{
	int err;
	struct resource *res;

#ifdef CONFIG_USE_OF
	struct device_node *np = pdev->dev.of_node;
	fh_perf_mon_obj.regs = of_iomap(np, 0);
	fh_perf_mon_obj.irq_no = irq_of_parse_and_map(np, 0);

#else
	fh_perf_mon_obj.irq_no = irq_create_mapping(NULL,
		platform_get_irq(pdev, 0));
	if (fh_perf_mon_obj.irq_no < 0) {
		dev_err(&pdev->dev, "fh perf interrupt is not available.\n");
		return fh_perf_mon_obj.irq_no;
	}

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "fh perf no resource\n");
		return -EBUSY;
	}

	fh_perf_mon_obj.regs = ioremap(res->start, resource_size(res));
#endif
	if (fh_perf_mon_obj.regs == NULL) {
		err = -ENXIO;
		goto fail_no_ioremap;
	}
	fh_perf_mon_obj.regs += 0x2018;

	err = request_irq(fh_perf_mon_obj.irq_no, fh_perf_mon_isr, 0,
			"fh_perf",
			&fh_perf_mon_obj);

	if (err) {
		dev_dbg(&pdev->dev, "request_irq failed, %d\n", err);
		err = -ENXIO;
		goto err_irq;
	}
	init_completion(&fh_perf_mon_obj.done);
	mutex_init(&fh_perf_mon_obj.perf_lock);

	err = misc_register(&fh_perf_mon_misc);

	if (err < 0) {
		pr_err("%s: ERROR: %s registration failed", __func__,
				FH_PERF_MON_MISC_DEVICE_NAME);
		err = -ENXIO;
		goto misc_error;
	}
	fh_perf_clk = clk_get(&pdev->dev, "ahb_clk");

	fh_perf_mon_obj.proc_file = proc_create(FH_PERF_MON_PROC_FILE,
		 0644, NULL, &fh_perf_mon_proc_ops);

	if (!fh_perf_mon_obj.proc_file)
		pr_err("%s: ERROR: %s proc file create failed",
		__func__, "PERF");

	return 0;

misc_error:
	free_irq(fh_perf_mon_obj.irq_no, &fh_perf_mon_obj);

err_irq:
	iounmap(fh_perf_mon_obj.regs);

fail_no_ioremap:
	release_mem_region(res->start, resource_size(res));

	return err;
}

static int __exit fh_perf_mon_remove(struct platform_device *pdev)
{

	struct resource *res;
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	misc_deregister(&fh_perf_mon_misc);
	free_irq(fh_perf_mon_obj.irq_no, &fh_perf_mon_obj);
	iounmap(fh_perf_mon_obj.regs);
	release_mem_region(res->start, resource_size(res));
	return 0;

}


static int fh_perf_mon_open(struct inode *inode, struct file *file)
{
	return 0;
}

static int fh_perf_mon_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static void set_fh_perf_hw_param(struct fh_perf_param_input* input)
{
	u32 reg;
	int i = 0;
	u32 rate;

	// window
	if (IS_ERR(fh_perf_clk)) {
		rate = 200000;
	} else {
		rate = clk_get_rate(fh_perf_clk)/1000;
	}
	reg = input->window_time*rate;
	writel(reg, REG_PERF_MONI_REG(1));

	// ddr bw
	reg = readl(REG_PERF_MONI_REG(0));
	if (input->ddr_bw) {
		reg |= BIT(24);
	} else {
		reg &= ~BIT(24);
	}
	// mode 
	if (input->mode == FH_PERF_SINGLE) {
		reg &= ~BIT(28);
	} else {
		reg |= BIT(28);
	}
	writel(reg, REG_PERF_MONI_REG(0));

	
	// axi addr filter
	for (i = 0;i < AXI_PORT_MAX; i++) {
		if (input->addr_param[i].used == 1) {
			reg = readl(REG_PERF_MONI_REG(0));
			reg |= BIT((8+i));
			writel(reg, REG_PERF_MONI_REG(0));
			writel(input->addr_param[i].filter, REG_PERF_MONI_REG(2+i));
			writel(input->addr_param[i].mask, REG_PERF_MONI_REG(7+i));
		} else {
			reg = readl(REG_PERF_MONI_REG(0));
			reg &= ~BIT((8+i));
			writel(reg, REG_PERF_MONI_REG(0));
			writel(0, REG_PERF_MONI_REG(2+i));
			writel(0, REG_PERF_MONI_REG(7+i));
		}
	}
	// axi id filter
	for (i = 0;i < AXI_PORT_MAX; i++) {
		if (input->id_param[i].used == 1) {
			reg = readl(REG_PERF_MONI_REG(0));
			reg |= BIT((16+i));
			writel(reg, REG_PERF_MONI_REG(0));
			switch (i) {
			case 0:
				reg = readl(REG_PERF_MONI_REG(12));
				reg &= 0xffff0000;
				reg |= input->id_param[i].filter|(input->id_param[i].mask<<8);
				writel(reg,  REG_PERF_MONI_REG(12));
				break;
			case 1:
				reg = readl(REG_PERF_MONI_REG(12));
				reg &= 0x0000ffff;
				reg |= (input->id_param[i].filter<<16)|(input->id_param[i].mask<<24);
				writel(reg,  REG_PERF_MONI_REG(12));
				break;
			case 2:
				reg = readl(REG_PERF_MONI_REG(13));
				reg &= 0xFFFF0000;
				reg |= (input->id_param[i].filter<<0)|(input->id_param[i].mask<<8);
				writel(reg,  REG_PERF_MONI_REG(13));
				break;
			case 3:
				reg = readl(REG_PERF_MONI_REG(13));
				reg &= 0x0000ffff;
				reg |= (input->id_param[i].filter<<16)|
					(input->id_param[i].mask<<24);
				writel(reg,  REG_PERF_MONI_REG(13));
				break;
			case 4:
				reg = readl(REG_PERF_MONI_REG(14));
				reg &= 0x0000ffff;
				reg |= (input->id_param[i].filter<<0)|
					(input->id_param[i].mask<<8);
				writel(reg,  REG_PERF_MONI_REG(14));
				break;
			default:
				break;
			}
		} else {
			reg = readl(REG_PERF_MONI_REG(0));
			reg &= ~BIT((16+i));
			writel(reg, REG_PERF_MONI_REG(0));
			switch (i) {
			case 0:
				reg = readl(REG_PERF_MONI_REG(12));
				reg &= 0xffff0000;
				writel(reg,  REG_PERF_MONI_REG(12));
				break;
			case 1:
				reg = readl(REG_PERF_MONI_REG(12));
				reg &= 0x0000ffff;
				writel(reg,  REG_PERF_MONI_REG(12));
				break;
			case 2:
				reg = readl(REG_PERF_MONI_REG(13));
				reg &= 0xFFFF0000;
				writel(reg,  REG_PERF_MONI_REG(13));
				break;
			case 3:
				reg = readl(REG_PERF_MONI_REG(13));
				reg &= 0x0000ffff;
				writel(reg,  REG_PERF_MONI_REG(13));
				break;
			case 4:
				reg = readl(REG_PERF_MONI_REG(14));
				reg &= 0xffff0000;
				writel(reg,  REG_PERF_MONI_REG(14));
				break;
			default:
				break;
			}
		}
	}
	// axi bw en
	for (i = 0;i < AXI_PORT_MAX; i++) {
		if (input->axi_bw[i] == 1) {
			reg = readl(REG_PERF_MONI_REG(0));
			reg |= BIT((i));
			writel(reg, REG_PERF_MONI_REG(0));
		} else {
			reg = readl(REG_PERF_MONI_REG(0));
			reg &= ~BIT((i));
			writel(reg, REG_PERF_MONI_REG(0));
		}
	}

}
static void ctl_monitor(int on)
{
	u32 reg;
	if (on) {
		lastres.serial_cnt = 0;
		fh_perf_state = FH_PERF_RUNING;
		if (fh_databuff.addr != 0 && fh_databuff.cnt>0) {
			memset((void *)fh_databuff.addr,0,sizeof(lastres)*fh_databuff.cnt);
		}
		reg = readl(REG_PERF_MONI_REG(0));
		reg |= BIT(31);
		writel(reg, REG_PERF_MONI_REG(0));
	} else {
		if (FH_PERF_RUNING == fh_perf_state)
			fh_perf_state = FH_PERF_STOP;
		reg = readl(REG_PERF_MONI_REG(0));
		reg &= ~BIT(31);
		writel(reg, REG_PERF_MONI_REG(0));
	}

}


static long fh_perf_mon_ioctl(struct file *filp, unsigned int cmd,
		unsigned long arg) 
{
	int ret;

	switch(cmd) {
	case FH_PERF_MON_START_ASYNC:
		if (last_input_param.mode != FH_PERF_CONTINUOUS) {
			return -EIO;
		}
		ctl_monitor(1);
		break;
	case FH_PERF_MON_STOP:
		ctl_monitor(0);
		break;
	case FH_PERF_MON_SETPARAM:
		memset(&last_input_param,0,sizeof(last_input_param));
		ret = copy_from_user((void *)&last_input_param, (void __user *)arg, sizeof(last_input_param));
		set_fh_perf_hw_param(&last_input_param);
		break;
	case FH_PERF_MON_START_SYNC:
		if (last_input_param.mode != FH_PERF_SINGLE) {
			return -EIO;
		}
		reinit_completion(&fh_perf_mon_obj.done);
		ctl_monitor(1);
		ret = wait_for_completion_interruptible_timeout(&fh_perf_mon_obj.done, last_input_param.window_time/10+1);
		ret = copy_to_user( (void __user *)arg,
			(void *)&lastres,sizeof(lastres));
		break;
	case FH_PERF_MON_GETLASTDATA:
		ret = copy_to_user( (void __user *)arg,
			(void *)&lastres,sizeof(lastres));
		break;
	case FH_PERF_SET_RAWDATA_BUFF:
		ret = copy_from_user((void *)&fh_databuff,
			(void __user *)arg, sizeof(last_input_param));
		fh_databuff.addr = 
			(u32)ioremap(fh_databuff.addr,
				fh_databuff.cnt*sizeof(lastres));
		break;
	default:
		break;
	}
	return 0;
}


static const struct of_device_id fh_perf_of_match[] = {
	{.compatible = "fh,fh-perf",},
	{},
};

static struct platform_driver fh_perf_mon_driver = {
	.driver	= {
		.name	= FH_PERF_MON_PLAT_DEVICE_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = fh_perf_of_match,
	},
	.probe	= fh_perf_mon_probe,
	.remove	= __exit_p(fh_perf_mon_remove),
};





static int __init fh_perf_mon_init(void)
{
	return platform_driver_register(&fh_perf_mon_driver);
}

static void __exit fh_perf_mon_exit(void)
{

	platform_driver_unregister(&fh_perf_mon_driver);

}

module_init(fh_perf_mon_init);
module_exit(fh_perf_mon_exit);

MODULE_DESCRIPTION("fh perf_mon driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("FULLHAN");
MODULE_ALIAS("platform:FH_perf_mon");
