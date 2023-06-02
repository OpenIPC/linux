/*
 * es8328.h  --  ES8328 Soc Audio driver
 *
 * Copyright 2011 Ambarella Ltd.
 *
 * Author: Cao Rongrong <rrcao@ambarella.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __SOUND_ES8328_H
#define __SOUND_ES8328_H

struct es8328_platform_data {
	unsigned int	power_pin;
	unsigned int 	power_delay;
};

#endif

