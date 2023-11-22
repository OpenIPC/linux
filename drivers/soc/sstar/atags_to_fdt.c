/*
* atags_to_fdt.c- Sigmastar
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
#include "setup.h"
#include "libfdt.h"

#define do_extend_cmdline 0

static int node_offset(void *fdt, const char *node_path)
{
    int offset = fdt_path_offset(fdt, node_path);
    if (offset == -FDT_ERR_NOTFOUND)
        offset = fdt_add_subnode(fdt, 0, node_path);
    return offset;
}

static int setprop_string(void *fdt, const char *node_path,
              const char *property, const char *string)
{
    int offset = node_offset(fdt, node_path);
    if (offset < 0)
        return offset;
    return fdt_setprop_string(fdt, offset, property, string);
}

static int setprop_cell(void *fdt, const char *node_path,
            const char *property, uint32_t val)
{
    int offset = node_offset(fdt, node_path);
    if (offset < 0)
        return offset;
    return fdt_setprop_cell(fdt, offset, property, val);
}

static const void *getprop(const void *fdt, const char *node_path,
               const char *property, int *len)
{
    int offset = fdt_path_offset(fdt, node_path);

    if (offset == -FDT_ERR_NOTFOUND)
        return NULL;

    return fdt_getprop(fdt, offset, property, len);
}

static void merge_fdt_bootargs(void *fdt, const char *fdt_cmdline)
{
    char cmdline[COMMAND_LINE_SIZE];
    const char *fdt_bootargs;
    char *ptr = cmdline;
    int len = 0;

    /* copy the fdt command line into the buffer */
    fdt_bootargs = getprop(fdt, "/chosen", "bootargs", &len);
    if (fdt_bootargs)
        if (len < COMMAND_LINE_SIZE) {
            memcpy(ptr, fdt_bootargs, len);
            /* len is the length of the string
             * including the NULL terminator */
            ptr += len - 1;
        }

    /* and append the ATAG_CMDLINE */
    if (fdt_cmdline) {
        len = strlen(fdt_cmdline);
        if (ptr - cmdline + len + 2 < COMMAND_LINE_SIZE) {
            *ptr++ = ' ';
            memcpy(ptr, fdt_cmdline, len);
            ptr += len;
        }
    }
    *ptr = '\0';

    setprop_string(fdt, "/chosen", "bootargs", cmdline);
}

/*
 * Convert and fold provided ATAGs into the provided FDT.
 *
 * REturn values:
 *    = 0 -> pretend success
 *    = 1 -> bad ATAG (may retry with another possible ATAG pointer)
 *    < 0 -> error from libfdt
 */
int early_atags_to_fdt(void *atag_list, void *fdt, int total_space)
{
    struct tag *atag = atag_list;

    int ret;

    /* make sure we've got an aligned pointer */
    if (((unsigned long)atag_list & 0x3) || (NULL == atag_list))
	    return 1;

    /* if we get a DTB here we're done already */
    if (*(unsigned long *)atag_list == fdt32_to_cpu(FDT_MAGIC))
           return 0;

    /* validate the ATAG */
    if (atag->hdr.tag != ATAG_CORE ||
        (atag->hdr.size != tag_size(tag_core) &&
         atag->hdr.size != 2))
        return 1;

    /* let's give it all the room it could need */
    ret = fdt_open_into(fdt, fdt, total_space);
    if (ret < 0)
        return ret;

    for_each_tag(atag, atag_list) {
        if (atag->hdr.tag == ATAG_CMDLINE) {
            /* Append the ATAGS command line to the device tree
             * command line.
             * NB: This means that if the same parameter is set in
             * the device tree and in the tags, the one from the
             * tags will be chosen.
             */
            if (do_extend_cmdline)
                merge_fdt_bootargs(fdt,
                           atag->u.cmdline.cmdline);
            else
                setprop_string(fdt, "/chosen", "bootargs",
                           atag->u.cmdline.cmdline);
        }
        else if (atag->hdr.tag == ATAG_INITRD2) {
            uint32_t initrd_start, initrd_size;
            initrd_start = atag->u.initrd.start;
            initrd_size = atag->u.initrd.size;
            setprop_cell(fdt, "/chosen", "linux,initrd-start",
                    initrd_start);
            setprop_cell(fdt, "/chosen", "linux,initrd-end",
                    initrd_start + initrd_size);
        }
    }
    return fdt_pack(fdt);
}
