/*
 * mdrv_gpic.h- Sigmastar
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

#ifndef _MDRV_GPIC_H_
#define _MDRV_GPIC_H_

typedef enum
{
    GPIC_RISING_EDGE,
    GPIC_FAILING_EDGE,
} GpicEdge_e;

typedef enum
{
    GPIC_UP_COUNT,
    GPIC_DOWN_COUNT,
} GpicDir_e;

typedef struct
{
    u8  pad;
    u8  dir;
    u8  edge;
    u16 init;
    u16 target;
    void (*callback)(void*);
    void* para;
} GpicCfg_t;

extern void* camdriver_gpic_get_handler(u8 group);
extern int   camdriver_gpic_config(void* handler, GpicCfg_t* cfg);
extern int   camdriver_gpic_enable(void* handler);
extern int   camdriver_gpic_disable(void* handler);
extern int   camdriver_gpic_get_count(void* handler, u16* count);

#endif
