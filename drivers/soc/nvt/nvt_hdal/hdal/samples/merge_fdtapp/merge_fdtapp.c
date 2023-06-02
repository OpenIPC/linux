/**
        @brief merge fdt.app in mtd form a dtb file.\n
        @author Niven Cho
        @ingroup mhdal
        @note Nothing.

        Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <stdio.h>
#include <stdlib.h>
#include <kwrap/cmdsys.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <mtd/mtd-user.h>
#include <sys/ioctl.h>
#include "libfdt.h"

#define CFG_DEBUG_OUTPUT_FILE "/mnt/sd/output.dtb"

int chk_file_exist(const char *filename)
{
	struct stat st;
	if (stat(filename, &st) != 0) {
		return -1;
	} else {
		return 0;
	}
}

int remove_dummy(char *str, int len)
{
	int i;
	for (i = 0; i < len; i++) {
		if (!isprint(str[i])) {
			str[i] = 0;
			return 0;
		}
	}
	return 0;
}

int get_partition_phy_size(int fd)
{
	mtd_info_t meminfo = {0};
	if (ioctl(fd, MEMGETINFO, &meminfo) != 0) {
		return -1;
	}
	return (int)meminfo.size;
}

int get_fdtapp_mtd_index(void)
{
	int i;
	char path[64] = {0};
	char name[32] = {0};
	// get mtd index of fdt.app
	for (i = 0; i < 100; i++) {
		sprintf(path, "/sys/class/mtd/mtd%d/name", i);
		if (chk_file_exist(path) != 0) {
			continue;
		}
		//check lable name
		FILE *fptr = fopen(path, "rt");
		if (fptr == NULL) {
			fprintf(stderr, "unable to open %s.\n", path);
			return -1;
		}
		if (fread(name, 1, sizeof(name), fptr) < 1) {
			fprintf(stderr, "failed to read %s.\n", path);
			fclose(fptr);
			return -1;
		}
		fclose(fptr);
		// because name include '\n'.
		remove_dummy(name, sizeof(name));
		if (strncmp(name, "fdt.app", 8) == 0) {
			return i;
		}
	}
	return -1;
}

int load_dtb_from_rootfs(unsigned char **pp_dtb, int *p_dtb_size, const char *path)
{
	struct stat st;
	unsigned char *p_dtb = NULL;

	if (chk_file_exist(path) != 0) {
		fprintf(stderr, "unable to open %s.\n", path);
		return -1;
	} else {
		int fd = open(path, O_RDONLY);
		if (fd < 0) {
			fprintf(stderr, "unable to open %s.\n", path);
			return -1;
		}
		
		if (stat(path, &st) != 0) {			
			fprintf(stderr, "unable to stat %s\n", path);
			close(fd);
			return -1;
		}
		
		p_dtb = (unsigned char *)malloc(st.st_size);
		if (p_dtb == NULL) {
			fprintf(stderr, "failed to alloc memory %d bytes for %s\n", st.st_size, path);
			close(fd);
			return -1;
		}

		if (read(fd, p_dtb, st.st_size) != st.st_size) {
			fprintf(stderr, "unable to read data from %s.\n", path);
			free(p_dtb);
			close(fd);
			return -1;
		}
		close(fd);

		if (fdt_check_full(p_dtb, st.st_size) != 0) {
			fprintf(stderr, "invalid fdt on %s.\n", path);
			free(p_dtb);
			return -1;
		}
	}
	*pp_dtb = p_dtb;
	*p_dtb_size = (int)st.st_size;
	return 0;
}

int load_fdt_from_mtd(unsigned char **pp_fdt, int *p_fdt_size, const char *path)
{
	unsigned char *p_fdt = NULL;

	if (chk_file_exist(path) != 0) {
		fprintf(stderr, "unable to open %s.\n", path);
		return -1;
	}

	int fd = open(path, O_RDONLY);
	if (fd < 0) {
		fprintf(stderr, "unable to open %s.\n", path);
		return -1;
	}

	//got mtd partition size
	int partition_size = get_partition_phy_size(fd);
	if (partition_size < 0) {
		fprintf(stderr, "unable to get partition size from %s.\n", path);
		close(fd);
		return -1;
	}

	p_fdt = (unsigned char *)malloc(partition_size);
	if (p_fdt == NULL) {
		fprintf(stderr, "failed to alloc memory %d bytes for %s\n", partition_size, path);
		close(fd);
		return -1;
	}
	if (read(fd, p_fdt, partition_size) != partition_size) {
		fprintf(stderr, "unable to read data from %s.\n", path);
		free(p_fdt);
		close(fd);
		return -1;
	}
	close(fd);

	//check fdt
	int er = fdt_check_full(p_fdt, partition_size);
	if (er != 0) {
		fprintf(stderr, "invalid fdt on %s.\n", path);
		free(p_fdt);
		return -1;
	}

	// enlarge the size to write more nodes
	fdt_set_totalsize(p_fdt, partition_size);

	*pp_fdt = p_fdt;
	*p_fdt_size = partition_size;
	return 0;
}

static int fdt_find_or_add_subnode(void *fdt, int parentoffset, const char *name)
{
	int offset;

	offset = fdt_subnode_offset(fdt, parentoffset, name);

	if (offset == -FDT_ERR_NOTFOUND) {
		offset = fdt_add_subnode(fdt, parentoffset, name);
	}

	if (offset < 0) {
		printf("%s: %s: %s\n", __func__, name, fdt_strerror(offset));
	}

	return offset;
}

static int merge_node(void *fdt_dst, void *fdt_src, const char *node_name)
{
	int er;
	int len;
	int child;
	char path[256] = { 0 };

	if (fdt_dst == NULL || fdt_src == NULL) {
		printf("merge_node: one of fdt is NULL\n");
		return -1;
	}

	int src_parent = fdt_path_offset(fdt_src, node_name);
	int dst_parent = fdt_path_offset(fdt_dst, node_name);

	if (dst_parent <= 0) {
		printf("unable to find %s on dst fdt.\n", node_name);
		return -1;
	}

	if (src_parent <= 0) {
		printf("unable to find %s on src fdt.\n", node_name);
		return -1;
	}

	fdt_for_each_property_offset(child, fdt_src, src_parent) {
		const struct fdt_property *fdt_prop = fdt_get_property_by_offset(fdt_src, child, &len);
		const char *name = fdt_string(fdt_src, fdt32_to_cpu(fdt_prop->nameoff));
		len = fdt32_to_cpu(fdt_prop->len);
		const void *nodep = fdt_prop->data;
		if ((er = fdt_setprop(fdt_dst, dst_parent, name, nodep, len)) != 0) {
			printf("failed to fdt_setprop %s, er = %d\n", name, er);
			return er;
		}
	}

	fdt_for_each_subnode(child, fdt_src, src_parent) {
		const char *name = fdt_get_name(fdt_src, child, &len);
		sprintf(path, "%s/%s", node_name, name);
		if (fdt_find_or_add_subnode(fdt_dst, dst_parent, name) <= 0) {
			return -1;
		}
		if ((er = merge_node(fdt_dst, fdt_src, path)) != 0) {
			return er;
		}
	}
	return 0;
}

int erase_mtd(const char *path, int size)
{
	int fd = open(path, O_RDWR);
	if (fd < 0) {
		fprintf(stderr, "unable to open %s.\n", path);
		return -1;
	}

	erase_info_t erase;
	erase.start = 0;
	erase.length = size;
	if (ioctl(fd, MEMERASE, &erase) < 0) {
		fprintf(stderr, "erase failed\n");
		close(fd);
		return -1;
	}

	close(fd);
	return 0;
}

int merge_fdtapp(const char *path_dtb)
{
	unsigned char *p_dtb = NULL;
	unsigned char *p_fdt = NULL;
	int dtb_size = 0, fdt_size = 0;

	// load source dtb
	if (load_dtb_from_rootfs(&p_dtb, &dtb_size, path_dtb) != 0) {
		return -1;
	}

	// get mtd index
	int mtd_idx = get_fdtapp_mtd_index();
	if (mtd_idx < 0) {
		fprintf(stderr, "unable to find fdt.app from mtd.\n");
		free(p_dtb);
		return -1;
	}

	// load target fdt
	char path_mtd[64] = {0};
	sprintf(path_mtd, "/dev/mtd%d", mtd_idx);
	if (load_fdt_from_mtd(&p_fdt, &fdt_size, path_mtd) != 0) {
		free(p_dtb);
		return -1;
	}

	// merge
	if (merge_node(p_fdt, p_dtb, "/fastboot") != 0) {
		fprintf(stderr, "failed to merge.\n");
		free(p_fdt);
		free(p_dtb);
		return -1;
	}

	//make it real size
	fdt_pack(p_fdt);

	// erase mtd before written
	if (erase_mtd(path_mtd, fdt_size) != 0) {
		free(p_fdt);
		free(p_dtb);
		return -1;
	}

	// write mtd
	int fd = open(path_mtd,  O_RDWR);
	int written = write(fd, p_fdt, fdt_size);
	printf("merge_fdtapp wrote %d bytes into %s\n", written, path_mtd);
	close(fd);

#if 0 //write to sd card
	fd = open(CFG_DEBUG_OUTPUT_FILE,  O_CREAT | O_WRONLY | O_SYNC);
	write(fd, p_fdt, fdt_totalsize(p_fdt));
	close(fd);
#endif
	free(p_fdt);
	free(p_dtb);

	return 0;
}

MAINFUNC_ENTRY(merge_fdtapp, argc, argv)
{
#if 0
	return merge_fdtapp("/etc/plugin_hdr.dtb");
#else
	if (argc < 2) {
		fprintf(stderr, "usage: merge_fdtapp [dtb]\n");
		return -1;
	}
	return merge_fdtapp(argv[1]);
#endif
}
