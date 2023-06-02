#ifndef __TSE_IOCTL_CMD_H_
#define __TSE_IOCTL_CMD_H_

#include <linux/ioctl.h>
#include <kwrap/type.h>

#if 0
#define MODULE_REG_LIST_NUM     10

typedef struct reg_info {
	unsigned int reg_addr;
	unsigned int reg_value;
} REG_INFO;

typedef struct reg_info_list {
	unsigned int reg_cnt;
	REG_INFO reg_list[MODULE_REG_LIST_NUM];
} REG_INFO_LIST;

//============================================================================
// IOCTL command
//============================================================================
#define XXX_IOC_COMMON_TYPE 'M'
#define XXX_IOC_START                   _IO(XXX_IOC_COMMON_TYPE, 1)
#define XXX_IOC_STOP                    _IO(XXX_IOC_COMMON_TYPE, 2)

#define XXX_IOC_READ_REG                _IOWR(XXX_IOC_COMMON_TYPE, 3, void*)
#define XXX_IOC_WRITE_REG               _IOWR(XXX_IOC_COMMON_TYPE, 4, void*)
#define XXX_IOC_READ_REG_LIST           _IOWR(XXX_IOC_COMMON_TYPE, 5, void*)
#define XXX_IOC_WRITE_REG_LIST          _IOWR(XXX_IOC_COMMON_TYPE, 6, void*)

#endif

typedef struct _TSE_IOC_OBJ {
	BOOL bWait;
	unsigned int OP_MODE;
} TSE_IOC_OBJ;

typedef struct _TSE_IOC_BUF {
	unsigned int addr;
	unsigned int size;
	struct _TSE_IOC_BUF *pnext;
} TSE_IOC_BUF;

typedef struct _TSE_IOC_MUX_CFG {
	unsigned int payload_size;
	TSE_IOC_BUF  src_info;
	TSE_IOC_BUF  dst_info;
	unsigned int muxing_len;
	unsigned int sync_byte;
	unsigned int continuity_cnt;
	unsigned int pid;
	unsigned int tei;
	unsigned int tp;
	unsigned int scramblectrl;
	unsigned int start_indictor;
	unsigned int stuff_val;
	unsigned int adapt_flags;
	unsigned int con_curr_cnt;
	unsigned int last_data_mux_mode;
} TSE_IOC_MUX_CFG;

typedef struct _TSE_IOC_MUX_OBJ {
	TSE_IOC_OBJ obj;
	TSE_IOC_MUX_CFG cfg;
} TSE_IOC_MUX_OBJ;

typedef struct _TSE_IOC_DEMUX_CFG {
	unsigned int sync_byte;
	unsigned int adaptation_flag;
	unsigned int pid_enable[3];
	unsigned int pid_value[3];
	unsigned int continuity_mode[3];
	unsigned int continuity_value[3];
	unsigned int continuity_last[3];
	TSE_IOC_BUF  in_info;
	TSE_IOC_BUF  out_info[3];
	unsigned int out_total_len[3];
} TSE_IOC_DEMUX_CFG;

typedef struct _TSE_IOC_DEMUX_OBJ {
	TSE_IOC_OBJ obj;
	TSE_IOC_DEMUX_CFG cfg;
} TSE_IOC_DEMUX_OBJ;

typedef struct _TSE_IOC_HWCPY_CFG {
	unsigned int id_cmd;
	unsigned int id_ctex;
	unsigned int src_addr;
	unsigned int dst_addr;
	unsigned int src_len;
	unsigned int total_len;
} TSE_IOC_HWCPY_CFG;

typedef struct _TSE_IOC_HWCPY_OBJ {
	TSE_IOC_OBJ obj;
	TSE_IOC_HWCPY_CFG cfg;
} TSE_IOC_HWCPY_OBJ;

//============================================================================
// IOCTL command
//============================================================================
#define TSE_IOC_COMMON_TYPE 'T'
#define TSE_IOC_OPEN                    _IO(TSE_IOC_COMMON_TYPE, 1)
#define TSE_IOC_CLOSE                   _IO(TSE_IOC_COMMON_TYPE, 2)

/* Add other command ID here*/
#define TSE_IOC_MUX_W_CFG               _IOWR(TSE_IOC_COMMON_TYPE, 10, TSE_IOC_MUX_OBJ)
#define TSE_IOC_DEMUX_W_CFG             _IOWR(TSE_IOC_COMMON_TYPE, 11, TSE_IOC_DEMUX_OBJ)
#define TSE_IOC_HWCPY_W_CFG             _IOWR(TSE_IOC_COMMON_TYPE, 12, TSE_IOC_HWCPY_OBJ)

#endif //__TSE_IOCTL_CMD_H_
