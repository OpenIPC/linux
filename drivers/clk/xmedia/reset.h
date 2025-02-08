/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef	__XMEDIA_RESET_H
#define	__XMEDIA_RESET_H

struct device_node;
struct xmedia_reset_controller;

#ifdef CONFIG_RESET_CONTROLLER
int __init xmedia_reset_init(struct device_node *np, int nr_rsts);
#endif

#endif	/* __XMEDIA_RESET_H */
