#if defined(__KERNEL__)
#include <linux/slab.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif

#include "vg_log_core.h"

#define MAX_PRINTK 1023

#if defined(__KERNEL__)
#define PRINTF printk
#define MALLOC(x) kmalloc((x), GFP_ATOMIC)
#define FREE(x) kfree((x))
#else
#define PRINTF printf
#define MALLOC(x) malloc((x))
#define FREE(x) free((x))
#endif

extern unsigned int log_base_start;
extern unsigned int log_base_end;
extern unsigned int log_real_size;
extern unsigned int log_ksize;

void prepare_dump_console(unsigned int log_print_ptr, unsigned int size)
{
	unsigned int len = 0;
	char *kbuf = 0;

	kbuf = (char *)MALLOC(MAX_PRINTK + 1);
	if (kbuf == NULL) {
		return;
	}

	memset(kbuf, 0, MAX_PRINTK + 1);

	while (size > 0) {
		unsigned int pr = 0;
		while ((strlen((char *)log_print_ptr + pr) > MAX_PRINTK) && (pr < size)) {
			memcpy(kbuf, (void *)(log_print_ptr + pr), MAX_PRINTK);
			PRINTF("%s", kbuf);
			pr += MAX_PRINTK;
		}

		PRINTF("%s", (char *)(log_print_ptr + pr));
		len = strlen((char *)log_print_ptr + pr) + 1;

		size = size - len;
		if (log_print_ptr + len <= log_base_end) {
			log_print_ptr += len;
		} else {
			log_print_ptr = log_base_start + (len - (log_base_end - log_print_ptr));
		}

		if (log_print_ptr == log_base_end) {
			log_print_ptr = log_base_start;
		}
	}
	FREE(kbuf);
#if 0 //size is most 0 to quit while loop
	if (size != 0) {
		printk("\n[LOG]Error size(%d) len(%d) log_real_size(%d) log_bsize(%d) "
			   "log_print_ptr(0x%x) log_start_ptr(0x%x) log_base_start(0x%x) log_base_end(0x%x)\n",
			   size, len, log_real_size, log_bsize, log_print_ptr, log_start_ptr,
			   log_base_start, log_base_end);
	}
#endif
}

int log_init_mem(void)
{
	log_bsize = log_ksize * SZ_1K;
	log_base_start = log_start_ptr = (unsigned int)MALLOC(log_bsize);
	log_base_end = log_base_start + log_bsize;
	mmap_msg = MALLOC(MMAP_MSG_LEN);
	memset(mmap_msg, 0, MMAP_MSG_LEN);
	return 0;
}

int log_uninit_mem(void)
{
	if (mmap_msg) {
		FREE(mmap_msg);
	}
	FREE((void *)log_base_start);
	return 0;
}
