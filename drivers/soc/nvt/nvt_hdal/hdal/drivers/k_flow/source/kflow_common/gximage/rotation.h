/**
    Rotation module driver

    @file       rotation.h
    @ingroup    mIDrvIPP_Rotation
    @note
\n              -rotation_open() to start this module.
\n              -rotation_close() to close this module.
\n
\n              This module will have to enable and wait the system interrupt.
\n
\n              -rotation_request. set parameters of images.
\n
\n
\n              Usage example:
\n
\n              (1)rotation_open(ROTATION_ID_1);
\n              (2)Fill request;
\n                 rotation_request(ROTATION_ID_1, &request);
\n                      :
\n              (3)rotation_close(ROTATION_ID_1);

    Copyright   Novatek Microelectronics Corp. 2014.  All rights reserved.
*/
#ifndef   _ROTATION_H
#define   _ROTATION_H

#if defined __UITRON || defined __ECOS
#include "Driver.h"
#else
//#include "sys/driver.h"
#endif

/**
    @addtogroup mIDrvIPP_Rotation
*/
//@{

/**
    Graphic channel ID

*/
typedef enum {
	ROTATION_ID_1,                          ///< Rotation Controller
	ROTATION_ID_2,                          ///< Rotation Controller 2

	ENUM_DUMMY4WORD(ROTATION_ID)
} ROTATION_ID;

/**
    Rotation configuration ID

    @note For rotation_setConfig()
*/
typedef enum {
	ROTATION_CONFIG_ID_FREQ,                ///< Rotation operating frequency (unit: MHz). Context can be:
	///< - @b 120: 120 MHz
	///< - @b 160: 160 MHz
	///< - @b 192: 192 MHz
	///< - @b 240: 240 MHz

	ENUM_DUMMY4WORD(ROTATION_CONFIG_ID)
} ROTATION_CONFIG_ID;
#define ROTATION_CONFIG_FREQUENCY           (ROTATION_CONFIG_ID_FREQ)

/**
    Geometric operation mode

    Select which Geometric operation mode for rotation engine operation.

*/
typedef enum {
	GOP_ROT_90,                    ///< Rotate 90 degree clockwise
	GOP_ROT_270,                   ///< Rotate 270 degree clockwise
	GOP_ROT_180,                   ///< Rotate 180 degree
	GOP_HRZ_FLIP,                  ///< Horizontal flip
	GOP_VTC_FLIP,                  ///< Vertical flip
	GOP_HRZ_FLIP_ROT_90,           ///< Horizontal flip and Rotate 90 degree clockwise
	GOP_HRZ_FLIP_ROT_270,          ///< Horizontal flip and Rotate 270 degree clockwise

	ENUM_DUMMY4WORD(GOP_MODE)
} GOP_MODE;

/**
    Rotation operation command

    Select which operation for rotation engine operation.

    @note Used for ROTATION_REQUEST.
*/
typedef enum {
	// GOP
	CMD_ROT_90,                             ///< Rotate 90 degree clockwise
	///< - (ROTATION_ID_1, ROTATION_ID_2)
	///< - Format: FORMAT_8BITS, FORMAT_16BITS, FORMAT_32BITS
	CMD_ROT_270,                            ///< Rotate 270 degree clockwise
	///< - (ROTATION_ID_1, ROTATION_ID_2)
	///< - Format: FORMAT_8BITS, FORMAT_16BITS, FORMAT_32BITS
	CMD_ROT_180,                            ///< Rotate 180 degree
	///< - (ROTATION_ID_1, ROTATION_ID_2)
	///< - Format: FORMAT_8BITS, FORMAT_16BITS, FORMAT_32BITS
	CMD_HRZ_FLIP,                           ///< Horizontal flip
	///< - (ROTATION_ID_1, ROTATION_ID_2)
	///< - Format: FORMAT_8BITS, FORMAT_16BITS, FORMAT_32BITS
	CMD_VTC_FLIP,                           ///< Vertical flip
	///< - (ROTATION_ID_1, ROTATION_ID_2)
	///< - Format: FORMAT_8BITS, FORMAT_16BITS, FORMAT_32BITS
	CMD_HRZ_FLIP_ROT_90,                    ///< Horizontal flip and Rotate 90 degree clockwise
	///< - (ROTATION_ID_1, ROTATION_ID_2)
	///< - Format: FORMAT_8BITS, FORMAT_16BITS, FORMAT_32BITS
	CMD_HRZ_FLIP_ROT_270,                   ///< Horizontal flip and Rotate 270 degree clockwise
	///< - (ROTATION_ID_1, ROTATION_ID_2)
	///< - Format: FORMAT_8BITS, FORMAT_16BITS, FORMAT_32BITS
	CMD_GOPMAX,                             ///< Max GOP count

	ENUM_DUMMY4WORD(ROTATION_CMD)
} ROTATION_CMD;

/**
    Rotation data format

    Select data format of rotation operation.

    @note For ROTATION_REQUEST
*/
typedef enum {
	FORMAT_4BITS,                      ///< Normal 4 bits data
	FORMAT_8BITS,                      ///< Normal 8 bits data
	FORMAT_16BITS,                     ///< Normal 16 bits data
	FORMAT_16BITS_UVPACK,              ///< 16 bits U/V pack data and apply to both plane
	FORMAT_16BITS_UVPACK_U,            ///< 16 bits U/V pack data and apply to U plane only
	FORMAT_16BITS_UVPACK_V,            ///< 16 bits U/V pack data and apply to V plane only
	FORMAT_16BITS_RGB565,              ///< 16 bits RGB565 pack data and apply to all plane
	FORMAT_32BITS,                     ///< Normal 32 bits data
	FORMAT_32BITS_ARGB8888_RGB,        ///< 32 bits ARGB8888 pack data and apply to RGB plane only
	FORMAT_32BITS_ARGB8888_A,          ///< 32 bits ARGB8888 pack data and apply to A plane only

	ENUM_DUMMY4WORD(ROTATION_FORMAT)
} ROTATION_FORMAT;

/**
    Rotation Image identifier

    Select Source and Destination Image.

    @note For ROTATION_IMG
*/
typedef enum {
	ROTATION_IMG_ID_SRC,                          ///< Source Image
	ROTATION_IMG_ID_DST,                          ///< Destination Image

	ENUM_DUMMY4WORD(ROTATION_IMG_ID)
} ROTATION_IMG_ID;


/**
    Rotation image structure

    Description of rotation image

    @note For ROTATION_REQUEST
*/
typedef struct ROTATION_IMG {
	ROTATION_IMG_ID img_id;       ///< Image identifier
	UINT32 address;               ///< Image DRAM address (unit: byte)
	UINT32 lineoffset;            ///< Image lineoffset (unit: byte) (Must be 4 byte alignment)
	UINT32 width;                 ///< Image width (unit: byte)
	UINT32 height;                ///< Image height (unit: line)

	struct ROTATION_IMG *p_next;         ///< Link to next image. (Fill NULL if this is last item)
} ROTATION_IMG, *PROTATION_IMG;

/**
    Rotation request structure

    Description of rotation request

    @note For rotation_request()
*/
typedef struct {
	DRV_VER_INFO ver_info;           ///< Driver version
	ROTATION_CMD command;           ///< Rotation command
	ROTATION_FORMAT format;             ///< format of operated images
	PROTATION_IMG p_image_descript;   ///< Descriptions of images for command
} ROTATION_REQUEST, *PROTATION_REQUEST;

// Functions exported from graphic driver
extern ER       rotation_open(ROTATION_ID id);
extern BOOL     rotation_is_opened(ROTATION_ID id);
extern ER       rotation_close(ROTATION_ID id);

extern ER       rotation_setconfig(ROTATION_ID rotationID, ROTATION_CONFIG_ID config_id, UINT32 config_context);
extern ER       rotation_getconfig(ROTATION_ID rotationID, ROTATION_CONFIG_ID config_id, UINT32 *p_config_context);

extern ER       rotation_request(ROTATION_ID id, PROTATION_REQUEST p_request);


//@}

#endif
