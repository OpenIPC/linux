/* =========================================================================
 * $File: //dwh/usb_iip/dev/software/fh_common_port_2/fh_cc.c $
 * $Revision: #4 $
 * $Date: 2010/11/04 $
 * $Change: 1621692 $
 *
 * Synopsys Portability Library Software and documentation
 * (hereinafter, "Software") is an Unsupported proprietary work of
 * Synopsys, Inc. unless otherwise expressly agreed to in writing
 * between Synopsys and you.
 *
 * The Software IS NOT an item of Licensed Software or Licensed Product
 * under any End User Software License Agreement or Agreement for
 * Licensed Product with Synopsys or any supplement thereto. You are
 * permitted to use and redistribute this Software in source and binary
 * forms, with or without modification, provided that redistributions
 * of source code must retain this notice. You may not view, use,
 * disclose, copy or distribute this file or any information contained
 * herein except pursuant to this license grant from Synopsys. If you
 * do not agree with this notice, including the disclaimer below, then
 * you are not authorized to use the Software.
 *
 * THIS SOFTWARE IS BEING DISTRIBUTED BY SYNOPSYS SOLELY ON AN "AS IS"
 * BASIS AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE HEREBY DISCLAIMED. IN NO EVENT SHALL
 * SYNOPSYS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * ========================================================================= */
#ifdef FH_CCLIB

#include "fh_cc.h"

typedef struct fh_cc
{
	uint32_t uid;
	uint8_t chid[16];
	uint8_t cdid[16];
	uint8_t ck[16];
	uint8_t *name;
	uint8_t length;
        FH_CIRCLEQ_ENTRY(fh_cc) list_entry;
} fh_cc_t;

FH_CIRCLEQ_HEAD(context_list, fh_cc);

/** The main structure for CC management.  */
struct fh_cc_if
{
	fh_mutex_t *mutex;
	char *filename;

	unsigned is_host:1;

	fh_notifier_t *notifier;

	struct context_list list;
};

#ifdef DEBUG
static inline void dump_bytes(char *name, uint8_t *bytes, int len)
{
	int i;
	FH_PRINTF("%s: ", name);
	for (i=0; i<len; i++) {
		FH_PRINTF("%02x ", bytes[i]);
	}
	FH_PRINTF("\n");
}
#else
#define dump_bytes(x...)
#endif

static fh_cc_t *alloc_cc(void *mem_ctx, uint8_t *name, uint32_t length)
{
	fh_cc_t *cc = fh_alloc(mem_ctx, sizeof(fh_cc_t));
	if (!cc) {
		return NULL;
	}
	FH_MEMSET(cc, 0, sizeof(fh_cc_t));

	if (name) {
		cc->length = length;
		cc->name = fh_alloc(mem_ctx, length);
		if (!cc->name) {
			fh_free(mem_ctx, cc);
			return NULL;
		}

		FH_MEMCPY(cc->name, name, length);
	}

	return cc;
}

static void free_cc(void *mem_ctx, fh_cc_t *cc)
{
	if (cc->name) {
		fh_free(mem_ctx, cc->name);
	}
	fh_free(mem_ctx, cc);
}

static uint32_t next_uid(fh_cc_if_t *cc_if)
{
	uint32_t uid = 0;
	fh_cc_t *cc;
	FH_CIRCLEQ_FOREACH(cc, &cc_if->list, list_entry) {
		if (cc->uid > uid) {
			uid = cc->uid;
		}
	}

	if (uid == 0) {
		uid = 255;
	}

	return uid + 1;
}

static fh_cc_t *cc_find(fh_cc_if_t *cc_if, uint32_t uid)
{
	fh_cc_t *cc;
	FH_CIRCLEQ_FOREACH(cc, &cc_if->list, list_entry) {
		if (cc->uid == uid) {
			return cc;
		}
	}
	return NULL;
}

static unsigned int cc_data_size(fh_cc_if_t *cc_if)
{
	unsigned int size = 0;
	fh_cc_t *cc;
	FH_CIRCLEQ_FOREACH(cc, &cc_if->list, list_entry) {
		size += (48 + 1);
		if (cc->name) {
			size += cc->length;
		}
	}
	return size;
}

static uint32_t cc_match_chid(fh_cc_if_t *cc_if, uint8_t *chid)
{
	uint32_t uid = 0;
	fh_cc_t *cc;

	FH_CIRCLEQ_FOREACH(cc, &cc_if->list, list_entry) {
		if (FH_MEMCMP(cc->chid, chid, 16) == 0) {
			uid = cc->uid;
			break;
		}
	}
	return uid;
}
static uint32_t cc_match_cdid(fh_cc_if_t *cc_if, uint8_t *cdid)
{
	uint32_t uid = 0;
	fh_cc_t *cc;

	FH_CIRCLEQ_FOREACH(cc, &cc_if->list, list_entry) {
		if (FH_MEMCMP(cc->cdid, cdid, 16) == 0) {
			uid = cc->uid;
			break;
		}
	}
	return uid;
}

/* Internal cc_add */
static int32_t cc_add(void *mem_ctx, fh_cc_if_t *cc_if, uint8_t *chid,
		      uint8_t *cdid, uint8_t *ck, uint8_t *name, uint8_t length)
{
	fh_cc_t *cc;
	uint32_t uid;

	if (cc_if->is_host) {
		uid = cc_match_cdid(cc_if, cdid);
	}
	else {
		uid = cc_match_chid(cc_if, chid);
	}

	if (uid) {
		FH_DEBUG("Replacing previous connection context id=%d name=%p name_len=%d", uid, name, length);
		cc = cc_find(cc_if, uid);
	}
	else {
		cc = alloc_cc(mem_ctx, name, length);
		cc->uid = next_uid(cc_if);
		FH_CIRCLEQ_INSERT_TAIL(&cc_if->list, cc, list_entry);
	}

	FH_MEMCPY(&(cc->chid[0]), chid, 16);
	FH_MEMCPY(&(cc->cdid[0]), cdid, 16);
	FH_MEMCPY(&(cc->ck[0]), ck, 16);

	FH_DEBUG("Added connection context id=%d name=%p name_len=%d", cc->uid, name, length);
	dump_bytes("CHID", cc->chid, 16);
	dump_bytes("CDID", cc->cdid, 16);
	dump_bytes("CK", cc->ck, 16);
	return cc->uid;
}

/* Internal cc_clear */
static void cc_clear(void *mem_ctx, fh_cc_if_t *cc_if)
{
	while (!FH_CIRCLEQ_EMPTY(&cc_if->list)) {
		fh_cc_t *cc = FH_CIRCLEQ_FIRST(&cc_if->list);
		FH_CIRCLEQ_REMOVE_INIT(&cc_if->list, cc, list_entry);
		free_cc(mem_ctx, cc);
	}
}

fh_cc_if_t *fh_cc_if_alloc(void *mem_ctx, void *mtx_ctx, 
			     fh_notifier_t *notifier, unsigned is_host)
{
	fh_cc_if_t *cc_if = NULL;

	/* Allocate a common_cc_if structure */
	cc_if = fh_alloc(mem_ctx, sizeof(fh_cc_if_t));

	if (!cc_if)
		return NULL;

#if (defined(FH_LINUX) && defined(CONFIG_DEBUG_MUTEXES))
	FH_MUTEX_ALLOC_LINUX_DEBUG(cc_if->mutex);
#else
	cc_if->mutex = fh_mutex_alloc(mtx_ctx);
#endif
	if (!cc_if->mutex) {
		fh_free(mem_ctx, cc_if);
		return NULL;
	}

	FH_CIRCLEQ_INIT(&cc_if->list);
	cc_if->is_host = is_host;
	cc_if->notifier = notifier;
	return cc_if;
}

void fh_cc_if_free(void *mem_ctx, void *mtx_ctx, fh_cc_if_t *cc_if)
{
#if (defined(FH_LINUX) && defined(CONFIG_DEBUG_MUTEXES))
	FH_MUTEX_FREE(cc_if->mutex);
#else
	fh_mutex_free(mtx_ctx, cc_if->mutex);
#endif
	cc_clear(mem_ctx, cc_if);
	fh_free(mem_ctx, cc_if);
}

static void cc_changed(fh_cc_if_t *cc_if)
{
	if (cc_if->notifier) {
		fh_notify(cc_if->notifier, FH_CC_LIST_CHANGED_NOTIFICATION, cc_if);
	}
}

void fh_cc_clear(void *mem_ctx, fh_cc_if_t *cc_if)
{
	FH_MUTEX_LOCK(cc_if->mutex);
	cc_clear(mem_ctx, cc_if);
	FH_MUTEX_UNLOCK(cc_if->mutex);
	cc_changed(cc_if);
}

int32_t fh_cc_add(void *mem_ctx, fh_cc_if_t *cc_if, uint8_t *chid,
		   uint8_t *cdid, uint8_t *ck, uint8_t *name, uint8_t length)
{
	uint32_t uid;

	FH_MUTEX_LOCK(cc_if->mutex);
	uid = cc_add(mem_ctx, cc_if, chid, cdid, ck, name, length);
	FH_MUTEX_UNLOCK(cc_if->mutex);
	cc_changed(cc_if);

	return uid;
}

void fh_cc_change(void *mem_ctx, fh_cc_if_t *cc_if, int32_t id, uint8_t *chid,
		   uint8_t *cdid, uint8_t *ck, uint8_t *name, uint8_t length)
{
	fh_cc_t* cc;

	FH_DEBUG("Change connection context %d", id);

	FH_MUTEX_LOCK(cc_if->mutex);
	cc = cc_find(cc_if, id);
	if (!cc) {
		FH_ERROR("Uid %d not found in cc list\n", id);
		FH_MUTEX_UNLOCK(cc_if->mutex);
		return;
	}

	if (chid) {
		FH_MEMCPY(&(cc->chid[0]), chid, 16);
	}
	if (cdid) {
		FH_MEMCPY(&(cc->cdid[0]), cdid, 16);
	}
	if (ck) {
		FH_MEMCPY(&(cc->ck[0]), ck, 16);
	}

	if (name) {
		if (cc->name) {
			fh_free(mem_ctx, cc->name);
		}
		cc->name = fh_alloc(mem_ctx, length);
		if (!cc->name) {
			FH_ERROR("Out of memory in fh_cc_change()\n");
			FH_MUTEX_UNLOCK(cc_if->mutex);
			return;
		}
		cc->length = length;
		FH_MEMCPY(cc->name, name, length);
	}

	FH_MUTEX_UNLOCK(cc_if->mutex);

	cc_changed(cc_if);

	FH_DEBUG("Changed connection context id=%d\n", id);
	dump_bytes("New CHID", cc->chid, 16);
	dump_bytes("New CDID", cc->cdid, 16);
	dump_bytes("New CK", cc->ck, 16);
}

void fh_cc_remove(void *mem_ctx, fh_cc_if_t *cc_if, int32_t id)
{
	fh_cc_t *cc;

	FH_DEBUG("Removing connection context %d", id);

	FH_MUTEX_LOCK(cc_if->mutex);
	cc = cc_find(cc_if, id);
	if (!cc) {
		FH_ERROR("Uid %d not found in cc list\n", id);
		FH_MUTEX_UNLOCK(cc_if->mutex);
		return;
	}

	FH_CIRCLEQ_REMOVE_INIT(&cc_if->list, cc, list_entry);
	FH_MUTEX_UNLOCK(cc_if->mutex);
	free_cc(mem_ctx, cc);

	cc_changed(cc_if);
}

uint8_t *fh_cc_data_for_save(void *mem_ctx, fh_cc_if_t *cc_if, unsigned int *length)
{
	uint8_t *buf, *x;
	uint8_t zero = 0;
	fh_cc_t *cc;

	FH_MUTEX_LOCK(cc_if->mutex);
	*length = cc_data_size(cc_if);
	if (!(*length)) {
		FH_MUTEX_UNLOCK(cc_if->mutex);
		return NULL;
	}

	FH_DEBUG("Creating data for saving (length=%d)", *length);

	buf = fh_alloc(mem_ctx, *length);
	if (!buf) {
		*length = 0;
		FH_MUTEX_UNLOCK(cc_if->mutex);
		return NULL;
	}

	x = buf;
	FH_CIRCLEQ_FOREACH(cc, &cc_if->list, list_entry) {
		FH_MEMCPY(x, cc->chid, 16);
		x += 16;
		FH_MEMCPY(x, cc->cdid, 16);
		x += 16;
		FH_MEMCPY(x, cc->ck, 16);
		x += 16;
		if (cc->name) {
			FH_MEMCPY(x, &cc->length, 1);
			x += 1;
			FH_MEMCPY(x, cc->name, cc->length);
			x += cc->length;
		}
		else {
			FH_MEMCPY(x, &zero, 1);
			x += 1;
		}
	}
	FH_MUTEX_UNLOCK(cc_if->mutex);

	return buf;
}

void fh_cc_restore_from_data(void *mem_ctx, fh_cc_if_t *cc_if, uint8_t *data, uint32_t length)
{
	uint8_t name_length;
	uint8_t *name;
	uint8_t *chid;
	uint8_t *cdid;
	uint8_t *ck;
	uint32_t i = 0;

	FH_MUTEX_LOCK(cc_if->mutex);
	cc_clear(mem_ctx, cc_if);

	while (i < length) {
		chid = &data[i];
		i += 16;
		cdid = &data[i];
		i += 16;
		ck = &data[i];
		i += 16;

		name_length = data[i];
		i ++;

		if (name_length) {
			name = &data[i];
			i += name_length;
		}
		else {
			name = NULL;
		}

		/* check to see if we haven't overflown the buffer */
		if (i > length) {
			FH_ERROR("Data format error while attempting to load CCs "
				  "(nlen=%d, iter=%d, buflen=%d).\n", name_length, i, length);
			break;
		}

		cc_add(mem_ctx, cc_if, chid, cdid, ck, name, name_length);
	}
	FH_MUTEX_UNLOCK(cc_if->mutex);

	cc_changed(cc_if);
}

uint32_t fh_cc_match_chid(fh_cc_if_t *cc_if, uint8_t *chid)
{
	uint32_t uid = 0;

	FH_MUTEX_LOCK(cc_if->mutex);
	uid = cc_match_chid(cc_if, chid);
	FH_MUTEX_UNLOCK(cc_if->mutex);
	return uid;
}
uint32_t fh_cc_match_cdid(fh_cc_if_t *cc_if, uint8_t *cdid)
{
	uint32_t uid = 0;

	FH_MUTEX_LOCK(cc_if->mutex);
	uid = cc_match_cdid(cc_if, cdid);
	FH_MUTEX_UNLOCK(cc_if->mutex);
	return uid;
}

uint8_t *fh_cc_ck(fh_cc_if_t *cc_if, int32_t id)
{
	uint8_t *ck = NULL;
	fh_cc_t *cc;

	FH_MUTEX_LOCK(cc_if->mutex);
	cc = cc_find(cc_if, id);
	if (cc) {
		ck = cc->ck;
	}
	FH_MUTEX_UNLOCK(cc_if->mutex);

	return ck;

}

uint8_t *fh_cc_chid(fh_cc_if_t *cc_if, int32_t id)
{
	uint8_t *retval = NULL;
	fh_cc_t *cc;

	FH_MUTEX_LOCK(cc_if->mutex);
	cc = cc_find(cc_if, id);
	if (cc) {
		retval = cc->chid;
	}
	FH_MUTEX_UNLOCK(cc_if->mutex);

	return retval;
}

uint8_t *fh_cc_cdid(fh_cc_if_t *cc_if, int32_t id)
{
	uint8_t *retval = NULL;
	fh_cc_t *cc;

	FH_MUTEX_LOCK(cc_if->mutex);
	cc = cc_find(cc_if, id);
	if (cc) {
		retval = cc->cdid;
	}
	FH_MUTEX_UNLOCK(cc_if->mutex);

	return retval;
}

uint8_t *fh_cc_name(fh_cc_if_t *cc_if, int32_t id, uint8_t *length)
{
	uint8_t *retval = NULL;
	fh_cc_t *cc;

	FH_MUTEX_LOCK(cc_if->mutex);
	*length = 0;
	cc = cc_find(cc_if, id);
	if (cc) {
		*length = cc->length;
		retval = cc->name;
	}
	FH_MUTEX_UNLOCK(cc_if->mutex);

	return retval;
}

#endif	/* FH_CCLIB */
