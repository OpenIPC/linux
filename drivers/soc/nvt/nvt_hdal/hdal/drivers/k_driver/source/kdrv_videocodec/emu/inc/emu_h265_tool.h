#ifndef _EMU_H265_TOOL_H_
#define _EMU_H265_TOOL_H_

#include "kwrap/type.h"

// for hw trigger //
#include "h26x.h"

// for sw driver //
#include "h26x_def.h"

// for emulation //
#include "emu_h26x_common.h"

//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)
extern char gucH26XEncTestSrc_Cut[][2][128];

void h26xMemSetAddr(UINT32 *puiVirAddr,UINT32 *puiVirMemAddr,UINT32 uiSize);
void h26xMemSet(UINT32 uiMemAddr,UINT32 uiVal,UINT32 uiSize);
void h26xMemCpy(UINT32 uiSrcMemAddr,UINT32 uiDesMemAddr,UINT32 uiSize);
void h26xSrand(unsigned int seed);
int h26xRand(void);
void h26xEmuRotateSrc(H26XFile *pH26XSrcFile,UINT8 *ucHwSrcAddr,UINT8 *ucEmuSrcAddr,UINT32 uiSrcSize,
                    UINT32 uiWidth,UINT32 uiHeight,UINT32 uiLineOffset,UINT32 uiMaxBuf, INT32 iAngle, UINT32 isY, UINT32 write_out, UINT32 uv_swap);
UINT32 emu_buf_chk_sum(UINT8 *buf, UINT32 size, UINT32 format);//hk TODO: it's buf_chk_sum
void h26xSwapRecAndRefIdx(UINT32 *a, UINT32 *b);
UINT8 emu_bit_reverse(UINT8 pix);//hk TODO: it's bit_reverse
void h26xPrintString(UINT8 *s, INT32 iLen);


//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif	// _EMU_H265_TOOL_H_



