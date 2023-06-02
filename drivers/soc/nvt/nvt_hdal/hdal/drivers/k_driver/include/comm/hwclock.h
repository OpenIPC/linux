#ifndef _HWCLOCK_H
#define _HWCLOCK_H

/**
     Get system counter us with 32 bit.

     @return Counter value, bit 31~0: us, NOTE: it will overflow after 71.58 min!
*/
extern UINT32 hwclock_get_counter(void);

/**
     Get system long counter us with 64 bit.

     @return LongCounter value, 63~0: us. NOTE: it will overflow after 584901 year!
*/
extern UINT64 hwclock_get_longcounter(void);

/**
     Get the time diff of two longcounter time.

     Get the time diff of two longcounter time.

     @return time diff value of us.
*/
extern UINT64 hwclock_diff_longcounter(UINT64 time_start, UINT64 time_end);

/**
     If long counter init ready.

     If long counter init ready.

     @return ready or not.
*/
extern BOOL hwclock_is_longcounter_ready(void);

#if defined(__FREERTOS)
#define TIME_ID_CURRENT     1 ///< current time
#define TIME_ID_ALARM       2 ///< alarm time => event callback function.
#define TIME_ID_HWRT        3 ///< hw reset time (by power alarm timer)
#define TIME_ID_SWRT        4 ///< sw reset counter (by watch dog timer)

typedef enum {
	HWCLOCK_MODE_RTC,   ///< battery rtc
	HWCLOCK_MODE_DRTC,  ///< dc power rtc
	ENUM_DUMMY4WORD(HWCLOCK_MODE)
} HWCLOCK_MODE;

typedef enum {
	HWCLOCK_PERMISSION_READWRITE, ///< read/write
	HWCLOCK_PERMISSION_READONLY,  ///< read only
	ENUM_DUMMY4WORD(HWCLOCK_PERMISSION)
} HWCLOCK_PERMISSION;

typedef enum {
	HWCLOCK_PARAM_ID_HWCLOCK_PERMISSION, ///< HwClock permission
	HWCLOCK_PARAM_ID_READ_TIME_OFFSET,   ///< read HwClock time offset
	ENUM_DUMMY4WORD(HWCLOCK_PARAM_ID)
} HWCLOCK_PARAM_ID;

/**
     Init system timer

     Init system timer of counter and longcounter, this API will be called when 1st task start
*/
extern void hwclock_init(void); ///< only for insmod on rtos

/**
     Open HwClock

     Open HwClock

     @param[in] handle      HwClock handle. 0 for internal HwClock.

     @return Operation status
        - @b E_OK       : Operation successful
        - @b E_ID       : Outside semaphore ID number range
        - @b E_NOEXS    : Semaphore does not yet exist
*/
extern ER hwclock_open(HWCLOCK_MODE mode);

/**
     Close HwClock

     Close HwClock
*/
extern void hwclock_close(void);

/**
     Set time

     Set time of timeID

     @param[in] id      parameter id.
     @param[in] p1      parameter 1.
     @param[in] p2      parameter 2.

     @return
        - @b E_OK: No error.
        - @b E_PAR: Parameter error.
*/
extern ER hwclock_set_param(HWCLOCK_PARAM_ID id, UINT32 p1, UINT32 p2);

/**
     Set time

     Set time of timeID

     @param[in] id      parameter id.
     @param[in] p1      parameter 1.

     @return Parameter value.
*/
extern UINT32 hwclock_get_param(HWCLOCK_PARAM_ID id, UINT32 p1);

/**
     Get time

     Get time of timeID

     @param[in] timeID      time index.

     @return get time.
*/
struct tm hwclock_get_time(UINT32 time_id);

/**
     Set time

     Set time of timeID

     @param[in] timeID      time index.
     @param[in] ctv         set time.
     @param[in] param       paramters.

     EX:
     HwClock_SetTime(TIME_ID_CURRENT, ctv, 0);
     HwClock_SetTime(TIME_ID_ALARM, ctv, (UINT32)MyAlaramCB);
     HwClock_SetTime(TIME_ID_HWRT, ctv, 0);
     HwClock_SetTime(TIME_ID_SWRT, ctv, 0);

     NOTE:
     The maximum value of ctv.tm_year is 1900(year) + 65535(days) when using
     internal HwClock. The maximum value of ctv.tm_year is 1900(year) + 131071(days)
     when using external HwClock.
*/
void hwclock_set_time(UINT32 timeID, struct tm ctv, UINT32 param);

/**
     Calculate maximum days number of given (year, month).

     Calculate maximum days number of given (year, month).

     @param[in] year      Year.
     @param[in] month     Month

     @return Days number.
*/
extern INT32 timeutil_calc_month_days(INT32 year, INT32 month);

/**
     Convert date-time value to days number.

     Convert date-time value to days number.

     @param[in] ctv     Date time value.

     @return Days number. [second precison]
*/
extern UINT32 timeutil_tm_to_days(struct tm ctv);

/**
     Convert days number to date-time value.

     Convert days number to date-time value.

     @param[in] days    Days number.

     @return Date time value. [day precison]
*/
extern struct tm timeutil_days_to_tm(UINT32 days);

/**
     Calculate sum of date-time value and another differece

     Calculate sum of date-time value and another differece

     @param[in] ctv     Date time value.
     @param[in] diff    Difference (date time value).

     @return Sum (date time value). [second precison]
*/
extern struct tm timeutil_tm_add(struct tm ctv, struct tm diff);

/**
    Check if leap year

    This function is used to check if the specified year is a leap year.

    @param[in] uiYear   The year you want to check (0~0xFFFFFFFF)
    @return
        - @b TRUE   : Is a leap year
        - @b FALSE  : Isn't a leap year
*/
extern BOOL timeutil_is_leap_year(UINT32 year);
#endif

#endif /* _HW_CLOCK_H */

