/*
 * mdrv_swtoe.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _MDRV_SWTOE_H_
#define _MDRV_SWTOE_H_

//-------------------------------------------------------------------------------------------------
//  Driver Capability
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------
typedef struct
{
    char* buf;
    int   buf_size;
    char* data;
    int   data_size;
    int   offset;
} drv_swtoe_rx_data;

typedef struct
{
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    u32 blk : 1;
    u32 reserved : 31;
} __attribute__((packed)) drv_swtoe_glue_cnx_data;

#define drv_swtoe_lstn_data drv_swtoe_glue_cnx_data

#if 0
typedef struct
{
    drv_swtoe_glue_cnx_data     lstn_data;
    // struct sock*                newsk;
} drv_swtoe_lstn_data;
#endif

//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//  Function and Variable
//-------------------------------------------------------------------------------------------------

/*
 * serv         : service for network transreciv
 * cnx          : connection
 * prot         : protocol
 *                    DRV_SWTOE_PROT_BYPASS     : bypass
 *                    DRV_SWTOE_PROT_TCP        : TCP
 *                    DRV_SWTOE_PROT_UDP        : UDP
 *                    3                         : reserved
 * q            : queue
 * tgt          : target
 * req          : request
int drv_swtoe_cnx_txq_create(int serv_id, int prot);
 * en           : enable
*/

#define DRV_SWTOE_PROT_BYPASS 0x00
#define DRV_SWTOE_PROT_TCP    0x01
#define DRV_SWTOE_PROT_UDP    0x02

#define DRV_SWTOE_CNX_INVALID (0x7FF)
int drv_swtoe_create(int prot, int* cnx_id);
int drv_swtoe_clone(int cnx_id, int* new_cnx_id, drv_swtoe_lstn_data* plstn_data);

int drv_swtoe_bind(int cnx_id, struct sockaddr* addr);
int drv_swtoe_bind_status(int cnx_id);

int drv_swtoe_lstn_start(int cnx_id, int backlog);
int drv_swtoe_lstn_stop(int cnx_id);
int drv_swtoe_lstn_empty(int cnx_id);
int drv_swtoe_lstn_remove(int cnx_id, drv_swtoe_lstn_data* plstn_data);
int drv_swtoe_lstn_clr(int cnx_id);

int drv_swtoe_acpt(int cnx_id, int cnx_id_new);

int drv_swtoe_shutdown(int cnx_id);
int drv_swtoe_close(int cnx_id);
int drv_swtoe_connect(int cnx_id, struct sockaddr* addr, int blk);
// int drv_swtoe_connect_timeo(int cnx_id, int timeo);
int drv_swtoe_disconnect(int cnx_id);

int drv_swtoe_rx_data_get(int cnx_id, drv_swtoe_rx_data* from, drv_swtoe_rx_data** pp_rx_data);
int drv_swtoe_rx_data_free(int cnx_id, drv_swtoe_rx_data* p_rx_data, int free);
int drv_swtoe_rx_data_avail(int cnx_id);

#define SWTOE_TX_SEND_SKB  0x00
#define SWTOE_TX_SEND_PAGE 0x01
#define SWTOE_TX_SEND_IOV  0x02
int drv_swtoe_tx_send(int cnx_id, void* p, int size, int offset, int data_cat);
int drv_swtoe_tx_avail(int cnx_id, int n);
int drv_swtoe_tx_pump(int cnx_id);

typedef int (*drv_swtoe_cb_func_t)(int cnx, int reason, void* reason_data, void* cb_data);

#define DRV_SWTOE_GLUE_TCOM      0x10000001
#define DRV_SWTOE_GLUE_RCOM      0x20000001
#define DRV_SWTOE_GLUE_CNX_OK    0x30000001
#define DRV_SWTOE_GLUE_CNX_FAIL  0x30000002
#define DRV_SWTOE_GLUE_CLS_RESP  0x30000003
#define DRV_SWTOE_GLUE_LSTN_RESP 0x30000004

int drv_swtoe_glue_req(int cnx_id, drv_swtoe_cb_func_t cb, void* cb_data);
int drv_swtoe_glue_en(int cnx_id, int mask, int bEn);

#endif // _MDRV_SWTOE_H_
