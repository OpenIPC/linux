#ifndef __AKPCM_H__
#define __AKPCM_H__

/********************** IOCTL *********************************************/
#define PCM_IOC_MAGIC			'P'
#define AKPCM_IO(nr)			_IOC(_IOC_NONE, PCM_IOC_MAGIC, nr, 0)
#define AKPCM_IOR(nr)			_IOR(PCM_IOC_MAGIC, nr, int)
#define AKPCM_IORn(nr, size)	_IOR(PCM_IOC_MAGIC, nr, size)
#define AKPCM_IOW(nr)			_IOW(PCM_IOC_MAGIC, nr, int)
#define AKPCM_IOWn(nr, size)	_IOW(PCM_IOC_MAGIC, nr, size)
/* ------------- command ------------------------------------------------ */
#define IOC_NR_PREPARE		(0xE0)
#define IOC_NR_RESUME		(0xE1)
#define IOC_NR_PAUSE		(0xE2)
#define IOC_NR_RSTBUF		(0xE3)
#define IOC_NR_RSTALL		(0xE4)
#define IOC_NR_GETELAPSE	(0xE5)
#define IOC_NR_GETTIMER		(0xE6)
#define IOC_NR_GETSTATUS	(0xE7)
#define IOC_NR_GETDATALEN	(0xE8)
#define IOC_NR_NOTICE_END	(0xE9)
#define IOC_PREPARE			AKPCM_IO(IOC_NR_PREPARE)
#define IOC_RESUME			AKPCM_IO(IOC_NR_RESUME)
#define IOC_PAUSE			AKPCM_IO(IOC_NR_PAUSE)
#define IOC_RSTBUF			AKPCM_IO(IOC_NR_RSTBUF)
#define IOC_RSTALL			AKPCM_IO(IOC_NR_RSTALL)
#define IOC_GETELAPSE		AKPCM_IORn(IOC_NR_GETELAPSE, unsigned long long)
#define IOC_GETTIMER		AKPCM_IORn(IOC_NR_GETTIMER, unsigned long)
#define IOC_GET_STATUS		AKPCM_IORn(IOC_NR_GETSTATUS, unsigned long)
#define IOC_GET_DATA_LENGTH	AKPCM_IORn(IOC_NR_GETDATALEN, unsigned long)
#define IOC_NOTICE_END	    AKPCM_IO(IOC_NR_NOTICE_END)

/* ------------- HW parameters ------------------------------------------ */
/* ------------- HW configures ------------------------------------------ */
/* ------------- SW configures ------------------------------------------ */
#define IOC_NR_FEATS		(0xF0) /* refer to akpcm_features */
#define IOC_NR_PARS			(0xF2) /* refer to akpcm_pars */
#define IOC_GET_FEATS		AKPCM_IORn(IOC_NR_FEATS, struct akpcm_features)
#define IOC_GET_PARS		AKPCM_IORn(IOC_NR_PARS, struct akpcm_pars)
#define IOC_SET_PARS		AKPCM_IOWn(IOC_NR_PARS, struct akpcm_pars)
/* ------------- SOURCEs -------------------------------------------------- */
#define IOC_NR_SOURCES		(0x10)
#define IOC_GET_SOURCES		AKPCM_IOR(IOC_NR_SOURCES)
/* If set to SIGNAL_SRC_MUTE, devices will be power down */
#define IOC_SET_SOURCES		AKPCM_IOW(IOC_NR_SOURCES)
/* ------------- GAINs ------------------------------------------------- */
#define IOC_NR_GAIN			(0x30) /* HP */
#define IOC_GET_GAIN		AKPCM_IOR(IOC_NR_GAIN)
#define IOC_SET_GAIN		AKPCM_IOW(IOC_NR_GAIN)
/* ------------- DEVICEs ---------------------------------------- */
#define IOC_NR_DEV			(0x70)
#define IOC_GET_DEV			AKPCM_IOR(IOC_NR_DEV)
#define IOC_SET_DEV			AKPCM_IOW(IOC_NR_DEV)
/* ------------- NR & AGC ---------------------------------------- */
#define IOC_NR_AGC			(0x80)
#define IOC_SET_NR_AGC		AKPCM_IOW(IOC_NR_AGC)
#define IOC_NR_MAX			(0x82)
#define IOC_SET_NR_MAX		AKPCM_IOW(IOC_NR_MAX)
#define IOC_NR_AEC          (0x83)
#define IOC_SET_AEC         AKPCM_IOWn(IOC_NR_AEC, unsigned int)
#define IOC_AEC_DUMP		(0x84)
#define IOC_SET_AEC_DUMP	AKPCM_IOW(IOC_AEC_DUMP)
#define IOC_AEC_READ_PARAM	(0x85)
#define IOC_GET_ECHO_PARAM	AKPCM_IOR(IOC_AEC_READ_PARAM)
#define IOC_AEC_WRITE_PARAM	(0x86)
#define IOC_SET_ECHO_PARAM	AKPCM_IOW(IOC_AEC_WRITE_PARAM)
#define IOC_AD_EQ			(0x87)
#define IOC_SET_AD_EQ	AKPCM_IOW(IOC_AD_EQ)
#define IOC_AD_EQ_ATTR		(0x88)
#define IOC_SET_AD_EQ_ATTR	AKPCM_IOW(IOC_AD_EQ_ATTR)

/* ------------- SPEAKER ---------------------------------------- */
#define IOC_SPEAKER			(0x81)
#define IOC_SET_SPEAKER		AKPCM_IOW(IOC_SPEAKER)

/* play_dev */
#define AKPCM_PLAYDEV_HP	(1UL<<0)
#define AKPCM_PLAYDEV_LO	(1UL<<1)
#define AKPCM_PLAYDEV_AUTO	(0UL<<0)

/* cptr_dev */
#define AKPCM_CPTRDEV_MIC	(1UL<<0)
#define AKPCM_CPTRDEV_LI	(1UL<<1)
#define AKPCM_CPTRDEV_AUTO	(0UL<<0)

/* sample_bits */
#define AKPCM_SMPL_BIT_U8	(1UL<<0)
#define AKPCM_SMPL_BIT_U16	(1UL<<2)

/* rates */
#define AKPCM_RATE_8000			(1<<1)		/* 8000Hz */
#define AKPCM_RATE_11025		(1<<2)		/* 11025Hz */
#define AKPCM_RATE_16000		(1<<3)		/* 16000Hz */
#define AKPCM_RATE_22050		(1<<4)		/* 22050Hz */
#define AKPCM_RATE_32000		(1<<5)		/* 32000Hz */
#define AKPCM_RATE_44100		(1<<6)		/* 44100Hz */
#define AKPCM_RATE_48000		(1<<7)		/* 48000Hz */
#define AKPCM_RATE_CONTINUOUS	(1<<30)		/* continuous range */
#define AKPCM_RATE_KNOT			(1<<31)		/* more non-continuos rates */
#define AKPCM_RATE_ALL	(AKPCM_RATE_8000 | AKPCM_RATE_11025 | \
            AKPCM_RATE_16000 | AKPCM_RATE_22050 | AKPCM_RATE_32000 | \
            AKPCM_RATE_44100 | AKPCM_RATE_48000)

/* HP_IN  ADC23_IN */
#define SOURCE_DAC           	(0b100)
#define SOURCE_LINEIN			(0b010)
#define SOURCE_MIC           	(0b001)
#define SIGNAL_SRC_MUTE      	0
#define SIGNAL_SRC_MAX       	(SOURCE_DAC|SOURCE_LINEIN|SOURCE_MIC)

#define SOURCE_DAC_MASK 		(0b100)
#define SOURCE_LINEIN_MASK   	(0b010)
#define SOURCE_MIC_MASK     	(0b001)
#define SOURCE_MIXED_ALL_MASK 	(SOURCE_DAC_MASK|SOURCE_LINEIN_MASK|SOURCE_MIC_MASK)

/* H240 */
#define SOURCE_ADC_DAC      	(0b001)
#define SOURCE_ADC_LINEIN		(0b100)
#define SOURCE_ADC_MIC    		(0b010)
#define SIGNAL_ADC_SRC_MUTE 	0
#define SIGNAL_ADC_SRC_MAX		(SOURCE_ADC_DAC|SOURCE_ADC_LINEIN|SOURCE_ADC_MIC)

#define HEADPHONE_GAIN_MIN    	0
#define HEADPHONE_GAIN_MAX    	5
#define LINEIN_GAIN_MIN       	0
#define LINEIN_GAIN_MAX       	15
#define MIC_GAIN_MIN          	0
#define MIC_GAIN_MAX          	7

/* bit[0]-AEC; bit[1]-NR; bit[2]-AGC */
enum echo_bit {
    ECHO_BIT_AEC = 0x00,
    ECHO_BIT_NR,
    ECHO_BIT_AGC
};

struct akpcm_features {
	unsigned int rates;		    /* AKPCM_RATE_* */
	unsigned int actual_rate;   /* actual rate in Hz */
	unsigned int rate_min;		/* min rate */
	unsigned int rate_max;		/* max rate */
	unsigned int sample_bits;	/* AKPCM_FMTBIT_XX */
	unsigned int channels_min;	/* min channels */
	unsigned int channels_max;	/* max channels */
	unsigned int period_bytes_min;	/* min period size */
	unsigned int period_bytes_max;	/* max period size */
	unsigned int periods_min;   /* min # of periods */
	unsigned int periods_max;   /* max # of periods */
	unsigned int hp_gain_max;   /* max value for headphone gain */
	unsigned int li_gain_max;   /* max value for linein gain */
	unsigned int mic_gain_max;  /* max value for mic gain */
	unsigned int play_dev;      /* HP or(and) LO */
	unsigned int cptr_dev;      /* MIC or(and) LI */
};

struct akpcm_pars {
	/* -- HW parameters -- */
	unsigned int rate; /* rate in Hz */
	unsigned int channels;
	unsigned int sample_bits;
	unsigned int period_bytes; /* DMA size  */
	unsigned int periods; /* buffer size = (periods * period_bytes) */
	/* -- SW parameters -- */
	unsigned int threshold;
	unsigned int reserved; //stop_threshold;
};

#endif
