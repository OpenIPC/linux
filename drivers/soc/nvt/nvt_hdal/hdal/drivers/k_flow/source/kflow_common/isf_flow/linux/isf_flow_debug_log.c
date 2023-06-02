#include "../isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
#define __MODULE__          isf_flow_d2
#define __DBGLVL__          8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__          "*" //*=All, [mark]=CustomClass
#include "kwrap/debug.h"
unsigned int isf_flow_d2_debug_level = NVT_DBG_WRN;
module_param_named(isf_flow_d2_debug_level, isf_flow_d2_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(isf_flow_d2_debug_level, "flow debug2 level");
///////////////////////////////////////////////////////////////////////////////

#include <kwrap/file.h>

#include <linux/wait.h>
#include <linux/seq_file.h>

static wait_queue_head_t log_wq;
static wait_queue_head_t log_wq2;
static int log_init = 0;
static int log_begin = 0;
static int log_end = 0;
static UINT32 log_unit = 0;
static struct seq_file *log_sfile = 0;

int debug_log_init(void)
{
	log_begin = 0;
	log_end = 0;
	log_unit = 0;
	log_sfile = 0;
	init_waitqueue_head(&log_wq);
	init_waitqueue_head(&log_wq2);
	log_init = 1;
	return 1;
}
int debug_log_exit(void)
{
	if(!log_init)
		return 0;
	//force to quit
	//debug_log_cb(0, 0xffffffff);
	log_sfile = 0;
	log_begin = 0xffffffff;
	wake_up_interruptible(&log_wq);
	log_init = 0;
	return 1;
}

void debug_log_cb(struct seq_file *sfile, UINT32 uid)
{
	if(!log_init) {
		return;
	}
	log_sfile = sfile;
	log_begin = uid;
	wake_up_interruptible(&log_wq);
	///.....
	wait_event_interruptible(log_wq2, (log_end!=0));
	log_sfile = 0;
	log_end = 0;
}
EXPORT_SYMBOL(debug_log_cb);
int debug_log_wait(void)
{
	if(!log_init) {
		return 0xffffffff;
	}
	wait_event_interruptible(log_wq, (log_begin!=0));
	log_unit = log_begin;
	log_begin = 0;
	return log_unit;
}
void debug_log_output(char *string)
{
	if(!log_init) {
		return;
	}
	if (log_sfile && (string!=NULL)) {
		seq_printf(log_sfile, "%s", string);
	}
}
void debug_log_sig(int uid)
{
	if(!log_init) {
		return;
	}
	log_end = uid;
	wake_up_interruptible(&log_wq2);
	log_unit = 0;
}


void debug_dump_current_stack(void)
{
	dump_stack();
}

