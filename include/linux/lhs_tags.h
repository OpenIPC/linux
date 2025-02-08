/******************************************************************************
* File:             lhs_tags.h
*
* Author:           Lynn
* Created:          12/07/22
* Description:      Param tag API and data structure
*****************************************************************************/
#ifndef _PARAM_ATAGS_H
#define _PARAM_ATAGS_H

void *early_atags_to_fdt(void *params);
int get_param_data(const char *name, char *buf, unsigned int buflen);
int set_param_data(const char *name, char *buf, unsigned int buflen);

int parse_lhs_tags(const struct tag *tag, void *fdt);

#endif /* _PARAM_ATAGS_H  */
