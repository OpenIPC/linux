#ifndef __NVT_FB_DBG_H__
#define __NVT_FB_DBG_H__

#define NVT_FB_DBG_IND                    0x00000001
#define NVT_FB_DBG_WRN                    0x00000002
#define NVT_FB_DBG_ERR                    0x00000004

#ifdef FB_NA51000_DEBUG
extern unsigned int nvt_fb_debug_level;

#define nvt_dbg(level, fmt, args...)                \
do {                                               \
	if (unlikely(NVT_FB_DBG_##level & nvt_fb_debug_level))    \
	printk("%s:" fmt, __func__, ##args);   \
} while (0)
#else
#define nvt_dbg(level, fmt, args...)
#endif

#define DBG_ERR(fmt, args...) nvt_dbg(ERR, fmt, ##args)
#define DBG_WRN(fmt, args...) nvt_dbg(WRN, fmt, ##args)
#define DBG_IND(fmt, args...) nvt_dbg(IND, fmt, ##args)

#endif


