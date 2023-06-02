/*
 * Copyright (c) 2016-2017 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

enum {
	/* hisi extended global controller registers */
	PHY_CTL0    = 0xA0,
	PHY_CTL1    = 0xA4,
	PHY_RST_BACK_MASK	= 0xAC,
	PHY_CTL2	= 0xB0,

#define PHY_DATA_INVERT     (0x1 << 3)
#define PHY0_RST_MASK		(0x1 << 4)
#define PHY_RST_MASK_ALL	(0xF << 4)

	/* hisi extended registers for each SATA port */
	PORT_FIFOTH		= 0x44,
	PORT_PHYCTL1    = 0x48,
	PORT_PHYCTL     = 0x74,

#define PHY_MODE_1_5G   0
#define PHY_MODE_3G     1
#define PHY_MODE_6G     2
};
