#ifndef _H265DEC_CFG_H_
#define _H265DEC_CFG_H_

//#include "Type.h"
#include "h265dec_api.h"
#include "h265dec_int.h"
#include "h265dec_header.h"

extern void h265Dec_initCfg(H265DEC_INIT *pH265DecInit, H265DecSeqCfg *pH265DecSeqCfg, H265DecPicCfg *pH265DecPicCfg, H265DecHdrObj *pH265DecHdrObj);
//extern void h265Dec_updateCfg(H265DEC_INFO *pH265DecInfo,H265DecPicCfg *pH265DecPicCfg,H265DecHdrObj *pH265DecHdrObj);

#endif // _H265DEC_CFG_H_
