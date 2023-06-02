/*
    Protected header file of NT96650's ide driver.

    The header file for protected APIs of NT96650's ide driver.

    @file       ide_protected.h
    @ingroup    mIDrvDisp_IDE
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _IDE_PROTECTED_H
#define _IDE_PROTECTED_H

#include "ide.h"

/*
    @addtogroup mIDrvDisp_IDE
*/
//@{

/*
   Defines ide DMA burst length

   Defines ide DMA burst length
*/
typedef enum {
	IDE_DMA_BURST_LEN_16 = 0,
	IDE_DMA_BURST_LEN_24 = 1,
	IDE_DMA_BURST_LEN_32 = 2,
	IDE_DMA_BURST_LEN_48 = 3,
	IDE_DMA_BURST_LEN_64 = 4,

	ENUM_DUMMY4WORD(IDE_DMA_BURST_LEN)
} IDE_DMA_BURST_LEN;


/*
    Set Video1 burst length

    Set Video1 burst length

    @param[in] id   ide ID
    @param[in] chy  channel y burst length
    @param[in] chc  channel c burst length

    @return
		- @b FALSE  : Something error
		- @b TRUE   : OK
*/
extern BOOL idec_set_v1_burst_len(IDE_ID id, IDE_DMA_BURST_LEN chy, IDE_DMA_BURST_LEN chc);

/*
    Set Video2 burst length

    Set Video2 burst length

    @param[in] id   ide ID
    @param[in] chy  channel y burst length
    @param[in] chc  channel c burst length

    @return
		- @b FALSE  : Something error
		- @b TRUE   : OK
*/
extern BOOL idec_set_v2_burst_len(IDE_ID id, IDE_DMA_BURST_LEN chy, IDE_DMA_BURST_LEN chc);

/*
    Get Video1 burst length

    Get Video1 burst length

    @param[in] id       ide ID
    @param[out] chy     channel y burst length
    @param[out] chc     channel c burst length

    @return Always return TRUE
*/
extern BOOL idec_get_v1_burst_len(IDE_ID id, IDE_DMA_BURST_LEN *chy, IDE_DMA_BURST_LEN *chc);

/*
    Get Video2 burst length

    Get Video2 burst length

    @param[in] id       ide ID
    @param[out] chy     channel y burst length
    @param[out] chc     channel c burst length

    @return Always return TRUE
*/
extern BOOL idec_get_v2_burst_len(IDE_ID id, IDE_DMA_BURST_LEN *chy, IDE_DMA_BURST_LEN *chc);


/*
    Set OSD1 burst length

    Set OSD1 burst length

    @param[in] id       ide ID
    @param[in] chdma    channel dma of palette or alpha burst length
    @param[in] chrgb    channel rgb burst length

    @return
		- @b FALSE  : Something error
		- @b TRUE   : OK
*/
extern BOOL idec_set_o1_burst_len(IDE_ID id, IDE_DMA_BURST_LEN chdma, IDE_DMA_BURST_LEN chrgb);

/*
    Set OSD2 burst length

    Set OSD2 burst length

    @param[in] id       ide ID
    @param[in] chdma    channel dma of palette or alpha burst length
    @param[in] chrgb    channel rgb burst length

    @return
		- @b FALSE  : Something error
		- @b TRUE   : OK
*/
extern BOOL idec_set_o2_burst_len(IDE_ID id, IDE_DMA_BURST_LEN chdma, IDE_DMA_BURST_LEN chrgb);

/*
    Get OSD1 burst length

    Get OSD1 burst length

    @param[in] id       ide ID
    @param[out] chy     channel dma of palette or alpha burst length
    @param[out] chc     channel rgb burst length

    @return Always return TRUE
*/
extern BOOL idec_get_o1_burst_len(IDE_ID id, IDE_DMA_BURST_LEN *chdma, IDE_DMA_BURST_LEN *chrgb);

/*
    Get OSD2 burst length

    Get OSD2 burst length

    @param[in] id       ide ID
    @param[out] chy     channel dma of palette or alpha burst length
    @param[out] chc     channel rgb burst length

    @return Always return TRUE
*/
extern BOOL idec_get_o2_burst_len(IDE_ID id, IDE_DMA_BURST_LEN *chdma, IDE_DMA_BURST_LEN *chrgb);


/**
    Set ide FD & Line layer swap

    Set ide OSD1 & OSD2 layer swap

    @param[in] id       ide ID
    @param[in] b_swap    Swap FD line or not(FALSE:FD->Line(up to low)/TRUE:Line->FD(up to low))


    @return void
*/
extern void idec_set_fd_line_layer_swap(IDE_ID id, BOOL b_swap);

/**
    Get ide FD & Line layer swap

    Get ide OSD1 & OSD2 layer swap

    @param[in] id       ide ID

     @return
		- @b TRUE: swap (Line->FD:up to low)
		- @b FALSE: not swap(FD->Line:up to low)
*/
extern BOOL idec_get_fd_line_layer_swap(IDE_ID id);
//@}

#endif
