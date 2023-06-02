/**
    Header file for USB device

    This file is the header file for USB device

    @file       usb_define.h
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _USBDEFINE_H
#define _USBDEFINE_H

#include <kwrap/type.h>

/**
    @addtogroup mIDrvUSB_Device
*/
//@{


/**
    USB End Point number
*/
typedef enum {
	USB_EP0,                                    ///< USB End Point number 0
	USB_EP1,                                    ///< USB End Point number 1
	USB_EP2,                                    ///< USB End Point number 2
	USB_EP3,                                    ///< USB End Point number 3
	USB_EP4,                                    ///< USB End Point number 4
	USB_EP5,                                    ///< USB End Point number 5
	USB_EP6,                                    ///< USB End Point number 6
	USB_EP7,                                    ///< USB End Point number 7
	USB_EP8,                                    ///< USB End Point number 8
	USB_EP9,                                    ///< USB End Point number 9
	USB_EP10,                                   ///< USB End Point number 10
	USB_EP11,                                   ///< USB End Point number 11
	USB_EP12,                                   ///< USB End Point number 12
	USB_EP13,                                   ///< USB End Point number 13
	USB_EP14,                                   ///< USB End Point number 14
	USB_EP15,                                   ///< USB End Point number 15

	USB_EP_MAX,
	USB_EP_NOT_USE = 0x10,
	ENUM_DUMMY4WORD(USB_EP)
} USB_EP;

/**
    @name USB generic call back prototype

    @param[in] UINT32   event for this call back

    @return void
*/
//@{
typedef void (*USB_GENERIC_CB)(UINT32);
//@}

/**
    device request

    @note For device_request of CONTROL_XFER
*/
typedef struct {
	UINT8       bm_request_type;                  ///< request type
	UINT8       b_request;                       ///< request
	UINT16      w_value;                         ///< value of b_request defined
	UINT16      w_index;                         ///< value of b_request defined
	UINT16      w_length;                        ///< data length of data stage
} USB_DEVICE_REQUEST, *PUSB_DEVICE_REQUEST;

/**
    @name   Basic parameters
*/
//@{
#define USB_EP_NUM                      8       ///< max number of endpoint
#define USB_NUM_CONFIG                  1       ///< number of configration
#define USB_NUM_INTERFACE               1       ///< number of interface
#define USB_NUM_STRING_DESC             4       ///< number of string descriptor
#define USB_CONFIG_DESC_TOTAL_SIZE      20      ///< configuration descriptor total size
#define EP0_PACKET_SIZE                 64      ///< EP0 : control packet size
#define USB_WAITBUFEMPTY_LOOPCNT        10000   ///< loop count for waiting buffer empty
//@}


/**
    @name   Block number

    (OBSOLETE)
*/
//@{
#define SINGLE_BLK                      1       ///< Single block
#define DOUBLE_BLK                      2       ///< Double block
#define TRIPLE_BLK                      3       ///< triple block
//@}

/**
    @name   Endpoint/FIFO direction
*/
//@{
#define DIRECTION_IN                    0       ///< direction IN
#define DIRECTION_OUT                   1       ///< direction OUT
//@}

/**
    @name   USB Request Type
*/
//@{
#define USB_DEV_REQ_TYPE_STANDARD       0x00    ///< standard request
#define USB_DEV_REQ_TYPE_CLASS          0x01    ///< class specific request
#define USB_DEV_REQ_TYPE_VENDER         0x02    ///< vendor specific request
#define USB_DEV_REQ_TYPE_RESERVE        0x03    ///< reserved
#define USB_DEV_REQ_TYPE_UNSUPPORTED    0xff    ///< unsupported
//@}


/**
    @name   USB Standard Device Request
*/
//@{
#define USB_GET_STATUS                  0       ///< GetStatus request
#define USB_CLEAR_FEATURE               1       ///< ClearFeature request
#define USB_SET_FEATURE                 3       ///< SetFeature request
#define USB_SET_ADDRESS                 5       ///< SetAddress request
#define USB_GET_DESCRIPTOR              6       ///< GetDescriptor request
#define USB_SET_DESCRIPTOR              7       ///< SetDescriptor request
#define USB_GET_CONFIGURATION           8       ///< GetConfiguratoin request
#define USB_SET_CONFIGURATION           9       ///< SetConfiguratoin request
#define USB_GET_INTERFACE               10      ///< GetInterface request
#define USB_SET_INTERFACE               11      ///< SetInterface request
#define USB_SYNCH_FRAME                 12      ///< SynchFrame request
//@}


/**
    @name   device request
*/
//@{
#define USB_DEVICE_REQUEST_SIZE         8       ///< device request size
#define USB_DEVICE_TO_HOST              0x80    ///< device to host transfer
#define USB_HOST_TO_DEVICE              0x00    ///< host to device transfer
#define USB_DEVICE                      0x00    ///< request to device
#define USB_INTERFACE                   0x01    ///< request to interface
#define USB_ENDPOINT                    0x02    ///< request to endpoint
#define USB_CLASS                       0x20    ///< class request
#define USB_VENDOR                      0x40    ///< vendor request
#define USB_STANDARD                    0x00    ///< standard request
//@}


/**
    @name   descriptor size

    Descriptor size for class library to build descriptor array
*/
//@{
#define USB_DEV_LENGTH                  0x12    ///< device descriptor size
#define USB_CFG_LENGTH                  0x09    ///< config descriptor size
#define USB_IF_LENGTH                   0x09    ///< interface descriptor size
#define USB_EP_LENGTH                   0x07    ///< endpoint descriptor size
#define USB_SSEP_COMPANISON_LENGTH      0x06    ///< SuperSpeed endpoint companion descriptor size
#define USB_AUDIO_AC_IF_LENGTH          0x09    ///< audio commander class specific interface descriptor size
#define USB_AUDIO_IT_IF_LENGTH          0x0C    ///< audio input terminal class specific interface descriptor size
#define USB_AUDIO_OT_IF_LENGTH          0x09    ///< audio output terminal class specific interface descriptor size
#define USB_AUDIO_AS_IF_LENGTH          0x07    ///< audio class specific AS interface descriptor size
#define USB_AUDIO_FT_IF_LENGTH          0x0B    ///< audio class specific format type interface descriptor size
#define USB_ISO_EP_LENGTH               0x09    ///< iso endpoint descriptor size
#define USB_AUDIO_CSEP_ISOC_LENGTH      0x07    ///< audio class specific endpoint descriptor size
#define USB_DEV_QUALI_LENGTH            0x0A    ///< device qualifier length */


// Backward compatible
#define USB_BULK_EP_LENGTH              0x07    ///< bulk endpoint descriptor size
//@}

/**
    @name   endpoint address
*/
//@{
#define USB_EP_IN_ADDRESS               0x80    ///< IN endpoint address
#define USB_EP_OUT_ADDRESS              0x00    ///< OUT endpoint address
#define USB_EP_EP1I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x01) ///< IN EP1 address
#define USB_EP_EP1O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x01) ///< OUT EP1 address
#define USB_EP_EP2I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x02) ///< IN EP2 address
#define USB_EP_EP2O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x02) ///< OUT EP2 address
#define USB_EP_EP3I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x03) ///< IN EP3 address
#define USB_EP_EP3O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x03) ///< OUT EP3 address
#define USB_EP_EP4I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x04) ///< IN EP4 address
#define USB_EP_EP4O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x04) ///< OUT EP4 address
#define USB_EP_EP5I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x05) ///< IN EP5 address
#define USB_EP_EP5O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x05) ///< OUT EP5 address
#define USB_EP_EP6I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x06) ///< IN EP6 address
#define USB_EP_EP6O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x06) ///< OUT EP6 address
#define USB_EP_EP7I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x07) ///< IN EP7 address
#define USB_EP_EP7O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x07) ///< OUT EP7 address
#define USB_EP_EP8I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x08) ///< IN EP8 address
#define USB_EP_EP8O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x08) ///< OUT EP8 address
#define USB_EP_EP9I_ADDRESS             (USB_EP_IN_ADDRESS  | 0x09) ///< IN EP9 address
#define USB_EP_EP9O_ADDRESS             (USB_EP_OUT_ADDRESS | 0x09) ///< OUT EP9 address
#define USB_EP_EP10I_ADDRESS            (USB_EP_IN_ADDRESS  | 0x0A) ///< IN EP10 address
#define USB_EP_EP10O_ADDRESS            (USB_EP_OUT_ADDRESS | 0x0A) ///< OUT EP10 address
#define USB_EP_EP11I_ADDRESS            (USB_EP_IN_ADDRESS  | 0x0B) ///< IN EP11 address
#define USB_EP_EP11O_ADDRESS            (USB_EP_OUT_ADDRESS | 0x0B) ///< OUT EP11 address
#define USB_EP_EP12I_ADDRESS            (USB_EP_IN_ADDRESS  | 0x0C) ///< IN EP12 address
#define USB_EP_EP12O_ADDRESS            (USB_EP_OUT_ADDRESS | 0x0C) ///< OUT EP12 address
#define USB_EP_EP13I_ADDRESS            (USB_EP_IN_ADDRESS  | 0x0D) ///< IN EP13 address
#define USB_EP_EP13O_ADDRESS            (USB_EP_OUT_ADDRESS | 0x0D) ///< OUT EP13 address
#define USB_EP_EP14I_ADDRESS            (USB_EP_IN_ADDRESS  | 0x0E) ///< IN EP14 address
#define USB_EP_EP14O_ADDRESS            (USB_EP_OUT_ADDRESS | 0x0E) ///< OUT EP14 address
#define USB_EP_EP15I_ADDRESS            (USB_EP_IN_ADDRESS  | 0x0F) ///< IN EP15 address
#define USB_EP_EP15O_ADDRESS            (USB_EP_OUT_ADDRESS | 0x0F) ///< OUT EP15 address
#define USB_EP_ADDRESS_MASK             (0x0F)                      ///< EP mask to get endpoint number (exclude IN/OUT)
//@}

/**
    @name   endpoint attribute
*/
//@{
typedef enum {
	USB_EP_ATR_CONTROL,                 ///< transfer mode : control
	USB_EP_ATR_ISO,                     ///< transfer mode : isochronous
	USB_EP_ATR_BULK,                    ///< transfer mode : bulk
	USB_EP_ATR_INTERRUPT,               ///< transfer mode : interrupt
	USB_EP_ATR_ASYNC,                   ///< synchornization type : asynchornization
	USB_EP_ATR_ADAPTIVE = 0x08,         ///< synchornization type : adaptive
	USB_EP_ATR_SYNC     = 0x0C,         ///< synchornization type : synchornization

	ENUM_DUMMY4WORD(USB_EP_ATR)
} USB_EP_ATR;
//@}

/**
    @name   b_interval value
*/
//@{
#define USB_EP_CONTROL_INTERVAL                 0x00    ///< control interval
#define USB_EP_BULK_INTERVAL                    0x00    ///< bulk interval
#define USB_EP_INT_INTERVAL                     0x01    ///< interrupt interval
#define USB_EP_ISO_INTERVAL                     0x01    ///< isochronous interval
#define USB_EP_INT_CFGA_IF0ALT0_INTERVAL        0x01    ///< interrupt CFGA_IF0ALT0 interval
#define USB_EP_INT_CFGA_IF0ALT1_INTERVAL        0x02    ///< interrupt CFGA_IF0ALT1 interval
#define USB_EP_INT_CFGA_IF0ALT2_INTERVAL        0x03    ///< interrupt CFGA_IF0ALT2 interval
#define USB_EP_INT_CFGA_IF0ALT3_INTERVAL        0x04    ///< interrupt CFGA_IF0ALT3 interval
#define USB_EP_INT_CFGB_IF1ALT0_INTERVAL        0x05    ///< interrupt CFGA_IF1ALT0 interval
//@}

/**
    @name   USB Feature Select
*/
//@{
#define USB_DEVICE_REMOTE_WAKEUP                1       ///< remote wake up
#define USB_ENDPOINT_STALL                      0       ///< endpoint stall
//@}

/**
    @name   USB descriptor type
*/
//@{
#define USB_DEVICE_DESCRIPTOR_TYPE                  1       ///< device descriptor
#define USB_CONFIGURATION_DESCRIPTOR_TYPE           2       ///< configuraton descriptor
#define USB_STRING_DESCRIPTOR_TYPE                  3       ///< string descriptor
#define USB_INTERFACE_DESCRIPTOR_TYPE               4       ///< interface descriptor
#define USB_ENDPOINT_DESCRIPTOR_TYPE                5       ///< endpoint descriptor
#define USB_DEVICE_QUALIFIER_DESCRIPTOR_TYPE        6       ///< Device_Qualifier descriptor, SS without this.
#define USB_OTHER_SPEED_CONFIGURATION               7       ///< Other_Speed_Configuration, SS without this.
#define USB_INTERFACE_POWER_DESCRIPTOR_TYPE         8       ///< Interface Power descriptor
#define USB_OTG_DESCRIPTOR_TYPE                     9       ///< OTG descriptor
#define USB_DEBUG_DESCRIPTOR_TYPE                   10      ///< Debug descriptor
#define USB_INTERFACE_ASSOCIATION_DESCRIPTOR_TYPE   11      ///< Interface Association descriptor
#define USB_BOS_DESCRIPTOR_TYPE                     15      ///< Binary Device Object Store (BOS) descriptor
#define USB_SS_DEVCAP_DESCRIPTOR_TYPE               16      ///< SuperSpeed Device Capability descriptor
#define USB_SS_EP_COMPANION_TYPE                    48      ///< SuperSpeed USB Endpoint Companion descriptor
//@}

/**
    @name   config descriptor definitions
*/
//@{
#define USB_DEVDESC_ATB_BUS_POWER               0x80    ///< bus power
#define USB_DEVDESC_ATB_SELF_POWER              0x40    ///< self power
#define USB_DEVDESC_ATB_RMT_WAKEUP              0x20    ///< remote wake up
//@}

/**
    @name   USB request-bm_request_type : RECIPIENT type
*/
//@{
#define USB_RECIPIENT                           0x1F    ///< recipient mask
#define USB_RECIPIENT_DEVICE                    0x00    ///< recipient device
#define USB_RECIPIENT_INTERFACE                 0x01    ///< recipient interface
#define USB_RECIPIENT_ENDPOINT                  0x02    ///< recipient endpoint
//@}

/**
    @name   USB defined Feature select
*/
//@{
#define USB_FEATURE_ENDPOINT_STALL              0x0000  ///< feature select stall
#define USB_FEATURE_REMOTE_WAKEUP               0x0001  ///< feature select remote wakeup
#define USB_FEATURE_POWER_D0                    0x0002  ///< feature select POWER_D0
#define USB_FEATURE_POWER_D1                    0x0003  ///< feature select POWER_D1
#define USB_FEATURE_POWER_D2                    0x0004  ///< feature select POWER_D2
#define USB_FEATURE_POWER_D3                    0x0005  ///< feature select POWER_D3
//@}

/**
    @name   USB request and masks
*/
//@{
#define USB_REQUEST_TYPE_MASK                   0x60    ///< request type mask
#define USB_STANDARD_REQUEST                    0x00    ///< standard request
#define USB_CLASS_REQUEST                       0x20    ///< class request
#define USB_VENDOR_REQUEST                      0x40    ///< vendor request
#define USB_VENDOR_D2H_MASK                     0x80    ///< vendor d2h mask
#define USB_REQUEST_MASK                        0x0F    ///< request mask
#define DEVICE_ADDRESS_MASK                     0x7F    ///< device address mask
#define USB_ENDPOINT_DIRECTION_MASK             0x80    ///< endpoint direction mask
//@}

/**
    @name   Endpoint number
*/
//@{
#define EP0                             0x00    ///< Endpoint 0
#define EP1                             0x01    ///< Endpoint 1
#define EP2                             0x02    ///< Endpoint 2
#define EP3                             0x03    ///< Endpoint 3
#define EP4                             0x04    ///< Endpoint 4
#define EP5                             0x05    ///< Endpoint 5
#define EP6                             0x06    ///< Endpoint 6
#define EP7                             0x07    ///< Endpoint 7
#define EP8                             0x08    ///< Endpoint 8
//@}

/**
    @name   FIFO number

    (OBSOLETE)
*/
//@{
#define FIFO0                           0x0     ///< FIFO 0
#define FIFO1                           0x1     ///< FIFO 1
#define FIFO2                           0x2     ///< FIFO 2
#define FIFO3                           0x3     ///< FIFO 3
//@}


/**
    USB type

    @note For usb_type of USB_MNG
*/
typedef enum {
	USB_NONE,                                   ///< USB type not set
	USB_MSD,                                    ///< USB type is MSDC
	USB_SIDC,                                   ///< USB type is SIDC
	USB_PCC,                                    ///< USB type is PCC
	USB_TEST,                                   ///< USB type is test class

	ENUM_DUMMY4WORD(USB_DEV_TYPE)
} USB_DEV_TYPE;

/**
    USB event definition

    @note For fp_event_callback() of USB_MNG
*/
typedef enum {
	USB_EVENT_NONE,
	USB_EVENT_RESET,                            ///< (000)USB Reset
	USB_EVENT_SETADDRESSCMD,                    ///< (001)USB set address from host
	USB_EVENT_SET_CONFIG,                       ///< (002)USB set configuration
	USB_EVENT_CLRFEATURE,                       ///< (003)USB Clear Feature
	USB_EVENT_SUSPEND,                          ///< (004)USB suspend event
	USB_EVENT_RESUME,                           ///< (005)USB resume event
	USB_EVENT_CONNECT,                          ///< (006)USB connect to host
	USB_EVENT_DISCONNECT,                       ///< (007)USB disconnect from host

	USB_EVENT_EP0_TX = 256,                     ///< (256)USB  (IN)EP0  TX
	USB_EVENT_EP1_TX,                           ///< (257)USB  (IN)EP1  TX
	USB_EVENT_EP2_TX,                           ///< (258)USB  (IN)EP2  TX
	USB_EVENT_EP3_TX,                           ///< (259)USB  (IN)EP3  TX
	USB_EVENT_EP4_TX,                           ///< (260)USB  (IN)EP4  TX
	USB_EVENT_EP5_TX,                           ///< (261)USB  (IN)EP5  TX
	USB_EVENT_EP6_TX,                           ///< (262)USB  (IN)EP6  TX
	USB_EVENT_EP7_TX,                           ///< (263)USB  (IN)EP7  TX
	USB_EVENT_EP8_TX,                           ///< (264)USB  (IN)EP8  TX
	USB_EVENT_EP9_TX,                           ///< (265)USB  (IN)EP9  TX
	USB_EVENT_EP10_TX,                          ///< (266)USB  (IN)EP10 TX
	USB_EVENT_EP11_TX,                          ///< (267)USB  (IN)EP11 TX
	USB_EVENT_EP12_TX,                          ///< (268)USB  (IN)EP12 TX
	USB_EVENT_EP13_TX,                          ///< (269)USB  (IN)EP13 TX
	USB_EVENT_EP14_TX,                          ///< (270)USB  (IN)EP14 TX
	USB_EVENT_EP15_TX,                          ///< (271)USB  (IN)EP15 TX

	USB_EVENT_EP0_RX,                           ///< (272)USB (OUT)EP0  RX
	USB_EVENT_EP1_RX,                           ///< (273)USB (OUT)EP1  RX
	USB_EVENT_EP2_RX,                           ///< (274)USB (OUT)EP2  RX
	USB_EVENT_EP3_RX,                           ///< (275)USB (OUT)EP3  RX
	USB_EVENT_EP4_RX,                           ///< (276)USB (OUT)EP4  RX
	USB_EVENT_EP5_RX,                           ///< (277)USB (OUT)EP5  RX
	USB_EVENT_EP6_RX,                           ///< (278)USB (OUT)EP6  RX
	USB_EVENT_EP7_RX,                           ///< (279)USB (OUT)EP7  RX
	USB_EVENT_EP8_RX,                           ///< (280)USB (OUT)EP8  RX
	USB_EVENT_EP9_RX,                           ///< (281)USB (OUT)EP9  RX
	USB_EVENT_EP10_RX,                          ///< (282)USB (OUT)EP10 RX
	USB_EVENT_EP11_RX,                          ///< (283)USB (OUT)EP11 RX
	USB_EVENT_EP12_RX,                          ///< (284)USB (OUT)EP12 RX
	USB_EVENT_EP13_RX,                          ///< (285)USB (OUT)EP13 RX
	USB_EVENT_EP14_RX,                          ///< (286)USB (OUT)EP14 RX
	USB_EVENT_EP15_RX,                          ///< (287)USB (OUT)EP15 RX

	USB_EVENT_POWERDOWN_SUSPEND,                ///< USB power down suspend event

	USB_EVENT_EP1TX0BYTE = 512,                 ///< (512)USB EP1  TX 0BYTE
	USB_EVENT_EP2TX0BYTE,                       ///< (513)USB EP2  TX 0BYTE
	USB_EVENT_EP3TX0BYTE,                       ///< (514)USB EP3  TX 0BYTE
	USB_EVENT_EP4TX0BYTE,                       ///< (515)USB EP4  TX 0BYTE
	USB_EVENT_EP5TX0BYTE,                       ///< (516)USB EP5  TX 0BYTE
	USB_EVENT_EP6TX0BYTE,                       ///< (517)USB EP6  TX 0BYTE
	USB_EVENT_EP7TX0BYTE,                       ///< (518)USB EP7  TX 0BYTE
	USB_EVENT_EP8TX0BYTE,                       ///< (519)USB EP8  TX 0BYTE
	USB_EVENT_EP9TX0BYTE,                       ///< (520)USB EP9  TX 0BYTE
	USB_EVENT_EP10TX0BYTE,                      ///< (521)USB EP10 TX 0BYTE
	USB_EVENT_EP11TX0BYTE,                      ///< (522)USB EP11 TX 0BYTE
	USB_EVENT_EP12TX0BYTE,                      ///< (523)USB EP12 TX 0BYTE
	USB_EVENT_EP13TX0BYTE,                      ///< (524)USB EP13 TX 0BYTE
	USB_EVENT_EP14TX0BYTE,                      ///< (525)USB EP14 TX 0BYTE
	USB_EVENT_EP15TX0BYTE,                      ///< (526)USB EP15 TX 0BYTE

	USB_EVENT_EP1RX0BYTE,                       ///< (527)USB EP1  TX 0BYTE
	USB_EVENT_EP2RX0BYTE,                       ///< (528)USB EP2  TX 0BYTE
	USB_EVENT_EP3RX0BYTE,                       ///< (529)USB EP3  TX 0BYTE
	USB_EVENT_EP4RX0BYTE,                       ///< (530)USB EP4  TX 0BYTE
	USB_EVENT_EP5RX0BYTE,                       ///< (531)USB EP5  TX 0BYTE
	USB_EVENT_EP6RX0BYTE,                       ///< (532)USB EP6  TX 0BYTE
	USB_EVENT_EP7RX0BYTE,                       ///< (533)USB EP7  TX 0BYTE
	USB_EVENT_EP8RX0BYTE,                       ///< (534)USB EP8  TX 0BYTE
	USB_EVENT_EP9RX0BYTE,                       ///< (535)USB EP9  TX 0BYTE
	USB_EVENT_EP10RX0BYTE,                      ///< (536)USB EP10 TX 0BYTE
	USB_EVENT_EP11RX0BYTE,                      ///< (537)USB EP11 TX 0BYTE
	USB_EVENT_EP12RX0BYTE,                      ///< (538)USB EP12 TX 0BYTE
	USB_EVENT_EP13RX0BYTE,                      ///< (539)USB EP13 TX 0BYTE
	USB_EVENT_EP14RX0BYTE,                      ///< (540)USB EP14 TX 0BYTE
	USB_EVENT_EP15RX0BYTE,                      ///< (541)USB EP15 TX 0BYTE

	ENUM_DUMMY4WORD(USB_EVENT)
} USB_EVENT;



/**
    USB3 events definition

    These event definitions are used for the usb3dev_setCallBack(U3DEV_CALLBACK_ID_EVENT_NOTIFY) Callback function.
*/
typedef enum {
	USB3_EVENT_EP0OUT = 0x10000,                 ///< (0x10000)USB3 (OUT)EP0  RX
	USB3_EVENT_EP0IN,                            ///< (0x10001)USB3  (IN)EP0  TX
	USB3_EVENT_EP1OUT,                           ///< (0x10002)USB3 (OUT)EP0  RX
	USB3_EVENT_EP1IN,                            ///< (0x10003)USB3  (IN)EP0  TX
	USB3_EVENT_EP2OUT,                           ///< (0x10004)USB3 (OUT)EP0  RX
	USB3_EVENT_EP2IN,                            ///< (0x10005)USB3  (IN)EP0  TX
	USB3_EVENT_EP3OUT,                           ///< (0x10006)USB3 (OUT)EP0  RX
	USB3_EVENT_EP3IN,                            ///< (0x10007)USB3  (IN)EP0  TX
	USB3_EVENT_EP4OUT,                           ///< (0x10008)USB3 (OUT)EP0  RX
	USB3_EVENT_EP4IN,                            ///< (0x10009)USB3  (IN)EP0  TX
	USB3_EVENT_EP5OUT,                           ///< (0x1000A)USB3 (OUT)EP0  RX
	USB3_EVENT_EP5IN,                            ///< (0x1000B)USB3  (IN)EP0  TX
	USB3_EVENT_EP6OUT,                           ///< (0x1000C)USB3 (OUT)EP0  RX
	USB3_EVENT_EP6IN,                            ///< (0x1000D)USB3  (IN)EP0  TX
	USB3_EVENT_EP7OUT,                           ///< (0x1000E)USB3 (OUT)EP0  RX
	USB3_EVENT_EP7IN,                            ///< (0x1000F)USB3  (IN)EP0  TX
	USB3_EVENT_EP8OUT,                           ///< (0x10010)USB3 (OUT)EP0  RX
	USB3_EVENT_EP8IN,                            ///< (0x10011)USB3  (IN)EP0  TX

	USB3_EVENT_RESET = 0x11000,                  ///< (0x11000)USB3 Reset
	USB3_EVENT_SUSPEND,                          ///< (0x11001)USB3 suspend event
	USB3_EVENT_RESUME,                           ///< (0x11002)USB3 resume event
	USB3_EVENT_CONNECT,                          ///< (0x11003)USB3 connect to host
	USB3_EVENT_CONNECT_DONE,                     ///< (0x11004)USB3 connect to host
	USB3_EVENT_DISCONNECT,                       ///< (0x11005)USB3 disconnect from host
	USB3_EVENT_CLRFEATURE,                       ///< (0x11006)USB3 Clear Feature

	ENUM_DUMMY4WORD(USB3_EVENT)
} USB3_EVENT;


/**
    USB set interface event

    @note For USB_CALLBACK_SET_INTERFACE
*/
typedef enum {
	USB_SETINTERFACE_EVENT_INT00_ALT00,                     ///< interface 0/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT00_ALT01,                     ///< interface 0/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT00_ALT02,                     ///< interface 0/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT00_ALT03,                     ///< interface 0/alternate 3 selected

	USB_SETINTERFACE_EVENT_INT01_ALT00 = 0x10000,           ///< interface 1/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT01_ALT01,                     ///< interface 1/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT01_ALT02,                     ///< interface 1/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT01_ALT03,                     ///< interface 1/alternate 3 selected

	USB_SETINTERFACE_EVENT_INT02_ALT00 = 0x20000,           ///< interface 2/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT02_ALT01,                     ///< interface 2/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT02_ALT02,                     ///< interface 2/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT02_ALT03,                     ///< interface 2/alternate 3 selected

	USB_SETINTERFACE_EVENT_INT03_ALT00 = 0x30000,           ///< interface 3/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT03_ALT01,                     ///< interface 3/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT03_ALT02,                     ///< interface 3/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT03_ALT03,                     ///< interface 3/alternate 3 selected

	USB_SETINTERFACE_EVENT_INT04_ALT00 = 0x40000,           ///< interface 4/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT04_ALT01,                     ///< interface 4/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT04_ALT02,                     ///< interface 4/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT04_ALT03,                     ///< interface 4/alternate 3 selected

	USB_SETINTERFACE_EVENT_INT05_ALT00 = 0x50000,           ///< interface 5/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT05_ALT01,                     ///< interface 5/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT05_ALT02,                     ///< interface 5/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT05_ALT03,                     ///< interface 5/alternate 3 selected

	USB_SETINTERFACE_EVENT_INT06_ALT00 = 0x60000,           ///< interface 6/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT06_ALT01,                     ///< interface 6/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT06_ALT02,                     ///< interface 6/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT06_ALT03,                     ///< interface 6/alternate 3 selected

	USB_SETINTERFACE_EVENT_INT07_ALT00 = 0x70000,           ///< interface 7/alternate 0 selected
	USB_SETINTERFACE_EVENT_INT07_ALT01,                     ///< interface 7/alternate 1 selected
	USB_SETINTERFACE_EVENT_INT07_ALT02,                     ///< interface 7/alternate 2 selected
	USB_SETINTERFACE_EVENT_INT07_ALT03,                     ///< interface 7/alternate 3 selected

	ENUM_DUMMY4WORD(USB_SETINTERFACE_EVENT)
} USB_SETINTERFACE_EVENT;

/**
    USB chargering event

    @note For USB_CALLBACK_CHARGING_EVENT
*/
typedef enum {
	USB_CHARGE_EVENT_SUSPEND,                               ///< suspend
	USB_CHARGE_EVENT_DISCONNECT = USB_CHARGE_EVENT_SUSPEND, ///< disconnect (disable D+ pull up)
	USB_CHARGE_EVENT_2P5MA = USB_CHARGE_EVENT_SUSPEND,      ///< currently can draw up to 2.5mA
	USB_CHARGE_EVENT_CONNECT,                               ///< connect, but not configured (enable D+ pull up)
	USB_CHARGE_EVENT_100MA = USB_CHARGE_EVENT_CONNECT,      ///< currently can draw up to 100mA
	USB_CHARGE_EVENT_CONFIGURED,                            ///< connect and configured by host
	USB_CHARGE_EVENT_500MA = USB_CHARGE_EVENT_CONFIGURED,   ///< currently can draw up to 500mA
	USB_CHARGE_EVENT_CHARGER,                               ///< charger connected
	USB_CHARGE_EVENT_CHARGING_PC = USB_CHARGE_EVENT_CHARGER,///< connect to charging downstream port PC
	USB_CHARGE_EVENT_1P5A = USB_CHARGE_EVENT_CHARGING_PC,   ///< currently can draw up to 1.5A


	ENUM_DUMMY4WORD(USB_CHARGE_EVENT)
} USB_CHARGE_EVENT;

/**
    USB charger detect state

    @note For usb2dev_check_charger()
*/
typedef enum {
	USB_CHARGER_STS_NONE,                               ///< No charger detected
	USB_CHARGER_STS_STANDARD_PC = USB_CHARGER_STS_NONE, ///< Standard PC
	USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT,           ///< Charging downstream port
	USB_CHARGER_STS_CHARGING_PC = USB_CHARGER_STS_CHARGING_DOWNSTREAM_PORT, ///< Charger capable PC

	USB_CHARGER_STS_CHARGER,                            ///< Charger detected

	USB_CHARGER_STS_DISCONNECT = 0x8000,                ///< USB cable is removed when charger detection is applied

	ENUM_DUMMY4WORD(USB_CHARGER_STS)
} USB_CHARGER_STS;



// backward compatible event definition
#define USB_EVENT_IN0           USB_EVENT_EP0_TX
#define USB_EVENT_OUT0          USB_EVENT_EP0_RX
#define USB_EVENT_BULKIN1       USB_EVENT_EP1_TX
#define USB_EVENT_BULKOUT2      USB_EVENT_EP2_RX
#define USB_EVENT_INT3          USB_EVENT_EP3_TX

//@}

#endif /* _USBDEFINE_H */

