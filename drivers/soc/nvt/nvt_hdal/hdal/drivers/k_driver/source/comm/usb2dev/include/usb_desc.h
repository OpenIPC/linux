/**
    Header file for USB device descriptor

    This file is the header file for USB device descriptor

    @file       usb_desc.h
    @ingroup    mIDrvUSB_Device
    @note       Nothing

    Copyright   Novatek Microelectronics Corp. 2009.  All rights reserved.
*/

#ifndef _USBDESC_H
#define _USBDESC_H

/**
    @addtogroup mIDrvUSB_Device
*/
//@{


/**
    USB Device Descriptor Data Structure
*/
typedef  struct {
	UINT8       b_length;                ///< size of Device Descriptor
	UINT8       b_descriptor_type;        ///< Device Dscriptor type
	UINT16      bcd_usb;                 ///< number of USB specifications
	UINT8       b_device_class;           ///< class code
	UINT8       b_device_subclass;        ///< sub class code
	UINT8       b_device_protocol;        ///< protocol code
	UINT8       b_max_packet_size0;        ///< max packt size of endpoint0
	UINT16      id_vendor;               ///< Vendor id
	UINT16      id_product;              ///< Protocol id
	UINT16      bcd_device;              ///< Device nmber
	UINT8       i_manufacturer;          ///< index of string Desc(maker)
	UINT8       i_product;               ///< index of string Desc(products)
	UINT8       i_serial_number;          ///< index of string Desc(serial number)
	UINT8       num_of_configurations;     ///< number for configration
} USB_DEVICE_DESC, *PUSB_DEVICE_DESC;

/**
    USB Configuration Descriptor Data Structure
*/
typedef  struct {
	UINT8       b_length;                ///< size of Configuration Descriptor
	UINT8       b_descriptor_type;        ///< Configuration Descriptor type
	UINT16      w_total_length;           ///< all length of data
	UINT8       b_num_interfaces;         ///< number of interface
	UINT8       b_configuration_value;    ///< value of argument
	UINT8       i_configuration;         ///< index of string Descriptor
	UINT8       bm_attributes;           ///< characteristic of composition
	UINT8       max_power;               ///< max power consumption
} USB_CONFIG_DESC, *PUSB_CONFIG_DESC;

/**
    USB Interface Descriptor Data Structure
*/
typedef struct {
	UINT8       b_length;                ///< size of Configuration Descriptor
	UINT8       b_descriptor_type;        ///< Configuration Descriptor type
	UINT8       b_interface_number;       ///< Number of this interface
	UINT8       b_alternate_setting;      ///< Value used to select this alternate setting for the interface.
	UINT8       b_num_endpoints;          ///< Number of endpoints used by this interface.
	UINT8       b_interface_class;        ///< Class Code
	UINT8       b_interface_subclass;     ///< SubClass Code
	UINT8       b_interface_protocol;     ///< Protocol Code
	UINT8       i_interface;             ///< index of string descriptor
} USB_INTF_DESC, *PUSB_INTF_DESC;

/**
    USB Endpoint Descriptor Data Structure
*/
typedef struct {
	UINT8       b_length;                ///< size of Configuration Descriptor
	UINT8       b_descriptor_type;        ///< Configuration Descriptor type
	UINT8       b_endpoint_address;       ///< The Address of this endpoint
	UINT8       bm_attributes;           ///< Endpoint Attributes bitmap
	UINT16      w_max_packet_size;         ///< Maximum Packet Size
	UINT8       b_interval;              ///< Minimum transaction Interval
} USB_ENDPOINT_DESC, *PUSB_ENDPOINT_DESC;

/**

*/
typedef struct {
	UINT8       b_length;                ///< size of SS Endpoint Companion Descriptor
	UINT8       b_descriptor_type;        ///< SS Endpoint Companion Descriptor type
	UINT8       b_max_burst;              ///< Max Burst Number. Valid Value from 0-15.
	UINT8       bm_attrubutes;           ///< For BULK, [4:0] is MaxStreams. [7:5] is reserved to 0.
	UINT16      w_bytes_per_interval;      ///<
} USB_EPCOMPANION_DESC, *PUSB_EPCOMPANION_DESC;



/**
    String Descriptor

    @note For p_string_desc of USB_MNG
*/
typedef  struct {
	UINT8       b_length;                ///< size of String Descriptor
	UINT8       b_descriptor_type;        ///< String Descriptor type
	UINT8       b_string[254];           ///< UNICODE stirng
} USB_STRING_DESC, *PUSB_STRING_DESC;


//@}

#endif  //_USBDESC_H

