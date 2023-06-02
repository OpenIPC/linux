#ifndef _VOS_UTIL_H_
#define _VOS_UTIL_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                                                                                 */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

#define VOS_UTIL_DELAY_INFINITELY     (-1)

//! Init/Exit flag
void  rtos_util_init(void *param);
void  rtos_util_exit(void);

//! Common api
int vos_util_msec_to_tick(int msec); //convert msec to tick(1/HZ), msec < 0 will get -1 returned
void vos_util_delay_ms(int ms); //VOS_UTIL_DELAY_INFINITELY to wait infinitely (Only supported by FreeRTOS)
void vos_util_delay_us(int us);
void vos_util_delay_us_polling(int us);

#ifdef __cplusplus
}
#endif

#endif /* _VOS_UTIL_H_ */

