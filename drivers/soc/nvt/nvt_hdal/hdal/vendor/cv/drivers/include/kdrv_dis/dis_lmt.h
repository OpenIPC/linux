/*
DIS module driver

NT98520 DIS limitation

@file       dis_lmt.h
@ingroup    mIIPPCNN
@note       Nothing

Copyright   Novatek Microelectronics Corp. 2019.  All rights reserved.
*/

#define	DIS_ENUM_SR_MIN				0		// -32 ~ +32
#define	DIS_ENUM_SR_MAX				1		// -16 ~ +16
#define	DIS_ENUM_BLOCK_SZ_MIN		0		// 64x48
#define	DIS_ENUM_BLOCK_SZ_MAX		1		// 32x32
#define	DIS_ENUM_LUT_MIN			0		// positive LUT
#define	DIS_ENUM_LUT_MAX			1		// linear LUT
#define	DIS_BLK_NUM_HOR_MIN			1		// min block# in horizontal direction
#define	DIS_BLK_NUM_HOR_MAX			4		// max block# in horizontal direction
#define	DIS_BLK_NUM_VER_MIN			1		// min block# in vertical direction
#define	DIS_BLK_NUM_VER_MAX			64		// max block# in vertical direction
#define	DIS_MDS_NUM_MIN				1		// min MDS# 
#define	DIS_MDS_NUM_MAX				32		// max MDS#
#define	DIS_SCROFFS_MIN				0
#define	DIS_SCROFFS_MAX				255
#define	DIS_SCRTHR_MIN				0
#define	DIS_SCRTHR_MAX				255
#define	DIS_CENTER_SCRTHR_MIN		0		//
#define	DIS_CENTER_SCRTHR_MAX		255		//
#define	DIS_FTCNT_THR_MIN			0		//
#define	DIS_FTCNT_THR_MAX			255		//
#define	DIS_MDSOFS_MIN				0		// ref = cur + (mdsofs-128) - 32
#define	DIS_MDSOFS_MAX				255		// ref = cur + (mdsofs-128) - 32
#define	DIS_CREDIT_MIN				0
#define	DIS_CREDIT_MAX				255

