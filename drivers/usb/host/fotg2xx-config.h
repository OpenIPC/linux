
/*******************************************************************************
 * Module name: config.h
 *
 * The information contained herein is confidential property of Company.
 * The user, copying, transfer or disclosure of such information is
 * prohibited except by express written agreement with Company.
 *
 * Module Description:
 *  Port from kernel_tree/include/Linux/config.h
 *  macro definition for configuration of platform and
 *  compiler (DEBUG macros)
 *
 ******************************************************************************/

#ifndef FOTG2XX_CONFIG_H_CHK
#define FOTG2XX_CONFIG_H_CHK

#ifdef CONFIG_GM_FOTG2XX_MODULE
#define CONFIG_GM_FOTG2XX
#endif

/* Define the number of OTG devices in SoC*/
#if defined(CONFIG_PLATFORM_GM8126) || defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
#define FOTG_DEV_NR 1
#endif

#if defined(CONFIG_PLATFORM_GM8210)
#define FOTG_DEV_NR 3
#endif

#if defined(CONFIG_PLATFORM_GM8287)
#define FOTG_DEV_NR 2
#endif

/* Define this const macro will disable any cod0e which is used to recover HW issue */
//#define REMOVE_COVERHW	1

/* =======================================================*/
/* OTG application related macro definitions              */
/* =======================================================*/
#define CONFIG_OTG

/* =======================================================*/
//Please only define following macros if you need to cover PHY issues
//#define CONFIG_711MA_PHY		/* For 711MA to enable hc to cover full speed reset issue */
/* =======================================================*/


////////////////////////////////////////
#ifdef CONFIG_GM_FUSBH200
#undef  CONFIG_OTG
#define FUSBH200_REGBASE   USB_FUSBH200_VA_BASE
#define FUSBH200_IRQ       USB_FUSBH200_IRQ
#endif

/* ======================================================*/
/* DEBUG fucniton macro definitions                      */
/* ======================================================*/
//#define SHOW_MSG
//#define DEBUG
//#define CONFIG_DEBUG
//#define CONFIG_USB_DEBUG
//#define VERBOSE_DEBUG
//#define KERNEL_DEBUG
#ifndef CONFIG_GM_FOTG2XX
#ifdef VERBOSE_DEBUG
#define OHCI_VERBOSE_DEBUG  //Enable Verbose debug, dump more details
#endif
#else
#ifdef VERBOSE_DEBUG
#define EHCI_VERBOSE_DEBUG  //Enable Verbose debug, dump more details
#endif
#endif

#ifdef SHOW_MSG
#define msg	printk
#else
#define msg(format,...) do {} while (0)
#endif

#if defined(CONFIG_GM_FOTG2XX_INFO) || defined(CONFIG_GM_FUSBH200_INFO)
#define fotg2xx_dbg(format, ...) printk(KERN_INFO " "format, __VA_ARGS__ )
#else
#define fotg2xx_dbg(format, ...) do {} while (0)
#endif

#ifdef KERNEL_DEBUG
#define kdbg dbg
#else
#define kdbg(format, ...) do {} while (0)
#endif

#endif  /* FOTG2XX-CONFIG_H_CHK */
