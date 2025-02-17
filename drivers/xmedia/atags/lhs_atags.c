/******************************************************************************
* File:             lhs_atags.c
*
* Author:           Lynn
* Created:          12/08/22
* Description:      LHS ATAGs parsing, this file setup a tag table of all
*                   lhs atags, and the function of parsing these atags.
*****************************************************************************/

#define pr_fmt(fmt) "Param Tag: " fmt

#include <asm/setup.h>
#include <linux/kernel.h>
#include <linux/stddef.h>
#include <linux/string.h>
#include <linux/export.h>

/****************************************************************************/
extern struct tagtable_lhs g_tag_param;

/* Tagtable defined by lhs */
static struct tagtable_lhs *tagtable[] __initdata = {
        &g_tag_param,
        NULL,
};

/****************************************************************************/
int __init parse_lhs_tags(const struct tag *tag, void *fdt)
{
        struct tagtable_lhs **tagtbl = tagtable;

        while (*tagtbl) {
                if ((*tagtbl)->tag == tag->hdr.tag)
                        return (*tagtbl)->parse(tag, fdt);
                tagtbl++;
        }

        return 0;
}
