/*
 * /linux/arch/arm/mach-anycloud/ak_notifier.c
 *
 * Copyright (C) 2021 Anyka(Guangzhou) Microelectronics Technology Co., Ltd.
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
 */

#include <linux/kernel.h>
#include <linux/notifier.h>

static RAW_NOTIFIER_HEAD(ak_notifier_chain);

int register_ak_notifier(struct notifier_block *block)
{
    return raw_notifier_chain_register(&ak_notifier_chain, block);
}
EXPORT_SYMBOL(register_ak_notifier);

int unregister_ak_notifier(struct notifier_block *block)
{
    return raw_notifier_chain_unregister(&ak_notifier_chain, block);
}
EXPORT_SYMBOL(unregister_ak_notifier);

int ak_notifier_call_chain(unsigned long action, void *data)
{
    return raw_notifier_call_chain(&ak_notifier_chain, action, data);
}
EXPORT_SYMBOL(ak_notifier_call_chain);
