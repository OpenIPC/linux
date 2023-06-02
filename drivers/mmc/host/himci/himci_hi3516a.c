/*
 * Copyright (c) 2016 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General Public License as published by the
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
#define MMC_CRG_MIN		25000000

#define TUNING_START_PHASE	0
#define TUNING_END_PHASE	7
#define HIMCI_PHASE_SCALE	8
#define DRV_PHASE_DFLT		(0x2<<23)
#define SMPL_PHASE_DFLT		(0x3<<16)

#define REG_PAD_CTRL			0x200f0800

#define REG_CTRL_SDIO0_CLK		0xcc
#define REG_CTRL_SDIO0_CMD		0xdc
#define REG_CTRL_SDIO0_DATA0	0xe0
#define REG_CTRL_SDIO0_DATA1	0xe4
#define REG_CTRL_SDIO0_DATA2	0xe8
#define REG_CTRL_SDIO0_DATA3	0xec

#define REG_CTRL_SDIO1_CLK		0x104
#define REG_CTRL_SDIO1_CMD		0x114
#define REG_CTRL_SDIO1_DATA0	0x118
#define REG_CTRL_SDIO1_DATA1	0x11c
#define REG_CTRL_SDIO1_DATA2	0x120
#define REG_CTRL_SDIO1_DATA3	0x124

#define SDIO_CLK_DS_3V3			0x60
#define SDIO_CMD_DS_3V3			0xe0
#define SDIO_DATA0_DS_3V3		0xe0
#define SDIO_DATA1_DS_3V3		0xe0
#define SDIO_DATA2_DS_3V3		0xe0
#define SDIO_DATA3_DS_3V3		0xe0

#define SDIO_CLK_DS_1V8			0x40
#define SDIO_CMD_DS_1V8			0xd0
#define SDIO_DATA0_DS_1V8		0xd0
#define SDIO_DATA1_DS_1V8		0xd0
#define SDIO_DATA2_DS_1V8		0xd0
#define SDIO_DATA3_DS_1V8		0xd0

struct sdio_drv_cap {
	unsigned int reg_addr;
	unsigned int ds_3v3;
	unsigned int ds_1v8;
};

#define SDIO_DRV_CAP(ofst, v1, v2) { \
	.reg_addr = ofst, \
	.ds_3v3 = v1, \
	.ds_1v8 = v2}
static  struct sdio_drv_cap sdio_ds[] = {
	SDIO_DRV_CAP(REG_CTRL_SDIO0_CLK, SDIO_CLK_DS_3V3, SDIO_CLK_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO0_CMD, SDIO_CMD_DS_3V3, SDIO_CMD_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO0_DATA0, SDIO_DATA0_DS_3V3, SDIO_DATA0_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO0_DATA1, SDIO_DATA1_DS_3V3, SDIO_DATA1_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO0_DATA2, SDIO_DATA2_DS_3V3, SDIO_DATA2_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO0_DATA3, SDIO_DATA3_DS_3V3, SDIO_DATA3_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO1_CLK, SDIO_CLK_DS_3V3, SDIO_CLK_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO1_CMD, SDIO_CMD_DS_3V3, SDIO_CMD_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO1_DATA0, SDIO_DATA0_DS_3V3, SDIO_DATA0_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO1_DATA1, SDIO_DATA1_DS_3V3, SDIO_DATA1_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO1_DATA2, SDIO_DATA2_DS_3V3, SDIO_DATA2_DS_1V8),
	SDIO_DRV_CAP(REG_CTRL_SDIO1_DATA3, SDIO_DATA3_DS_3V3, SDIO_DATA3_DS_1V8),
};

static void himci_set_drv_cap(struct himci_host *host, unsigned int vdd_180)
{
	unsigned int i, offset;

	offset = host->devid * 6;
	for (i = 0; i < 6; i++) {
		if (vdd_180)
			himci_writel(sdio_ds[i + offset].ds_1v8, 
				IO_ADDRESS(REG_PAD_CTRL + sdio_ds[i + offset].reg_addr));
		else
			himci_writel(sdio_ds[i + offset].ds_3v3, 
				IO_ADDRESS(REG_PAD_CTRL + sdio_ds[i + offset].reg_addr));
	}
}
