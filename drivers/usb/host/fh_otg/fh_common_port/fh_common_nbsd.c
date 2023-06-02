#include "fh_os.h"
#include "fh_list.h"

#ifdef FH_CCLIB
# include "fh_cc.h"
#endif

#ifdef FH_CRYPTOLIB
# include "fh_modpow.h"
# include "fh_dh.h"
# include "fh_crypto.h"
#endif

#ifdef FH_NOTIFYLIB
# include "fh_notifier.h"
#endif

/* OS-Level Implementations */

/* This is the NetBSD 4.0.1 kernel implementation of the FH platform library. */


/* MISC */

void *FH_MEMSET(void *dest, uint8_t byte, uint32_t size)
{
	return memset(dest, byte, size);
}

void *FH_MEMCPY(void *dest, void const *src, uint32_t size)
{
	return memcpy(dest, src, size);
}

void *FH_MEMMOVE(void *dest, void *src, uint32_t size)
{
	bcopy(src, dest, size);
	return dest;
}

int FH_MEMCMP(void *m1, void *m2, uint32_t size)
{
	return memcmp(m1, m2, size);
}

int FH_STRNCMP(void *s1, void *s2, uint32_t size)
{
	return strncmp(s1, s2, size);
}

int FH_STRCMP(void *s1, void *s2)
{
	return strcmp(s1, s2);
}

int FH_STRLEN(char const *str)
{
	return strlen(str);
}

char *FH_STRCPY(char *to, char const *from)
{
	return strcpy(to, from);
}

char *FH_STRDUP(char const *str)
{
	int len = FH_STRLEN(str) + 1;
	char *new = FH_ALLOC_ATOMIC(len);

	if (!new) {
		return NULL;
	}

	FH_MEMCPY(new, str, len);
	return new;
}

int FH_ATOI(char *str, int32_t *value)
{
	char *end = NULL;

	/* NetBSD doesn't have 'strtol' in the kernel, but 'strtoul'
	 * should be equivalent on 2's complement machines
	 */
	*value = strtoul(str, &end, 0);
	if (*end == '\0') {
		return 0;
	}

	return -1;
}

int FH_ATOUI(char *str, uint32_t *value)
{
	char *end = NULL;

	*value = strtoul(str, &end, 0);
	if (*end == '\0') {
		return 0;
	}

	return -1;
}


#ifdef FH_UTFLIB
/* From usbstring.c */

int FH_UTF8_TO_UTF16LE(uint8_t const *s, uint16_t *cp, unsigned len)
{
	int	count = 0;
	u8	c;
	u16	uchar;

	/* this insists on correct encodings, though not minimal ones.
	 * BUT it currently rejects legit 4-byte UTF-8 code points,
	 * which need surrogate pairs.  (Unicode 3.1 can use them.)
	 */
	while (len != 0 && (c = (u8) *s++) != 0) {
		if (unlikely(c & 0x80)) {
			// 2-byte sequence:
			// 00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
			if ((c & 0xe0) == 0xc0) {
				uchar = (c & 0x1f) << 6;

				c = (u8) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

			// 3-byte sequence (most CJKV characters):
			// zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
			} else if ((c & 0xf0) == 0xe0) {
				uchar = (c & 0x0f) << 12;

				c = (u8) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c << 6;

				c = (u8) *s++;
				if ((c & 0xc0) != 0xc0)
					goto fail;
				c &= 0x3f;
				uchar |= c;

				/* no bogus surrogates */
				if (0xd800 <= uchar && uchar <= 0xdfff)
					goto fail;

			// 4-byte sequence (surrogate pairs, currently rare):
			// 11101110wwwwzzzzyy + 110111yyyyxxxxxx
			//     = 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
			// (uuuuu = wwww + 1)
			// FIXME accept the surrogate code points (only)
			} else
				goto fail;
		} else
			uchar = c;
		put_unaligned (cpu_to_le16 (uchar), cp++);
		count++;
		len--;
	}
	return count;
fail:
	return -1;
}

#endif	/* FH_UTFLIB */


/* fh_debug.h */

fh_bool_t FH_IN_IRQ(void)
{
//	return in_irq();
	return 0;
}

fh_bool_t FH_IN_BH(void)
{
//	return in_softirq();
	return 0;
}

void FH_VPRINTF(char *format, va_list args)
{
	vprintf(format, args);
}

int FH_VSNPRINTF(char *str, int size, char *format, va_list args)
{
	return vsnprintf(str, size, format, args);
}

void FH_PRINTF(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_VPRINTF(format, args);
	va_end(args);
}

int FH_SPRINTF(char *buffer, char *format, ...)
{
	int retval;
	va_list args;

	va_start(args, format);
	retval = vsprintf(buffer, format, args);
	va_end(args);
	return retval;
}

int FH_SNPRINTF(char *buffer, int size, char *format, ...)
{
	int retval;
	va_list args;

	va_start(args, format);
	retval = vsnprintf(buffer, size, format, args);
	va_end(args);
	return retval;
}

void __FH_WARN(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_VPRINTF(format, args);
	va_end(args);
}

void __FH_ERROR(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_VPRINTF(format, args);
	va_end(args);
}

void FH_EXCEPTION(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_VPRINTF(format, args);
	va_end(args);
//	BUG_ON(1);	???
}

#ifdef DEBUG
void __FH_DEBUG(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_VPRINTF(format, args);
	va_end(args);
}
#endif


/* fh_mem.h */

#if 0
fh_pool_t *FH_DMA_POOL_CREATE(uint32_t size,
				uint32_t align,
				uint32_t alloc)
{
	struct dma_pool *pool = dma_pool_create("Pool", NULL,
						size, align, alloc);
	return (fh_pool_t *)pool;
}

void FH_DMA_POOL_DESTROY(fh_pool_t *pool)
{
	dma_pool_destroy((struct dma_pool *)pool);
}

void *FH_DMA_POOL_ALLOC(fh_pool_t *pool, uint64_t *dma_addr)
{
//	return dma_pool_alloc((struct dma_pool *)pool, GFP_KERNEL, dma_addr);
	return dma_pool_alloc((struct dma_pool *)pool, M_WAITOK, dma_addr);
}

void *FH_DMA_POOL_ZALLOC(fh_pool_t *pool, uint64_t *dma_addr)
{
	void *vaddr = FH_DMA_POOL_ALLOC(pool, dma_addr);
	memset(..);
}

void FH_DMA_POOL_FREE(fh_pool_t *pool, void *vaddr, void *daddr)
{
	dma_pool_free(pool, vaddr, daddr);
}
#endif

void *__FH_DMA_ALLOC(void *dma_ctx, uint32_t size, fh_dma_t *dma_addr)
{
	fh_dmactx_t *dma = (fh_dmactx_t *)dma_ctx;
	int error;

	error = bus_dmamem_alloc(dma->dma_tag, size, 1, size, dma->segs,
				 sizeof(dma->segs) / sizeof(dma->segs[0]),
				 &dma->nsegs, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: bus_dmamem_alloc(%ju) failed: %d\n", __func__,
		       (uintmax_t)size, error);
		goto fail_0;
	}

	error = bus_dmamem_map(dma->dma_tag, dma->segs, dma->nsegs, size,
			       (caddr_t *)&dma->dma_vaddr,
			       BUS_DMA_NOWAIT | BUS_DMA_COHERENT);
	if (error) {
		printf("%s: bus_dmamem_map failed: %d\n", __func__, error);
		goto fail_1;
	}

	error = bus_dmamap_create(dma->dma_tag, size, 1, size, 0,
				  BUS_DMA_NOWAIT, &dma->dma_map);
	if (error) {
		printf("%s: bus_dmamap_create failed: %d\n", __func__, error);
		goto fail_2;
	}

	error = bus_dmamap_load(dma->dma_tag, dma->dma_map, dma->dma_vaddr,
				size, NULL, BUS_DMA_NOWAIT);
	if (error) {
		printf("%s: bus_dmamap_load failed: %d\n", __func__, error);
		goto fail_3;
	}

	dma->dma_paddr = (bus_addr_t)dma->segs[0].ds_addr;
	*dma_addr = dma->dma_paddr;
	return dma->dma_vaddr;

fail_3:
	bus_dmamap_destroy(dma->dma_tag, dma->dma_map);
fail_2:
	bus_dmamem_unmap(dma->dma_tag, dma->dma_vaddr, size);
fail_1:
	bus_dmamem_free(dma->dma_tag, dma->segs, dma->nsegs);
fail_0:
	dma->dma_map = NULL;
	dma->dma_vaddr = NULL;
	dma->nsegs = 0;

	return NULL;
}

void __FH_DMA_FREE(void *dma_ctx, uint32_t size, void *virt_addr, fh_dma_t dma_addr)
{
	fh_dmactx_t *dma = (fh_dmactx_t *)dma_ctx;

	if (dma->dma_map != NULL) {
		bus_dmamap_sync(dma->dma_tag, dma->dma_map, 0, size,
				BUS_DMASYNC_POSTREAD | BUS_DMASYNC_POSTWRITE);
		bus_dmamap_unload(dma->dma_tag, dma->dma_map);
		bus_dmamap_destroy(dma->dma_tag, dma->dma_map);
		bus_dmamem_unmap(dma->dma_tag, dma->dma_vaddr, size);
		bus_dmamem_free(dma->dma_tag, dma->segs, dma->nsegs);
		dma->dma_paddr = 0;
		dma->dma_map = NULL;
		dma->dma_vaddr = NULL;
		dma->nsegs = 0;
	}
}

void *__FH_ALLOC(void *mem_ctx, uint32_t size)
{
	return malloc(size, M_DEVBUF, M_WAITOK | M_ZERO);
}

void *__FH_ALLOC_ATOMIC(void *mem_ctx, uint32_t size)
{
	return malloc(size, M_DEVBUF, M_NOWAIT | M_ZERO);
}

void __FH_FREE(void *mem_ctx, void *addr)
{
	free(addr, M_DEVBUF);
}


#ifdef FH_CRYPTOLIB
/* fh_crypto.h */

void FH_RANDOM_BYTES(uint8_t *buffer, uint32_t length)
{
	get_random_bytes(buffer, length);
}

int FH_AES_CBC(uint8_t *message, uint32_t messagelen, uint8_t *key, uint32_t keylen, uint8_t iv[16], uint8_t *out)
{
	struct crypto_blkcipher *tfm;
	struct blkcipher_desc desc;
	struct scatterlist sgd;
	struct scatterlist sgs;

	tfm = crypto_alloc_blkcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC);
	if (tfm == NULL) {
		printk("failed to load transform for aes CBC\n");
		return -1;
	}

	crypto_blkcipher_setkey(tfm, key, keylen);
	crypto_blkcipher_set_iv(tfm, iv, 16);

	sg_init_one(&sgd, out, messagelen);
	sg_init_one(&sgs, message, messagelen);

	desc.tfm = tfm;
	desc.flags = 0;

	if (crypto_blkcipher_encrypt(&desc, &sgd, &sgs, messagelen)) {
		crypto_free_blkcipher(tfm);
		FH_ERROR("AES CBC encryption failed");
		return -1;
	}

	crypto_free_blkcipher(tfm);
	return 0;
}

int FH_SHA256(uint8_t *message, uint32_t len, uint8_t *out)
{
	struct crypto_hash *tfm;
	struct hash_desc desc;
	struct scatterlist sg;

	tfm = crypto_alloc_hash("sha256", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		FH_ERROR("Failed to load transform for sha256: %ld", PTR_ERR(tfm));
		return 0;
	}
	desc.tfm = tfm;
	desc.flags = 0;

	sg_init_one(&sg, message, len);
	crypto_hash_digest(&desc, &sg, len, out);
	crypto_free_hash(tfm);

	return 1;
}

int FH_HMAC_SHA256(uint8_t *message, uint32_t messagelen,
		    uint8_t *key, uint32_t keylen, uint8_t *out)
{
	struct crypto_hash *tfm;
	struct hash_desc desc;
	struct scatterlist sg;

	tfm = crypto_alloc_hash("hmac(sha256)", 0, CRYPTO_ALG_ASYNC);
	if (IS_ERR(tfm)) {
		FH_ERROR("Failed to load transform for hmac(sha256): %ld", PTR_ERR(tfm));
		return 0;
	}
	desc.tfm = tfm;
	desc.flags = 0;

	sg_init_one(&sg, message, messagelen);
	crypto_hash_setkey(tfm, key, keylen);
	crypto_hash_digest(&desc, &sg, messagelen, out);
	crypto_free_hash(tfm);

	return 1;
}

#endif	/* FH_CRYPTOLIB */


/* Byte Ordering Conversions */

uint32_t FH_CPU_TO_LE32(uint32_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint32_t FH_CPU_TO_BE32(uint32_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint32_t FH_LE32_TO_CPU(uint32_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint32_t FH_BE32_TO_CPU(uint32_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;

	return (u_p[3] | (u_p[2] << 8) | (u_p[1] << 16) | (u_p[0] << 24));
#endif
}

uint16_t FH_CPU_TO_LE16(uint16_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return (u_p[1] | (u_p[0] << 8));
#endif
}

uint16_t FH_CPU_TO_BE16(uint16_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return (u_p[1] | (u_p[0] << 8));
#endif
}

uint16_t FH_LE16_TO_CPU(uint16_t *p)
{
#ifdef __LITTLE_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return (u_p[1] | (u_p[0] << 8));
#endif
}

uint16_t FH_BE16_TO_CPU(uint16_t *p)
{
#ifdef __BIG_ENDIAN
	return *p;
#else
	uint8_t *u_p = (uint8_t *)p;
	return (u_p[1] | (u_p[0] << 8));
#endif
}


/* Registers */

uint32_t FH_READ_REG32(void *io_ctx, uint32_t volatile *reg)
{
	fh_ioctx_t *io = (fh_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	return bus_space_read_4(io->iot, io->ioh, ior);
}

#if 0
uint64_t FH_READ_REG64(void *io_ctx, uint64_t volatile *reg)
{
	fh_ioctx_t *io = (fh_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	return bus_space_read_8(io->iot, io->ioh, ior);
}
#endif

void FH_WRITE_REG32(void *io_ctx, uint32_t volatile *reg, uint32_t value)
{
	fh_ioctx_t *io = (fh_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	bus_space_write_4(io->iot, io->ioh, ior, value);
}

#if 0
void FH_WRITE_REG64(void *io_ctx, uint64_t volatile *reg, uint64_t value)
{
	fh_ioctx_t *io = (fh_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	bus_space_write_8(io->iot, io->ioh, ior, value);
}
#endif

void FH_MODIFY_REG32(void *io_ctx, uint32_t volatile *reg, uint32_t clear_mask,
		      uint32_t set_mask)
{
	fh_ioctx_t *io = (fh_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	bus_space_write_4(io->iot, io->ioh, ior,
			  (bus_space_read_4(io->iot, io->ioh, ior) &
			   ~clear_mask) | set_mask);
}

#if 0
void FH_MODIFY_REG64(void *io_ctx, uint64_t volatile *reg, uint64_t clear_mask,
		      uint64_t set_mask)
{
	fh_ioctx_t *io = (fh_ioctx_t *)io_ctx;
	bus_size_t ior = (bus_size_t)reg;

	bus_space_write_8(io->iot, io->ioh, ior,
			  (bus_space_read_8(io->iot, io->ioh, ior) &
			   ~clear_mask) | set_mask);
}
#endif


/* Locking */

fh_spinlock_t *FH_SPINLOCK_ALLOC(void)
{
	struct simplelock *sl = FH_ALLOC(sizeof(*sl));

	if (!sl) {
		FH_ERROR("Cannot allocate memory for spinlock");
		return NULL;
	}

	simple_lock_init(sl);
	return (fh_spinlock_t *)sl;
}

void FH_SPINLOCK_FREE(fh_spinlock_t *lock)
{
	struct simplelock *sl = (struct simplelock *)lock;

	FH_FREE(sl);
}

void FH_SPINLOCK(fh_spinlock_t *lock)
{
	simple_lock((struct simplelock *)lock);
}

void FH_SPINUNLOCK(fh_spinlock_t *lock)
{
	simple_unlock((struct simplelock *)lock);
}

void FH_SPINLOCK_IRQSAVE(fh_spinlock_t *lock, fh_irqflags_t *flags)
{
	simple_lock((struct simplelock *)lock);
	*flags = splbio();
}

void FH_SPINUNLOCK_IRQRESTORE(fh_spinlock_t *lock, fh_irqflags_t flags)
{
	splx(flags);
	simple_unlock((struct simplelock *)lock);
}

fh_mutex_t *FH_MUTEX_ALLOC(void)
{
	fh_mutex_t *mutex = FH_ALLOC(sizeof(struct lock));

	if (!mutex) {
		FH_ERROR("Cannot allocate memory for mutex");
		return NULL;
	}

	lockinit((struct lock *)mutex, 0, "dw3mtx", 0, 0);
	return mutex;
}

#if (defined(FH_LINUX) && defined(CONFIG_DEBUG_MUTEXES))
#else
void FH_MUTEX_FREE(fh_mutex_t *mutex)
{
	FH_FREE(mutex);
}
#endif

void FH_MUTEX_LOCK(fh_mutex_t *mutex)
{
	lockmgr((struct lock *)mutex, LK_EXCLUSIVE, NULL);
}

int FH_MUTEX_TRYLOCK(fh_mutex_t *mutex)
{
	int status;

	status = lockmgr((struct lock *)mutex, LK_EXCLUSIVE | LK_NOWAIT, NULL);
	return status == 0;
}

void FH_MUTEX_UNLOCK(fh_mutex_t *mutex)
{
	lockmgr((struct lock *)mutex, LK_RELEASE, NULL);
}


/* Timing */

void FH_UDELAY(uint32_t usecs)
{
	DELAY(usecs);
}

void FH_MDELAY(uint32_t msecs)
{
	do {
		DELAY(1000);
	} while (--msecs);
}

void FH_MSLEEP(uint32_t msecs)
{
	struct timeval tv;

	tv.tv_sec = msecs / 1000;
	tv.tv_usec = (msecs - tv.tv_sec * 1000) * 1000;
	tsleep(&tv, 0, "dw3slp", tvtohz(&tv));
}

uint32_t FH_TIME(void)
{
	struct timeval tv;

	microuptime(&tv);	// or getmicrouptime? (less precise, but faster)
	return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


/* Timers */

struct fh_timer {
	struct callout t;
	char *name;
	fh_spinlock_t *lock;
	fh_timer_callback_t cb;
	void *data;
};

fh_timer_t *FH_TIMER_ALLOC(char *name, fh_timer_callback_t cb, void *data)
{
	fh_timer_t *t = FH_ALLOC(sizeof(*t));

	if (!t) {
		FH_ERROR("Cannot allocate memory for timer");
		return NULL;
	}

	callout_init(&t->t);

	t->name = FH_STRDUP(name);
	if (!t->name) {
		FH_ERROR("Cannot allocate memory for timer->name");
		goto no_name;
	}

	t->lock = FH_SPINLOCK_ALLOC();
	if (!t->lock) {
		FH_ERROR("Cannot allocate memory for timer->lock");
		goto no_lock;
	}

	t->cb = cb;
	t->data = data;

	return t;

 no_lock:
	FH_FREE(t->name);
 no_name:
	FH_FREE(t);

	return NULL;
}

void FH_TIMER_FREE(fh_timer_t *timer)
{
	callout_stop(&timer->t);
	FH_SPINLOCK_FREE(timer->lock);
	FH_FREE(timer->name);
	FH_FREE(timer);
}

void FH_TIMER_SCHEDULE(fh_timer_t *timer, uint32_t time)
{
	struct timeval tv;

	tv.tv_sec = time / 1000;
	tv.tv_usec = (time - tv.tv_sec * 1000) * 1000;
	callout_reset(&timer->t, tvtohz(&tv), timer->cb, timer->data);
}

void FH_TIMER_CANCEL(fh_timer_t *timer)
{
	callout_stop(&timer->t);
}


/* Wait Queues */

struct fh_waitq {
	struct simplelock lock;
	int abort;
};

fh_waitq_t *FH_WAITQ_ALLOC(void)
{
	fh_waitq_t *wq = FH_ALLOC(sizeof(*wq));

	if (!wq) {
		FH_ERROR("Cannot allocate memory for waitqueue");
		return NULL;
	}

	simple_lock_init(&wq->lock);
	wq->abort = 0;

	return wq;
}

void FH_WAITQ_FREE(fh_waitq_t *wq)
{
	FH_FREE(wq);
}

int32_t FH_WAITQ_WAIT(fh_waitq_t *wq, fh_waitq_condition_t cond, void *data)
{
	int ipl;
	int result = 0;

	simple_lock(&wq->lock);
	ipl = splbio();

	/* Skip the sleep if already aborted or triggered */
	if (!wq->abort && !cond(data)) {
		splx(ipl);
		result = ltsleep(wq, PCATCH, "dw3wat", 0, &wq->lock); // infinite timeout
		ipl = splbio();
	}

	if (result == 0) {			// awoken
		if (wq->abort) {
			wq->abort = 0;
			result = -FH_E_ABORT;
		} else {
			result = 0;
		}

		splx(ipl);
		simple_unlock(&wq->lock);
	} else {
		wq->abort = 0;
		splx(ipl);
		simple_unlock(&wq->lock);

		if (result == ERESTART) {	// signaled - restart
			result = -FH_E_RESTART;
		} else {			// signaled - must be EINTR
			result = -FH_E_ABORT;
		}
	}

	return result;
}

int32_t FH_WAITQ_WAIT_TIMEOUT(fh_waitq_t *wq, fh_waitq_condition_t cond,
			       void *data, int32_t msecs)
{
	struct timeval tv, tv1, tv2;
	int ipl;
	int result = 0;

	tv.tv_sec = msecs / 1000;
	tv.tv_usec = (msecs - tv.tv_sec * 1000) * 1000;

	simple_lock(&wq->lock);
	ipl = splbio();

	/* Skip the sleep if already aborted or triggered */
	if (!wq->abort && !cond(data)) {
		splx(ipl);
		getmicrouptime(&tv1);
		result = ltsleep(wq, PCATCH, "dw3wto", tvtohz(&tv), &wq->lock);
		getmicrouptime(&tv2);
		ipl = splbio();
	}

	if (result == 0) {			// awoken
		if (wq->abort) {
			wq->abort = 0;
			splx(ipl);
			simple_unlock(&wq->lock);
			result = -FH_E_ABORT;
		} else {
			splx(ipl);
			simple_unlock(&wq->lock);

			tv2.tv_usec -= tv1.tv_usec;
			if (tv2.tv_usec < 0) {
				tv2.tv_usec += 1000000;
				tv2.tv_sec--;
			}

			tv2.tv_sec -= tv1.tv_sec;
			result = tv2.tv_sec * 1000 + tv2.tv_usec / 1000;
			result = msecs - result;
			if (result <= 0)
				result = 1;
		}
	} else {
		wq->abort = 0;
		splx(ipl);
		simple_unlock(&wq->lock);

		if (result == ERESTART) {	// signaled - restart
			result = -FH_E_RESTART;

		} else if (result == EINTR) {		// signaled - interrupt
			result = -FH_E_ABORT;

		} else {				// timed out
			result = -FH_E_TIMEOUT;
		}
	}

	return result;
}

void FH_WAITQ_TRIGGER(fh_waitq_t *wq)
{
	wakeup(wq);
}

void FH_WAITQ_ABORT(fh_waitq_t *wq)
{
	int ipl;

	simple_lock(&wq->lock);
	ipl = splbio();
	wq->abort = 1;
	wakeup(wq);
	splx(ipl);
	simple_unlock(&wq->lock);
}


/* Threading */

struct fh_thread {
	struct proc *proc;
	int abort;
};

fh_thread_t *FH_THREAD_RUN(fh_thread_function_t func, char *name, void *data)
{
	int retval;
	fh_thread_t *thread = FH_ALLOC(sizeof(*thread));

	if (!thread) {
		return NULL;
	}

	thread->abort = 0;
	retval = kthread_create1((void (*)(void *))func, data, &thread->proc,
				 "%s", name);
	if (retval) {
		FH_FREE(thread);
		return NULL;
	}

	return thread;
}

int FH_THREAD_STOP(fh_thread_t *thread)
{
	int retval;

	thread->abort = 1;
	retval = tsleep(&thread->abort, 0, "dw3stp", 60 * hz);

	if (retval == 0) {
		/* FH_THREAD_EXIT() will free the thread struct */
		return 0;
	}

	/* NOTE: We leak the thread struct if thread doesn't die */

	if (retval == EWOULDBLOCK) {
		return -FH_E_TIMEOUT;
	}

	return -FH_E_UNKNOWN;
}

fh_bool_t FH_THREAD_SHOULD_STOP(fh_thread_t *thread)
{
	return thread->abort;
}

void FH_THREAD_EXIT(fh_thread_t *thread)
{
	wakeup(&thread->abort);
	FH_FREE(thread);
	kthread_exit(0);
}

/* tasklets
 - Runs in interrupt context (cannot sleep)
 - Each tasklet runs on a single CPU
 - Different tasklets can be running simultaneously on different CPUs
 [ On NetBSD there is no corresponding mechanism, drivers don't have bottom-
   halves. So we just call the callback directly from FH_TASK_SCHEDULE() ]
 */
struct fh_tasklet {
	fh_tasklet_callback_t cb;
	void *data;
};

static void tasklet_callback(void *data)
{
	fh_tasklet_t *task = (fh_tasklet_t *)data;

	task->cb(task->data);
}

fh_tasklet_t *FH_TASK_ALLOC(char *name, fh_tasklet_callback_t cb, void *data)
{
	fh_tasklet_t *task = FH_ALLOC(sizeof(*task));

	if (task) {
		task->cb = cb;
		task->data = data;
	} else {
		FH_ERROR("Cannot allocate memory for tasklet");
	}

	return task;
}

void FH_TASK_FREE(fh_tasklet_t *task)
{
	FH_FREE(task);
}

void FH_TASK_SCHEDULE(fh_tasklet_t *task)
{
	tasklet_callback(task);
}


/* workqueues
 - Runs in process context (can sleep)
 */
typedef struct work_container {
	fh_work_callback_t cb;
	void *data;
	fh_workq_t *wq;
	char *name;
	int hz;
	struct work task;
} work_container_t;

struct fh_workq {
	struct workqueue *taskq;
	fh_spinlock_t *lock;
	fh_waitq_t *waitq;
	int pending;
	struct work_container *container;
};

static void do_work(struct work *task, void *data)
{
	fh_workq_t *wq = (fh_workq_t *)data;
	work_container_t *container = wq->container;
	fh_irqflags_t flags;

	if (container->hz) {
		tsleep(container, 0, "dw3wrk", container->hz);
	}

	container->cb(container->data);
	FH_DEBUG("Work done: %s, container=%p", container->name, container);

	FH_SPINLOCK_IRQSAVE(wq->lock, &flags);
	if (container->name)
		FH_FREE(container->name);
	FH_FREE(container);
	wq->pending--;
	FH_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
	FH_WAITQ_TRIGGER(wq->waitq);
}

static int work_done(void *data)
{
	fh_workq_t *workq = (fh_workq_t *)data;

	return workq->pending == 0;
}

int FH_WORKQ_WAIT_WORK_DONE(fh_workq_t *workq, int timeout)
{
	return FH_WAITQ_WAIT_TIMEOUT(workq->waitq, work_done, workq, timeout);
}

fh_workq_t *FH_WORKQ_ALLOC(char *name)
{
	int result;
	fh_workq_t *wq = FH_ALLOC(sizeof(*wq));

	if (!wq) {
		FH_ERROR("Cannot allocate memory for workqueue");
		return NULL;
	}

	result = workqueue_create(&wq->taskq, name, do_work, wq, 0 /*PWAIT*/,
				  IPL_BIO, 0);
	if (result) {
		FH_ERROR("Cannot create workqueue");
		goto no_taskq;
	}

	wq->pending = 0;

	wq->lock = FH_SPINLOCK_ALLOC();
	if (!wq->lock) {
		FH_ERROR("Cannot allocate memory for spinlock");
		goto no_lock;
	}

	wq->waitq = FH_WAITQ_ALLOC();
	if (!wq->waitq) {
		FH_ERROR("Cannot allocate memory for waitqueue");
		goto no_waitq;
	}

	return wq;

 no_waitq:
	FH_SPINLOCK_FREE(wq->lock);
 no_lock:
	workqueue_destroy(wq->taskq);
 no_taskq:
	FH_FREE(wq);

	return NULL;
}

void FH_WORKQ_FREE(fh_workq_t *wq)
{
#ifdef DEBUG
	fh_irqflags_t flags;

	FH_SPINLOCK_IRQSAVE(wq->lock, &flags);

	if (wq->pending != 0) {
		struct work_container *container = wq->container;

		FH_ERROR("Destroying work queue with pending work");

		if (container && container->name) {
			FH_ERROR("Work %s still pending", container->name);
		}
	}

	FH_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
#endif
	FH_WAITQ_FREE(wq->waitq);
	FH_SPINLOCK_FREE(wq->lock);
	workqueue_destroy(wq->taskq);
	FH_FREE(wq);
}

void FH_WORKQ_SCHEDULE(fh_workq_t *wq, fh_work_callback_t cb, void *data,
			char *format, ...)
{
	fh_irqflags_t flags;
	work_container_t *container;
	static char name[128];
	va_list args;

	va_start(args, format);
	FH_VSNPRINTF(name, 128, format, args);
	va_end(args);

	FH_SPINLOCK_IRQSAVE(wq->lock, &flags);
	wq->pending++;
	FH_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
	FH_WAITQ_TRIGGER(wq->waitq);

	container = FH_ALLOC_ATOMIC(sizeof(*container));
	if (!container) {
		FH_ERROR("Cannot allocate memory for container");
		return;
	}

	container->name = FH_STRDUP(name);
	if (!container->name) {
		FH_ERROR("Cannot allocate memory for container->name");
		FH_FREE(container);
		return;
	}

	container->cb = cb;
	container->data = data;
	container->wq = wq;
	container->hz = 0;
	wq->container = container;

	FH_DEBUG("Queueing work: %s, container=%p", container->name, container);
	workqueue_enqueue(wq->taskq, &container->task);
}

void FH_WORKQ_SCHEDULE_DELAYED(fh_workq_t *wq, fh_work_callback_t cb,
				void *data, uint32_t time, char *format, ...)
{
	fh_irqflags_t flags;
	work_container_t *container;
	static char name[128];
	struct timeval tv;
	va_list args;

	va_start(args, format);
	FH_VSNPRINTF(name, 128, format, args);
	va_end(args);

	FH_SPINLOCK_IRQSAVE(wq->lock, &flags);
	wq->pending++;
	FH_SPINUNLOCK_IRQRESTORE(wq->lock, flags);
	FH_WAITQ_TRIGGER(wq->waitq);

	container = FH_ALLOC_ATOMIC(sizeof(*container));
	if (!container) {
		FH_ERROR("Cannot allocate memory for container");
		return;
	}

	container->name = FH_STRDUP(name);
	if (!container->name) {
		FH_ERROR("Cannot allocate memory for container->name");
		FH_FREE(container);
		return;
	}

	container->cb = cb;
	container->data = data;
	container->wq = wq;
	tv.tv_sec = time / 1000;
	tv.tv_usec = (time - tv.tv_sec * 1000) * 1000;
	container->hz = tvtohz(&tv);
	wq->container = container;

	FH_DEBUG("Queueing work: %s, container=%p", container->name, container);
	workqueue_enqueue(wq->taskq, &container->task);
}

int FH_WORKQ_PENDING(fh_workq_t *wq)
{
	return wq->pending;
}
