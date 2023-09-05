/*
* hal_card_paltform_config.h - Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: joe.su <joe.su@sigmastar.com.tw>
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

#define EN_DEV_TREE_SUP            (TRUE)

#define D_IP1_IP                   EV_IP_FCIE1                    //SDIO
#define D_IP1_PORT                 EV_PFCIE5_SDIO                 //Port Setting for FCIE5 (SDIO)

#define D_IP2_IP                   EV_IP_FCIE2                    //FCIE

#define D_IP2_PORT                 EV_PFCIE5_SDIO                 //Port Setting for FCIE5 (SDIO)

#define D_IP3_IP                   EV_IP_FCIE3                    //FCIE
#define D_IP3_PORT                 EV_PFCIE5_FCIE                 //Port Setting for FCIE5 (FCIE)


#define WT_POWERUP                 20 //(ms)
#define WT_POWERON                 60 //(ms)
#define WT_POWEROFF                80 //(ms)


#define D_SDMMC1_MUTEX             EV_MUTEX1
#define D_SDMMC2_MUTEX             EV_MUTEX2
#define D_SDMMC3_MUTEX             EV_MUTEX3

#define EV_SDMMC1_DOWN_LVL         (FALSE)
#define EV_SDMMC2_DOWN_LVL         (FALSE)
#define EV_SDMMC3_DOWN_LVL         (FALSE)

#define EV_SDMMC1_SDIO_IRQ         (TRUE)
#define EV_SDMMC2_SDIO_IRQ         (TRUE)
#define EV_SDMMC3_SDIO_IRQ         (FALSE)

#define EV_SDMMC1_SDIO_PRT         (FALSE)
#define EV_SDMMC2_SDIO_PRT         (FALSE)
#define EV_SDMMC3_SDIO_PRT         (FALSE)

#include "../../../sstar/include/infinity2m/irqs.h"

#define V_IP1_MIEIRQ               (INT_IRQ_SDIO+32)
#define V_IP2_MIEIRQ               0//(INT_IRQ_FCIE+32)
#define V_IP3_MIEIRQ               0

#define V_IP_MIEIRQ_PARA           IRQF_SHARED //|IRQF_DISABLED

#define V_PAD1_CDZIRQ              (INT_FIQ_SD_CDZ+32)
#define V_PAD2_CDZIRQ              0//(INT_PMSLEEP_GPIO_7+160)
#define V_PAD3_CDZIRQ              0

#define V_PAD1_PWRGPIO             (19)
#define V_PAD2_PWRGPIO             0
#define V_PAD3_PWRGPIO             0

#define V_PAD1_CDZIRQ_PARA         IRQF_SHARED //|IRQF_DISABLED
#define V_PAD2_CDZIRQ_PARA         IRQF_SHARED //|IRQF_DISABLED
#define V_PAD3_CDZIRQ_PARA         IRQF_SHARED //|IRQF_DISABLED

#define EN_PAD1_CDZIRQ_SHARD       (FALSE)
#define EN_PAD2_CDZIRQ_SHARD       (FALSE)
#define EN_PAD3_CDZIRQ_SHARD       (FALSE)
#define EN_PAD1_CDZIRQ_WAKEUP      (FALSE)
#define EN_PAD2_CDZIRQ_WAKEUP      (FALSE)
#define EN_PAD3_CDZIRQ_WAKEUP      (FALSE)

#define WT_EVENT_RSP                10      //(ms)
#define WT_EVENT_READ               2000    //(ms)
#define WT_EVENT_WRITE              3000    //(ms)

#define EN_MSYS_REQ_DMEM            (FALSE)

