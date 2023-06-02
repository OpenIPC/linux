#ifndef _AE_DTSI_H_
#define _AE_DTSI_H_

#define SUB_NODE_LENGTH 64

typedef struct _AE_DTSI {
	CHAR sub_node_name[SUB_NODE_LENGTH];
	INT32 size;
} AE_DTSI;

extern void ae_dtsi_load(UINT32 id, void *param);

#endif

