/*
 * mdrv_swtoe_ipc.h- Sigmastar
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

#ifndef _MDRV_SWTOE_IPC_H__
#define _MDRV_SWTOE_IPC_H__

//--------------------------------------------------------------------------------------------------
//  Constant definition
//--------------------------------------------------------------------------------------------------
#define MAX_CNX_TXQ_NUM 2048

// IPC
// arg0
#define SWTOE_IPC_ARG0 0xfe000000

#define SWTOE_IPC_ARG1_CTRL_BUF_ALLOC 0x01000001
#define SWTOE_IPC_ARG1_CTRL_BUF_FREE  0x01000002 /// must but not used at this stage
// #define SWTOE_IPC_ARG1_STATS_BUF_ALLOC  0x01000003 /// must but not used at this stage
// #define SWTOE_IPC_ARG1_STATS_BUF_FREE   0x01000004 /// must but not used at this stage

#define SWTOE_IPC_ARG1_CTRL_TXQ_ALLOC 0x01000005
#define SWTOE_IPC_ARG1_CTRL_TXQ_FREE  0x01000006 /// must but not used at this stage
#define SWTOE_IPC_ARG1_CTRL_RXQ_ALLOC 0x01000007 /// must but not used at this stage
#define SWTOE_IPC_ARG1_CTRL_RXQ_FREE  0x01000008 /// must but not used at this stage

// TX arg1
#define SWTOE_IPC_ARG1_TXQ_PUT 0x02000001
#define SWTOE_IPC_ARG1_TXQ_GET 0x02000002

// RX
#define SWTOE_IPC_ARG1_RXQ_GET 0x03000001

// Protocol
#define SWTOE_IPC_ARG1_PROT_CREATE 0x04000001

#define SWTOE_IPC_ARG1_PROT_CNX_REQ  0x04000002
#define SWTOE_IPC_ARG1_PROT_CNX_RESP 0x04000003

#define SWTOE_IPC_ARG1_PROT_DISCNX_REQ  0x04000004
#define SWTOE_IPC_ARG1_PROT_DISCNX_RESP 0x04000005 // Is this required??

#define SWTOE_IPC_ARG1_PROT_BIND_REQ  0x04000006
#define SWTOE_IPC_ARG1_PROT_BIND_RESP 0x04000007

#define SWTOE_IPC_ARG1_PROT_LSTN_START 0x04000008
#define SWTOE_IPC_ARG1_PROT_LSTN_STOP  0x04000009
#define SWTOE_IPC_ARG1_PROT_LSTN_RESP  0x0400000a

#define SWTOE_IPC_ARG1_PROT_ACPT_REQ  0x0400000c
#define SWTOE_IPC_ARG1_PROT_ACPT_RESP 0x0400000d // Is this required??

#define SWTOE_IPC_ARG1_PROT_SHDN_REQ  0x040000F0
#define SWTOE_IPC_ARG1_PROT_SHDN_RESP 0x040000F1 // Is this required??
#define SWTOE_IPC_ARG1_PROT_CLS_REQ   0x040000F2
#define SWTOE_IPC_ARG1_PROT_CLS_RESP  0x040000F3

//--------------------------------------------------------------------------------------------------
//  Some helper functions
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  Data structure definition
//--------------------------------------------------------------------------------------------------
typedef struct
{
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    u32 blk : 1;
    u32 status : 1;
    u32 cause : 2;
    /// 0x0 : 'connection' information from SWTOE_IPC_ARG1_PROT_CNX_REQ
    /// 0x1 : 'binding' information from SWTOE_IPC_ARG1_PROT_BIND_REQ
    /// 0x2 : 'listen' information
    u32 reserved : 28;
} __attribute__((packed)) sstoe_prot_cnx_info_t;

typedef struct
{
    u32                   used : 1;
    u32                   prot : 2;
    u32                   cnx_id : 11;
    u32                   reserved : 16;
    u32                   desc_addr;
    u16                   pkt_size;
    u16                   depth;
    u16                   write; // Linux write. RTK read
    u16                   read;  // Linux read. RTK write
    sstoe_prot_cnx_info_t cnx_info;
} __attribute__((packed)) sstoe_ipc_txq_info;

typedef struct
{
    u32 used : 1;
    u32 cnx_id : 11;
    u32 reserved : 20;
    u32 desc_addr;
    u16 pkt_size;
    u16 depth;
    u16 write; // Linux read. RTK write
    u16 read;  // Linux write. RTK read
} __attribute__((packed)) sstoe_ipc_rxq_info;

typedef struct
{
    u32 dummy;
} __attribute__((packed)) sstoe_ipc_stats_t;

typedef struct
{
    sstoe_prot_cnx_info_t lstn;        // lstn data. Linux read only. RTK read/write
    u32                   lock_lx : 2; // 0x0 : initial state which RTK can write it
                                       // 0x1 : RTK write complete and Linux can read it
                                       // 0x2 : Linux read complete and RTK can write it
    u32 reserved : 30;                 // Linux read/write. RTK read only
} __attribute__((packed)) sstoe_ipc_lstn_t;

typedef struct
{
    sstoe_ipc_txq_info txq_info[MAX_CNX_TXQ_NUM];
    sstoe_ipc_rxq_info rxq_info[1];
    sstoe_ipc_lstn_t   lstn_data;

    /// @FIXME : to be refine
    // u32                 txq_signal[((MAX_CNX_TXQ_NUM + 31)>> 3)/sizeof(u32)]; /// this should be used, but ...
    /// @FIXME : to be refine
    // u32                 rxq_signal[((1 + 31)>> 3)/sizeof(u32)];
    sstoe_ipc_stats_t stats;
} __attribute__((packed)) sstoe_ipc_ctrl_t;

typedef struct
{
    u32 hw_own : 1;
    u32 eof : 1;
    u32 reserved1 : 6;
    u32 buf_size : 16;
    /// u32         reserved2       :  2;
    u32 buf_addr : 29;
    u32 reserved3 : 11;
} __attribute__((packed)) sstoe_tx_desc_t;

// rx desc : 12 bytes
typedef struct
{
    u32 hw_own : 1;
    u32 crc_flag : 2; // might not be required
                      // #define CHECKSUM_NONE            0
                      // #define CHECKSUM_UNNECESSARY     1
                      // #define CHECKSUM_COMPLETE        2
                      // #define CHECKSUM_PARTIAL         3
    u32 reserved1 : 5;
    u32 buf_size : 16;
    u32 buf_addr : 29;
    u32 cnx_id : 11;
    // u32         seq_num         : 32;
} __attribute__((packed)) sstoe_rx_desc_t;

//--------------------------------------------------------------------------------------------------
//  global variables
//--------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------------------
//  malloc helper function
//--------------------------------------------------------------------------------------------------

#endif
