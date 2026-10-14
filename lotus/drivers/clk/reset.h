/* SPDX-License-Identifier: GPL-2.0 */

#ifndef	__LOTUS_RESET_H
#define	__LOTUS_RESET_H

struct device_node;
struct lotus_reset_controller;

#ifdef CONFIG_RESET_CONTROLLER
int __init lotus_reset_init(struct device_node *np, int nr_rsts);
#endif

#endif	/* __LOTUS_RESET_H */
