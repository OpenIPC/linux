
#if PQ_GRULE_UFSC_DYNAMIC_CONTRAST_ENABLE
 unsigned char MST_GRule_DYNAMIC_CONTRAST_IP_Index_Main[PQ_GRULE_DYNAMIC_CONTRAST_IP_NUM_Main]=
{
    PQ_IP_VIP_DLC_Main,
};
#endif


#if PQ_GRULE_UFSC_DYNAMIC_CONTRAST_ENABLE
 unsigned char MST_GRule_DYNAMIC_CONTRAST_Main[QM_INPUTTYPE_NUM_Main][PQ_GRULE_DYNAMIC_CONTRAST_NUM_Main][PQ_GRULE_DYNAMIC_CONTRAST_IP_NUM_Main]=
{
    #if PQ_QM_ISP
    {//FHD_YUV422, 0
        {//Off
            PQ_IP_VIP_DLC_OFF_Main,
        },
        {//On
            PQ_IP_VIP_DLC_OFF_Main,
        },
    },
    #endif
};
#endif
