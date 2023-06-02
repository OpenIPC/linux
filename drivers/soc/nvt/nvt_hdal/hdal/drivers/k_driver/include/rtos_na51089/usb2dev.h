/**
    Header file for USB device

    This file is the header file for USB device module driver

    @file       usb.h
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#ifndef _USB2DEV_H
#define _USB2DEV_H

#include <kwrap/type.h>


#include "usb_desc.h"
#include "usb_define.h"

#ifndef DRV_CB
typedef void (*DRV_CB)(UINT32 event);
#endif

/**
    @addtogroup mIDrvUSB_Device
*/
//@{

/**
    USB physical max DMA length
*/
//@{
#define USB_MAX_DMA_LENGTH      0x7fffff         ///< Max DMA length: 8MB-1
//@}

/**
    USB controller mode
*/
typedef enum {
	USB_CTRL_MODE_DEVICE,                       ///< USB device mode
	USB_CTRL_MODE_HOST,                         ///< USB host mode

	ENUM_DUMMY4WORD(USB_CTRL_MODE)
} USB_CTRL_MODE;


/**
    Endpoint Block number used

    Describe endpoint receive/transmit data with manner of single/ping-pong/tripple buffer

    @note For blk_num of USB_ENP_CONFIG
*/
typedef enum {
	BLKNUM_NOT_USE,                             ///< block not used
	BLKNUM_SINGLE,                              ///< use single block
	BLKNUM_DOUBLE,                              ///< use double block
	BLKNUM_TRIPLE,                              ///< use triple block
	ENUM_DUMMY4WORD(USB_EP_BLKNUM)
} USB_EP_BLKNUM;

/**
    Endpoint direction

    @note For Direction of USB_ENP_CONFIG
*/
typedef enum {
	EP_DIR_NOT_USE,                             ///< Endpoint direction not set
	EP_DIR_IN,                                  ///< Endpoint direction is IN
	EP_DIR_OUT,                                 ///< Endpoint direction is OUT
	ENUM_DUMMY4WORD(USB_EP_DIR)
} USB_EP_DIR;

/**
    Endpoint transfer type

    @note For trnsfer_type of USB_ENP_CONFIG
*/
typedef enum {
	EP_TYPE_NOT_USE,                            ///< Endpoint transfer type not set
	EP_TYPE_ISOCHRONOUS,                        ///< Endpoint transfer type is isochronous
	EP_TYPE_BULK,                               ///< Endpoint transfer type is bulk
	EP_TYPE_INTERRUPT,                          ///< Endpoint transfer type is interrupt
	ENUM_DUMMY4WORD(USB_EP_TYPE)
} USB_EP_TYPE;

/**
    transaction number for high bandwidth endpoint

    @note For high_bandwidth of USB_ENP_CONFIG
*/
typedef enum {
	HBW_NOT,                                    ///< not high bandwidth
	HBW_1,                                      ///< one transaction
	HBW_2,                                      ///< two transaction
	HBW_3,                                      ///< three transaction
	ENUM_DUMMY4WORD(USB_EP_HBW)
} USB_EP_HBW;

/**
    USB controller state

    @note For usb_setControllerState(), usb2dev_get_controller_state()
*/
typedef enum {
	USB_CONTROLLER_STATE_NONE,                  ///< USB controller state is none
	USB_CONTROLLER_STATE_RESET,                 ///< USB controller state is reset
	USB_CONTROLLER_STATE_SUSPEND,               ///< USB controller state is suspend
	ENUM_DUMMY4WORD(USB_CONTROLLER_STATE)
} USB_CONTROLLER_STATE;

/**
    Endpoint configuration

    @note For ep_config_hs and ep_config_fs of USB_MNG
*/
typedef  struct Endpoint_Config {
	BOOL           enable;                      ///< enable this endpoint or not
												///< - @b TRUE: this endpoint descriptor is enabled
												///< - @b FALSE: this endpoint descriptor is disabled
	UINT32         blk_size;                     ///< block size : inform usb controller allocate 512/1024 bytes block for this endpoint
												///< - @b 512: max_pkt_size <= 512 bytes
												///< - @b 1024: max_pkt_size > 512 bytes
	USB_EP_BLKNUM  blk_num;                      ///< block number : single/double/triple
	USB_EP_DIR     direction;                   ///< IN or OUT drection
	USB_EP_TYPE    trnsfer_type;                 ///< bulk/interrupt/isochronous
	UINT32         max_pkt_size;                  ///< max packet size (unit: byte)
	USB_EP_HBW     high_bandwidth;               ///< transzction number for high bandwidth endpoint
} USB_ENP_CONFIG;


/**
    USB Management Infomation Table

    This is used in usb2dev_init_management() / usb2dev_set_management() to apply usb configurations before usb2dev_open().
*/
typedef struct {
	UINT32           magic_no;                      ///< No not modify. this value is assigned by usb2dev_init_management().
	USB_DEV_TYPE     usb_type;                      ///< USB_MSD, USB_SIDC, USB_PCC

	USB_DEVICE_DESC  *p_dev_desc;                   ///< USB Device Descriptor ptr
	USB_CONFIG_DESC  *p_config_desc;                  ///< USB Config Descriptor ptr
	USB_CONFIG_DESC  *p_config_others_desc;            ///< USB Config Descriptor ptr
	USB_CONFIG_DESC  *p_config_desc_hs;                ///< USB HighSpeed Descriptor
	USB_CONFIG_DESC  *p_config_desc_fs;                ///< USB FullSpeed Descriptor
	USB_CONFIG_DESC  *p_config_desc_fs_other;           ///< USB HighSpeed Descriptor
	USB_CONFIG_DESC  *p_config_desc_hs_other;           ///< USB FullSpeed Descriptor
	USB_ENP_CONFIG   ep_config_hs[15];              ///< set Highspeed endpoint configuration
	USB_ENP_CONFIG   ep_config_fs[15];              ///< set Fullspeed endpoint configuration
	USB_STRING_DESC  *p_string_desc[8];               ///< USB String Descriptor ptr
	USB_DEVICE_DESC  *p_dev_quali_desc;                ///< USB Qualifier ptr; High speed vs low speed.
	UINT16           num_of_configurations;            ///< configuration number
	UINT16           num_of_strings;                   ///< string number

	void (*fp_open_needed_fifo)(void);               ///< callback function to enable/disable FIFO
	void (*fp_event_callback)(UINT32 event);              ///< ISR callback function
	FP               fp_usb_suspend;               ///< callback function pointer for suspend

} USB_MNG;


/**
    control transfer

    @note For global variable usb2dev_control_data which pass control transfer information between usb driver and class library
*/
typedef struct {
	UINT8               *p_data;                  ///< data pointer to CX IN data
	USB_DEVICE_REQUEST  device_request;           ///< device request
	UINT16              w_length;                 ///< length
	UINT16              reserved;                ///< reserved
} CONTROL_XFER;

/**
    USB configuration identifier

    @note For usb2dev_set_config(), usb2dev_get_config()
*/
typedef enum {
	USB_CONFIG_ID_STANDARD_CHARGER,             ///< Select standard charger support
												///<   Context can be:
												///<   - @b TRUE: USB standard charger (Default value)
												///<   - @b FALSE: non-standard charger

	USB_CONFIG_ID_SQUELCH_LEVEL,                ///< Get squelch level.
	USB_CONFIG_ID_HS_SWING,                     ///< Get high speed TX Swing
	USB_CONFIG_ID_REF_CURRENT,                  ///< Get reference current

	USB_CONFIG_ID_USE_SHARE_FIFO,               ///< Allow the USB Endpoint use the Bi-Directional share FIFO or not.
												///<   Context can be:
												///<   - @b TRUE:  Allow to use share FIFO for IN/OUT endpoints.
												///<   - @b FALSE: Always use independent FIFO for all endpoints (Default value)
	USB_CONFIG_ID_FIFOMAP_4FIRST,				///< Configure internal 8 fifo to endpoint map is from fifo0 or fifo4 first.
												///<   Context can be:
												///<   - @b TRUE:  FIFO4 first. (default)
												///<   - @b FALSE: By USB_CONFIG_ID_FIFO_START_OFS settings value.
	USB_CONFIG_ID_FIFO_START_OFS,               ///< Select FIFO Mapping Start Offset. Default is 0(FIFO0). Valid setting range is 0-7.
	USB_CONFIG_ID_AUTO_FIFO_CLR,				///< Select Endpoint number for auto fifo clear. Please use USB_EP as input.
	USB_CONFIG_ID_SUSPEND_DRAMOFF,

	ENUM_DUMMY4WORD(USB_CONFIG_ID)
} USB_CONFIG_ID;

/**
    USB call back list

    @note For usb2dev_set_callback()
*/
typedef enum {
	USB_CALLBACK_CHARGING_EVENT,            ///< usb charging event call back (Designed for project layer)
											///< (SUGGESTED to be invoked in system init phase)
											///< The installed call back can receive following event:
											///< - @b USB_CHARGE_EVENT_2P5MA
											///< - @b USB_CHARGE_EVENT_100MA
											///< - @b USB_CHARGE_EVENT_500MA
											///< - @b USB_CHARGE_EVENT_5A
	USB_CALLBACK_CX_CLASS_REQUEST,          ///< usb control pipe class request call back (Designed for usb class)
											///< The installed call back doesn't need to receive parameter
	USB_CALLBACK_SET_INTERFACE,             ///< usb set interface call back (Designed for project/class layer)
											///< (SUGGESTED to be invoked in class init phase)
											///< The installed call back can receive USB_SETINTERFACE_EVENT
	USB_CALLBACK_CX_VENDOR_REQUEST,         ///< usb control pipe vendor request call back (Designed for usb class)
											///< The installed call back should process usb2dev_control_data by itself
											///< The installed call back will be invoked when vendor request is received
	USB_CALLBACK_STD_UNKNOWN_REQ,			///< usb std request with unknown request type event callback. prototype as USB_GENERIC_CB.


	ENUM_DUMMY4WORD(USB_CALLBACK)
} USB_CALLBACK;


/**
    USB Endpoint Configuration Select ID

    This is used in usb2dev_set_ep_config()/usb2dev_get_ep_config() to select which Endpoint Config is chosen to assign new configuration.
*/
typedef enum {
	USB_EPCFG_ID_AUTOHDR_EN,            ///< Set Auto-Header function Enable/Disable. TRUE is enable and FALSE is disable.
	USB_EPCFG_ID_AUTOHDR_LEN,           ///< Set Auto-Header Length. The configuration value must be 2 or 12. Others are not supported.

	USB_EPCFG_ID_AUTOHDR_START,         ///< If setting START to TRUE/ENABLE, the next DMA Trigger start would toggle FID bit in the header field.
										///< If setting START to FALSE/DISABLE, the next DMA Trigger start would keep current FID bit in the header field.
	USB_EPCFG_ID_AUTOHDR_STOP,          ///< If setting STOP to TRUE/ENABLE, the controller would set EOF to 1 in the header field at the next DMA ending SOF slice.
	USB_EPCFG_ID_AUTOHDR_RSTCOUNTER,    ///< Set TRUE to reset internal 32bits counter value to 0 for PTS[31:0] and SCR[31:0].

	/* Get Only */
	USB_EPCFG_ID_AUTOHDR_FID_VAL,       ///< Get current FID bit value in the auto header field.
	USB_EPCFG_ID_AUTOHDR_EOF_VAL,       ///< Get current EOF bit value in the auto header field.
	USB_EPCFG_ID_AUTOHDR_PTS_VAL,       ///< Get current PTS[31:0] value in the auto header field.
	USB_EPCFG_ID_AUTOHDR_SCRLSB_VAL,    ///< Get current SCR[31:0] value in the auto header field.
	USB_EPCFG_ID_AUTOHDR_SCRMSB_VAL,    ///< Get current SCR[47:32] value in the auto header field.


	/* Below Config IDs are reserved for internal usage only */
	USB_EPCFG_ID_AUTOHDR_HWFID_EN,      // Novatek internal usage only
	USB_EPCFG_ID_AUTOHDR_SWFID,         // Novatek internal usage only
	USB_EPCFG_ID_AUTOHDR_HWPTS_EN,      // Novatek internal usage only
	USB_EPCFG_ID_AUTOHDR_SWPTS,         // Novatek internal usage only

	ENUM_DUMMY4WORD(USB_EPCFG_ID)
} USB_EPCFG_ID;











/**
    usb2dev_control_data

    Control Transfer information

    - If SETUP token is received, usb driver may pass usb2dev_control_data to class library for vendor control transfer
    - If class library needs return CX_IN data, it should fill usb2dev_control_data then invoke usb2dev_reture_setup_data()
*/
extern CONTROL_XFER usb2dev_control_data;


//
// Normal USB Driver API
//

/**
    Initialize USB Management Infomation Table Data Structure

    Initialize USB Management Infomation Table Data Structure.
    User can allocate local parameter of USB_MNG and apply usb2dev_init_management to it.
    And then start configure the management table.

    @param[in] pMng USB Management Infomation Table pointer

    @return void
*/
extern void                 usb2dev_init_management(USB_MNG *p_management);

/**
    Apply USB Management Infomation Table

    Apply USB Management Infomation Table to USB Driver.

    @param[in] pMng USB Management Infomation Table pointer

    @return
     - @b E_OK:  Apply management info success.
     - @b E_PAR: Input parameter is NULL pointer.
     - @b E_CTX: No Init. Please apply usb2dev_init_management() to pMng first.
*/
extern ER                   usb2dev_set_management(USB_MNG *p_management);

/**
    Get Current USB Management Infomation Table

    Get Current  USB Management Infomation Table to USB Driver.

    @param[out] pMng USB Management Infomation Table pointer

    @return
     - @b E_OK:  Apply management info success.
     - @b E_PAR: Input parameter is NULL pointer.
     - @b E_CTX: No Init. Please apply usb2dev_init_management() to pMng first.
*/
extern ER                   usb2dev_get_management(USB_MNG *p_management);

/**
    Open USB driver

    Initialize USB module and enable interrupt, start running
    control & setup task for ISR etc...

    @return
        - @b E_OK: open success
        - @b E_OACV: USB driver is already opened
        - @b E_ID: Outside semaphore ID number range
        - @b E_NOEXS: Semaphore does not yet exist
        - @b E_SYS: USB has been unplug, so ignore this request...
*/
extern ER                   usb2dev_open(void);

/**
    Check usb charger

    Check if connect with a usb charger

    @param[in] debounce       debounce time to detect usb charger, unit: 10ms

    @return
        - @b USB_CHARGER_STS_NONE: PC or not charger detected
        - @b USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT: charging downstream port
        - @b USB_CHARGER_STS_CHARGER: charger detected
*/
extern USB_CHARGER_STS      usb2dev_check_charger(UINT32 debounce);

/**
    USB Power on initialization.

    @note This API SHOULD be invoked in system init phase. Else USB may be malfunctioned.

    @param[in] highspeed   set USB in high/full speed mode
                            - @b TRUE: high speed mode
                            - @b FALSE: full speed mode
    @return void
*/
extern void                 usb2dev_power_on_init(BOOL highspeed);

/**
    USB Change state

    Calling this function to get the current plug status.
    Application should remember the plug/unplug state to avoid calling
    open/close function multiple!

    @return
        - @b FALSE: Unplug State.
        - @b TRUE: Plug State.
*/
extern BOOL                 usb2dev_state_change(void);

/**
    Close USB device driver access.

    This function will stop USB DMA, terminate control and setup task,
    disable interrupt and USB clock etc...

    @return
        - @b E_OK: open success
        - @b E_OACV: USB driver is not opened
        - @b E_ID: Outside semaphore ID number range
        - @b E_NOEXS: Semaphore does not yet exist
        - @b E_QOVR: Semaphore's counter error, maximum counter < counter
*/
extern ER                   usb2dev_close(void);

/**
    Set VBUSI state

    Force VBUSI state for USB controller
    This API should be invoked when upper layer want to force USB connect without usb2dev_state_change()

    @param[in] plug    state of current VBUSI
                        - @b TRUE: VBUSI is HIGH
                        - @b FALSE: VBUSI is LOW

    @return void
*/
extern void                 usb2dev_set_vbusi(BOOL plug);

/**
    Set the callback function when wakeup from standby mode

    User can use this API to register call back when system is waked up from power down/standby mode.
    The installed call back will be invoked if system is waked up by USB (resume/reset).

    @param[in] fp_isr    User defined ISR for wakeup interrupt

    @return void
*/
extern void                 usb2dev_set_standby_wakeup(DRV_CB fp_isr);

/**
    get USB controller state.

    @return    USB controller state
        - @b USB_CONTROLLER_STATE_NONE: USB is not used
        - @b USB_CONTROLLER_STATE_RESET: USB is in reset state
        - @b USB_CONTROLLER_STATE_SUSPEND: USB is in suspend state
*/
extern USB_CONTROLLER_STATE usb2dev_get_controller_state(void);

/**
    Check set configuration result

    This API is provided to upper layer to check if USB is under set configuration state.

    @return
        - @b TRUE: under set configuration
        - @b FALSE: not under set configuration
*/
extern BOOL                 usb2dev_check_configured(void);

/**
    Set USB controller configuration

    @param[in] config_id         configuration identifier
    @param[in] config_context    configuration context for config_id

    @return
        - @b E_OK: set configuration success
        - @b E_NOSPT: config_id is not supported
*/
extern ER                   usb2dev_set_config(USB_CONFIG_ID config_id, UINT32 config_context);

/**
    Get USB controller configuration

    @param[in] config_id         configuration identifier

    @return configuration context for config_id
*/
extern UINT32               usb2dev_get_config(USB_CONFIG_ID config_id);

/**
    Set usb call back routine

    This function provides a facility for upper layer to install callback routine.

    @param[in] callback_id   callback routine identifier
    @param[in] p_callback    callback routine to be installed

    @return
        - @b E_OK: install callback success
        - @b E_NOSPT: callback_id is not valid
*/
extern ER                   usb2dev_set_callback(USB_CALLBACK callback_id, USB_GENERIC_CB p_callback);

/**
    Get the current setting of USB speed mode.

    @return
        - @b TRUE: device is in High speed
        - @b FALSE: device is in full speed
*/
extern BOOL                 usb2dev_is_highspeed_enabled(void);

//
// USB read /write functions
//

/**
    Return setup data to host.

    Class library can use this API to return CX IN data after receive a setup packet.

    @note Before invoke this API, usb2dev_control_data.p_data should point to return data\n
            and usb2dev_control_data.w_length should be filled with length of return data.

    @return void
*/
extern void                 usb2dev_reture_setup_data(void);

/**
    Read the buffer content from FIFO

    This function is blocking mode. It will return until DMA done or
    some error occured. Check the return ER and p_dma_length for actual
    transfered length.

    @param[in] epn          endpoint number (USB_EP0~USB_EP15)
    @param[in] p_buffer      buffer pointer
    @param[in,out] p_dma_length  input length to read, output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: read done (maybe aborted, check return p_dma_length)
*/
extern ER                   usb2dev_read_endpoint(USB_EP epn, UINT8 *p_buffer, UINT32 *p_dma_length);
extern ER 					usb2dev_read_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms);

/**
    Write the buffer content to FIFO

    This function is blocking mode. It will return until DMA done or
    some error occured. Check the return ER and p_dma_length for actual
    transfered length.

    @param[in] epn          endpoint number (USB_EP0~USB_EP15)
    @param[in] p_buffer      buffer pointer
    @param[in,out] p_dma_length  input length to write, output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: read done (maybe aborted, check return p_dma_length)
*/
extern ER                   usb2dev_write_endpoint(USB_EP epn, UINT8 *p_buffer, UINT32 *p_dma_length);
extern ER 					usb2dev_write_endpoint_timeout(USB_EP EPn, UINT8 *pBuffer, UINT32 *pDMALen, UINT32 timeout_ms);

/**
    Abort the endpoint transfer

    @param[in] epn  endpoint number (1~15)

    @return
        - @b E_OK: abort success
        - @b Else: abort fail
*/
extern ER                   usb2dev_abort_endpoint(USB_EP epn);

/**
    Read the buffer content from FIFO

    This function is non-blocking mode and will return immediately.
    Used with usb2dev_wait_ep_done() to wait for DMA complete.

    @param[in] epn      endpoint number (USB_EP0~USB_EP15)
    @param[in] p_buffer  buffer pointer
    @param[in] p_dma_length  DMA length to be read.
                        Valid length 0x1 to 0x40 for EP0.
                        Valid length 0x1 to 0x7fffff for EP1~EP15.

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: start to transfer
*/
extern ER                   usb2dev_set_ep_read(USB_EP epn, UINT8 *p_buffer, UINT32 *p_dma_length);

/**
    Write the buffer content to FIFO

    This function is non-blocking mode and will return immediately.
    Used with usb2dev_wait_ep_done() to wait for DMA complete.

    @param[in] epn      endpoint number (USB_EP0~USB_EP15)
    @param[in] p_buffer  buffer pointer
    @param[in] p_dma_length  DMA length to be written.
                        Valid length 0x1 to 0x40 for EP0.
                        Valid length 0x1 to 0x7fffff for EP1~EP15.

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_PAR: input DMA length or EP number incorrect
        - @b E_OBJ: FIFO is busy
        - @b E_SYS: USB is SUSPENDED
        - @b E_OK: start to transfer
*/
extern ER                   usb2dev_set_ep_write(USB_EP epn, UINT8 *p_buffer, UINT32 *p_dma_length);

/**
    Wait for endpoint DMA done

    This function is used with usb2dev_set_ep_read() or usb2dev_set_ep_write().
    It will Wait until DMA done and return actual transfered length

    @param[in] epn          endpoint number (USB_EP0~USB_EP8)
    @param[in,out] p_dma_length  input DMA length (1~0x7fffff), output actual transfered length

    @return
        - @b E_OACV: USB driver is not opened
        - @b E_OK: DMA done (maybe aborted, check return p_dma_length)
*/
extern ER                   usb2dev_wait_ep_done(USB_EP epn, UINT32 *p_dma_length);

//
// USB endpoint and FIFO setting functions
//

/**
    Set Configuration value to dedicate Endpoint

    Set Configuration value to dedicate Endpoint

    @param[in] epn          Endpoint number.
    @param[in] config_id        Configuration Select ID.
    @param[in] config_value   Configuration value.

    @return void
*/
extern void                 usb2dev_set_ep_config(USB_EP epn, USB_EPCFG_ID config_id, UINT32 config_value);

/**
    Get Configuration value from dedicate Endpoint

    Get Configuration value from dedicate Endpoint

    @param[in] epn      Endpoint number.
    @param[in] config_id    Configuration Select ID.

    @return Configuration value
*/
extern UINT32               usb2dev_get_ep_config(USB_EP epn, USB_EPCFG_ID config_id);

/**
    Mask(Disable) the endpoint interrupt

    @param[in] epn  endpoint number (USB_EP0~USB_EP8)

    @return void
*/
extern void                 usb2dev_mask_ep_interrupt(USB_EP epn);

/**
    Unmask(Enable) the endpoint interrupt

    @param[in] epn  endpoint number (USB_EP0~USB_EP8)

    @return void
*/
extern void                 usb2dev_unmask_ep_interrupt(USB_EP epn);

/**
    Set endpoint0 CX_DONE

    When class library completes transfers of CX IN/OUT, it should invoke this API to enter status stage.

    @return void
*/
extern void                 usb2dev_set_ep0_done(void);

/**
    Set endpoint stall

    When class library finds somethins wrong, it can use this API to make a function stall for this endpoint.

    @param[in] epn  endpoint number (USB_EP0~USB_EP15)

    @return void
*/
extern void                 usb2dev_set_ep_stall(USB_EP epn);

/**
    Clear Endpoint FIFO

    Clear endpoint related FIFO.

    @param[in] epn  endpoint number (USB_EP0~USB_EP15)
    @return void
*/
extern void                 usb2dev_clear_ep_fifo(USB_EP epn);

/**
    clear FIFO

    @return void
*/
extern void                 usb2dev_clear_fifo(void);

/**
    Set the endpoint to send a 0 byte data

    @param[in] epn      endpoint number (USB_EP1~USB_EP15)

    @return void
*/
extern void                 usb2dev_set_tx0byte(USB_EP epn);

/**
    Get endpoint byte count.

    Some application may not know the exactly transfering size at
    the beginning. Use this function to get the FIFO byte counts.
    This API should be used after OUT interrupt issued.

    @param[in] epn      endpoint number (USB_EP0~USB_EP15)

    @return endpoint byte count
*/
extern UINT32               usb2dev_get_ep_bytecount(USB_EP epn);

/**
    Check endpoint FIFO empty

    @param[in] epn      endpoint number (USB_EP1~USB_EP8)

    @return
        - @b TRUE: FIFO for this endpoint is empty
        - @b FALSE: FIFO for this endpoint is NOT empty
*/
extern BOOL                 usb2dev_check_ep_empty(USB_EP epn);

/**
    Check endpoint is Busy or not.

    @param[in] epn      endpoint number (USB_EP1~USB_EP15)

    @return
        - @b FALSE:      FIFO for this endpoint is empty and DMA is not working.
        - @b TRUE:     FIFO for this endpoint is busy.
*/
extern BOOL                 usb2dev_check_ep_busy(USB_EP epn);

/**
    Get SOF

    Return current SOF 11bits value

    @return SOF value
*/
extern UINT32               usb2dev_get_sof_number(void);

extern void usb2dev_init(void);
extern void usb2dev_uninit(void);

//@}
#endif // _USB_H
