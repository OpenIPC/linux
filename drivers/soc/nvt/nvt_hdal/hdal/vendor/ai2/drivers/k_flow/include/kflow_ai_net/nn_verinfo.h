#ifndef _NN_VERINFO_H_
#define _NN_VERINFO_H_

#define CNN_528_PSW                     1  ////////// _TODO : remove this later
#define CNN_25_MATLAB                   0 //0: new interface, 1: matlab interface
#define CNN_CGEN_NEW_TMP_BUF            1 //0: original mctrl, 1: mctrl with new tmp buffer info (only available if CNN_25_MATLAB=0) (only available if CNN_FMT_V4=0)
#define CNN_FMT_V4                      1 //0: v3 format, 1: v4 format (only available if CNN_25_MATLAB=0)
#define CNN_INNER_POSTPROC              1 //0: user call postproc, 1: ai2 call postproc
#define CNN_USER_POSTPROC               1 //0: user call postproc, 1: ai2 call postproc
#if defined(_BSP_NA51089_)
#define CNN_NUE_EXCLUSIVE               1 //0: original flow, 1: exclusive flow for _BSP_NA51089_
#else
#define CNN_NUE_EXCLUSIVE               0 //0: original flow, 1: exclusive flow for _BSP_NA51089_
#endif
#define CNN_MULTI_INPUT                 1 //0: original flow, 1: support muliple input flow
#define CNN_AI_FASTBOOT                 1 //1: ai fastboot related flow

#if defined(_BSP_NA51068_)
#define USE_GET_BLK                     1 // only for 32x, use get_blk to alloc init buf & work buf
#endif

/////////////////////////////////////////////////////////////////////////////////////////////
//           CNN_25_MATLAB    CNN_CGEN_NEW_TMP_BUF       CNN_FMT_V4
//  0714           0                    0                     0
//  0729           0                    1                     0  (But 0729 is NG bin, DO NOT use)
//  0805           0                    1                     0
//  0812           0                    1                     0
//  0921           0                    1                     1
/////////////////////////////////////////////////////////////////////////////////////////////

#define AI_V4                           CNN_FMT_V4
#define CUST_SUPPORT_MULTI_IO           CNN_FMT_V4

#if CNN_25_MATLAB
typedef enum {
	NN_CHIP_CNN20   = 0,              ///< chip CNN 2.0
	NN_CHIP_CNN25_A = 1,              ///< chip CNN 2.5  520
	NN_CHIP_CNN25_B = 2,              ///< chip CNN 2.5  321
	NN_CHIP_CNN25_C = 3,              ///< chip CNN 2.5  528
	NN_CHIP_CNN25_A_NEW_FMT ,         ///< chip CNN 2.5  520 new format
	NN_CHIP_CNN25_B_NEW_FMT ,         ///< chip CNN 2.5  321 new format
	NN_CHIP_CNN25_C_NEW_FMT ,         ///< chip CNN 2.5  528 new format
	ENUM_DUMMY4WORD(NN_CHIP_ID)
} NN_CHIP_ID;
#else
typedef enum {
	NN_CHIP_CNN20   = 0,         ///< chip CNN 313 
	NN_CHIP_CNN25_A = 1,         ///< chip CNN 520 
	NN_CHIP_CNN25_B = 2,         ///< chip CNN 321 
	NN_CHIP_CNN25_C = 3,         ///< chip CNN 528 
	NN_CHIP_CNN25_D = 4,         ///< chip CNN 560
	ENUM_DUMMY4WORD(NN_CHIP_ID)
} NN_CHIP_ID;

typedef enum { 
	NN_CHIP_AI1 = 0,         //v2
	NN_CHIP_AI2 = 1,         //v3
	ENUM_DUMMY4WORD(NN_CHIP_FMT)
} NN_CHIP_FMT;

typedef enum {
	NN_CHIP_AI_SUBVER0 = 0,         //vx.0
	NN_CHIP_AI_SUBVER1 = 1,         //vx.1
	NN_CHIP_AI_SUBVER2 = 2,         //vx.2
	ENUM_DUMMY4WORD(NN_CHIP_FMT_SUBVER)
} NN_CHIP_FMT_SUBVER;
#endif


#define NN_VERSION_ENTRY(_major, _minor, _bugfix, _branch)  ((_major << 24) + (_minor << 16) + (_bugfix << 8) + _branch)
#if CNN_25_MATLAB
#define NN_GEN_VERSION			NN_VERSION_ENTRY(0x13, 0x0, 0x0, 0x0)
#define NN_CHIP                 NN_VERSION_ENTRY(0x0, NN_CHIP_AI1, 0x0, NN_CHIP_CNN25_A)
#else
#define NN_GEN_VERSION			NN_VERSION_ENTRY(0x15, 0x0, 0x0, 0x0)
#define NN_CHIP                 NN_VERSION_ENTRY(NN_CHIP_AI_SUBVER1, NN_CHIP_AI2, 0x0, NN_CHIP_CNN25_A)
#endif

#endif
