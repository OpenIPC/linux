/*
 * Copyright (c) XMEDIA. All rights reserved.
 */

#ifndef FLASH_CACHE_H
#define FLASH_CACHE_H

#define FLASHCACHE_DISABLE          (0x01)
/* page has not be read, need read flash device */
#define FLASHCACHE_PAGE_UNKNOWN     (0x00)
/* page has be read, but it is empty */
#define FLASHCACHE_PAGE_EMPTY       (0x1000000)
/* page is bad bock, need read flash device */
#define FLASHCACHE_PAGE_BADBLOCK    (0x2000000)
/* page has cache, no need to read from flash device */
#define FLASHCACHE_PAGE_CACHE       (0x3000000)

static inline char *flash_cache_status(int status)
{
	char *str[] = {"unknown", "empty", "badblock", "cache"};
	return str[(status >> 24) & 0x3];
}

struct flash_cache {
	int oobsize;
	u32 max_pages;
	int pages_per_block;

	volatile bool enable;

	int (*peek_status)(struct flash_cache *cache, u32 pageindex);

	int (*get_status_lock_read_page)(struct flash_cache *cache, u32 pageindex);

	int (*unlock_read_page)(struct flash_cache *cache, u32 pageindex);

	int (*new_cache)(struct flash_cache *cache, u32 pageindex,
		char **pagebuf, char **oobbuf);

	int (*get_cache)(struct flash_cache *cache, u32 pageindex,
		char **pagebuf, char **oobbuf);

	int (*discard_cache)(struct flash_cache *cache, u32 pageindex,
		int nr_pages, int status);
};

struct flash_cache_create_arg {
	char *name;
	u64 totalsize;
	int blocksize;
	int pagesize;
	int oobsize;
	int nr_cache;
	void *read_ahead_args;

	int (*read_ahead)(void *arg, int pageindex, int *skip_pages);
};

struct flash_cache *flash_cache_create(struct flash_cache_create_arg *arg);

void flash_cache_destory(struct flash_cache *cache);
#endif
/******************************************************************************/

