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
        ///< - @b 320: 320 MHz
        ///< - @b 260: 360 MHz (PLL13, need confirm when realchip)
        ///< - @b 480: 480 MHz
        GRPH_CONFIG_ID_MODE,		///< (Not used) compatible to kdrv layer

	GRPH_CONFIG_ID_ABORT,		///< Abort graphic operation

        ENUM_DUMMY4WORD(GRPH_CONFIG_ID)
} GRPH_CONFIG_ID;

typedef enum {
        DRV_VER_96650,
        DRV_VER_96660,
        DRV_VER_96680,
        DRV_VER_510,
        DRV_VER_520,

        ENUM_DUMMY4WORD(DRV_VER_INFO)
} DRV_VER_INFO;

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
typedef struct _GRPH_REQUEST {
        DRV_VER_INFO ver_info;          ///< Driver version
        GRPH_CMD command;               ///< Graphic command
        GRPH_FORMAT format;             ///< format of operated images
        PGRPH_IMG p_images;             ///< Descriptions of images for command
        PGRPH_PROPERTY p_property;      ///< Descriptions of properties
        PGRPH_CKEYFILTER p_ckeyfilter;  ///< ColorKey filter control info. Valid when command is GRPH_AOP_COLOR_LE or GRPH_AOP_COLOR_MR and property is GRPH_COLOR_FLT_PROPTY
        BOOL		 is_skip_cache_flush;	///< Inform driver to skip flushing cache (instead YOU should do it). 0: normal, 1: skip cache flush

        struct _GRPH_REQUEST *p_next;
} GRPH_REQUEST, *PGRPH_REQUEST;


// Functions exported from graphic driver
extern ER       graph_open(GRPH_ID id);
extern BOOL     graph_is_opened(GRPH_ID id);
extern ER       graph_close(GRPH_ID id);

extern ER       graph_set_config(GRPH_ID id, GRPH_CONFIG_ID config_id, UINT32 config_context);
extern ER       graph_get_config(GRPH_ID id, GRPH_CONFIG_ID config_id, UINT32 *p_config_context);

extern ER       graph_request(GRPH_ID id, PGRPH_REQUEST p_request);

extern ER graph_enqueue(GRPH_ID id, KDRV_GRPH_TRIGGER_PARAM *p_param,
                KDRV_CALLBACK_FUNC *p_cb_func);



#endif
