/*
 * Copyright (c) Hunan Goke,Chengdu Goke,Shandong Goke. 2021. All rights reserved.
 */

#ifndef	__GOKE_RESET_H
#define	__GOKE_RESET_H

struct device_node;
struct gk_reset_controller;

#ifdef CONFIG_RESET_CONTROLLER
int __init gk_reset_init(struct device_node *np, int nr_rsts);
#endif

#endif	/* __GOKE_RESET_H */
