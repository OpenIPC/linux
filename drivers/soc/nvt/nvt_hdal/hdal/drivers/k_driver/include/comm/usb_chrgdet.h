#ifndef __USB_CHRGDET_H_
#define __USB_CHRGDET_H_

#include "kwrap/type.h"

/**

*/
typedef enum {
	USB_CHRGDET_PORT_USB30,		///< [get]
	USB_CHRGDET_PORT_USB20,		///< [get]

	ENUM_DUMMY4WORD(USB_CHRGDET_PORT)
} USB_CHRGDET_PORT;



/**


*/
typedef enum {
	USB_CHRGDET_STS_NONE,													///< No charger detected
	USB_CHRGDET_STS_STANDARD_PC = USB_CHRGDET_STS_NONE,						///< Standard PC

	USB_CHRGDET_STS_CHARGING_DOWNSTREAM_PORT,								///< Charging downstream port
	USB_CHRGDET_STS_CHARGING_PC = USB_CHRGDET_STS_CHARGING_DOWNSTREAM_PORT,	///< Charger capable PC

	USB_CHRGDET_STS_CHARGER,												///< Charger detected

	USB_CHRGDET_STS_DISCONNECT,												///< USB cable is removed when charger detection is applied

	ENUM_DUMMY4WORD(USB_CHRGDET_STS)
} USB_CHRGDET_STS;




/**
    usb charger detect parameter select id

    This is used in usb_chrgdet_get to get the usb device mode charger detect result
*/
typedef enum {
	USB_CHRGDET_TYPE_STD,	///< [set] std or non-std. TRUE: Standard Charger. FALSE: Non-Standard Charger.
							///<       Default is Non-Standard Charger.

	USB_CHRGDET_PLUG,		///< [get] Get the usb cable is plug or not @usb-device-mode
	USB_CHRGDET_STATUS,		///< [get] usb charger detect return result would be USB_CHRGDET_STS

	ENUM_DUMMY4WORD(USB_CHRGDET_PARAM_ID)
} USB_CHRGDET_PARAM_ID;



/*!
 * @fn INT32 usb_chrgdet_get(UINT32 handler, USB_CHRGDET_PARAM_ID id, VOID *param)
 * @brief set parameters to hardware engine
 * @param id		the handler of hardware
 * @param param_id	the id of parameters
 * @param p_param	shall the (UINT32 *) or the address the get structure.
 * @return return 0 on success, -1 on error
 */
INT32 __usb_chrgdet_set(USB_CHRGDET_PORT usb_port, USB_CHRGDET_PARAM_ID param_id, VOID *p_param);


/*!
 * @fn INT32 usb_chrgdet_get(UINT32 handler, USB_CHRGDET_PARAM_ID id, VOID *param)
 * @brief set parameters to hardware engine
 * @param id		the handler of hardware
 * @param param_id	the id of parameters
 * @param p_param	shall the (UINT32 *) or the address the get structure.
 * @return return 0 on success, -1 on error
 */
INT32 __usb_chrgdet_get(USB_CHRGDET_PORT usb_port, USB_CHRGDET_PARAM_ID param_id, VOID *p_param);
#define usb_chrgdet_get(id, param_id, p_param) __usb_chrgdet_get((UINT32)(id), (USB_CHRGDET_PARAM_ID)(param_id), (VOID *)(p_param))

INT32 __usb_chrgdet_set(USB_CHRGDET_PORT usb_port, USB_CHRGDET_PARAM_ID param_id, VOID *p_param);
#define usb_chrgdet_set(id, param_id, p_param) __usb_chrgdet_set((UINT32)(id), (USB_CHRGDET_PARAM_ID)(param_id), (VOID *)(p_param))


#endif
