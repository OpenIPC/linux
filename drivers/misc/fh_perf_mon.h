#ifndef __FH_PERF_MON_H
#define __FH_PERF_MON_H

#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/scatterlist.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/workqueue.h>
#include <linux/bug.h>
#include <linux/completion.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>

#define REG_PERF_MONI_REG(id) (fh_perf_mon_obj.regs+4*(id))



#define FH_PERF_MON_IOCTL_BASE		'P'
#define FH_PERF_MON_START_ASYNC		_IOWR(FH_PERF_MON_IOCTL_BASE, 0, int)
#define FH_PERF_MON_SETPARAM		_IOWR(FH_PERF_MON_IOCTL_BASE, 1, int)
#define FH_PERF_MON_START_SYNC		_IOWR(FH_PERF_MON_IOCTL_BASE, 2, int)
#define FH_PERF_MON_GETLASTDATA		_IOWR(FH_PERF_MON_IOCTL_BASE, 3, int)
#define FH_PERF_MON_STOP			_IOWR(FH_PERF_MON_IOCTL_BASE, 4, int)
#define FH_PERF_SET_RAWDATA_BUFF	_IOWR(FH_PERF_MON_IOCTL_BASE, 5, int)



#define FH_PERF_MON_DEVNAME "fh_perf"
#define FH_PERF_MON_PROC_FILE    "driver/fh_perf"
#define FH_PERF_MON_PLAT_DEVICE_NAME		"fh_perf_mon"
#define FH_PERF_MON_MISC_DEVICE_NAME "fh_perf"

struct fh_perf_mon_obj_t {
	void *regs;
	u32 irq_no;
	spinlock_t lock;
	struct mutex		perf_lock;
	struct completion done;
	struct proc_dir_entry *proc_file;
};

enum fh_perf_mode_e
{
	FH_PERF_SINGLE,
	FH_PERF_CONTINUOUS,
};

struct fh_perf_adv_port_param
{
	u32 used;
	u32 filter;
	u32 mask;
};

struct fh_perf_data_buff
{
	u32 addr;
	u32 cnt;
};



struct fh_perf_param_input
{
	enum fh_perf_mode_e mode;
	u32 ddr_bw;
	u32 window_time;
	u32 axi_bw[10];
	struct fh_perf_adv_port_param addr_param[10];
	struct fh_perf_adv_port_param id_param[10];

};

struct fh_perf_param_output
{
	u32 serial_cnt;
	u32 hw_cnt;
	u32 wr_ot[10];
	u32 rd_ot[10];
	u32 wr_cmd_cnt[10];
	u32 rd_cmd_cnt[10];
	u32 wr_cmd_byte[10];
	u32 rd_cmd_byte[10];
	u32 wr_sum_lat[10];
	u32 rd_sum_lat[10];
	u32 wr_cmd_cnt_lat[10];
	u32 rd_cmd_cnt_lat[10];
	u32 ddr_wr_bw;
	u32 ddr_rd_bw;
};













#endif
