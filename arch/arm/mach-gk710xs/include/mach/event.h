/*!
*****************************************************************************
** \file        arch/arm/mach-gk710xs/include/plat/event.h
**
** \version
**
** \brief
**
** \attention   THIS SAMPLE CODE IS PROVIDED AS IS. GOKE MICROELECTRONICS
**              ACCEPTS NO RESPONSIBILITY OR LIABILITY FOR ANY ERRORS OR
**              OMMISSIONS
**
** (C) Copyright 2012-2015 by GOKE MICROELECTRONICS CO.,LTD
**
*****************************************************************************
*/
#ifndef __PLAT_EVENT_H
#define __PLAT_EVENT_H

/* ==========================================================================*/
#define GK_EVENT_PRE                (0x80000000)
#define GK_EVENT_POST               (0x40000000)
#define GK_EVENT_CHECK              (0x20000000)

#define GK_EVENT_ID_CPUFREQ         (1)
#define GK_EVENT_ID_PM              (2)
#define GK_EVENT_ID_TOSS            (3)
#define GK_EVENT_ID_GIVEUP_DSP      (4)
#define GK_EVENT_ID_TAKEOVER_DSP    (5)
#define GK_EVENT_ID_USBVBUS         (6)
#define GK_EVENT_ID_VI_LOSS         (7)

#define GK_EVENT_PRE_CPUFREQ        (GK_EVENT_ID_CPUFREQ | GK_EVENT_PRE)
#define GK_EVENT_POST_CPUFREQ       (GK_EVENT_ID_CPUFREQ | GK_EVENT_POST)
#define GK_EVENT_CHECK_CPUFREQ      (GK_EVENT_ID_CPUFREQ | GK_EVENT_CHECK)
#define GK_EVENT_PRE_PM             (GK_EVENT_ID_PM | GK_EVENT_PRE)
#define GK_EVENT_POST_PM            (GK_EVENT_ID_PM | GK_EVENT_POST)
#define GK_EVENT_CHECK_PM           (GK_EVENT_ID_PM | GK_EVENT_CHECK)
#define GK_EVENT_PRE_TOSS           (GK_EVENT_ID_TOSS | GK_EVENT_PRE)
#define GK_EVENT_POST_TOSS          (GK_EVENT_ID_TOSS | GK_EVENT_POST)
#define GK_EVENT_CHECK_TOSS         (GK_EVENT_ID_TOSS | GK_EVENT_CHECK)

#define GK_EVENT_PRE_GIVEUP_DSP     (GK_EVENT_ID_GIVEUP_DSP | GK_EVENT_PRE)
#define GK_EVENT_POST_GIVEUP_DSP    (GK_EVENT_ID_GIVEUP_DSP | GK_EVENT_POST)
#define GK_EVENT_GIVEUP_DSP         (GK_EVENT_ID_GIVEUP_DSP | GK_EVENT_CHECK)
#define GK_EVENT_PRE_TAKEOVER_DSP   (GK_EVENT_ID_TAKEOVER_DSP | GK_EVENT_PRE)
#define GK_EVENT_POST_TAKEOVER_DSP  (GK_EVENT_ID_TAKEOVER_DSP | GK_EVENT_POST)
#define GK_EVENT_TAKEOVER_DSP       (GK_EVENT_ID_TAKEOVER_DSP | GK_EVENT_CHECK)

#define GK_EVENT_CHECK_USBVBUS      (GK_EVENT_ID_USBVBUS | GK_EVENT_CHECK)

#define GK_EVENT_POST_VI_LOSS       (GK_EVENT_ID_VI_LOSS | GK_EVENT_POST)

/* ==========================================================================*/
#ifndef __ASSEMBLER__
#include <linux/kobject.h>

/* ==========================================================================*/
enum gk_event_type {
    /* No Event */
    GK_EV_NONE                          = 0x00000000,

    /* VI Event */
    GK_EV_VI_DECODER_SOURCE_PLUG        = 0x00010000,
    GK_EV_VI_DECODER_SOURCE_REMOVE,

    /* VO Event */
    GK_EV_VO_CVBS_PLUG                  = 0x00020000,
    GK_EV_VO_CVBS_REMOVE,
    GK_EV_VO_YPBPR_PLUG,
    GK_EV_VO_YPBPR_REMOVE,
    GK_EV_VO_HDMI_PLUG,
    GK_EV_VO_HDMI_REMOVE,

    /* SENSOR Event*/
    GK_EV_ACCELEROMETER_REPORT          = 0x00030000,
    GK_EV_MAGNETIC_FIELD_REPORT,
    GK_EV_LIGHT_REPORT,
    GK_EV_PROXIMITY_REPORT,
    GK_EV_GYROSCOPE_REPORT,
    GK_EV_TEMPERATURE_REPORT,

    /* FB2 Event */
    GK_EV_FB2_PAN_DISPLAY               = 0x00040000,
};

struct gk_event {
    u32                 sno;        //sequential number
    u64                 time_code;
    enum gk_event_type  type;
    u8                  data[32];
};

struct gk_event_pool {
    struct mutex        op_mutex;
    struct gk_event     events[256];
    unsigned int        ev_sno;
    unsigned char       ev_index;
};

extern int gk_event_pool_init(struct gk_event_pool *pool);
extern int gk_event_pool_affuse(struct gk_event_pool *pool,
    struct gk_event event);
extern int gk_event_pool_query_index(struct gk_event_pool *pool);
extern int gk_event_pool_query_event(struct gk_event_pool *pool,
    struct gk_event *event, unsigned char index);
extern int gk_event_report_uevent(struct kobject *kobj,
    enum kobject_action action, char *envp_ext[]);

/* ==========================================================================*/
extern int gk_unregister_event_notifier(void *nb);
extern int gk_set_event(unsigned long val, void *v);
extern int gk_register_raw_event_notifier(void *nb);
extern int gk_unregister_raw_event_notifier(void *nb);
extern int gk_set_raw_event(unsigned long val, void *v);

#endif /* __ASSEMBLER__ */
/* ==========================================================================*/

#endif

