/**
        @brief Sample code.\n

        @file dummy.c

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
#include "libfdt.h"

int gen_rc_profile(const char *path_dst, const char *path_dtb)
{
        FILE *fin = fopen(path_dtb, "rb");
        if (!fin) {
                fprintf(stderr, "unable to open %s.\n", path_dtb);
                return -1;
        }

        struct stat st;
        if(stat(path_dtb, &st) != 0) {
                fprintf(stderr, "unable to stat %s.\n", path_dtb);
                fclose(fin);
                return -1;
        }

        void *fdt = (void *)malloc(st.st_size);
        if (fdt == NULL) {
                fprintf(stderr, "memory is not enough.\n");
		fclose(fin);
                return -1;
        }

        size_t fdt_size = fread(fdt, 1, st.st_size, fin);
        if (fdt_size != st.st_size) {
                fprintf(stderr, "unable to read %s %d %d.\n", path_dtb, (int)fdt_size, (int)st.st_size);
                free(fdt);
                fclose(fin);
                return -1;
        }
        fclose(fin);

	FILE *fout = fopen(path_dst, "wt");
        if (!fout) {
                fprintf(stderr, "unable to create %s.\n", path_dst);
                free(fdt);
                return -1;
        }

	int len;
	int nodeoffset;
	nodeoffset = fdt_path_offset(fdt, "/nvt_info");
	if (nodeoffset != -FDT_ERR_NOTFOUND) {
		// other fields in nvt_info
		for (nodeoffset = fdt_first_property_offset(fdt, nodeoffset);
			(nodeoffset >= 0);
			(nodeoffset = fdt_next_property_offset(fdt, nodeoffset))) {
			const struct fdt_property *prop_reg;
			if (!(prop_reg = fdt_get_property_by_offset(fdt, nodeoffset, &len))) {
				break;
			}
			const char *name = fdt_string(fdt, fdt32_to_cpu(prop_reg->nameoff));
			const char *value = (const char *)prop_reg->data;
                        fprintf(fout, "export %s='%s'\n", name, value);
		}
	}

	free(fdt);
        fclose(fout);
        return 0;
}

MAINFUNC_ENTRY(dummy, argc, argv)
{
#if 0
        return gen_rc_profile("/mnt/sd/profile", "/sys/firmware/fdt");
#else
        if (argc < 3) {
                fprintf(stderr, "usage: rc_profile output_profile input_dtb\n");
                return -1;
        }
        return gen_rc_profile(argv[1], argv[2]);
#endif
}
