/*
CNN module driver

NT98321 CNN registers header file.

@file       cnn_lmt.h
@ingroup    mIIPPCNN
@note       Nothing

Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/
#ifndef _CNN_LMT_H
#define _CNN_LMT_H

#ifdef __cplusplus
extern "C" {
#endif


#define CNN_CONV_K11_WIDTH_MIN          (11)
#define CNN_CONV_K11_HEIGHT_MIN         (11)
#define CNN_CONV_K9_WIDTH_MIN			(9)
#define CNN_CONV_K9_HEIGHT_MIN			(9)
#define CNN_CONV_K7_WIDTH_MIN           (7)
#define CNN_CONV_K7_HEIGHT_MIN          (7)
#define CNN_CONV_K5_WIDTH_MIN           (5)
#define CNN_CONV_K5_HEIGHT_MIN          (5)
#define CNN_CONV_K3_WIDTH_MIN           (3)
#define CNN_CONV_K3_HEIGHT_MIN          (3)
#define CNN_CONV_K1_WIDTH_MIN           (1)
#define CNN_CONV_K1_HEIGHT_MIN          (1)

#define CNN_WIDTH_MAX                   (1023)
#define CNN_WIDTH_MIN                   (1)
#define CNN_HEIGHT_MAX                  (1023)
#define CNN_HEIGHT_MIN                  (1)
#define CNN_CHANNEL_MAX       (2048)
#define CNN_CHANNEL_MIN       (1)
#define CNN_CONV_IMAGE_CHANNEL_MAX      (4)
#define CNN_CONV_IMAGE_CHANNEL_MIN      (1)
#define CNN_ELT_WIDTH_MAX               (1023)
#define CNN_ELT_WIDTH_MIN               (1)
#define CNN_ELT_HEIGHT_MAX              (1023)
#define CNN_ELT_HEIGHT_MIN              (1)
#define CNN_POOL_WIDTH_MAX               (1020)
#define CNN_POOL_WIDTH_MIN               (1)
#define CNN_POOL_HEIGHT_MAX              (1020)
#define CNN_POOL_HEIGHT_MIN              (1)
#define CNN_DECONV_WIDTH_MAX               (1023)
#define CNN_DECONV_WIDTH_MIN               (1)
#define CNN_DECONV_HEIGHT_MAX              (1023)
#define CNN_DECONV_HEIGHT_MIN              (1)
#define CNN_SCALE_WIDTH_MAX               (1023)
#define CNN_SCALE_WIDTH_MIN               (1)
#define CNN_SCALE_HEIGHT_MAX              (1023)
#define CNN_SCALE_HEIGHT_MIN              (1)

#define CNN_CONV_BATCH_MAX              (64)
#define CNN_CONV_BATCH_MIN              (1)
#define CNN_DECONV_BATCH_MAX            (31)
#define CNN_DECONV_BATCH_MIN            (1)

#define CNN_CONV_SIZE_MAX               (11)
#define CNN_CONV_SETNUM_MAX             (2048)
#define CNN_CONV_SETNUM_MIN             (1)
#define CNN_CONV_SHIFTB_MAX             (20)
#define CNN_CONV_SHIFTACC_MAX           (4)
#define CNN_CONV_OUT0OFFSET_MAX         (4)

#define CNN_BN_SHIFTM_MAX               (20)
#define CNN_SCALE_SHIFTB_MAX            (20)
#define SCALE_SHIFT_ALPHA_MAX           (31)

#define CNN_ELT_IN_SCALE_MAX			(65535)
#define CNN_ELT_IN_SCALE_MIN			(0)
#define CNN_ELT_IN_SHF_MAX			(31)
#define CNN_ELT_IN_SHF_MIN			(0)
#define CNN_ELT_SHF0_MAX			(31)
#define CNN_ELT_SHF0_MIN			(0)
#define CNN_ELT_SHF1_MAX			(31)
#define CNN_ELT_SHF1_MIN			(0)
#define CNN_ELT_OUTSHF_MAX			(31)
#define CNN_ELT_OUTSHF_MIN			(0)
#define CNN_ELT_COEF0_MAX			(32767)
#define CNN_ELT_COEF0_MIN			(-32768)
#define CNN_ELT_COEF1_MAX			(32767)
#define CNN_ELT_COEF1_MIN			(-32768)

#define CNN_RELU_LEAKY_VAL_MAX          (1023)
#define CNN_RELU_LEAKY_VAL_MIN          (-1024)
#define CNN_RELU_LEAKY_SHF_MAX          (18)
#define CNN_RELU_LEAKY_SHF_MIN          (7)

#define CNN_ROI_RATIO_MUL_MAX           (255)
#define CNN_ROI_RATIO_SHF_MAX           (15)
#define CNN_ROI_SHIFT_MAX               (15)
#define CNN_ROI_SHIFT_MIN               (-15)

#define CNN_POOL_AVE_MUL_MAX            (1024)
#define CNN_POOL_AVE_MUL_MIN            (0)
#define CNN_POOL_AVE_SHF_MAX            (28)
#define CNN_POOL_AVE_SHF_MIN            (10)
#define CNN_POOL_SHF_MAX                (7)
#define CNN_POOL_SHF_MIN                (0)

#define CNN_DECONV_PADNUM_MAX           (3)
#define CNN_DECONV_PADVAL_MAX           (0xffff)
#define CNN_DECONV_PADVAL_MIN           (0x0000)
#define CNN_DECONV_STRIDE_MAX           (2)


#define CNN_SCALEOUT_MAX                (65535)
#define CNN_SHFOUT_MAX                  (31)

#define CNN_RELU_LEAKY_BIT              (11)

#define CNN_FCD_ENC_BIT_LENGTH_MAX      (0xffffffff)
#define CNN_FCD_VLC_CODE_MAX            (0x007fffff)
#define CNN_FCD_VLC_VALID_MAX           (1)
#define CNN_FCD_VLC_OFFSET_MAX          (0x0000ffff)


#ifdef __cplusplus
}
#endif

#endif
