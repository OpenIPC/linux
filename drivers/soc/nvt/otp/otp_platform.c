#if defined __UITRON || defined __ECOS

#include "ddr_arb.h"
#include "interrupt.h"
#include "top.h"
#include "dma.h"
static const DRV_SEM v_sem[] = {SEMID_ARB};

#elif defined(__FREERTOS)
#include <kwrap/error_no.h>
#include <kwrap/semaphore.h>
#include <kwrap/nvt_type.h>
#include <kwrap/flag.h>
#include <kwrap/spinlock.h>

#include <kwrap/task.h>
#include <string.h>
#include "rcw_macro.h"
#include "io_address.h"
#include "dma_protected.h"
#include "cache_protected.h"
#include "interrupt.h"


static SEM_HANDLE *v_sem[1];
static SEM_HANDLE SEMID_OTP;
#else
#include <plat/hardware.h>
#include "otp_platform.h"
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/delay.h>

#include <mach/fmem.h>

UINT32 IOADDR_EFUSE_REG_BASE = 0x0;
DEFINE_SEMAPHORE(otp_sem);
//static struct semaphore otp_sem;
#endif


ER otp_platform_sem_wait(void)
{
#if defined __UITRON || defined __ECOS
	return wai_sem(v_sem[0]);
#elif defined(__FREERTOS)
	return SEM_WAIT(*v_sem[0]);
#else
    down(&otp_sem);
	return E_OK;
#endif
}

ER otp_platform_sem_signal(void)
{
#if defined __UITRON || defined __ECOS
	return sig_sem(v_sem[0]);
#elif defined(__FREERTOS)
	SEM_SIGNAL(*v_sem[0]);
	return E_OK;
#else
    up(&otp_sem);
	return E_OK;
#endif
}

void otp_platform_delay_ms(UINT32 ms)
{
#if defined __UITRON || defined __ECOS
	Delay_DelayMs(ms);
#elif defined __FREERTOS
	vos_task_delay_ms(ms);
#else
	msleep(ms);
#endif
}
#if defined __FREERTOS
void otp_platform_create_resource(void)
#else
void otp_platform_create_resource(MODULE_INFO *pmodule_info)
#endif
{
#if defined __FREERTOS
   	SEM_CREATE(SEMID_OTP, 1);
	v_sem[0] = &SEMID_OTP;
#else
	IOADDR_EFUSE_REG_BASE = (UINT32)pmodule_info->io_addr[0];
	//sema_init(&otp_sem, 1);
	//otp_sem_init = 1;
#endif
}

void otp_platform_release_resource(void)
{
#if defined __FREERTOS
	SEM_DESTROY(SEMID_OTP);
#endif
}

#if defined __KERNEL__
void otp_platform_earily_create_resource(void)
{
	if(!IOADDR_EFUSE_REG_BASE) {
		IOADDR_EFUSE_REG_BASE = (UINT32)ioremap_nocache(NVT_EFUSE_BASE_PHYS, 0x100);
	}
}

void otp_platform_earily_release_resource(void)
{
	iounmap((volatile void __iomem *)IOADDR_EFUSE_REG_BASE);
}

EXPORT_SYMBOL(otp_platform_earily_create_resource);
EXPORT_SYMBOL(otp_platform_earily_release_resource);
#endif
