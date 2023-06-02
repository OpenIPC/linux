#include "h26x.h"
#include "h26xenc_api.h"
#include "h26xenc_wrap.h"
#include "h26x_bitstream.h"
#include "h264enc_api.h"
#include "h265enc_api.h"
#include "h26xenc_rc.h"

#if defined(__LINUX)
EXPORT_SYMBOL(h26x_getHwRegSize);
EXPORT_SYMBOL(h26x_setNextBsDmaBuf);
EXPORT_SYMBOL(h26x_resetHW);
EXPORT_SYMBOL(h26x_close);
EXPORT_SYMBOL(h26x_setBsLen);
EXPORT_SYMBOL(h26x_setBSDMA);
EXPORT_SYMBOL(h26x_flushCache);
EXPORT_SYMBOL(h26x_cache_clean);
EXPORT_SYMBOL(h26x_cache_invalidate);
EXPORT_SYMBOL(h26x_getPhyAddr);
EXPORT_SYMBOL(h26x_waitINT);
EXPORT_SYMBOL(h26x_start);
EXPORT_SYMBOL(h26x_efuse_check);
EXPORT_SYMBOL(h26x_exit_isr);
EXPORT_SYMBOL(h26x_reset);
EXPORT_SYMBOL(h26x_setIntEn);
EXPORT_SYMBOL(h26x_getIntEn);
EXPORT_SYMBOL(h26x_setEncDirectRegSet);
EXPORT_SYMBOL(h26x_getBusyStatus);
//EXPORT_SYMBOL(h26x_getChipId);
EXPORT_SYMBOL(h26x_setEncLLRegSet);
EXPORT_SYMBOL(h26x_llc_cb_init);
//EXPORT_SYMBOL(h26x_wp_cb_init);
EXPORT_SYMBOL(h26x_prtReg);
EXPORT_SYMBOL(h26x_prtMem);
EXPORT_SYMBOL(h26x_getDebug);
EXPORT_SYMBOL(h26x_low_latency_patch);
EXPORT_SYMBOL(h26x_module_reset);
EXPORT_SYMBOL(h26x_setDmaChDis);
EXPORT_SYMBOL(h26x_tick_open);
EXPORT_SYMBOL(h26x_tick_close);
EXPORT_SYMBOL(h26x_tick_result);

EXPORT_SYMBOL(h26xEnc_getGdrCfg);
EXPORT_SYMBOL(h26xEnc_wrapJndCfg);
EXPORT_SYMBOL(h26XEnc_setAqCfg);
EXPORT_SYMBOL(h26xEnc_getAqCfg);
EXPORT_SYMBOL(h26xEnc_getIspRatioCfg);
EXPORT_SYMBOL(h26xEnc_getRowRcCfg);
EXPORT_SYMBOL(h26XEnc_setRcInit);
EXPORT_SYMBOL(h26XEnc_setRoiCfg);
EXPORT_SYMBOL(h26XEnc_setOsgPalCfg);
//EXPORT_SYMBOL(h26XEnc_setMotAddrCfg);
EXPORT_SYMBOL(h26XEnc_setMdInfoCfg);
EXPORT_SYMBOL(H26XEnc_setUsrQpCfg);
EXPORT_SYMBOL(h26xEnc_getVaAPBAddr);
EXPORT_SYMBOL(h26XEnc_setRowRcCfg);
EXPORT_SYMBOL(h26XEnc_setOsgWinCfg);
EXPORT_SYMBOL(h26XEnc_setIspCbCfg);
EXPORT_SYMBOL(h26XEnc_setSliceSplitCfg);
EXPORT_SYMBOL(h26XEnc_setOsgRgbCfg);
EXPORT_SYMBOL(h26XEnc_setTnrCfg);
EXPORT_SYMBOL(h26XEnc_setGdrCfg);
EXPORT_SYMBOL(h26xEnc_getBsLen);
EXPORT_SYMBOL(h26XEnc_setJndCfg);
EXPORT_SYMBOL(h26xEnc_getNaluLenResult);
#if H26X_SET_PROC_PARAM
EXPORT_SYMBOL(h26xEnc_getRCDumpLog);
EXPORT_SYMBOL(h26xEnc_setRCDumpLog);
#endif
#if JND_DEFAULT_ENABLE
EXPORT_SYMBOL(h26xEnc_getJNDParam);
EXPORT_SYMBOL(h26xEnc_setJNDParam);
#endif
#if H26X_USE_DIFF_MAQ
EXPORT_SYMBOL(h26xEnc_getDiffMAQParam);
EXPORT_SYMBOL(h26xEnc_setDiffMAQParam);
#endif
#if LPM_PROC_ENABLE
EXPORT_SYMBOL(h26XEnc_getLpmCfg);
EXPORT_SYMBOL(h26XEnc_setLpmCfg);
#endif

EXPORT_SYMBOL(h26xEnc_getNDQPStep);
EXPORT_SYMBOL(h26xEnc_setNDQPStep);
EXPORT_SYMBOL(h26xEnc_getNDQPRange);
EXPORT_SYMBOL(h26xEnc_setNDQPRange);

EXPORT_SYMBOL(h26xEnc_getVersion);
EXPORT_SYMBOL(h26xEnc_getVaLLCAddr);

EXPORT_SYMBOL(h26xEnc_wrapAqCfg);
EXPORT_SYMBOL(h26xEnc_wrapVarCfg);
EXPORT_SYMBOL(h26xEnc_wrapRndCfg);
EXPORT_SYMBOL(h26xEnc_wrapMotAqCfg);
EXPORT_SYMBOL(h26xEnc_wrapLpmCfg);
EXPORT_SYMBOL(h26xEnc_wrapQpRelatedCfg);
EXPORT_SYMBOL(h26XEnc_setSpnCfg);

//EXPORT_SYMBOL(h26xEnc_wrapBgrCfg);
//EXPORT_SYMBOL(h26xEnc_wrapRmdCfg);
//EXPORT_SYMBOL(h26xEnc_wrapTnrCfg);
//EXPORT_SYMBOL(h26xEnc_wrapLambdaCfg);
EXPORT_SYMBOL(h26xEnc_queryRecFrmNum);
EXPORT_SYMBOL(get_bits);
EXPORT_SYMBOL(init_parse_bitstream);
EXPORT_SYMBOL(read_rbsp_trailing_bits);
EXPORT_SYMBOL(read_uvlc_codeword);
EXPORT_SYMBOL(read_signed_uvlc_codeword);

EXPORT_SYMBOL(SetMemoryAddr);
EXPORT_SYMBOL(ebspTorbsp);
EXPORT_SYMBOL(save_to_reg);
EXPORT_SYMBOL(h26x_setLLCmd);

EXPORT_SYMBOL(h264Enc_InitRdoCfg);
EXPORT_SYMBOL(h264Enc_setRdoCfg);
EXPORT_SYMBOL(h264Enc_getGopNum);
EXPORT_SYMBOL(h264Enc_setFroCfg);
EXPORT_SYMBOL(h264Enc_queryMemSize);
EXPORT_SYMBOL(h264Enc_getNxtPicType);
EXPORT_SYMBOL(h264Enc_prepareOnePicture);
EXPORT_SYMBOL(h264Enc_getResult);
EXPORT_SYMBOL(h264Enc_initEncoder);
EXPORT_SYMBOL(h264Enc_getSeqHdr);
EXPORT_SYMBOL(h264Enc_getRdoCfg);
EXPORT_SYMBOL(h264Enc_setGopNum);
EXPORT_SYMBOL(h264Enc_queryRecFrmSize);

#if H26X_SET_PROC_PARAM
EXPORT_SYMBOL(h264Enc_getRowRCStopFactor);
EXPORT_SYMBOL(h264Enc_setRowRCStopFactor);
EXPORT_SYMBOL(h264Enc_getPReduce16Planar);
EXPORT_SYMBOL(h264Enc_setPReduce16Planar);
#endif
EXPORT_SYMBOL(h264Enc_setFrameNumGapAllow);
EXPORT_SYMBOL(h264Enc_setFixSPSLog2Poc);
EXPORT_SYMBOL(h264Enc_getEncodeRatio);


EXPORT_SYMBOL(h265Enc_getSeqHdr);
EXPORT_SYMBOL(h265Enc_queryMemSize);
EXPORT_SYMBOL(h265Enc_getNxtPicType);
EXPORT_SYMBOL(h265Enc_prepareOnePicture);
EXPORT_SYMBOL(h265Enc_initEncoder);
EXPORT_SYMBOL(h265Enc_setFroCfg);
EXPORT_SYMBOL(h265Enc_getResult);
EXPORT_SYMBOL(h265Enc_InitRdoCfg);
EXPORT_SYMBOL(h265Enc_setRdoCfg);
EXPORT_SYMBOL(h265Enc_getRdoCfg);
EXPORT_SYMBOL(h265Enc_setGopNum);
EXPORT_SYMBOL(h265Enc_queryRecFrmSize);
EXPORT_SYMBOL(h265Enc_getEncodeRatio);

#if H26X_SET_PROC_PARAM
EXPORT_SYMBOL(h265Enc_getRowRCStopFactor);
EXPORT_SYMBOL(h265Enc_setRowRCStopFactor);
#endif
EXPORT_SYMBOL(h26x_getDummyWTSize);
EXPORT_SYMBOL(rc_cb_init);
#if H26X_MEM_USAGE
EXPORT_SYMBOL(h26xEnc_getMemUsage);
#endif
EXPORT_SYMBOL(h265Enc_setLongStartCode);
#endif

