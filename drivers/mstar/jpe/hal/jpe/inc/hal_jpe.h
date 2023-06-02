#ifndef _Jpe_HAL_H_
#define _Jpe_HAL_H_

//#include "vm_types.ht"
//#include <stdbool.h>
#include <linux/kernel.h>
#include <asm/types.h>
#include "_hal_jpe_platform.h"
#include "hal_ext_int_pub.h"
#include "_hal_jpe_reg.h"

#if defined(_FPGA_) || defined(_UDMA_) || defined(_HIF_)
#include "fpgaapi.h"
#endif

//typedef  int                bool;
//typedef  unsigned short     U16;

#define JPE_HAL_ALLOC_MEM_FAIL 0
#define JPE_HAL_ALLOC_MEM_SUCCESS 1

typedef enum
{
    JPE_HAL_QLEVEL_1 = 0,
    JPE_HAL_QLEVEL_2,
    JPE_HAL_QLEVEL_3,
    JPE_HAL_QLEVEL_4,
    JPE_HAL_QLEVEL_5,
    JPE_HAL_QLEVEL_6,
}JpeHalQLevel_e;

typedef enum
{
    JPE_HAL_IRQ_FRAME_DONE      = 0x00000001,   // bit 0, jpe_framedone
    JPE_HAL_IRQ_BSPOBUF0_FULL   = 0x00000002,   // bit 1, jpe_bspobuf0_full, SW mode: buf full; HW mode: buf0 full
    JPE_HAL_IRQ_IMGBUF_FULL     = 0x00000004,   // bit 2, img_buf_full_irq
    JPE_HAL_IRQ_BSPOBUF1_FULL   = 0x00000008,   // bit 3, jpe_bspobuf1_full, SW mode: not used; HW mode: buf1 full
    JPE_HAL_IRQ_FS_FAIL         = 0x00000020,   // bit 5, fs_fail_irq
    JPE_HAL_IRQ_LESS_ROW_DONE   = 0x00000080,   // bit 7, less_row_down_irq
    JPE_HAL_IRQ_IMI_OVERFLOW    = 0x00000100,   // bit 8, jpe2scl_imi_overflow_int
    JPE_HAL_IRQ_TXIP_TIMEOUT    = 0x10000000,   // bit 4 when reg_jpe_g_vs_irq_en = 0, txip_time_out_irq
    JPE_HAL_IRQ_VS              = 0x20000000,   // bit 4 when reg_jpe_g_vs_irq_en = 1, vs
    JPE_HAL_IRQ_NET_TRIGGER     = 0x40000000,   // bit 4 when reg_jpe_g_row_down_irq_en = 0, net_trigger
    JPE_HAL_IRQ_VIU_ROW_DONE    = 0x80000000,   // bit 6 when reg_jpe_g_row_down_irq_en = 1, viu2jpe_row_done_jpe
    JPE_HAL_IRQ_ALL             = 0xF00001AF    // ALL bits
}JpeHalIrqEvent_e;

typedef enum
{
    JPE_OBUF_0=0,
    JPE_OBUF_1=1,
} JpeHalOBufOdr_e;

typedef enum
{
    JPE_FSVS_SW_MODE=0,
    JPE_FSVS_SW_HW_MODE,
    JPE_FSVS_HW_MODE,
    JPE_FSVS_HW_WO_RESTART_MODE,
    JPE_FSVS_HW_WI_RESTART_MODE,
    JPE_FSVS_INVALID,
} JpeFsVsGenerationMode_e;

typedef enum
{
    JPE_HAL_OBUF_SINGLE_MODE = 0x0,
    JPE_HAL_OBUF_MULTI_MODE = 0x1
} JpeHalOutBufMode_e;

typedef enum
{
    JPE_HAL_BUFFER_ROW_MODE = 0x0,
    JPE_HAL_BUFFER_FRAME_MODE = 0x1
} JpeHalInBufMode_e;

typedef enum
{
    JPE_HAL_CODEC_JPEG = 0x1,
    JPE_HAL_CODEC_H263I = 0x2,
    JPE_HAL_CODEC_ENCODE_DCT = 0x4,
} JpeHalCodecFormat_e;

typedef enum
{
    JPE_HAL_SCL_HANDSHAKE_OFF = 0,
    JPE_HAL_SCL_HANDSHAKE_ON
} JpeHalSclHandShake_e;

typedef enum
{
    JPE_HAL_RAW_YUYV = 0x0,
    JPE_HAL_RAW_YVYU = 0x1,
    JPE_HAL_RAW_NV12 = 0x3,
    JPE_HAL_RAW_NV21 = 0x4,
} JpeHalRawFormat_e;

typedef struct
{
    u32 nInBufYAddr[2];
    u32 nInBufCAddr[2];
} JpeHalInBufCfg_t, *pJpeHalInBufCfg;


typedef struct
{
    u32 nOutBufAddr[2];
    u32 nOutBufSize[2];
    u32 nJpeOutBitOffset;
} JpeHalOutBufCfg_t, *pJpeHalOutBufCfg;

typedef struct
{
    u32 nBaseAddr;
    u32 nSize;
    JpeReg_t* pJpeReg;
} JpeHalHandle_t, *pJpeHalHandle;



//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                 IRQ API                                                                          //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HalJpeSetIrqMask(JpeHalHandle_t* handle,JpeHalIrqEvent_e mask);
void HalJpeClearIrq(JpeHalHandle_t* handle,JpeHalIrqEvent_e mask);
JpeHalIrqEvent_e HalJpeGetIrqCheck(JpeHalHandle_t* handle,JpeHalIrqEvent_e mask);
u16 HalJpeGetIrq(JpeHalHandle_t* handle);
//JpeHalIrqEvent_e HalJpeGetIrqEvent(u16 u16IrqReg, JpeHalIrqEvent_e IrqEventMask);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                            Quantization Tables Setting                                                            //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int HalJpeSetQTable(JpeHalHandle_t* handle,const u16 *q_table_y, const u16 *q_table_c, u32 size);
int HalJpeReadQTable(JpeHalHandle_t* handle,u16 *q_table_y, u16 *q_table_c, u32 *size);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                 Configurations                                                                   //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HalJpeSetJpeMode(JpeHalHandle_t* handle,u32 bUseJPE);
void HalJpeSetCodecFormat(JpeHalHandle_t* handle,JpeHalCodecFormat_e format);
void HalJpeSetRawFormat(JpeHalHandle_t* handle,JpeHalRawFormat_e format);
void HalJpeSetPicDim(JpeHalHandle_t* handle,u32 width, u32 height);
void HalJpeToggleOutBuf(JpeHalHandle_t* handle,u32 nObufIdx);
void HalJpeSetInputMode(JpeHalHandle_t* handle,JpeHalInBufMode_e mode);
void HalJpeSetOutputMode(JpeHalHandle_t* handle,JpeHalOutBufMode_e mode);
void HalJpeSetInBuf(JpeHalHandle_t* handle,pJpeHalInBufCfg ptInBufCfg);
void HalJpeSetOutBuf(JpeHalHandle_t* handle,pJpeHalOutBufCfg ptOutBufCfg);
void HalJpeSetDctMbAddr(JpeHalHandle_t* handle,u32 PhyAddr);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                  Controls                                                                        //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HalJpeSoftReset(JpeHalHandle_t *handle);
void HalJpeSetEnable(JpeHalHandle_t *handle);
void HalJpeHwRowCounterReset(JpeHalHandle_t *handle);
void HalJpeSetSWRowDown(JpeHalHandle_t *handle);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                  Status                                                                          //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool HalJpeResetSafe(JpeHalHandle_t *handle);
u32 HalJpeGetOutputByteSize(JpeHalHandle_t *handle,u32 nObufSel);
u16 HalJpeGetRowDownCnt(JpeHalHandle_t *handle);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                   TXIP?                                                                          //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HalJpeStop(JpeHalHandle_t *handle,bool opt);
u16 HalJpeGetRowCount(JpeHalHandle_t *handle);
void HalJpeIdleCount(JpeHalHandle_t *handle,u16 nIdleCount);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                   Debugs?                                                                        //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void HalJpeSetCycleReport(JpeHalHandle_t *handle,u16 mode);
void HalJpeSetCrc(JpeHalHandle_t *handle,u16 mode);

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                                                                                                                  //
//                                                                Capabilities?                                                                     //
//                                                                                                                                                  //
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
u8 HalJpeGetBufferModeSupportInfo(void);
u8 HalJpeGetSclHandShakeSupportInfo(void);
u8 HalJpeGetCodecSupportInfo(void);
u32 HalJpeGetOutBufSLogAddrAlignInfo(void);
u32 HalJpeGetOutBufELogAddrAlignInfo(void);
u32 HalJpeGetRefYLogAddrAlignInfo(u8 bufID);
u32 HalJpeGetRefCLogAddrAlignInfo(u8 bufID);

#endif

