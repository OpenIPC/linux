#include "os_intf.h"
void xradio_k_msleep(u32 ms)
{
#if PLATFORM_LINUX
	msleep(ms);
#endif
}
void xradio_k_udelay(u32 us)
{
#if PLATFORM_LINUX
	udelay(us);
#endif
}

void xradio_k_mdely(u32 ms)
{
#if PLATFORM_LINUX
	mdelay(ms);
#endif
}

void xradio_k_spin_lock_init(xr_spinlock_t *l)
{
#if PLATFORM_LINUX
	spin_lock_init(&l->lock);
#endif
}

// disable all cpu(smp),soft irq, hw irq
void xradio_k_spin_lock_bh(xr_spinlock_t *l)
{
#if PLATFORM_LINUX
	spin_lock_bh(&l->lock);
#endif
}

void xradio_k_spin_unlock_bh(xr_spinlock_t *l)
{
#if PLATFORM_LINUX
	spin_unlock_bh(&l->lock);
#endif
}

void *xradio_k_zmalloc(u32 sz)
{
#if PLATFORM_LINUX
	return kzalloc(sz, GFP_ATOMIC | GFP_DMA);
#endif
}

void xradio_k_free(void *p)
{
#if PLATFORM_LINUX
	kfree(p);
#endif
}

int xradio_k_thread_create(xr_thread_handle_t *thread, const char *name, xr_thread_func_t func,
			   void *arg, u8 priority, u32 stack_size)
{
#if PLATFORM_LINUX
	thread->handle = kthread_run(func, arg, name);
	if (IS_ERR(thread->handle))
		return -1;
	return 0;
#endif
}

int xradio_k_thread_should_stop(xr_thread_handle_t *thread)
{
#if PLATFORM_LINUX
	return kthread_should_stop();
#endif
}

int xradio_k_thread_delete(xr_thread_handle_t *thread)
{
#if PLATFORM_LINUX
	kthread_stop(thread->handle);
	return 0;
#endif
}

void xradio_k_thread_exit(xr_thread_handle_t *thread)
{
#if PLATFORM_LINUX
	;
#endif
}

u16 xradio_k_cpu_to_le16(u16 val)
{
#if PLATFORM_LINUX
	return cpu_to_le16(val);
#endif
}

u16 xradio_k_cpu_to_be16(u16 val)
{
#if PLATFORM_LINUX
	return cpu_to_be16(val);
#endif
}

u32 xradio_k_cpu_to_be32(u32 val)
{
#if PLATFORM_LINUX
	return cpu_to_be32(val);
#endif
}

void xradio_k_atomic_add(int i, xr_atomic_t *v)
{
#if PLATFORM_LINUX
	atomic_add(i, &v->h);
#endif
}

int xradio_k_atomic_read(xr_atomic_t *v)
{
#if PLATFORM_LINUX
	return atomic_read(&v->h);
#endif
}

void xradio_k_atomic_set(xr_atomic_t *v, int i)
{
#if PLATFORM_LINUX
	atomic_set(&v->h, i);
#endif
}

void xradio_k_atomic_dec(xr_atomic_t *v)
{
#if PLATFORM_LINUX
	atomic_dec(&v->h);
#endif
}

struct sk_buff *xradio_alloc_skb(u32 len, const char *func)
{
#if PLATFORM_LINUX
	struct sk_buff *skb = NULL;

	u8 offset;

	skb = netdev_alloc_skb(NULL, len + INTERFACE_HEADER_PADDING);

	if (skb) {
		/* Align SKB data pointer */
		offset = ((unsigned long)skb->data) & (SKB_DATA_ADDR_ALIGNMENT - 1);

		if (offset)
			skb_reserve(skb, INTERFACE_HEADER_PADDING - offset);
	}
	// printk("m(%s): %pK\n", func, skb);
	return skb;
#endif
}

void xradio_free_skb(struct sk_buff *skb, const char *func)
{
#if PLATFORM_LINUX
	if (skb) {
		// printk("f(%s): %pK\n", func, skb);
		dev_kfree_skb(skb);
	}
#endif
}

void xraido_free_skb_any(struct sk_buff *skb)
{
	if (skb)
		dev_kfree_skb_any(skb);
}
