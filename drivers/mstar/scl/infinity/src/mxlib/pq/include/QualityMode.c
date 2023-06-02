
#include "QualityMode.h"

#define PQMODE_DBG(x)   //x
#ifndef ENABLE_VGA_EIA_TIMING
#define ENABLE_VGA_EIA_TIMING             1
#endif

#if PQ_ENABLE_UNUSED_FUNC
static MS_PQ_Hdmi_Info _stHDMI_Info[PQ_MAX_WINDOW];
static MS_PQ_Vd_Info _stVD_Info[PQ_MAX_WINDOW];
static MS_PQ_Mode_Info  _stMode_Info[PQ_MAX_WINDOW];
static MS_U8 _u8ModeIndex[PQ_MAX_WINDOW];
static MS_PQ_MuliMedia_Info _stMultiMedia_Info[PQ_MAX_WINDOW];
static MS_PQ_Dtv_Info _stDTV_Info[PQ_MAX_WINDOW];
static MS_PQ_Init_Info _stPQ_Info;
static MS_BOOL _gbMemfmt422[PQ_MAX_WINDOW]={1};
static MS_BOOL _gIsSrcHDMode[PQ_MAX_WINDOW];
static PQ_FOURCE_COLOR_FMT _gFourceColorFmt[PQ_MAX_WINDOW];
static MS_BOOL _gIs3D_En = 0;
static MS_BOOL _gIsH264 = 0;
static MS_BOOL _gIsMMplayer=0;
#endif


MS_U16 QM_InputSourceToIndex(PQ_WIN eWindow, MS_U8 enInputSourceType)
{
    MS_U16 u16SrcType = 0;
#if (PQ_QM_HDMI_PC) || (PQ_QM_PC)

    MS_U16 u16Input_HSize = QM_GetInputHSize(eWindow);
    MS_U16 u16Input_VSize = QM_GetInputVSize(eWindow);
    MS_U16 u16Input_VFreq = QM_GetInputVFreq(eWindow);
#endif

#if(PQ_ENABLE_UNUSED_FUNC)
    MS_BOOL bIsInterlaced = QM_IsInterlaced(eWindow);
#else
    MS_BOOL bIsInterlaced = 0;
#endif

#if (PQ_QM_CVBS)
    PQ_VIDEOSTANDARD_TYPE eStandard;
#endif

#if (PQ_QM_HDMI_PC) || (PQ_QM_PC)
    MS_U16 u16Disp_HSize = QM_GetDispHSize(eWindow);
    MS_U16 u16Disp_VSize = QM_GetDispVSize(eWindow);

    PQMODE_DBG(printf("\r\n QM: Src = %u, H = %u, V = %u", enInputSourceType, u16Input_HSize, u16Input_VSize));
    PQMODE_DBG(printf("\r\n     VFreq = %u, IsInterlace = %u", u16Input_VFreq, (MS_U16)bIsInterlaced));
    PQMODE_DBG(printf("\r\n     DispH = %u, DispV = %u", u16Disp_HSize, u16Disp_VSize));
#endif

#if ENABLE_VGA_EIA_TIMING
    // Only Progressiv mode
    if ((
         QM_IsSourceVGA(enInputSourceType)
    #if(PQ_ENABLE_UNUSED_FUNC)
         || QM_IsSourceDVI_HDMIPC(enInputSourceType, eWindow)
    #endif
         )
         && (!bIsInterlaced))
#else
    // Only DVI progressive mode, VGA could be interlace
    if (QM_IsSourceVGA(enInputSourceType) || (QM_IsSourceDVI_HDMIPC(enInputSourceType, eWindow) && !bIsInterlaced))
#endif
    {
        //DVI YUV mode is some special pattern gen timing in Korea company
        if (QM_IsSourceHDMI(enInputSourceType)
        #if(PQ_ENABLE_UNUSED_FUNC)
            &&  QM_HDMIPC_COLORYUV444(eWindow)
        #endif
            )
        {
#if (PQ_QM_HDMI_PC)
            // PC YUV444
            if (u16Input_HSize < u16Disp_HSize)
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_444_PC_Hup_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_444_PC_Hup_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_HDMI_444_PC_Hup_Vno_Main;
                }
            }
            else if (u16Input_HSize > u16Disp_HSize)
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_444_PC_Hdown_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_444_PC_Hdown_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_HDMI_444_PC_Hdown_Vno_Main;
                }
            }
            else
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_444_PC_Hno_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_444_PC_Hno_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_HDMI_444_PC_Hno_Vno_Main;
                }
            }
#endif
        }
        else if (QM_IsSourceHDMI(enInputSourceType)
            #if(PQ_ENABLE_UNUSED_FUNC)
                 && QM_HDMIPC_COLORYUV422(eWindow)
            #endif
                )
        {
#if (PQ_QM_HDMI_PC)
            // PC YUV422
            if (u16Input_HSize < u16Disp_HSize)
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_422_PC_Hup_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_422_PC_Hup_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_HDMI_422_PC_Hup_Vno_Main;
                }
            }
            else if (u16Input_HSize > u16Disp_HSize)
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_422_PC_Hdown_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_422_PC_Hdown_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_HDMI_422_PC_Hdown_Vno_Main;
                }
            }
            else
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_422_PC_Hno_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_HDMI_422_PC_Hno_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_HDMI_422_PC_Hno_Vno_Main;
                }
            }
#endif
        }
        else //(QM_HDMIPC_COLORRGB(eWindow)) & default
        {
#if (PQ_QM_PC)
            // PC RGB
            if (u16Input_HSize < u16Disp_HSize)
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hup_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hup_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hup_Vno_Main;
                }
            }
            else if (u16Input_HSize > u16Disp_HSize)
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hdown_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hdown_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hdown_Vno_Main;
                }
            }
            else
            {
                if (u16Input_VSize < u16Disp_VSize)
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hno_Vup_Main;
                }
                else if (u16Input_VSize > u16Disp_VSize)
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hno_Vdown_Main;
                }
                else
                {
                    u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hno_Vno_Main;
                }
            }
#endif
        }
    }
    else if (QM_IsSourceHDMI(enInputSourceType)
#if ENABLE_VGA_EIA_TIMING
             || QM_IsSourceVGA(enInputSourceType) //interlace VGA comes here
#endif
            )
    {
#if (PQ_QM_HMDI)
#if 0//debug message
    if(QM_HDMIPC_COLORRGB(eWindow))
        printf("HDMI RGB\n");
    else if(QM_HDMIPC_COLORYUV444(eWindow))
        printf("HDMI YUV 444\n");
    else
        printf("HDMI YUV 422\n");
#endif
        if( QM_IsSourceHDMI(enInputSourceType) && QM_HDMIPC_COLORRGB(eWindow) &&
            (MDrv_PQ_Get_PointToPoint(PQ_MAIN_WINDOW)) && (!bIsInterlaced))
        {
            // HDMI RGB progressive
            u16SrcType = QM_DVI_Dsub_HDMI_RGB_PC_Hno_Vno_Main;
        }
        else if( QM_IsSourceHDMI(enInputSourceType) && QM_HDMIPC_COLORYUV444(eWindow) &&
                 (MDrv_PQ_Get_PointToPoint(PQ_MAIN_WINDOW)) && (!bIsInterlaced))
        {
            // HDMI YUV444 progressive
            u16SrcType = QM_HDMI_444_PC_Hno_Vno_Main;
        }
        else
        {
            // HDMI
           if ((u16Input_HSize < 800) && (u16Input_VSize < 500))
            {
                if (bIsInterlaced)
                    u16SrcType = QM_HDMI_480i_Main;
                else
                    u16SrcType = QM_HDMI_480p_Main;
            }
            else if ((u16Input_HSize < 800) && (u16Input_VSize < 600))
            {
                if (bIsInterlaced)
                    u16SrcType = QM_HDMI_576i_Main;
                else
                    u16SrcType = QM_HDMI_576p_Main;
            }
            else if ((u16Input_HSize < 1300) && (u16Input_VSize < 800) && (!bIsInterlaced))
            {
                if (u16Input_VFreq > 550)
                {
                    u16SrcType = QM_HDMI_720p_60hz_Main;
                }
                else if(u16Input_VFreq > 250)
                {
                    u16SrcType = QM_HDMI_720p_50hz_Main;
                }
                else
                {
                    u16SrcType = QM_HDMI_720p_24hz_Main;
                }
            }
            else
            {
                if (bIsInterlaced)
                {
                    if (u16Input_VFreq > 550)
                    {
                        u16SrcType = QM_HDMI_1080i_60hz_Main;
                    }
                    else
                    {
                        u16SrcType = QM_HDMI_1080i_50hz_Main;
                    }
                }
                else
                {
                    if (u16Input_VFreq > 550)
                    {
                        u16SrcType = QM_HDMI_1080p_60hz_Main;
                    }
                    else if(u16Input_VFreq > 450)
                    {
                        u16SrcType = QM_HDMI_1080p_50hz_Main;
                    }
                    else  // Other timing
                    {
                        u16SrcType = QM_HDMI_1080p_24hz_Main;
                    }
                }
            }
        }
#endif
    }
    else if (QM_IsSourceYPbPr(enInputSourceType))
    {
#if (PQ_QM_YPBPR)
        if( QM_IsSourceYPbPr(enInputSourceType) &&
            (MDrv_PQ_Get_PointToPoint(PQ_MAIN_WINDOW)) && (!bIsInterlaced))
        {
            // YUV444 progressive
            u16SrcType = QM_HDMI_444_PC_Hno_Vno_Main;
        }
        else if (QM_IsYPbPr_720x480_60I(eWindow)){
            u16SrcType = QM_YPbPr_480i_Main;
        }
        else if (QM_IsYPbPr_720x480_60P(eWindow)){
            u16SrcType = QM_YPbPr_480p_Main;
        }
        else if (QM_IsYPbPr_720x576_50I(eWindow)){
            u16SrcType = QM_YPbPr_576i_Main;
        }
        else if (QM_IsYPbPr_720x576_50P(eWindow)){
            u16SrcType = QM_YPbPr_576p_Main;
        }
        else if (QM_IsYPbPr_1280x720_50P(eWindow)){
            u16SrcType = QM_YPbPr_720p_50hz_Main;
        }
        else if (QM_IsYPbPr_1280x720_60P(eWindow)){
            u16SrcType = QM_YPbPr_720p_60hz_Main;
        }
        else if (QM_IsYPbPr_1920x1080_50I(eWindow)){
            u16SrcType = QM_YPbPr_1080i_50hz_Main;
        }
        else if (QM_IsYPbPr_1920x1080_60I(eWindow)){
            u16SrcType = QM_YPbPr_1080i_60hz_Main;
        }
        else if (QM_IsYPbPr_1920x1080_24P(eWindow) || QM_IsYPbPr_1920x1080_30P(eWindow) || QM_IsYPbPr_1920x1080_25P(eWindow) ){
            u16SrcType = QM_YPbPr_1080p_24hz_Main;
        }
        else if (QM_IsYPbPr_1920x1080_50P(eWindow)){
            u16SrcType = QM_YPbPr_1080p_50hz_Main;
        }
        else if (QM_IsYPbPr_1920x1080_60P(eWindow)){
            u16SrcType = QM_YPbPr_1080p_60hz_Main;
        }
        else {
            u16SrcType = QM_YPbPr_720p_24hz_Main;
        }
#endif
    }
    else if(QM_IsSrource656(enInputSourceType))
    {
#if (PQ_QM_CVBS)
        if(u16Input_VSize <= 480)
        {
            if(bIsInterlaced)
            {
                u16SrcType = QM_HDMI_480i_Main;
            }
            else
            {
                u16SrcType = QM_HDMI_480p_Main;
            }
        }
        else
        {
            if(bIsInterlaced)
            {
                u16SrcType = QM_HDMI_576i_Main;
            }
            else
            {
                u16SrcType = QM_HDMI_576p_Main;
            }
        }
#endif
    }
    else if (QM_IsSourceDTV(enInputSourceType))
    {
#if (PQ_QM_DTV)
        if(QM_IsDTV_IFRAME(eWindow))
        {
          #if 1 //( CHIP_FAMILY_TYPE != CHIP_FAMILY_S7J)
            //If PQ has devide the iframe setting according to P/I mode, then use new PQ srctype
            if (u16Input_VSize < 720)
            {
                if (bIsInterlaced)
                    u16SrcType = QM_DTV_iFrame_SD_interlace_Main;
                else
                    u16SrcType = QM_DTV_iFrame_SD_progressive_Main;
            }
            else
            {
                if (bIsInterlaced)
                    u16SrcType = QM_DTV_iFrame_HD_interlace_Main;
                else
                    u16SrcType = QM_DTV_iFrame_HD_progressive_Main;
            }
          #else
             u16SrcType = QM_DTV_iFrame_Main;
          #endif
        }
        else if (QM_IsDTV_MPEG2(eWindow))
        {
            if (u16Input_VSize < 500)
            {
                if ((u16Input_HSize < 400) && bIsInterlaced)
                {
                    u16SrcType = QM_DTV_480i_352x480_MPEG2_Main;
                }
                else
                {
                    if (bIsInterlaced)
                        u16SrcType = QM_DTV_480i_MPEG2_Main;
                    else
                        u16SrcType = QM_DTV_480p_MPEG2_Main;
                }
            }
            else if (u16Input_VSize < 650)
            {
                if (bIsInterlaced)
                    u16SrcType = QM_DTV_576i_MPEG2_Main;
                else
                    u16SrcType = QM_DTV_576p_MPEG2_Main;
            }
            else if ((u16Input_VSize < 900) && (bIsInterlaced==0))
            {
                if (u16Input_VFreq > 550)
                {
                    u16SrcType = QM_DTV_720p_60hz_MPEG2_Main;
                }
                else if(u16Input_VFreq > 250)
                {
                    u16SrcType = QM_DTV_720p_50hz_MPEG2_Main;
                }
                else
                {
                    u16SrcType = QM_DTV_720p_24hz_MPEG2_Main;
                }
            }
            else
            {
                if (bIsInterlaced)
                {
                    if (u16Input_VFreq > 550)
                    {
                        u16SrcType = QM_DTV_1080i_60hz_MPEG2_Main;
                    }
                    else
                    {
                        u16SrcType = QM_DTV_1080i_50hz_MPEG2_Main;
                    }
                }
                else
                {
                    if (u16Input_VFreq > 550)
                    {
                        u16SrcType = QM_DTV_1080p_60hz_MPEG2_Main;
                    }
                    else if(u16Input_VFreq > 250)
                    {
                        u16SrcType = QM_DTV_1080p_50hz_MPEG2_Main;
                    }
                    else
                    {
                        u16SrcType = QM_DTV_1080p_24hz_MPEG2_Main;
                    }

                }
            }
        }
        else // QM_IsDTV_H264(eWindow)
        {
            if ((u16Input_HSize < 1260) && (u16Input_VSize < 650))
            {
                if (u16Input_VSize < 500)
                {
                    if ((u16Input_HSize < 400) && bIsInterlaced)
                    {
                        u16SrcType = QM_DTV_480i_352x480_H264_Main;
                    }
                    else
                    {
                        if (bIsInterlaced)
                            u16SrcType = QM_DTV_480i_H264_Main;
                        else
                            u16SrcType = QM_DTV_480p_H264_Main;
                    }
                }
                else if (u16Input_VSize < 650)
                {
                    if (bIsInterlaced)
                        u16SrcType = QM_DTV_576i_H264_Main;
                    else
                        u16SrcType = QM_DTV_576p_H264_Main;
                }
            }
            else
            {
                if ((u16Input_VSize < 900) && (bIsInterlaced==0))
                {
                    if (u16Input_VFreq > 550)
                    {
                        u16SrcType = QM_DTV_720p_60hz_H264_Main;
                    }
                    else if(u16Input_VFreq > 250)
                    {
                        u16SrcType = QM_DTV_720p_50hz_H264_Main;
                    }
                    else
                    {
                        u16SrcType = QM_DTV_720p_24hz_H264_Main;
                    }
                }
                else
                {
                    if (bIsInterlaced)
                    {
                        if (u16Input_VFreq > 550)
                        {
                            u16SrcType = QM_DTV_1080i_60hz_H264_Main;
                        }
                        else
                        {
                            u16SrcType = QM_DTV_1080i_50hz_H264_Main;
                        }
                    }
                    else
                    {
                        if (u16Input_VFreq > 550)
                        {
                            u16SrcType = QM_DTV_1080p_60hz_H264_Main;
                        }
                        else if(u16Input_VFreq > 250)
                        {
                            u16SrcType = QM_DTV_1080p_50hz_H264_Main;
                        }
                        else
                        {
                            u16SrcType = QM_DTV_1080p_24hz_H264_Main;
                        }
                    }
                }
            }
        }
#endif
    }
    else if (QM_IsSourceMultiMedia(enInputSourceType))
    {
#if (PQ_QM_MM_VIDEO) || (PQ_QM_MM_PHOTO)
        if (QM_IsMultiMediaMOVIE(eWindow))
        {
            if ((u16Input_HSize <= 720) && (u16Input_VSize <= 576))
            {
                if (bIsInterlaced)
                {
                    u16SrcType = QM_Multimedia_video_SD_interlace_Main;
                }
                else
                {
                    u16SrcType = QM_Multimedia_video_SD_progressive_Main;
                }
            }
            else
            {
                if (bIsInterlaced)
                {
                    u16SrcType = QM_Multimedia_video_HD_interlace_Main;
                }
                else
                {
                    u16SrcType = QM_Multimedia_video_HD_progressive_Main;
                }
            }
        }
        else //QM_IsMultiMediaPHOTO(eWindow)
        {
            if ((u16Input_HSize <= 720) && (u16Input_VSize <= 576))
            {
                u16SrcType = QM_Multimedia_photo_SD_progressive_Main;
            }
            else
            {
                u16SrcType = QM_Multimedia_photo_HD_progressive_Main;
            }
        }
#endif
    }
#if (PQ_ENABLE_UNUSED_FUNC)
    else if (QM_IsSourceScartRGB(enInputSourceType, eWindow))
    {
#if (PQ_QM_CVBS)
        if (u16Input_VFreq > 550)
        {
            u16SrcType = QM_SCART_RGB_NTSC_Main;
        }
        else
        {
            u16SrcType = QM_SCART_RGB_PAL_Main;
        }
#endif
    }
    else if (QM_IsSourceScartCVBS(enInputSourceType, eWindow))
    {
#if (PQ_QM_CVBS)
        eStandard = QM_GetATVStandard(eWindow);
        switch(eStandard)
        {
        case E_PQ_VIDEOSTANDARD_PAL_M:
            u16SrcType = QM_SCART_AV_PAL_M_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_N:
            u16SrcType = QM_SCART_AV_PAL_N_Main;
            break;
        case E_PQ_VIDEOSTANDARD_NTSC_44:
            u16SrcType = QM_SCART_AV_NTSC_44_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_60:
            u16SrcType = QM_SCART_AV_PAL_60_Main;
            break;
        case E_PQ_VIDEOSTANDARD_NTSC_M:
            u16SrcType = QM_SCART_AV_NTSC_M_Main;
            break;
        case E_PQ_VIDEOSTANDARD_SECAM:
            u16SrcType = QM_SCART_AV_SECAM_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_BGHI:
        default:
            u16SrcType = QM_SCART_AV_PAL_BGHI_Main;
            break;
        }
#endif
    }
#endif
    else if (QM_IsSourceATV(enInputSourceType))
    {
#if (PQ_QM_CVBS)
        eStandard = QM_GetATVStandard(eWindow);
        if (QM_FRONTEND_RFIN(eWindow))
        {
            switch(eStandard)
            {
            case E_PQ_VIDEOSTANDARD_PAL_M:
                u16SrcType = QM_RF_PAL_M_Main;
                break;
            case E_PQ_VIDEOSTANDARD_PAL_N:
                u16SrcType = QM_RF_PAL_N_Main;
                break;
            case E_PQ_VIDEOSTANDARD_NTSC_44:
                u16SrcType = QM_RF_NTSC_44_Main;
                break;
            case E_PQ_VIDEOSTANDARD_PAL_60:
                u16SrcType = QM_RF_PAL_60_Main;
                break;
            case E_PQ_VIDEOSTANDARD_NTSC_M:
                u16SrcType = QM_RF_NTSC_M_Main;
                break;
            case E_PQ_VIDEOSTANDARD_SECAM:
                u16SrcType = QM_RF_SECAM_Main;
                break;
            case E_PQ_VIDEOSTANDARD_PAL_BGHI:
            default:
                u16SrcType = QM_RF_PAL_BGHI_Main;
                break;
            }
        }
        else // QM_FRONTEND_VIFIN(eWindow)
        {
            switch(eStandard)
            {
            case E_PQ_VIDEOSTANDARD_PAL_M:
                u16SrcType = QM_VIF_PAL_M_Main;
                break;
            case E_PQ_VIDEOSTANDARD_PAL_N:
                u16SrcType = QM_VIF_PAL_N_Main;
                break;
            case E_PQ_VIDEOSTANDARD_NTSC_44:
                u16SrcType = QM_VIF_NTSC_44_Main;
                break;
            case E_PQ_VIDEOSTANDARD_PAL_60:
                u16SrcType = QM_VIF_PAL_60_Main;
                break;
            case E_PQ_VIDEOSTANDARD_NTSC_M:
                u16SrcType = QM_VIF_NTSC_M_Main;
                break;
            case E_PQ_VIDEOSTANDARD_SECAM:
                u16SrcType = QM_VIF_SECAM_Main;
                break;
            case E_PQ_VIDEOSTANDARD_PAL_BGHI:
            default:
                u16SrcType = QM_VIF_PAL_BGHI_Main;
                break;
            }
        }
#endif
    }
    else if (QM_IsSourceSV(enInputSourceType))
    {
#if (PQ_QM_CVBS)
        eStandard = QM_GetATVStandard(eWindow);
        switch(eStandard)
        {
        case E_PQ_VIDEOSTANDARD_PAL_M:
            u16SrcType = QM_SV_PAL_M_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_N:
            u16SrcType = QM_SV_PAL_N_Main;
            break;
        case E_PQ_VIDEOSTANDARD_NTSC_44:
            u16SrcType = QM_SV_NTSC_44_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_60:
            u16SrcType = QM_SV_PAL_60_Main;
            break;
        case E_PQ_VIDEOSTANDARD_NTSC_M:
            u16SrcType = QM_SV_NTSC_M_Main;
            break;
        case E_PQ_VIDEOSTANDARD_SECAM:
            u16SrcType = QM_SV_SECAM_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_BGHI:
        default:
            u16SrcType = QM_SV_PAL_BGHI_Main;
            break;
        }
#endif
    }

    else if(QM_IsSourceISP(enInputSourceType))
    {
#if (PQ_QM_ISP)
        u16SrcType = QM_FHD_YUV422_Main;
#endif
    }
    else // AV
    {
#if (PQ_QM_CVBS)
        eStandard = QM_GetATVStandard(eWindow);
        switch(eStandard)
        {
        case E_PQ_VIDEOSTANDARD_PAL_M:
            u16SrcType = QM_AV_PAL_M_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_N:
            u16SrcType = QM_AV_PAL_N_Main;
            break;
        case E_PQ_VIDEOSTANDARD_NTSC_44:
            u16SrcType = QM_AV_NTSC_44_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_60:
            u16SrcType = QM_AV_PAL_60_Main;
            break;
        case E_PQ_VIDEOSTANDARD_NTSC_M:
            u16SrcType = QM_AV_NTSC_M_Main;
            break;
        case E_PQ_VIDEOSTANDARD_SECAM:
            u16SrcType = QM_AV_SECAM_Main;
            break;
        case E_PQ_VIDEOSTANDARD_PAL_BGHI:
        default:
            u16SrcType = QM_AV_PAL_BGHI_Main;
            break;
        }
#endif
    }
    return u16SrcType;

}
