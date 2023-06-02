#ifndef __KDRV_GFX_IF_H__
#define __KDRV_GFX_IF_H__

#include "kwrap/type.h"
#include "kdrv_gfx2d/kdrv_affine.h"
#include "kdrv_gfx2d/kdrv_affine_lmt.h"

INT32 kdrv_gfx_if_register_affine_open(INT32 (*open)(UINT32 chip, UINT32 engine));
INT32 kdrv_gfx_if_register_affine_trigger(INT32 (*trigger)(UINT32 id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
						  KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data));
INT32 kdrv_gfx_if_register_affine_close(INT32 (*close)(UINT32 chip, UINT32 engine));

extern INT32 (*kdrv_gfx_if_affine_open)(UINT32 chip, UINT32 engine);
extern INT32 (*kdrv_gfx_if_affine_trigger)(UINT32 id, KDRV_AFFINE_TRIGGER_PARAM *p_param,
						  KDRV_CALLBACK_FUNC *p_cb_func, VOID *p_user_data);
extern INT32 (*kdrv_gfx_if_affine_close)(UINT32 chip, UINT32 engine);

INT32 kdrv_gfx_if_init(void);
INT32 kdrv_gfx_if_exit(void);

#endif