/*
 * hal_rtcpwc.c- Sigmastar
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
#include <ms_platform.h>
#include <ms_types.h>
#include <ms_msys.h>
#include <cam_os_wrapper.h>
#include <hal_rtcpwc.h>
#include <hal_rtcpwcreg.h>

#define RTC_CHECK_STATUS_DELAY_TIME_US 100

#define READ_WORD(_reg)        (*(volatile u16 *)(_reg))
#define WRITE_WORD(_reg, _val) (*((volatile u16 *)(_reg))) = (u16)(_val)
#define WRITE_WORD_MASK(_reg, _val, _mask) \
    (*((volatile u16 *)(_reg))) = ((*((volatile u16 *)(_reg))) & ~(_mask)) | ((u16)(_val) & (_mask))

#define RTC_READ(_reg_)         READ_WORD(hal->rtc_base + ((_reg_) << 2))
#define RTC_WRITE(_reg_, _val_) WRITE_WORD(hal->rtc_base + ((_reg_) << 2), (_val_))

#define RTC_WRITE_MASK(_reg_, _val_, mask) WRITE_WORD_MASK(hal->rtc_base + ((_reg_) << 2), (_val_), (mask))

bool hal_rtc_iso_ctrl_ex(struct hal_rtcpwc_t *hal)
{
    u16 reg     = 0;
    u8  ubCheck = ISO_ACK_RETRY_TIME;

    RTC_WRITE(RTCPWC_ISO_TRIG, RTCPWC_ISO_TRIG_BIT);
    reg = RTC_READ(RTCPWC_ISO_EN_ACK);
    reg = reg & RTCPWC_ISO_EN_ACK_BIT;

    while ((!reg) && (--ubCheck))
    {
        CamOsUsSleep(RTC_CHECK_STATUS_DELAY_TIME_US);
        reg = RTC_READ(RTCPWC_ISO_EN_ACK);
        reg = reg & RTCPWC_ISO_EN_ACK_BIT;
    }
    if (ubCheck == 0)
        return FALSE;

    RTC_WRITE(RTCPWC_ISO_EN_CLR, RTCPWC_ISO_EN_CLR_BIT);

    reg     = RTC_READ(RTCPWC_ISO_EN_ACK);
    reg     = reg & RTCPWC_ISO_EN_ACK_BIT;
    ubCheck = ISO_ACK_RETRY_TIME;
    while ((reg) && (--ubCheck))
    {
        CamOsUsSleep(RTC_CHECK_STATUS_DELAY_TIME_US);
        reg = RTC_READ(RTCPWC_ISO_EN_ACK);
        reg = reg & RTCPWC_ISO_EN_ACK_BIT;
    }
    if (ubCheck == 0)
        return FALSE;

    return TRUE;
}

static int hal_rtc_has_1k_clk(struct hal_rtcpwc_t *hal)
{
    u8  ubCheck = ISO_ACK_RETRY_TIME;
    u16 reg     = 0;

    do
    {
        reg = RTC_READ(RTCPWC_DIG2PWC_RTC_TESTBUS);
        if (reg & RTCPWC_CLK_1K)
        {
            return 1;
        }
        CamOsUsSleep(RTC_CHECK_STATUS_DELAY_TIME_US);
        ubCheck--;
    } while (ubCheck);
    return 0;
}

static void hal_rtc_iso_ctrl(struct hal_rtcpwc_t *hal)
{
    static int warn_once = 0;
    u8         ubCheck   = ISO_ACK_RETRY_TIME;

    if (0 == hal_rtc_has_1k_clk(hal))
    {
        if (!warn_once)
        {
            warn_once = 1;
            printk("[%s][%d] RTCPWC fail to enter correct state and possibly caused by no power supplied\n",
                   __FUNCTION__, __LINE__);
        }
        hal->fail_count.clock_fail++;
        return;
    }

    while (!hal_rtc_iso_ctrl_ex(hal) && (--ubCheck))
    {
        hal->fail_count.iso_fail++;
        CamOsUsSleep(RTC_CHECK_STATUS_DELAY_TIME_US);
    }
}

//------------------------------------------------------------------------------
//  Function    : ms_RTC_GetSW0
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for getting RTC SW0.

This function is used for getting RTC SW0.
@param[out] The value of RTC SW0(magic number).
@return It reports the status of the operation.
*/
// SW0 has only 16 bits
u32 hal_rtc_get_sw0(struct hal_rtcpwc_t *hal)
{
    u16 BaseH = 0, BaseL = 0;
    u32 ulBaseTime = 0;
    u16 reg        = 0;
    // I.   read SW0
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_SW0_RD);
    hal_rtc_iso_ctrl(hal);
    // read base time
    BaseH = RTC_READ(RTCPWC_RTC2DIG_RDDATA_H);
    BaseL = RTC_READ(RTCPWC_RTC2DIG_RDDATA_L);
    // RTC_DBG("SW0 BaseH %x \n", BaseH);
    // RTC_DBG("SW0 BaseL %x \n", BaseL);
    ulBaseTime = BaseH << 16;
    ulBaseTime |= BaseL;
    // reset read bit of base time
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_SW0_RD);
    return ulBaseTime;
}

// SW0 has only 16 bits
void hal_rtc_set_sw0(struct hal_rtcpwc_t *hal, u32 val)
{
    u16 reg = 0;
    // Set sw bit
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_SW0_WR);
    // Set sw password
    RTC_WRITE(RTCPWC_DIG2RTC_WRDATA_L, ((val >> 0) & 0xFFFF));
    RTC_DBG("Set RTC SetSW0=%x\r\n", RTC_READ(RTCPWC_DIG2RTC_WRDATA_L));
    // Trigger ISO
    hal_rtc_iso_ctrl(hal);
    // reset control bits
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_SW0_WR);
}

#ifdef CONFIG_RTCPWC_INNER_EHHE
// SW1 has only 16 bits
u32 hal_rtc_get_sw1(struct hal_rtcpwc_t *hal)
{
    u16 reg        = 0;
    u16 BaseH      = 0;
    u16 BaseL      = 0;
    u32 ulBaseTime = 0;
    // I.   read SW1
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_SW1_RD);
    hal_rtc_iso_ctrl(hal);
    // read base time
    BaseH      = RTC_READ(RTCPWC_RTC2DIG_RDDATA_H);
    BaseL      = RTC_READ(RTCPWC_RTC2DIG_RDDATA_L);
    ulBaseTime = BaseH << 16;
    ulBaseTime |= BaseL;
    // reset read bit of base time
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_SW1_RD);
    return ulBaseTime;
}

// SW1 has only 16 bits
void hal_rtc_set_sw1(struct hal_rtcpwc_t *hal, u32 val)
{
    u16 reg = 0;
    // Set sw bit
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_SW1_WR);
    // Set sw password
    RTC_WRITE(RTCPWC_DIG2RTC_WRDATA_L, ((val >> 0) & 0xFFFF));
    RTC_DBG("Set RTC SetSW1=%x\r\n", RTC_READ(RTCPWC_DIG2RTC_WRDATA_L));
    // Trigger ISO
    hal_rtc_iso_ctrl(hal);
    // reset control bits
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_SW1_WR);
}
#endif

static u32 hal_rtc_get_base(struct hal_rtcpwc_t *hal)
{
    u16 reg;
    u16 BaseH      = 0;
    u16 BaseL      = 0;
    u32 ulBaseTime = 0;

    // reset read bit
    // reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    // RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_CNT_RD_BIT);

    // Set read bit of base time
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_BASE_RD);
    hal_rtc_iso_ctrl(hal);
    // read base time
    BaseH = RTC_READ(RTCPWC_RTC2DIG_RDDATA_H);
    BaseL = RTC_READ(RTCPWC_RTC2DIG_RDDATA_L);
    RTC_DBG("BaseH %x \n", BaseH);
    RTC_DBG("BaseL %x \n", BaseL);
    ulBaseTime = BaseH << 16;
    ulBaseTime |= BaseL;
    // reset read bit of base time
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_BASE_RD);

    return ulBaseTime;
}

//------------------------------------------------------------------------------
//  Function    : ms_RTC_SetBaseTime
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for getting RTC BaseTime.

This function is used for getting RTC BaseTime.
@param[out] The value of RTC BaseTime.
@return It reports the status of the operation.
*/
void hal_rtc_set_base_time(struct hal_rtcpwc_t *hal, u32 seconds)
{
    u16 reg;

#ifndef CONFIG_RTCPWC_SW_RST_OFF
    // Toggle reset
    reg = RTC_READ(RTCPWC_DIG2PWC_OPT);
    RTC_DBG("%s: RTC SW reset\r\n", __FUNCTION__);
    RTC_WRITE(RTCPWC_DIG2PWC_OPT, reg | RTCPWC_SW_RST);
    CamOsMsSleep(1);
    RTC_WRITE(RTCPWC_DIG2PWC_OPT, reg);
#endif

    hal->base_time.is_vaild  = TRUE;
    hal->base_time.base_time = seconds;

    // Set Base time bit
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_BASE_WR_BIT);

    // Set RTC Base Time
    RTC_WRITE(RTCPWC_DIG2RTC_WRDATA_L, (u16)seconds);
    RTC_WRITE(RTCPWC_DIG2RTC_WRDATA_H, (u16)((seconds) >> 16));
    RTC_DBG("Set RTC Base Time=%x\r\n", RTC_READ(RTCPWC_DIG2RTC_WRDATA_L));
    RTC_DBG("Set RTC Base Time=%x\r\n", RTC_READ(RTCPWC_DIG2RTC_WRDATA_H));

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);

    // Set counter RST bit
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_CNT_RST_WR);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_CNT_RST_WR);

    // reset control bits
    reg = RTC_READ(RTCPWC_DIG2RTC_SET);
    RTC_WRITE(RTCPWC_DIG2RTC_SET, reg & ~RTCPWC_DIG2RTC_SET_BIT);
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_BASE_WR_BIT);
}

//------------------------------------------------------------------------------
//  Function    : ms_RTC_GetBaseTime
//  Description :
//------------------------------------------------------------------------------
/** @brief This function is used for getting RTC BaseTime.

This function is used for getting RTC BaseTime.
@param[out] The value of RTC BaseTime.
@return It reports the status of the operation.
*/
static u32 hal_rtc_get_base_time(struct hal_rtcpwc_t *hal)
{
    u32 password = 0;
    u32 BaseTime = 0;
#ifdef CONFIG_RTCPWC_INNER_EHHE
    u32 Sw1Value = 0;
#endif

    if (hal->base_time.is_vaild)
    {
        return hal->base_time.base_time;
    }

    password = hal_rtc_get_sw0(hal);
#ifdef CONFIG_RTCPWC_INNER_EHHE
    BaseTime = hal_rtc_get_base(hal);
    Sw1Value = hal_rtc_get_sw1(hal);
#endif

#ifdef CONFIG_RTCPWC_INNER_EHHE
    RTC_DBG("password = %08x BaseTime = %08x Sw1Value = %08x\r\n", password, BaseTime, Sw1Value);
#else
    RTC_DBG("password = %08x\r\n", password);
#endif

#ifdef CONFIG_RTCPWC_INNER_EHHE
    if ((password == RTC_PASSWORD) && ((Sw1Value & 0xFFFF) == (BaseTime & 0xFFFF)))
#else
    if (password == RTC_PASSWORD)
#endif
    {
#ifndef CONFIG_RTCPWC_INNER_EHHE
        BaseTime = hal_rtc_get_base(hal);
#endif
    }
    else
    {
        RTC_ERR("Please set rtc timer (hwclock -w) \n");
        hal_rtc_set_base_time(hal, hal->default_base);
        hal_rtc_set_sw0(hal, RTC_PASSWORD);
        BaseTime = hal->default_base;
#ifdef CONFIG_RTCPWC_INNER_EHHE
        hal_rtc_set_sw1(hal, hal->default_base);
#endif // #ifdef CONFIG_RTCPWC_INNER_EHHE
    }

    hal->base_time.is_vaild  = TRUE;
    hal->base_time.base_time = BaseTime;

    return BaseTime;
}

int hal_rtc_read_time(struct hal_rtcpwc_t *hal, u32 *seconds)
{
    u16 reg           = 0;
    u32 run_sec       = 0;
    u16 counterH      = 0;
    u16 counterL      = 0;
    u32 chk_times     = 5;
    u64 second_result = 0;

    second_result = hal_rtc_get_base_time(hal);

    RTC_DBG("second_result= 0x%llx\r\n", second_result);

    if (RTC_PASSWORD == hal_rtc_get_sw0(hal))
    {
        // Read RTC Counter
        do
        {
            // Set read bit of RTC counter
            reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
            RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg | RTCPWC_DIG2RTC_CNT_RD_BIT);

            // Trigger ISO
            hal_rtc_iso_ctrl(hal);
            chk_times = 5;

            // Latch RTC counter and Check valid bit of RTC counter
            do
            {
                reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD_TRIG);
                RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD_TRIG, reg | RTCPWC_DIG2RTC_CNT_RD_TRIG_BIT);
                // Note : The first to retrieve RTC counter will failed without below delay
                CamOsUsSleep(200);
            } while ((RTC_READ(RTCPWC_RTC2DIG_CNT_UPDATING) & RTCPWC_RTC2DIG_CNT_UPDATING_BIT) && (chk_times--));

            if (chk_times == 0)
            {
                hal->fail_count.read_fail++;
                RTC_ERR("Check valid bit of RTC counter failed!\n");
                // Reset read bit of RTC counter
                reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
                RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_CNT_RD_BIT);
                return 0;
            }

            // read RTC counter
            {
                counterH = RTC_READ(RTCPWC_REG_RTC2DIG_RDDATA_CNT_H);
                counterL = RTC_READ(RTCPWC_REG_RTC2DIG_RDDATA_CNT_L);
                run_sec  = counterH << 16;
                run_sec |= counterL;
                RTC_DBG("CounterL = 0x%x\r\n", counterL);
                RTC_DBG("CounterH = 0x%x\r\n", counterH);
            }
            // Reset read bit of RTC counter
            reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
            RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_CNT_RD_BIT);
        } while (0);
    }
    RTC_DBG("run_sec = 0x%x\r\n", run_sec);
    RTC_DBG("second_result = 0x%llx\r\n", second_result);
    second_result = second_result + run_sec;

    //_RTC_PRINT("Base = 0x%x, counter = 0x%x, ullSeconds = 0x%x\n",m_ulBaseTimeInSeconds,run_sec,ullSeconds);

    if (second_result > 0xFFFFFFFF)
    {
        second_result = 0xFFFFFFFF;
    }

    *seconds = second_result;
    return 0;
}

int hal_rtc_set_time(struct hal_rtcpwc_t *hal, u32 seconds)
{
    hal_rtc_set_base_time(hal, seconds);
    hal_rtc_set_sw0(hal, RTC_PASSWORD);
#ifdef CONFIG_RTCPWC_INNER_EHHE
    hal_rtc_set_sw1(hal, seconds);
#endif // #ifdef CONFIG_RTCPWC_INNER_EHHE
    return 0;
}

static unsigned long hal_rtc_get_count(struct hal_rtcpwc_t *hal)
{
    u16 reg       = 0;
    u32 run_sec   = 0;
    u32 chk_times = 5;
    u16 counterH  = 0;
    u16 counterL  = 0;

    // Set read bit of RTC counter
    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg | RTCPWC_DIG2RTC_CNT_RD_BIT);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);
    chk_times = 5;

    // Latch RTC counter and Check valid bit of RTC counter
    do
    {
        reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD_TRIG);
        RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD_TRIG, reg | RTCPWC_DIG2RTC_CNT_RD_TRIG_BIT);
        // Note : The first to retrieve RTC counter will failed without below delay
        CamOsUsSleep(200);
    } while ((RTC_READ(RTCPWC_RTC2DIG_CNT_UPDATING) & RTCPWC_RTC2DIG_CNT_UPDATING_BIT) && (chk_times--));

    if (chk_times == 0)
    {
        hal->fail_count.read_fail++;
        RTC_ERR("Check valid bit of RTC counter failed!\n");
        // Reset read bit of RTC counter
        reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
        RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_CNT_RD_BIT);
        return 0;
    }

    // read RTC counter
    {
        counterH = RTC_READ(RTCPWC_REG_RTC2DIG_RDDATA_CNT_H);
        counterL = RTC_READ(RTCPWC_REG_RTC2DIG_RDDATA_CNT_L);
        run_sec  = counterH << 16;
        run_sec |= counterL;
        RTC_DBG("CounterL = 0x%x\r\n", counterL);
        RTC_DBG("CounterH = 0x%x\r\n", counterH);
    }
    // Reset read bit of RTC counter
    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_CNT_RD_BIT);

    return run_sec;
}

static void hal_rtc_set_alarm_time(struct hal_rtcpwc_t *hal, u32 seconds)
{
    u16 reg;

    // Clear ALARM Interrupt
    // reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    // RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg | RTCPWC_DIG2RTC_INT_CLR_BIT);

    // Set Base time bit
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg | RTCPWC_DIG2RTC_ALARM_WR);

    // Set RTC Alarm Time
    RTC_WRITE(RTCPWC_DIG2RTC_WRDATA_L, (u16)seconds);
    RTC_WRITE(RTCPWC_DIG2RTC_WRDATA_H, (u16)((seconds) >> 16));
    RTC_DBG("Set RTC Alarm Time=%x\r\n", RTC_READ(RTCPWC_DIG2RTC_WRDATA_L));
    RTC_DBG("Set RTC Alarm Time=%x\r\n", RTC_READ(RTCPWC_DIG2RTC_WRDATA_H));

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);

    // Reset control bits
    reg = RTC_READ(RTCPWC_DIG2RTC_BASE_WR);
    RTC_WRITE(RTCPWC_DIG2RTC_BASE_WR, reg & ~RTCPWC_DIG2RTC_ALARM_WR);

    // Reset control bits
    // reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    // RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_INT_CLR_BIT);
}

void hal_rtc_get_alarm(struct hal_rtcpwc_t *hal, u32 *seconds)
{
    u16 reg;
    u32 base;
    u32 alarm;

    base = hal_rtc_get_base(hal);

    // Set read bit of alarm
    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg | RTCPWC_DIG2RTC_ALARM_RD_BIT);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);

    // Reset control bits
    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_ALARM_RD_BIT);

    alarm = RTC_READ(RTCPWC_RTC2DIG_RDDATA_L) | (RTC_READ(RTCPWC_RTC2DIG_RDDATA_H) << 16);

    *seconds = base + alarm;
}

void hal_rtc_set_alarm(struct hal_rtcpwc_t *hal, u32 seconds)
{
    // set alarm time
    hal->alarm_time = hal_rtc_get_count(hal) + seconds;

    hal_rtc_set_alarm_time(hal, hal->alarm_time);
}

u32 hal_rtc_get_rddata(struct hal_rtcpwc_t *hal)
{
    return ((RTC_READ(RTCPWC_RTC2DIG_RDDATA_H) << 16) | (RTC_READ(RTCPWC_RTC2DIG_RDDATA_L)));
}

void hal_rtc_alarm_enable(struct hal_rtcpwc_t *hal, u8 enable)
{
    u16 reg;

    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    if (enable)
        reg |= RTCPWC_DIG2RTC_ALARM_EN_BIT;
    else
        reg &= ~RTCPWC_DIG2RTC_ALARM_EN_BIT;

    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);
}

static void hal_rtc_power_init(struct hal_rtcpwc_t *hal)
{
    u16 reg;

    if (hal->pwc_io4_valid)
    {
        reg = RTC_READ(RTCPWC_DIG2PWC_PWR_EN_CTRL);
        if (!(hal->pwc_io4_value & 0x2))
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR_EN_CTRL, reg & ~RTCPWC_ALARM_ON_EN);
        }
        else
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR_EN_CTRL, reg | RTCPWC_ALARM_ON_EN);
        }

        reg = RTC_READ(RTCPWC_DIG2PWC_PWR_EN_CTRL);
        if (!(hal->pwc_io4_value & 0x1))
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR_EN_CTRL, reg & ~RTCPWC_PWR_EN);
        }
        else
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR_EN_CTRL, reg | RTCPWC_PWR_EN);
        }

        // Trigger ISO
        hal_rtc_iso_ctrl(hal);
    }

    if (hal->pwc_io5_valid)
    {
        reg = RTC_READ(RTCPWC_DIG2PWC_PWR2_EN_CTRL);
        if (!(hal->pwc_io5_value & 0x2))
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR2_EN_CTRL, reg & ~RTCPWC_ALARM2_ON_EN);
        }
        else
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR2_EN_CTRL, reg | RTCPWC_ALARM2_ON_EN);
        }

        reg = RTC_READ(RTCPWC_DIG2PWC_PWR2_EN_CTRL);
        if (!(hal->pwc_io5_value & 0x1))
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR2_EN_CTRL, reg & ~RTCPWC_PWR2_EN);
        }
        else
        {
            RTC_WRITE(RTCPWC_DIG2PWC_PWR2_EN_CTRL, reg | RTCPWC_PWR2_EN);
        }

        // Trigger ISO
        hal_rtc_iso_ctrl(hal);
    }
}

static void hal_rtc_alarm_init(struct hal_rtcpwc_t *hal)
{
    if (hal->pwc_alm_enable)
    {
        hal_rtc_alarm_enable(hal, 1);
    }

    if (hal->pwc_alm_init_on && hal->pwc_alm_enable)
    {
        hal_rtc_set_alarm(hal, 1);
        return;
    }

    hal_rtc_power_init(hal);
}

void hal_rtc_event_init(struct hal_rtcpwc_t *hal)
{
    u16 reg;

    if (hal->pwc_io0_hiz)
    {
        reg = RTC_READ(RTCPWC_DIG2PWC_PWR2_EN_CTRL);
        reg |= RTCPWC_IO0_HIZ_EN_BIT;
        RTC_WRITE(RTCPWC_DIG2PWC_PWR2_EN_CTRL, reg);

        // Trigger ISO
        hal_rtc_iso_ctrl(hal);
    }

    if (hal->pwc_io2_valid)
    {
        reg = RTC_READ(RTCPWC_WOS_CTRL_REG);
        reg &= ~RTCPWC_WOS_CLR_BIT;
        reg |= RTCPWC_WOS_HP_EN_BIT;
        reg &= ~RTCPWC_WOS_CMPOUT_SEL_MASK;
        reg |= ((hal->pwc_io2_cmp << RTCPWC_WOS_CMPOUT_SEL_SHIFT) & RTCPWC_WOS_CMPOUT_SEL_MASK);
        RTC_WRITE(RTCPWC_WOS_CTRL_REG, reg);

        if (hal->pwc_io2_vlsel > 7)
        {
            hal->pwc_io2_vlsel = 7;
        }
        if (hal->pwc_io2_vhsel > 7)
        {
            hal->pwc_io2_vhsel = 7;
        }
        reg = (hal->pwc_io2_vlsel << 3) | hal->pwc_io2_vhsel;
        RTC_WRITE(RTCPWC_WOS_V_SEL_REG, reg);

        // Trigger ISO
        hal_rtc_iso_ctrl(hal);
    }

    if (hal->pwc_io3_pu)
    {
        reg = RTC_READ(RTCPWC_DIG2PWC_PWR2_EN_CTRL);
        reg &= ~RTCPWC_IO3_POL_BIT;
        reg |= RTCPWC_IO3_PUPD_SEL_BIT;
        RTC_WRITE(RTCPWC_DIG2PWC_PWR2_EN_CTRL, reg);

        // Trigger ISO
        hal_rtc_iso_ctrl(hal);
    }
}

u32 hal_rtc_get_alarm_int(struct hal_rtcpwc_t *hal)
{
    return (RTC_READ(RTCPWC_RTC2DIG_ISO_ALARM_INT) & RTCPWC_RTC2DIG_ISO_ALARM_INT_BIT);
}

void hal_rtc_clear_alarm_int(struct hal_rtcpwc_t *hal)
{
    u16 reg;

    // Clear ALARM Interrupt
    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg | RTCPWC_DIG2RTC_INT_CLR_BIT);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);

    // Reset control bits
    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg & ~RTCPWC_DIG2RTC_INT_CLR_BIT);
}

void hal_rtc_power_off(struct hal_rtcpwc_t *hal)
{
    u16 reg;

    // Enable all power control for wakeup
    reg = RTC_READ(RTCPWC_DIG2PWC_PWR2_EN_CTRL);
    reg |= RTCPWC_PWR2_EN | RTCPWC_ALARM2_ON_EN;
    RTC_WRITE(RTCPWC_DIG2PWC_PWR2_EN_CTRL, reg);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);

    // Enable pwc1 alarm for wakeup
    reg = RTC_READ(RTCPWC_DIG2PWC_PWR_EN_CTRL);
    reg |= RTCPWC_ALARM_ON_EN;
    RTC_WRITE(RTCPWC_DIG2PWC_PWR_EN_CTRL, reg);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);

    // Clear ALARM Interrupt
    reg = RTC_READ(RTCPWC_DIG2RTC_CNT_RD);
    reg |= RTCPWC_DIG2RTC_INT_CLR_BIT;
    RTC_WRITE(RTCPWC_DIG2RTC_CNT_RD, reg);

    // Turn off power enable
    reg = RTC_READ(RTCPWC_DIG2PWC_PWR_EN_CTRL);
    reg &= ~RTCPWC_PWR_EN;
    RTC_WRITE(RTCPWC_DIG2PWC_PWR_EN_CTRL, reg);

    // Trigger ISO
    hal_rtc_iso_ctrl(hal);
}

void hal_rtc_interrupt(struct hal_rtcpwc_t *hal)
{
    if (hal->pwc_alm_init_on && hal->pwc_alm_enable)
    {
        hal_rtc_power_init(hal);
    }
}

s16 hal_rtc_get_offset(struct hal_rtcpwc_t *hal)
{
    return 0;
}

void hal_rtc_set_offset(struct hal_rtcpwc_t *hal, s16 offset) {}

void hal_rtc_init(struct hal_rtcpwc_t *hal)
{
    if (hal->iso_auto_regen)
    {
        RTC_WRITE_MASK(RTCPWC_ISO_AUTO_REGEN, BIT0, BIT0);
    }

    hal_rtc_alarm_init(hal);
    hal_rtc_event_init(hal);
    hal_rtc_set_offset(hal, hal->offset_count);
}

static char *hal_rtc_wakeup_name[RTC_WAKEUP_MAX] = {
    [RTC_IO0_WAKEUP] = "rtc_io0", [RTC_IO1_WAKEUP] = "rtc_io1",     [RTC_IO2_WAKEUP] = "rtc_io2",
    [RTC_IO3_WAKEUP] = "rtc_io3", [RTC_ALARM_WAKEUP] = "rtc_alarm",
};

char *hal_rtc_get_wakeup_name(struct hal_rtcpwc_t *hal)
{
    u16 reg;

    reg = RTC_READ(RTC_PWC_PWC2DIG_FLAG) & RTC_PWC_PWC2DIG_FLAG_MASK;
    if (reg >= RTC_WAKEUP_MAX)
        return NULL;
    else
        return hal_rtc_wakeup_name[reg];
}

u16 hal_rtc_get_wakeup_value(struct hal_rtcpwc_t *hal)
{
    return RTC_READ(RTC_PWC_PWC2DIG_FLAG) & RTC_PWC_PWC2DIG_FLAG_MASK;
}

static char *hal_rtc_event_name[RTC_STATE_MAX] = {
    [RTC_IO0_STATE] = "rtc_io0",
    [RTC_IO1_STATE] = "rtc_io1",
    [RTC_IO2_STATE] = "rtc_io2",
    [RTC_IO3_STATE] = "rtc_io3",
};

u16 hal_rtc_get_event_state(struct hal_rtcpwc_t *hal)
{
    return RTC_READ(RTC_PWC_PWC2DIG_STATE) & RTC_PWC_PWC2DIG_STATE_MASK;
}

char **hal_rtc_get_event_name(struct hal_rtcpwc_t *hal)
{
    return &hal_rtc_event_name[0];
}
