#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>

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

/* This is the Linux kernel implementation of the FH platform library. */
#include <linux/moduleparam.h>
#include <linux/ctype.h>
#include <linux/crypto.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/cdev.h>
#include <linux/errno.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/list.h>
#include <linux/pci.h>
#include <linux/random.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/usb.h>

#include <linux/version.h>

#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24)
# include <linux/usb/gadget.h>
#else
# include <linux/usb_gadget.h>
#endif

#include <asm/io.h>
#include <asm/page.h>
#include <asm/uaccess.h>
#include <asm/unaligned.h>

#include "fh_os.h"
#include "fh_list.h"


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
	return memmove(dest, src, size);
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

int FH_ATOI(const char *str, int32_t *value)
{
	char *end = NULL;

	*value = simple_strtol(str, &end, 0);
	if (*end == '\0') {
		return 0;
	}

	return -1;
}

int FH_ATOUI(const char *str, uint32_t *value)
{
	char *end = NULL;

	*value = simple_strtoul(str, &end, 0);
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
	return in_irq();
}

fh_bool_t FH_IN_BH(void)
{
	return in_softirq();
}

void FH_VPRINTF(char *format, va_list args)
{
	vprintk(format, args);
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
	FH_PRINTF(KERN_WARNING);
	FH_VPRINTF(format, args);
	va_end(args);
}

void __FH_ERROR(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_PRINTF(KERN_ERR);
	FH_VPRINTF(format, args);
	va_end(args);
}

void FH_EXCEPTION(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_PRINTF(KERN_ERR);
	FH_VPRINTF(format, args);
	va_end(args);
	BUG_ON(1);
}

#ifdef DEBUG
void __FH_DEBUG(char *format, ...)
{
	va_list args;

	va_start(args, format);
	FH_PRINTF(KERN_DEBUG);
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
	return dma_pool_alloc((struct dma_pool *)pool, GFP_KERNEL, dma_addr);
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
#ifdef xxCOSIM /* Only works for 32-bit cosim */
	void *buf = dma_alloc_coherent(dma_ctx, (size_t)size, dma_addr, GFP_KERNEL);
#else
	void *buf = dma_alloc_coherent(dma_ctx, (size_t)size, dma_addr, GFP_ATOMIC);
#endif
	if (!buf) {
		return NULL;
	}

	memset(buf, 0, (size_t)size);
	return buf;
}

void *__FH_DMA_ALLOC_ATOMIC(void *dma_ctx, uint32_t size, fh_dma_t *dma_addr)
{
	void *buf = dma_alloc_coherent(NULL, (size_t)size, dma_addr, GFP_ATOMIC);
	if (!buf) {
		return NULL;
	}
	memset(buf, 0, (size_t)size);
	return buf;
}

void __FH_DMA_FREE(void *dma_ctx, uint32_t size, void *virt_addr, fh_dma_t dma_addr)
{
	dma_free_coherent(dma_ctx, size, virt_addr, dma_addr);
}

void *__FH_ALLOC(void *mem_ctx, uint32_t size)
{
	return kzalloc(size, GFP_KERNEL);
}

void *__FH_ALLOC_ATOMIC(void *mem_ctx, uint32_t size)
{
	return kzalloc(size, GFP_ATOMIC);
}

void __FH_FREE(void *mem_ctx, void *addr)
{
	kfree(addr);
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
		FH_ERROR("Failed to load transform for sha256: %ld\n", PTR_ERR(tfm));
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
		FH_ERROR("Failed to load transform for hmac(sha256): %ld\n", PTR_ERR(tfm));
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

uint32_t FH_READ_REG32(uint32_t volatile *reg)
{
	return readl(reg);
}

#if 0
uint64_t FH_READ_REG64(uint64_t volatile *reg)
{
}
#endif

void FH_WRITE_REG32(uint32_t volatile *reg, uint32_t value)
{
	writel(value, reg);
}

#if 0
void FH_WRITE_REG64(uint64_t volatile *reg, uint64_t value)
{
}
#endif

void FH_MODIFY_REG32(uint32_t volatile *reg, uint32_t clear_mask, uint32_t set_mask)
{
	writel((readl(reg) & ~clear_mask) | set_mask, reg);
}

#if 0
void FH_MODIFY_REG64(uint64_t volatile *reg, uint64_t clear_mask, uint64_t set_mask)
{
}
#endif


/* Locking */

fh_spinlock_t *FH_SPINLOCK_ALLOC(void)
{
	spinlock_t *sl = (spinlock_t *)1;

#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
	sl = FH_ALLOC(sizeof(*sl));
	if (!sl) {
		FH_ERROR("Cannot allocate memory for spinlock\n");
		return NULL;
	}

	spin_lock_init(sl);
#endif
	return (fh_spinlock_t *)sl;
}

void FH_SPINLOCK_FREE(fh_spinlock_t *lock)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
	FH_FREE(lock);
#endif
}

void FH_SPINLOCK(fh_spinlock_t *lock)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
	spin_lock((spinlock_t *)lock);
#endif
}

void FH_SPINUNLOCK(fh_spinlock_t *lock)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
	spin_unlock((spinlock_t *)lock);
#endif
}

void FH_SPINLOCK_IRQSAVE(fh_spinlock_t *lock, fh_irqflags_t *flags)
{
	fh_irqflags_t f;

#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
	spin_lock_irqsave((spinlock_t *)lock, f);
#else
	local_irq_save(f);
#endif
	*flags = f;
}

void FH_SPINUNLOCK_IRQRESTORE(fh_spinlock_t *lock, fh_irqflags_t flags)
{
#if defined(CONFIG_PREEMPT) || defined(CONFIG_SMP)
	spin_unlock_irqrestore((spinlock_t *)lock, flags);
#else
	local_irq_restore(flags);
#endif
}

fh_mutex_t *FH_MUTEX_ALLOC(void)
{
	struct mutex *m;
	fh_mutex_t *mutex = (fh_mutex_t *)FH_ALLOC(sizeof(struct mutex));

	if (!mutex) {
		FH_ERROR("Cannot allocate memory for mutex\n");
		return NULL;
	}

	m = (struct mutex *)mutex;
	mutex_init(m);
	return mutex;
}

#if (defined(FH_LINUX) && defined(CONFIG_DEBUG_MUTEXES))
#else
void FH_MUTEX_FREE(fh_mutex_t *mutex)
{
	mutex_destroy((struct mutex *)mutex);
	FH_FREE(mutex);
}
#endif

void FH_MUTEX_LOCK(fh_mutex_t *mutex)
{
	struct mutex *m = (struct mutex *)mutex;
	mutex_lock(m);
}

int FH_MUTEX_TRYLOCK(fh_mutex_t *mutex)
{
	struct mutex *m = (struct mutex *)mutex;
	return mutex_trylock(m);
}

void FH_MUTEX_UNLOCK(fh_mutex_t *mutex)
{
	struct mutex *m = (struct mutex *)mutex;
	mutex_unlock(m);
}


/* Timing */

void FH_UDELAY(uint32_t usecs)
{
	udelay(usecs);
}

void FH_MDELAY(uint32_t msecs)
{
	mdelay(msecs);
}

void FH_MSLEEP(uint32_t msecs)
{
	msleep(msecs);
}

uint32_t FH_TIME(void)
{
	return jiffies_to_msecs(jiffies);
}


/* Timers */

struct fh_timer {
	struct timer_list *t;
	char *name;
	fh_timer_callback_t cb;
	void *data;
	uint8_t scheduled;
	fh_spinlock_t *lock;
};

static void timer_callback(unsigned long data)
{
	fh_timer_t *timer = (fh_timer_t *)data;
	fh_irqflags_t flags;

	FH_SPINLOCK_IRQSAVE(timer->lock, &flags);
	timer->scheduled = 0;
	FH_SPINUNLOCK_IRQRESTORE(timer->lock, flags);
	FH_DEBUG("Timer %s callback", timer->name);
	timer->cb(timer->data);
}

fh_timer_t *FH_TIMER_ALLOC(char *name, fh_timer_callback_t cb, void *data)
{
	fh_timer_t *t = FH_ALLOC(sizeof(*t));

	if (!t) {
		FH_ERROR("Cannot allocate memory for timer");
		return NULL;
	}

	t->t = FH_ALLOC(sizeof(*t->t));
	if (!t->t) {
		FH_ERROR("Cannot allocate memory for timer->t");
		goto no_timer;
	}

	t->name = FH_STRDUP(name);
	if (!t->name) {
		FH_ERROR("Cannot allocate memory for timer->name");
		goto no_name;
	}

	t->lock = FH_SPINLOCK_ALLOC();
	if (!t->lock) {
		FH_ERROR("Cannot allocate memory for lock");
		goto no_lock;
	}

	t->scheduled = 0;
	t->t->base = &boot_tvec_bases;
	t->t->expires = jiffies;
	setup_timer(t->t, timer_callback, (unsigned long)t);

	t->cb = cb;
	t->data = data;

	return t;

 no_lock:
	FH_FREE(t->name);
 no_name:
	FH_FREE(t->t);
 no_timer:
	FH_FREE(t);
	return NULL;
}

void FH_TIMER_FREE(fh_timer_t *timer)
{
	fh_irqflags_t flags;

	FH_SPINLOCK_IRQSAVE(timer->lock, &flags);

	if (timer->scheduled) {
		del_timer(timer->t);
		timer->scheduled = 0;
	}

	FH_SPINUNLOCK_IRQRESTORE(timer->lock, flags);
	FH_SPINLOCK_FREE(timer->lock);
	FH_FREE(timer->t);
	FH_FREE(timer->name);
	FH_FREE(timer);
}

void FH_TIMER_SCHEDULE(fh_timer_t *timer, uint32_t time)
{
	fh_irqflags_t flags;

	FH_SPINLOCK_IRQSAVE(timer->lock, &flags);

	if (!timer->scheduled) {
		timer->scheduled = 1;
		FH_DEBUG("Scheduling timer %s to expire in +%d msec", timer->name, time);
		timer->t->expires = jiffies + msecs_to_jiffies(time);
		add_timer(timer->t);
	} else {
		FH_DEBUG("Modifying timer %s to expire in +%d msec", timer->name, time);
		mod_timer(timer->t, jiffies + msecs_to_jiffies(time));
	}

	FH_SPINUNLOCK_IRQRESTORE(timer->lock, flags);
}

void FH_TIMER_CANCEL(fh_timer_t *timer)
{
	del_timer(timer->t);
}


/* Wait Queues */

struct fh_waitq {
	wait_queue_head_t queue;
	int abort;
};

fh_waitq_t *FH_WAITQ_ALLOC(void)
{
	fh_waitq_t *wq = FH_ALLOC(sizeof(*wq));

	if (!wq) {
		FH_ERROR("Cannot allocate memory for waitqueue\n");
		return NULL;
	}

	init_waitqueue_head(&wq->queue);
	wq->abort = 0;
	return wq;
}

void FH_WAITQ_FREE(fh_waitq_t *wq)
{
	FH_FREE(wq);
}

int32_t FH_WAITQ_WAIT(fh_waitq_t *wq, fh_waitq_condition_t cond, void *data)
{
	int result = wait_event_interruptible(wq->queue,
					      cond(data) || wq->abort);
	if (result == -ERESTARTSYS) {
		wq->abort = 0;
		return -FH_E_RESTART;
	}

	if (wq->abort == 1) {
		wq->abort = 0;
		return -FH_E_ABORT;
	}

	wq->abort = 0;

	if (result == 0) {
		return 0;
	}

	return -FH_E_UNKNOWN;
}

int32_t FH_WAITQ_WAIT_TIMEOUT(fh_waitq_t *wq, fh_waitq_condition_t cond,
			       void *data, int32_t msecs)
{
	int32_t tmsecs;
	int result = wait_event_interruptible_timeout(wq->queue,
						      cond(data) || wq->abort,
						      msecs_to_jiffies(msecs));
	if (result == -ERESTARTSYS) {
		wq->abort = 0;
		return -FH_E_RESTART;
	}

	if (wq->abort == 1) {
		wq->abort = 0;
		return -FH_E_ABORT;
	}

	wq->abort = 0;

	if (result > 0) {
		tmsecs = jiffies_to_msecs(result);
		if (!tmsecs) {
			return 1;
		}

		return tmsecs;
	}

	if (result == 0) {
		return -FH_E_TIMEOUT;
	}

	return -FH_E_UNKNOWN;
}

void FH_WAITQ_TRIGGER(fh_waitq_t *wq)
{
	wq->abort = 0;
	wake_up_interruptible(&wq->queue);
}

void FH_WAITQ_ABORT(fh_waitq_t *wq)
{
	wq->abort = 1;
	wake_up_interruptible(&wq->queue);
}


/* Threading */

fh_thread_t *FH_THREAD_RUN(fh_thread_function_t func, char *name, void *data)
{
	struct task_struct *thread = kthread_run(func, data, name);

	if (thread == ERR_PTR(-ENOMEM)) {
		return NULL;
	}

	return (fh_thread_t *)thread;
}

int FH_THREAD_STOP(fh_thread_t *thread)
{
	return kthread_stop((struct task_struct *)thread);
}

fh_bool_t FH_THREAD_SHOULD_STOP(void)
{
	return kthread_should_stop();
}


/* tasklets
 - run in interrupt context (cannot sleep)
 - each tasklet runs on a single CPU
 - different tasklets can be running simultaneously on different CPUs
 */
struct fh_tasklet {
	struct tasklet_struct t;
	fh_tasklet_callback_t cb;
	void *data;
};

static void tasklet_callback(unsigned long data)
{
	fh_tasklet_t *t = (fh_tasklet_t *)data;
	t->cb(t->data);
}

fh_tasklet_t *FH_TASK_ALLOC(char *name, fh_tasklet_callback_t cb, void *data)
{
	fh_tasklet_t *t = FH_ALLOC(sizeof(*t));

	if (t) {
		t->cb = cb;
		t->data = data;
		tasklet_init(&t->t, tasklet_callback, (unsigned long)t);
	} else {
		FH_ERROR("Cannot allocate memory for tasklet\n");
	}

	return t;
}

void FH_TASK_FREE(fh_tasklet_t *task)
{
	FH_FREE(task);
}

void FH_TASK_SCHEDULE(fh_tasklet_t *task)
{
	tasklet_schedule(&task->t);
}


/* workqueues
 - run in process context (can sleep)
 */
typedef struct work_container {
	fh_work_callback_t cb;
	void *data;
	fh_workq_t *wq;
	char *name;

#ifdef DEBUG
	FH_CIRCLEQ_ENTRY(work_container) entry;
#endif
	struct delayed_work work;
} work_container_t;

#ifdef DEBUG
FH_CIRCLEQ_HEAD(work_container_queue, work_container);
#endif

struct fh_workq {
	struct workqueue_struct *wq;
	fh_spinlock_t *lock;
	fh_waitq_t *waitq;
	int pending;

#ifdef DEBUG
	struct work_container_queue entries;
#endif
};

static void do_work(struct work_struct *work)
{
	fh_irqflags_t flags;
	struct delayed_work *dw = container_of(work, struct delayed_work, work);
	work_container_t *container = container_of(dw, struct work_container, work);
	fh_workq_t *wq = container->wq;

	container->cb(container->data);

#ifdef DEBUG
	FH_CIRCLEQ_REMOVE(&wq->entries, container, entry);
#endif
	FH_DEBUG("Work done: %s, container=%p", container->name, container);
	if (container->name) {
		FH_FREE(container->name);
	}
	FH_FREE(container);

	FH_SPINLOCK_IRQSAVE(wq->lock, &flags);
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
	fh_workq_t *wq = FH_ALLOC(sizeof(*wq));

	if (!wq) {
		return NULL;
	}

	wq->wq = create_singlethread_workqueue(name);
	if (!wq->wq) {
		goto no_wq;
	}

	wq->pending = 0;

	wq->lock = FH_SPINLOCK_ALLOC();
	if (!wq->lock) {
		goto no_lock;
	}

	wq->waitq = FH_WAITQ_ALLOC();
	if (!wq->waitq) {
		goto no_waitq;
	}

#ifdef DEBUG
	FH_CIRCLEQ_INIT(&wq->entries);
#endif
	return wq;

 no_waitq:
	FH_SPINLOCK_FREE(wq->lock);
 no_lock:
	destroy_workqueue(wq->wq);
 no_wq:
	FH_FREE(wq);

	return NULL;
}

void FH_WORKQ_FREE(fh_workq_t *wq)
{
#ifdef DEBUG
	if (wq->pending != 0) {
		struct work_container *wc;
		FH_ERROR("Destroying work queue with pending work");
		FH_CIRCLEQ_FOREACH(wc, &wq->entries, entry) {
			FH_ERROR("Work %s still pending", wc->name);
		}
	}
#endif
	destroy_workqueue(wq->wq);
	FH_SPINLOCK_FREE(wq->lock);
	FH_WAITQ_FREE(wq->waitq);
	FH_FREE(wq);
}
bool FH_SCHEDULE_SYSTEM_WORK(struct work_struct *work){
	
	 return queue_work(system_wq, work);
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
		FH_ERROR("Cannot allocate memory for container\n");
		return;
	}

	container->name = FH_STRDUP(name);
	if (!container->name) {
		FH_ERROR("Cannot allocate memory for container->name\n");
		FH_FREE(container);
		return;
	}

	container->cb = cb;
	container->data = data;
	container->wq = wq;
	FH_DEBUG("Queueing work: %s, container=%p", container->name, container);
	INIT_WORK(&container->work.work, do_work);

#ifdef DEBUG
	FH_CIRCLEQ_INSERT_TAIL(&wq->entries, container, entry);
#endif
	queue_work(wq->wq, &container->work.work);
}

void FH_WORKQ_SCHEDULE_DELAYED(fh_workq_t *wq, fh_work_callback_t cb,
				void *data, uint32_t time, char *format, ...)
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
		FH_ERROR("Cannot allocate memory for container\n");
		return;
	}

	container->name = FH_STRDUP(name);
	if (!container->name) {
		FH_ERROR("Cannot allocate memory for container->name\n");
		FH_FREE(container);
		return;
	}

	container->cb = cb;
	container->data = data;
	container->wq = wq;
	FH_DEBUG("Queueing work: %s, container=%p", container->name, container);
	INIT_DELAYED_WORK(&container->work, do_work);

#ifdef DEBUG
	FH_CIRCLEQ_INSERT_TAIL(&wq->entries, container, entry);
#endif
	queue_delayed_work(wq->wq, &container->work, msecs_to_jiffies(time));
}

int FH_WORKQ_PENDING(fh_workq_t *wq)
{
	return wq->pending;
}


#ifdef FH_LIBMODULE

#ifdef FH_CCLIB
/* CC */
EXPORT_SYMBOL(fh_cc_if_alloc);
EXPORT_SYMBOL(fh_cc_if_free);
EXPORT_SYMBOL(fh_cc_clear);
EXPORT_SYMBOL(fh_cc_add);
EXPORT_SYMBOL(fh_cc_remove);
EXPORT_SYMBOL(fh_cc_change);
EXPORT_SYMBOL(fh_cc_data_for_save);
EXPORT_SYMBOL(fh_cc_restore_from_data);
EXPORT_SYMBOL(fh_cc_match_chid);
EXPORT_SYMBOL(fh_cc_match_cdid);
EXPORT_SYMBOL(fh_cc_ck);
EXPORT_SYMBOL(fh_cc_chid);
EXPORT_SYMBOL(fh_cc_cdid);
EXPORT_SYMBOL(fh_cc_name);
#endif	/* FH_CCLIB */

#ifdef FH_CRYPTOLIB
# ifndef CONFIG_MACH_IPMATE
/* Modpow */
EXPORT_SYMBOL(fh_modpow);

/* DH */
EXPORT_SYMBOL(fh_dh_modpow);
EXPORT_SYMBOL(fh_dh_derive_keys);
EXPORT_SYMBOL(fh_dh_pk);
# endif	/* CONFIG_MACH_IPMATE */

/* Crypto */
EXPORT_SYMBOL(fh_wusb_aes_encrypt);
EXPORT_SYMBOL(fh_wusb_cmf);
EXPORT_SYMBOL(fh_wusb_prf);
EXPORT_SYMBOL(fh_wusb_fill_ccm_nonce);
EXPORT_SYMBOL(fh_wusb_gen_nonce);
EXPORT_SYMBOL(fh_wusb_gen_key);
EXPORT_SYMBOL(fh_wusb_gen_mic);
#endif	/* FH_CRYPTOLIB */

/* Notification */
#ifdef FH_NOTIFYLIB
EXPORT_SYMBOL(fh_alloc_notification_manager);
EXPORT_SYMBOL(fh_free_notification_manager);
EXPORT_SYMBOL(fh_register_notifier);
EXPORT_SYMBOL(fh_unregister_notifier);
EXPORT_SYMBOL(fh_add_observer);
EXPORT_SYMBOL(fh_remove_observer);
EXPORT_SYMBOL(fh_notify);
#endif

/* Memory Debugging Routines */
#ifdef FH_DEBUG_MEMORY
EXPORT_SYMBOL(fh_alloc_debug);
EXPORT_SYMBOL(fh_alloc_atomic_debug);
EXPORT_SYMBOL(fh_free_debug);
EXPORT_SYMBOL(fh_dma_alloc_debug);
EXPORT_SYMBOL(fh_dma_free_debug);
#endif

EXPORT_SYMBOL(FH_MEMSET);
EXPORT_SYMBOL(FH_MEMCPY);
EXPORT_SYMBOL(FH_MEMMOVE);
EXPORT_SYMBOL(FH_MEMCMP);
EXPORT_SYMBOL(FH_STRNCMP);
EXPORT_SYMBOL(FH_STRCMP);
EXPORT_SYMBOL(FH_STRLEN);
EXPORT_SYMBOL(FH_STRCPY);
EXPORT_SYMBOL(FH_STRDUP);
EXPORT_SYMBOL(FH_ATOI);
EXPORT_SYMBOL(FH_ATOUI);

#ifdef FH_UTFLIB
EXPORT_SYMBOL(FH_UTF8_TO_UTF16LE);
#endif	/* FH_UTFLIB */

EXPORT_SYMBOL(FH_IN_IRQ);
EXPORT_SYMBOL(FH_IN_BH);
EXPORT_SYMBOL(FH_VPRINTF);
EXPORT_SYMBOL(FH_VSNPRINTF);
EXPORT_SYMBOL(FH_PRINTF);
EXPORT_SYMBOL(FH_SPRINTF);
EXPORT_SYMBOL(FH_SNPRINTF);
EXPORT_SYMBOL(__FH_WARN);
EXPORT_SYMBOL(__FH_ERROR);
EXPORT_SYMBOL(FH_EXCEPTION);

#ifdef DEBUG
EXPORT_SYMBOL(__FH_DEBUG);
#endif

EXPORT_SYMBOL(__FH_DMA_ALLOC);
EXPORT_SYMBOL(__FH_DMA_ALLOC_ATOMIC);
EXPORT_SYMBOL(__FH_DMA_FREE);
EXPORT_SYMBOL(__FH_ALLOC);
EXPORT_SYMBOL(__FH_ALLOC_ATOMIC);
EXPORT_SYMBOL(__FH_FREE);

#ifdef FH_CRYPTOLIB
EXPORT_SYMBOL(FH_RANDOM_BYTES);
EXPORT_SYMBOL(FH_AES_CBC);
EXPORT_SYMBOL(FH_SHA256);
EXPORT_SYMBOL(FH_HMAC_SHA256);
#endif

EXPORT_SYMBOL(FH_CPU_TO_LE32);
EXPORT_SYMBOL(FH_CPU_TO_BE32);
EXPORT_SYMBOL(FH_LE32_TO_CPU);
EXPORT_SYMBOL(FH_BE32_TO_CPU);
EXPORT_SYMBOL(FH_CPU_TO_LE16);
EXPORT_SYMBOL(FH_CPU_TO_BE16);
EXPORT_SYMBOL(FH_LE16_TO_CPU);
EXPORT_SYMBOL(FH_BE16_TO_CPU);
EXPORT_SYMBOL(FH_READ_REG32);
EXPORT_SYMBOL(FH_WRITE_REG32);
EXPORT_SYMBOL(FH_MODIFY_REG32);

#if 0
EXPORT_SYMBOL(FH_READ_REG64);
EXPORT_SYMBOL(FH_WRITE_REG64);
EXPORT_SYMBOL(FH_MODIFY_REG64);
#endif

EXPORT_SYMBOL(FH_SPINLOCK_ALLOC);
EXPORT_SYMBOL(FH_SPINLOCK_FREE);
EXPORT_SYMBOL(FH_SPINLOCK);
EXPORT_SYMBOL(FH_SPINUNLOCK);
EXPORT_SYMBOL(FH_SPINLOCK_IRQSAVE);
EXPORT_SYMBOL(FH_SPINUNLOCK_IRQRESTORE);
EXPORT_SYMBOL(FH_MUTEX_ALLOC);

#if (!defined(FH_LINUX) || !defined(CONFIG_DEBUG_MUTEXES))
EXPORT_SYMBOL(FH_MUTEX_FREE);
#endif

EXPORT_SYMBOL(FH_MUTEX_LOCK);
EXPORT_SYMBOL(FH_MUTEX_TRYLOCK);
EXPORT_SYMBOL(FH_MUTEX_UNLOCK);
EXPORT_SYMBOL(FH_UDELAY);
EXPORT_SYMBOL(FH_MDELAY);
EXPORT_SYMBOL(FH_MSLEEP);
EXPORT_SYMBOL(FH_TIME);
EXPORT_SYMBOL(FH_TIMER_ALLOC);
EXPORT_SYMBOL(FH_TIMER_FREE);
EXPORT_SYMBOL(FH_TIMER_SCHEDULE);
EXPORT_SYMBOL(FH_TIMER_CANCEL);
EXPORT_SYMBOL(FH_WAITQ_ALLOC);
EXPORT_SYMBOL(FH_WAITQ_FREE);
EXPORT_SYMBOL(FH_WAITQ_WAIT);
EXPORT_SYMBOL(FH_WAITQ_WAIT_TIMEOUT);
EXPORT_SYMBOL(FH_WAITQ_TRIGGER);
EXPORT_SYMBOL(FH_WAITQ_ABORT);
EXPORT_SYMBOL(FH_THREAD_RUN);
EXPORT_SYMBOL(FH_THREAD_STOP);
EXPORT_SYMBOL(FH_THREAD_SHOULD_STOP);
EXPORT_SYMBOL(FH_TASK_ALLOC);
EXPORT_SYMBOL(FH_TASK_FREE);
EXPORT_SYMBOL(FH_TASK_SCHEDULE);
EXPORT_SYMBOL(FH_WORKQ_WAIT_WORK_DONE);
EXPORT_SYMBOL(FH_WORKQ_ALLOC);
EXPORT_SYMBOL(FH_WORKQ_FREE);
EXPORT_SYMBOL(FH_SCHEDULE_SYSTEM_WORK);
EXPORT_SYMBOL(FH_WORKQ_SCHEDULE);
EXPORT_SYMBOL(FH_WORKQ_SCHEDULE_DELAYED);
EXPORT_SYMBOL(FH_WORKQ_PENDING);

static int fh_common_port_init_module(void)
{
	int result = 0;

	printk(KERN_DEBUG "Module fh_common_port init\n" );

#ifdef FH_DEBUG_MEMORY
	result = fh_memory_debug_start(NULL);
	if (result) {
		printk(KERN_ERR
		       "fh_memory_debug_start() failed with error %d\n",
		       result);
		return result;
	}
#endif

#ifdef FH_NOTIFYLIB
	result = fh_alloc_notification_manager(NULL, NULL);
	if (result) {
		printk(KERN_ERR
		       "fh_alloc_notification_manager() failed with error %d\n",
		       result);
		return result;
	}
#endif
	return result;
}

static void fh_common_port_exit_module(void)
{
	printk(KERN_DEBUG "Module fh_common_port exit\n" );

#ifdef FH_NOTIFYLIB
	fh_free_notification_manager();
#endif

#ifdef FH_DEBUG_MEMORY
	fh_memory_debug_stop();
#endif
}

module_init(fh_common_port_init_module);
module_exit(fh_common_port_exit_module);

MODULE_DESCRIPTION("FH Common Library - Portable version");
MODULE_AUTHOR("Synopsys Inc.");
MODULE_LICENSE ("GPL");

#endif	/* FH_LIBMODULE */
