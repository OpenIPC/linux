#ifndef __OS_INT_F_H__
#define __OS_INT_F_H__

#ifdef CONFIG_PLATFORM_LINUX
#define PLATFORM_LINUX 1
#endif

#if PLATFORM_LINUX
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/kthread.h>
#include <linux/unistd.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/list.h>

#define xradio_printf(fmt, arg...) printk(KERN_ERR fmt, ##arg)

#define SKB_DATA_ADDR_ALIGNMENT 4

#define INTERFACE_HEADER_PADDING (SKB_DATA_ADDR_ALIGNMENT * 4)

typedef int (*xr_thread_func_t)(void *arg);

typedef struct {
	struct task_struct *handle;
} xr_thread_handle_t;

typedef struct {
	atomic_t h;
} xr_atomic_t;

typedef struct {
	spinlock_t lock;
} xr_spinlock_t;
#endif

void xradio_k_msleep(u32 ms);

void xradio_k_usleep(u32 us);

void xradio_k_udelay(u32 us);

void xradio_k_mdely(u32 ms);

void xradio_k_spin_lock_init(xr_spinlock_t *l);

void xradio_k_spin_lock_bh(xr_spinlock_t *l);

void xradio_k_spin_unlock_bh(xr_spinlock_t *l);

void *xradio_k_zmalloc(u32 sz);

void xradio_k_free(void *p);

int xradio_k_thread_create(xr_thread_handle_t *thread, const char *name, xr_thread_func_t func,
			   void *arg, u8 priority, u32 stack_size);

int xradio_k_thread_should_stop(xr_thread_handle_t *thread);

int xradio_k_thread_delete(xr_thread_handle_t *thread);

void xradio_k_thread_exit(xr_thread_handle_t *thread);

u16 xradio_k_cpu_to_le16(u16 val);

u32 xradio_k_cpu_to_be32(u32 val);

u16 xradio_k_cpu_to_be16(u16 val);

void xradio_k_atomic_add(int i, xr_atomic_t *v);

int xradio_k_atomic_read(xr_atomic_t *v);

void xradio_k_atomic_set(xr_atomic_t *v, int i);

void xradio_k_atomic_dec(xr_atomic_t *v);

struct sk_buff *xradio_alloc_skb(u32 len, const char *func);

void xradio_free_skb(struct sk_buff *skb, const char *func);

void xraido_free_skb_any(struct sk_buff *skb);

#endif
