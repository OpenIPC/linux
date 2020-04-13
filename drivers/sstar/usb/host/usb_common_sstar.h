

#ifndef _USB_SSTAR_H
#define _USB_SSTAR_H

//#if (MP_USB_MSTAR==1)
#if 1

//Host
#ifndef MP_USB_MSTAR
#include <usb_patch_mstar.h>
#endif

#include "ehci-mstar.h"

//Core
#include "xhci-mstar.h"
#include <asm/io.h>
#if (_UTMI_PWR_SAV_MODE_ENABLE == 1) || defined(USB_MAC_SRAM_POWER_DOWN_ENABLE)
#include "bc-mstar.h"
#endif


#endif

#endif	/* _USB_SSTAR_H */
