////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   devEMAC.c
/// @brief  EMAC Driver
/// @author MStar Semiconductor Inc.
///
///////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////
// sync 2015.9.25
// CL1408633 //THEALE/RedLion/mstar2/drv/emac/
// CL1317815 //THEALE/RedLion/mstar2/hal/clippers/emac/
// sync 2015.12.7
// CL1426052 //THEALE/RedLion/mstar2/drv/emac/
/////////////////////////////////////////////////////

#define CONFIG_EMAC_PHY_RESTART_AN 1
//-------------------------------------------------------------------------------------------------
//  Include files
//-------------------------------------------------------------------------------------------------
#include <linux/module.h>
#include <linux/init.h>
//#include <linux/autoconf.h>
#include <linux/mii.h>
#include <linux/netdevice.h>
#include <linux/etherdevice.h>
#include <linux/skbuff.h>
#include <linux/pci.h>
#include <linux/crc32.h>
#include <linux/ethtool.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/timer.h>
#include <linux/version.h>
#include <asm/io.h>
#include <asm/uaccess.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>

#if defined(CONFIG_MIPS)
#include <asm/mips-boards/prom.h>
#include "mhal_chiptop_reg.h"
#elif defined(CONFIG_ARM)
#include <asm/prom.h>
#include <asm/mach/map.h>
#endif

#include "mdrv_types.h"
//#include "mst_platform.h"
//#include "mdrv_system.h"
//#include "chip_int.h"
#include "mhal_emac.h"
#include "mdrv_emac.h"
#include "ms_platform.h"
#include "registers.h"

#ifdef CONFIG_EMAC_SUPPLY_RNG
#include <linux/input.h>
#include <random.h>
#include "mhal_rng_reg.h"
#endif

#include "mdrv_msys_io_st.h"
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include "ms_msys.h"
#include "gpio.h"


int g_emac_led_orange = -1;
int g_emac_led_green  = -1;
int g_emac_led_count  = 0;
int g_emac_led_flick_speed = 30;

extern void MDrv_GPIO_Set_Low(U8 u8IndexGPIO);
extern void MDrv_GPIO_Set_High(U8 u8IndexGPIO);
extern void MDrv_GPIO_Pad_Set(U8 u8IndexGPIO);

int msys_request_dmem(MSYS_DMEM_INFO *mem_info);
int msys_release_dmem(MSYS_DMEM_INFO *mem_info);

//--------------------------------------------------------------------------------------------------
//  Forward declaration
//--------------------------------------------------------------------------------------------------
#define EMAC_RX_TMR         (0)
#define EMAC_LINK_TMR       (1)
#define EMAC_FLOW_CTL_TMR   (2)

#define EMAC_CHECK_LINK_TIME    	(HZ)
u32 gu32CheckLinkTime = HZ;
u32 gu32CheckLinkTimeDis = 100;
u32 gu32intrEnable;



#ifdef TX_SOFTWARE_QUEUE
    #define EMAC_CHECK_CNT              (10)
#else
    #define EMAC_CHECK_CNT              (500000)
#endif

#define EMAC_TX_PTK_BASE            (TX_SKB_BASE + RAM_VA_PA_OFFSET)

#define ALBANY_OUI_MSB              (0)
#define RTL_8210                    (0x1CUL)

#define RX_THROUGHPUT_TEST 0
#define TX_THROUGHPUT_TEST 0
#define PACKET_THRESHOLD 260
#define TXCOUNT_THRESHOLD 10

#define TX_SW_QUEUE_IN_GENERAL_TX     0
#define TX_SW_QUEUE_IN_IRQ            1
#define TX_SW_QUEUE_IN_TIMER          2

#if defined (SOFTWARE_TX_FLOW_CONTROL)
#define MAC_CONTROL_TYPE              0x8808
#define MAC_CONTROL_OPCODE            0x0001
#define PAUSE_QUANTA_TIME_10M         ((1000000*10)/500)
#define PAUSE_QUANTA_TIME_100M        ((1000000*100)/500)
#define PAUSE_TIME_DIVISOR_10M        (PAUSE_QUANTA_TIME_10M/HZ)
#define PAUSE_TIME_DIVISOR_100M       (PAUSE_QUANTA_TIME_100M/HZ)
unsigned char g_emac_speed = 0;
static unsigned int eth_pause_cmd_enable=0;
spinlock_t emac_flow_ctl_lock;
static struct timer_list EMAC_flow_ctl_timer;
#endif
//--------------------------------------------------------------------------------------------------
//  Local variable
//--------------------------------------------------------------------------------------------------
u32 contiROVR = 0;
u32 initstate= 0;
u8 txidx =0;
u32 txcount = 0;
spinlock_t emac_lock;

#ifdef RX_ZERO_COPY
struct sk_buff *rx_skb[MAX_RX_DESCR];
u32 rx_abso_addr[MAX_RX_DESCR];
//struct sk_buff * rx_skb_dummy;
//u32 	rx_abso_addr_dummy;
#endif

// 0x78c9: link is down.
static u32 phy_status_register = 0x78c9UL;

struct sk_buff *pseudo_packet;

unsigned char phyaddr = 0;
#if TX_THROUGHPUT_TEST
unsigned char packet_content[] = {
0xa4, 0xba, 0xdb, 0x95, 0x25, 0x29, 0x00, 0x30, 0x1b, 0xba, 0x02, 0xdb, 0x08, 0x00, 0x45, 0x00,
0x05, 0xda, 0x69, 0x0a, 0x40, 0x00, 0x40, 0x11, 0xbe, 0x94, 0xac, 0x10, 0x5a, 0xe3, 0xac, 0x10,
0x5a, 0x70, 0x92, 0x7f, 0x13, 0x89, 0x05, 0xc6, 0x0c, 0x5b, 0x00, 0x00, 0x03, 0x73, 0x00, 0x00,
0x00, 0x65, 0x00, 0x06, 0xe1, 0xef, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00,
0x13, 0x89, 0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0xff, 0xff, 0xfc, 0x18, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37,
0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33,
0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35,
0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31,
0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39
};
#endif

static unsigned int max_rx_packet_count=0;
static unsigned int max_tx_packet_count=0;
static unsigned int min_tx_fifo_idle_count=0xffff;
static unsigned int tx_bytes_per_timerbak=0;
static unsigned int tx_bytes_per_timer=0;
u32 RAM_ALLOC_SIZE=0;


//-------------------------------------------------------------------------------------------------
//  Data structure
//-------------------------------------------------------------------------------------------------
static struct timer_list EMAC_timer, Link_timer;
#if RX_THROUGHPUT_TEST
static struct timer_list RX_timer;
#endif
static struct net_device *emac_dev;
//-------------------------------------------------------------------------------------------------
//  EMAC Function
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_tx (struct sk_buff *skb, struct net_device *dev);
static void MDev_EMAC_timer_callback( unsigned long value );
static int MDev_EMAC_SwReset(struct net_device *dev);
#ifndef TX_SKB_PTR
static void MDev_EMAC_Send_PausePkt(struct net_device* dev);
#endif
#ifdef RX_ZERO_COPY
//static int fill_rx_ring(struct net_device *netdev);
//static int dequeue_rx_buffer(struct EMAC_private *p, struct sk_buff **pskb);
static void free_rx_skb(void)
{
	int i = 0;

	for (i = 0; i < MAX_RX_DESCR; i ++)
	{
		if (rx_skb[i])
			kfree_skb(rx_skb[i]);

	}

}
#endif


#ifdef TX_SW_QUEUE
static void _MDev_EMAC_tx_reset_TX_SW_QUEUE(struct net_device* netdev);
#endif

unsigned long oldTime;
unsigned long PreLinkStatus;
#ifdef MSTAR_EMAC_NAPI
static void MDEV_EMAC_ENABLE_RX_REG(void);
static void MDEV_EMAC_DISABLE_RX_REG(void);
static int MDev_EMAC_napi_poll(struct napi_struct *napi, int budget);
#endif

#ifdef CONFIG_MSTAR_EEE
static int MDev_EMAC_IS_TX_IDLE(void);
#endif //CONFIG_MSTAR_EEE


//!!!! PACKET_DUMP has not been tested as they are not used. 2016/07/18
#if defined(PACKET_DUMP)
extern struct file* msys_kfile_open(const char* path, int flags, int rights);
extern void msys_kfile_close(struct file* fp);
extern int msys_kfile_write(struct file* fp, unsigned long long offset, unsigned char* data, unsigned int size);

static int txDumpCtrl=0;
static int rxDumpCtrl=0;
static int txDumpFileLength=0;
static int rxDumpFileLength=0;
static char txDumpFileName[32]={0};
static char rxDumpFileName[32]={0};
static struct file* txDumpFile=NULL;
static struct file* rxDumpFile=NULL;

static ssize_t tx_dump_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    txDumpCtrl = simple_strtoul(buf, NULL, 10);
    if(1==txDumpCtrl)
    {
        strcpy(txDumpFileName,"/tmp/emac/tx_dump");
        txDumpFile=msys_kfile_open(rxDumpFileName, O_RDWR | O_CREAT, 0644);
        if(NULL!=txDumpFile)
        {
            txDumpFileLength=0;
            printk(KERN_WARNING"success to open emac tx_dump file, '%s'...\n",txDumpFileName);
        }
        else
        {
            printk(KERN_WARNING"failed to open emac tx_dump file, '%s'!!\n",txDumpFileName);
        }
    }
    else if(0==txDumpCtrl && txDumpFile!=NULL)
    {

        msys_kfile_close(txDumpFile);
        txDumpFile=NULL;
    }
    return count;
}
static ssize_t tx_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{

    return sprintf(buf, "%d\n", txDumpCtrl);
}
DEVICE_ATTR(tx_dump, 0644, tx_dump_show, tx_dump_store);

static ssize_t rx_dump_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{

    rxDumpCtrl = simple_strtoul(buf, NULL, 10);
    if(1==rxDumpCtrl)
    {
        strcpy(rxDumpFileName,"/tmp/emac/rx_dump");
        rxDumpFile=msys_kfile_open(rxDumpFileName, O_RDWR | O_CREAT, 0644);
        if(NULL!=rxDumpFile)
        {
            rxDumpFileLength=0;
            printk(KERN_WARNING"success to open emac rx_dump file, '%s'...\n",rxDumpFileName);
        }
        else
        {
            printk(KERN_WARNING"failed to open emac rx_dump file, '%s'!!\n",rxDumpFileName);
        }
    }
    else if(0==rxDumpCtrl)
    {
        if(rxDumpFile!=NULL)
        {
            msys_kfile_close(rxDumpFile);
            rxDumpFile=NULL;
        }
    }
    return count;
}
static ssize_t rx_dump_show(struct device *dev, struct device_attribute *attr, char *buf)
{

    return sprintf(buf, "%d\n", rxDumpCtrl);
}
DEVICE_ATTR(rx_dump, 0644, rx_dump_show, rx_dump_store);
#endif

static unsigned long getCurMs(void)
{
	struct timeval tv;
	unsigned long curMs;

	do_gettimeofday(&tv);
    curMs = tv.tv_usec/1000;
	curMs += tv.tv_sec * 1000;
    return curMs;
}

#if RX_THROUGHPUT_TEST
int receive_bytes = 0;
static void RX_timer_callback( unsigned long value){
    int get_bytes = receive_bytes;
    int cur_speed;
    receive_bytes = 0;

    cur_speed = get_bytes*8/20/1024;
    printk(" %dkbps",cur_speed);
    RX_timer.expires = jiffies + 20*EMAC_CHECK_LINK_TIME;
    add_timer(&RX_timer);
}
#endif

//-------------------------------------------------------------------------------------------------
// PHY MANAGEMENT
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Access the PHY to determine the current Link speed and Mode, and update the
// MAC accordingly.
// If no link or auto-negotiation is busy, then no changes are made.
// Returns:  0 : OK
//              -1 : No link
//              -2 : AutoNegotiation still in progress
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_update_linkspeed (struct net_device *dev)
{
    u32 bmsr, bmcr, adv, lpa, neg;
    u32 speed, duplex;

#ifdef CONFIG_EMAC_PHY_RESTART_AN
        u32 hcd_link_st_ok, an_100t_link_st = 0;
        static unsigned int phy_restart_cnt = 0;
#endif /* CONFIG_EMAC_PHY_RESTART_AN */

    // Link status is latched, so read twice to get current value //
    MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);
    MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);
    if (!(bmsr & BMSR_LSTATUS)){
    #ifdef CONFIG_MSTAR_EEE
        MHal_EMAC_Disable_EEE();

        if (PreLinkStatus == 1)
        {
            MHal_EMAC_Reset_EEE();
        }
    #endif

        PreLinkStatus = 0;

        return -1;          //no link //
    }

    MHal_EMAC_read_phy (phyaddr, MII_BMCR, &bmcr);

    if (bmcr & BMCR_ANENABLE)
    {               //AutoNegotiation is enabled //
        if (!(bmsr & BMSR_ANEGCOMPLETE))
        {
            //EMAC_DBG("==> AutoNegotiation still in progress\n");
            return -2;
        }

        /* Get Link partner and advertisement from the PHY not from the MAC */
        MHal_EMAC_read_phy(phyaddr, MII_ADVERTISE, &adv);
        MHal_EMAC_read_phy(phyaddr, MII_LPA, &lpa);

        /* For Link Parterner adopts force mode and EPHY used,
         * EPHY LPA reveals all zero value.
         * EPHY would be forced to Full-Duplex mode.
         */
        if (!lpa)
        {
            /* 100Mbps Full-Duplex */
            if (bmcr & BMCR_SPEED100)
                lpa |= LPA_100FULL;
            else /* 10Mbps Full-Duplex */
                lpa |= LPA_10FULL;
        }

        neg = adv & lpa;

        if (neg & LPA_100FULL)
        {
            speed = SPEED_100;
            duplex = DUPLEX_FULL;
        }
        else if (neg & LPA_100HALF)
        {
            speed = SPEED_100;
            duplex = DUPLEX_HALF;
        }
        else if (neg & LPA_10FULL)
        {
            speed = SPEED_10;
            duplex = DUPLEX_FULL;
        }
        else if (neg & LPA_10HALF)
        {
            speed = SPEED_10;
            duplex = DUPLEX_HALF;
        }
        else
        {
            speed = SPEED_10;
            duplex = DUPLEX_HALF;
            EMAC_DBG("%s: No speed and mode found (LPA=0x%x, ADV=0x%x)\n", __FUNCTION__, lpa, adv);
        }

    }
    else
    {
        speed = (bmcr & BMCR_SPEED100) ? SPEED_100 : SPEED_10;
        duplex = (bmcr & BMCR_FULLDPLX) ? DUPLEX_FULL : DUPLEX_HALF;
    }

    // Update the MAC //
    MHal_EMAC_update_speed_duplex(speed,duplex);


#ifdef CONFIG_MSTAR_EEE
    /*TX idle, enable EEE*/
    if((MDev_EMAC_IS_TX_IDLE()) && (speed == SPEED_100) && (duplex == DUPLEX_FULL))
    {
        if (PreLinkStatus == 0)
        {
            MHal_EMAC_Enable_EEE(300);
        }
        else
        {
            MHal_EMAC_Enable_EEE(0);
        }
    }
#endif

    PreLinkStatus = 1;
#ifdef CONFIG_EMAC_PHY_RESTART_AN
        if (speed == SPEED_100) {
            MHal_EMAC_read_phy(phyaddr, 0x21, &hcd_link_st_ok);
            MHal_EMAC_read_phy(phyaddr, 0x22, &an_100t_link_st);
            if (((hcd_link_st_ok & 0x100) && !(an_100t_link_st & 0x300)) || (!(hcd_link_st_ok & 0x100) && ((an_100t_link_st & 0x300) == 0x200))) {
                phy_restart_cnt++;
                if (phy_restart_cnt > 10) {
                    EMAC_DBG("MDev_EMAC_update_linkspeed: restart AN process\n");
                    MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1200UL);
                    phy_restart_cnt = 0;
                }
            } else {
                phy_restart_cnt = 0;
            }
        }
#endif /* CONFIG_EMAC_PHY_RESTART_AN */

    return 0;
}

static int MDev_EMAC_get_info(struct net_device *dev)
{
    u32 bmsr, bmcr, LocPtrA;
    u32 uRegStatus =0;

    // Link status is latched, so read twice to get current value //
    MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);
    MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);
    if (!(bmsr & BMSR_LSTATUS)){
        uRegStatus &= ~ETHERNET_TEST_RESET_STATE;
        uRegStatus |= ETHERNET_TEST_NO_LINK; //no link //
    }
    MHal_EMAC_read_phy (phyaddr, MII_BMCR, &bmcr);

    if (bmcr & BMCR_ANENABLE)
    {
        //AutoNegotiation is enabled //
        if (!(bmsr & BMSR_ANEGCOMPLETE))
        {
            uRegStatus &= ~ETHERNET_TEST_RESET_STATE;
            uRegStatus |= ETHERNET_TEST_AUTO_NEGOTIATION; //AutoNegotiation //
        }
        else
        {
            uRegStatus &= ~ETHERNET_TEST_RESET_STATE;
            uRegStatus |= ETHERNET_TEST_LINK_SUCCESS; //link success //
        }

        MHal_EMAC_read_phy (phyaddr, MII_LPA, &LocPtrA);
        if ((LocPtrA & LPA_100FULL) || (LocPtrA & LPA_100HALF))
        {
            uRegStatus |= ETHERNET_TEST_SPEED_100M; //SPEED_100//
        }
        else
        {
            uRegStatus &= ~ETHERNET_TEST_SPEED_100M; //SPEED_10//
        }

        if ((LocPtrA & LPA_100FULL) || (LocPtrA & LPA_10FULL))
        {
            uRegStatus |= ETHERNET_TEST_DUPLEX_FULL; //DUPLEX_FULL//
        }
        else
        {
            uRegStatus &= ~ETHERNET_TEST_DUPLEX_FULL; //DUPLEX_HALF//
        }
    }
    else
    {
        if(bmcr & BMCR_SPEED100)
        {
            uRegStatus |= ETHERNET_TEST_SPEED_100M; //SPEED_100//
        }
        else
        {
            uRegStatus &= ~ETHERNET_TEST_SPEED_100M; //SPEED_10//
        }

        if(bmcr & BMCR_FULLDPLX)
        {
            uRegStatus |= ETHERNET_TEST_DUPLEX_FULL; //DUPLEX_FULL//
        }
        else
        {
            uRegStatus &= ~ETHERNET_TEST_DUPLEX_FULL; //DUPLEX_HALF//
        }
    }

    return uRegStatus;
}

//-------------------------------------------------------------------------------------------------
//Program the hardware MAC address from dev->dev_addr.
//-------------------------------------------------------------------------------------------------
void MDev_EMAC_update_mac_address (struct net_device *dev)
{
    u32 value;
    value = (dev->dev_addr[3] << 24) | (dev->dev_addr[2] << 16) | (dev->dev_addr[1] << 8) |(dev->dev_addr[0]);
    MHal_EMAC_Write_SA1L(value);
    value = (dev->dev_addr[5] << 8) | (dev->dev_addr[4]);
    MHal_EMAC_Write_SA1H(value);
}

//-------------------------------------------------------------------------------------------------
// ADDRESS MANAGEMENT
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Set the ethernet MAC address in dev->dev_addr
//-------------------------------------------------------------------------------------------------
static void MDev_EMAC_get_mac_address (struct net_device *dev)
{
    char addr[6];
    u32 HiAddr, LoAddr;

    // Check if bootloader set address in Specific-Address 1 //
    HiAddr = MHal_EMAC_get_SA1H_addr();
    LoAddr = MHal_EMAC_get_SA1L_addr();

    addr[0] = (LoAddr & 0xffUL);
    addr[1] = (LoAddr & 0xff00UL) >> 8;
    addr[2] = (LoAddr & 0xff0000UL) >> 16;
    addr[3] = (LoAddr & 0xff000000UL) >> 24;
    addr[4] = (HiAddr & 0xffUL);
    addr[5] = (HiAddr & 0xff00UL) >> 8;

    if (is_valid_ether_addr (addr))
    {
        memcpy (dev->dev_addr, &addr, 6);
        return;
    }
    // Check if bootloader set address in Specific-Address 2 //
    HiAddr = MHal_EMAC_get_SA2H_addr();
    LoAddr = MHal_EMAC_get_SA2L_addr();
    addr[0] = (LoAddr & 0xffUL);
    addr[1] = (LoAddr & 0xff00UL) >> 8;
    addr[2] = (LoAddr & 0xff0000UL) >> 16;
    addr[3] = (LoAddr & 0xff000000UL) >> 24;
    addr[4] = (HiAddr & 0xffUL);
    addr[5] = (HiAddr & 0xff00UL) >> 8;

    if (is_valid_ether_addr (addr))
    {
        memcpy (dev->dev_addr, &addr, 6);
        return;
    }
}

#ifdef URANUS_ETHER_ADDR_CONFIGURABLE
//-------------------------------------------------------------------------------------------------
// Store the new hardware address in dev->dev_addr, and update the MAC.
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_set_mac_address (struct net_device *dev, void *addr)
{
    struct sockaddr *address = addr;
    if (!is_valid_ether_addr (address->sa_data))
        return -EADDRNOTAVAIL;

    memcpy (dev->dev_addr, address->sa_data, dev->addr_len);
    MDev_EMAC_update_mac_address (dev);
    return 0;
}
#endif

//-------------------------------------------------------------------------------------------------
// Mstar Multicast hash rule
//-------------------------------------------------------------------------------------------------
//Hash_index[5] = da[5] ^ da[11] ^ da[17] ^ da[23] ^ da[29] ^ da[35] ^ da[41] ^ da[47]
//Hash_index[4] = da[4] ^ da[10] ^ da[16] ^ da[22] ^ da[28] ^ da[34] ^ da[40] ^ da[46]
//Hash_index[3] = da[3] ^ da[09] ^ da[15] ^ da[21] ^ da[27] ^ da[33] ^ da[39] ^ da[45]
//Hash_index[2] = da[2] ^ da[08] ^ da[14] ^ da[20] ^ da[26] ^ da[32] ^ da[38] ^ da[44]
//Hash_index[1] = da[1] ^ da[07] ^ da[13] ^ da[19] ^ da[25] ^ da[31] ^ da[37] ^ da[43]
//Hash_index[0] = da[0] ^ da[06] ^ da[12] ^ da[18] ^ da[24] ^ da[30] ^ da[36] ^ da[42]
//-------------------------------------------------------------------------------------------------

static void MDev_EMAC_sethashtable(unsigned char *addr)
{
    u32 mc_filter[2];
    u32 uHashIdxBit;
    u32 uHashValue;
    u32 i;
    u32 tmpcrc;
    u32 uSubIdx;
    u64 macaddr;
    u64 mac[6];

    uHashValue = 0;
    macaddr = 0;

    // Restore mac //
    for(i = 0; i < 6;  i++)
    {
        mac[i] =(u64)addr[i];
    }

    // Truncate mac to u64 container //
    macaddr |=  mac[0] | (mac[1] << 8) | (mac[2] << 16);
    macaddr |=  (mac[3] << 24) | (mac[4] << 32) | (mac[5] << 40);

    // Caculate the hash value //
    for(uHashIdxBit = 0; uHashIdxBit < 6;  uHashIdxBit++)
    {
        tmpcrc = (macaddr & (0x1UL << uHashIdxBit)) >> uHashIdxBit;
        for(i = 1; i < 8;  i++)
        {
            uSubIdx = uHashIdxBit + (i * 6);
            tmpcrc = tmpcrc ^ ((macaddr >> uSubIdx) & 0x1);
        }
        uHashValue |= (tmpcrc << uHashIdxBit);
    }

    mc_filter[0] = MHal_EMAC_ReadReg32( REG_ETH_HSL);
    mc_filter[1] = MHal_EMAC_ReadReg32( REG_ETH_HSH);

    // Set the corrsponding bit according to the hash value //
    if(uHashValue < 32)
    {
        mc_filter[0] |= (0x1UL <<  uHashValue);
        MHal_EMAC_WritReg32( REG_ETH_HSL, mc_filter[0] );
    }
    else
    {
        mc_filter[1] |= (0x1UL <<  (uHashValue - 32));
        MHal_EMAC_WritReg32( REG_ETH_HSH, mc_filter[1] );
    }
}

//-------------------------------------------------------------------------------------------------
//Enable/Disable promiscuous and multicast modes.
//-------------------------------------------------------------------------------------------------
static void MDev_EMAC_set_rx_mode (struct net_device *dev)
{
    u32 uRegVal;
    struct netdev_hw_addr *ha;

    uRegVal  = MHal_EMAC_Read_CFG();

    if (dev->flags & IFF_PROMISC)
    {
        // Enable promiscuous mode //
        uRegVal |= EMAC_CAF;
    }
    else if (dev->flags & (~IFF_PROMISC))
    {
        // Disable promiscuous mode //
        uRegVal &= ~EMAC_CAF;
    }
    MHal_EMAC_Write_CFG(uRegVal);

    if (dev->flags & IFF_ALLMULTI)
    {
        // Enable all multicast mode //
        MHal_EMAC_update_HSH(-1,-1);
        uRegVal |= EMAC_MTI;
    }
    else if (dev->flags & IFF_MULTICAST)
    {
        // Enable specific multicasts//
        MHal_EMAC_update_HSH(0,0);
        netdev_for_each_mc_addr(ha, dev)
        {
            MDev_EMAC_sethashtable(ha->addr);
        }
        uRegVal |= EMAC_MTI;
    }
    else if (dev->flags & ~(IFF_ALLMULTI | IFF_MULTICAST))
    {
        // Disable all multicast mode//
        MHal_EMAC_update_HSH(0,0);
        uRegVal &= ~EMAC_MTI;
    }

    MHal_EMAC_Write_CFG(uRegVal);
}
//-------------------------------------------------------------------------------------------------
// IOCTL
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
// Enable/Disable MDIO
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_mdio_read (struct net_device *dev, int phy_id, int location)
{
    u32 value;
    MHal_EMAC_read_phy (phy_id, location, &value);
    return value;
}

static void MDev_EMAC_mdio_write (struct net_device *dev, int phy_id, int location, int value)
{
    MHal_EMAC_write_phy (phy_id, location, value);
}

//-------------------------------------------------------------------------------------------------
//ethtool support.
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_ethtool_ioctl (struct net_device *dev, void *useraddr)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    u32 ethcmd;
    int res = 0;

    if (copy_from_user (&ethcmd, useraddr, sizeof (ethcmd)))
        return -EFAULT;

    spin_lock_irq (LocPtr->lock);

    switch (ethcmd)
    {
        case ETHTOOL_GSET:
        {
            struct ethtool_cmd ecmd = { ETHTOOL_GSET };
            res = mii_ethtool_gset (&LocPtr->mii, &ecmd);
            if (copy_to_user (useraddr, &ecmd, sizeof (ecmd)))
                res = -EFAULT;
            break;
        }
        case ETHTOOL_SSET:
        {
            struct ethtool_cmd ecmd;
            if (copy_from_user (&ecmd, useraddr, sizeof (ecmd)))
                res = -EFAULT;
            else
                res = mii_ethtool_sset (&LocPtr->mii, &ecmd);
            break;
        }
        case ETHTOOL_NWAY_RST:
        {
            res = mii_nway_restart (&LocPtr->mii);
            break;
        }
        case ETHTOOL_GLINK:
        {
            struct ethtool_value edata = { ETHTOOL_GLINK };
            edata.data = mii_link_ok (&LocPtr->mii);
            if (copy_to_user (useraddr, &edata, sizeof (edata)))
                res = -EFAULT;
            break;
        }
        default:
            res = -EOPNOTSUPP;
    }
    spin_unlock_irq (LocPtr->lock);
    return res;
}

//-------------------------------------------------------------------------------------------------
// User-space ioctl interface.
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_ioctl (struct net_device *dev, struct ifreq *rq, int cmd)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    struct mii_ioctl_data *data = if_mii(rq);
    u32 value;
    if (!netif_running(dev))
    {
        rq->ifr_metric = ETHERNET_TEST_INIT_FAIL;
    }

    switch (cmd)
    {
        case SIOCGMIIPHY:
            data->phy_id = (phyaddr & 0x1FUL);
            return 0;

        case SIOCDEVPRIVATE:
            rq->ifr_metric = (MDev_EMAC_get_info(emac_dev)|initstate);
            return 0;
/*
        case SIOCDEVON:
            MHal_EMAC_Power_On_Clk();
            return 0;

        case SIOCDEVOFF:
            MHal_EMAC_Power_Off_Clk();
            return 0;
*/
        case SIOCGMIIREG:
            // check PHY's register 1.
            if((data->reg_num & 0x1fUL) == 0x1UL)
            {
                // PHY's register 1 value is set by timer callback function.
                spin_lock_irq(LocPtr->lock);
                data->val_out = phy_status_register;
                spin_unlock_irq(LocPtr->lock);
            }
            else
            {
                MHal_EMAC_read_phy((phyaddr & 0x1FUL), (data->reg_num & 0x1fUL), (u32 *)&(value));
                data->val_out = value;
            }
            return 0;

        case SIOCSMIIREG:
            if (!capable(CAP_NET_ADMIN))
                return -EPERM;
            MHal_EMAC_write_phy((phyaddr & 0x1FUL), (data->reg_num & 0x1fUL), data->val_in);
            return 0;

        case SIOCETHTOOL:
            return MDev_EMAC_ethtool_ioctl (dev, (void *) rq->ifr_data);

        default:
            return -EOPNOTSUPP;
    }
}
//-------------------------------------------------------------------------------------------------
// MAC
//-------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------
//Initialize and start the Receiver and Transmit subsystems
//-------------------------------------------------------------------------------------------------
static void MDev_EMAC_start (struct net_device *dev)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    struct recv_desc_bufs *dlist;
#ifdef SOFTWARE_DESCRIPTOR
    int i;
#endif
    u32 uRegVal;
    dlist = LocPtr->dlist;

#ifdef SOFTWARE_DESCRIPTOR
    for(i = 0; i < MAX_RX_DESCR; i++)
    {
        dlist->descriptors[i].addr &= ~EMAC_DESC_DONE;
        dlist->descriptors[i].size = 0;
        //// Set the Wrap bit on the last descriptor //
        //   if(i ==(MAX_RX_DESCR - 1))
        //           dlist->descriptors[MAX_RX_DESCR - 1].addr |= EMAC_DESC_WRAP;
    }
    // Program address of descriptor list in Rx Buffer Queue register //
    uRegVal = ((EMAC_REG) & dlist->descriptors)- RAM_VA_PA_OFFSET - MIU0_BUS_BASE;
    MHal_EMAC_Write_RBQP(uRegVal);
#else
    // set offset of read and write pointers in the receive circular buffer //
    uRegVal = MHal_EMAC_Read_BUFF();
    uRegVal = (RX_BUFFER_BASE|RX_BUFFER_SEL) - MIU0_BUS_BASE;
    MHal_EMAC_Write_BUFF(uRegVal);
    MHal_EMAC_Write_RDPTR(0);
    MHal_EMAC_Write_WRPTR(0);
#endif

    //Reset buffer index//
    LocPtr->rxBuffIndex = 0;

    // Enable Receive and Transmit //
    uRegVal = MHal_EMAC_Read_CTL();
    uRegVal |= (EMAC_RE | EMAC_TE);
    MHal_EMAC_Write_CTL(uRegVal);
}

//-------------------------------------------------------------------------------------------------
// Open the ethernet interface
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_open (struct net_device *dev)
{

    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    u32 uRegVal;
    unsigned long flags;
    int ret;
#ifdef MSTAR_EMAC_NAPI
    napi_enable(&LocPtr->napi);
#endif

    spin_lock_irqsave (LocPtr->lock,flags);
    ret = MDev_EMAC_update_linkspeed(dev);
    spin_unlock_irqrestore (LocPtr->lock,flags);

    if (!is_valid_ether_addr (dev->dev_addr))
       return -EADDRNOTAVAIL;
#ifdef TX_SW_QUEUE
    _MDev_EMAC_tx_reset_TX_SW_QUEUE(dev);
#endif
    //ato  EMAC_SYS->PMC_PCER = 1 << EMAC_ID_EMAC;   //Re-enable Peripheral clock //
    //MHal_EMAC_Power_On_Clk(dev->dev);
    uRegVal = MHal_EMAC_Read_CTL();
    uRegVal |= EMAC_CSR;
    MHal_EMAC_Write_CTL(uRegVal);
    // Enable PHY interrupt //
    MHal_EMAC_enable_phyirq ();

    // Enable MAC interrupts //
    MHal_EMAC_Write_IDR(0xFFFFFFFF); //clear interrupt maybe it was set in uboot
    gu32intrEnable = EMAC_INT_RBNA|EMAC_INT_TUND|EMAC_INT_RTRY|EMAC_INT_ROVR|EMAC_INT_HRESP;
#if defined(TX_SKB_PTR)
    gu32intrEnable = gu32intrEnable | EMAC_INT_TCOM;
#endif
#if defined(INT_JULIAN_D)
    // enable delay interrupt
    uRegVal = MHal_EMAC_Read_JULIAN_0104();
    uRegVal |= 0x00000080UL;
    MHal_EMAC_Write_JULIAN_0104(uRegVal);
#else
    gu32intrEnable = gu32intrEnable | EMAC_INT_RCOM;
#endif
    MHal_EMAC_Write_IER(gu32intrEnable);

    LocPtr->ep_flag |= EP_FLAG_OPEND;

    MDev_EMAC_start (dev);
    netif_start_queue (dev);

    init_timer( &Link_timer );
    Link_timer.data = EMAC_LINK_TMR;
    Link_timer.function = MDev_EMAC_timer_callback;
    Link_timer.expires = jiffies + EMAC_CHECK_LINK_TIME;
    add_timer(&Link_timer);

    /* check if network linked */
    if (-1 == ret)
    {
        netif_carrier_off(dev);
        ThisBCE.connected = 0;
    }
    else if(0 == ret)
    {
        netif_carrier_on(dev);
        ThisBCE.connected = 1;
    }
    return 0;
}

//-------------------------------------------------------------------------------------------------
// Close the interface
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_close (struct net_device *dev)
{
    u32 uRegVal;
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
#ifdef MSTAR_EMAC_NAPI
    napi_disable(&LocPtr->napi);
#endif

    spin_lock_irq (LocPtr->lock);
    //Disable Receiver and Transmitter //
    uRegVal = MHal_EMAC_Read_CTL();
    uRegVal &= ~(EMAC_TE | EMAC_RE);
    MHal_EMAC_Write_CTL(uRegVal);
    // Disable PHY interrupt //
    MHal_EMAC_disable_phyirq ();

    MHal_EMAC_Write_IDR(0xFFFFFFFF);
#ifdef INT_JULIAN_D

#endif
    netif_stop_queue (dev);
    netif_carrier_off(dev);
    del_timer(&Link_timer);
    //MHal_EMAC_Power_Off_Clk(dev->dev);
    ThisBCE.connected = 0;
    LocPtr->ep_flag &= (~EP_FLAG_OPEND);

#if defined(TX_SKB_PTR)
#ifdef TX_SW_QUEUE
    _MDev_EMAC_tx_reset_TX_SW_QUEUE(dev);
#else
{
    int i;
    for (i=0;i<TX_RING_SIZE;i++)
    {
        if( LocPtr->tx_swq[i].used ==1 && LocPtr->tx_swq[i].skb != NULL)
        {
            dma_unmap_single(&dev->dev, LocPtr->tx_swq[i].skb_physaddr,
                LocPtr->tx_swq[i].skb->len, DMA_TO_DEVICE);
            dev_kfree_skb_any(LocPtr->tx_swq[i].skb);
            LocPtr->tx_swq[i].used = 0;
            LocPtr->tx_swq[i].skb = NULL;
            LocPtr->tx_swq[i].skb_physaddr = 0;
        }
    }
}
#endif
#endif
    spin_unlock_irq (LocPtr->lock);

    return 0;
}

//-------------------------------------------------------------------------------------------------
// Update the current statistics from the internal statistics registers.
//-------------------------------------------------------------------------------------------------
static struct net_device_stats * MDev_EMAC_stats (struct net_device *dev)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    int ale, lenerr, seqe, lcol, ecol;

    spin_lock_irq (LocPtr->lock);

    if (netif_running (dev))
    {
        LocPtr->stats.rx_packets += MHal_EMAC_Read_OK();            /* Good frames received */
        ale = MHal_EMAC_Read_ALE();
        LocPtr->stats.rx_frame_errors += ale;                       /* Alignment errors */
        lenerr = MHal_EMAC_Read_ELR();
        LocPtr->stats.rx_length_errors += lenerr;                   /* Excessive Length or Undersize Frame error */
        seqe = MHal_EMAC_Read_SEQE();
        LocPtr->stats.rx_crc_errors += seqe;                        /* CRC error */
        LocPtr->stats.rx_fifo_errors += MHal_EMAC_Read_ROVR();
        LocPtr->stats.rx_errors += ale + lenerr + seqe + MHal_EMAC_Read_SE() + MHal_EMAC_Read_RJB();
        LocPtr->stats.tx_packets += MHal_EMAC_Read_FRA();           /* Frames successfully transmitted */
        LocPtr->stats.tx_fifo_errors += MHal_EMAC_Read_TUE();       /* Transmit FIFO underruns */
        LocPtr->stats.tx_carrier_errors += MHal_EMAC_Read_CSE();    /* Carrier Sense errors */
        LocPtr->stats.tx_heartbeat_errors += MHal_EMAC_Read_SQEE(); /* Heartbeat error */
        lcol = MHal_EMAC_Read_LCOL();
        ecol = MHal_EMAC_Read_ECOL();
        LocPtr->stats.tx_window_errors += lcol;                     /* Late collisions */
        LocPtr->stats.tx_aborted_errors += ecol;                    /* 16 collisions */
        LocPtr->stats.collisions += MHal_EMAC_Read_SCOL() + MHal_EMAC_Read_MCOL() + lcol + ecol;
    }

    spin_unlock_irq (LocPtr->lock);

    return &LocPtr->stats;
}

static int MDev_EMAC_TxReset(void)
{
    u32 val = MHal_EMAC_Read_CTL() & 0x000001FFUL;
    MHal_EMAC_Write_CTL((val & ~EMAC_TE));

    val = MHal_EMAC_Read_CTL() & 0x000001FFUL;
    EMAC_ERR ("MAC0_CTL:0x%08x\n", val);
    //MHal_EMAC_Write_TCR(0);
    mdelay(1);
    MHal_EMAC_Write_CTL((MHal_EMAC_Read_CTL() | EMAC_TE));
    val = MHal_EMAC_Read_CTL() & 0x000001FFUL;
    EMAC_ERR ("MAC0_CTL:0x%08x\n", val);
    return 0;
}

#ifndef TX_SKB_PTR
static int MDev_EMAC_CheckTSR(void)
{
    u32 check;
    u32 tsrval = 0;

    #ifdef TX_QUEUE_4
    u8  avlfifo[8] = {0};
    u8  avlfifoidx;
    u8  avlfifoval = 0;

    for (check = 0; check < EMAC_CHECK_CNT; check++)
    {
        tsrval = MHal_EMAC_Read_TSR();

        avlfifo[0] = ((tsrval & EMAC_IDLETSR) != 0)? 1 : 0;
        avlfifo[1] = ((tsrval & EMAC_BNQ)!= 0)? 1 : 0;
        avlfifo[2] = ((tsrval & EMAC_TBNQ) != 0)? 1 : 0;
        avlfifo[3] = ((tsrval & EMAC_FBNQ) != 0)? 1 : 0;
        avlfifo[4] = ((tsrval & EMAC_FIFO1IDLE) !=0)? 1 : 0;
        avlfifo[5] = ((tsrval & EMAC_FIFO2IDLE) != 0)? 1 : 0;
        avlfifo[6] = ((tsrval & EMAC_FIFO3IDLE) != 0)? 1 : 0;
        avlfifo[7] = ((tsrval & EMAC_FIFO4IDLE) != 0)? 1 : 0;

        avlfifoval = 0;

        for(avlfifoidx = 0; avlfifoidx < 8; avlfifoidx++)
        {
            avlfifoval += avlfifo[avlfifoidx];
        }

        if (avlfifoval > 4)
        {
            return NETDEV_TX_OK;
        }
    }
    #else
    for (check = 0; check < EMAC_CHECK_CNT; check++)
    {
        tsrval = MHal_EMAC_Read_TSR();
        if ((tsrval & EMAC_IDLETSR) || (tsrval & EMAC_BNQ))
            return NETDEV_TX_OK;
    }
    #endif

#ifndef TX_SOFTWARE_QUEUE
    EMAC_ERR("Err CheckTSR:0x%x\n", tsrval);
    MDev_EMAC_TxReset();
#endif

    return NETDEV_TX_BUSY;
}
#endif

int MDev_EMAC_GetTXFIFOIdle(void)
{
    u32 tsrval = 0;
    u8  avlfifo[8] = {0};
    u8  avlfifoidx;
    u8  avlfifoval = 0;

    tsrval = MHal_EMAC_Read_TSR();
    avlfifo[0] = ((tsrval & EMAC_IDLETSR) != 0)? 1 : 0;
    avlfifo[1] = ((tsrval & EMAC_BNQ)!= 0)? 1 : 0;
    avlfifo[2] = ((tsrval & EMAC_TBNQ) != 0)? 1 : 0;
    avlfifo[3] = ((tsrval & EMAC_FBNQ) != 0)? 1 : 0;
    avlfifo[4] = ((tsrval & EMAC_FIFO1IDLE) !=0)? 1 : 0;
    avlfifo[5] = ((tsrval & EMAC_FIFO2IDLE) != 0)? 1 : 0;
    avlfifo[6] = ((tsrval & EMAC_FIFO3IDLE) != 0)? 1 : 0;
    avlfifo[7] = ((tsrval & EMAC_FIFO4IDLE) != 0)? 1 : 0;

    avlfifoval = 0;
    for(avlfifoidx = 0; avlfifoidx < 8; avlfifoidx++)
    {
        avlfifoval += avlfifo[avlfifoidx];
    }

    if (avlfifoval > 4)
    {
        avlfifoval-=4;
    }
    else
    {
        avlfifoval= 0;
    }

#if defined(NEW_TX_QUEUE_128)
      avlfifoval = avlfifoval + (NEW_TX_QUEUE_SIZE - MHal_EMAC_get_TXQUEUE_Count());
#endif
    return avlfifoval;
}

#ifdef CONFIG_MSTAR_EEE
static int MDev_EMAC_IS_TX_IDLE(void)
{
    u32 check;
    u32 tsrval = 0;

    u8  avlfifo[8] = {0};
    u8  avlfifoidx;
    u8  avlfifoval = 0;

#ifdef TX_QUEUE_4
    for (check = 0; check < EMAC_CHECK_CNT; check++)
    {
        tsrval = MHal_EMAC_Read_TSR();

        avlfifo[0] = ((tsrval & EMAC_IDLETSR) != 0)? 1 : 0;
        avlfifo[1] = ((tsrval & EMAC_BNQ)!= 0)? 1 : 0;
        avlfifo[2] = ((tsrval & EMAC_TBNQ) != 0)? 1 : 0;
        avlfifo[3] = ((tsrval & EMAC_FBNQ) != 0)? 1 : 0;
        avlfifo[4] = ((tsrval & EMAC_FIFO1IDLE) !=0)? 1 : 0;
        avlfifo[5] = ((tsrval & EMAC_FIFO2IDLE) != 0)? 1 : 0;
        avlfifo[6] = ((tsrval & EMAC_FIFO3IDLE) != 0)? 1 : 0;
        avlfifo[7] = ((tsrval & EMAC_FIFO4IDLE) != 0)? 1 : 0;

        avlfifoval = 0;

        for(avlfifoidx = 0; avlfifoidx < 8; avlfifoidx++)
        {
            avlfifoval += avlfifo[avlfifoidx];
        }

        if (avlfifoval == 8)
            return 1;
    }
#endif

    return 0;
}
#endif //CONFIG_MSTAR_EEE


void MDrv_EMAC_DumpMem(phys_addr_t addr, u32 len)
{
    u8 *ptr = (u8 *)addr;
    u32 i;

    printk("\n ===== Dump %lx =====\n", (long unsigned int)ptr);
    for (i=0; i<len; i++)
    {
        if ((u32)i%0x10UL ==0)
            printk("%lx: ", (long unsigned int)ptr);
        if (*ptr < 0x10UL)
            printk("0%x ", *ptr);
        else
            printk("%x ", *ptr);
        if ((u32)i%0x10UL == 0x0fUL)
            printk("\n");
	ptr++;
    }
    printk("\n");
}

#ifndef TX_SKB_PTR
static u8 pause_pkt[] =
{
    //DA - multicast
    0x01, 0x80, 0xC2, 0x00, 0x00, 0x01,
    //SA
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    //Len-Type
    0x88, 0x08,
    //Ctrl code
    0x00, 0x01,
    //Ctrl para 8192
    0x20, 0x00
};

static dma_addr_t get_tx_addr(void)
{
    dma_addr_t addr;

    addr = EMAC_TX_PTK_BASE + 2048*txidx;
    txidx ++;
    txidx = txidx % TX_RING_SIZE;
    return addr;
}

//Background send
static int MDev_EMAC_BGsend(struct net_device* dev, phys_addr_t addr, int len )
{
	dma_addr_t skb_addr;
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);

    if (NETDEV_TX_OK != MDev_EMAC_CheckTSR())
        return NETDEV_TX_BUSY;

    skb_addr = get_tx_addr();
    memcpy((void*)skb_addr,(void *)addr, len);

    LocPtr->stats.tx_bytes += len;

    Chip_Flush_Cache_Range(skb_addr, len); // Chip_Flush_Memory();

    //Set address of the data in the Transmit Address register //
    MHal_EMAC_Write_TAR(skb_addr - RAM_VA_PA_OFFSET - MIU0_BUS_BASE);
    //printk("[EMAC]%d TODO:DMA buffer 0x%08x\n",__LINE__,skb_addr - RAM_VA_PA_OFFSET - MIU0_BUS_BASE);

    // Set length of the packet in the Transmit Control register //
    MHal_EMAC_Write_TCR(len);

    return NETDEV_TX_OK;
}

static void MDev_EMAC_Send_PausePkt(struct net_device* dev)
{
    u32 val = MHal_EMAC_Read_CTL() & 0x000001FFUL;

    //Disable Rx
    MHal_EMAC_Write_CTL((val & ~EMAC_RE));
    memcpy(&pause_pkt[6], dev->dev_addr, 6);
    MDev_EMAC_BGsend(dev, (phys_addr_t)pause_pkt, sizeof(pause_pkt));
    //Enable Rx
    MHal_EMAC_Write_CTL((MHal_EMAC_Read_CTL() | EMAC_RE));
}
#endif
//-------------------------------------------------------------------------------------------------
//Patch for losing small-size packet when running SMARTBIT
//-------------------------------------------------------------------------------------------------
#ifdef CONFIG_MP_ETHERNET_MSTAR_ICMP_ENHANCE
static void MDev_EMAC_Period_Retry(struct sk_buff *skb, struct net_device* dev)
{
    u32 xval;
    u32 uRegVal;

    xval = MHal_EMAC_ReadReg32(REG_ETH_CFG);

    if((skb->len <= PACKET_THRESHOLD) && !(xval & EMAC_SPD) && !(xval & EMAC_FD))
    {
        txcount++;
    }
    else
    {
        txcount = 0;
    }

    if(txcount > TXCOUNT_THRESHOLD)
    {
        uRegVal  = MHal_EMAC_Read_CFG();
        uRegVal  |= 0x00001000UL;
        MHal_EMAC_Write_CFG(uRegVal);
    }
    else
    {
        uRegVal = MHal_EMAC_Read_CFG();
        uRegVal &= ~(0x00001000UL);
        MHal_EMAC_Write_CFG(uRegVal);
    }
}
#endif

#ifdef TX_SOFTWARE_QUEUE
int MDev_EMAC_enqueue(struct skb_node **queue_head, struct skb_node **queue_tail, struct sk_buff *skb)
{
    struct skb_node *pskb_node = (struct skb_node *)kmalloc(sizeof(struct skb_node), GFP_ATOMIC);

    if(!pskb_node)
    {
        EMAC_ERR("*pskb_node kmalloc fail\n");
        BUG_ON(1);
        return 1;
    }

    /*Reset the content of pskb_node*/
    memset(pskb_node, 0x0, sizeof(struct skb_node));

    pskb_node->skb = skb;
    pskb_node->next = *queue_head;
    pskb_node->prev = NULL;

    if(!(*queue_tail))
    {
        (*queue_tail) = pskb_node;
    }

    (*queue_tail)->queuesize = (*queue_tail)->queuesize + 1;

    if(!(*queue_head))
    {
        (*queue_head) = pskb_node;
    }
    else
    {
        (*queue_head)->prev = pskb_node;
    }

    (*queue_head) = pskb_node;

    return 0;
}

struct sk_buff* MDev_EMAC_dequeue(struct skb_node **queue_head, struct skb_node **queue_tail)
{
    struct sk_buff *skb;

    if(!(*queue_tail))
    {
        return NULL;
    }
    else
    {
        skb = (*queue_tail)->skb;

        /* If the size of queue is equal to 1*/
        if(*queue_head == *queue_tail)
        {
            kfree(*queue_tail);
            *queue_tail = NULL;
            *queue_head = NULL;
        }
        else
        {
            (*queue_tail)->prev->queuesize = (*queue_tail)->queuesize - 1;
            *queue_tail = (*queue_tail)->prev;
            kfree((*queue_tail)->next);
            (*queue_tail)->next = NULL;
        }
    }

    return skb;
}

void MDev_EMAC_bottom_tx_task(struct work_struct *work)
{
    struct sk_buff *skb;
    unsigned long flags;
    dma_addr_t skb_addr;
    struct EMAC_private *LocPtr = container_of(work, struct EMAC_private, tx_task);
    struct net_device *dev = LocPtr->dev;


    spin_lock_irqsave(LocPtr->lock, flags);

    while(NETDEV_TX_OK == MDev_EMAC_CheckTSR())
    {
        skb = MDev_EMAC_dequeue(&(LocPtr->tx_queue_head), &(LocPtr->tx_queue_tail));

        if(skb == NULL)
        {
            break;
        }
        else
        {

            skb_addr = get_tx_addr();
            memcpy((void*)skb_addr, skb->data, skb->len);

        #ifdef CONFIG_MP_ETHERNET_MSTAR_ICMP_ENHANCE
            MDev_EMAC_Period_Retry(skb, dev);
        #endif
            LocPtr->stats.tx_bytes += skb->len;

            Chip_Flush_Cache_Range(skb_addr, skb->len); //Chip_Flush_Memory_Range(0, 0xFFFFFFFFUL);

            //MDrv_EMAC_DumpMem(tx_swq->skb_physaddr,skb->len);

            //Set address of the data in the Transmit Address register //
            MHal_EMAC_Write_TAR(skb_addr - RAM_VA_PA_OFFSET - MIU0_BUS_BASE);

            // Set length of the packet in the Transmit Control register //
            MHal_EMAC_Write_TCR(skb->len);

            dev->trans_start = jiffies;
            dev_kfree_skb_irq(skb);
        }
    }
    spin_unlock_irqrestore(LocPtr->lock, flags);
}


//-------------------------------------------------------------------------------------------------
//Transmit packet.
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_tx (struct sk_buff *skb, struct net_device *dev)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    unsigned long flags;
    int _ret;

    spin_lock_irqsave(LocPtr->lock, flags);

    /*Add skb to tx software queue*/
    _ret = MDev_EMAC_enqueue(&(LocPtr->tx_queue_head), &(LocPtr->tx_queue_tail), skb);

    /*Triger tx_task*/
    schedule_work(&LocPtr->tx_task);

    spin_unlock_irqrestore(LocPtr->lock, flags);

    if(_ret == 1)
    {
        return NETDEV_TX_BUSY;
    }
    else
    {
        return NETDEV_TX_OK;
    }
}

#elif defined(TX_SKB_PTR)

//-------------------------------------------------------------------------------------------------
//Transmit packet.
//-------------------------------------------------------------------------------------------------
#ifdef TX_SW_QUEUE

//  read skb from TX_SW_QUEUE to HW,
//  !!!! NO SPIN LOCK INSIDE !!!!
static void _MDev_EMAC_tx_read_TX_SW_QUEUE(int txIdleCount,struct net_device *dev,int intr)
{
	struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
//	int txIdleCount=MDev_EMAC_GetTXFIFOIdle();

	while(txIdleCount>0){

	    struct tx_ring *txq=&(LocPtr->tx_swq[LocPtr->tx_rdidx]);
		if(txq->used == TX_DESC_WROTE)
		{
		    Chip_Flush_Cache_Range((size_t)txq->skb->data,txq->skb->len);
			MHal_EMAC_Write_TAR(txq->skb_physaddr  - MIU0_BUS_BASE );
			MHal_EMAC_Write_TCR(txq->skb->len);
			txq->used=TX_DESC_READ;
			LocPtr->tx_rdidx ++;
			if(TX_SW_QUEUE_SIZE==LocPtr->tx_rdidx)
			{
				LocPtr->tx_rdwrp++;
				LocPtr->tx_rdidx =0;
			}
		}else{
			break;
		}
		txIdleCount--;
	}

}

//  clear skb from TX_SW_QUEUE
//  !!!! NO SPIN LOCK INSIDE !!!!
static void _MDev_EMAC_tx_clear_TX_SW_QUEUE(int txIdleCount,struct net_device *dev,int intr)
{

	struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    int clearcnt=0;
	int fifoCount=0;//LocPtr->tx_rdidx-LocPtr->tx_clidx;
	if(0==txIdleCount)return;

	fifoCount=LocPtr->tx_rdidx-LocPtr->tx_clidx;
	if(fifoCount<0)fifoCount=LocPtr->tx_rdidx+(TX_SW_QUEUE_SIZE-LocPtr->tx_clidx);

	/*
	 * "fifoCount" is the count of the packets that has been sent to the EMAC HW.
	 * "(TX_FIFO_SIZE-txIdleCount)" is the packet count that has not yet been sent out completely by EMAC HW
	 */
    clearcnt = fifoCount-(TX_FIFO_SIZE-txIdleCount);
	if((clearcnt > TX_FIFO_SIZE) || (clearcnt < 0)){
		printk(KERN_ERR"fifoCount in _MDev_EMAC_tx_clear_TX_SW_QUEUE() ERROR!! fifoCount=%d intr=%d, %d, %d, %d, %d\n",
            fifoCount,intr,LocPtr->tx_rdidx,LocPtr->tx_clidx,txIdleCount,TX_FIFO_SIZE);
	}

	while(clearcnt>0)
	{
	    struct tx_ring *txq=&(LocPtr->tx_swq[LocPtr->tx_clidx]);
		if(TX_DESC_READ==txq->used)
		{

			dma_unmap_single(&dev->dev, txq->skb_physaddr, txq->skb->len, DMA_TO_DEVICE);

			LocPtr->stats.tx_bytes += txq->skb->len;
            tx_bytes_per_timer += txq->skb->len;

			dev_kfree_skb_any(txq->skb);

			txq->used = TX_DESC_CLEARED;
			txq->skb=NULL;
			LocPtr->tx_clidx++;
			if(TX_SW_QUEUE_SIZE==LocPtr->tx_clidx)
			{
				LocPtr->tx_clwrp++;
				LocPtr->tx_clidx =0;
			}
		}else{
			break;
		}
		clearcnt--;
	}
}

static void _MDev_EMAC_tx_reset_TX_SW_QUEUE(struct net_device* netdev)
{
	struct EMAC_private *LocPtr;
	u32 i=0;
	LocPtr = (struct EMAC_private*) netdev_priv(netdev);
    for (i=0;i<TX_SW_QUEUE_SIZE;i++)
    {
        if(LocPtr->tx_swq[i].skb != NULL)
        {
            dma_unmap_single(&netdev->dev, LocPtr->tx_swq[i].skb_physaddr, LocPtr->tx_swq[i].skb->len, DMA_TO_DEVICE);
            dev_kfree_skb_any(LocPtr->tx_swq[i].skb);

        }
        LocPtr->tx_swq[i].skb = NULL;
        LocPtr->tx_swq[i].used = TX_DESC_CLEARED;
        LocPtr->tx_swq[i].skb_physaddr = 0;
    }
    LocPtr->tx_clidx = 0;
    LocPtr->tx_wridx = 0;
    LocPtr->tx_rdidx = 0;
    LocPtr->tx_clwrp = 0;
	LocPtr->tx_wrwrp = 0;
	LocPtr->tx_rdwrp = 0;
	LocPtr->tx_swq_full_cnt=0;
}

#endif
static int MDev_EMAC_tx (struct sk_buff *skb, struct net_device *dev)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    unsigned long flags;
    dma_addr_t skb_addr;
    int txIdleCount=0;

    if(g_emac_led_orange!=-1 && g_emac_led_green!=-1)
    {
        if(g_emac_led_count++ > g_emac_led_flick_speed){
            MDrv_GPIO_Set_Low(g_emac_led_orange);
            g_emac_led_count=0;
        }
    }

    spin_lock_irqsave(LocPtr->lock, flags);
#ifdef CONFIG_MSTAR_EEE
    MHal_EMAC_Disable_EEE_TX();
#endif

    if (netif_queue_stopped(emac_dev)){
        EMAC_ERR("netif_queue_stopped\n");
        spin_unlock_irqrestore(LocPtr->lock, flags);
        return NETDEV_TX_BUSY;
    }
    if (!netif_carrier_ok(emac_dev)){
        EMAC_ERR("netif_carrier_off\n");
        spin_unlock_irqrestore(LocPtr->lock, flags);
        return NETDEV_TX_BUSY;
    }
    if (skb->len > EMAC_MTU)
    {
        EMAC_ERR("Wrong Tx len:%u\n", skb->len);
        spin_unlock_irqrestore(LocPtr->lock, flags);
        return NETDEV_TX_BUSY;
    }
#if defined(PACKET_DUMP)
    if(1==txDumpCtrl && NULL!=txDumpFile)
    {
        txDumpFileLength+=msys_kfile_write(txDumpFile,txDumpFileLength,skb->data,skb->len);
    }
    else if(2==txDumpCtrl && NULL!=txDumpFile)
    {
        msys_kfile_close(txDumpFile);
        txDumpFile=NULL;
        printk(KERN_WARNING"close emac tx_dump file '%s', len=0x%08X...\n",txDumpFileName,txDumpFileLength);
    }
#endif

#ifdef TX_SW_QUEUE
    {

        int queue_size=0;
        //FIFO full, loop until HW empty then try again
        //This is an abnormal condition as the upper network tx_queue should already been stopped by "netif_stop_queue(dev)" in code below
        if( LocPtr->tx_swq[LocPtr->tx_wridx].used > TX_DESC_CLEARED)
        {
            printk(KERN_ERR"ABNORMAL !! %d, %d, %d, %d\n",LocPtr->tx_wridx,LocPtr->tx_rdidx,LocPtr->tx_clidx, LocPtr->tx_swq[LocPtr->tx_wridx].used );
            BUG();
        }

        //map skbuffer for DMA
        skb_addr = dma_map_single(&dev->dev, skb->data, skb->len, DMA_TO_DEVICE);

        if (dma_mapping_error(&dev->dev, skb_addr))
        {
            dev_kfree_skb_any(skb);
            printk(KERN_ERR"ERROR!![%s]%d\n",__FUNCTION__,__LINE__);
            dev->stats.tx_dropped++;

            goto out_unlock;
        }

        LocPtr->tx_swq[LocPtr->tx_wridx].skb = skb;
        LocPtr->tx_swq[LocPtr->tx_wridx].skb_physaddr= skb_addr;
        LocPtr->tx_swq[LocPtr->tx_wridx].used = TX_DESC_WROTE;
        LocPtr->tx_wridx ++;
        if(TX_SW_QUEUE_SIZE==LocPtr->tx_wridx)
        {
            LocPtr->tx_wridx=0;
            LocPtr->tx_wrwrp++;
        }


        //if FIFO is full, netif_stop_queue
        if( LocPtr->tx_swq[LocPtr->tx_wridx].used > TX_DESC_CLEARED)
        {
            LocPtr->tx_swq_full_cnt++;
            netif_stop_queue(dev);
        }

        queue_size=LocPtr->tx_wridx-LocPtr->tx_clidx;
        if(queue_size<0)queue_size=LocPtr->tx_wridx+(TX_SW_QUEUE_SIZE-LocPtr->tx_clidx);
        if(max_tx_packet_count<queue_size) max_tx_packet_count=queue_size;

        // clear & read to HW FIFO
        txIdleCount=MDev_EMAC_GetTXFIFOIdle();

        _MDev_EMAC_tx_clear_TX_SW_QUEUE(txIdleCount,dev,TX_SW_QUEUE_IN_GENERAL_TX);
        _MDev_EMAC_tx_read_TX_SW_QUEUE(txIdleCount,dev,TX_SW_QUEUE_IN_GENERAL_TX);
	}
#else

    //if buffer remains one space, notice upperr layer to block transmition.
    txIdleCount = MDev_EMAC_GetTXFIFOIdle();
    if( txIdleCount <= 0)
    {
        netif_stop_queue(dev);
        spin_unlock_irqrestore(LocPtr->lock, flags);
        return NETDEV_TX_BUSY;
    }

    //map skbuffer for DMA
    skb_addr = dma_map_single(&dev->dev, skb->data, skb->len, DMA_TO_DEVICE);
    if (dma_mapping_error(&dev->dev, skb_addr))
    {
        dev_kfree_skb_any(skb);
        printk(KERN_ERR"ERROR!![%s]%d\n",__FUNCTION__,__LINE__);
        dev->stats.tx_dropped++;

        goto out_unlock;
    }

    /* push skb into software queue*/
    /* kick out the earliest skb in queue */
    if( LocPtr->tx_swq[LocPtr->tx_wridx].used ==1 && LocPtr->tx_swq[LocPtr->tx_wridx].skb != NULL)
    {
        dma_unmap_single(&dev->dev, LocPtr->tx_swq[LocPtr->tx_wridx].skb_physaddr, skb->len, DMA_TO_DEVICE);
        dev_kfree_skb_any(LocPtr->tx_swq[LocPtr->tx_wridx].skb);
        LocPtr->tx_swq[LocPtr->tx_wridx].used = 0;
        LocPtr->stats.tx_bytes += skb->len;
        tx_bytes_per_timer += skb->len;
    }
    LocPtr->tx_swq[LocPtr->tx_wridx].skb = skb;
    LocPtr->tx_swq[LocPtr->tx_wridx].used = 1;
    LocPtr->tx_swq[LocPtr->tx_wridx].skb_physaddr= skb_addr;
    LocPtr->tx_wridx ++;
    LocPtr->tx_wridx = LocPtr->tx_wridx % TX_RING_SIZE;

    //set DMA address and trigger DMA
    Chip_Flush_Cache_Range((size_t)skb->data, skb->len);
    MHal_EMAC_Write_TAR( skb_addr - MIU0_BUS_BASE );
    // Set length of the packet in the Transmit Control register //
    MHal_EMAC_Write_TCR(skb->len);

#endif
    if(min_tx_fifo_idle_count>txIdleCount) min_tx_fifo_idle_count=txIdleCount;

    dev->trans_start = jiffies;


out_unlock:
    spin_unlock_irqrestore(LocPtr->lock, flags);
    return NETDEV_TX_OK;
}



#else
//-------------------------------------------------------------------------------------------------
//Transmit packet.
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_tx (struct sk_buff *skb, struct net_device *dev)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    unsigned long flags;
    dma_addr_t skb_addr;

    spin_lock_irqsave(LocPtr->lock, flags);

#ifdef CONFIG_MSTAR_EEE
    MHal_EMAC_Disable_EEE_TX();
#endif

    if (skb->len > EMAC_MTU)
    {
        EMAC_ERR("Wrong Tx len:%u\n", skb->len);
        spin_unlock_irqrestore(LocPtr->lock, flags);
        return NETDEV_TX_BUSY;
    }

    //find out the last one buffer, notice upperr layer to block transmit.
    //if( MDev_EMAC_GetTXFIFOIdle() <= 1)
    //    netif_stop_queue(dev);


    if (NETDEV_TX_OK != MDev_EMAC_CheckTSR())
    {
	    spin_unlock_irqrestore(LocPtr->lock, flags);
	    return NETDEV_TX_BUSY; //check
    }



    skb_addr = get_tx_addr();

    memcpy((void*)skb_addr, skb->data, skb->len);

    #ifdef CONFIG_MP_ETHERNET_MSTAR_ICMP_ENHANCE
    MDev_EMAC_Period_Retry(skb, dev);
    #endif

    if (!skb_addr)
    {
        dev_err(NULL,
                "dma map 2 failed (%p, %i). Dropping packet\n",
                skb->data, skb->len);
		spin_unlock_irqrestore(LocPtr->lock, flags);
        return -ENOMEM;
    }

    // Store packet information (to free when Tx completed) //
    //LocPtr->skb = skb;
    //LocPtr->skb_length =(int) skb->len;
    LocPtr->stats.tx_bytes += skb->len;

    Chip_Flush_Cache_Range(skb_addr, skb->len); //Chip_Flush_Memory_Range(0, 0xFFFFFFFFUL);

    //MDrv_EMAC_DumpMem(tx_swq->skb_physaddr,skb->len);
    //Set address of the data in the Transmit Address register //
    MHal_EMAC_Write_TAR(skb_addr - RAM_VA_PA_OFFSET - MIU0_BUS_BASE);

    // Set length of the packet in the Transmit Control register //
    MHal_EMAC_Write_TCR(skb->len);

    dev->trans_start = jiffies;

    dev_kfree_skb_irq(skb);

    spin_unlock_irqrestore(LocPtr->lock, flags);
    return NETDEV_TX_OK;
}
#endif

#if 0 //ajtest
static int ajtest_recv_count=0;
static ssize_t ajtest_recv_count_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{

    int val = simple_strtoul(buf, NULL, 10);
    if(0==val)
    {

        ajtest_recv_count=0;
    }
    return count;
}
static ssize_t ajtest_recv_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{

    return sprintf(buf, "%d\n", ajtest_recv_count);
}
DEVICE_ATTR(ajtest_recv_count, 0644, ajtest_recv_count_show, ajtest_recv_count_store);


unsigned int aj_checksum( unsigned char * buffer, long length )
{

     long index;
     unsigned int checksum;

     for( index = 0L, checksum = 0; index < length; index++)
     {
         checksum += (int)buffer[index];
         checksum &= 0x0FFFFFFF;
//         if(index<10)printf("%d %d\n",buffer[index],checksum);

     }
     return checksum;

}

static int aj_check(char *pData, int pktlen, int flag)
{
    int res=0;
    if(pktlen>32)
    {
        char *dbuf=(pData+0x36);

        if(0x51==dbuf[0] && 0x58==dbuf[1] && 0x91==dbuf[2] && 0x58==dbuf[3])
        {
            int dlen=0;
            int pktid=0;
            int pktcs=0;//((buf[ret-1])<<24) + (buf[ret-2]<<16)+(buf[ret-3]<<8) + (buf[ret-4]);
            int cs=0;
            unsigned char *buf=(unsigned char *)(dbuf+8);

            dlen=(dbuf[7]<<24) + (dbuf[6]<<16)+(dbuf[5]<<8) + (dbuf[4]);
            pktid=(dbuf[11]<<24) + (dbuf[10]<<16)+(dbuf[9]<<8) + (dbuf[8]);

//                                printf("pktCount: %08d\n",pktCount);
            pktcs=((buf[dlen-1])<<24) + (buf[dlen-2]<<16)+(buf[dlen-3]<<8) + (buf[dlen-4]);
            cs=aj_checksum(buf,dlen-4);
            if(pktcs!=cs)
            {
//                    int j=0;
//                    unsigned int lc=0;
                printk(KERN_WARNING"<[!! AJ_ERR %d, %d: 0x%08X, 0x%08X, %d ]>\n\n",ajtest_recv_count,pktid,cs,pktcs,flag);
                res=-1;
//                    for(j=0;j<ret;j++)
//                    {
//                        if(0==(j%16)){printf("0x%08X: ",lc++);}
//                        printf("%02X ",buf[j]);
//                        if(15==(j%16))printf("\n");
//                    }
//                    printf("cs2=0x%08X\n",checksum(buf,ret-4));
//                    printf("\n");
//                    while(1);
            }
//          else
//          {
//              printk(KERN_WARNING"<[AJOK %d:%d: 0x%08X, 0x%08X, %d ]>\n",ajss_recv_count,pktid,cs,pktcs,flag);
//          }

            ajtest_recv_count++;
        }
    }
    return res;
}
#endif


//-------------------------------------------------------------------------------------------------
// Extract received frame from buffer descriptors and sent to upper layers.
// (Called from interrupt context)
// (Disable RX software discriptor)
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_rx (struct net_device *dev)
{
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    struct recv_desc_bufs *dlist;
    unsigned char *p_recv;
    u32 pktlen;
    u32 retval=0;
    u32 received=0;
    struct sk_buff *skb;

#ifdef RX_ZERO_COPY
    u32 RBQP_offset;
    u32 RBQP_rx_skb_addr = 0;
//    unsigned long flags;
//    int cidx=0;
#endif

    if(g_emac_led_orange!=-1 && g_emac_led_green!=-1)
    {
        if(g_emac_led_count++ > g_emac_led_flick_speed){
            MDrv_GPIO_Set_Low(g_emac_led_orange);
            g_emac_led_count=0;
        }
    }


    dlist = LocPtr->dlist ;
    // If any Ownership bit is 1, frame received.
    do
    {
#ifdef RX_ZERO_COPY
        char* pData;
#endif
         Chip_Inv_Cache_Range((size_t)(&(dlist->descriptors[LocPtr->rxBuffIndex].addr)), sizeof(dlist->descriptors[LocPtr->rxBuffIndex].addr) );

        if(!((dlist->descriptors[LocPtr->rxBuffIndex].addr) & EMAC_DESC_DONE))
        {
            break;
        }

        p_recv = (char *) ((((dlist->descriptors[LocPtr->rxBuffIndex].addr) & 0xFFFFFFFFUL) + RAM_VA_PA_OFFSET + MIU0_BUS_BASE) &~(EMAC_DESC_DONE | EMAC_DESC_WRAP));

        pktlen = dlist->descriptors[LocPtr->rxBuffIndex].size & 0x7ffUL;    /* Length of frame including FCS */

    #if RX_THROUGHPUT_TEST
    	receive_bytes += pktlen;
    #endif

        if (pktlen > EMAC_MTU || pktlen < 64)
        {
            EMAC_ERR("drop packet!!(pktlen = %d)", pktlen);
            dlist->descriptors[LocPtr->rxBuffIndex].addr  &= ~EMAC_DESC_DONE;  /* reset ownership bit */
            Chip_Flush_Cache_Range((unsigned long)(&(dlist->descriptors[LocPtr->rxBuffIndex].addr)), sizeof(dlist->descriptors[LocPtr->rxBuffIndex].addr));

            //wrap after last buffer //
            LocPtr->rxBuffIndex++;
            if (LocPtr->rxBuffIndex == MAX_RX_DESCR)
            {
                LocPtr->rxBuffIndex = 0;
            }
            LocPtr->stats.rx_length_errors++;
            LocPtr->stats.rx_errors++;
            LocPtr->stats.rx_dropped++;
            continue;
        }

#ifndef RX_ZERO_COPY

        skb = alloc_skb (pktlen + 6, GFP_ATOMIC);
		if (!skb)
		{
			LocPtr->stats.rx_dropped += 1;
            printk(KERN_ERR"ERROR!![%s]%d\n",__FUNCTION__,__LINE__);;
			return -ENOMEM;
		}
        skb_reserve (skb, 2); /* Align IP on 16 byte boundaries */

        Chip_Inv_Cache_Range((unsigned long)p_recv, pktlen);  //Chip_Inv_Cache_Range_VA_PA((unsigned long)p_recv,(unsigned long)(p_recv - RAM_VA_PA_OFFSET) ,pktlen);

        /* 'skb_put()' points to the start of sk_buff data area. */
        memcpy(skb_put(skb, pktlen), p_recv, pktlen);
#else
        pktlen -= 4; /* Remove FCS */
        skb = rx_skb[LocPtr->rxBuffIndex];
        pData=skb_put(rx_skb[LocPtr->rxBuffIndex], pktlen);
        Chip_Inv_Cache_Range((unsigned long)pData, pktlen);

        //ajtest
        /* below code is used to find the offset of ajtest header in incoming packet
        for(cidx=0;cidx<pktlen;cidx++)
        {
          if((cidx+4)>=pktlen)
          {
              break;
          }
          if(0x51==pData[cidx] && 0x58==pData[cidx+1] && 0x91==pData[cidx+2] && 0x58==pData[cidx+3])
          {
              printk(KERN_WARNING"cidx: 0x%08X\n",cidx);
              break;
          }
        }
        */
#if defined(PACKET_DUMP)
        if(1==rxDumpCtrl && NULL!=rxDumpFile)
        {
            rxDumpFileLength+=msys_kfile_write(rxDumpFile,rxDumpFileLength,pData,pktlen);
        }
        else if(2==rxDumpCtrl && NULL!=rxDumpFile)
        {
            msys_kfile_close(rxDumpFile);
            rxDumpFile=NULL;
            printk(KERN_WARNING"close emac rx_dump file '%s', len=0x%08X...\n",rxDumpFileName,rxDumpFileLength);
        }
#endif
#endif

        skb->dev = dev;
        skb->protocol = eth_type_trans (skb, dev);
        //skb->len = pktlen;
        dev->last_rx = jiffies;
        LocPtr->stats.rx_bytes += pktlen;
#if defined (SOFTWARE_TX_FLOW_CONTROL)
        //check the pause ctl frame protocol //MAC_CONTROL_TYPE=0x8808
        if (((MAC_CONTROL_TYPE&0xFF)==((skb->protocol>>8)&0xFF)) && \
           (((MAC_CONTROL_TYPE>>8)&0xFF)==(skb->protocol&0xFF)))
        {
            int pause_time = 0;
            int pause_time_to_jiffies = 0;
            unsigned int mac_ctl_opcode = 0;
            #if 0
            int i = 0;
            unsigned char *pcc = p_recv;

            printk("Get ethernet flow control data:\n");
            for (i=0; i<((pktlen<0x20)?pktlen:0x20); i++)
            {
                printk("%02x ",*(pcc+i));
                if (i==7) printk(" ");
                if ((0x10<i) && (0==(i-7)%0x10)) printk(" ");
                if ((i>0) && (0 == (i+1)%0x10)) printk("\n");
            }
            //printk("\n");
            #endif

            //Get the pause ctl frame opcode //MAC_CONTROL_OPCODE=0x0001
            mac_ctl_opcode = (((*(p_recv+14))<<8)&0xFF00) + ((*(p_recv+15))&0xFF);
            if (MAC_CONTROL_OPCODE == mac_ctl_opcode)
            {
                pause_time = (((*(p_recv+16))<<8)&0xFF00) + ((*(p_recv+17))&0xFF);

                spin_lock_irq(&emac_flow_ctl_lock);
                if (0 == eth_pause_cmd_enable)
                {
                    netif_stop_queue (dev);

                    //init_timer(&EMAC_flow_ctl_timer);
                    //EMAC_flow_ctl_timer.data = EMAC_FLOW_CTL_TMR;
                    if (EMAC_SPEED_100 == ThisBCE.speed)
                    {
                        pause_time_to_jiffies = (pause_time/PAUSE_TIME_DIVISOR_100M)+((0==(pause_time%PAUSE_TIME_DIVISOR_100M))?0:1);
                        EMAC_flow_ctl_timer.expires = jiffies + pause_time_to_jiffies;
                    }
                    else if (EMAC_SPEED_10 == ThisBCE.speed)
                    {
                        pause_time_to_jiffies = (pause_time/PAUSE_TIME_DIVISOR_10M)+((0==(pause_time%PAUSE_TIME_DIVISOR_10M))?0:1);
                        EMAC_flow_ctl_timer.expires = jiffies + pause_time_to_jiffies;
                    }
                    else
                    {
                        printk(KERN_WARNING"[%s:%d] Get emac speed(%d) error!\n", __func__, __LINE__, g_emac_speed);
                    }

                    //printk(KERN_WARNING"[pause ctl] jiffies=0x%lx, expires==0x%lx, pause_time_to_jiffies=%d\n",
                    //                      jiffies, EMAC_flow_ctl_timer.expires, pause_time_to_jiffies);
                    add_timer(&EMAC_flow_ctl_timer);

                    eth_pause_cmd_enable = 1;
                }
                else
                {
                    unsigned long expires_new = 0;

                    if (EMAC_SPEED_100 == ThisBCE.speed)
                    {
                        pause_time_to_jiffies = (pause_time/PAUSE_TIME_DIVISOR_100M)+((0==(pause_time%PAUSE_TIME_DIVISOR_100M))?0:1);
                        expires_new = jiffies + pause_time_to_jiffies;
                    }
                    else if (EMAC_SPEED_10 == ThisBCE.speed)
                    {
                        pause_time_to_jiffies = (pause_time/PAUSE_TIME_DIVISOR_10M)+((0==(pause_time%PAUSE_TIME_DIVISOR_10M))?0:1);
                        expires_new = jiffies + pause_time_to_jiffies;
                    }
                    else
                    {
                        printk(KERN_WARNING"[%s:%d] Get emac speed(%d) error!\n", __func__, __LINE__, g_emac_speed);
                    }

                    mod_timer(&EMAC_flow_ctl_timer, expires_new);
                }
                spin_unlock_irq(&emac_flow_ctl_lock);
                printk(KERN_WARNING"[%s:%d] Get pause time(0x%04x) from MAC PAUSE frame, eth_pause_cmd_enable=%d\n",
                                              __func__, __LINE__, pause_time, eth_pause_cmd_enable);
            }
        }
#endif
    #if RX_THROUGHPUT_TEST
        kfree_skb(skb);
    #else

    #ifdef RX_CHECKSUM
        if(((dlist->descriptors[LocPtr->rxBuffIndex].size & EMAC_DESC_TCP ) || (dlist->descriptors[LocPtr->rxBuffIndex].size & EMAC_DESC_UDP )) && \
               (dlist->descriptors[LocPtr->rxBuffIndex].size & EMAC_DESC_IP_CSUM) && \
               (dlist->descriptors[LocPtr->rxBuffIndex].size & EMAC_DESC_TCP_UDP_CSUM) )
        {
            skb->ip_summed = CHECKSUM_UNNECESSARY;
        }
    else
        {
            skb->ip_summed = CHECKSUM_NONE;
        }
    #endif

    #ifdef ISR_BOTTOM_HALF
        retval = netif_rx_ni(skb);
    #elif defined MSTAR_EMAC_NAPI
        retval = netif_receive_skb(skb);
    #else
        retval = netif_rx (skb);
    #endif

        received++;
        #endif/*RX_THROUGHPUT_TEST*/

        if (dlist->descriptors[LocPtr->rxBuffIndex].size & EMAC_MULTICAST)
        {
            LocPtr->stats.multicast++;
        }

#ifdef RX_ZERO_COPY
        //dequeue_rx_buffer(LocPtr, &skb);
        //if (!skb) {
        //    printk(KERN_ERR"%d: rx_next:%d rx_next_fill:%d rx_current_fill:%d\n",__LINE__,LocPtr->rx_next, LocPtr->rx_next_fill, LocPtr->rx_current_fill);
        //    panic("Can't dequeue skb from buffer.");
        //}
        if (!(skb = alloc_skb (SOFTWARE_DESC_LEN, GFP_ATOMIC))) {
            printk(KERN_ERR"Can't alloc skb.[%s]%d\n",__FUNCTION__,__LINE__);;
			return -ENOMEM;
		}

        rx_skb[LocPtr->rxBuffIndex] = skb;
        rx_abso_addr[LocPtr->rxBuffIndex] = (u32)rx_skb[LocPtr->rxBuffIndex]->data;

        RBQP_offset = LocPtr->rxBuffIndex * RBQP_HW_BYTES;

        RBQP_rx_skb_addr = __virt_to_phys(rx_abso_addr[LocPtr->rxBuffIndex]) - MIU0_BUS_BASE;
        RBQP_rx_skb_addr |= EMAC_DESC_DONE;
        if(LocPtr->rxBuffIndex == (MAX_RX_DESCR-1))
            RBQP_rx_skb_addr |= EMAC_DESC_WRAP;
        MHal_EMAC_WritRam32(RAM_VA_PA_OFFSET, RBQP_BASE + RBQP_offset, RBQP_rx_skb_addr);
#endif

        dlist->descriptors[LocPtr->rxBuffIndex].addr  &= ~EMAC_DESC_DONE;  /* reset ownership bit */
        //Chip_Inv_Cache_Range_VA_PA((unsigned long)(&(dlist->descriptors[LocPtr->rxBuffIndex].addr)),(unsigned long)(&(dlist->descriptors[LocPtr->rxBuffIndex].addr)) - RAM_VA_PA_OFFSET ,sizeof(dlist->descriptors[LocPtr->rxBuffIndex].addr));
        Chip_Flush_Cache_Range((unsigned long)(&(dlist->descriptors[LocPtr->rxBuffIndex].addr)), sizeof(dlist->descriptors[LocPtr->rxBuffIndex].addr));


        //wrap after last buffer //
        LocPtr->rxBuffIndex++;
        if (LocPtr->rxBuffIndex == MAX_RX_DESCR)
        {
            LocPtr->rxBuffIndex = 0;
        }

#ifdef MSTAR_EMAC_NAPI
        if(received >= EMAC_NAPI_WEIGHT) {
            break;
        }
#endif

    }while(1);


#ifdef RX_ZERO_COPY
    //spin_lock_irqsave(LocPtr->lock, flags);
    //fill_rx_ring(emac_dev);
    //spin_unlock_irqrestore(LocPtr->lock, flags);
#endif

    if(received>max_rx_packet_count)max_rx_packet_count=received;
    return received;
}



//-------------------------------------------------------------------------------------------------
//MAC interrupt handler
//(Interrupt delay enable)
//-------------------------------------------------------------------------------------------------
#if defined ISR_BOTTOM_HALF
void MDev_EMAC_bottom_rx_task(struct work_struct *work)
{
    struct EMAC_private *LocPtr = container_of(work, struct EMAC_private, rx_task);
    struct net_device *dev = LocPtr->dev;

    mutex_lock(&(LocPtr->mutex));
    MDev_EMAC_rx(dev);
    mutex_unlock(&(LocPtr->mutex));
}
#endif
static int RBNA_detailed=0;
irqreturn_t MDev_EMAC_interrupt(int irq,void *dev_id)
{
    struct net_device *dev = (struct net_device *) dev_id;
    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(dev);
    u32 intstatus=0;
    unsigned long flags;
#ifdef TX_SW_QUEUE
    int txIdleCount=0;
#endif
#ifndef RX_ZERO_COPY
    u32 wp = 0;
#endif

    spin_lock_irqsave(LocPtr->lock, flags);
    LocPtr->irqcnt++;
#ifdef INT_JULIAN_D
    //MAC Interrupt Status register indicates what interrupts are pending.
    //It is automatically cleared once read.
    xoffsetValue = MHal_EMAC_Read_JULIAN_0108() & 0x0000FFFFUL;
    xReceiveNum += xoffsetValue&0xFFUL;
    if(xoffsetValue&0x8000UL)
    {
        LocPtr->xReceiveFlag = 1;
    }
#endif

#ifndef RX_ZERO_COPY
    wp = MHal_EMAC_Read_JULIAN_0100() & 0x00100000UL;
    if(wp)
    {
        EMAC_ERR("EMAC HW write invalid address\n");
    }
#endif


    ThisUVE.flagRBNA = 0;

    oldTime = getCurMs();
    while((LocPtr->xReceiveFlag == 1) || (intstatus = (MHal_EMAC_Read_ISR() & ~(MHal_EMAC_Read_IMR()) & EMAC_INT_MASK )) )
    {
        if (intstatus & EMAC_INT_RBNA)
        {
            struct recv_desc_bufs *dlist=LocPtr->dlist;
            int empty=0;
            int idx;
            LocPtr->stats.rx_missed_errors++;
            ThisUVE.flagRBNA = 1;
            LocPtr->xReceiveFlag = 1;
            //write 1 clear
            MHal_EMAC_Write_RSR(EMAC_BNA);
            if(RBNA_detailed>0)
            {
                u32 u32RBQP_Addr = MHal_EMAC_Read_RBQP()-(RBQP_BASE - MIU0_BUS_BASE);
                for(idx=0;idx<MAX_RX_DESCR;idx++)
                {
                    if(!((dlist->descriptors[idx].addr) & EMAC_DESC_DONE))
                    {
                        empty++;
                    }
                    else
                    {
                        printk(KERN_ERR"RBNA: [0x%X]\n",idx);
                    }

                }
                printk(KERN_ERR"RBNA: empty=0x%X, LocPtr->rxBuffIndex=0x%X, rx_missed_errors=%ld RBQP_offset=0x%x\n",empty,LocPtr->rxBuffIndex,LocPtr->stats.rx_missed_errors,u32RBQP_Addr);
            }
            //printk("RBNA\n");
        }

        // Transmit complete //
        if (intstatus & EMAC_INT_TCOM)
        {
            LocPtr->tx_irqcnt++;
            if(g_emac_led_orange!=-1 && g_emac_led_green!=-1)
            {
                if(g_emac_led_count++ > g_emac_led_flick_speed){
                    MDrv_GPIO_Set_High(g_emac_led_orange);
                    g_emac_led_count=0;
                }
            }

        #ifdef TX_SOFTWARE_QUEUE
            /*Triger tx_task*/
            schedule_work(&LocPtr->tx_task);
        #endif

            // The TCOM bit is set even if the transmission failed. //
            if (intstatus & (EMAC_INT_TUND | EMAC_INT_RTRY))
            {
                LocPtr->stats.tx_errors += 1;
                if(intstatus & EMAC_INT_TUND)
                {
                    //write 1 clear
                    MHal_EMAC_Write_TSR(EMAC_UND);

                    //Reset TX engine
                    MDev_EMAC_TxReset();
                    EMAC_ERR ("Transmit TUND error, TX reset\n");
                }
            }
            else
            {
                LocPtr->retx_count = 0;
            }

        #if TX_THROUGHPUT_TEST
            MDev_EMAC_tx(pseudo_packet, emac_dev);
        #endif
        #if defined(TX_SKB_PTR)
        #ifdef TX_SW_QUEUE
            if( ((LocPtr->ep_flag&EP_FLAG_SUSPENDING)==0) && netif_queue_stopped (dev))
            {
                netif_wake_queue(dev);
            }
            txIdleCount=MDev_EMAC_GetTXFIFOIdle();
            while(txIdleCount>0 && (LocPtr->tx_rdidx != LocPtr->tx_wridx))
            {

                _MDev_EMAC_tx_clear_TX_SW_QUEUE(txIdleCount,dev,TX_SW_QUEUE_IN_IRQ);
                _MDev_EMAC_tx_read_TX_SW_QUEUE(txIdleCount,dev,TX_SW_QUEUE_IN_IRQ);
                txIdleCount=MDev_EMAC_GetTXFIFOIdle();
            }
        #else
            if( (MDev_EMAC_GetTXFIFOIdle()>0) && netif_queue_stopped (dev))
                netif_wake_queue(dev);

            if (((LocPtr->ep_flag&EP_FLAG_SUSPENDING)==0) && netif_queue_stopped (dev))
                netif_wake_queue(dev);
        #endif
        #endif
        }

        if(intstatus&EMAC_INT_DONE)
        {
            ThisUVE.flagISR_INT_DONE = 0x01UL;
        }

        //Overrun //
        if(intstatus & EMAC_INT_ROVR)
        {
            LocPtr->stats.rx_over_errors++;
            contiROVR++;
            //write 1 clear
            MHal_EMAC_Write_RSR(EMAC_RSROVR);
            EMAC_ERR ("==> %s: ROVR error %u times!\n", dev->name, contiROVR);
            #ifndef TX_SKB_PTR
            if (contiROVR < 3)
            {
                MDev_EMAC_Send_PausePkt(dev);
            }
            else
            {
                MDev_EMAC_SwReset(dev);
                #ifdef INT_JULIAN_D
                xReceiveNum = 0;
                #endif
            }
            #endif
        }
        else
        {
            contiROVR = 0;
        }

        //if(xReceiveNum != 0)
        //{
        //    LocPtr->xReceiveFlag = 1;
        //}

        if(intstatus&EMAC_INT_RCOM)
        {
            LocPtr->xReceiveFlag = 1;
        }

        // Receive complete //
        if(LocPtr->xReceiveFlag == 1)
        {
            if(g_emac_led_orange!=-1 && g_emac_led_green!=-1)
            {
                if(g_emac_led_count++ > g_emac_led_flick_speed){
                    MDrv_GPIO_Set_High(g_emac_led_orange);
                    g_emac_led_count=0;
                }

            }

            LocPtr->xReceiveFlag = 0;
        #ifdef MSTAR_EMAC_NAPI
            /* Receive packets are processed by poll routine. If not running start it now. */
            if (napi_schedule_prep(&LocPtr->napi)) {
                MDEV_EMAC_DISABLE_RX_REG();
                __napi_schedule(&LocPtr->napi);
            }
        #elif defined ISR_BOTTOM_HALF
            /*Triger rx_task*/
            schedule_work(&LocPtr->rx_task);
        #else
            spin_unlock_irqrestore(LocPtr->lock, flags);
            MDev_EMAC_rx(dev);
            spin_lock_irqsave(LocPtr->lock, flags);
        #endif
        }
    }
    spin_unlock_irqrestore(LocPtr->lock, flags);
    return IRQ_HANDLED;
}

#ifdef MSTAR_EMAC_NAPI

// Enable MAC interrupts
static void MDEV_EMAC_ENABLE_RX_REG(void)
{
    u32 uRegVal;
    //printk( KERN_ERR "[EMAC] %s\n" , __FUNCTION__);
#ifndef INT_JULIAN_D
    // enable MAC interrupts
    MHal_EMAC_Write_IER(EMAC_INT_RCOM);
#else
    // enable delay interrupt
    uRegVal = MHal_EMAC_Read_JULIAN_0104();
    uRegVal |= 0x00000080UL;
    MHal_EMAC_Write_JULIAN_0104(uRegVal);
#endif
}

// Disable MAC interrupts
static void MDEV_EMAC_DISABLE_RX_REG(void)
{
    u32 uRegVal;
    //printk( KERN_ERR "[EMAC] %s\n" , __FUNCTION__);
#ifndef INT_JULIAN_D
    MHal_EMAC_Write_IDR(EMAC_INT_RCOM);
#else
    // disable delay interrupt
    uRegVal = MHal_EMAC_Read_JULIAN_0104();
    uRegVal &= ~(0x00000080UL);
    MHal_EMAC_Write_JULIAN_0104(uRegVal);
#endif
}

static int MDev_EMAC_napi_poll(struct napi_struct *napi, int budget)
{
    struct EMAC_private  *LocPtr = container_of(napi, struct EMAC_private,napi);
    struct net_device *dev = LocPtr->dev;
    unsigned long flags = 0;
    int work_done = 0;

    //mutex_lock(&(LocPtr->mutex));
    work_done = MDev_EMAC_rx(dev);
    //mutex_unlock(&(LocPtr->mutex));

    /* If budget not fully consumed, exit the polling mode */
    if (work_done < budget) {
        napi_complete(napi);
        // enable MAC interrupt
        spin_lock_irqsave(LocPtr->lock, flags);
        MDEV_EMAC_ENABLE_RX_REG();
        spin_unlock_irqrestore(LocPtr->lock, flags);
    }

    return work_done;
}


#endif

#ifdef LAN_ESD_CARRIER_INTERRUPT
irqreturn_t MDev_EMAC_interrupt_cable_unplug(int irq,void *dev_id)
{
    //printk( KERN_ERR "[EMAC] %s\n" , __FUNCTION__);
    if (netif_carrier_ok(emac_dev))
        netif_carrier_off(emac_dev);
    if (!netif_queue_stopped(emac_dev))
        netif_stop_queue(emac_dev);
    ThisBCE.connected = 0;

    #ifdef TX_SW_QUEUE
    _MDev_EMAC_tx_reset_TX_SW_QUEUE(emac_dev);
    #endif

    if(g_emac_led_orange!=-1 && g_emac_led_green!=-1)
    {
        MDrv_GPIO_Set_Low(g_emac_led_orange);
        MDrv_GPIO_Set_Low(g_emac_led_green);
    }

    return IRQ_HANDLED;
}
#endif

//-------------------------------------------------------------------------------------------------
// EMAC Hardware register set
//-------------------------------------------------------------------------------------------------
void MDev_EMAC_HW_init(void)
{
    u32 word_ETH_CTL = 0x00000000UL;
    u32 word_ETH_CFG = 0x00000800UL;
    u32 uJulian104Value = 0;
    u32 uNegPhyVal = 0;
#ifdef SOFTWARE_DESCRIPTOR
    u32 idxRBQP = 0;
    u32 RBQP_offset = 0;
#ifdef RX_ZERO_COPY
    struct sk_buff *skb = NULL;
    u32 RBQP_rx_skb_addr = 0;
//    struct EMAC_private *LocPtr = (struct EMAC_private*) netdev_priv(emac_dev);
#endif
#endif

    // (20071026_CHARLES) Disable TX, RX and MDIO:   (If RX still enabled, the RX buffer will be overwrited)
    MHal_EMAC_Write_CTL(word_ETH_CTL);
    MHal_EMAC_Write_BUFF(0x00000000UL);
#ifndef SOFTWARE_DESCRIPTOR
    // Init RX --------------------------------------------------------------
    memset((u8*)RAM_VA_PA_OFFSET + RX_BUFFER_BASE, 0x00UL, RX_BUFFER_SIZE);

    MHal_EMAC_Write_BUFF((RX_BUFFER_BASE | RX_BUFFER_SEL) - MIU0_BUS_BASE);
    MHal_EMAC_Write_RDPTR(0x00000000UL);
    MHal_EMAC_Write_WRPTR(0x00000000UL);
#endif
#ifdef INT_JULIAN_D
    //Reg_rx_frame_cyc[15:8] -0xFF range 1~255
    //Reg_rx_frame_num[7:0]  -0x05 receive frames per INT.
    //0x80 Enable interrupt delay mode.
    //register 0x104 receive counter need to modify smaller for ping
    //Modify bigger(need small than 8) for throughput
    uJulian104Value = JULIAN_104_VAL&(~0x80);//0xFF050080;
    MHal_EMAC_Write_JULIAN_0104(uJulian104Value);
#else
    // Enable Interrupts ----------------------------------------------------
    uJulian104Value = 0x00000000UL;
    MHal_EMAC_Write_JULIAN_0104(uJulian104Value);
#endif
    // Set MAC address ------------------------------------------------------
    MHal_EMAC_Write_SA1_MAC_Address(ThisBCE.sa1[0], ThisBCE.sa1[1], ThisBCE.sa1[2], ThisBCE.sa1[3], ThisBCE.sa1[4], ThisBCE.sa1[5]);
    MHal_EMAC_Write_SA2_MAC_Address(ThisBCE.sa2[0], ThisBCE.sa2[1], ThisBCE.sa2[2], ThisBCE.sa2[3], ThisBCE.sa2[4], ThisBCE.sa2[5]);
    MHal_EMAC_Write_SA3_MAC_Address(ThisBCE.sa3[0], ThisBCE.sa3[1], ThisBCE.sa3[2], ThisBCE.sa3[3], ThisBCE.sa3[4], ThisBCE.sa3[5]);
    MHal_EMAC_Write_SA4_MAC_Address(ThisBCE.sa4[0], ThisBCE.sa4[1], ThisBCE.sa4[2], ThisBCE.sa4[3], ThisBCE.sa4[4], ThisBCE.sa4[5]);

#ifdef SOFTWARE_DESCRIPTOR
    // Initialize "Receive Buffer Queue Pointer"
    MHal_EMAC_Write_RBQP(RBQP_BASE - MIU0_BUS_BASE);
    // Initialize Receive Buffer Descriptors
    memset((u8*)RAM_VA_PA_OFFSET + RBQP_BASE, 0x00UL, RBQP_SIZE);        // Clear for max(8*1024)bytes (max:1024 descriptors)

    #ifdef RX_CHECKSUM
    uJulian104Value=uJulian104Value | (RX_CHECKSUM_ENABLE | SOFTWARE_DESCRIPTOR_ENABLE);
    #else
    uJulian104Value=uJulian104Value | SOFTWARE_DESCRIPTOR_ENABLE;
    #endif
    MHal_EMAC_Write_JULIAN_0104(uJulian104Value);

#ifdef RX_ZERO_COPY
    for(idxRBQP = 0; idxRBQP < RBQP_LENG; idxRBQP++)
    {

        //dequeue_rx_buffer(LocPtr, &skb);
        //if (!skb) {
        //    skb = alloc_skb(SOFTWARE_DESC_LEN, GFP_ATOMIC);
        //}
        if (!(skb = alloc_skb (SOFTWARE_DESC_LEN, GFP_ATOMIC))) {
			printk("%s %d: alloc skb failed!\n",__func__, __LINE__);
            panic("can't alloc skb");
		}

        rx_skb[idxRBQP] = skb;

        rx_abso_addr[idxRBQP] = (u32)rx_skb[idxRBQP]->data;
        RBQP_offset = idxRBQP * 8;
        if(idxRBQP < (RBQP_LENG - 1))
        {
			RBQP_rx_skb_addr = __virt_to_phys(rx_abso_addr[idxRBQP]) - MIU0_BUS_BASE;
            MHal_EMAC_WritRam32(RAM_VA_PA_OFFSET, RBQP_BASE + RBQP_offset, RBQP_rx_skb_addr);
        }
        else
        {
			RBQP_rx_skb_addr = __virt_to_phys(rx_abso_addr[idxRBQP]) - MIU0_BUS_BASE + EMAC_DESC_WRAP;
            MHal_EMAC_WritRam32(RAM_VA_PA_OFFSET, RBQP_BASE + RBQP_offset, RBQP_rx_skb_addr);
        }
    }
#else
    for(idxRBQP = 0; idxRBQP < RBQP_LENG; idxRBQP++)
    {

        RBQP_offset = idxRBQP * RBQP_HW_BYTES;
        if(idxRBQP < (RBQP_LENG - 1))
        {
            MHal_EMAC_WritRam32(RAM_VA_PA_OFFSET, RBQP_BASE + RBQP_offset, (RX_BUFFER_BASE - MIU0_BUS_BASE + SOFTWARE_DESC_LEN * idxRBQP));
        }
        else
        {
            MHal_EMAC_WritRam32(RAM_VA_PA_OFFSET, RBQP_BASE + RBQP_offset, (RX_BUFFER_BASE - MIU0_BUS_BASE + SOFTWARE_DESC_LEN * idxRBQP + EMAC_DESC_WRAP));
        }

    }
#endif //#ifdef RX_ZERO_COPY
#endif //#ifdef SOFTWARE_DESCRIPTOR

    if (!ThisUVE.initedEMAC)
    {
#ifdef CONFIG_EMAC_PHY_RESTART_AN
        MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1000UL);
        MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1000UL);
        //MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1200UL);
#else
        MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x9000UL);
        MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1000UL);
#endif /* CONFIG_EMAC_PHY_RESTART_AN */

#if defined (SOFTWARE_TX_FLOW_CONTROL)
        MHal_EMAC_write_phy(phyaddr, MII_ADVERTISE, 0xDE1UL); //BIT0|BIT5~8|BIT10|BIT11 //pause disable transmit, enable receive
#endif
#if !defined( CONFIG_ETHERNET_ALBANY)
        //force to set 10M on FPGA
        MHal_EMAC_write_phy(phyaddr, MII_ADVERTISE, 0x0061UL);
        MHal_EMAC_write_phy(phyaddr, MII_CTRL1000, 0x0000UL);
        mdelay(10);
        MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1200UL);
#endif

        // IMPORTANT: Run NegotiationPHY() before writing REG_ETH_CFG.
        uNegPhyVal = MHal_EMAC_NegotiationPHY();
        if(uNegPhyVal == 0x01UL)
        {
            ThisUVE.flagMacTxPermit = 0x01UL;
            ThisBCE.duplex = 1;

        }
        else if(uNegPhyVal == 0x02UL)
        {
            ThisUVE.flagMacTxPermit = 0x01UL;
            ThisBCE.duplex = 2;
        }

        // ETH_CFG Register -----------------------------------------------------
        word_ETH_CFG = 0x00000800UL;        // Init: CLK = 0x2
        // (20070808) IMPORTANT: REG_ETH_CFG:bit1(FD), 1:TX will halt running RX frame, 0:TX will not halt running RX frame.
        // If always set FD=0, no CRC error will occur. But throughput maybe need re-evaluate.
        // IMPORTANT: (20070809) NO_MANUAL_SET_DUPLEX : The real duplex is returned by "negotiation"
        if(ThisBCE.speed     == EMAC_SPEED_100) word_ETH_CFG |= 0x00000001UL;
        if(ThisBCE.duplex    == 2)              word_ETH_CFG |= 0x00000002UL;
        if(ThisBCE.cam       == 1)              word_ETH_CFG |= 0x00000200UL;
        if(ThisBCE.rcv_bcast == 0)              word_ETH_CFG |= 0x00000020UL;
        if(ThisBCE.rlf       == 1)              word_ETH_CFG |= 0x00000100UL;

        MHal_EMAC_Write_CFG(word_ETH_CFG);
        // ETH_CTL Register -----------------------------------------------------
        word_ETH_CTL = 0x0000000CUL;                          // Enable transmit and receive : TE + RE = 0x0C (Disable MDIO)
        if(ThisBCE.wes == 1) word_ETH_CTL |= 0x00000080UL;
        MHal_EMAC_Write_CTL(word_ETH_CTL);
        MHal_EMAC_Write_JULIAN_0100(JULIAN_100_VAL);

        ThisUVE.flagPowerOn = 1;
        ThisUVE.initedEMAC  = 1;
    }

    MHal_EMAC_HW_init();

}


//-------------------------------------------------------------------------------------------------
// EMAC init Variable
//-------------------------------------------------------------------------------------------------
extern phys_addr_t memblock_start_of_DRAM(void);
extern phys_addr_t memblock_size_of_first_region(void);

static phys_addr_t MDev_EMAC_VarInit(void)
{
    phys_addr_t alloRAM_PA_BASE;
    phys_addr_t alloRAM_SIZE;

    char addr[6];
    u32 HiAddr, LoAddr;
    phys_addr_t *alloRAM_VA_BASE;
    MSYS_DMEM_INFO mem_info;
    int ret=0;
    /********************************************************************/
    /*alloRAM_VA_BASE++buffer_offset       private date(no use)         */
    /*      RBQP_BASE++RBQP_LENG*0x8       Descripter table             */
    /* RX_BUFFER_BASE++RBQP_LENG*0x800     rx buffer                    */
    /* TX_BUFFER_BASE++RBQP_LENG*0x8       tx Descripter(no use)        */
    /*    TX_SKB_BASE++TX_RING_SIZE*0x800  tx skb buffer for TX_QUEUE_4 */
    /********************************************************-***********/

#ifdef RX_ZERO_COPY
    alloRAM_SIZE = RBQP_SIZE+SOFTWARE_DESC_LEN*TX_RING_SIZE;
#else
    alloRAM_SIZE = RBQP_SIZE+RX_BUFFER_SIZE+SOFTWARE_DESC_LEN*TX_RING_SIZE;
#endif

    mem_info.length = alloRAM_SIZE;
    strcpy(mem_info.name, "EMAC_BUFF");
    if((ret=msys_request_dmem(&mem_info)))
    {
    	panic("unable to locate DMEM for EMAC alloRAM!! error=%d\n",ret);
    }
    alloRAM_PA_BASE = mem_info.phys;
    alloRAM_VA_BASE = (phys_addr_t *)((size_t)mem_info.kvirt);

    EMAC_DBG("alloRAM_VA_BASE=0x%zx alloRAM_PA_BASE=0x%zx\n  alloRAM_SIZE=0x%zx\n", (size_t) alloRAM_VA_BASE,(size_t) alloRAM_PA_BASE,(size_t)alloRAM_SIZE);
    BUG_ON(!alloRAM_VA_BASE);

#ifndef RX_ZERO_COPY
    //Add Write Protect
    MHal_EMAC_Write_Protect(alloRAM_PA_BASE - MIU0_BUS_BASE, alloRAM_SIZE);
#endif
    memset((phys_addr_t *)alloRAM_VA_BASE,0x00UL,alloRAM_SIZE);

    //buffer_offset  = 0;//(sizeof(struct EMAC_private) + 0x3FFFUL) & ~0x3FFFUL;  //karl20151021:no used
    RAM_ALLOC_SIZE    = alloRAM_SIZE;
    RAM_VA_BASE       = ((phys_addr_t)alloRAM_VA_BASE );   // IMPORTANT: Let lowest 14 bits as zero.
    RAM_PA_BASE       = ((phys_addr_t)alloRAM_PA_BASE );   // IMPORTANT: Let lowest 14 bits as zero.
    RAM_VA_PA_OFFSET  = RAM_VA_BASE - RAM_PA_BASE;  // IMPORTANT_TRICK_20070512

    RBQP_BASE         = RAM_PA_BASE;
#ifndef RX_ZERO_COPY
    RX_BUFFER_BASE    = RBQP_BASE + RBQP_SIZE;
    TX_SKB_BASE       = RX_BUFFER_BASE + RX_BUFFER_SIZE;     //TX_BUFFER_BASE + MAX_RX_DESCR * sizeof(struct rbf_t);
#else
    TX_SKB_BASE       = RBQP_BASE + RBQP_SIZE;     //TX_BUFFER_BASE + MAX_RX_DESCR * sizeof(struct rbf_t);
#endif

    EMAC_DBG("RAM_VA_BASE=0x%08x\n", RAM_VA_BASE);
    EMAC_DBG("RAM_PA_BASE=0x%08x\n", RAM_PA_BASE);
    EMAC_DBG("RAM_VA_PA_OFFSET=0x%08x\n", RAM_VA_PA_OFFSET);
    EMAC_DBG("RBQP_BASE=0x%08x size=0x%x\n", RBQP_BASE, RBQP_SIZE);
#ifndef RX_ZERO_COPY
    EMAC_DBG("RX_BUFFER_BASE=0x%08x size=0x%x\n", RX_BUFFER_BASE, RX_BUFFER_SIZE);
#endif
    EMAC_DBG("TX_SKB_BASE=0x%08x size=0x%x\n", TX_SKB_BASE,SOFTWARE_DESC_LEN*TX_RING_SIZE);

    memset(&ThisBCE,0x00UL,sizeof(BasicConfigEMAC));
    memset(&ThisUVE,0x00UL,sizeof(UtilityVarsEMAC));

    ThisBCE.wes       	= 0;             		// 0:Disable, 1:Enable (WR_ENABLE_STATISTICS_REGS)
    ThisBCE.duplex 		= 2;                    // 1:Half-duplex, 2:Full-duplex
    ThisBCE.cam			= 0;                 	// 0:No CAM, 1:Yes
    ThisBCE.rcv_bcast	= 0;                  	// 0:No, 1:Yes
    ThisBCE.rlf  		= 0;                 	// 0:No, 1:Yes receive long frame(1522)
    ThisBCE.rcv_bcast   = 1;
    ThisBCE.speed       = EMAC_SPEED_100;

    // Check if bootloader set address in Specific-Address 1 //
    HiAddr = MHal_EMAC_get_SA1H_addr();
    LoAddr = MHal_EMAC_get_SA1L_addr();

    addr[0] = (LoAddr & 0xffUL);
    addr[1] = (LoAddr & 0xff00UL) >> 8;
    addr[2] = (LoAddr & 0xff0000UL) >> 16;
    addr[3] = (LoAddr & 0xff000000UL) >> 24;
    addr[4] = (HiAddr & 0xffUL);
    addr[5] = (HiAddr & 0xff00UL) >> 8;

    if (is_valid_ether_addr (addr))
    {
        memcpy (ThisBCE.sa1, &addr, 6);
    }
    else
    {
        // Check if bootloader set address in Specific-Address 2 //
        HiAddr = MHal_EMAC_get_SA2H_addr();
        LoAddr = MHal_EMAC_get_SA2L_addr();
        addr[0] = (LoAddr & 0xffUL);
        addr[1] = (LoAddr & 0xff00UL) >> 8;
        addr[2] = (LoAddr & 0xff0000UL) >> 16;
        addr[3] = (LoAddr & 0xff000000UL) >> 24;
        addr[4] = (HiAddr & 0xffUL);
        addr[5] = (HiAddr & 0xff00UL) >> 8;

        if (is_valid_ether_addr (addr))
        {
            memcpy (ThisBCE.sa1, &addr, 6);
        }
        else
        {
            ThisBCE.sa1[0]      = MY_MAC[0];
            ThisBCE.sa1[1]      = MY_MAC[1];
            ThisBCE.sa1[2]      = MY_MAC[2];
            ThisBCE.sa1[3]      = MY_MAC[3];
            ThisBCE.sa1[4]      = MY_MAC[4];
            ThisBCE.sa1[5]      = MY_MAC[5];
        }
    }
    ThisBCE.connected = 0;
#if defined (SOFTWARE_TX_FLOW_CONTROL)
    ThisBCE.sa2[0] = ETH_PAUSE_FRAME_DA_MAC[0];
    ThisBCE.sa2[1] = ETH_PAUSE_FRAME_DA_MAC[1];
    ThisBCE.sa2[2] = ETH_PAUSE_FRAME_DA_MAC[2];
    ThisBCE.sa2[3] = ETH_PAUSE_FRAME_DA_MAC[3];
    ThisBCE.sa2[4] = ETH_PAUSE_FRAME_DA_MAC[4];
    ThisBCE.sa2[5] = ETH_PAUSE_FRAME_DA_MAC[5];
#endif
    return (phys_addr_t)alloRAM_VA_BASE;
}

#ifdef CONFIG_NET_POLL_CONTROLLER
static void MDev_EMAC_netpoll(struct net_device *dev)
{
	unsigned long flags;

	local_irq_save(flags);
	MDev_EMAC_interrupt(dev->irq, dev);
	local_irq_restore(flags);
}
#endif

//-------------------------------------------------------------------------------------------------
// Initialize the ethernet interface
// @return TRUE : Yes
// @return FALSE : FALSE
//-------------------------------------------------------------------------------------------------

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
static const struct net_device_ops mstar_lan_netdev_ops = {
        .ndo_open               = MDev_EMAC_open,
        .ndo_stop               = MDev_EMAC_close,
        .ndo_start_xmit         = MDev_EMAC_tx,
        .ndo_set_mac_address    = MDev_EMAC_set_mac_address,
        .ndo_set_rx_mode        = MDev_EMAC_set_rx_mode,
        .ndo_do_ioctl           = MDev_EMAC_ioctl,
        .ndo_get_stats          = MDev_EMAC_stats,
#ifdef CONFIG_NET_POLL_CONTROLLER
        .ndo_poll_controller    = MDev_EMAC_netpoll,
#endif

};

static int MDev_EMAC_get_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    struct EMAC_private *LocPtr =(struct EMAC_private *) netdev_priv(dev);

    mii_ethtool_gset (&LocPtr->mii, cmd);

	return 0;
}

static int MDev_EMAC_set_settings(struct net_device *dev, struct ethtool_cmd *cmd)
{
    struct EMAC_private *LocPtr =(struct EMAC_private *) netdev_priv(dev);

    mii_ethtool_sset (&LocPtr->mii, cmd);

    return 0;
}

static int MDev_EMAC_nway_reset(struct net_device *dev)
{
    struct EMAC_private *LocPtr =(struct EMAC_private *) netdev_priv(dev);

    mii_nway_restart (&LocPtr->mii);

    return 0;
}

static u32 MDev_EMAC_get_link(struct net_device *dev)
{
    u32	u32data;
    struct EMAC_private *LocPtr =(struct EMAC_private *) netdev_priv(dev);

    u32data = mii_link_ok (&LocPtr->mii);

    return u32data;
}

static const struct ethtool_ops ethtool_ops = {
    .get_settings = MDev_EMAC_get_settings,
    .set_settings = MDev_EMAC_set_settings,
    .nway_reset   = MDev_EMAC_nway_reset,
    .get_link     = MDev_EMAC_get_link,
};

#endif

static ssize_t dlist_info_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
	u32 input;

    input = simple_strtoul(buf, NULL, 10);

    if(0==input)
    {
        RBNA_detailed=0;
    }
    else if(1==input)
    {
        RBNA_detailed=1;
    }
    else if(2==input)
    {
        max_rx_packet_count=0;
        max_tx_packet_count=0;
        min_tx_fifo_idle_count=0xffff;
    }
    return count;
}

static ssize_t dlist_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
	int idx=0;
	struct EMAC_private *LocPtr = (struct EMAC_private *) netdev_priv(emac_dev);
	struct recv_desc_bufs *dlist=LocPtr->dlist;
	int empty=0;
	int max=MAX_RX_DESCR;
    u32 u32RBQP_Addr=0;

	for(idx=0;idx<MAX_RX_DESCR;idx++)
	{
		if(!((dlist->descriptors[idx].addr) & EMAC_DESC_DONE))
		{
			empty++;
		}

	}
    u32RBQP_Addr =( MHal_EMAC_Read_RBQP()-(RBQP_BASE - MIU0_BUS_BASE))/RBQP_HW_BYTES;


	str += scnprintf(str, end - str, "RBQP_size=0x%x, empty=0x%x, LocPtr->rxBuffIndex=0x%X, max_rx_packet_count=%d,u32RBQP_Addr=0x%x\n",
						max,empty,LocPtr->rxBuffIndex,max_rx_packet_count,u32RBQP_Addr);


	return (str - buf);
}
DEVICE_ATTR(dlist_info, 0644, dlist_info_show, dlist_info_store);

static ssize_t info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
    //struct EMAC_private *LocPtr = (struct EMAC_private *) netdev_priv(emac_dev);

    //str += scnprintf(str, end - str, "%s %s\n", __DATE__, __TIME__);
    str += scnprintf(str, end - str, "RAM_ALLOC_SIZE=0x%08x\n", RAM_ALLOC_SIZE);
    str += scnprintf(str, end - str, "RAM_VA_BASE=0x%08x\n", RAM_VA_BASE);
    str += scnprintf(str, end - str, "RAM_PA_BASE=0x%08x\n", RAM_PA_BASE);
    str += scnprintf(str, end - str, "RAM_VA_PA_OFFSET=0x%08x\n", RAM_VA_PA_OFFSET);
    str += scnprintf(str, end - str, "RBQP_BASE=0x%08x size=0x%x\n", RBQP_BASE, RBQP_SIZE);
#ifndef RX_ZERO_COPY
    str += scnprintf(str, end - str, "RX_BUFFER_BASE=0x%08x size=0x%x\n", RX_BUFFER_BASE, RX_BUFFER_SIZE);
#endif
    str += scnprintf(str, end - str, "TX_SKB_BASE=0x%08x size=0x%x\n", TX_SKB_BASE,SOFTWARE_DESC_LEN*TX_RING_SIZE);

#ifdef MSTAR_EMAC_NAPI
    str += scnprintf(str, end - str, "NAPI enabled, NAPI_weight=%d\n", EMAC_NAPI_WEIGHT);
#endif
#ifdef RX_ZERO_COPY
    str += scnprintf(str, end - str, "ZERO_COPY enabled\n");
#endif
#ifdef TX_SKB_PTR
    str += scnprintf(str, end - str, "TX_SKB_PTR enabled\n");
#endif
#ifdef TX_SW_QUEUE
    str += scnprintf(str, end - str, "TX_SW_QUEUE enabled, TX_SW_QUEUE_SIZE=%d\n", TX_SW_QUEUE_SIZE);
#endif
#ifdef NEW_TX_QUEUE_128
    str += scnprintf(str, end - str, "NEW_TX_QUEUE_128 enabled, NEW_TX_QUEUE_SIZE=%d\n",NEW_TX_QUEUE_SIZE);
#endif
#ifdef NEW_TX_QUEUE_INTERRUPT_THRESHOLD
    str += scnprintf(str, end - str, "NEW_TX_QUEUE_INTERRUPT_THRESHOLD enabled\n");
#endif

	if (str > buf)
		str--;

	str += scnprintf(str, end - str, "\n");

	return (str - buf);
}
DEVICE_ATTR(info, 0444, info_show, NULL);


//struct timeval proc_read_time;
static ssize_t tx_sw_queue_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	char *str = buf;
	char *end = buf + PAGE_SIZE;
#ifdef TX_SKB_PTR
    struct EMAC_private *LocPtr = (struct EMAC_private *) netdev_priv(emac_dev);
    int idleCount=0;

#if defined(TX_SW_QUEUE)
//    unsigned long flags=0;
//    spin_lock_irqsave(LocPtr->lock,flags);

	str += scnprintf(str, end - str, "tx_wr[%d:%d], tx_rd[%d:%d], tx_cl[%d:%d]\ntx_swq_full_cnt=%d \nmax_tx_packet_count=%d\n",
			LocPtr->tx_wrwrp,LocPtr->tx_wridx,
			LocPtr->tx_rdwrp,LocPtr->tx_rdidx,
			LocPtr->tx_clwrp,LocPtr->tx_clidx,
			LocPtr->tx_swq_full_cnt,
			max_tx_packet_count);

//	spin_unlock_irqrestore(LocPtr->lock,flags);
#endif
    idleCount=MDev_EMAC_GetTXFIFOIdle();
	str += scnprintf(str, end - str,
	"netif_queue_stopped=%d \n idleCount=%d \n irqcnt=%d, tx_irqcnt=%d \n tx_bytes_per_timerbak=%d \n min_tx_fifo_idle_count=%d \n LocPtr->tx_wridx=%d\n",
			netif_queue_stopped(emac_dev),
			idleCount, LocPtr->irqcnt, LocPtr->tx_irqcnt,
			tx_bytes_per_timerbak,
			min_tx_fifo_idle_count, LocPtr->tx_wridx);
#endif

	if (str > buf)
		str--;

	str += scnprintf(str, end - str, "\n");

	return (str - buf);
}

DEVICE_ATTR(tx_sw_queue_info, 0444, tx_sw_queue_info_show, NULL);


static ssize_t reverse_led_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    u32 input;

    input = simple_strtoul(buf, NULL, 10);
    MHal_EMAC_Set_Reverse_LED(input);
    return count;
}
static ssize_t reverse_led_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    u8 u8reg=0;
    u8reg = MHal_EMAC_Get_Reverse_LED();
    return sprintf(buf, "%d\n", u8reg);
}
DEVICE_ATTR(reverse_led, 0644, reverse_led_show, reverse_led_store);


static ssize_t check_link_time_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    u32 input;

    input = simple_strtoul(buf, NULL, 10);
    gu32CheckLinkTime = input;
    return count;
}
static ssize_t check_link_time_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32CheckLinkTime);
}
DEVICE_ATTR(check_link_time, 0644, check_link_time_show, check_link_time_store);

static ssize_t check_link_timedis_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    u32 input;

    input = simple_strtoul(buf, NULL, 10);
    gu32CheckLinkTimeDis = input;
    return count;
}
static ssize_t check_link_timedis_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gu32CheckLinkTimeDis);
}
DEVICE_ATTR(check_link_timedis, 0644, check_link_timedis_show, check_link_timedis_store);

static ssize_t sw_led_flick_speed_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    u32 input;

    input = simple_strtoul(buf, NULL, 10);
    g_emac_led_flick_speed = input;
    return count;
}
static ssize_t sw_led_flick_speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return sprintf(buf, "LED flick speed, the smaller the faster\n%d\n", g_emac_led_flick_speed);
}
DEVICE_ATTR(sw_led_flick_speed, 0644, sw_led_flick_speed_show, sw_led_flick_speed_store);

extern void MHal_EMAC_phy_trunMax(void);
extern void MHal_EMAC_trim_phy(void);
static ssize_t turndrv_store(struct device *dev, struct device_attribute *attr,const char *buf, size_t count)
{
    u32 input;
    if(!strncmp(buf, "0",strlen("0")))
    {
        MHal_EMAC_trim_phy();
        return count;
    }

    if(!strncmp(buf, "max",strlen("max")))
    {
        MHal_EMAC_phy_trunMax();
        return count;
    }

    if(!strncmp(buf, "f10t",strlen("10t")))
    {
        //force to set 10M on FPGA
        MHal_EMAC_write_phy(phyaddr, MII_ADVERTISE, 0x0061UL);
        mdelay(10);
        MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1200UL);
        return count;
    }
    if(!strncmp(buf, "an",strlen("an")))
    {
        //force to set 10M on FPGA
        MHal_EMAC_write_phy(phyaddr, MII_ADVERTISE, 0x01e1UL);
        mdelay(10);
        MHal_EMAC_write_phy(phyaddr, MII_BMCR, 0x1200UL);
        return count;
    }

    input = simple_strtoul(buf, NULL, 10);

    return count;
}
static ssize_t turndrv_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    return 0;
}

DEVICE_ATTR(turndrv, 0644, turndrv_show, turndrv_store);

static int MDev_EMAC_setup (struct net_device *dev, unsigned long phy_type)
{
    struct EMAC_private *LocPtr;
    static int already_initialized = 0;
    dma_addr_t dmaaddr;
    phys_addr_t RetAddr;
#ifdef CONFIG_MSTAR_HW_TX_CHECKSUM
    u32 retval;
#endif

    if (already_initialized)
        return FALSE;

    LocPtr = (struct EMAC_private *) netdev_priv(dev);
    if (LocPtr == NULL)
    {
        free_irq (dev->irq, dev);
        EMAC_ERR("LocPtr fail\n");
        return -ENOMEM;
    }

#if defined ISR_BOTTOM_HALF
    /*Init tx and rx tasks*/
    INIT_WORK(&LocPtr->rx_task, MDev_EMAC_bottom_rx_task);
#endif
#ifdef TX_SOFTWARE_QUEUE
    INIT_WORK(&LocPtr->tx_task, MDev_EMAC_bottom_tx_task);
#endif

    LocPtr->dev = dev;
    RetAddr = MDev_EMAC_VarInit();
    if(!RetAddr)
    {
        EMAC_ERR("Var init fail!!\n");
        return FALSE;
    }

    dev->base_addr = (long) (EMAC_RIU_REG_BASE+REG_BANK_EMAC0*0x200);

    MDev_EMAC_HW_init();
#ifdef TX_SKB_PTR
#ifndef TX_SW_QUEUE
    {
        int i;

        for (i=0;i<TX_RING_SIZE;i++)
        {
            LocPtr->tx_swq[i].used = 0;
            LocPtr->tx_swq[i].skb = 0;
            LocPtr->tx_swq[i].skb_physaddr = 0;
        }
        //LocPtr->tx_rdidx = 0;
        LocPtr->tx_wridx = 0;
    }
#endif
#endif

    // Allocate memory for DMA Receive descriptors //
     LocPtr->dlist = (struct recv_desc_bufs *) (RBQP_BASE + RAM_VA_PA_OFFSET);

    if (LocPtr->dlist == NULL)
    {
        dma_free_noncoherent((void *)LocPtr, EMAC_ABSO_MEM_SIZE,&dmaaddr,0);//kfree (dev->priv);
        free_irq (dev->irq, dev);
        return -ENOMEM;
    }

    LocPtr->lock = &emac_lock;
    spin_lock_init (LocPtr->lock);
    mutex_init(&(LocPtr->mutex));

    ether_setup (dev);
#if LINUX_VERSION_CODE == KERNEL_VERSION(2,6,28)
    dev->open = MDev_EMAC_open;
    dev->stop = MDev_EMAC_close;
    dev->hard_start_xmit = MDev_EMAC_tx;
    dev->get_stats = MDev_EMAC_stats;
    dev->set_multicast_list = MDev_EMAC_set_rx_mode;
    dev->do_ioctl = MDev_EMAC_ioctl;
    dev->set_mac_address = MDev_EMAC_set_mac_address;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
    dev->netdev_ops = &mstar_lan_netdev_ops;
#endif
    dev->tx_queue_len = EMAC_MAX_TX_QUEUE;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
    ////SET_ETHTOOL_OPS(dev, &ethtool_ops);
    //EMAC_TODO("set Ethtool_ops\n");
    netdev_set_default_ethtool_ops(dev, &ethtool_ops);
#endif


    MDev_EMAC_get_mac_address (dev);    // Get ethernet address and store it in dev->dev_addr //
    MDev_EMAC_update_mac_address (dev); // Program ethernet address into MAC //
    spin_lock_irq (LocPtr->lock);
    MHal_EMAC_enable_mdi ();

    spin_unlock_irq (LocPtr->lock);

    //Support for ethtool //
    LocPtr->mii.dev = dev;
    LocPtr->mii.mdio_read = MDev_EMAC_mdio_read;
    LocPtr->mii.mdio_write = MDev_EMAC_mdio_write;
    already_initialized = 1;
#ifdef CONFIG_MSTAR_HW_TX_CHECKSUM
    retval = MHal_EMAC_Read_JULIAN_0104() | TX_CHECKSUM_ENABLE;
    MHal_EMAC_Write_JULIAN_0104(retval);
    dev->features |= NETIF_F_IP_CSUM;
#endif

    LocPtr->irqcnt=0;
    LocPtr->tx_irqcnt=0;

    /* clear interrupt status,  to avoid interrupt was set in uboot*/
    MHal_EMAC_Read_ISR();
    MHal_EMAC_Write_IDR(0xFFFF);
    #ifdef INT_JULIAN_D
    MHal_EMAC_Read_JULIAN_0108();
    #endif

	emac_dev->irq = irq_of_parse_and_map(dev->dev.of_node, 0);
    if (!emac_dev->irq)
    {
        EMAC_ERR("Get irq number0 error from DTS\n");
        return -EPROBE_DEFER;
    }

    //Install the interrupt handler //
    //Notes: Modify linux/kernel/irq/manage.c  /* interrupt.h */
    if (request_irq(dev->irq, MDev_EMAC_interrupt, 0/*SA_INTERRUPT*/, dev->name, dev))
        return -EBUSY;

#ifdef LAN_ESD_CARRIER_INTERRUPT
    val = irq_of_parse_and_map(dev->dev.of_node, 1);
    if (!val)
    {
        EMAC_ERR("Get irq number0 error from DTS\n");
        return -EPROBE_DEFER;
    }
    if (request_irq(val/*INT_FIQ_LAN_ESD+32*/, MDev_EMAC_interrupt_cable_unplug, 0/*SA_INTERRUPT*/, dev->name, dev))
        return -EBUSY;
#endif
    //Determine current link speed //
    spin_lock_irq (LocPtr->lock);
    (void) MDev_EMAC_update_linkspeed (dev);
    spin_unlock_irq (LocPtr->lock);


    LocPtr->mstar_class_emac_device = device_create(msys_get_sysfs_class(), NULL, MKDEV(MAJOR_EMAC_NUM, MINOR_EMAC_NUM), NULL, "emac");
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_tx_sw_queue_info);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_dlist_info);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_reverse_led);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_check_link_time);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_check_link_timedis);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_info);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_sw_led_flick_speed);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_turndrv);
#if 0//ajtest
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_ajtest_recv_count);
#endif
#if defined(PACKET_DUMP)
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_tx_dump);
    device_create_file(LocPtr->mstar_class_emac_device, &dev_attr_rx_dump);
#endif
    return 0;
}

//-------------------------------------------------------------------------------------------------
// Restar the ethernet interface
// @return TRUE : Yes
// @return FALSE : FALSE
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_SwReset(struct net_device *dev)
{
    struct EMAC_private *LocPtr = (struct EMAC_private *) netdev_priv(dev);
    u32 oldCFG, oldCTL;
    u32 retval;

    MDev_EMAC_get_mac_address (dev);
    oldCFG = MHal_EMAC_Read_CFG();
    oldCTL = MHal_EMAC_Read_CTL() & ~(EMAC_TE | EMAC_RE);

    //free tx skb
    if (LocPtr->retx_count)
    {
        if (LocPtr->skb)
        {
            dev_kfree_skb_irq(LocPtr->skb );
            LocPtr->skb = NULL;
        }
        if (netif_queue_stopped (dev))
            netif_wake_queue (dev);
    }
#ifdef RX_ZERO_COPY
	free_rx_skb();
#endif
    netif_stop_queue (dev);

    retval = MHal_EMAC_Read_JULIAN_0100();
    MHal_EMAC_Write_JULIAN_0100(retval & 0x00000FFFUL);
    MHal_EMAC_Write_JULIAN_0100(retval);

    MDev_EMAC_HW_init();
    MHal_EMAC_Write_CFG(oldCFG);
    MHal_EMAC_Write_CTL(oldCTL);
    MHal_EMAC_enable_mdi ();
    MDev_EMAC_update_mac_address (dev); // Program ethernet address into MAC //
    (void)MDev_EMAC_update_linkspeed (dev);
    MHal_EMAC_Write_IDR(0xFFFFFFFF);
    MHal_EMAC_Write_IER(gu32intrEnable);
    MDev_EMAC_start (dev);
    MDev_EMAC_set_rx_mode(dev);
    netif_start_queue (dev);
    contiROVR = 0;
    LocPtr->retx_count = 0;
#ifdef CONFIG_MSTAR_HW_TX_CHECKSUM
    retval = MHal_EMAC_Read_JULIAN_0104() | TX_CHECKSUM_ENABLE;
    MHal_EMAC_Write_JULIAN_0104(retval);
#endif
    EMAC_ERR("=> Take %lu ms to reset EMAC!\n", (getCurMs() - oldTime));
    return 0;
}

//-------------------------------------------------------------------------------------------------
// Detect MAC and PHY and perform initialization
//-------------------------------------------------------------------------------------------------
#if defined (CONFIG_OF)
static struct of_device_id mstaremac_of_device_ids[] = {
         {.compatible = "mstar-emac"},
         {},
};
#endif

static int MDev_EMAC_probe (struct net_device *dev)
{
    int detected;
    /* Read the PHY ID registers - try all addresses */
    detected = MDev_EMAC_setup(dev, MII_URANUS_ID);
    return detected;
}

//-------------------------------------------------------------------------------------------------
// EMAC Timer to detect cable pluged/unplugged
//-------------------------------------------------------------------------------------------------
static void MDev_EMAC_timer_callback(unsigned long value)
{
    int ret = 0;
    struct EMAC_private *LocPtr = (struct EMAC_private *) netdev_priv(emac_dev);
    static u32 bmsr, time_count = 0;

#if defined (SOFTWARE_TX_FLOW_CONTROL)
    if (EMAC_FLOW_CTL_TMR == value)
    {
        netif_wake_queue((struct net_device *)emac_dev);
        spin_lock_irq(&emac_flow_ctl_lock);
        eth_pause_cmd_enable = 0;
        spin_unlock_irq(&emac_flow_ctl_lock);
        return;
    }
#endif

#ifndef INT_JULIAN_D
    if (EMAC_RX_TMR == value)
    {
        MHal_EMAC_timer_callback(value);
        return;
    }
#endif

    spin_lock_irq (LocPtr->lock);
    ret = MDev_EMAC_update_linkspeed(emac_dev);

    if (!netif_queue_stopped(emac_dev))
    {


#ifdef TX_SW_QUEUE
    {
        int txIdleCount=MDev_EMAC_GetTXFIFOIdle();
        _MDev_EMAC_tx_clear_TX_SW_QUEUE(txIdleCount,emac_dev,TX_SW_QUEUE_IN_TIMER);
        _MDev_EMAC_tx_read_TX_SW_QUEUE(txIdleCount,emac_dev,TX_SW_QUEUE_IN_TIMER);
    }

#endif
    }
    spin_unlock_irq (LocPtr->lock);

    tx_bytes_per_timerbak = tx_bytes_per_timer;
    tx_bytes_per_timer = 0;

    if (0 == ret)
    {
        if (!ThisBCE.connected)
        {
            ThisBCE.connected = 1;
            netif_carrier_on(emac_dev);
            netif_start_queue(emac_dev);
            //EMAC_ERR("connected\n");
        }

        if(g_emac_led_orange!=-1 && g_emac_led_green!=-1)
        {
            MDrv_GPIO_Set_High(g_emac_led_orange);
            MDrv_GPIO_Set_High(g_emac_led_green);
        }

        // Link status is latched, so read twice to get current value //
        MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);
        MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);
        time_count = 0;
        spin_lock_irq (LocPtr->lock);
        phy_status_register = bmsr;
        spin_unlock_irq (LocPtr->lock);
        // Normally, time out sets 1 Sec.
        Link_timer.expires = jiffies + gu32CheckLinkTime;
    }
    else    //no link
    {
        if(ThisBCE.connected) {
            ThisBCE.connected = 0;
        }

        if(g_emac_led_orange!=-1 && g_emac_led_green!=-1)
        {
            MDrv_GPIO_Set_Low(g_emac_led_orange);
            MDrv_GPIO_Set_Low(g_emac_led_green);
        }

        // If disconnected is over 3 Sec, the real value of PHY's status register will report to application.
        if(time_count > CONFIG_DISCONNECT_DELAY_S*(HZ/gu32CheckLinkTimeDis)) {
            // Link status is latched, so read twice to get current value //
            MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);
            MHal_EMAC_read_phy (phyaddr, MII_BMSR, &bmsr);

            // Report to kernel.
            if (netif_carrier_ok(emac_dev))
                netif_carrier_off(emac_dev);
            if (!netif_queue_stopped(emac_dev))
                netif_stop_queue(emac_dev);
            spin_lock_irq (LocPtr->lock);
            phy_status_register = bmsr;
#ifdef TX_SW_QUEUE
            _MDev_EMAC_tx_reset_TX_SW_QUEUE(emac_dev);
#endif
            spin_unlock_irq (LocPtr->lock);
            ThisBCE.connected = 0;
            // Normally, time out is set 1 Sec.
            Link_timer.expires = jiffies + gu32CheckLinkTime;
        }
        else// if(time_count <= CONFIG_DISCONNECT_DELAY_S*10)
        {
            time_count++;
            // Time out is set 100ms. Quickly checks next phy status.
            Link_timer.expires = jiffies + gu32CheckLinkTimeDis;
            //EMAC_ERR("disconnect\n");
        }
    }
    add_timer(&Link_timer);
}

//-------------------------------------------------------------------------------------------------
// EMAC MACADDR Setup
//-------------------------------------------------------------------------------------------------

#define MACADDR_FORMAT "XX:XX:XX:XX:XX:XX"

static int __init macaddr_auto_config_setup(char *addrs)
{
    if (strlen(addrs) == strlen(MACADDR_FORMAT)
        && ':' == addrs[2]
        && ':' == addrs[5]
        && ':' == addrs[8]
        && ':' == addrs[11]
        && ':' == addrs[14]
       )
    {
        addrs[2]  = '\0';
        addrs[5]  = '\0';
        addrs[8]  = '\0';
        addrs[11] = '\0';
        addrs[14] = '\0';

        MY_MAC[0] = (u8)simple_strtoul(&(addrs[0]),  NULL, 16);
        MY_MAC[1] = (u8)simple_strtoul(&(addrs[3]),  NULL, 16);
        MY_MAC[2] = (u8)simple_strtoul(&(addrs[6]),  NULL, 16);
        MY_MAC[3] = (u8)simple_strtoul(&(addrs[9]),  NULL, 16);
        MY_MAC[4] = (u8)simple_strtoul(&(addrs[12]), NULL, 16);
        MY_MAC[5] = (u8)simple_strtoul(&(addrs[15]), NULL, 16);

        /* set back to ':' or the environment variable would be destoried */ // REVIEW: this coding style is dangerous
        addrs[2]  = ':';
        addrs[5]  = ':';
        addrs[8]  = ':';
        addrs[11] = ':';
        addrs[14] = ':';
    }

    return 1;
}

__setup("macaddr=", macaddr_auto_config_setup);

//-------------------------------------------------------------------------------------------------
// EMAC init module
//-------------------------------------------------------------------------------------------------
static int MDev_EMAC_ScanPhyAddr(void)
{
    unsigned char addr = 0;
    u32 value = 0;

    MHal_EMAC_Write_JULIAN_0100(JULIAN_100_VAL);

    MHal_EMAC_enable_mdi();
    do
    {
        MHal_EMAC_read_phy(addr, MII_BMSR, &value);
        if (0 != value && 0x0000FFFFUL != value)
        {

            EMAC_DBG("[ PHY Addr:%d ] ==> :%x\n", addr, value);
            break;
        }
    }while(++addr && addr < 32);
    MHal_EMAC_disable_mdi();
    phyaddr = addr;

	if (phyaddr >= 32)
	{
		EMAC_ERR("Wrong PHY Addr and reset to 0\n");
		phyaddr = 0;
		return -1;
	}
	return 0;
}

static void Rtl_Patch(void)
{
    u32 val;

    MHal_EMAC_read_phy(phyaddr, 25, &val);
    MHal_EMAC_write_phy(phyaddr, 25, 0x400UL);
    MHal_EMAC_read_phy(phyaddr, 25, &val);
}

static void MDev_EMAC_Patch_PHY(void)
{
    u32 val;

    MHal_EMAC_read_phy(phyaddr, 2, &val);
    if (RTL_8210 == val)
        Rtl_Patch();
}

#ifdef RX_ZERO_COPY
/*
static int dequeue_rx_buffer(struct EMAC_private *p, struct sk_buff **pskb)
{
	//p->rx_next = (p->rx_next + 1) % RX_RING_SIZE;
	p->rx_current_fill--;
	*pskb = __skb_dequeue(&p->rx_list);

//	if (gmac_debug)
//		GMAC_DBG("rx_next:%d rx_next_fill:%d rx_current_fill:%d\n",p->rx_next, p->rx_next_fill, p->rx_current_fill);
	return p->rx_current_fill;
}

static int fill_rx_ring(struct net_device *netdev)
{
    struct EMAC_private0 *p = netdev_priv(netdev);
    printk("%s :current fill:%d\n",__func__, p->rx_current_fill);

	while (p->rx_current_fill < RX_RING_SIZE)
	{
		struct sk_buff *skb;
        if (!(skb = alloc_skb (SOFTWARE_DESC_LEN, GFP_ATOMIC))) {
//			GMAC_DBG("%s %d: alloc skb failed! RX current fill:%d\n",__func__,__LINE__, p->rx_current_fill);
			break;
		}

//		skb_reserve(skb, NET_IP_ALIGN);
		__skb_queue_tail(&p->rx_list, skb);
		p->rx_current_fill++;
		p->rx_next_fill = (p->rx_next_fill + 1) % RX_RING_SIZE;
	}

//	if (unlikely(gmac_debug))
//		GMAC_DBG("%s Current fill:%d. rx next fill:%d\n",__func__, p->rx_current_fill, p->rx_next_fill);
    printk("%s :  RX current fill:%d\n",__func__, p->rx_current_fill);

	return p->rx_current_fill;
}
*/
#endif /*GMAC_RX_ZERO_COPY*/


static int MDev_EMAC_init(struct platform_device *pdev)
{
    struct EMAC_private *LocPtr;
    int ret;
/*
    if(MDrv_SYS_GetPowerStates()!= SYS_POWER_Supply_Full)
    {
        EMAC_DBG("Power state(%d) does not support ethernet\n",MDrv_SYS_GetPowerStates());
        //return -1;
    }
*/
    if(emac_dev)
        return -1;

    emac_dev = alloc_etherdev(sizeof(*LocPtr));
    LocPtr = netdev_priv(emac_dev);

    if (!emac_dev)
    {
        EMAC_ERR( KERN_ERR "No EMAC dev mem!\n" );
        return -ENOMEM;
    }
#ifdef RX_ZERO_COPY
	//skb_queue_head_init(&LocPtr->rx_list);
	//LocPtr->rx_next = 0;
	//LocPtr->rx_next_fill = 0;
	//LocPtr->rx_current_fill = 0;
	//fill_rx_ring(emac_dev);
#endif
#if defined (NEW_TX_QUEUE_128)
    MHal_EMAC_enable_new_TXQUEUE();
#endif

#if TX_THROUGHPUT_TEST
    printk("==========TX_THROUGHPUT_TEST===============");
    pseudo_packet = alloc_skb(SOFTWARE_DESC_LEN, GFP_ATOMIC);
    memcpy(pseudo_packet->data, (void *)packet_content, sizeof(packet_content));
    pseudo_packet->len = sizeof(packet_content);
#endif

#if RX_THROUGHPUT_TEST
    printk("==========RX_THROUGHPUT_TEST===============");
    init_timer(&RX_timer);

    RX_timer.data = EMAC_RX_TMR;
    RX_timer.function = RX_timer_callback;
    RX_timer.expires = jiffies + 20*EMAC_CHECK_LINK_TIME;
    add_timer(&RX_timer);
#endif

    MHal_EMAC_Power_On_Clk(&pdev->dev);

    init_timer(&EMAC_timer);
    init_timer(&Link_timer);

    EMAC_timer.data = EMAC_RX_TMR;
    EMAC_timer.function = MDev_EMAC_timer_callback;
    EMAC_timer.expires = jiffies;
#if defined (SOFTWARE_TX_FLOW_CONTROL)
    spin_lock_init (&emac_flow_ctl_lock);
    spin_lock_irq(&emac_flow_ctl_lock);
    eth_pause_cmd_enable = 0;
    spin_unlock_irq(&emac_flow_ctl_lock);

    init_timer(&EMAC_flow_ctl_timer);
    EMAC_flow_ctl_timer.data = EMAC_FLOW_CTL_TMR;
    EMAC_flow_ctl_timer.expires = jiffies;
    EMAC_flow_ctl_timer.function = MDev_EMAC_timer_callback;  /* timer handler */
#endif
    MHal_EMAC_Write_JULIAN_0100(JULIAN_100_VAL);

    if (0 > MDev_EMAC_ScanPhyAddr())
        goto end;

    MDev_EMAC_Patch_PHY();

#ifdef MSTAR_EMAC_NAPI
        netif_napi_add(emac_dev, &LocPtr->napi, MDev_EMAC_napi_poll, EMAC_NAPI_WEIGHT);
#endif

    emac_dev->dev.of_node = pdev->dev.of_node; //pass of_node to MDev_EMAC_setup()

    if (MDev_EMAC_probe (emac_dev))
        return -1;


    ret = register_netdev (emac_dev);

    if(!ret)
    {
        printk( KERN_ERR "[EMAC]Init EMAC success! (add delay in reset)\n" );
        return ret;
    }



end:
    free_netdev(emac_dev);
    emac_dev = 0;
    initstate = ETHERNET_TEST_INIT_FAIL;
    EMAC_ERR( KERN_ERR "Init EMAC error!\n" );
    return -1;
}
//-------------------------------------------------------------------------------------------------
// EMAC exit module
//-------------------------------------------------------------------------------------------------
static void MDev_EMAC_exit(void)
{
    if (emac_dev)
    {
    #ifndef INT_JULIAN_D
        del_timer(&EMAC_timer);
    #endif
        unregister_netdev(emac_dev);
        free_netdev(emac_dev);
    }
#if defined (SOFTWARE_TX_FLOW_CONTROL)
    del_timer(&EMAC_flow_ctl_timer);
#endif
}

static int mstar_emac_drv_suspend(struct platform_device *dev, pm_message_t state)
{
    struct net_device *netdev=(struct net_device*)dev->dev.platform_data;
    struct EMAC_private *LocPtr;
    u32 uRegVal;
    printk(KERN_INFO "mstar_emac_drv_suspend\n");
    if(!netdev)
    {
        return -1;
    }

    LocPtr = (struct EMAC_private*) netdev_priv(netdev);
    LocPtr->ep_flag |= EP_FLAG_SUSPENDING;
    //netif_stop_queue (netdev);


    disable_irq(netdev->irq);
    del_timer(&Link_timer);

    //MHal_EMAC_Power_On_Clk(dev->dev);

    //corresponds with resume call MDev_EMAC_open
#ifdef MSTAR_EMAC_NAPI
    napi_disable(&LocPtr->napi);
#endif

    //Disable Receiver and Transmitter //
    uRegVal = MHal_EMAC_Read_CTL();
    uRegVal &= ~(EMAC_TE | EMAC_RE);
    MHal_EMAC_Write_CTL(uRegVal);

#ifdef TX_SW_QUEUE
    //make sure that TX HW FIFO is empty
    while(TX_FIFO_SIZE!=MDev_EMAC_GetTXFIFOIdle());
#endif

    // Disable PHY interrupt //
    MHal_EMAC_disable_phyirq ();

    MHal_EMAC_Write_IDR(0xFFFFFFFF);


    MDev_EMAC_SwReset(netdev);
    MHal_EMAC_Power_Off_Clk(&dev->dev);
#if defined(TX_SKB_PTR)
#ifdef TX_SW_QUEUE
    _MDev_EMAC_tx_reset_TX_SW_QUEUE(netdev);
#endif
#endif


    return 0;
}
static int mstar_emac_drv_resume(struct platform_device *dev)
{
    struct net_device *netdev=(struct net_device*)dev->dev.platform_data;
    struct EMAC_private *LocPtr;

#ifndef RX_ZERO_COPY
    phys_addr_t alloRAM_PA_BASE;
    phys_addr_t alloRAM_SIZE;
    MSYS_DMEM_INFO mem_info;
    int ret;
#endif
    printk(KERN_INFO "mstar_emac_drv_resume\n");
    if(!netdev)
    {
        return -1;
    }
    LocPtr = (struct EMAC_private*) netdev_priv(netdev);;
    LocPtr->ep_flag &= ~EP_FLAG_SUSPENDING;

    MHal_EMAC_Power_On_Clk(&dev->dev);

    MHal_EMAC_Write_JULIAN_0100(JULIAN_100_VAL);

    if (0 > MDev_EMAC_ScanPhyAddr())
        return -1;

    MDev_EMAC_Patch_PHY();

#ifndef RX_ZERO_COPY

    strcpy(mem_info.name, "EMAC_BUFF");
    if((ret=msys_request_dmem(&mem_info)))
    {
        panic("unable to locate DMEM for alloRAM_PA_BASE!! error=%d\n",ret);
    }
    alloRAM_PA_BASE = mem_info.phys;
    alloRAM_SIZE = mem_info.length;

    //Add Write Protect
    MHal_EMAC_Write_Protect(alloRAM_PA_BASE - MIU0_BUS_BASE, alloRAM_SIZE);
#endif

    ThisUVE.initedEMAC = 0;
    MDev_EMAC_HW_init();

    MDev_EMAC_update_mac_address (netdev); // Program ethernet address into MAC //
    spin_lock_irq (LocPtr->lock);
    MHal_EMAC_enable_mdi ();

    spin_unlock_irq (LocPtr->lock);

#ifdef CONFIG_MSTAR_HW_TX_CHECKSUM
    retval = MHal_EMAC_Read_JULIAN_0104() | TX_CHECKSUM_ENABLE;
    MHal_EMAC_Write_JULIAN_0104(retval);
#endif

    enable_irq(netdev->irq);
    if(LocPtr->ep_flag & EP_FLAG_OPEND)
    {
        if(0>MDev_EMAC_open(netdev))
        {
            printk(KERN_WARNING "Driver Emac: open failed after resume\n");
        }
    }

    return 0;
}

static int mstar_emac_drv_probe(struct platform_device *pdev)
{
    int retval=0;
    unsigned int led_data;

    if( !(pdev->name) || strcmp(pdev->name,"Mstar-emac")
        || pdev->id!=0)
    {
        retval = -ENXIO;
    }

    retval = MDev_EMAC_init(pdev);
    if(!retval)
    {
        pdev->dev.platform_data=emac_dev;
    }


    if(!of_property_read_u32(pdev->dev.of_node, "led-orange", &led_data))
    {
        g_emac_led_orange = (unsigned char)led_data;
        printk(KERN_ERR "[EMAC]Set emac_led_orange=%d\n",led_data);
    }

    if(!of_property_read_u32(pdev->dev.of_node, "led-green", &led_data))
    {
         g_emac_led_green = (unsigned char)led_data;
         printk(KERN_ERR "[EMAC]Set emac_led_green=%d\n",led_data);
    }

    if(g_emac_led_orange!=-1)
    {
        MDrv_GPIO_Pad_Set(g_emac_led_orange);
    }
    if(g_emac_led_green!=-1)
    {
        MDrv_GPIO_Pad_Set(g_emac_led_green);
    }


    return retval;
}

static int mstar_emac_drv_remove(struct platform_device *pdev)
{
    if( !(pdev->name) || strcmp(pdev->name,"Mstar-emac")
        || pdev->id!=0)
    {
        return -1;
    }
    MDev_EMAC_exit();
    MHal_EMAC_Power_Off_Clk(&pdev->dev);

    pdev->dev.platform_data=NULL;
    return 0;
}



static struct platform_driver Mstar_emac_driver = {
	.probe 		= mstar_emac_drv_probe,
	.remove 	= mstar_emac_drv_remove,
    .suspend    = mstar_emac_drv_suspend,
    .resume     = mstar_emac_drv_resume,

	.driver = {
		.name	= "Mstar-emac",
#if defined(CONFIG_OF)
		.of_match_table = mstaremac_of_device_ids,
#endif
        .owner  = THIS_MODULE,
	}
};



static int __init mstar_emac_drv_init_module(void)
{
    int retval=0;

    emac_dev=NULL;
    retval = platform_driver_register(&Mstar_emac_driver);
    if (retval)
    {
        printk(KERN_INFO"Mstar_emac_driver register failed...\n");
        return retval;
    }

    return retval;
}

static void __exit mstar_emac_drv_exit_module(void)
{
    platform_driver_unregister(&Mstar_emac_driver);
    emac_dev=NULL;
}



module_init(mstar_emac_drv_init_module);
module_exit(mstar_emac_drv_exit_module);

MODULE_AUTHOR("MSTAR");
MODULE_DESCRIPTION("EMAC Ethernet driver");
MODULE_LICENSE("GPL");
