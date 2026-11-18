#pragma once

typedef enum {
    BL_VPU_VDEC = 0,
    BL_VPU_VENC,
    BL_VPU_JENC,
    BL_MM_VPP,
    BL_MM_GDC,
    BL_MM_TDP,
    BL_MM_IVE,
    BL_MM_DPU,
    BL_MM_DPU_LITE,
    BL_ISP_IFE,
    BL_ISP_ITP,
    BL_ISP_YUV,
    BL_CPU2DDR,
    BL_FLASH_AXDMA,
    BL_NPU,
    BL_NUM_MAX
} SUB_SYS_BW_LIMITERS;

typedef enum {
    BL_VPU_SYS = 1,
    BL_MM_SYS,
    BL_ISP_SYS,
    BL_COMM_SYS,
    BL_FLASH_SYS,
    BL_SYS_ID_MAX
} SYS_ID_BW_LIMITERS;

