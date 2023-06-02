#ifndef __PR2100K_H__
#define __PR2100L_H__

//#include <linux/ioctl.h>
enum{
    PR2100K_720P25 =	    0x01,
    PR2100K_720P30 =	    0x02,
    PR2100K_720P50 =	    0x03,
    PR2100K_720P60 =    	0x04,
    PR2100K_1080P25   =	    0x05,
    PR2100K_1080P30   =    	0x06,
    PR2100K_1080I50 =	    0x07,
    PR2100K_1080I60 =		0x08,
    PR2100K_960_576I50	=   0x09,
    PR2100K_960_480I60   =  0x0a,
    PR2100K_720_576I50  =   0x0b,
    PR2100K_720_480I60   =  0x0c,
};



enum{
    VIDEO_UNPLUG,
    VIDEO_IN,
    VIDEO_LOCKED,
    VIDEO_UNLOCK
};

enum{
    DATA_PAGE=0,
    CH_0_PAGE=1,   //
    CH_1_PAGE=2,    //
    PTZ_0_PAGE=3,    //
    PTZ_1_PAGE=4,     //
    MIPI_LOG_PAGE=5,
    MIPI_PHY_PAGE=6,
};

enum{
    STD_PVI,
    STD_CVI,
    STD_HDA,
    STD_HDT,
};

enum{
    RES_480I=0,
    RES_576I=1,
    RES_720P=2,
    RES_1080P=3,
    RES_960P=4,
};
enum{
    RATE_25HZ,
    RATE_30HZ,
    RATE_50HZ,
    RATE_60HZ,
};
enum{ // Added AUDIO detection enum value - 10/16/2018
    AUDIO_NO_DETECTED,
    AUDIO_DETECTED
};

#define FLAG_LOSS           0x80
#define FLAG_H_LOCKED     0x20
#define FLAG_HV_LOCKED    0x60

#define CHANNELS_PER_CHIP 	4
#define MAX_CHIPS 	2
#define SUCCESS				0
#define FAILURE				-1

#endif


