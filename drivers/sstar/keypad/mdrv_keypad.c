/*
 * mdrv_keypad.c- Sigmastar
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

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/of_irq.h>
#include <linux/irq.h>
#include <linux/vmalloc.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/proc_fs.h>
#include <linux/input.h>
#include <linux/of_platform.h>
#include "mdrv_keypad.h"
#include "mhal_keypad.h"

#define CONFIG_WORKQUEL
#define CONFIG_KEYPADTS

// static U32 debug_msg = FALSE;
static U32 debug_msg = TRUE;

#define MDRV_KEYPAD_DEBUG(format, args...) \
    do                                     \
    {                                      \
        if (debug_msg)                     \
            printk(format, ##args);        \
    } while (0)

#define DEFAULT_DEBOUNCE_TIME 5        // ms unit
#define KEYPADINPUTCLK        12000000 // 12M
#define KEYPADADJUSTCLK       32000    // 32khz

/* ms unit. max.170-min42(0x1f0-0x7c) */
#define KEYPAD_ALIGNTIMELENGTH 100

/**
 * mdrv_keypadcalc_debounce - returns debounce num access to clk
 * @clk: clk of keypad
 *
 * debounce time = (setting number) * 4096 / (clk)
 * Return value of this function set to register glitch time num
 */
#define mdrv_keypadcalc_DEBOUNCE(clk) (clk * DEFAULT_DEBOUNCE_TIME / 4096 / 1000)

/**
 * mdrv_keypadcalc_scanrate - returns scanrate num access to scanclk
 * @scanclk: clk of keypad adjust CLK
 *
 * scan_rate time = (input clk rate) / (setting number +1).
 * Return value of this function set to register scan_rate time num.
 */
#define mdrv_keypadcalc_SCANRATE(scanclk) (KEYPADINPUTCLK / scanclk - 1)

/**
 * mdrv_keypadcalc_ALIGN - returns align num access to ALIGN_NUM
 * @ALIGN_NUM: time of align(ms unit).
 *
 * Align time length is (setting number) / (deglitch function clock);
 * When testing FPGA ,the align time length is (setting number) * 4096 / (FPGA clk);
 * Return value of this function set to register scan_rate time num.
 */
#define mdrv_keypadcalc_ALIGN(ALIGN_NUM) (ALIGN_NUM * KEYPADINPUTCLK / 4096 / 1000)

S32 mdrv_keypad_init(struct ms_keypad_drvdata *ddata);

#ifdef CONFIG_KEYPADTS
static U32 keypad_desc[KEYPAD_MAXROWNUM][KEYPAD_MAXCOLNUM] = {};

#else

static U32 keypad_desc[KEYPAD_MAXROWNUM][KEYPAD_MAXCOLNUM] = {
    {KEY_ESC, KEY_8, KEY_W, KEY_P, KEY_F, KEY_GRAVE, KEY_N, KEY_SPACE},
    {KEY_1, KEY_9, KEY_E, KEY_LEFTBRACE, KEY_G, KEY_LEFTSHIFT, KEY_M, KEY_CAPSLOCK},
    {KEY_2, KEY_0, KEY_R, KEY_RIGHTBRACE, KEY_H, KEY_BACKSLASH, KEY_COMMA, KEY_F1},
    {KEY_3, KEY_MINUS, KEY_T, KEY_ENTER, KEY_J, KEY_Z, KEY_DOT, KEY_F2},
    {KEY_4, KEY_EQUAL, KEY_Y, KEY_LEFTCTRL, KEY_K, KEY_X, KEY_SLASH, KEY_F3},
    {KEY_5, KEY_BACKSPACE, KEY_U, KEY_A, KEY_L, KEY_C, KEY_RIGHTSHIFT, KEY_F4},
    {KEY_6, KEY_TAB, KEY_I, KEY_S, KEY_SEMICOLON, KEY_V, KEY_KPASTERISK, KEY_F5},
    {KEY_7, KEY_Q, KEY_O, KEY_D, KEY_APOSTROPHE, KEY_B, KEY_LEFTALT, KEY_F6},
};

#endif

static ssize_t keypad_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct platform_device *  pdev  = to_platform_device(dev);
    struct ms_keypad_drvdata *ddata = platform_get_drvdata(pdev);
    // ssize_t error;
    char *str = buf;
    char *end = buf + PAGE_SIZE * 2;

    str += CamOsSnprintf(str, end - str, "======================== Keypad Msg ========================\n");
    str += CamOsSnprintf(str, end - str, "Irq_num : %hd\n", ddata->data->irq);
    str += CamOsSnprintf(str, end - str, "mode : %hd\n", ddata->data->modesel);
    str += CamOsSnprintf(str, end - str, "debounce : %hd\n", ddata->data->debounce);
    str += CamOsSnprintf(str, end - str, "mode : %hd\n", ddata->data->softrst);

    if (ddata->data->modesel == E_KP_STATUS_MODE_1 || ddata->data->modesel == E_KP_STATUS_MODE_2)
    {
        str += CamOsSnprintf(str, end - str, "===================== Keypad Extra Msg =====================\n");
        str += CamOsSnprintf(str, end - str, "align : %hd\n", ddata->data->align);
    }

    str += CamOsSnprintf(str, end - str, "======================== Keypad Msg ========================\n");
    return (str - buf);
}
static ssize_t keypad_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    struct platform_device *  pdev     = to_platform_device(dev);
    struct ms_keypad_drvdata *ddata    = platform_get_drvdata(pdev);
    U8                        initflag = 0;
    if (NULL != buf)
    {
        char *pBuf  = (char *)buf;
        char *p_val = NULL;
        char *pName = NULL;

        while (1)
        {
            pName = strsep(&pBuf, "=");
            if (pName != NULL && strncmp(pName, "mode", 4) == 0)
            {
                p_val = strsep(&pBuf, " ");
                if (NULL == p_val)
                    goto printf_usage;
                ddata->data->modesel = CamOsStrtol(p_val, NULL, 10);
                if (NULL == pBuf)
                {
                    initflag = 1;
                    break;
                }
            }
            else if (pName != NULL && strncmp(pName, "debounce", 8) == 0)
            {
                p_val = strsep(&pBuf, " ");
                if (NULL == p_val)
                    goto printf_usage;
                ddata->data->debounce = CamOsStrtol(p_val, NULL, 10);
                if (NULL == pBuf)
                {
                    initflag = 1;
                    break;
                }
            }
            else if (pName != NULL && strncmp(pName, "dbg", 3) == 0)
            {
                p_val = strsep(&pBuf, " ");
                if (NULL == p_val)
                    goto printf_usage;
                debug_msg = CamOsStrtol(p_val, NULL, 10);
                if (debug_msg != 0)
                    debug_msg = 1;
                pr_info("keypad_store change debugmsg = %d\n", debug_msg);
                if (NULL == pBuf)
                    break;
            }
            else if (pName != NULL && strncmp(pName, "numsel", 6) == 0)
            {
                U32 num;
                p_val = strsep(&pBuf, " ");
                if (NULL == p_val)
                    goto printf_usage;
                num              = CamOsStrtol(p_val, NULL, 10);
                ddata->data->col = num / 10;
                ddata->data->row = num % 10;
                if (NULL == pBuf)
                {
                    initflag = 1;
                    break;
                }
            }
            else
                goto printf_usage;
        }
    }
    else
        goto printf_usage;

    if (initflag == 1)
    {
        mdrv_keypad_init(ddata);
        pr_info("keypad_store call mdrv_keypad_init success\n");
    }
    return count;

printf_usage:
    pr_info("keypad_store usage:\n");
    pr_info("   echo mode=xx > keypad\n");
    pr_info("   echo debounce=xx > keypad\n");
    pr_info("   echo dbg=xx > keypad\n");
    pr_info("   echo mode=xx debounce=xx dbg=xx > keypad\n");
    return count;
}

static DEVICE_ATTR(keypad, S_IWUSR | S_IRUGO, keypad_show, keypad_store);

static struct attribute *keypad_attrs[] = {
    &dev_attr_keypad.attr,
    NULL,
};

static struct attribute_group keypad_attr_group = {
    .attrs = keypad_attrs,
};

/**
 * mdrv_keypad_hex2bit() - Returns bits of buttons Numbering
 * @val: data of register
 *
 * @Returns Number of register has been pressed bit num
 */
static U32 mdrv_keypad_hex2bit(U32 val)
{
    U32 i;
    for (i = 0; i <= 3; i++)
    {
        if ((val >> i) & BIT0)
            return i;
    }
    return -1;
}

/**
 * mdrv_keypad_GetRulenum() - Returns Number of buttons Numbering
 * @value: Button numbers of keypadnote
 * @rulenum: Store the col&row nums for get access report value
 *
 * max supprose four key pressed simultaneously. rulenum only alloc four space
 * @Returns Number of success
 */
static U32 mdrv_keypad_GetRulenum(U32 value, struct ms_keypad_rulenum *rulenum)
{
    if (value >= KEPYAD_MAXKEYNUM)
        value -= KEPYAD_MAXKEYNUM;
    rulenum->col = value / KEYPAD_MAXCOLNUM;
    rulenum->row = value % KEYPAD_MAXROWNUM;

    return TRUE;
}

/**
 * mdrv_keypad_GetPressedkey() - Returns Number of buttons pressed simultaneously
 * @data: pointer to ms_keypad_date
 * @stagecode Button release or press
 * max supprose four key pressed simultaneously
 * @Returns Number of buttons pressed simultaneously
 */
static U32 mdrv_keypad_GetPressedkey(struct ms_keypad_date *data, U32 *stagecode)
{
    U32 cnt = 0;
    U32 tmp = 0;
    U32 i = 0, j = 0;
    U32 begin = 0, end = 3;
    U32 code   = 0; /*num of Weight coefficient */
    U32 value0 = 0, value1 = 0, value2 = 0, value3 = 0;

    if (data->modesel == E_KP_STATUS_MODE_0)
    {
        if (data->type == E_KP_STATUS_RELEASE)
        {
            begin = 4;
            end   = 7;
        }
    }
    else if (data->modesel == E_KP_STATUS_MODE_3)
    {
        value0 = data->s_value[0] & 0xff;
        mdrv_keypad_GetRulenum(value0, &data->rulenum[0]);
        *stagecode = 0;
        return 1;
    }

    for (i = begin; i <= end; i++)
    {
        /* this 4 mean of 16bit have 4*4 bit*/
        for (j = 0; j < 4; j++)
        {
            tmp = ((data->s_value[i]) >> (4 * j)) & 0xf; /* judgment 4bit of register value */
            if (tmp == 0)
                continue;

            MDRV_KEYPAD_DEBUG("find value %x\n", tmp);
            code = *stagecode = i;
            switch (tmp)
            {
                case 0x1:
                case 0x2:
                case 0x4:
                case 0x8:
                {
                    if (cnt > 3)
                        return 0;
                    value0 = code * 16 + mdrv_keypad_hex2bit(tmp) + (j * 4);
                    if (!mdrv_keypad_GetRulenum(value0, &data->rulenum[cnt++]))
                        return 0;
                    continue;
                }
                case 0x3:
                case 0x5:
                case 0x9:
                case 0x6:
                case 0xA:
                case 0xC:
                {
                    if (cnt > 2)
                        return 0;
                    if (tmp % 4 == 0)
                    {
                        value0 = code * 16 + 8 + (j * 4);
                        value1 = code * 16 + 4 + (j * 4);
                    }
                    else if (tmp % 2 == 0)
                    {
                        value0 = code * 16 + 2 + (j * 4);
                        value1 = code * 16 + (tmp - 2) + (j * 4);
                    }
                    else
                    {
                        value0 = code * 16 + 1 + (j * 4);
                        value1 = code * 16 + (tmp - 1) + (j * 4);
                    }

                    mdrv_keypad_GetRulenum(value0, &data->rulenum[cnt++]);
                    mdrv_keypad_GetRulenum(value1, &data->rulenum[cnt++]);
                    continue;
                }
                case 0x7:
                case 0xB:
                case 0xe:
                {
                    if (cnt > 1)
                        return 0;
                    if (tmp % 2 != 0)
                    {
                        value0 = code * 16 + 1 + (j * 4);
                        value1 = code * 16 + 2 + (j * 4);
                        value2 = code * 16 + (tmp - 3) + (j * 4);
                    }
                    else
                    {
                        value0 = code * 16 + 2 + (j * 4);
                        value1 = code * 16 + 4 + (j * 4);
                        value2 = code * 16 + 8 + (j * 4);
                    }
                    mdrv_keypad_GetRulenum(value0, &data->rulenum[cnt++]);
                    mdrv_keypad_GetRulenum(value1, &data->rulenum[cnt++]);
                    mdrv_keypad_GetRulenum(value2, &data->rulenum[cnt++]);
                    continue;
                }
                case 0xf:
                {
                    if (cnt != 0)
                        return 0;
                    value0 = code * 16 + 1 + (j * 4);
                    value1 = code * 16 + 2 + (j * 4);
                    value2 = code * 16 + 4 + (j * 4);
                    value3 = code * 16 + 8 + (j * 4);
                    mdrv_keypad_GetRulenum(value0, &data->rulenum[cnt++]);
                    mdrv_keypad_GetRulenum(value1, &data->rulenum[cnt++]);
                    mdrv_keypad_GetRulenum(value2, &data->rulenum[cnt++]);
                    mdrv_keypad_GetRulenum(value3, &data->rulenum[cnt++]);
                    break;
                }
                default:
                    return -1;
            }
        }
    }
    return cnt;
}

/**
 * mdrv_keypadreport_event() - report keypad pressed value
 * @ddata: pointer to ms_keypad_drvdata
 *
 * pressed_num: how much of simultaneously pressed key num
 * max supprose four key pressed simultaneously
 * @Returns void
 */
static void mdrv_keypadreport_event(struct ms_keypad_drvdata *ddata)
{
    struct input_dev *input       = ddata->input;
    U32               code        = 0;
    U32               pressed_num = 0;
    U32               row = 0, col = 0;
    U32               key;

    pressed_num = mdrv_keypad_GetPressedkey(ddata->data, &code);
    MDRV_KEYPAD_DEBUG("code=%d simultaneously pressed=%d \n", code, pressed_num);

    for (key = 1; key <= pressed_num; key++)
    {
        row = ddata->data->rulenum[key - 1].row;
        col = ddata->data->rulenum[key - 1].col;

        if (code < 4)
        {
            /* pressed : value: 0-release event, 1-pressed event
             * when in mode 0 There are pressed or release have ISR.
             * when in mode mode 1|2|3 There only pressed/release have one ISR.
             * So in mode 1|2|3. there are all in code less than four No matter release or pressed
             */
            if (ddata->data->modesel == E_KP_STATUS_MODE_0)
            {
                input_event(input, EV_KEY, keypad_desc[row][col], 1);
                input_sync(input);
            }
            else
            {
                input_event(input, EV_KEY, keypad_desc[row][col], 1);
                input_sync(input);
                input_event(input, EV_KEY, keypad_desc[row][col], 0);
                input_sync(input);
            }
        }
        else
        {
            /* release : value: 0-release, 1-pressed
             * when code Greater than 4.mode 1|2|3 no enrty. so it's only for mode 0.
             */
            if (ddata->data->modesel == E_KP_STATUS_MODE_0)
            {
                input_event(input, EV_KEY, keypad_desc[row][col], 0);
                input_sync(input);
            }
        }
    }

    memset(ddata->data->rulenum, 0, sizeof(ddata->data->rulenum));
    memset(ddata->data->s_value, 0, sizeof(ddata->data->s_value));

    return;
}

#ifdef CONFIG_WORKQUEL

static void mdrv_keypadworkfunc(struct work_struct *work)
{
    struct ms_keypad_drvdata *ddata = container_of(work, struct ms_keypad_drvdata, work);

    mdrv_keypadreport_event(ddata);

    if (debug_msg)
    {
        U32 stage;
        MDRV_KEYPAD_DEBUG("->s_value:\n");
        for (stage = 0; stage <= 7; stage++)
        {
            MDRV_KEYPAD_DEBUG("%.4x\n", ddata->data->s_value[stage]);
        }
    }
    return;
}
#else
static void mdrv_keypad_timer_function(unsigned long data)
{
    struct ms_keypad_drvdata *ddata = (struct ms_keypad_drvdata *)data;

    mdrv_keypadreport_event(ddata);
    return;
}
#endif

irqreturn_t keypadIsr(int irq, void *priv)
{
    struct ms_keypad_drvdata *ddata = priv;
    U32                       stage = 0, begin = 0, end = 0, value = 0;
    unsigned long             flags;
    spin_lock_irqsave(&ddata->lock, flags);

    if (ddata->data->modesel == E_KP_STATUS_MODE_0)
    {
        ddata->data->type = mhal_KeyPad_GetTwoStatusFlag();
        MDRV_KEYPAD_DEBUG("Isr:%d\n", ddata->data->type);
        /* keypad pressed*/
        if (ddata->data->type == E_KP_STATUS_PRESSED)
        {
            begin = 0;
            end   = 3;
        }
        else /* keypad release*/
        {
            begin = 4;
            end   = 7;
        }
    }
    else
    {
        begin = 0;
        end   = 7;
    }

    for (stage = begin; stage <= end; stage++)
    {
        mhal_KeyPad_GetFinal_Stage(stage, &value);
        ddata->data->s_value[stage] = value;

        // mhal_KeyPad_GetRaw_Stage(stage,&value);
        // ddata->data->r_value[stage] = value;
    }
    // MDRV_KEYPAD_DEBUG("register:%d \n",mhal_KeyPad_GetTwoStatusFlag());

    mhal_KeyPad_ClearIrq();

#ifdef CONFIG_WORKQUEL
    schedule_work(&ddata->work);
    // mod_delayed_work(system_wq,&ddata->work,msecs_to_jiffies(1));
#else
    mod_timer(&ddata->keypad_timer, jiffies + HZ / 1000); // 1ms
#endif

    spin_unlock_irqrestore(&ddata->lock, flags);

    return IRQ_NONE;
}

/**
 * mdrv_keypadMaskAllEN() - For disable mask key interrupt
 * @num: set to zero it's will simultaneously set all keymask.
 *       if need modify one key.set num
 * @Enbale: mask the key. if TURE No matter the key is pressed or not, irq will not arise.
 * Key num correspondence num please access to keypadnote
 * @Returns void
 */
static void mdrv_keypadMaskAllEN(U32 num, bool Enbale)
{
    U32 i;
    if (num == 0)
    {
        for (i = 1; i <= KEPYAD_MAXKEYNUM_TWOSTATUS; i++)
        {
            mhal_KeyPad_SetMask(i - 1, Enbale);
        }
    }
    else
        mhal_KeyPad_SetMask(num, Enbale);
    return;
}

/**
 * mdrv_keypadSetForce() -For Force key interrupt
 * @num: set to zero it's will simultaneously set all keyForce.
 *       if need modify one key.set num
 * @Enbale: Force the key. if TURE No matter the key is pressed or not, irq will arise
 * Key num correspondence num please access to keypadnote
 * @Returns void
 */
static void mdrv_keypadSetForce(U32 num, bool Enbale)
{
    S32 i;
    if (num == 0)
    {
        for (i = 1; i <= KEPYAD_MAXKEYNUM_TWOSTATUS; i++)
        {
            mhal_KeyPad_SetForce(i - 1, Enbale);
        }
    }
    else
        mhal_KeyPad_SetForce(num, Enbale);
    return;
}

/**
 * mdrv_keypadGlhrALLEN() -For enbale keypad glictch time form HW.
 * @Enbale: glictch the key.for disable glictch remove if needed
 * @Returns void
 */
static void mdrv_keypadGlhrALLEN(bool Enbale)
{
    S32 i;
    for (i = 0; i < KEPYAD_MAXKEYNUM; i++)
    {
        mhal_KeyPad_SetGlhrEN(i, Enbale);
    }
    return;
}

/**
 * mdrv_keypadSetColRowNum() -set keypad Col&column amount
 * @row: for keypad row amount.
 * @col: for keypad column amount.
 * @Returns void
 */
static void mdrv_keypadSetColRowNum(U32 row, U32 col)
{
    mhal_KeyPad_SetColNum(col);
    mhal_KeyPad_SetRowNum(row);
    return;
}

/**
 * mdrv_keypadReset() -Reset keypad if need.
 * it will clear irq and already pressed key register value
 * @Returns void
 */
static void mdrv_keypadReset(void)
{
    mhal_KeyPad_RST(TRUE);
    mhal_KeyPad_RST(FALSE);
    return;
}

/**
 * mdrv_keypad_init() -init keypad if change mode or glictch time etc.
 * @ddata: pointer to drvdata
 * alignnum only set in mode1 or mode2.in other mode should be reset.
 * @Returns KEYPAD_SUCCESS
 */
S32 mdrv_keypad_init(struct ms_keypad_drvdata *ddata)
{
    S32 scanrate = 0;
    mutex_lock(&ddata->disable_lock);

    mdrv_keypadReset();
    mdrv_keypadSetColRowNum(ddata->data->row, ddata->data->col);
    mdrv_keypadMaskAllEN(0, FALSE);

    scanrate = mdrv_keypadcalc_SCANRATE(KEYPADADJUSTCLK);
    mhal_KeyPad_SetScanDfs(scanrate);
    mhal_KeyPad_SetGlhrmDfs(scanrate);
    mhal_KeyPad_ModeSel((KEYPAD_STATUS_MODE)ddata->data->modesel);

    if (ddata->data->modesel != E_KP_STATUS_MODE_3)
    {
        ddata->data->align = mdrv_keypadcalc_ALIGN(KEYPAD_ALIGNTIMELENGTH);
        MDRV_KEYPAD_DEBUG("Alignnum = %d\n", ddata->data->align);
        mhal_KeyPad_KEYPAD_ALIGN_NUM(ddata->data->align);
    }
    else
        mhal_KeyPad_KEYPAD_ALIGN_NUM(0);

    mdrv_keypadGlhrALLEN(TRUE);
    mdrv_keypadSetForce(0, FALSE);
    mhal_KeyPad_SetGlhrNum(ddata->data->debounce);
    mhal_KeyPad_EnableScan(TRUE);

    mutex_unlock(&ddata->disable_lock);

    return KEYPAD_SUCCESS;
}

/*
 * Translate OpenFirmware node properties into platform_data
 */
static struct ms_keypad_platform_data *mdrv_keypad_get_devtree_pdata(struct device *dev)
{
    struct device_node *            node;
    struct ms_keypad_platform_data *pdata;
    struct ms_keypad_dts_s *        keypad;
    U32                             error = 0;
#ifdef CONFIG_KEYPADTS
    char keypadname[12];
    U32  keypadvalue[8];
    U32  i, j;
#endif

    node = dev->of_node;
    if (!node)
        return ERR_PTR(-ENODEV);

    pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
    if (pdata == NULL)
    {
        dev_err(dev, "Failed to allocate pdata memory\n");
        return ERR_PTR(-ENODEV);
    }

    keypad = devm_kzalloc(dev, sizeof(*keypad), GFP_KERNEL);
    if (keypad == NULL)
    {
        dev_err(dev, "Failed to allocate dts memory\n");
        return ERR_PTR(-ENODEV);
    }

    keypad->keypad_rowcol = devm_kzalloc(dev, 2 * sizeof(*keypad->keypad_rowcol), GFP_KERNEL);
    if (keypad->keypad_rowcol == NULL)
    {
        dev_err(dev, "Failed to allocate rowcol memory\n");
        return ERR_PTR(-EINVAL);
    }

    if ((error != of_property_read_u32_array(node, "numsel", keypad->keypad_rowcol, 2)))
    {
        dev_err(dev, "Failed to allocate numsel memory\n");
        return ERR_PTR(-EINVAL);
    }

    pdata->ms_keypadts = keypad;

    pdata->ms_keypadts->irq = irq_of_parse_and_map(node, 0);
    if (!pdata->ms_keypadts->irq)
    {
        dev_err(dev, "Failed to get keypad irq\n");
        return ERR_PTR(-EINVAL);
    }

    if (of_property_read_u32(node, "keypadmode", &keypad->keypadmode))
    {
        dev_err(dev, "Failed to get keypadmode.Defalut set mdoe0.\n");
        keypad->keypadmode = 0;
    }

    // if (of_property_read_u32(node, "debounce",&keypad->debounce))
    {
        keypad->debounce = mdrv_keypadcalc_DEBOUNCE(KEYPADINPUTCLK);
    }
#ifdef CONFIG_KEYPADTS

    for (i = 0; i < KEYPAD_MAXROWNUM; i++)
    {
        sprintf(keypadname, "keypad-row%d", i + 1);
        if ((error != of_property_read_u32_array(node, keypadname, keypadvalue, 8)))
        {
            dev_err(dev, "Failed to get %s\n", keypadname);
            return ERR_PTR(-EINVAL);
        }
        for (j = 0; j < KEYPAD_MAXCOLNUM; j++)
        {
            keypad_desc[i][j] = keypadvalue[j];
        }
    }

#endif

    return pdata;
}

static S32 mdrv_keypad_probe(struct platform_device *pdev)
{
    S32                             s32Ret, i, j;
    size_t                          size;
    struct input_dev *              input;
    struct device *                 dev   = &pdev->dev;
    struct ms_keypad_platform_data *pdata = dev_get_platdata(dev);
    struct ms_keypad_drvdata *      ddata;
    struct ms_keypad_date *         data;

    if (!pdata)
    {
        pdata = mdrv_keypad_get_devtree_pdata(dev);
        if (IS_ERR(pdata))
            return PTR_ERR(pdata);
    }

    size = sizeof(struct ms_keypad_date) + 4 * sizeof(struct ms_keypad_rulenum);
    data = devm_kzalloc(dev, size, GFP_KERNEL);
    if (!data)
    {
        dev_err(dev, "Failed to allocate date\n");
        return -ENOMEM;
    }

    ddata = devm_kzalloc(dev, sizeof(struct ms_keypad_drvdata), GFP_KERNEL);
    if (!ddata)
    {
        dev_err(dev, "Failed to allocate drvdata\n");
        return -ENOMEM;
    }

    input = devm_input_allocate_device(dev);
    if (!input)
    {
        dev_err(dev, "Failed to allocate input device\n");
        return -ENOMEM;
    }

    platform_set_drvdata(pdev, ddata);
    input_set_drvdata(input, ddata);

    ddata->data  = data;
    ddata->pdata = pdata;
    ddata->input = input;

    input->name       = pdata->name ?: pdev->name;
    input->phys       = "keypad/input0";
    input->dev.parent = &pdev->dev;
    input->id.bustype = BUS_HOST;
    input->id.vendor  = 0x0001;
    input->id.product = 0x0001;
    input->id.version = 0x0100;

    ddata->data->irq      = pdata->ms_keypadts->irq;
    ddata->data->modesel  = pdata->ms_keypadts->keypadmode;
    ddata->data->debounce = pdata->ms_keypadts->debounce;
    ddata->data->row      = pdata->ms_keypadts->keypad_rowcol[0];
    ddata->data->col      = pdata->ms_keypadts->keypad_rowcol[1];

    set_bit(EV_KEY, input->evbit);
    for (i = 0; i < ddata->data->col; i++)
    {
        for (j = 0; j < ddata->data->row; j++)
        {
            set_bit(keypad_desc[i][j], input->keybit);
        }
    }

    s32Ret = input_register_device(input);
    if (s32Ret)
    {
        dev_err(dev, "Unable to register input device, error: %d\n", s32Ret);
        return -EBUSY;
    }

    s32Ret = sysfs_create_group(&pdev->dev.kobj, &keypad_attr_group);
    if (s32Ret)
    {
        dev_err(dev, "Unable to export keys/switches, error: %d\n", s32Ret);
        return s32Ret;
    }

    mutex_init(&ddata->disable_lock);
    spin_lock_init(&ddata->lock);

#ifdef CONFIG_WORKQUEL
    INIT_WORK(&ddata->work, mdrv_keypadworkfunc);
#else
    setup_timer(&ddata->keypad_timer, mdrv_keypad_timer_function, (unsigned long)ddata);
#endif

    mhal_keypad_pinmux();
    mdrv_keypad_init(ddata);
    MDRV_KEYPAD_DEBUG("keypad_Isr = %d\n", pdata->ms_keypadts->irq);

    s32Ret = request_irq(ddata->data->irq, keypadIsr, IRQF_SHARED, "keypad_Isr0", ddata);
    if (s32Ret)
    {
        dev_err(dev, "Unable to request irq %d, error: %d\n", ddata->data->irq, s32Ret);
        return -EBUSY;
    }

    return KEYPAD_SUCCESS;
}

static S32 mdrv_keypad_remove(struct platform_device *pdev)
{
    struct ms_keypad_drvdata *ddata = platform_get_drvdata(pdev);
    struct input_dev *        input = ddata->input;

    free_irq(ddata->data->irq, ddata);
#ifdef CONFIG_WORKQUEL
    cancel_work_sync(&ddata->work);
#else
    del_timer_sync(&ddata->keypad_timer);
#endif
    input_unregister_device(input);
    input_free_device(input);

    sysfs_remove_group(&pdev->dev.kobj, &keypad_attr_group);
    MDRV_KEYPAD_DEBUG("%s success\n", __FUNCTION__);
    return 0;
}

static const struct of_device_id mdrv_keypad_of_match_table[] = {{.compatible = "sstar,keypad"}, {}};

static struct platform_driver mdrv_keypad_platform_driver = {
    .remove = mdrv_keypad_remove,
    .probe  = mdrv_keypad_probe,
    .driver =
        {
            .name           = "sstar,keypad",
            .owner          = THIS_MODULE,
            .of_match_table = mdrv_keypad_of_match_table,
        },
};

module_platform_driver(mdrv_keypad_platform_driver);

MODULE_AUTHOR("Sigmastar Technology, Inc.");
MODULE_DESCRIPTION("Driver for Sigmastar keypad");
MODULE_LICENSE("GPL v2");
