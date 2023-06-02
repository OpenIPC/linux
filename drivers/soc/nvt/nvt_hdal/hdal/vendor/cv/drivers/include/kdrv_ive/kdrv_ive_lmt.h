/**
 * @file kdrv_ive_lmt.h
 * @brief parameter limitation of KDRV IVE
 * @author CVAI
 * @date in the year 2019
 */

#ifndef _KDRV_IVE_LIMIT_H_
#define _KDRV_IVE_LIMIT_H_

//=====================================================
// input / output limitation
#define IVE_WIDTH_MIN         1
#define IVE_WIDTH_MAX         16383

#define IVE_HEIGHT_MIN        1
#define IVE_HEIGHT_MAX        8191

#define IVE_OFSI_ALIGN        0x00000004
#define IVE_OFSO_ALIGN        0x00000004
#define IVE_IN_ADDR_ALIGN     0x00000004
#define IVE_OUT_ADDR_ALIGN    0x00000004

//=====================================================
// Para limitation
//#define IVE_GEN_FILT_MAX      15
#define IVE_EDGE_FILT_MAX     15
#define IVE_EDGE_FILT_MIN     -15
#define IVE_EDGE_THRES_MAX    255
#define IVE_EDGE_MAG_TH_MAX   128

#endif