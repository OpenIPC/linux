#ifndef _AE_UI_H_
#define _AE_UI_H_

#define AE_UI_INFOR_ERR   0xFFFFFFFF

typedef struct _AE_UI_INFO_TAB {
	UINT32 value;
	UINT32 max_value;
} AE_UI_INFO_TAB;

/**
	AE informatin selection index(ae_set_ui_info() or ae_get_ui_info())
*/
typedef enum _AE_UI_INFO {
	AE_UI_FREQUENCY,                ///<AE anti-frequency, data type #AE_FREQUENCY
	AE_UI_METER,                    ///<AE detect mode, data type #AE_METER
	AE_UI_EV,                       ///<AE ev compensation, data type #AE_EV
	AE_UI_ISO,                      ///<AE iso index, data type #AE_ISO
	AE_UI_CAP_LONGEXP,              ///<Capture long exposure on/off, data type #AE_CAP_LONGEXP
	AE_UI_CAP_LONGEXP_EXPT,         ///<Capture exposure time for long exposure mode
	AE_UI_CAP_LONGEXP_ISO,          ///<Capture ISO for long exposure mode
	AE_UI_OPERATION,                ///<AE movie/photo, data type ##AE_OPERATION
	AE_UI_STITCH_ID,                ///<AE stitch ID mapping, MSB 16bit for LEFT, LSB 16bit for RIGHT, set 0 for disable stitch mode
	AE_UI_MAX_CNT,
	ENUM_DUMMY4WORD(AE_UI_INFO)
} AE_UI_INFO;

/**
	AE frequency
*/
typedef enum _AE_FREQUENCY {
	AE_FREQUENCY_50HZ,              ///< Anti-freq 50Hz
	AE_FREQUENCY_60HZ,              ///< Anti-freq 60Hz
	AE_FREQUENCY_55HZ,              ///< Anti-freq 55Hz
	AE_FREQUENCY_MAX_CNT,
	ENUM_DUMMY4WORD(AE_FREQUENCY)
} AE_FREQUENCY;

/**
	AE meter
*/
typedef enum _AE_METER {
	AE_METER_CENTERWEIGHTED,          ///< Center weighting
	AE_METER_SPOT,                    ///< Spot
	AE_METER_MATRIX,                  ///< Matic
	AE_METER_EVALUATIVE,              ///<
	AE_METER_FACEDETECTION,           ///< Face
	AE_METER_USERDEFINE,              ///< User define
	AE_METER_SMARTIR,                 ///< Smart IR
	AE_METER_ROI,                     ///< ROI
	AE_METER_LA,                      ///< LA Matrix
	AE_METER_MAX_CNT,
	ENUM_DUMMY4WORD(AE_METER)
} AE_METER;

/**
	AE EV
*/
typedef enum _AE_EV {
	AE_EV_N4,                ///< EV -4
	AE_EV_N11_3,             ///< EV -11/3
	AE_EV_N7_2,              ///< EV -7/2
	AE_EV_N10_3,             ///< EV -10/3
	AE_EV_N3,                ///< EV -3
	AE_EV_N8_3,              ///< EV -8/3
	AE_EV_N5_2,              ///< EV -5/2
	AE_EV_N7_3,              ///< EV -7/3
	AE_EV_N2,                ///< EV -2
	AE_EV_N5_3,              ///< EV -5/3
	AE_EV_N3_2,              ///< EV -3/2
	AE_EV_N4_3,              ///< EV -4/3
	AE_EV_N1,                ///< EV -1
	AE_EV_N2_3,              ///< EV -2/3
	AE_EV_N1_2,              ///< EV -1/2
	AE_EV_N1_3,              ///< EV -1/3
	AE_EV_00,                ///< EV +0
	AE_EV_P1_3,              ///< EV +1/3
	AE_EV_P1_2,              ///< EV +1/2
	AE_EV_P2_3,              ///< EV +2/3
	AE_EV_P1,                ///< EV +1
	AE_EV_P4_3,              ///< EV +4/3
	AE_EV_P3_2,              ///< EV +3/2
	AE_EV_P5_3,              ///< EV +5/3
	AE_EV_P2,                ///< EV +2
	AE_EV_P7_3,              ///< EV +7/3
	AE_EV_P5_2,              ///< EV +5/2
	AE_EV_P8_3,              ///< EV +8/3
	AE_EV_P3,                ///< EV +3
	AE_EV_P10_3,             ///< EV +10/3
	AE_EV_P7_2,              ///< EV +7/2
	AE_EV_P11_3,             ///< EV +11/3
	AE_EV_P4,                ///< EV +4
	AE_EV_MAX_CNT,
	ENUM_DUMMY4WORD(AE_EV)
} AE_EV;

/**
	AE iso index
*/
typedef enum _AE_ISO {
	AE_ISO_AUTO =     0,
	AE_ISO_100 =      100,
	AE_ISO_200 =      200,
	AE_ISO_400 =      400,
	AE_ISO_800 =      800,
	AE_ISO_1600 =     1600,
	AE_ISO_3200 =     3200,
	AE_ISO_6400 =     6400,
	AE_ISO_12800 =    12800,
	AE_ISO_25600 =    25600,
	AE_ISO_MAX_CNT,
	ENUM_DUMMY4WORD(AE_ISO)
} AE_ISO;

/**
	AE anti shake on/off
*/
typedef enum _AE_ANTISHAKE {
	AE_ANTISHAKE_LV0 = 0,   ///< disable
	AE_ANTISHAKE_LV1,
	AE_ANTISHAKE_LV2,
	AE_ANTISHAKE_MAX_CNT,
	ENUM_DUMMY4WORD(AE_ANTISHAKE)
} AE_ANTISHAKE;

/**
	AE capture long exposure time on/off
*/
typedef enum _AE_CAP_LONGEXP {
	AE_CAP_LONGEXP_OFF,
	AE_CAP_LONGEXP_ON,
	AE_CAP_LONGEXP_MAX_CNT,
	ENUM_DUMMY4WORD(AE_CAP_LONGEXP)
} AE_CAP_LONGEXP;

/**
	AE movie/photo
*/
typedef enum _AE_OPERATION {
	AE_OPERATION_MOVIE,
	AE_OPERATION_PHOTO,
	AE_OPERATION_CAPTURE,
	AE_OPERATION_MAX_CNT,
	ENUM_DUMMY4WORD(AE_OPERATION)
} AE_OPERATION;

#endif

