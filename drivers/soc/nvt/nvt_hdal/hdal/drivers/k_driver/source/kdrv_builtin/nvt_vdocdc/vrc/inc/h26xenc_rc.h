#ifndef _VRC_H_
#define _VRC_H_

#include "h26xenc_rc_param.h"
#include "h26xenc_rate_control.h"

typedef struct _rc_cb_{
	int (*h26xEnc_RcInit)(H26XEncRC *pRc, H26XEncRCParam *pRCParam);
	int (*h26xEnc_RcPreparePicture)(H26XEncRC *pRc, H26XEncRCPreparePic *pPic);
	int (*h26xEnc_RcUpdatePicture)(H26XEncRC *pRc, H26XEncRCUpdatePic *pUPic);
    int (*h26xEnc_RcGetLog)(H26XEncRC *pRc, unsigned int *log_addr);
}rc_cb;

rc_cb *rc_cb_init(void);

#endif
