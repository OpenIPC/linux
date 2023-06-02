#ifndef __FH_LINBA0_I2S_H
#define __FH_LINBA0_I2S_H

#define	FH_I2S_IOCTL_MEM_BASE	'M'
#define	I2S_INIT_CAPTURE_MEM    	       _IOWR(FH_I2S_IOCTL_MEM_BASE, 0, int)
#define	I2S_INIT_PLAYBACK_MEM           _IOWR(FH_I2S_IOCTL_MEM_BASE, 1, int)

#define	FH_I2S_IOCTL_PARAM_BASE	'P'
#define I2S_SET_VOL                     _IOWR(FH_I2S_IOCTL_PARAM_BASE, 0, int)
#define I2S_SET_INPUT_MODE              _IOWR(FH_I2S_IOCTL_PARAM_BASE, 1, int)
#define I2S_SET_OUTPUT_MODE             _IOWR(FH_I2S_IOCTL_PARAM_BASE, 2, int)

#define	FH_I2S_IOCTL_ENA_BASE	'E'
#define I2S_AI_EN                       _IOWR(FH_I2S_IOCTL_ENA_BASE, 0, int)
#define I2S_AO_EN                       _IOWR(FH_I2S_IOCTL_ENA_BASE, 1, int)
#define I2S_AI_DISABLE                  _IOWR(FH_I2S_IOCTL_ENA_BASE, 2, int)
#define I2S_AO_DISABLE       		   _IOWR(FH_I2S_IOCTL_ENA_BASE, 3, int)
#define I2S_AI_PAUSE               	   _IOWR(FH_I2S_IOCTL_ENA_BASE, 4, int)
#define I2S_AI_RESUME          		   _IOWR(FH_I2S_IOCTL_ENA_BASE, 5, int)
#define I2S_AO_PAUSE              	   _IOWR(FH_I2S_IOCTL_ENA_BASE, 6, int)
#define I2S_AO_RESUME          		   _IOWR(FH_I2S_IOCTL_ENA_BASE, 7, int)


struct fh_i2s_cfg_arg{

	int volume;
	int rate;
	int frame_bit;
	int channels;
	int buffer_size;
	int period_size;
};

#endif

