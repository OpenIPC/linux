/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_LOGDEBUG_AX__H__
#define __OSAL_LOGDEBUG_AX__H__

#ifdef __cplusplus
extern "C" {
#endif

#include "osal_type_ax.h"
#include "osal_list_ax.h"

#define AX_LOG_LVL_ERROR			   3
#define AX_LOG_LVL_WARNING 			   4
#define AX_LOG_LVL_INFO				   6
#define AX_LOG_LVL_DBG 				   7

//debug
typedef struct AX_PROC_DIR_ENTRY {
    char name[50];
    void *proc_dir_entry;
    int (*open)(struct AX_PROC_DIR_ENTRY *entry);
    int (*read)(struct AX_PROC_DIR_ENTRY *entry);
    int (*write)(struct AX_PROC_DIR_ENTRY *entry, const char *buf, int count, long long *);
    void *private_data;
    void *seqfile;
    struct AX_LIST_HEAD node;
    struct AX_PROC_DIR_ENTRY *parent;
} AX_PROC_DIR_ENTRY_T;

int AX_OSAL_DBG_printk(const char *fmt, ...);
void AX_OSAL_DBG_panic(const char *fmt, const char *fun,int line, const char *cond);
void AX_OSAL_DBG_LogOutput(int target,int level, const char *tag, const char *fmt, ...);

void AX_OSAL_DBG_ISPLogoutput(int level, const char *fmt,...);
void AX_OSAL_DBG_NPULogoutput(int level, const char *fmt,...);
void AX_OSAL_DBG_CMMLogoutput(int level, const char *fmt,...);
void AX_OSAL_DBG_POOLLogoutput(int level, const char *fmt,...);
void AX_OSAL_DBG_APPLogoutput(int level, const char *fmt,...);

int AX_OSAL_DBG_SetLogLevel(int level);
int AX_OSAL_DBG_SetLogTarget(int level);
int AX_OSAL_DBG_EnableTimestamp(int enable);

extern void rt_backtrace(void);
#define AX_OSAL_DBG_Warnon(condition) do { \
	if ((condition!=0)) {   \
			rt_kprintf("Badness in %s at %s:%d/n",__FUNCTION__, __FILE__, __LINE__); \
			rt_backtrace(); \
			} \
}while(0);


#define AX_OSAL_DBG_Bugon(condition) do { \
	void  (* PANIC_BACKTRACE_func) (void ); \
	if ((condition!=0)) {   \
			rt_kprintf("Badness in %s at %s:%d/n",__FUNCTION__, __FILE__, __LINE__); \
			PANIC_BACKTRACE_func = 0; \
			PANIC_BACKTRACE_func(); \
			} \
}while(0);

#define AX_OSAL_DBG_Assert(condition) do { \
	if ((condition==0)) {   \
			rt_assert_handler("Assert ",__FUNCTION__, __LINE__); \
			} \
}while(0);


int AX_OSAL_DBG_EnableTraceEvent(int module,int enable);


AX_PROC_DIR_ENTRY_T *AX_OSAL_DBG_create_proc_entry(const char *name, AX_PROC_DIR_ENTRY_T *parent);
AX_PROC_DIR_ENTRY_T *AX_OSAL_DBG_proc_mkdir(const char *name, AX_PROC_DIR_ENTRY_T *parent);
void AX_OSAL_DBG_remove_proc_entry(const char *name, AX_PROC_DIR_ENTRY_T *parent);
void AX_OSAL_DBG_seq_printf(AX_PROC_DIR_ENTRY_T *entry, const char *fmt, ...);

#ifdef __cplusplus
}
#endif

#endif /*__OSAL_LOGDEBUG_AX__H__*/
