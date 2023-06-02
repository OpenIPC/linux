

#define QM_IsSourceVGA(x)               ((x)==PQ_INPUT_SOURCE_VGA)
#define QM_IsSourceHDMI(x)              ((x)==PQ_INPUT_SOURCE_HDMI)
#define QM_IsSourceDVI_HDMIPC(x,y)      (QM_IsSourceHDMI(x) && (_stHDMI_Info[y].bIsHDMI == FALSE))
#define QM_IsSourceHDMI_Video(x,y)      (QM_IsSourceHDMI(x) && (_stHDMI_Info[y].bIsHDMI == TRUE) && (_stHDMI_Info[y].bIsHDMIPC == FALSE))
#define QM_IsSourceHDMI_PC(x,y)         (QM_IsSourceHDMI(x) && (_stHDMI_Info[y].bIsHDMI == TRUE) && (_stHDMI_Info[y].bIsHDMIPC == TRUE))

#define QM_HDMIPC_COLORRGB(x)           (_stHDMI_Info[x].enColorFmt == PQ_HDMI_COLOR_RGB)
#define QM_HDMIPC_COLORYUV422(x)        (_stHDMI_Info[x].enColorFmt == PQ_HDMI_COLOR_YUV_422)
#define QM_HDMIPC_COLORYUV444(x)        (_stHDMI_Info[x].enColorFmt == PQ_HDMI_COLOR_YUV_444)

#define QM_IsSrource656(x)              ((x)== PQ_INPUT_SOURCE_BT656)

#define QM_IsSourceYPbPr(x)             ((x)==PQ_INPUT_SOURCE_YPBPR)

#define QM_IsSourceYPbPrSD(x,y)         (QM_IsSourceYPbPr(x)                        \
                                        && ( _u8ModeIndex[y] == PQ_MD_720x480_60I   \
                                           ||_u8ModeIndex[y] == PQ_MD_720x480_60P   \
                                           ||_u8ModeIndex[y] == PQ_MD_720x576_50I   \
                                           ||_u8ModeIndex[y] == PQ_MD_720x576_50P) )

#define QM_IsSourceDTV(x)               ((x)==PQ_INPUT_SOURCE_DTV)
#define QM_IsSourceMultiMedia(x) (((x)>=PQ_INPUT_SOURCE_STORAGE) && ((x)<=PQ_INPUT_SOURCE_JPEG))

#define QM_IsMultiMediaMOVIE(x)         (_stMultiMedia_Info[x].eType == E_PQ_MULTIMEDIA_MOVIE)
#define QM_IsMultiMediaPHOTO(x)         (_stMultiMedia_Info[x].eType == E_PQ_MULTIMEDIA_PHOTO)

#define QM_IsDTV_MPEG2(x)               (_stDTV_Info[x].eType == E_PQ_DTV_MPEG2)
#define QM_IsDTV_H264(x)                (_stDTV_Info[x].eType == E_PQ_DTV_H264)
#define QM_IsDTV_IFRAME(x)              (_stDTV_Info[x].eType == E_PQ_DTV_IFRAME)

#define QM_IsSourceScartRGB(x,y)   (((x)==PQ_INPUT_SOURCE_SCART) && (_stVD_Info[y].bIsSCART_RGB == TRUE))
#define QM_IsSourceScartCVBS(x,y)  (((x)==PQ_INPUT_SOURCE_SCART) && (_stVD_Info[y].bIsSCART_RGB == FALSE))
#define QM_IsSourceScart(x)             ((x)==PQ_INPUT_SOURCE_SCART)
#define QM_IsSourceATV(x)               ((x)==PQ_INPUT_SOURCE_TV)
#define QM_IsSourceSV(x)                ((x)==PQ_INPUT_SOURCE_SVIDEO)
#define QM_IsSourceAV(x)                ((x)==PQ_INPUT_SOURCE_CVBS)
#define QM_IsSourceVD(x)                (  QM_IsSourceATV(x)    \
                                        || QM_IsSourceAV(x)     \
                                        || QM_IsSourceSV(x)     \
                                        || QM_IsSourceScart(x) )
#define QM_IsSourceISP(x)           ((x) == PQ_INPUT_SOURCE_ISP)
#define QM_Is3DVideo()           (_gIs3D_En == TRUE)

#define QM_FRONTEND_RFIN(x)             (!_stVD_Info[x].bIsVIFIN)
#define QM_FRONTEND_VIFIN(x)            (_stVD_Info[x].bIsVIFIN)

#define QM_GetInputHSize(x)             (_stMode_Info[x].u16input_hsize)
#define QM_GetInputVSize(x)             (_stMode_Info[x].u16input_vsize)

#define QM_H_Size_Check_x1(x, y)        ( ((x) < ((y)*1+50)) &&  ((x) > ((y)*1-50)) )
#define QM_H_Size_Check_x2(x, y)        ( ((x) < ((y)*2+50)) &&  ((x) > ((y)*2-50)) )
#define QM_H_Size_Check_x4(x, y)        ( ((x) < ((y)*4+50)) &&  ((x) > ((y)*4-50)) )
#define QM_H_Size_Check_x8(x, y)        ( ((x) < ((y)*8+50)) &&  ((x) > ((y)*8-50)) )

#define QM_IsYPbPr_720x480_60I(x)       (_u8ModeIndex[x] == PQ_MD_720x480_60I)
#define QM_IsYPbPr_720x480_60P(x)       (_u8ModeIndex[x] == PQ_MD_720x480_60P)
#define QM_IsYPbPr_720x576_50I(x)       (_u8ModeIndex[x] == PQ_MD_720x576_50I)
#define QM_IsYPbPr_720x576_50P(x)       (_u8ModeIndex[x] == PQ_MD_720x576_50P)
#define QM_IsYPbPr_1280x720_50P(x)      (_u8ModeIndex[x] == PQ_MD_1280x720_50P)
#define QM_IsYPbPr_1280x720_60P(x)      (_u8ModeIndex[x] == PQ_MD_1280x720_60P)
#define QM_IsYPbPr_1920x1080_50I(x)     (_u8ModeIndex[x] == PQ_MD_1920x1080_50I)
#define QM_IsYPbPr_1920x1080_60I(x)     (_u8ModeIndex[x] == PQ_MD_1920x1080_60I)

#define QM_IsYPbPr_1920x1080_24P(x)     (_u8ModeIndex[x] == PQ_MD_1920x1080_24P)
#define QM_IsYPbPr_1920x1080_25P(x)     (_u8ModeIndex[x] == PQ_MD_1920x1080_25P)
#define QM_IsYPbPr_1920x1080_30P(x)     (_u8ModeIndex[x] == PQ_MD_1920x1080_30P)
#define QM_IsYPbPr_1920x1080_50P(x)     (_u8ModeIndex[x] == PQ_MD_1920x1080_50P)
#define QM_IsYPbPr_1920x1080_60P(x)     (_u8ModeIndex[x] == PQ_MD_1920x1080_60P)

#define QM_GetInputVFreq(x)             (_stMode_Info[x].u16input_vfreq)
#define QM_IsInterlaced(x)              (_stMode_Info[x].bInterlace)

#define QM_GetDispHSize(x)              (_stMode_Info[x].u16display_hsize)
#define QM_GetDispVSize(x)              (_stMode_Info[x].u16display_vsize)

#define QM_GetATVStandard(x)            (_stVD_Info[x].enVideoStandard)
