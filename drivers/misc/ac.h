#ifndef __AC_H
#define __AC_H

/* #define _FHIOWR _IOWR */
#define _FHIOWR(x, y, z) y

#define	FH_AUDIO_IOCTL_BASE	'M'
#define	AC_INIT_CAPTURE_MEM         _FHIOWR(FH_AUDIO_IOCTL_BASE, 0,  int)
#define	AC_INIT_PLAYBACK_MEM        _FHIOWR(FH_AUDIO_IOCTL_BASE, 1,  int)
#define AC_SET_VOL                  _FHIOWR(FH_AUDIO_IOCTL_BASE, 2,  int)
#define AC_SET_INPUT_MODE           _FHIOWR(FH_AUDIO_IOCTL_BASE, 3,  int)
#define AC_SET_OUTPUT_MODE          _FHIOWR(FH_AUDIO_IOCTL_BASE, 4,  int)
#define AC_AI_EN                    _FHIOWR(FH_AUDIO_IOCTL_BASE, 5,  int)
#define AC_AO_EN                    _FHIOWR(FH_AUDIO_IOCTL_BASE, 6,  int)
#define AC_AI_DISABLE               _FHIOWR(FH_AUDIO_IOCTL_BASE, 7,  int)
#define AC_AO_DISABLE               _FHIOWR(FH_AUDIO_IOCTL_BASE, 8,  int)
#define AC_AI_PAUSE                 _FHIOWR(FH_AUDIO_IOCTL_BASE, 9,  int)
#define AC_AI_RESUME                _FHIOWR(FH_AUDIO_IOCTL_BASE, 10, int)
#define AC_AO_PAUSE                 _FHIOWR(FH_AUDIO_IOCTL_BASE, 11, int)
#define AC_AO_RESUME                _FHIOWR(FH_AUDIO_IOCTL_BASE, 12, int)
#define AC_AI_READ                  _FHIOWR(FH_AUDIO_IOCTL_BASE, 13, int)
#define AC_AO_WRITE                 _FHIOWR(FH_AUDIO_IOCTL_BASE, 14, int)
#define AC_AI_SET_VOL               _FHIOWR(FH_AUDIO_IOCTL_BASE, 15, int)
#define AC_AO_SET_VOL               _FHIOWR(FH_AUDIO_IOCTL_BASE, 16, int)
#define AC_AI_MICIN_SET_VOL         _FHIOWR(FH_AUDIO_IOCTL_BASE, 17, int)
#define AC_AEC_SET_CONFIG           _FHIOWR(FH_AUDIO_IOCTL_BASE, 18, int)
#define AC_NR_SET_CONFIG            _FHIOWR(FH_AUDIO_IOCTL_BASE, 19, int)
#define AC_NR2_SET_CONFIG           _FHIOWR(FH_AUDIO_IOCTL_BASE, 20, int)
#define AC_AGC_SET_CONFIG           _FHIOWR(FH_AUDIO_IOCTL_BASE, 21, int)
#define AC_WORK_MODE                _FHIOWR(FH_AUDIO_IOCTL_BASE, 22, int)
#define AC_AI_READ_FRAME_EXT        _FHIOWR(FH_AUDIO_IOCTL_BASE, 23, int)
#define AC_AO_SET_MODE              _FHIOWR(FH_AUDIO_IOCTL_BASE, 24, int)
#define AC_USING_EXTERNAL_CODEC     _FHIOWR(FH_AUDIO_IOCTL_BASE, 25, int)
#define AC_EXT_INTF                 _FHIOWR(FH_AUDIO_IOCTL_BASE, 100, int)

enum io_select {
	mic_in = 0,
	line_in = 1,
	speaker_out = 2,
	line_out = 3,
};

struct fh_audio_cfg_arg {
	int io_type;
	int volume;
	int rate;
	int frame_bit;
	int channels;
	int buffer_size;
	int period_size;
	int enc_type; /* encoding type */
};


struct fh_audio_ai_read_frame_ext {
	unsigned int  len;
	unsigned char *data;
	unsigned long long pts;
};

#endif
