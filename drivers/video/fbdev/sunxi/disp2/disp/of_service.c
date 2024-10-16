/*
 *
 * Copyright (c) 2021 Allwinnertech Co., Ltd.
 * Author: libairong <libairong@allwinnertech.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include "of_service.h"

struct device_node *get_node_by_name(char *main_name)
{
	char compat[NAME_LENGTH];
	u32 len = 0;
	struct device_node *node;

	len = sprintf(compat, "allwinner,sunxi-%s", main_name);

	if (len > 32)
		printk("size of mian_name is out of range\n");

	node = of_find_compatible_node(NULL, NULL, compat);

	if (!node) {
		printk("of_find_compatible_node %s fail\n", compat);
		return NULL;
	}

	return node;
}
/**
 * get_property_by_name - retrieve a property from of by name
 */
struct property *get_property_by_name(struct device_node *node, char *property_name, int *property_length)
{
	struct property *pp;

	pp = of_find_property(node, property_name, property_length);
	if (pp == NULL) {
		printk("[DEBUG] pp is not found! pp name : %s \n", property_name);
		return NULL;
	}

	return pp;
}

/**
 * get_property_by_index - retrieve a property from of by index
 */
struct property *get_property_by_index(struct device_node *node, char *config_start, int index)
{
	struct property *pp;
	int i;
	int length;

	pp = get_property_by_name(node, config_start, &length);
	// printk("property index : %d \n", index);
	for (i = 0; pp; pp = pp->next, i++) {
		if (i == index - 1) {
			return pp;
		}
	}

	return NULL;

}

int insert_property(struct device_node *node, struct property **new_pp, char *reference_name, int index)
{
	struct property **pp;
	int i;

	pp = &node->properties;

	// Find start property by reference name
	while (*pp) {
		if (strcmp(reference_name, (*pp)->name) == 0) {
			break;
		}

		pp = &(*pp)->next;
	}

	if (pp == NULL) {
		return 1;
	}

	for (i = 0; i < index; i++, pp = &(*pp)->next) {
		if (i == (index - 1)) {
			// Insert
			(*new_pp)->next = (*pp)->next;
			(*pp)->next = (*new_pp);
			return 0;
		}
	}

	return 2;
}

/* FIXME，can not to call function, when build disp to be a module. */
#ifdef MODULE
int of_remove_property(struct device_node *np, struct property *prop)
{
	printk("[INFO] Can not to remove property in device tree, when disp was built to be a module \n");
	return -1;
}
#endif

int delete_property(struct device_node *np, struct property *prop)
{
	return of_remove_property(np, prop);
}

