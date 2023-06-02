#ifndef __PWM_PLATFORM_H_
#define __PWM_PLATFORM_H__


#if (defined __UITRON || defined __ECOS)
#include "interrupt.h"
#include "pll.h"
#include "top.h"
#include "DrvCommon.h"
#include "gpio.h"

#elif defined(__FREERTOS)
#include "kwrap/error_no.h"
#include "kwrap/flag.h"
#include "kwrap/spinlock.h"
#include "kwrap/task.h"
#include "kwrap/mem.h"
#include "rcw_macro.h"
#include "interrupt.h"
#include <interrupt.h>
#include "io_address.h"
#include "comm/timer.h"
#include "trng.h"
#include "top.h"
#include "stdlib.h"
#include "ethernet_protected.h"
#include "efuse_protected_na51089.h"
#include <kwrap/cpu.h>
//#include "netif/nvt_eth.h"




#define __MODULE__    rtos_eth
#define __DBGLVL__ NVT_DBG_USER
#include <kwrap/debug.h>
extern unsigned int rtos_eth_debug_level;

#else

#endif

#if defined __FREERTOS || defined __KERNEL__
#define _EMULATION_             (0)	// no emu macro in linux
#define _EMULATION_ON_CPU2_     (0)
#define _FPGA_EMULATION_         0

#define ETH_DRV_FLG_ID              1
#define ETH_FLGPTN_ISR              FLGPTN_BIT(0)
#define ETH_FLGPTN_TASK_CREATED     FLGPTN_BIT(1)
#define ETH_FLGPTN_TASK_STOP_REQ    FLGPTN_BIT(2)
#define ETH_FLGPTN_TASK_STOPPED     FLGPTN_BIT(3)

#define ETH_TX_FLGPTN_ISR              FLGPTN_BIT(4)
#define ETH_RX_FLGPTN_ISR              FLGPTN_BIT(5)

#define ETH_FLGPTN_MDIO     		FLGPTN_BIT(6)

#define ETH_FLGPTN_TASK_TX_DONE     FLGPTN_BIT(16)
#define ETH_FLGPTN_TASK_RX_DONE     FLGPTN_BIT(17)

#define ETH_FLGPTN_TASK_FREEZE      FLGPTN_BIT(31)

#define ETH_SETREG(ofs,value)  OUTW(IOADDR_ETH_REG_BASE+(ofs),(value))
#define ETH_GETREG(ofs)        INW(IOADDR_ETH_REG_BASE+(ofs))

#endif

extern THREAD_HANDLE uiEthernetTaskID;

extern ER eth_platform_flg_clear(ID id, FLGPTN flg);
extern ER eth_platform_flg_set(ID id, FLGPTN flg);
extern FLGPTN eth_platform_flg_wait(ID id, FLGPTN flg);
extern void eth_platform_spin_lock(void);
extern void eth_platform_spin_unlock(void);
extern void eth_platform_int_enable(ID id);
extern void eth_platform_int_disable(ID id);
extern void eth_platform_txint_enable(ID id);
extern void eth_platform_txint_disable(ID id);
extern void eth_platform_rxint_enable(ID id);
extern void eth_platform_rxint_disable(ID id);
extern void eth_platform_delay_ms(UINT32 ms);
extern void eth_platform_delay_us(UINT32 us);
extern void eth_platform_sta_tsk(ID id);
extern void eth_platform_ter_tsk(ID id);
extern void eth_platform_sta_phy_chk_tsk(ID id);
extern void eth_platform_ter_phy_chk_tsk(ID id);
//extern void abort(void);

#if !(defined __UITRON || defined __ECOS)
//extern int DBG_ERR(const char *fmt, ...);
//extern int DBG_WRN(const char *fmt, ...);
//extern int DBG_IND(const char *fmt, ...);
#if defined __FREERTOS
extern void eth_platform_create_resource(void);
#else
#endif
extern void eth_task(void *parameters);
extern void eth_tx_task(void *parameters);
extern void eth_rx_task(void *parameters);
extern void eth_isr(void);
extern void eth_isr_tx(void);
extern void eth_isr_rx(void);
extern void eth_platform_release_resource(void);
extern void nvtimeth_phy_thread(void);
#endif


#endif
