#include "nand_secure_storage.h"
#include "sst_crc.h"
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include "rawnand/rawnand_cfg.h"
#include "rawnand/rawnand_debug.h"

#define SUNXI_SECSTORE_VERSION  1
/* Store element struct in nand/emmc */
#define MAX_STORE_LEN 0xc00	/*3K payload */
#define STORE_OBJECT_MAGIC  0x17253948
#define STORE_REENCRYPT_MAGIC 0x86734716
#define STORE_WRITE_PROTECT_MAGIC 0x8ad3820f
typedef struct {
	unsigned int magic;	/* store obj magic */
	int id;			/*store id, 0x01,0x02.. for user */
	char name[64];		/*OEM name */
	unsigned int re_encrypt;	/*flag for OEM obj */
	unsigned int version;
	/*can be placed or not, =0, can be write_protectd */
	unsigned int write_protect;
	unsigned int reserved[3];
	unsigned int actual_len;	/*the actual len in data buffer */
	unsigned char data[MAX_STORE_LEN];	/*the payload of sec obj */
	int crc;		/*crc to check the sotre_objce valid */
} store_object_t;

#define SEC_BLK_SIZE                        (4096)
#define MAX_SECURE_STORAGE_MAX_ITEM         (32)
#define MAP_KEY_NAME_SIZE                   (64)
#define MAP_KEY_DATA_SIZE                   (32)

struct map_info {
	unsigned char data[SEC_BLK_SIZE - sizeof(int) * 2];
	unsigned int magic;
	int crc;
} secure_storage_map;

static unsigned int secure_storage_inited;

static unsigned int map_dirty;
static inline void set_map_dirty(void)
{
	map_dirty = 1;
}

static inline void clear_map_dirty(void)
{
	map_dirty = 0;
}

static inline int try_map_dirty(void)
{
	return map_dirty;
}

static int check_secure_storage_map(void *buffer)
{
	struct map_info *map_buf = buffer;

	if (map_buf->magic != STORE_OBJECT_MAGIC) {
		RAWNAND_ERR("Item0 (Map) magic is bad\n");
		return 2;
	}
	if (map_buf->crc !=
	    sst_crc32(0, (void *)map_buf, sizeof(struct map_info) - 4)) {
		RAWNAND_ERR("Item0 (Map) crc is fail [0x%x]\n", map_buf->crc);
		return -1;
	}
	return 0;
}

/*nand secure storage read/write*/
static int _nand_read(int id, unsigned char *buf, ssize_t len)
{
	if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
		RAWNAND_ERR("out of range id %x\n", id);
		return -1;
	}

	return nand_secure_storage_read(id, buf, len);
}

static int _nand_write(int id, unsigned char *buf, ssize_t len)
{
	if (id > MAX_SECURE_STORAGE_MAX_ITEM) {
		RAWNAND_ERR("out of range id %x\n", id);
		return -1;
	}

	return nand_secure_storage_write(id, buf, len);
}

static int nv_read(int id, unsigned char *buf, ssize_t len)
{
	int ret;

	ret = _nand_read(id, buf, len);

	return ret;
}

static int nv_write(int id, unsigned char *buf, ssize_t len)
{
	int ret;

	ret = _nand_write(id, buf, len);

	return ret;
}

/*Low-level operation*/
static int sunxi_secstorage_read(int item, unsigned char *buf, unsigned int len)
{
	return nv_read(item, buf, len);
}

static int sunxi_secstorage_write(int item, unsigned char *buf,
				  unsigned int len)
{
	return nv_write(item, buf, len);
}

/*
 * Map format:
 *      name:length\0
 *      name:length\0
 */
static int __probe_name_in_map(unsigned char *buffer, const char *item_name,
			       int *len)
{
	unsigned char *buf_start = buffer;
	int index = 1;
	char name[MAP_KEY_NAME_SIZE], length[MAP_KEY_DATA_SIZE];
	unsigned int i, j;

	while (*buf_start != '\0') {
		memset(name, 0, MAP_KEY_NAME_SIZE);
		memset(length, 0, MAP_KEY_DATA_SIZE);
		i = 0;
		while (buf_start[i] != ':' &&
			(buf_start[i] != '\0') &&
			(&buf_start[i] - buffer) < SEC_BLK_SIZE &&
			i < sizeof(name)) {
			name[i] = buf_start[i];
			i++;
		}

		if (i >= sizeof(name))
			return -1;

		i++;
		j = 0;
		while ((buf_start[i] != ' ') &&
				(buf_start[i] != '\0') &&
				(&buf_start[i] - buffer) < SEC_BLK_SIZE &&
				j < sizeof(length)) {
			length[j] = buf_start[i];
			i++;
			j++;
		}

		/* deal rubbish data */
		if ((&buf_start[i] - buffer) >= SEC_BLK_SIZE
		    || j >= sizeof(length)) {
			return -1;
		}

		if (memcmp(item_name, name, strlen(item_name)) == 0) {
			buf_start += strlen(item_name) + 1;
			if (kstrtoint((const char *)length, 10, len) == 0)
				return index;
			else
				return -1;
		}
		index++;
		buf_start += strlen((const char *)buf_start) + 1;
	}

	return -1;
}

static int __fill_name_in_map(unsigned char *buffer, const char *item_name,
			      int length)
{
	unsigned char *buf_start = buffer;
	int index = 1;
	int name_len;

	while (*buf_start != '\0' &&
			(buf_start - buffer) < SEC_BLK_SIZE) {
		name_len = 0;
		while (buf_start[name_len] != ':' &&
				(&buf_start[name_len] - buffer) < SEC_BLK_SIZE &&
				name_len < MAP_KEY_NAME_SIZE)
			name_len++;

		/* deal rubbish data */
		if ((&buf_start[name_len] - buffer) >= SEC_BLK_SIZE
		    || name_len >= MAP_KEY_NAME_SIZE) {
			RAWNAND_ERR("dirty map, memset 0\n");
			memset(buffer, 0x0, SEC_BLK_SIZE);
			buf_start = buffer;
			index = 1;
			break;
		}

		if (!memcmp
		    ((const char *)buf_start, item_name, strlen(item_name))) {
			RAWNAND_DBG("name in map %s\n", buf_start);
			return index;
		}
		index++;
		buf_start += strlen((const char *)buf_start) + 1;
	}
	if (index >= 32)
		return -1;

	sprintf((char *)buf_start, "%s:%d", item_name, length);

	return index;
}

/*load source data to secure_object struct
 *
 * src      : secure_object
 * len      : secure_object buffer len
 * payload  : taregt payload
 * retLen   : target payload actual length
 */
static int unwrap_secure_object(void *src, unsigned int len, void *payload,
				int *retLen)
{
	store_object_t *obj;

	if (len != sizeof(store_object_t)) {
		RAWNAND_DBG("Input len not equal sec obj size 0x%x\n", len);
		return -1;
	}

	obj = (store_object_t *) src;

	if (obj->magic != STORE_OBJECT_MAGIC) {
		RAWNAND_ERR("Input obj magic fail [0x%x]\n", obj->magic);
		return -1;
	}

	if (obj->re_encrypt == STORE_REENCRYPT_MAGIC)
		RAWNAND_DBG("sec obj is encrypt by chip\n");

	if (obj->crc != sst_crc32(0, (void *)obj, sizeof(*obj) - 4)) {
		RAWNAND_ERR("Input obj crc fail [0x%x]\n", obj->crc);
		return -1;
	}

	memcpy(payload, obj->data, obj->actual_len);
	*retLen = obj->actual_len;

	return 0;
}

/*Store source data to secure_object struct
 *
 * src      : payload data to secure_object
 * name     : input payloader data name
 * tagt     : taregt secure_object
 * len      : input payload data length
 * retLen   : target secure_object length
 */
static int wrap_secure_object(void *src, const char *name,
		unsigned int len, void *tagt, int *retLen, int write_protect)
{
	store_object_t *obj;

	if (len > MAX_STORE_LEN) {
		RAWNAND_ERR("Input len larger then sec obj payload size\n");
		return -1;
	}

	obj = (store_object_t *) tagt;
	*retLen = sizeof(store_object_t);

	obj->magic = STORE_OBJECT_MAGIC;
	strncpy(obj->name, name, 64);
	obj->re_encrypt = 0;
	obj->version = SUNXI_SECSTORE_VERSION;
	obj->id = 0;
	obj->write_protect =
		(write_protect == 0) ? 0:STORE_WRITE_PROTECT_MAGIC;
	memset(obj->reserved, 0, 4);
	obj->actual_len = len;
	if (src != NULL)
		memcpy(obj->data, src, len);

	obj->crc = sst_crc32(0, (void *)obj, sizeof(*obj) - 4);

	return 0;
}

static int sunxi_secure_storage_read(const char *item_name,
		char *buffer, int buffer_len, int *data_len)
{
	int ret, index;
	int len_in_store;
	unsigned char *buffer_to_sec;

	if (!secure_storage_inited) {
		RAWNAND_ERR("%s: secure storage has not been inited\n",
				__func__);
		return -1;
	}

	buffer_to_sec = kmalloc(SEC_BLK_SIZE, GFP_KERNEL);
	if (!buffer_to_sec) {
		RAWNAND_ERR("%s out of memory", __func__);
		return -1;
	}

	index =
	    __probe_name_in_map(secure_storage_map.data, item_name,
				&len_in_store);
	if (index < 0) {
		RAWNAND_ERR("no item name %s in the map\n", item_name);
		kfree(buffer_to_sec);
		return -1;
	}
	memset(buffer, 0, buffer_len);
	ret = sunxi_secstorage_read(index, buffer_to_sec, SEC_BLK_SIZE);
	if (ret) {
		RAWNAND_ERR("read secure storage block %d name %s err\n",
				index, item_name);
		kfree(buffer_to_sec);
		return -1;
	}
	if (len_in_store > buffer_len)
		memcpy(buffer, buffer_to_sec, buffer_len);
	else
		memcpy(buffer, buffer_to_sec, len_in_store);
	*data_len = len_in_store;
	RAWNAND_DBG("read secure storage: %s ok\n", item_name);

	kfree(buffer_to_sec);
	return 0;
}

/*Add new item to secure storage*/
static int sunxi_secure_storage_write(const char *item_name, char *buffer,
				      int length)
{
	int ret, index;

	if (!secure_storage_inited) {
		RAWNAND_ERR("%s: secure storage has not been inited\n",
				__func__);

		return -1;
	}
	index = __fill_name_in_map(secure_storage_map.data,
			item_name, length);
	if (index < 0) {
		RAWNAND_ERR("write secure storage block %d name %s overrage\n",
				index, item_name);

		return -1;
	}

	ret = sunxi_secstorage_write(index, (unsigned char *)buffer,
				   SEC_BLK_SIZE);
	if (ret) {
		RAWNAND_ERR("write secure storage block %d name %s err\n",
				index, item_name);

		return -1;
	}
	set_map_dirty();
	RAWNAND_DBG("write secure storage: %d ok\n", index);

	return 0;
}

static int sunxi_secure_storage_probe(const char *item_name)
{
	int ret;
	int len;

	if (!secure_storage_inited) {
		RAWNAND_ERR("%s: secure storage has not been inited\n",
				__func__);

		return -1;
	}
	ret = __probe_name_in_map(secure_storage_map.data,
			item_name, &len);
	if (ret < 0) {
		RAWNAND_ERR("no item name %s in the map\n", item_name);

		return -1;
	}
	return 0;
}

int sunxi_secure_object_read(const char *item_name, char *buffer,
			     int buffer_len, int *data_len)
{
	int retLen, ret;
	char *secure_object;

	secure_object = kmalloc(SEC_BLK_SIZE, GFP_KERNEL);
	if (!secure_object) {
		RAWNAND_ERR("%s out of memory", __func__);
		return -1;
	}

	memset(secure_object, 0, SEC_BLK_SIZE);

	ret =
	    sunxi_secure_storage_read(item_name, secure_object, SEC_BLK_SIZE,
				      &retLen);
	if (ret) {
		RAWNAND_ERR("sunxi storage read fail\n");
		kfree(secure_object);
		return -1;
	}
	ret = unwrap_secure_object(secure_object, retLen, buffer, data_len);

	kfree(secure_object);
	return ret;
}

int sunxi_secure_object_write(const char *item_name, char *buffer,
		int length, int write_protect)
{
	int retLen;
	int ret;
	store_object_t *so;

	char *secure_object;

	secure_object = kmalloc(SEC_BLK_SIZE, GFP_KERNEL);
	if (!secure_object) {
		RAWNAND_ERR("%s out of memory", __func__);
		return -1;
	}

	/*
	 * If there is THE same name itme in the secure storage,
	 * we need to decide how to deal with it.
	 * case 1. The same name in the secure storage is
	 *         write_protected, Do thing.
	 * case 2. Otherwise, write it.
	 */
	if (sunxi_secure_storage_probe(item_name) == 0) {
		/*the same name in map */
		sunxi_secure_storage_read(item_name, secure_object, SEC_BLK_SIZE,
					 &retLen);
		so = (store_object_t *) secure_object;
		if (so->magic == STORE_OBJECT_MAGIC &&
		    so->write_protect == STORE_WRITE_PROTECT_MAGIC) {
			RAWNAND_ERR("Can't write a write_protect secure item\n");
			kfree(secure_object);
			return -1;
		} else {
			RAWNAND_DBG("sec obj name[%s] already in device\n",
			      item_name);
		}
	}

	memset(secure_object, 0, SEC_BLK_SIZE);
	retLen = 0;
	ret = wrap_secure_object((void *)buffer, item_name, length,
			secure_object, &retLen, write_protect);
	if (ret < 0 || retLen > SEC_BLK_SIZE) {
		RAWNAND_ERR("wrap fail before secure storage write\n");
		kfree(secure_object);
		return -1;
	}
	ret = sunxi_secure_storage_write(item_name, secure_object, retLen);

	kfree(secure_object);

	return ret;
}

int sunxi_secure_object_erase_one(const char *item_name)
{
	int retLen;
	int ret;
	int write_protect = 0;

	char *secure_object;

	secure_object = kmalloc(SEC_BLK_SIZE, GFP_KERNEL);
	if (!secure_object) {
		RAWNAND_ERR("%s out of memory", __func__);
		return -1;
	}

	if (sunxi_secure_storage_probe(item_name) != 0) {
		RAWNAND_ERR("sec obj name[%s] not in device\n",
				item_name);
		kfree(secure_object);
		return 0;
	}
	memset(secure_object, 0, SEC_BLK_SIZE);
	retLen = 0;
	ret = wrap_secure_object(NULL, item_name, 0,
			secure_object, &retLen, write_protect);
	if (ret < 0 || retLen > SEC_BLK_SIZE) {
		RAWNAND_ERR("wrap fail before secure storage write\n");
		kfree(secure_object);
		return -1;
	}
	ret = sunxi_secure_storage_write(item_name, secure_object, retLen);

	kfree(secure_object);

	return ret;
}

int sunxi_secure_storage_init(void)
{
	int ret;

	if (secure_storage_inited == 1) {
		RAWNAND_ERR("err: secure storage alreay inited\n");
		return 0;
	}

	ret = sunxi_secstorage_read(0, (void *)&secure_storage_map,
			SEC_BLK_SIZE);
	if (ret == -1) {
		RAWNAND_ERR("get secure storage map err\n");
		return -1;
	}

	ret = check_secure_storage_map(&secure_storage_map);
	if (ret == -1) {
		memset(&secure_storage_map, 0, SEC_BLK_SIZE);
	} else if (ret == 2) {
		if ((secure_storage_map.data[0] == 0xff)
				|| (secure_storage_map.data[0] == 0x00)) {
			RAWNAND_DBG("the secure storage map is empty\n");
			memset(&secure_storage_map, 0, SEC_BLK_SIZE);
		}
	}
	secure_storage_inited = 1;

	return 0;
}

int sunxi_secure_storage_exit(int mode)
{
	int ret;

	if (!secure_storage_inited) {
		RAWNAND_ERR("err: secure storage has not been inited\n");
		return -1;
	}
	if (mode || try_map_dirty()) {
		secure_storage_map.magic = STORE_OBJECT_MAGIC;
		secure_storage_map.crc =
		    sst_crc32(0, &secure_storage_map,
				    sizeof(struct map_info) - 4);
		ret = sunxi_secstorage_write(0, (void *)&secure_storage_map,
					   SEC_BLK_SIZE);
		if (ret < 0) {
			RAWNAND_ERR("write secure storage map\n");
			return -1;
		}
	}
	secure_storage_inited = 0;
	clear_map_dirty();

	return 0;
}

/*
 * Check the platform support secure storage or not
 * If yes return 0, otherwise return -1 ;
 *
 */
int secure_storage_support(void)
{
	return 0;
}
