/*
 * mhal_keypad.h- Sigmastar
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

#ifndef __MHAL_KEYPAD_H__
#define __MHAL_KEYPAD_H__

#include <linux/types.h>
#include <linux/kernel.h>
#include "cam_sysfs.h"
#include "cam_os_wrapper.h"
#include "mdrv_types.h"

struct device;

#define KEYPAD_SUCCESS (0)
#define KEYPAD_FAILED  (1)

#define KEYPAD_MAXCOLNUM (8)
#define KEYPAD_MAXROWNUM (8)

#define KEPYAD_MAXKEYNUM           (KEYPAD_MAXROWNUM * KEYPAD_MAXCOLNUM)
#define KEPYAD_MAXKEYNUM_TWOSTATUS (2 * KEYPAD_MAXROWNUM * KEYPAD_MAXCOLNUM)

#define REGBYTENUM (16)

typedef enum
{
    /* Key_raw_status. meens the value is the current keypad matrix status */
    E_KP_RAW_KEYPR = 0,
    /* Key_raw_status. meens the value is the same as reg_key_finial_status */
    E_KP_RAW_KEYSTATUS = 1,

} KEYPAD_RAW_MODE;

typedef enum
{
    /*Occur Irq in key Pressed or release. Keep all press and all release key status.
      release result will be store R17-1e->{64:128} */
    E_KP_STATUS_MODE_0 = 0,
    /*Occur Irq in key Pressed.Keep current press and last time press status.
      last time pressd result store R17-1e->{64:128}*/
    E_KP_STATUS_MODE_1 = 1,
    /*Occur Irq in key release. Keep current release and last time release status.
      last time relsase result store R17-1e->{64:128}*/
    E_KP_STATUS_MODE_2 = 2,
    /*Occur Irq in key Pressed. Support press single key.*/
    E_KP_STATUS_MODE_3 = 3,

} KEYPAD_STATUS_MODE;

typedef enum
{
    E_KP_STATUS_PRESSED = 0,
    E_KP_STATUS_RELEASE = 1,
} KEYPAD_KEY_STATUS;

struct ms_keypad_rulenum
{
    U32 row;
    U32 col;
};

struct ms_keypad_date
{
    U32 wakeup; /* reserve */
    /* for mode0. pressed or release */
    U32 type;
    U32 row;
    U32 col;
    U32 modesel;
    U32 rawstatus;
    U32 irq;
    /* keypad glitch time,set to HW */
    U32 debounce;

    /* keypad softreset */
    U32 softrst;

    /* reg_key_final_status register value */
    U32 s_value[8];

    /* reg_key_raw_status register value   */
    U32 r_value[8];

    /* for mode 0&1&2 */
    U32 align;

    /* mask the irq */
    U32 mask;

    /* force the irq */
    U32 force;

    struct ms_keypad_rulenum rulenum[0];
};

struct ms_keypad_drvdata
{
    const struct ms_keypad_platform_data *pdata;
    struct input_dev *                    input;
    // struct delayed_work work;
    struct work_struct     work;
    struct timer_list      keypad_timer;
    struct mutex           disable_lock;
    spinlock_t             lock;
    struct ms_keypad_date *data;
};

struct ms_keypad_dts_s
{
    struct clk *  clk;
    void __iomem *base;
    u32 *         keypad_rowcol;
    void *        group_data;
    U32           keypadmode;
    U32           debounce;
    U32           irq;
};

struct ms_keypad_platform_data
{
    struct ms_keypad_dts_s *ms_keypadts;
    U32                     nbuttons;
    U32                     poll_interval;
    U32                     rep : 1;
    U32 (*enable)(struct device *dev);
    void (*disable)(struct device *dev);
    const char *name;
};

/******************************* Register *********************************/
#define REG_KEYPAD_BANK_BASE      0x100E00UL
#define REG_KEYPAD_GPIO_BANK_BASE 0x103C00UL

#define KEYPAD_RIU_BASE 0xfd000000UL

#define REG16(bank, addr) (*((volatile U16 *)(((KEYPAD_RIU_BASE) + (bank << 1U)) + ((addr) << 2U))))
#define REG32(bank, addr) (*((volatile U32 *)(((KEYPAD_RIU_BASE) + (bank << 1U)) + ((addr) << 2U))))

#define KEYPAD_READ(addr)       (REG16(REG_KEYPAD_BANK_BASE, addr))
#define KEYPAD_WRITE(addr, val) (REG16(REG_KEYPAD_BANK_BASE, addr) = (val))

#define KEYPAD_READ_32(addr)       (REG32(REG_KEYPAD_BANK_BASE, addr))
#define KEYPAD_WRITE_32(addr, val) (REG32(REG_KEYPAD_BANK_BASE, addr) = (val))

#define KEYPAD_GPIO_WRITE(addr, val) (REG32(REG_KEYPAD_GPIO_BANK_BASE, addr) = (val))

#define KEYPAD_GPIO_PAMUX (0x72)

#define KEYPAD_GPIO_ENABLE0 (0x78)
#define KEYPAD_GPIO_ENABLE1 (0x79)
#define KEYPAD_GPIO_ENABLE2 (0x7A)
#define KEYPAD_GPIO_ENABLE3 (0x7B)

#define KEYPAD_KEY_NUM (0x00)
#define KEYPAD_COL     (BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define KEYPAD_ROW     (BIT(4) | BIT(5) | BIT(6) | BIT(7))

#define KEYPAD_GLHRM_NUM (0x01)

#define KEYPAD_GLHRM_ENABLE0 (0x02)
#define KEYPAD_GLHRM_ENABLE1 (0x03)
#define KEYPAD_GLHRM_ENABLE2 (0x04)
#define KEYPAD_GLHRM_ENABLE3 (0x05)

#define KEYPAD_FORCE_ENABLE0 (0x06)
#define KEYPAD_FORCE_ENABLE1 (0x07)
#define KEYPAD_FORCE_ENABLE2 (0x08)
#define KEYPAD_FORCE_ENABLE3 (0x09)
#define KEYPAD_FORCE_ENABLE4 (0x0A)
#define KEYPAD_FORCE_ENABLE5 (0x0B)
#define KEYPAD_FORCE_ENABLE6 (0x0C)
#define KEYPAD_FORCE_ENABLE7 (0x0D)

#define KEYPAD_MASK0 (0x0E)
#define KEYPAD_MASK1 (0x0F)
#define KEYPAD_MASK2 (0x10)
#define KEYPAD_MASK3 (0x11)
#define KEYPAD_MASK4 (0x12)
#define KEYPAD_MASK5 (0x13)
#define KEYPAD_MASK6 (0x14)
#define KEYPAD_MASK7 (0x15)

#define KEYPAD_CLEANIRQ (0x16)

#define KEYPAD_FINAL_STATUS0 (0x17)
#define KEYPAD_FINAL_STATUS1 (0x18)
#define KEYPAD_FINAL_STATUS2 (0x19)
#define KEYPAD_FINAL_STATUS3 (0x1A)
#define KEYPAD_FINAL_STATUS4 (0x1B)
#define KEYPAD_FINAL_STATUS5 (0x1C)
#define KEYPAD_FINAL_STATUS6 (0x1D)
#define KEYPAD_FINAL_STATUS7 (0x1E)

#define KEYPAD_RAW_STATUS0 (0x1F)
#define KEYPAD_RAW_STATUS1 (0x20)
#define KEYPAD_RAW_STATUS2 (0x21)
#define KEYPAD_RAW_STATUS3 (0x22)
#define KEYPAD_RAW_STATUS4 (0x23)
#define KEYPAD_RAW_STATUS5 (0x24)
#define KEYPAD_RAW_STATUS6 (0x25)
#define KEYPAD_RAW_STATUS7 (0x26)

#define KEYPAD_RAW_SEL (0x27)
#define KEYPAD_SCAN_EN (0x28)
#define KEYPAD_SW_RST  (0x29)

#define KEYPAD_SCAN_DFS_CFG     (0x2A)
#define KEYPAD_GLHRM_DFS_CFG    (0x2B)
#define KEYPAD_MODE_SEL         (0x2C)
#define KEYPAD_DUMMY            (0x2D)
#define KEYPAD_MODE_STATUS_FLAG (0x2E)
#define KEYPAD_ALIGN_NUM        (0x2F)

U32 mhal_KeyPad_SetColNum(U32 num);
U32 mhal_KeyPad_SetRowNum(U32 num);
U32 mhal_KeyPad_SetGlhrNum(U32 num);
U32 mhal_KeyPad_SetGlhrEN(U32 num, U32 enable);
U32 mhal_KeyPad_SetForce(U32 num, U32 enable);
U32 mhal_KeyPad_SetMask(U32 num, U32 enable);
U32 mhal_KeyPad_ClearIrq(void);
U32 mhal_KeyPad_SetRawSel(KEYPAD_RAW_MODE num);
U32 mhal_KeyPad_EnableScan(bool Enable);
U32 mhal_KeyPad_RST(bool Enable);
U32 mhal_KeyPad_SetScanDfs(U32 ScanClk);
U32 mhal_KeyPad_SetGlhrmDfs(U32 GlhClk);
U32 mhal_KeyPad_ModeSel(KEYPAD_STATUS_MODE mode);
U32 mhal_KeyPad_Dummy(bool dummy);
U32 mhal_KeyPad_GetTwoStatusFlag(void);
U32 mhal_keypad_pinmux(void);
U32 mhal_KeyPad_KEYPAD_ALIGN_NUM(U32 num);
U32 mhal_KeyPad_GetFinal_Stage(U32 stage, U32 *value);
U32 mhal_KeyPad_GetRaw_Stage(U32 stage, U32 *value);

#endif
