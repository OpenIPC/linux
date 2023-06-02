#ifndef _VOS_PERF_H_
#define _VOS_PERF_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

typedef UINT32 VOS_TICK; //tick unit: usec

void vos_perf_init(void *param);
void vos_perf_exit(void);

/**
    Get the current tick by updating the pointer value

    @param[in] p_tick  The tick pointer to be updated

	@return none
*/
void vos_perf_mark(VOS_TICK *p_tick); //tick unit: usec

/**
    Calculate the duration between to VOS_TICK

    @note
    -# The max valid duration is about 71.58 min. Measurement more than this value will get a wrong result.

    @param[in] t_begin  The counter tick at the beginning
    @param[in] t_end    The counter tick at the end

    @return Return the duration from t_begin to t_end
*/
VOS_TICK vos_perf_duration(VOS_TICK t_begin, VOS_TICK t_end); //tick unit: usec

/**
    Mark a tag in the perf list buffer

    A tag can contain a function name, a line number, and a custom value.

    @param[in] p_name       the mark name. Or use __func__ macro.
    @param[in] line_no      the line number. Or use __LINE__ macro.
    @param[in] cus_val      the custom defined value. E.g. a counter variable.

    Example:
    @code
    {
		void func1(void)
		{
			vos_perf_list_mark(__func__, __LINE__, 0);
			//do something
			vos_perf_list_mark(__func__, __LINE__, 0);
		}

		void func2(void)
		{
			vos_perf_list_mark(__func__, __LINE__, 0);
			//do something
			vos_perf_list_mark(__func__, __LINE__, 0);
		}

		void main(void)
		{
	        vos_perf_list_reset(); //reset perf list

			func1();
			func2();

			vos_perf_list_dump(); //dump perf list
		}
    }
    @endcode

	Dump result:
	===== vos_perf_list_dump_by_idx =====
	[00] func1() ln 53 ts 12163639 df 0 cus 0
	[01] func2() ln 60 ts 12163653 df 14 cus 0
	[02] func2() ln 62 ts 12186673 df 23020 cus 0
	[03] func1() ln 55 ts 12187887 df 1214 cus 0

	===== vos_perf_list_dump_by_name =====
	Name      func1      func2
	[00]   12163639          -
	[01]          -   12163653
	[02]          -   12186673
	[03]   12187887          -
	-----------------------------------------------
	Diff      24248      23020

	Format meanings for vos_perf_list_dump_by_idx:
	ln: line number
	ts: timestamp tick (usec)
	df: difference to the previous ts
*/
void vos_perf_list_mark(const CHAR *p_name, UINT32 line_no, UINT32 cus_val);

/**
    Clean all data in the perf list buffer
*/
void vos_perf_list_reset(void);

/**
    Dump all data in the perf list buffer
*/
void vos_perf_list_dump(void);

#ifdef __cplusplus
}
#endif

#endif /* _VOS_PERF_H_ */

