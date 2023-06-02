#ifndef __ISF_FLOW_IOCTL_CMD_H
#define __ISF_FLOW_IOCTL_CMD_H

//#include <linux/ioctl.h>
#include "kwrap/ioctl.h"

//============================================================================
// IOCTL command
//============================================================================
#define ISF_FLOW_IOC_MAGIC   'I'

typedef struct {
    unsigned int rv;               ///< [out]result. (see ISF_RV)
    unsigned int cmd;              ///< [in]cmd
    unsigned int p0;               ///< [in]param
    unsigned int p1;               ///< [in]param
    unsigned int p2;               ///< [in]param
}ISF_FLOW_IOCTL_CMD;

typedef struct {
    unsigned int rv;               ///< [out]result. (see ISF_RV)
    unsigned int src;              ///< [in]src unit id + port id (see ISF_OUT)
    unsigned int dest;             ///< [in]dest unit id + port id (see ISF_IN)
}ISF_FLOW_IOCTL_BIND_ITEM;

typedef struct {
    unsigned int rv;               ///< [out]result. (see ISF_RV)
    unsigned int src;              ///< [in]src unit id + port id (see ISF_OUT)
    unsigned int state;            ///< [in]state of this port (see ISF_CONNECT_TYPE)
}ISF_FLOW_IOCTL_STATE_ITEM;

typedef struct {
    unsigned int rv;               ///< [out]result. (see ISF_RV)
    unsigned int dest;             ///< [in]dest unit id + port id (see ISF_IN and ISF_OUT)
    unsigned int param;            ///< [in]parameter id. (see ISF_PARAM)
    unsigned int value;            ///< [in]parameter value.
    unsigned int size;             ///< [in]sizeof parameter.
}ISF_FLOW_IOCTL_PARAM_ITEM;

typedef struct {
    unsigned int rv;              	///< [out]result. (see ISF_RV)
    unsigned int dest;             ///< [in]dest unit id + dest port id (see ISF_IN and ISF_OUT)
    void *p_data;                  ///< [in]data pointer.
    signed int async;              ///< [in]async mode. (for push, pull, notify)
    unsigned int size;             ///< [in]new size. (for new)
    unsigned int addr;             ///< [out]data addr. (for new)
}ISF_FLOW_IOCTL_DATA_ITEM;

typedef struct {
	unsigned long long    timestamp;     ///< [out] timestamp with us
}ISF_FLOW_IOCTL_GET_TIMESTAMP;

typedef struct {
	unsigned int vdocap_active_list[8];
	unsigned int vdoprc_maxdevice;
	unsigned int vdoenc_maxpath;
	unsigned int vdodec_maxpath;
	unsigned int vdoout_maxdevice;
	unsigned int adocap_maxdevice;
	unsigned int adoout_maxdevice;
	unsigned int adoenc_maxpath;
	unsigned int adodec_maxpath;
	unsigned int gfx_maxjob;
	unsigned int stamp_maximg;
	unsigned int vdoprc_maxstamp[2];
	unsigned int vdoprc_maxmask[2];
	unsigned int vdoenc_maxstamp[2];
	unsigned int vdoenc_maxmask[2];
	unsigned int vdoout_maxstamp[2];
	unsigned int vdoout_maxmask[2];
}ISF_FLOW_IOCTL_GET_MAX_PATH;


typedef struct {
    char str[1024];              	///< [in]string.
    int len;                       ///< [in]length of string.
    int uid;             			///< [out]duump unit id
}ISF_FLOW_IOCTL_OUT_LOG;

typedef struct {
    int dummy;             			///< [out]duump unit id
}ISF_FLOW_IOCTL_DUMMY;


#define ISF_FLOW_CMD_INIT		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 31, ISF_FLOW_IOCTL_CMD)
#define ISF_FLOW_CMD_UNINIT		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 32, ISF_FLOW_IOCTL_CMD)
#define ISF_FLOW_CMD_CMD        _VOS_IOWR(ISF_FLOW_IOC_MAGIC, 33, ISF_FLOW_IOCTL_CMD)

#define ISF_FLOW_CMD_SET_BIND		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 1, ISF_FLOW_IOCTL_BIND_ITEM)
#define ISF_FLOW_CMD_GET_BIND		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 2, ISF_FLOW_IOCTL_BIND_ITEM)
#define ISF_FLOW_CMD_SET_STATE		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 3, ISF_FLOW_IOCTL_STATE_ITEM)
#define ISF_FLOW_CMD_GET_STATE		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 4, ISF_FLOW_IOCTL_STATE_ITEM)
#define ISF_FLOW_CMD_SET_PARAM 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 5, ISF_FLOW_IOCTL_PARAM_ITEM)
#define ISF_FLOW_CMD_GET_PARAM 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 6, ISF_FLOW_IOCTL_PARAM_ITEM)

#define ISF_FLOW_CMD_NEW_DATA 		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 8, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_ADD_DATA 		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 9, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_RELEASE_DATA 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 10, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_PUSH_DATA 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 11, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_PULL_DATA 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 12, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_NOTIFY_DATA 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 13, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_INIT_DATA 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 14, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_EVENT_DATA 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 15, ISF_FLOW_IOCTL_DATA_ITEM)
#define ISF_FLOW_CMD_GET_TIMESTAMP 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 16, ISF_FLOW_IOCTL_GET_TIMESTAMP)
#define ISF_FLOW_CMD_GET_MAX_PATH 	_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 17, ISF_FLOW_IOCTL_GET_MAX_PATH)

#define ISF_FLOW_CMD_OUT_WAI		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 20, ISF_FLOW_IOCTL_OUT_LOG)
#define ISF_FLOW_CMD_OUT_STR		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 21, ISF_FLOW_IOCTL_OUT_LOG)
#define ISF_FLOW_CMD_OUT_SIG		_VOS_IOWR(ISF_FLOW_IOC_MAGIC, 22, ISF_FLOW_IOCTL_OUT_LOG)


///////////////////////////////////////////////////////////////////////////////

#if defined(__FREERTOS)
int isf_flow_open (char* file, int flag);
int isf_flow_ioctl (int fd, unsigned int cmd, void *p_arg);
int isf_flow_close (int fd);
#endif

#endif
