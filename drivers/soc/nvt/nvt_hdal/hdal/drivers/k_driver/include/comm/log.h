/*
 *   @file   log.h
 *
 *   @brief  log header file.
 *
 *   Here defines some APIs for log purpose.
 *
 *   Copyright   Novatek Microelectronics Corp. 2018.  All rights reserved.
 */

#ifndef _LOG_H_
#define _LOG_H_

//#define MEM_LEAK_TRACE
#ifdef MEM_LEAK_TRACE
#ifndef _LOG_C_
#include "dbg_wrapper.h"
#endif
#endif

#define MAX_STRING_LEN  256

#define IOCTL_PRINTM                0x9969
#define IOCTL_PRINTM_WITH_PANIC     0x9970
#define IOCTL_SET_HDAL_VERSION      0x9971
#define IOCTL_SET_IMPL_VERSION      0x9972

typedef struct {
	unsigned int ddr_id;
	unsigned int pa;
	unsigned int size;
	char filename[50];
} dump_info_t;

int register_panic_notifier(int (*func)(int));
int register_printout_notifier(int (*func)(int));
int register_master_print_notifier(int (*func)(int));
int register_hdal_proc_notifier(int (*func)(int));
int damnit(char *module);
void printm(char *module, const char *fmt, ...);
void master_print(const char *fmt, ...);
void dumpbuf_pa(dump_info_t *dump_info, unsigned int counts, char *path);
void dumpbuf_va(int ddr_id, unsigned int va, unsigned int size, char *filename, char *path);
void register_version(char *);
int unregister_printout_notifier(int (*func)(int));
int unregister_panic_notifier(int (*func)(int));
int dumplog(char *module);
int get_cpu_state(void);
void *get_hdal_proc_dir_entry(void); //have to case to (struct proc_dir_entry *)

#if defined(__FREERTOS)
int log_init(void *p_param)  __attribute__ ((section (".init.table.")));
int log_exit(void *p_param)  __attribute__ ((section (".exit.table.")));
void *log_mmap(void *p_addr, unsigned int len, int prot, int flags, int fd, unsigned int offset);
int log_munmap(void *p_addr, unsigned int len);
int log_ioctl(int fd, unsigned int cmd, void *p_arg);
#endif

#define VG_ASSERT(condition)\
	do{\
		if(unlikely(0 == (condition))){\
			printk("%s %d:assert error!\n", __FUNCTION__, __LINE__);\
			damnit("VG");\
		}\
	}while(0)


#endif
