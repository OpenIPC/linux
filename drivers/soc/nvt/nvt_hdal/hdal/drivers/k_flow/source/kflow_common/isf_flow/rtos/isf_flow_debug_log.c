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

//static int g_init = 0;
static ID FLG_ID_WQ = 0; //wait user CB begin
static ID FLG_ID_WQ2 = 0; //wait user CB end
static int log_init = 0;
static int log_begin = 0;
static int log_end = 0;
static UINT32 log_unit = 0;
//static struct seq_file *log_sfile = 0;

#define FLG_ID_WQ_ALL		0xFFFFFFFF
#define FLG_ID_WQ_BEGIN	FLGPTN_BIT(0)
#define FLG_ID_WQ_END		FLGPTN_BIT(1)
//#define FLG_ID_WQ_SIGNAL  	FLGPTN_BIT(31)

int debug_log_init(void)
{
	log_begin = 0;
	log_end = 0;
	log_unit = 0;
	//log_sfile = 0;
	//if (g_init == 0) {
		OS_CONFIG_FLAG(FLG_ID_WQ);
		OS_CONFIG_FLAG(FLG_ID_WQ2);
	//}
	clr_flg(FLG_ID_WQ, FLG_ID_WQ_ALL);
	clr_flg(FLG_ID_WQ2, FLG_ID_WQ_ALL);
	log_init = 1;
	return 1;
}
int debug_log_exit(void)
{
	if(!log_init)
		return 0;
	//force to quit
	//debug_log_cb(0, 0xffffffff);
	//log_sfile = 0;
	log_begin = 0xffffffff;
	//wake_up_interruptible(&log_wq);
	set_flg(FLG_ID_WQ, FLG_ID_WQ_BEGIN); //force quit from wait user CB begin?
	rel_flg(FLG_ID_WQ);
	rel_flg(FLG_ID_WQ2);
	log_init = 0;
	return 1;
}

void debug_log_cb(UINT32 uid)
{
	FLGPTN          flag = 0;
	if(!log_init) {
		return;
	}
	//log_sfile = sfile;
	log_begin = uid;
	//wake_up_interruptible(&log_wq);
	set_flg(FLG_ID_WQ, FLG_ID_WQ_BEGIN);
	///.....
	//wait_event_interruptible(log_wq2, (log_end!=0));
	wai_flg(&flag, FLG_ID_WQ2, FLG_ID_WQ_END, TWF_ORW|TWF_CLR);
	//log_sfile = 0;
	log_end = 0;
	//clr_flg(FLG_ID_WQ2, FLG_ID_WQ_END);
}
int debug_log_wait(void)
{
	FLGPTN          flag = 0;
	if(!log_init) {
		return 0xffffffff;
	}
	//wait_event_interruptible(log_wq, (log_begin!=0));
	wai_flg(&flag, FLG_ID_WQ, FLG_ID_WQ_BEGIN, TWF_ORW|TWF_CLR);
	log_unit = log_begin;
	log_begin = 0;
	//clr_flg(FLG_ID_WQ, FLG_ID_WQ_BEGIN);
	return log_unit;
}
void debug_log_output(char *string)
{
	if(!log_init) {
		return;
	}
	//if (log_sfile && (string!=NULL)) {
	if (string != NULL) {
		DBG_DUMP("%s", string);
	}
}
void debug_log_sig(int uid)
{
	if(!log_init) {
		return;
	}
	log_end = uid;
	//wake_up_interruptible(&log_wq2);
	set_flg(FLG_ID_WQ2, FLG_ID_WQ_END);
	log_unit = 0;
}


void debug_dump_current_stack(void)
{
}

