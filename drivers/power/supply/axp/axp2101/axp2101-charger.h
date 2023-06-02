/*
 * drivers/power/axp/axp2101/axp2101-charger.h
 * (C) Copyright 2010-2016
 * Allwinner Technology Co., Ltd. <www.allwinnertech.com>
 * Pannan <pannan@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 */

#ifndef AXP2101_CHARGER_H
#define AXP2101_CHARGER_H
#include "axp2101.h"

#define AXP2101_CHARGER_ENABLE            (1 << 1)
#define AXP2101_FAULT_LOG_COLD            (1 << 0)
#define AXP2101_FAULT_LOG_BATINACT        (1 << 2)
#define AXP2101_FAULT_LOG_OVER_TEMP       (1 << 1)
#define AXP2101_COULOMB_ENABLE            (1 << 3)

#define AXP2101_ADC_BATVOL_ENABLE         (1 << 0)
#define AXP2101_ADC_USBVOL_ENABLE         (1 << 2)
#define AXP2101_ADC_APSVOL_ENABLE         (1 << 3)
#define AXP2101_ADC_TSVOL_ENABLE          (1 << 1)
#define AXP2101_ADC_INTERTEM_ENABLE       (1 << 4)

#endif /* AXP2101_CHARGER_H */
