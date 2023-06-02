#ifndef _AWB_DTSI_H_
#define _AWB_DTSI_H_

#define SUB_NODE_LENGTH 64

typedef struct _AWB_DTSI {
	CHAR sub_node_name[SUB_NODE_LENGTH];
	INT32 size;
} AWB_DTSI;

extern void awb_dtsi_load(UINT32 id, void *param);

#endif

