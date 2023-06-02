#ifndef _FREERTOS_EXT_KDRV_H_
#define _FREERTOS_EXT_KDRV_H_
#ifdef __cplusplus
extern "C" {
#endif

#if !defined(__FREERTOS)
#error Only used by FreeRTOS
#endif

/*-----------------------------------------------------------------------------*/
/* Extern functions (Only used by FREERTOS)                                    */
/*-----------------------------------------------------------------------------*/
// timer
extern void delay_us_poll(UINT32 micro_sec);
extern void delay_us(UINT32 micro_sec);

// cache
#if defined(_BSP_NA51055_)
#define VOS_CPU_DRAM_OFFSET         0x60000000UL
#define VOS_CPU_DRAM_END            0xC0000000UL
#define VOS_CACHE_SET_NUMBER_SHIFT  5
#elif defined(_BSP_NA51000_)
#define VOS_CPU_DRAM_OFFSET         0x60000000UL
#define VOS_CPU_DRAM_END            0xC0000000UL
#define VOS_CACHE_SET_NUMBER_SHIFT  6
#elif defined(_BSP_NA51068_)
#define VOS_CPU_DRAM_OFFSET         0x60000000UL
#define VOS_CPU_DRAM_END            0xC0000000UL
#define VOS_CACHE_SET_NUMBER_SHIFT  6
#elif defined(_BSP_NA51089_)
#define VOS_CPU_DRAM_OFFSET         0x60000000UL
#define VOS_CPU_DRAM_END            0xC0000000UL
#define VOS_CACHE_SET_NUMBER_SHIFT  5
#elif defined(_BSP_NA51090_)
#define VOS_CPU_DRAM_OFFSET         0x60000000UL
#define VOS_CPU_DRAM_END            0xC0000000UL
#define VOS_CACHE_SET_NUMBER_SHIFT  6
#else
#error VOS_CPU_DRAM param not defined
#endif

extern BOOL dma_flushReadCacheWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);
extern void dma_flushReadCacheDmaEndWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);
extern BOOL dma_flushReadCacheWidthNEQLineOffsetWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);
extern BOOL dma_flushWriteCacheWithoutCheck(UINT32 uiStartAddr, UINT32 uiLength);

// uart (rtos-code\driver\na51000\include\uart.h)
extern ER uart_putString(CHAR *pString);

// timer 0 (hdal\drivers\k_driver\include\comm\hwclock.h)
extern UINT32 hwclock_get_counter(void);
extern UINT64 hwclock_get_longcounter(void);

#ifdef __cplusplus
}
#endif

#endif /* _FREERTOS_EXT_KDRV_H_ */

