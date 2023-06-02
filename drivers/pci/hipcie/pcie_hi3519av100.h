/*
 * Copyright (c) 2017-2018 HiSilicon Technologies Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __HISI_PCIE_H__
#define __HISI_PCIE_H__

#define MISC_CTRL_BASE      0x04528000
#define PCIE_MEM_BASE       0x18000000
#define PCIE_EP_CONF_BASE   0x10000000
#define PCIE_DBI_BASE       0x0EFF0000
#define PERI_CRG_BASE       0x04510000

#define PERI_CRG99      0x18C
#define PCIE_X2_SRST_REQ    2

#define PCIE_X2_AUX_CKEN    7
#define PCIE_X2_PIPE_CKEN   6
#define PCIE_X2_SYS_CKEN    5
#define PCIE_X2_BUS_CKEN    4
#define PCIE_PAD_OE_MASK    (0x7 << 8)

#define PCIE_SYS_CTRL0      0xC00
#define PCIE_DEVICE_TYPE    28
#define PCIE_WM_EP      0x0
#define PCIE_WM_LEGACY      0x1
#define PCIE_WM_RC      0x4

#define PCIE_SYS_CTRL7      0xC1C
#define PCIE_APP_LTSSM_ENBALE   11
#define PCIE_ACCESS_ENABLE  13

#define PCIE_SYS_STATE0     0xF00
#define PCIE_XMLH_LINK_UP   15
#define PCIE_RDLH_LINK_UP   5

#define PCIE_IRQ_INTA       102
#define PCIE_IRQ_INTB       103
#define PCIE_IRQ_INTC       104
#define PCIE_IRQ_INTD       105
#define PCIE_IRQ_EDMA       106
#define PCIE_IRQ_MSI        107
#define PCIE_IRQ_LINK_DOWN  108

#define PCIE_INTA_PIN       1
#define PCIE_INTB_PIN       2
#define PCIE_INTC_PIN       3
#define PCIE_INTD_PIN       4


#endif
