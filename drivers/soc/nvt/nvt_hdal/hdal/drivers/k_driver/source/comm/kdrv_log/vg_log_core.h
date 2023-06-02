#ifndef _VG_LOG_CORE_H
#define _VG_LOG_CORE_H

#ifndef SZ_1K
#define SZ_1K				0x00000400
#define SZ_2K				0x00000800
#define SZ_4K				0x00001000
#define SZ_8K				0x00002000
#define SZ_16K				0x00004000
#define SZ_32K				0x00008000
#define SZ_64K				0x00010000
#define SZ_128K             0x00020000
#endif

/* need to sync the value in gmlib_proc.c */
#define SETTING_MSG_SIZE   (SZ_1K + SZ_1K)
#define FLOW_MSG_SIZE      SZ_1K
#define ERR_MSG_SIZE       SZ_1K
#define HDAL_SETTING_MSG_SIZE   (SZ_1K + SZ_1K)
#define HDAL_FLOW_MSG_SIZE SZ_128K
#define MSG_LENGTH_SIZE    8
#define MSG_OFFSET_SIZE    8
#define MMAP_MSG_LEN (SETTING_MSG_SIZE + FLOW_MSG_SIZE + ERR_MSG_SIZE + HDAL_SETTING_MSG_SIZE + HDAL_FLOW_MSG_SIZE)

#define MAX_PATH_WIDTH  60
#define MAX_CHAR 512
#define MAX_DUMP_FILE       10

#define MODE_STORAGE    0
#define MODE_CONSOLE    1
#define MODE_PRINT      2

extern unsigned int hdal_version;
extern unsigned int impl_version;
extern char *mmap_msg;
extern unsigned int log_base_start, log_base_start_pa, log_base_ddr;
extern unsigned int log_bsize;
extern unsigned int log_base_end;
extern unsigned int log_start_ptr;
extern unsigned int log_real_size;
extern unsigned int log_ksize;
extern unsigned int log_slice_ptr[MAX_DUMP_FILE], log_size[MAX_DUMP_FILE];
extern char log_path[MAX_DUMP_FILE][MAX_PATH_WIDTH];
extern int mode; //-1:disable  0:storage  1:dump console  2:direct print

unsigned int get_gm_jiffies(void);
void print_chars(char *log, int size);
int putlog_safe(char *log, int size);
void putlog(char *log, int size);
void calculate_log(unsigned int *log_start, unsigned int *log_size);
void prepare_dump_console(unsigned int log_print_ptr, unsigned int size);
void prepare_dump_storage(unsigned int log_print_ptr, unsigned int size);
int log_ioctl_p(unsigned int request, void *p_arg);
int log_init_mem(void);
int log_uninit_mem(void);
#endif
