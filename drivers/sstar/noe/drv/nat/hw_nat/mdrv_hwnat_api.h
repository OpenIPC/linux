/*
 * mdrv_hwnat_api.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */
#ifndef _MDRV_HWNAT_API_H
#define _MDRV_HWNAT_API_H

int MDrv_HWNAT_Get_Ppe_Entry_Idx(struct foe_pri_key *key, struct foe_entry *entry, int del);
int MDrv_HWNAT_Get_Mib_Entry_Idx(struct foe_pri_key *key, struct foe_entry *entry);
#endif /* _MDRV_HWNAT_API_H */
