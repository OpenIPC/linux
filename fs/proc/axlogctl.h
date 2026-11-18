#ifndef __AXLOGCTL_H__
#define __AXLOGCTL_H__

#define AX_MOD_NAME_LEN     12
#define AX_MAX_LINE         128
#define AX_MAX_ID           255

#define PROC_NAME        "ax_proc/logctl"
#define LOGCTL_DEV       "/proc/"PROC_NAME

#define LOG_TYPE_USER    "ulog"
#define LOG_TYPE_KERNEL  "klog"

#define CFG_ULOG_STATE    "ulog_state"
#define CFG_ULOG_TARGET   "ulog_target"

#define CFG_KLOG_STATE    "klog_state"

#define CMD_ALL          "all"
#define CMD_LOG_ON       "on"
#define CMD_LOG_OFF      "off"

#define TARGET_CONSOLE   "console"
#define TARGET_FILE      "file"
#define TARGET_NULL      "null"

#define AX_LOGCTL_IOC_MAGIC 'L'
#define AX_LOGCTL_GET_MEM_CFG _IOWR(AX_LOGCTL_IOC_MAGIC, 0, axlogctl_mem_cfg_ptr)

typedef struct __axlogctl_mem_cfg{
	unsigned long phyaddr;    /*phy addr*/
	unsigned long kviraddr;   /*kernel start addr*/
	unsigned long klvladdr;   /*log level info addr - kernel*/
	unsigned long uviraddr;   /*user start addr*/
	unsigned long ulvladdr;   /*log level info addr - user*/
	unsigned long mem_size;   /*total mem size*/
	unsigned char ulog_state; /*user log state 0: off 1: on*/
	unsigned char klog_state; /*kernel log state 0: off 1: on*/
	unsigned char level_min;  /*minimal log level*/
	unsigned char level_max;  /*maximal log level*/
	unsigned char ulog_target;/*user log target: file or console*/
	unsigned char max_id;     /*maximal id of log level info*/
	char config[AX_MAX_LINE]; /*log level config file path*/
}axlogctl_mem_cfg_s, *axlogctl_mem_cfg_ptr;

typedef struct axlog_lvl_info{
	char name[AX_MOD_NAME_LEN];
	unsigned char id;
	unsigned char level;  /*user log level*/
	unsigned char klevel; /*kernel log level*/
}axlog_lvl_info_s, *axlog_lvl_info_ptr;

#endif //__AXLOGCTL_H__
