/*
 * ms_os_desc.h- Sigmastar
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

#ifndef __MS_OS_DESC_H__
#define __MS_OS_DESC_H__

#define USBVideoMSOS20_bmRequestType 0xC0
#define USBVideoMSOS20_wValue 0x00
#define USBVideoMSOS20_wIndex 0x07
#define USBVideoMSOS20_wLength 0x2C8

char USBVideoMSOS20_PlatformCapabilityUUID[16] = { 0xDF, 0x60, 0xDD, 0xD8,
						   0x89, 0x45, 0xC7, 0x4C,
						   0x9C, 0xD2, 0x65, 0x9D,
						   0x9E, 0x64, 0x8A, 0x9F };

char USBVideoMSOS20_CapabilityData[8] = {
	0x00, 0x00, 0x00, 0x0A, // dwWindowsVersion for Windows 10 and later
	0xC8, 0x02, // wLength 0x2C8(712)
	0x01, // bMS_VendorCode - any value. e.g. 0x01
	0x00 // bAltEnumCmd 0
};

char USBVideoMSOS20_DescriptorSet[USBVideoMSOS20_wLength] = {
	/* Microsoft OS 2.0 Descriptor Set Header */
	0x0A, 0x00, // wLength of MSOS20_SET_HEADER_DESCRIPTOR
	0x00, 0x00, // wDescriptorType == MSOS20_SET_HEADER_DESCRIPTOR
	0x00, 0x00, 0x00, 0x0A, // dwWindowsVersion – 0x10000000 for Windows 10
	0xC8, 0x02, // wTotalLength - Total length 0x2C8 (712)

	/* Microsoft OS 2.0 Configuration Subset Header */
	0x08, 0x00, // wLength of MSOS20_SUBSET_HEADER_CONFIGURATION
	0x01, 0x00, // wDescriptorType == MSOS20_SUBSET_HEADER_CONFIGURATION
	0x00, // bConfigurationValue set to the first configuration
	0x00, // bReserved set to 0.
	0xBE, 0x02, // wTotalLength - Total length 0x2BE (702)

	/****************Color Camera Function******************/

	/* Microsoft OS 2.0 Function Subset Header */
	0x08, 0x00, // wLength of MSOS20_SUBSET_HEADER_FUNCTION
	0x02, 0x00, // wDescriptorType == MSOS20_SUBSET_HEADER_FUNCTION
	0x00, // bFirstInterface field of the first IAD
	0x00, // bReserved set to 0.
	0x6E, 0x01, // wSubsetLength - Length 0x16E (366)

	/****************Register the Color Camera in a sensor group******************/

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x80, 0x00, // wLength 0x80 (128) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x01, 0x00, // wPropertyDataType - REG_SZ
	0x28, 0x00, // wPropertyNameLength – 0x28 (40) bytes
	'U', 0x00, 'V', 0x00, // Property Name - "UVC-FSSensorGroupID"
	'C', 0x00, '-', 0x00, 'F', 0x00, 'S', 0x00, 'S', 0x00, 'e', 0x00, 'n',
	0x00, 's', 0x00, 'o', 0x00, 'r', 0x00, 'G', 0x00, 'r', 0x00, 'o', 0x00,
	'u', 0x00, 'p', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00, 0x4E,
	0x00, // wPropertyDataLength – 0x4E (78) bytes
	// FSSensorGroupID GUID in string format:
	// "{20C94C5C-F402-4F1F-B324-0C1CF0257870}"
	'{', 0x00, '2', 0x00, // This is just an example GUID.
	'0', 0x00, 'C', 0x00, // You need to generate and use your
	'9', 0x00, '4', 0x00, // own GUID for the sensor group ID
	'C', 0x00, '5', 0x00, 'C', 0x00, '-', 0x00, 'F', 0x00, '4', 0x00, '0',
	0x00, '2', 0x00, '-', 0x00, '4', 0x00, 'F', 0x00, '1', 0x00, 'F', 0x00,
	'-', 0x00, 'B', 0x00, '3', 0x00, '2', 0x00, '4', 0x00, '-', 0x00, '0',
	0x00, 'C', 0x00, '1', 0x00, 'C', 0x00, 'F', 0x00, '0', 0x00, '2', 0x00,
	'5', 0x00, '7', 0x00, '8', 0x00, '7', 0x00, '0', 0x00, '}', 0x00, 0x00,
	0x00,

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x56, 0x00, // wLength 0x56 (86) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x01, 0x00, // wPropertyDataType - REG_SZ
	0x2C, 0x00, // wPropertyNameLength – 0x2C (44) bytes
	'U', 0x00, 'V', 0x00, // Property Name - "UVC-FSSensorGroupName"
	'C', 0x00, '-', 0x00, 'F', 0x00, 'S', 0x00, 'S', 0x00, 'e', 0x00, 'n',
	0x00, 's', 0x00, 'o', 0x00, 'r', 0x00, 'G', 0x00, 'r', 0x00, 'o', 0x00,
	'u', 0x00, 'p', 0x00, 'N', 0x00, 'a', 0x00, 'm', 0x00, 'e', 0x00, 0x00,
	0x00, 0x20, 0x00, // wPropertyDataLength – 0x20 (32) bytes
	// FSSensorGroupName "YourCameraGroup"
	'Y', 0x00, 'o', 0x00, 'u', 0x00, 'r', 0x00, 'C', 0x00, 'a', 0x00, 'm',
	0x00, 'e', 0x00, 'r', 0x00, 'a', 0x00, 'G', 0x00, 'r', 0x00, 'o', 0x00,
	'u', 0x00, 'p', 0x00, 0x00, 0x00,

	/****************Enable Still Image Capture for Color Camera************/

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x54, 0x00, // wLength 0x54 (84) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x04, 0x00, // wPropertyDataType - REG_DWORD
	0x46, 0x00, // wPropertyNameLength – 0x46 (70) bytes
	'U', 0x00, 'V',
	0x00, // Property Name - "UVC-EnableDependentStillPinCapture"
	'C', 0x00, '-', 0x00, 'E', 0x00, 'n', 0x00, 'a', 0x00, 'b', 0x00, 'l',
	0x00, 'e', 0x00, 'D', 0x00, 'e', 0x00, 'p', 0x00, 'e', 0x00, 'n', 0x00,
	'd', 0x00, 'e', 0x00, 'n', 0x00, 't', 0x00, 'S', 0x00, 't', 0x00, 'i',
	0x00, 'l', 0x00, 'l', 0x00, 'P', 0x00, 'i', 0x00, 'n', 0x00, 'C', 0x00,
	'a', 0x00, 'p', 0x00, 't', 0x00, 'u', 0x00, 'r', 0x00, 'e', 0x00, 0x00,
	0x00, 0x04, 0x00, // wPropertyDataLength – 4 bytes
	0x01, 0x00, 0x00,
	0x00, // Enable still pin capture using Method 2 or Method 3

	/****************Enable Platform DMFT for ROI-capable USB Camera************/

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x3C, 0x00, // wLength 0x3C (60) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x04, 0x00, // wPropertyDataType - REG_DWORD
	0x2E, 0x00, // wPropertyNameLength – 0x2E (46) bytes
	'U', 0x00, 'V', 0x00, // Property Name - "UVC-EnablePlatformDmft"
	'C', 0x00, '-', 0x00, 'E', 0x00, 'n', 0x00, 'a', 0x00, 'b', 0x00, 'l',
	0x00, 'e', 0x00, 'P', 0x00, 'l', 0x00, 'a', 0x00, 't', 0x00, 'f', 0x00,
	'o', 0x00, 'r', 0x00, 'm', 0x00, 'D', 0x00, 'm', 0x00, 'f', 0x00, 't',
	0x00, 0x00, 0x00, 0x04, 0x00, // wPropertyDataLength – 4 bytes
	0x01, 0x00, 0x00, 0x00, // Enable Platform DMFT

	/****************IR Camera Function*********************************************/

	/* Microsoft OS 2.0 Function Subset Header */
	0x08, 0x00, // wLength of MSOS20_SUBSET_HEADER_FUNCTION
	0x02, 0x00, // wDescriptorType == MSOS20_SUBSET_HEADER_FUNCTION
	0x02, //0x01,					// bFirstInterface set of the second function
	0x00, // bReserved set to 0.
	0x48, 0x01, // wSubsetLength - Length 0x148 (328)

	/********Register the IR Camera to the same sensor group as the Color Camera*****/

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x80, 0x00, // wLength 0x80 (128) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x01, 0x00, // wPropertyDataType - REG_SZ
	0x28, 0x00, // wPropertyNameLength – 0x28 (40) bytes
	'U', 0x00, 'V', 0x00, // Property Name - "UVC-FSSensorGroupID"
	'C', 0x00, '-', 0x00, 'F', 0x00, 'S', 0x00, 'S', 0x00, 'e', 0x00, 'n',
	0x00, 's', 0x00, 'o', 0x00, 'r', 0x00, 'G', 0x00, 'r', 0x00, 'o', 0x00,
	'u', 0x00, 'p', 0x00, 'I', 0x00, 'D', 0x00, 0x00, 0x00, 0x4E,
	0x00, // wPropertyDataLength – 78 bytes
	// FSSensorGroupID GUID in string format:
	// "{20C94C5C-F402-4F1F-B324-0C1CF0257870}"
	'{', 0x00, '2', 0x00, '0', 0x00, 'C', 0x00, '9', 0x00, '4', 0x00, 'C',
	0x00, '5', 0x00, 'C', 0x00, '-', 0x00, 'F', 0x00, '4', 0x00, '0', 0x00,
	'2', 0x00, '-', 0x00, '4', 0x00, 'F', 0x00, '1', 0x00, 'F', 0x00, '-',
	0x00, 'B', 0x00, '3', 0x00, '2', 0x00, '4', 0x00, '-', 0x00, '0', 0x00,
	'C', 0x00, '1', 0x00, 'C', 0x00, 'F', 0x00, '0', 0x00, '2', 0x00, '5',
	0x00, '7', 0x00, '8', 0x00, '7', 0x00, '0', 0x00, '}', 0x00, 0x00, 0x00,

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x56, 0x00, // wLength 0x56 (86) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x01, 0x00, // wPropertyDataType - REG_SZ
	0x2C, 0x00, // wPropertyNameLength – 0x2C (44) bytes
	'U', 0x00, 'V', 0x00, // Property Name - "UVC-FSSensorGroupName"
	'C', 0x00, '-', 0x00, 'F', 0x00, 'S', 0x00, 'S', 0x00, 'e', 0x00, 'n',
	0x00, 's', 0x00, 'o', 0x00, 'r', 0x00, 'G', 0x00, 'r', 0x00, 'o', 0x00,
	'u', 0x00, 'p', 0x00, 'N', 0x00, 'a', 0x00, 'm', 0x00, 'e', 0x00, 0x00,
	0x00, 0x20, 0x00, // wPropertyDataLength – 32 bytes
	// FSSensorGroupName "YourCameraGroup"
	'Y', 0x00, 'o', 0x00, 'u', 0x00, 'r', 0x00, 'C', 0x00, 'a', 0x00, 'm',
	0x00, 'e', 0x00, 'r', 0x00, 'a', 0x00, 'G', 0x00, 'r', 0x00, 'o', 0x00,
	'u', 0x00, 'p', 0x00, 0x00, 0x00,

	/****************Make IR camera visible to applications*********************/

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x30, 0x00, // wLength 0x30 (48) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x04, 0x00, // wPropertyDataType - REG_DWORD
	0x22, 0x00, // wPropertyNameLength – 0x22 (34) bytes
	'S', 0x00, 'e', 0x00, 'n', 0x00, 's', 0x00, 'o', 0x00, 'r', 0x00, 'C',
	0x00, 'a', 0x00, 'm', 0x00, 'e', 0x00, 'r', 0x00, 'a', 0x00, 'M', 0x00,
	'o', 0x00, 'd', 0x00, 'e', 0x00, 0x00, 0x00, 0x04,
	0x00, // wPropertyDataLength – 4 bytes
	0x01, 0x00, 0x00,
	0x00, // This exposes the camera to OS as an IR only camera
	// i.e. KSCATEGORY_SENSOR_CAMERA

	/* Microsoft OS 2.0 Registry Value Feature Descriptor */
	0x3A, 0x00, // wLength 0x3A (58) in bytes of this descriptor
	0x04, 0x00, // wDescriptorType – MSOS20_FEATURE_REG_PROPERTY
	0x04, 0x00, // wPropertyDataType - REG_DWORD
	0x2C, 0x00, // wPropertyNameLength – 0x2C (44) bytes
	'S', 0x00, 'k', 0x00, 'i', 0x00, 'p', 0x00, 'C', 0x00, 'a', 0x00, 'm',
	0x00, 'e', 0x00, 'r', 0x00, 'a', 0x00, 'E', 0x00, 'n', 0x00, 'u', 0x00,
	'm', 0x00, 'e', 0x00, 'r', 0x00, 'a', 0x00, 't', 0x00, 'i', 0x00, 'o',
	0x00, 'n', 0x00, 0x00, 0x00, 0x04,
	0x00, // wPropertyDataLength – 4 bytes
	0x01, 0x00, 0x00,
	0x00 // This exposes the camera to applications looking for IR only cameras
};

#endif /* __MS_OS_DESC_H__ */
