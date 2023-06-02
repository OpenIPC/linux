#ifndef _MPATCH_MACRO_H
#define _MPATCH_MACRO_H

#define MSTAR_MIU_BUS_BASE_NONE		0xFFFFFFFF

#if defined(CONFIG_ARCH_CEDRIC)
	#define CONFIG_MSTAR_CEDRIC	1

	//#include "../../arch/arm/mach-cedric/include/mach/platform.h"
	#define MSTAR_MIU0_BUS_BASE	0x40000000
	/* Cedric only support MIU0 */
	#define MSTAR_MIU1_BUS_BASE	MSTAR_MIU_BUS_BASE_NONE
	
#elif defined(CONFIG_ARCH_CHICAGO)
	#define CONFIG_MSTAR_CHICAGO	1
	#include "../../arch/arm/mach-chicago/include/mach/platform.h"
	#define MSTAR_MIU0_BUS_BASE	MIU0_BUS_ADDR
	/* Cedric only support MIU0 */
	#define MSTAR_MIU1_BUS_BASE	MSTAR_MIU_BUS_BASE_NONE
#elif defined(CONFIG_ARCH_INFINITY)
	#define CONFIG_MSTAR_INFINITY	1

	//#include "../../arch/arm/mach-cedric/include/mach/platform.h"
	#define MSTAR_MIU0_BUS_BASE	0x20000000
	/* Cedric only support MIU0 */
	#define MSTAR_MIU1_BUS_BASE	MSTAR_MIU_BUS_BASE_NONE
#elif defined(CONFIG_ARCH_INFINITY3)
	#define CONFIG_MSTAR_INFINITY3	1
    #define BUS_PA_PATCH 1
        //#include "../../arch/arm/mach-cedric/include/mach/platform.h"
	#define MSTAR_MIU0_BUS_BASE	0x20000000
        /* Cedric only support MIU0 */
	#define MSTAR_MIU1_BUS_BASE	MSTAR_MIU_BUS_BASE_NONE

#else 
	#error Unsupported Mstar Platform !!!
#endif

//=========================== Module:USB=======================================
#ifdef CONFIG_MP_USB_MSTAR
	#define MP_USB_MSTAR	1
#else
	#define MP_USB_MSTAR	0
#endif

#ifdef CONFIG_MP_USB_MSTAR_DEBUG
	#define MP_USB_MSTAR_DEBUG	1
#else
	#define MP_USB_MSTAR_DEBUG	0
#endif

//=========================== Module:USB=======================================
#ifdef CONFIG_MP_USB_STR_PATCH
	#define MP_USB_STR_PATCH	1
#else
	#define MP_USB_STR_PATCH	0
#endif

#ifdef CONFIG_MP_USB_STR_PATCH_DEBUG
	#define MP_USB_STR_PATCH_DEBUG	1
#else
	#define MP_USB_STR_PATCH_DEBUG	0
#endif

#endif //_MPATCH_MACRO_H
