/*
* ipc.c- Sigmastar
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
 * ipc.c
 *   ipc rsq with LH/RTK
 */
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/ioport.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <asm/compiler.h>
#include <asm/io.h>
#include "rsq.h"
#include "lh_syscall.h"
#include "linux/arm-smccc.h"
#include <linux/module.h>

/* proc */
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/uaccess.h>
#include <linux/list.h>

static int disable_rtos;
module_param(disable_rtos, int, 0644);
MODULE_PARM_DESC(disable_rtos, "Disable RTOS (TTFF) feature");

static unsigned long hello_lh(u32 type, u32 arg1, u32 arg2, u32 arg3);
static struct file *file_open(const char *path, int flags, int rights);
static void file_close(struct file *file);
static int file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size);
static int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size);
static int file_sync(struct file *file);
/* */
struct sstream {
	reservoir_t		*rvr;
	struct resource	*res;
	unsigned int	id;
	void			*private;
};

#define	SHARE_SIZE		(0x1000)
static rtkinfo_t 		*_rtklin;
static struct sstream	_sstr[8] = { { 0 } };

#define	FRAME_SIZE		(1920 * 1088 * 2)
static reservoir_t		*_mst = 0;	// master stream
static struct resource	*_rtkres;
static void				*_frm;
static unsigned int		_frmerr;
static unsigned int		_frmI = 0;
static unsigned int		_frmP = 0;
static unsigned int		_frmcnt = 0;
struct semaphore		_frmrdy;
static int				_start_ = 0;
unsigned int			_ipi[8] = { 0 };
static int				_recpid;

static unsigned short	*_io;
struct rsqcb {
	struct list_head	list;
	int	(*rsqproc)(void *param, void *buf, int size, slot_t* slot);
	void			*private;
};

LIST_HEAD(_rsqcblst);
struct rsqcb	rec_rsq;
struct rsqrec {
	struct rsqcb	cb;
	char			file[32];
	reservoir_t		*rvr;
	unsigned int	frms;
	struct file		*fp;
	long long		off;
};

static int record_cb(void *param, void *buf, int size, slot_t *slot)
{
	struct rsqrec	*rec;

	rec = (struct rsqrec*)param;
	// printk(KERN_ERR "write %s/%03d No.%05d size %u type %d\n", rec->file, rec->frms, slot->no, size, slot->mark);
	rec->off += file_write(rec->fp, rec->off, buf, size);
	rec->frms--;
	if (rec->frms == 0) {
		file_close(rec->fp);
		rec->fp = NULL;
		return 0;
	}
	return size;
}

static int copydata(struct rsqrec *rec)
{
	slot_t			s;

	memset(&s, 0, sizeof(s));
	printk(KERN_ERR "\tdebug\t: wp/rp %p/%p\n", rec->rvr->ns.wp, rec->rvr->ns.rp);
	de_rsqslot(rec->rvr, NULL, 0, (slot_t*)&s);
	printk(KERN_ERR "Write %p/%u to %s\n", s.buf, s.dc, rec->file);
	if (s.buf) {
		rec->off += file_write(rec->fp, rec->off, s.buf, s.dc);
		file_close(rec->fp);
	}
	vfree(rec);
	return 0;
}
/*
 * echo file <filename> <frames> > <proc-rvr-name>
 */
static int start_record(reservoir_t *rvr, const char *file)
{
	struct rsqrec	*rec;
	int				np;

	rec = vmalloc(sizeof(struct rsqrec));
	if (!rec) return 0;
	rec->file[0] = 0;
	np = sscanf(file, "%s %d", rec->file, &rec->frms);
	if (np != 2) {
		printk(KERN_ERR "\tusage: echo file frames > p1\n\texample: echo file /hello.264 30 > p1\n");
		vfree(rec);
		return 0;
	}
	rec->fp  = file_open(rec->file, O_CREAT | O_RDWR | O_NONBLOCK, 0777);
	if (rec->fp == NULL) {
		printk(KERN_ERR "\tcreate file - '%s' failed\n", rec->file);
		vfree(rec);
		return 0;
	}
	rec->off = 0;
	rec->rvr = rvr;
	rec->cb.private = rec;
	if (rvr->datatype == DATATYPE_VIDEO) {
		int		empty;

		rec->cb.rsqproc = record_cb;
		INIT_LIST_HEAD((struct list_head *)rec);
		empty = list_empty(&_rsqcblst);
		list_add((struct list_head *)rec, &_rsqcblst);
		/* when no file in rsqcb list, to trig _frmrdy to check and save if any */
		if (empty)
			up(&_frmrdy);
	} else if (rvr->datatype == DATATYPE_STILL) {
		copydata(rec);
	}
	return 1;
}

static int c_show(struct seq_file *m, void *v)
{
	reservoir_t	*rv;
	struct rsqcb *rc, *nxt;

	rv = m->private;
	if (!rv) {
		seq_printf(m, "not available\n");
		return 0;
	}
	seq_printf(m, "Reservoir - %s(%p) size %dK", rv->name, rv, rv->size >> 10);
	if (rv->iid != -1)
		seq_printf(m, " SGI %d", sgi_rsqslot(rv));
	seq_printf(m, "\n\tCurrent available frame NO.\t: %d~%d\n", rv->headno, rv->slotno);
	if (rv->datatype == DATATYPE_VIDEO) {
		seq_printf(m, "\tFrames (I/P)\t: %u/%u, Err(%u)\n", _frmI, _frmP, _frmerr);
	} else {
		seq_printf(m, "\ttime        \t: %u\n", rv->timestamp / 10);
	}
	seq_printf(m, "\tDrop Count/Frms\t: %u/%u\n", rv->dropcnt, rv->dropfrms);
	seq_printf(m, "\tCorrupted frames: %u\n", rv->corrfrms);
	seq_printf(m, "\tSlot Error\t: %u\n",     rv->sloterr);
	seq_printf(m, "\tReservoir Reset\t: %u\n", rv->resetcnt);
	seq_printf(m, "\tHot sync\t: %u\n", rv->synccnt);
	//
	for (rc = (struct rsqcb*)_rsqcblst.next; rc != (struct rsqcb*)&_rsqcblst; rc = nxt) {
		struct rsqrec	*rec = (struct rsqrec*)rc;
		seq_printf(m, "\tfile\t: %s (%u)\n", rec->file? rec->file : "(null)", rec->frms);
		nxt = (struct rsqcb*)rc->list.next;
	}

	return 0;
}

static int c_logshow(struct seq_file *m, void *v)
{
	reservoir_t		*rv;
	unsigned int	rs;
	char			msg[256];
	slot_t			slot;

	rv = m->private;
	if (!rv) {
		seq_printf(m, "not available\n");
		return 0;
	}
/* for debug
	seq_printf(m, "Reservoir - %s(%p) size %dK\n", rv->name, rv, rv->size >> 10);
	seq_printf(m, "\tCurrent available frame NO.\t: %d~%d\n", rv->ns.rdno, rv->slotno);
	seq_printf(m, "\treset id                   \t: %d~%d\n", rv->ss.reset_id, rv->ns.reset_id);
	seq_printf(m, "\tDrop Count/Frms\t: %u/%u\n", rv->dropcnt, rv->dropfrms);
	seq_printf(m, "\tReservoir Reset\t: %u\n", rv->resetcnt);
	seq_printf(m, "\tHot sync\t: %u\n", rv->synccnt);
*/
	do {
		rs = de_rsqslot(rv, msg, sizeof(msg), (slot_t*)&slot);
		if (!rs) break;
		seq_printf(m, msg);
	} while (m->size >= (m->count + sizeof(msg)));

	return 0;
}

static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

static void c_stop(struct seq_file *m, void *v)
{
}

const struct seq_operations rsqinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show
};

const struct seq_operations rsqlog_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_logshow
};

static int rsqinfo_open(struct inode *inode, struct file *file)
{
	int					res = -ENOMEM;
	int					i;
	struct seq_file		*seq;

    res = seq_open(file, NULL);
	if (res)
		return res;
	seq = file->private_data;
	for (i = 0; i < sizeof(_sstr) / sizeof(struct sstream); i++) {
		if (!_sstr[i].rvr) continue;
		if (!strncmp(file->f_path.dentry->d_iname,
					 _sstr[i].rvr->name, sizeof(_sstr[i].rvr->name))) {
			seq->private = _sstr[i].rvr;
			break;
		}
	}
	if (i == (sizeof(_sstr) / sizeof(struct sstream)))
		return -ENOMEM;
	seq->op = _sstr[i].private;
	return res;
}

static ssize_t rsqinfo_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
	char	code[32];
	reservoir_t	*rv;

	rv = (reservoir_t*)((struct seq_file *)file->private_data)->private;
	if (!rv) return len;
	if (copy_from_user(code, buf, 32))
		return -EFAULT;
	if (strncmp(rv->name, "log", 3) == 0) {
		hello_lh(RTK_LOG, 0, 0, 0);
	}
	// rvr simple command parser
	//		file -
	//		stop -
	//		start -
	if (!strncmp(code, "file", 4))
		start_record(rv, code + 5);
	else if (!strncmp(code, "stop", 4))
		_start_ = 0;
	else if (!strncmp(code, "start", 4))
		_start_ = 1;
	return len;
}

#define	TTM(s) (((s) + 3000) / 6000)
static u64	_spent = 0, _lifet = 0;
static int c_show_rtk(struct seq_file *m, void *v)
{
	rtkinfo_t		*rtk;
	int		i;
	int		s;
	u64		cs, cl;

	rtk = m->private;
	if (!rtk) {
		seq_printf(m, "not avaliable\n");
		return 0;
	}
	/* reset cpu usage after present */
	cs = rtk->spent; cl = rtk->lifet;
	s = (int)div64_u64((cs - _spent) * 1000, (cl - _lifet));
	seq_printf(m, "RTOS: %s\n", rtk->version);
	seq_printf(m, "\tcpu usage: %u.%u%% (%llu/%llu)\n", s / 10, s % 10, cs - _spent, cl - _lifet);
	seq_printf(m, "\tttff(isp): %u ms\n", TTM(rtk->ttff_isp));
	seq_printf(m, "\tttff(scl): %u ms\n", TTM(rtk->ttff_scl));
	seq_printf(m, "\tttff(mfe): %u ms\n", TTM(rtk->ttff_mfe));
	seq_printf(m, "\tload ns  : %u ms\n", TTM(rtk->ldns_ts));
	seq_printf(m, "\tfiq count: %u\n", rtk->fiq_cnt);
	for (i = 0; i < 8; i++)
		if (_ipi[i]) seq_printf(m, "\tipi%02d count: %u\n", i + 8, _ipi[i]);
	seq_printf(m, "\trecv pid : %d\n", _recpid);
	_spent = cs; _lifet = cl;
	return 0;
}

static ssize_t rtkinfo_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
	char		code[128];
	rtkinfo_t	*rtk;

	rtk = (rtkinfo_t*)((struct seq_file *)file->private_data)->private;
	if (!rtk) return len;
	if (copy_from_user(code, buf, sizeof(code)))
		return -EFAULT;
	// rtkinfo simple command parser
	//		cli -
	if (!strncmp(code, "cli", 3)) {
		char	*ptr;
		// strip return and space in tail and head.
		ptr = strrchr(code, 0x0a);
		if (ptr) *ptr = '\0';
		for (ptr = code + 4; *ptr == ' ' || *ptr == '\t'; ptr++);
		// copy command to share buffer and send to S
		strncpy(rtk->sbox, ptr, 127);
		hello_lh(RTK_CLI, (u32)rtk, (u32)rtk->diff, 0);
	} else if (!strncmp(code, "reset", 4)) {
	}
	return len;
}

const struct seq_operations rtkinfo_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_show_rtk
};

static int rtkinfo_open(struct inode *inode, struct file *file)
{
	int					res = -ENOMEM;
	struct seq_file		*seq;

    res = seq_open(file, &rtkinfo_op);
	if (res)
		return res;
	seq = file->private_data;
	seq->private = (void*)_rtklin;
    return res;
}

static const struct file_operations proc_rsqinfo_operations = {
    .open       = rsqinfo_open,
    .read       = seq_read,
	.write		= rsqinfo_write,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

static const struct file_operations proc_rtkinfo_operations = {
    .open       = rtkinfo_open,
    .read       = seq_read,
	.write		= rtkinfo_write,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

/* for io */
static ssize_t io_write(struct file *file, const char __user *buf, size_t len, loff_t *pos)
{
	char			code[128];
	char			msg[256];
	char			*ptr;
	unsigned int	ioadd;
	unsigned int	l;
	unsigned int	bank, off;
	int				np;
	int				op;

	if (copy_from_user(code, buf, 128))
		return -EFAULT;

	ptr = strrchr(code, '\n');
	if (ptr) *ptr = '\0';
	for (ptr = code; *ptr == ' ' || *ptr == '\t'; ptr++);
	if (*ptr == 'w') {
		/* write */
		ptr++;
		for (ptr = code; *ptr != '\0' && *ptr != ' ' && *ptr != '\t' ; ptr++);
		op = 0;
		np = sscanf(ptr, "%x %x", &ioadd, &l);
		if (!ptr || np < 2)   return len;
	} else {
		/* read */
		if (!isxdigit(*ptr))
			for (ptr = code; *ptr != '\0' && *ptr != ' ' && *ptr != '\t' ; ptr++);
		op = 1;
		np = sscanf(ptr, "%x %d", &ioadd, &l);
		if (!ptr || np == 0) return len;
		if (np == 1) l = 128;	// not read len, set default read 128 shorts
	}
	bank = ioadd >> 8;
	off  = ioadd & 0xff;
	if (op) {
		#define	COL_NUM		(8)
		unsigned int	col;
		int				t;
		while (l) {
			t = snprintf(msg, sizeof(msg), "0x%06x - ", ioadd);
			for (col = 0; col < COL_NUM && l != 0; col++, l--, off++) {
				t += snprintf(msg + t, sizeof(msg) - t, "%04x ", (unsigned short)*(_io + ((bank << 8) + (off << 1))));
				if (col == 3 && l > 1) t += snprintf(msg + t, sizeof(msg) - t, "- ");
			}
			printk("%s\n", msg);
			ioadd += col;
		}
	} else {
		*(_io + ((bank << 8) + (off << 1))) = l;
		printk("write 0x%06x %04x\n", ioadd, l);
	}
	return len;
}

static int c_ioshow(struct seq_file *m, void *v)
{
	seq_printf(m, "none\n");
	return 0;
}

const struct seq_operations io_op = {
	.start	= c_start,
	.next	= c_next,
	.stop	= c_stop,
	.show	= c_ioshow
};

static int io_open(struct inode *inode, struct file *file)
{
    return seq_open(file, &io_op);
}

static const struct file_operations proc_io_operations = {
    .open       = io_open,
    .read       = seq_read,
	.write		= io_write,
    .llseek     = seq_lseek,
    .release    = seq_release,
};

/* call to RTOS */
static unsigned long hello_lh(u32 type, u32 arg1, u32 arg2, u32 arg3)
{
	struct arm_smccc_res res;

	arm_smccc_smc(type, arg1, arg2, arg3,
		      0, 0, 0, 0, &res);

	return res.a0;
}
static bool alkaid_notify_nop(int no){
    return false;
}
static bool (*alkaid_notify[1])(int no) = {alkaid_notify_nop};
void alkaid_registe_notify(int cpu, void *notify){
    alkaid_notify[cpu] = notify;
}
EXPORT_SYMBOL(alkaid_registe_notify);
void *alkaid_unregiste_notify(int cpu){
    void *notify = alkaid_notify[cpu];
    alkaid_notify[cpu] = alkaid_notify_nop;
    return notify;
}
EXPORT_SYMBOL(alkaid_unregiste_notify);

/* call from gic_handle_irq (gic-irq.c) */
void handle_rsq(unsigned int irqnr)
{
    if (alkaid_notify[0](irqnr))
    {
        return;
    }
	_ipi[irqnr - 8]++;
	if (!_start_) return;
	if (_mst && _mst->iid == (irqnr - 8))
		up(&_frmrdy);
}

static int rsq_receiver(void *unused)
{
	struct task_struct	*tsk = current;
	unsigned int		size = FRAME_SIZE;
	gslot_t				gho;
	unsigned int		rs;
	struct rsqcb		*rc;
	struct rsqcb		*nxt;

	/* Setup a clean context for our children to inherit. */
	set_task_comm(tsk, "rsq_receiver");
	sema_init(&_frmrdy, 0);
	gho.ghost = NULL;	// set start to ghost
	gho.copy.no = 0;
	_start_ = 1;
	while (1) {
		int	empty;
		down(&_frmrdy);
		if (list_empty(&_rsqcblst)) continue;
		empty = 0;
		do {
			unsigned int *h = _frm;

			// rs = de_ghost_rsqslot(_mst, _frm, size, &gho);
			rs = de_rsqslot(_mst, _frm, size, (slot_t*)&gho);
			if (rs == 0)
				break;
			_frmcnt++;
			// frame check!!
			if (*h != 0x01000000) _frmerr++;
			else if ((*(h + 1) & 0x1f) == 7) _frmI++;
			else if ((*(h + 1) & 0x1f) == 1) _frmP++;
			else _frmerr++;
			//
			for (rc = (struct rsqcb*)_rsqcblst.next; rc != (struct rsqcb*)&_rsqcblst; rc = nxt) {
				struct rsqrec	*rec = (struct rsqrec*)rc;
				nxt = (struct rsqcb*)rc->list.next;
				if (rec->cb.rsqproc(rec->cb.private, _frm, rs, &(gho.copy)) == 0) {
					list_del((struct list_head*)rec);
					printk(KERN_ERR "---- end rdno %d ----\n", gho.copy.no);
					vfree(rec);
					if (list_empty(&_rsqcblst)) empty = 1;
				}
			}
		} while (!empty);
	}
	return 0;
}

static int __init epoch_init(void)
{
	char				proc[32];
	long				share;
	struct rlink_head	*rl;
	
	if (disable_rtos)
		return 0;

    proc_mkdir("rtos", NULL);
	/* get RTK info */
	share  = (long)hello_lh(RTK_CALL, (u32)0, (u32)0, (u32)0);
	_rtkres = request_mem_region(share, SHARE_SIZE, "rtos");
	_rtklin = (rtkinfo_t*)ioremap(_rtkres->start, resource_size(_rtkres));
	if (!_rtklin							||
		_rtklin->size  != sizeof(*_rtklin)	||
		_rtklin->verid != RSQ_VERSION_ID) {
		proc_create("rtos/version_not_match", 0, NULL, &proc_rtkinfo_operations);
		printk(KERN_ERR "Error!! RTOS version not match\n");
		_rtklin = NULL;
		return 0;
	}
	snprintf(proc, sizeof(proc), "rtos/%s", _rtklin->name);
    proc_create(proc, 0, NULL, &proc_rtkinfo_operations);
	/* */
	for (rl = &(_rtklin->root); rl->next != (void*)share;) {
		struct sstream		sst;

		sst.res = request_mem_region((long)rl->next, rl->nsize, "rsq");
		sst.rvr = (reservoir_t*)ioremap(sst.res->start, resource_size(sst.res));
		init_nsrsq(sst.rvr);
		snprintf(proc, sizeof(proc), "rtos/%s", sst.rvr->name);
		proc_create(proc, 0, NULL, &proc_rsqinfo_operations);
		sst.id = sst.rvr->iid;
		if (strncmp(sst.rvr->name, "v1",  2) == 0) _mst = sst.rvr;	// set mater stream
		if (strncmp(sst.rvr->name, "log", 3) == 0)
			sst.private = (void*)&rsqlog_op;  // set log read op
		else
			sst.private = (void*)&rsqinfo_op; // set stream info read op
		_sstr[sst.id] = sst;
		rl = &(sst.rvr->link);
	}
	_rtklin->diff = (unsigned int)share - (unsigned int)_rtklin;	// offset for address transfer
	_frm	= vmalloc(FRAME_SIZE);
	memset(_frm, 0, FRAME_SIZE);
	_frmerr	= 0;
	/* for test only */
	/* map 0x1f200000 ~ 0x1f3fffff for io */
	{
		struct resource	*iores;
		char				proc[32];

		#define	IO		(0x1f000000)
		#define	IOSZ	(0x00400000)
		iores = request_mem_region((long)IO, IOSZ, "io");
		_io = (unsigned short*)ioremap(iores->start, resource_size(iores));
		snprintf(proc, sizeof(proc), "rtos/io");
	    proc_create(proc, 0, NULL, &proc_io_operations);
	}
	/* create a thread to get rsq */
    _recpid = kernel_thread(rsq_receiver, NULL, CLONE_FS | CLONE_FILES);
	/* for debug */
	hello_lh(RSQ_INIT, (u32)_rtklin, _rtklin->diff, 0);
	return 0;
}

/*
 * kerenl file I/O
 */
struct file *file_open(const char *path, int flags, int rights)
{
    struct file *filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if (IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void file_close(struct file *file)
{
    filp_close(file, NULL);
}

int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

int file_write(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_write(file, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

int file_sync(struct file *file)
{
    vfs_fsync(file, 0);
    return 0;
}

typedef int	(*rsqproc)(void *param, void *buf, int size, slot_t* slot);
int acquire_rsqproc(char *cmd, rsqproc rsqfunc)
{
	struct rsqrec	*rec;

	rec = kmalloc(sizeof(struct rsqrec), GFP_KERNEL);
	if (!rec) return 0;
	rec->off = 0;
	rec->rvr = _mst;
	rec->cb.rsqproc = rsqfunc;
	rec->cb.private = rec;
	INIT_LIST_HEAD((struct list_head *)rec);
	list_add((struct list_head *)rec, &_rsqcblst);
	return 0;
}

rtkinfo_t* get_rtkinfo(void)
{
    return _rtklin;
}

fs_initcall(epoch_init);
