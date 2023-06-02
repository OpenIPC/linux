/*
 * mdrv_sound_io.h
 *
 *  Created on: Jul 12, 2016
 *      Author: trevor.wu
 */

#ifndef MDRV_SOUND_IO_H_
#define MDRV_SOUND_IO_H_


#define AUDIO_IOCTL_MAGIC               'S'

#define MDRV_SOUND_STARTTIME_READ		  _IOR(AUDIO_IOCTL_MAGIC, 0, unsigned long long)

#define IOCTL_AUDIO_MAXNR 0

#endif /* MDRV_SOUND_IO_H_ */
