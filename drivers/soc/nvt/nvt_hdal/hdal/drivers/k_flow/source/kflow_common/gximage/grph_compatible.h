#ifndef __GRPH_COMPATIBLE_H_
#define __GRPH_COMPATIBLE_H_

#include "kwrap/type.h"
#include "kdrv_gfx2d/kdrv_grph.h"

/**
    Graphic channel ID

*/
typedef enum {
        GRPH_ID_1,                          ///< Graphic Controller
        GRPH_ID_2,                          ///< Graphic Controller 2

        ENUM_DUMMY4WORD(GRPH_ID)
} GRPH_ID;

/**
    Graphic configuration ID

    @note For graph_setConfig()
*/
typedef enum {
        GRPH_CONFIG_ID_FREQ,                ///< Graphic operating frequency (unit: MHz). Context can be:
        ///< - @b 240: 240 MHz
        ///< - @b 380: 380 MHz (PLL6, need confirm when realchip)
        ///< - @b 420: 420 MHz (PLL13, need confirm when realchip)
        ///< - @b 480: 480 MHz

        ENUM_DUMMY4WORD(GRPH_CONFIG_ID)
} GRPH_CONFIG_ID;

/**
    Accumulation control

    (OBSOLETE)

    Accumulation skip control

    @note used for GRPH_PROPERTY_ID_ACC_SKIPCTRL.
*/
typedef enum {
	GRPH_ACC_SKIP_NONE,                 ///< Accumulation none skip.
	GRPH_ACC_SKIP_ODD,                  ///< Accumulation odd skip.
	GRPH_ACC_SKIP_EVEN,                 ///< Accumulation even skip.

	ENUM_DUMMY4WORD(GRPH_ACC_SKIP_CTL)
} GRPH_ACC_SKIP_CTL __attribute__((deprecated));

/**
    Graphic request structure

    Description of graphic request

    @note For graph_request()
*/
typedef struct {
        DRV_VER_INFO ver_info;          ///< Driver version
        GRPH_CMD command;               ///< Graphic command
        GRPH_FORMAT format;             ///< format of operated images
        PGRPH_IMG p_images;             ///< Descriptions of images for command
        PGRPH_PROPERTY p_property;      ///< Descriptions of properties
        PGRPH_CKEYFILTER p_ckeyfilter;  ///< ColorKey filter control info. Valid when command is GRPH_AOP_COLOR_LE or GRPH_AOP_COLOR_MR and property is GRPH_COLOR_FLT_PROPTY
} GRPH_REQUEST, *PGRPH_REQUEST;


// Functions exported from graphic driver
extern ER       graph_open(GRPH_ID id);
extern BOOL     graph_is_opened(GRPH_ID id);
extern ER       graph_close(GRPH_ID id);

extern ER       graph_set_config(GRPH_ID id, GRPH_CONFIG_ID config_id, UINT32 config_context);
extern ER       graph_get_config(GRPH_ID id, GRPH_CONFIG_ID config_id, UINT32 *p_config_context);

extern ER       graph_request(GRPH_ID id, PGRPH_REQUEST p_request);


#endif
