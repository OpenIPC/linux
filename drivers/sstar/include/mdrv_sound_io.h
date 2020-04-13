/*
* mdrv_sound_io.h- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
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
