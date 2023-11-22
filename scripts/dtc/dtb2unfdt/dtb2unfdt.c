/*
* Copyright (C) 2018 Sigmastar Technology Corp.
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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

//#include <libfdt_env.h>
//#include <fdt.h>
#include <libfdt.h>
#include <util.h>

#include "dtb2unfdt.h"

//#include <uapi/asm-generic/errno-base.h>
#define	EINVAL		22	/* Invalid argument */

//===================
#define ALIGN(x, a)	(((x) + ((a) - 1)) & ~((a) - 1))
#define PALIGN(p, a)	((void *)(ALIGN((unsigned long)(p), (a))))
#define GET_CELL(p)	(p += 4, *((const uint32_t *)(p-4)))
#define PTR_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))  //linux/kernel.h

//===================
#define pr_warn printf
#define pr_debug
#define pr_warning printf
#define pr_err printf
#define WARN_ON_ONCE(x)  (x)

//===================
//convert be32_to_cpu/cpu_to_be32
#include <arpa/inet.h>
#define  cpu_to_be32(x) htonl((uint32_t )x)
#define  be32_to_cpu(x) ntohl((uint32_t )x)

//===================
#define of_compat_cmp(s1, s2, l)	strcasecmp((s1), (s2))
#define of_prop_cmp(s1, s2)		strcmp((s1), (s2))
#define of_node_cmp(s1, s2)		strcasecmp((s1), (s2))


static struct property *__of_find_property(const struct device_node *np,
		const char *name, int *lenp)
{
	struct property *pp;

	if (!np)
		return NULL;

	for (pp = np->properties; pp; pp = pp->next) {
		if (of_prop_cmp(pp->name, name) == 0) {
			if (lenp)
				*lenp = pp->length;
			break;
		}
	}

	return pp;
}
struct property *of_find_property(const struct device_node *np,
		const char *name,
		int *lenp)
{
	struct property *pp;
	//unsigned long flags;

	//raw_spin_lock_irqsave(&devtree_lock, flags);
	pp = __of_find_property(np, name, lenp);
	//raw_spin_unlock_irqrestore(&devtree_lock, flags);

	return pp;
}
const void *of_get_property(const struct device_node *np, const char *name,
		int *lenp)
{
	struct property *pp = of_find_property(np, name, lenp);

	return pp ? pp->value : NULL;
}
//=================
static void *unflatten_dt_alloc(void **mem, unsigned long size,
		unsigned long align)
{
	void *res;

	*mem = PTR_ALIGN(*mem, align);
	res = *mem;
	*mem += size;

	return res;
}

static void populate_properties(const void *blob,
		int offset,
		void **mem,
		struct device_node *np,
		const char *nodename,
		bool dryrun)
{
	struct property *pp, **pprev = NULL;
	int cur;
	bool has_name = false;

	pprev = &np->properties;
	for (cur = fdt_first_property_offset(blob, offset);
			cur >= 0;
			cur = fdt_next_property_offset(blob, cur)) {
		const __be32 *val;
		const char *pname;
		u32 sz;

		val = fdt_getprop_by_offset(blob, cur, &pname, &sz);
		if (!val) {
			pr_warn("Cannot locate property at 0x%x\n", cur);
			continue;
		}

		if (!pname) {
			pr_warn("Cannot find property name at 0x%x\n", cur);
			continue;
		}

		if (!strcmp(pname, "name"))
			has_name = true;

		pp = unflatten_dt_alloc(mem, sizeof(struct property),
				__alignof__(struct property));
		if (dryrun)
			continue;

		/* We accept flattened tree phandles either in
		 * ePAPR-style "phandle" properties, or the
		 * legacy "linux,phandle" properties.  If both
		 * appear and have different values, things
		 * will get weird. Don't do that.
		 */
		if (!strcmp(pname, "phandle") ||
				!strcmp(pname, "linux,phandle")) {
			if (!np->phandle)
				np->phandle = be32_to_cpu(*val);
		}

		/* And we process the "ibm,phandle" property
		 * used in pSeries dynamic device tree
		 * stuff
		 */
		if (!strcmp(pname, "ibm,phandle"))
			np->phandle = be32_to_cpu(*val);

		pp->name   = (char *)pname;
		pp->length = sz;
		pp->value  = (__be32 *)val;
		*pprev     = pp;
		pprev      = &pp->next;
	}

	/* With version 0x10 we may not have the name property,
	 * recreate it here from the unit name if absent
	 */
	if (!has_name) {
		const char *p = nodename, *ps = p, *pa = NULL;
		int len;

		while (*p) {
			if ((*p) == '@')
				pa = p;
			else if ((*p) == '/')
				ps = p + 1;
			p++;
		}

		if (pa < ps)
			pa = p;
		len = (pa - ps) + 1;
		pp = unflatten_dt_alloc(mem, sizeof(struct property) + len,
				__alignof__(struct property));
		if (!dryrun) {
			pp->name   = "name";
			pp->length = len;
			pp->value  = pp + 1;
			*pprev     = pp;
			pprev      = &pp->next;
			memcpy(pp->value, ps, len - 1);
			((char *)pp->value)[len - 1] = 0;
			pr_debug("fixed up name for %s -> %s\n",
					nodename, (char *)pp->value);
		}
	}

	if (!dryrun)
		*pprev = NULL;
}

static unsigned int populate_node(const void *blob,
		int offset,
		void **mem,
		struct device_node *dad,
		unsigned int fpsize,
		struct device_node **pnp,
		bool dryrun)
{
	struct device_node *np;
	const char *pathp;
	unsigned int l, allocl;
	int new_format = 0;

	pathp = fdt_get_name(blob, offset, &l);
	if (!pathp) {
		*pnp = NULL;
		return 0;
	}

	allocl = ++l;

	/* version 0x10 has a more compact unit name here instead of the full
	 * path. we accumulate the full path size using "fpsize", we'll rebuild
	 * it later. We detect this because the first character of the name is
	 * not '/'.
	 */
	if ((*pathp) != '/') {
		new_format = 1;
		if (fpsize == 0) {
			/* root node: special case. fpsize accounts for path
			 * plus terminating zero. root node only has '/', so
			 * fpsize should be 2, but we want to avoid the first
			 * level nodes to have two '/' so we use fpsize 1 here
			 */
			fpsize = 1;
			allocl = 2;
			l = 1;
			pathp = "";
		} else {
			/* account for '/' and path size minus terminal 0
			 * already in 'l'
			 */
			fpsize += l;
			allocl = fpsize;
		}
	}

	np = unflatten_dt_alloc(mem, sizeof(struct device_node) + allocl,
			__alignof__(struct device_node));
	if (!dryrun) {
		char *fn;
		//of_node_init(np);
		np->full_name = fn = ((char *)np) + sizeof(*np);
		if (new_format) {
			/* rebuild full path for new format */
			if (dad && dad->parent) {
				strcpy(fn, dad->full_name);
#ifdef DEBUG
				if ((strlen(fn) + l + 1) != allocl) {
					pr_debug("%s: p: %d, l: %d, a: %d\n",
							pathp, (int)strlen(fn),
							l, allocl);
				}
#endif
				fn += strlen(fn);
			}
			*(fn++) = '/';
		}
		memcpy(fn, pathp, l);

		if (dad != NULL) {
			np->parent = dad;
			np->sibling = dad->child;
			dad->child = np;
		}
	}

	populate_properties(blob, offset, mem, np, pathp, dryrun);
	if (!dryrun) {
		np->name = of_get_property(np, "name", NULL);
		np->type = of_get_property(np, "device_type", NULL);

		if (!np->name)
			np->name = "<NULL>";
		if (!np->type)
			np->type = "<NULL>";
	}

	*pnp = np;
	return fpsize;
}

static void reverse_nodes(struct device_node *parent)
{
	struct device_node *child, *next;

	/* In-depth first */
	child = parent->child;
	while (child) {
		reverse_nodes(child);

		child = child->sibling;
	}

	/* Reverse the nodes in the child list */
	child = parent->child;
	parent->child = NULL;
	while (child) {
		next = child->sibling;

		child->sibling = parent->child;
		parent->child = child;
		child = next;
	}
}

/**
 * unflatten_dt_nodes - Alloc and populate a device_node from the flat tree
 * @blob: The parent device tree blob
 * @mem: Memory chunk to use for allocating device nodes and properties
 * @dad: Parent struct device_node
 * @nodepp: The device_node tree created by the call
 *
 * It returns the size of unflattened device tree or error code
 */
static int unflatten_dt_nodes(const void *blob,
		void *mem,
		struct device_node *dad,
		struct device_node **nodepp)
{
	struct device_node *root;
	int offset = 0, depth = 0, initial_depth = 0;
#define FDT_MAX_DEPTH	64
	unsigned int fpsizes[FDT_MAX_DEPTH];
	struct device_node *nps[FDT_MAX_DEPTH];
	void *base = mem;
	bool dryrun = !base;

	if (nodepp)
		*nodepp = NULL;

	/*
	 * We're unflattening device sub-tree if @dad is valid. There are
	 * possibly multiple nodes in the first level of depth. We need
	 * set @depth to 1 to make fdt_next_node() happy as it bails
	 * immediately when negative @depth is found. Otherwise, the device
	 * nodes except the first one won't be unflattened successfully.
	 */
	if (dad)
		depth = initial_depth = 1;

	root = dad;
	fpsizes[depth] = dad ? strlen(of_node_full_name(dad)) : 0;
	nps[depth] = dad;

	for (offset = 0;
			offset >= 0 && depth >= initial_depth;
			offset = fdt_next_node(blob, offset, &depth)) {
		if (WARN_ON_ONCE(depth >= FDT_MAX_DEPTH))
			continue;

		fpsizes[depth+1] = populate_node(blob, offset, &mem,
				nps[depth],
				fpsizes[depth],
				&nps[depth+1], dryrun);
		if (!fpsizes[depth+1])
			return mem - base;

		if (!dryrun && nodepp && !*nodepp)
			*nodepp = nps[depth+1];
		if (!dryrun && !root)
			root = nps[depth+1];
	}

	if (offset < 0 && offset != -FDT_ERR_NOTFOUND) {
		pr_err("Error %d processing FDT\n", offset);
		return -EINVAL;
	}

	/*
	 * Reverse the child list. Some drivers assumes node order matches .dts
	 * node order
	 */
	if (!dryrun)
		reverse_nodes(root);

	return mem - base;
}

/**
 * __unflatten_device_tree - create tree of device_nodes from flat blob
 *
 * unflattens a device-tree, creating the
 * tree of struct device_node. It also fills the "name" and "type"
 * pointers of the nodes so the normal device-tree walking functions
 * can be used.
 * @blob: The blob to expand
 * @dad: Parent device node
 * @mynodes: The device_node tree created by the call
 * @dt_alloc: An allocator that provides a virtual address to memory
 * for the resulting tree
 *
 * Returns NULL on failure or the memory chunk containing the unflattened
 * device tree on success.
 */
static void *__unflatten_device_tree(const void *blob,
		struct device_node *dad,
		struct device_node **mynodes,
		bool detached, int* unfdt_size)
{
	int size;
	void *mem;

	pr_debug(" -> unflatten_device_tree()\n");

	if (!blob) {
		pr_debug("No device tree pointer\n");
		return NULL;
	}


	pr_err("blob size: 0x%x\n", fdt_totalsize(blob));
	pr_err("blob base: 0x%08x\n", (unsigned int)blob);
	pr_debug("  magic: 0x%08x\n", fdt_magic(blob));
	pr_debug("version: 0x%08x\n", fdt_version(blob));

	if (fdt_check_header(blob)) {
		pr_err("Invalid device tree blob header\n");
		return NULL;
	}

	/* First pass, scan for size */
	size = unflatten_dt_nodes(blob, NULL, dad, NULL);
	if (size < 0)
		return NULL;


	size = ALIGN(size, 4);
	pr_err("unflattened size: 0x%x\n", size);
	*unfdt_size = size;

	/* Allocate memory for the expanded device tree */
	mem = malloc(size + 4); //mem = dt_alloc(size + 4, __alignof__(struct device_node));
	if (!mem)
		return NULL;

	memset(mem, 0, size);

	*(__be32 *)(mem + size) = cpu_to_be32(0xdeadbeef);

	pr_err("unflattened base: 0x%08x\n", (unsigned int)mem);

	/* Second pass, do actual unflattening */
	unflatten_dt_nodes(blob, mem, dad, mynodes);
	if (be32_to_cpu(*(__be32 *)(mem + size)) != 0xdeadbeef)
		pr_warning("End of tree marker overwritten: %08x\n",
				be32_to_cpu(*(__be32 *)(mem + size)));

	if (detached && mynodes) {
		of_node_set_flag(*mynodes, OF_DETACHED);
		pr_debug("unflattened tree is detached\n");
	}

	pr_debug(" <- unflatten_device_tree()\n");
	return mem;
}


int main(int argc, char *argv[])
{
	int fd = 0;	/* assume stdin */
	char *blob_buf;
	int unfdt_size;
	void *unfdt_buf;
	struct device_node *of_root;
	char *input_name;
	char *output_name;
	FILE *outf = NULL;

	if (argc < 3) {
		printf("supply input filename\n");
		return 5;
	}

	input_name = argv[1];
	output_name = argv[2];
	printf ("%s: %s\n", " input", input_name);
	printf ("%s: %s\n", "output", output_name);
	blob_buf = (char *)utilfdt_read(input_name);

	unfdt_buf = __unflatten_device_tree(blob_buf, NULL, &of_root, false, &unfdt_size);
	free(blob_buf);

#if 1
	outf = fopen(output_name, "wb");
	if (! outf)
		printf("Couldn't open output file %s\n",    output_name);

	if (fwrite(&unfdt_buf, 4, 1, outf) != 1)
		printf("Error writing \n");

	if (fwrite(&blob_buf, 4, 1, outf) != 1)
		printf("Error writing \n");

	if (fwrite(unfdt_buf, unfdt_size, 1, outf) != 1)
		printf("Error writing \n");

	fclose(outf);
#endif

#if 0
	{
		char (*__kobject)[sizeof(struct kobject)];
		printf("%d", __kobject ); //36
	}
	{
		char (*__property)[sizeof(struct property)];
		printf("%d", __property );//target 52
	}
	{
		char (*__device_node)[sizeof(struct device_node)];
		printf("%d", __device_node );//144 target 88
	}
	{
		char (*__void_p)[sizeof(void	*)];
		printf("%d", __void_p );//144 target 88
	}
#endif
	return 0;
}
