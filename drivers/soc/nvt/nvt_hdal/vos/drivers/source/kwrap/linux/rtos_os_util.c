/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/jiffies.h>

#include <kwrap/util.h>

#define __MODULE__    rtos_util
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_UTIL_INITED_TAG       MAKEFOURCC('R', 'U', 'T', 'L') ///< a key value

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_util_debug_level = NVT_DBG_WRN;

module_param_named(rtos_util_debug_level, rtos_util_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rtos_util_debug_level, "Debug message level");

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void rtos_util_init(void *param)
{
	return;
}

void rtos_util_exit(void)
{
}

int vos_util_msec_to_tick(int msec)
{
	if (-1 == msec) {
		//Assume -1 msec to be used for timeout, return -1 without error messages
		return -1;
	} else if (msec < 0) {
		DBG_ERR("invalid msec %d\r\n", msec);
		return -1;
	}

	return msecs_to_jiffies(msec);
}

void vos_util_delay_ms(int ms)
{
	if (ms < 0) {
		DBG_ERR("Invalid delay %d ms\r\n", ms);
		return;
	}

	msleep(ms);
}

void vos_util_delay_us(int us)
{
	if (us < 0) {
		DBG_ERR("Invalid delay %d us\r\n", us);
		return;
	}

	//Linux driver do not have usleep, use usleep_range instead
	//https://github.com/torvalds/linux/blob/master/Documentation/timers/timers-howto.txt
	usleep_range(us, us+1);
}

void vos_util_delay_us_polling(int us)
{
	if (us < 0) {
		DBG_ERR("Invalid delay %d us\r\n", us);
		return;
	}

	udelay(us);
}

EXPORT_SYMBOL(vos_util_msec_to_tick);
EXPORT_SYMBOL(vos_util_delay_ms);
EXPORT_SYMBOL(vos_util_delay_us);
EXPORT_SYMBOL(vos_util_delay_us_polling);