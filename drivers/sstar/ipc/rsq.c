/*
* rsq.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: Canlet.Lin <Canlet.Lin@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
/*
 * rsq.c
 */
#include "types.h"
#include "rsq.h"
#include "string.h"

#define	RQ_TAG			(0xfeeffe2f)

#define	ATOMIC_BEGIN
#define	ATOMIC_END

#ifndef	READ_ONCE
#define	READ_ONCE(x)	({ volatile typeof(x) __val = (x); __val; })
#endif
#define	CHECK_CONSISTENT(r)	(READ_ONCE((r)->ss.reset_id) == (r)->ns.reset_id)

/*
 **NOTE**: RQ_ALIGN size should be larger or equal than sizeof(slot_t)!!
 */
#ifdef	_LINUX_
#include <linux/printk.h>
#define	UartSendTrace(x, args...)	printk(x, ##args)
#define	MsHMEMPA2VA(x)	(x)
#define sys_Invalidate_data_cache_buffer(addr, size)
#define sys_flush_data_cache_buffer(addr, size)
#else
signed char UartSendTrace(const char *fmt, ...);
#define	printk(x, args...)	UartSendTrace(x, ##args)
#define	KERN_ERR
void * MsHMEMPA2VA(void *pMem);
void sys_Invalidate_data_cache_buffer(u32 addr, s32 size);
void sys_flush_data_cache_buffer(u32 addr, s32 size);
#endif

/* rsq/int usage map,
 * each rsq with an interrupt (SGI/IPI) to send irq to NS.
 * bit is 0 for free, 1 is occupied.
 * Total number of SGI is 16 for CA7, linux occupied 0~7.
 * RSQ can use 8~15
 */
static int _rsqid_ = 0x00ffffff;
/*
 * init_rsq
 */
void *init_rsq(void* rqbuf, unsigned int size, unsigned int oobsize, char *name, int datatype)
{
	reservoir_t	*rvr;
	rsq_t		*prsq;

	if (rqbuf == NULL || size == 0 || _rsqid_ == -1)
		return NULL;
	/* share memory needs locate at 4K boundary */
	rvr       = (reservoir_t*)ALIGN4K(rqbuf);
	rvr->size = size - ((u32)rvr - (u32)rqbuf);
	if (rvr->size < (SIZEOF_RESERVIOR + oobsize))
		return NULL;
	rvr->me   = rvr;
	rvr->slotno  = 0;
	rvr->headno  = 0;
	rvr->dropcnt = 0;
	rvr->dropfrms = 0;
	rvr->corrfrms = 0;
	rvr->sloterr  = 0;
	rvr->synccnt  = 0;
	rvr->tran     = 0;
	rvr->resetcnt = 0;
	rvr->datatype = datatype;
	rvr->iid      = 8 - __builtin_clz(_rsqid_);
	_rsqid_      |= (1 << (24 + rvr->iid));
	strncpy(rvr->name, name, sizeof(rvr->name));
	init_rlink(&(rvr->link));
	/* int ss ns */
	rvr->ss.reset_id = 1;
	rvr->ns.reset_id = 0;
	/* init ring buffer */
	prsq = &(rvr->ss);
	prsq->woff = 0;
	prsq->sbuf = (unsigned char*)rvr + SIZEOF_RESERVIOR;
	prsq->end  = (unsigned char*)rvr + size;
	prsq->oob  = prsq->end - oobsize;
	/* init write and read pointer */
	prsq->wp = prsq->sbuf;
	prsq->rp = prsq->sbuf;
	prsq->wrno =
	prsq->rdno = rvr->slotno;
	prsq->head =
	prsq->tail = NULL;
	return rvr;
}

#define	RSQ_WORLD_TRANSFER(p, r)	(typeof(p))((unsigned int)(p) + (r)->woff)
void init_nsrsq(reservoir_t *rvr)
{
	rsq_t	*ns;
	rsq_t	*ss;

	ns = &(rvr->ns);
	ss = &(rvr->ss);
	ns->woff = (void*)rvr - (void*)rvr->me;
	ns->sbuf = RSQ_WORLD_TRANSFER(ss->sbuf, ns);
	ns->wp   = RSQ_WORLD_TRANSFER(ss->wp,   ns);
	ns->rp	 = RSQ_WORLD_TRANSFER(ss->rp,   ns);
	ns->oob  = RSQ_WORLD_TRANSFER(ss->oob,  ns);
	ns->end  = RSQ_WORLD_TRANSFER(ss->end,  ns);
	ns->head = ss->head;
	rvr->tran = 1;
}

void reset_rsq(reservoir_t *rvr)
{
	reservoir_t	*nrvr;
	rsq_t	*prsq;
	slot_t	*slot;

	/* */
	prsq = &(rvr->ss);
	prsq->head =
	prsq->tail = NULL;
	/* init write and read pointer */
	prsq->wp = prsq->sbuf;
	prsq->rp = prsq->sbuf;
	prsq->wrno =
	prsq->rdno = rvr->slotno;
	/* init size */
	/* init sequeuce slot queue */
	slot = (slot_t*)prsq->wp;
	slot->tag = 0;
	rvr->resetcnt++;
	// flush
	nrvr = (reservoir_t*)MsHMEMPA2VA(rvr);
	nrvr->resetcnt = rvr->resetcnt;
}

/* allocate a slot for unknow size writting */
/* alloc_rsqslot -> get slot size of buffer */
/* write data to buffer and get how many size to written */
/* trim_rsqslot -> update the size in slot_t */
void *begin_rsqslot(reservoir_t *rvr, unsigned int expect)
{
	rsq_t			*prsq;
	slot_t			*slot;
	unsigned int	nws;
	unsigned char	*rp;
	unsigned char	*ptr;

	if (!rvr)
		return NULL;
	sys_Invalidate_data_cache_buffer((u32)rvr, sizeof(reservoir_t));
	prsq  = &(rvr->ss);
	if ((rvr->tran) &&
		(rvr->ss.reset_id == rvr->ns.reset_id) &&
		(prsq->rdno       != rvr->ns.rdno)) {
		prsq->rp   = rvr->ns.rp - rvr->ns.woff;
		prsq->head = rvr->ns.head;
		prsq->rdno = rvr->ns.rdno;
	}
	rp  = prsq->rp;
	ptr = prsq->wp;
	/* need total written size */
	nws = SIZEOF_SLOTHEADER + expect;
	slot = (slot_t*)rp;
	if ((ptr <= rp) && (prsq->rdno != prsq->wrno) && ((rp - ptr) < nws)) {
		/* |-------WsssssRss----O----------| */
		/* sbuf   ptr          oob           */
		/* FULL!! TODO: drop a group slots */
		unsigned int	df = 0;
		rvr->dropcnt++;
		do {
			slot_t	*s = (slot_t*)(prsq->rp);
			if (s->mark)
				prsq->head = s->next;
			if (prsq->head == NULL) {
				/* buffer too small, cannot store one group!! */
				/* Reset QSQ */
				printk(KERN_ERR "NOT enough!! RESET RSQ wp/rp %x/%x, wrno/rdno %u/%u, rp - ptr = %d/%d\n",
								(unsigned int)ptr, (unsigned int)rp, prsq->wrno, prsq->rdno, (int)(rp - ptr), nws);
				reset_rsq(rvr);
				ptr  = prsq->wp;
				rp   = prsq->rp;
				slot = (slot_t*)rp;
				df += (rvr->slotno - s->no + 1);
			} else {
				rp		 =
				prsq->rp = (unsigned char*)prsq->head;
				slot     = (slot_t*)rp;
				/* for check only */
				if (slot->tag != RQ_TAG) {
					printk(KERN_ERR "DO DROP 0x%x mark %d to 0x%x %s\n",
									(unsigned int)s, s->mark, (unsigned int)prsq->rp,
									slot->tag == RQ_TAG? "OK" : "BAD");
				}
				df += (slot->no - s->no);
				prsq->rdno  =
				rvr->headno = slot->no;		/* oldest no. in rvr */
			}
			if (ptr >= rp || (rp - ptr) >= nws) {
				prsq->reset_id = rvr->ns.reset_id + 1;
				rvr->dropfrms += df;
				break;
			}
			// printk(KERN_ERR "%s %d new - rp %x\n", __FUNCTION__, __LINE__, (unsigned int)prsq->rp);
		} while (1);
	}
	// flush to physical storage
	sys_flush_data_cache_buffer((u32)rvr, sizeof(reservoir_t));
	//
	slot = (slot_t*)(prsq->wp);
	sys_Invalidate_data_cache_buffer((u32)slot, sizeof(slot_t));
	slot->rese = rvr;
	sys_flush_data_cache_buffer((u32)slot, sizeof(slot_t));
	/* |-------Wssssssss-R--O----------|  */
	/* sbuf   ptr        ^ oob        end */

	/* |----R---Wsssssss----O----------|  */
	/* sbuf ^  ptr         oob        end */
	ptr += SIZEOF_SLOTHEADER;
	return ptr;
}

/* trim slot size after data in el */
unsigned int end_rsqslot(void* wp, unsigned int ws, unsigned int mark, unsigned int ts)
{
	reservoir_t 	*rvr;
	rsq_t			*prsq;
	slot_t			*slot;
	unsigned char	*ptr;

	if (!wp) return 0;
	ptr  = wp;
	slot = (slot_t*)(ptr - SIZEOF_SLOTHEADER);
	sys_Invalidate_data_cache_buffer((u32)slot, sizeof(slot_t));
	rvr  = slot->rese;
	sys_Invalidate_data_cache_buffer((u32)rvr, sizeof(reservoir_t));
	prsq = &(rvr->ss);

	/* for debug/verify only */
	if ((void*)slot != prsq->wp) {
		/* wp is invalid */
		printk(KERN_ERR "ERROR: %s %d\n", __FUNCTION__, __LINE__);
		return 0;
	}
	slot->tag  = RQ_TAG;			/* a tag for allocated */
	slot->no   = ++rvr->slotno;
	slot->dc   = ws;				/* real size of data */
	slot->bs   = ALIGN32(ws);		/* the slot allocated size */
	slot->sc   = SIZEOF_SLOTHEADER;	/* slot header size */
	slot->next = NULL;
	slot->mark = mark;
	slot->ts   = ts;
	ptr += slot->bs;							/* move write pointer for next writting */
	if ((prsq->wp < prsq->rp && ptr > prsq->rp) ||
		(ptr > prsq->end)) {
		/* |-----------WdddRddp-O----------|  */
		/* |----------------WdddOdddddddddddp */
		/* overwrite!! */
		/* TODO: expect in begin_rsqslot too small !! */
		printk(KERN_ERR "%s %d RSQ OVERWRITE!!\n", __FUNCTION__, __LINE__);
		return 0;
	}
	/* update write pointer */
	if (ptr >= prsq->oob) {
		/* |--R---------------WdOddddp-----|  */
		/* data in oob, wrap to head of buffer*/
		prsq->wp = prsq->sbuf;
	} else {
		/* |--R--------Wddddp-R-O----------|  */
		prsq->wp = ptr;
	}
	if (mark) {
		/* update group list for dropping */
		if (prsq->tail) {
			if (!prsq->head) prsq->head = slot;
			prsq->tail->next = slot;
			prsq->tail = slot;
		} else {
			prsq->tail = prsq->head = slot;
		}
	}
	prsq->wrno = slot->no;
	// flush to pyhsical storage
	sys_flush_data_cache_buffer((u32)rvr, sizeof(reservoir_t));
	sys_flush_data_cache_buffer((u32)slot, sizeof(slot_t));
	return ws;
}

void sync_rsq(reservoir_t *rvr)
{
	rsq_t	*ns;
	rsq_t	*ss;

	ns = &(rvr->ns);
	ss = &(rvr->ss);
	ns->wp   = RSQ_WORLD_TRANSFER(ss->wp,   ns);
	ns->rp	 = RSQ_WORLD_TRANSFER(ss->rp,   ns);
	ns->wrno = ss->wrno;
	ns->rdno = ss->rdno;
	ns->reset_id = ss->reset_id;
	rvr->synccnt++;
}

/*
 * rvr should be a noncache port (device memory)
 */
unsigned int de_rsqslot(reservoir_t *rvr, void *buf, unsigned int size, slot_t *out)
{
	rsq_t			*prsq;
	unsigned char	*rp;

	prsq = (rvr != rvr->me)? &(rvr->ns) : &(rvr->ss);
	do {
		do {
			if (prsq->rdno == rvr->ss.wrno) {
				out->buf = rvr->ns.sbuf + out->sc;
				return 0;	// empty!!
			}
			prsq->wrno = rvr->ss.wrno;
			memcpy(out, prsq->rp, sizeof(slot_t));
			rp  = prsq->rp + out->sc;
			if (CHECK_CONSISTENT(rvr)) {
				break;
			}
			/* data flushed, by producer */
			ATOMIC_BEGIN /* SYSC ss and ns */
				sync_rsq(rvr);
			ATOMIC_END
		} while (1);
		/* for debuging, can remove!! */
		if (out->tag != RQ_TAG) {
			// printk(KERN_ERR "Not Slot header 0x%08x\n", (int)prsq->rp);
			rvr->sloterr++;
			out->buf = NULL;
			return 0;
		}
		if (buf && size > out->dc) {
			/* enough to copy */
			memcpy(buf, rp, out->dc);
			rp += out->bs;
			prsq->rp = (rp >= prsq->oob)? prsq->sbuf : rp;
		} else {
			printk(KERN_ERR "buffer size %x return %p\n", size, rp);
			out->buf = rp;
			rp += out->bs;
			prsq->rp = (rp >= prsq->oob)? prsq->sbuf : rp;
			return 0;
		}
		if (CHECK_CONSISTENT(rvr))
			break;
		ATOMIC_BEGIN /* SYSC ss and ns */
			sync_rsq(rvr);
		ATOMIC_END
		printk(KERN_ERR "rp %x %x\n", (unsigned int)prsq->rp, (unsigned int)rvr->ns.rp);
	} while (1);
	if (out->mark) {
		/* update head of marker, head is presented as ss domain and for dropping frames case */
		prsq->head = out->next;
	}
	prsq->rdno = out->no;
	return out->dc;
}

/* ghost rsqslot
 * not to move read ptr
 */
unsigned int de_ghost_rsqslot(reservoir_t *rvr, void *buf, unsigned int size, gslot_t *out)
{
	rsq_t			*prsq;
	unsigned char	*rp;
	slot_t			slot;

	prsq = (rvr != rvr->me)? &(rvr->ns) : &(rvr->ss);
	do {
		do {
			if (prsq->rdno == rvr->ss.wrno) return 0;	/* queue is empty */
			prsq->wrno = rvr->ss.wrno;
			if (out->ghost == NULL) {
				out->ghost = (rvr->ss.head)? RSQ_WORLD_TRANSFER(rvr->ss.head, &(rvr->ns)) :
											 RSQ_WORLD_TRANSFER(rvr->ss.tail, &(rvr->ns));
			} else {
				if (prsq->wrno > prsq->rdno) {
					/* |------RggggggggggW----| */
					if (out->copy.no > rvr->slotno || out->copy.no < rvr->headno)
						out->ghost = (rvr->ss.head)? RSQ_WORLD_TRANSFER(rvr->ss.head, &(rvr->ns)) :
													 RSQ_WORLD_TRANSFER(rvr->ss.tail, &(rvr->ns));
				} else {
					/* |gggW---------------Rgg| */
					if (out->copy.no > prsq->wrno && out->copy.no <= prsq->rdno)
						out->ghost = (rvr->ss.head)? RSQ_WORLD_TRANSFER(rvr->ss.head, &(rvr->ns)) :
													 RSQ_WORLD_TRANSFER(rvr->ss.tail, &(rvr->ns));
				}
				if (out->copy.no == prsq->wrno)
					return 0;	/* ghost is up to top */
			}
			memcpy(&slot, out->ghost, sizeof(slot_t));
			rp  = (unsigned char*)out->ghost + slot.sc;
			if (CHECK_CONSISTENT(rvr)) {
				break;
			}
			/* data flushed, by producer */
			ATOMIC_BEGIN
				sync_rsq(rvr); /* SYNC ss and ns */
			ATOMIC_END
		} while (1);
		/* for debuging, can remove!! */
		if (slot.tag != RQ_TAG) { printk(KERN_ERR "Not Slot header wrno %d gno %d!!\n", prsq->wrno, out->copy.no);
								  out->ghost = NULL; continue; }
		/* */
		if (buf && size > slot.dc) {
			/* enough to copy */
			memcpy(buf, rp, slot.dc);
			/* for ghost, not to update read ptr */
			rp += slot.bs;
		} else {
			printk(KERN_ERR "buffer size %dK is Too Small(%dK)!!\n", size >> 10, out->copy.dc);
			return 0;
		}
		if (CHECK_CONSISTENT(rvr))
			break;
		ATOMIC_BEGIN
			sync_rsq(rvr); /* SYNC ss and ns */
		ATOMIC_END
	} while (1);
	out->ghost = (slot_t*)((rp >= prsq->oob)? prsq->sbuf : rp);
	memcpy(&(out->copy), &slot, sizeof(slot_t));
	return out->copy.dc;
}

void cnt_rsqerr(void *p)
{
	reservoir_t 	*rvr, *nrvr;

	rvr = (reservoir_t*)p;
	rvr->corrfrms++;
	// flush
	nrvr = (reservoir_t*)MsHMEMPA2VA(rvr);
	nrvr->corrfrms = rvr->corrfrms;
}

void corrupted_frame(void *p)
{
	reservoir_t 	*rvr, *nrvr;
	slot_t			*slot;
	unsigned char	*ptr;

	if (!p) return;
	ptr  = p;
	slot = (slot_t*)(ptr - SIZEOF_SLOTHEADER);
	rvr  = slot->rese;
	rvr->corrfrms++;
	// flush
	nrvr = (reservoir_t*)MsHMEMPA2VA(rvr);
	nrvr->corrfrms = rvr->corrfrms;
}

slot_t *get_srqslot(void *databuf)
{
	slot_t*	slot;

	slot = (slot_t*)((unsigned char*)databuf - SIZEOF_SLOTHEADER);
	if (slot->tag == RQ_TAG)
		return slot;
	return NULL;
}
