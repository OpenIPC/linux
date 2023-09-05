/*
* mdrv_cryptioctl.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: edie.chen <edie.chen@sigmastar.com.tw>
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
#include <crypto/hash.h>
#include <linux/crypto.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/ioctl.h>
#include <linux/random.h>
#include <linux/syscalls.h>
#include <linux/pagemap.h>
#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/scatterlist.h>
#include <linux/rtnetlink.h>
#include <crypto/authenc.h>
#include <linux/sysctl.h>
#include "mdrv_cryptodev_int.h"
#include "mdrv_cryptzc.h"
#include "mdrv_cryptversion.h"
#include <mdrv_crypto_io.h>
#include <mdrv_crypto_io_st.h>
MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("CryptoDev driver");
MODULE_LICENSE("GPL");

/* ====== Compile-time config ====== */

/* Default (pre-allocated) and maximum size of the job queue.
 * These are free, pending and done items all together. */
#define DEF_COP_RINGSIZE 2
#define MAX_COP_RINGSIZE 8

/* ====== Module parameters ====== */

int cryptodev_verbosity=2;
module_param(cryptodev_verbosity, int, 0644);

MODULE_PARM_DESC(cryptodev_verbosity, "0: normal, 1: verbose, 2: debug");

/* ====== CryptoAPI ====== */
struct todo_list_item {
	struct list_head __hook;
	struct kernel_crypt_op kcop;
	int result;
};

struct locked_list {
	struct list_head list;
	struct mutex lock;
};

struct crypt_priv {
	struct fcrypt fcrypt;
	struct locked_list free, todo, done;
	int itemcount;
	struct work_struct cryptask;
	wait_queue_head_t user_waiter;
};

#define FILL_SG(sg, ptr, len)					\
	do {							\
		(sg)->page = virt_to_page(ptr);			\
		(sg)->offset = offset_in_page(ptr);		\
		(sg)->length = len;				\
		(sg)->dma_address = 0;				\
	} while (0)

/* cryptodev's own workqueue, keeps crypto tasks from disturbing the force */
static struct workqueue_struct *cryptodev_wq;

/* Prepare session for future use. */
static int
crypto_create_session(struct fcrypt *fcr, struct session_op *sop)
{
	struct csession	*ses_new = NULL, *ses_ptr;
	int ret = 0;
	const char *alg_name = NULL;
	const char *hash_name = NULL;
	int hmac_mode = 1, stream = 0, aead = 0;
	/*
	 * With composite aead ciphers, only ckey is used and it can cover all the
	 * structure space; otherwise both keys may be used simultaneously but they
	 * are confined to their spaces
	 */
	struct {
		uint8_t ckey[CRYPTO_CIPHER_MAX_KEY_LEN];
		uint8_t mkey[CRYPTO_HMAC_MAX_KEY_LEN];
		/* padding space for aead keys */
		uint8_t pad[RTA_SPACE(sizeof(struct crypto_authenc_key_param))];
	} keys;
	/* Does the request make sense? */
	if (unlikely(!sop->cipher && !sop->mac)) {
		return -EINVAL;
	}
	switch (sop->cipher) {
	case 0:
		break;
	case CRYPTO_AES_CBC:
		alg_name = "cbc(aes)";
		break;
	case CRYPTO_AES_ECB:
		alg_name = "ecb(aes)";
		break;
	case CRYPTO_AES_CTR:
		alg_name = "ctr(aes)";
		stream = 1;
		break;
	default:
		return -EINVAL;
	}

	switch (sop->mac) {
	case 0:
		break;
	case CRYPTO_SHA2_256:
		hash_name = "sha256";
		hmac_mode = 0;
		break;

	default:
		return -EINVAL;
	}
	/* Create a session and put it to the list. */
	ses_new = kzalloc(sizeof(*ses_new), GFP_KERNEL);
	if (!ses_new)
       {return -ENOMEM;
       }
	/* Set-up crypto transform. */
	if (alg_name) {
		unsigned int keylen;
		ret = cryptodev_get_cipher_keylen(&keylen, sop, aead);
		if (unlikely(ret < 0)) {
			goto error_cipher;
		}

		ret = cryptodev_get_cipher_key(keys.ckey, sop, aead);
		if (unlikely(ret < 0))
			goto error_cipher;

		ret = cryptodev_cipher_init(&ses_new->cdata, alg_name, keys.ckey,
						keylen, stream, aead);
		if (ret < 0) {
			ret = -EINVAL;
			goto error_cipher;
		}
	}

	if (hash_name && aead == 0) {
		if (unlikely(sop->mackeylen > CRYPTO_HMAC_MAX_KEY_LEN)) {
			ret = -EINVAL;
			goto error_hash;
		}

		if (sop->mackey && unlikely(copy_from_user(keys.mkey, sop->mackey,
					    sop->mackeylen))) {
			ret = -EFAULT;
			goto error_hash;
		}
		ret = cryptodev_hash_init(&ses_new->hdata, hash_name, hmac_mode,
							keys.mkey, sop->mackeylen);
		if (ret != 0) {
			ret = -EINVAL;
			goto error_hash;
		}
	}
	ses_new->alignmask = max(ses_new->cdata.alignmask,
	                                          ses_new->hdata.alignmask);
	ses_new->array_size = DEFAULT_PREALLOC_PAGES;
	ses_new->pages = kzalloc(ses_new->array_size *
			sizeof(struct page *), GFP_KERNEL);
	ses_new->sg = kzalloc(ses_new->array_size *
			sizeof(struct scatterlist), GFP_KERNEL);
	if (ses_new->sg == NULL || ses_new->pages == NULL) {
		ret = -ENOMEM;
		goto error_hash_kfree;
	}

	/* put the new session to the list */
	get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
	mutex_init(&ses_new->sem);

	mutex_lock(&fcr->sem);
restart:
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		/* Check for duplicate SID */
		if (unlikely(ses_new->sid == ses_ptr->sid)) {
			get_random_bytes(&ses_new->sid, sizeof(ses_new->sid));
			/* Unless we have a broken RNG this
			   shouldn't loop forever... ;-) */
			goto restart;
		}
	}

	list_add(&ses_new->entry, &fcr->list);
	mutex_unlock(&fcr->sem);

	/* Fill in some values for the user. */
	sop->ses = ses_new->sid;

	return 0;

error_hash:
	cryptodev_cipher_deinit(&ses_new->cdata);
	kfree(ses_new);
	return ret;
error_hash_kfree:
	kfree(ses_new->sg);
	kfree(ses_new->pages);
error_cipher:
	kfree(ses_new);

	return ret;

}

/* Everything that needs to be done when remowing a session. */
static inline void
crypto_destroy_session(struct csession *ses_ptr)
{
	if (!mutex_trylock(&ses_ptr->sem)) {
		mutex_lock(&ses_ptr->sem);
	}
	cryptodev_cipher_deinit(&ses_ptr->cdata);
	cryptodev_hash_deinit(&ses_ptr->hdata);
	kfree(ses_ptr->pages);
	kfree(ses_ptr->sg);
	mutex_unlock(&ses_ptr->sem);
	mutex_destroy(&ses_ptr->sem);
	kfree(ses_ptr);
}

/* Look up a session by ID and remove. */
static int
crypto_finish_session(struct fcrypt *fcr, uint32_t sid)
{
	struct csession *tmp, *ses_ptr;
	struct list_head *head;
	int ret = 0;

	mutex_lock(&fcr->sem);
	head = &fcr->list;
	list_for_each_entry_safe(ses_ptr, tmp, head, entry) {
		if (ses_ptr->sid == sid) {
			list_del(&ses_ptr->entry);
			crypto_destroy_session(ses_ptr);
			break;
		}
	}

	if (unlikely(!ses_ptr)) {
		ret = -ENOENT;
	}
	mutex_unlock(&fcr->sem);

	return ret;
}

/* Remove all sessions when closing the file */
static int
crypto_finish_all_sessions(struct fcrypt *fcr)
{
	struct csession *tmp, *ses_ptr;
	struct list_head *head;

	mutex_lock(&fcr->sem);

	head = &fcr->list;
	list_for_each_entry_safe(ses_ptr, tmp, head, entry) {
		list_del(&ses_ptr->entry);
		crypto_destroy_session(ses_ptr);
	}
	mutex_unlock(&fcr->sem);

	return 0;
}

/* Look up session by session ID. The returned session is locked. */
struct csession *
crypto_get_session_by_sid(struct fcrypt *fcr, uint32_t sid)
{
	struct csession *ses_ptr, *retval = NULL;

	if (unlikely(fcr == NULL))
		return NULL;

	mutex_lock(&fcr->sem);
	list_for_each_entry(ses_ptr, &fcr->list, entry) {
		if (ses_ptr->sid == sid) {
			mutex_lock(&ses_ptr->sem);
			retval = ses_ptr;
			break;
		}
	}
	mutex_unlock(&fcr->sem);

	return retval;
}

static void cryptask_routine(struct work_struct *work)
{
	struct crypt_priv *pcr = container_of(work, struct crypt_priv, cryptask);
	struct todo_list_item *item;
	LIST_HEAD(tmp);

	/* fetch all pending jobs into the temporary list */
	mutex_lock(&pcr->todo.lock);
	list_cut_position(&tmp, &pcr->todo.list, pcr->todo.list.prev);
	mutex_unlock(&pcr->todo.lock);

	/* handle each job locklessly */
	list_for_each_entry(item, &tmp, __hook) {
		item->result = crypto_run(&pcr->fcrypt, &item->kcop);
		if (unlikely(item->result))
            printk("crypto_run() failed: %d\n", item->result);
//			derr(0, "crypto_run() failed: %d", item->result);
	}

	/* push all handled jobs to the done list at once */
	mutex_lock(&pcr->done.lock);
	list_splice_tail(&tmp, &pcr->done.list);
	mutex_unlock(&pcr->done.lock);

	/* wake for POLLIN */
	wake_up_interruptible(&pcr->user_waiter);
}

/* ====== /dev/crypto ====== */

static int
cryptodev_open(struct inode *inode, struct file *filp)
{
    struct todo_list_item *tmp[DEF_COP_RINGSIZE], *tmp_next;
	struct crypt_priv *pcr;
	int i;
	pcr = kzalloc(sizeof(*pcr), GFP_KERNEL);
	if (!pcr)
		return -ENOMEM;
	filp->private_data = pcr;
	mutex_init(&pcr->fcrypt.sem);
	mutex_init(&pcr->free.lock);
	mutex_init(&pcr->todo.lock);
	mutex_init(&pcr->done.lock);
	INIT_LIST_HEAD(&pcr->fcrypt.list);
	INIT_LIST_HEAD(&pcr->free.list);
	INIT_LIST_HEAD(&pcr->todo.list);
	INIT_LIST_HEAD(&pcr->done.list);
	INIT_WORK(&pcr->cryptask, cryptask_routine);
	init_waitqueue_head(&pcr->user_waiter);
	for (i = 0; i < DEF_COP_RINGSIZE; i++) {
		tmp[i] = kzalloc(sizeof(struct todo_list_item), GFP_KERNEL);
		if (!tmp[i])
            goto err_ringalloc;
		pcr->itemcount++;
		list_add(&tmp[i]->__hook, &pcr->free.list);
        }
	return 0;

/* In case of errors, free any memory allocated so far */
err_ringalloc:
	list_for_each_entry_safe(tmp[i], tmp_next, &pcr->free.list, __hook) {
		list_del(&tmp[i]->__hook);
		kfree(tmp[i]);
	}

    mutex_destroy(&pcr->done.lock);
	mutex_destroy(&pcr->todo.lock);
	mutex_destroy(&pcr->free.lock);
	mutex_destroy(&pcr->fcrypt.sem);
	kfree(pcr);
	filp->private_data = NULL;
	return -ENOMEM;
}

static int
cryptodev_release(struct inode *inode, struct file *filp)
{
    struct crypt_priv *pcr = filp->private_data;
	struct todo_list_item *item, *item_safe;
    int items_freed = 0;
	if (!pcr)
		return 0;

	cancel_work_sync(&pcr->cryptask);

	list_splice_tail(&pcr->todo.list, &pcr->free.list);
	list_splice_tail(&pcr->done.list, &pcr->free.list);

	list_for_each_entry_safe(item, item_safe, &pcr->free.list, __hook) {
		list_del(&item->__hook);
		kfree(item);
		items_freed++;
	}

	if (items_freed != pcr->itemcount) {
	}

	crypto_finish_all_sessions(&pcr->fcrypt);

	mutex_destroy(&pcr->done.lock);
	mutex_destroy(&pcr->todo.lock);
	mutex_destroy(&pcr->free.lock);
	mutex_destroy(&pcr->fcrypt.sem);

	kfree(pcr);
	filp->private_data = NULL;

	return 0;
}

#ifdef ENABLE_ASYNC
/* enqueue a job for asynchronous completion
 *
 * returns:
 * -EBUSY when there are no free queue slots left
 *        (and the number of slots has reached it MAX_COP_RINGSIZE)
 * -EFAULT when there was a memory allocation error
 * 0 on success */
static int crypto_async_run(struct crypt_priv *pcr, struct kernel_crypt_op *kcop)
{
	struct todo_list_item *item = NULL;
	if (unlikely(kcop->cop.flags & COP_FLAG_NO_ZC))
		return -EINVAL;
	mutex_lock(&pcr->free.lock);
	if (likely(!list_empty(&pcr->free.list))) {
		item = list_first_entry(&pcr->free.list,
				struct todo_list_item, __hook);
		list_del(&item->__hook);
	} else if (pcr->itemcount < MAX_COP_RINGSIZE) {
		pcr->itemcount++;
	} else {
		mutex_unlock(&pcr->free.lock);
		return -EBUSY;
	}
	mutex_unlock(&pcr->free.lock);

	if (unlikely(!item)) {
		item = kzalloc(sizeof(struct todo_list_item), GFP_KERNEL);
		if (unlikely(!item))
			return -EFAULT;
	}

	memcpy(&item->kcop, kcop, sizeof(struct kernel_crypt_op));

	mutex_lock(&pcr->todo.lock);
	list_add_tail(&item->__hook, &pcr->todo.list);
	mutex_unlock(&pcr->todo.lock);

	queue_work(cryptodev_wq, &pcr->cryptask);
	return 0;
}

/* get the first completed job from the "done" queue
 *
 * returns:
 * -EBUSY if no completed jobs are ready (yet)
 * the return value of crypto_run() otherwise */
static int crypto_async_fetch(struct crypt_priv *pcr,
		struct kernel_crypt_op *kcop)
{
    struct todo_list_item *item;
	int retval;

	mutex_lock(&pcr->done.lock);
	if (list_empty(&pcr->done.list)) {
		mutex_unlock(&pcr->done.lock);
		return -EBUSY;
	}
	item = list_first_entry(&pcr->done.list, struct todo_list_item, __hook);
	list_del(&item->__hook);
	mutex_unlock(&pcr->done.lock);

	memcpy(kcop, &item->kcop, sizeof(struct kernel_crypt_op));
	retval = item->result;

	mutex_lock(&pcr->free.lock);
	list_add_tail(&item->__hook, &pcr->free.list);
	mutex_unlock(&pcr->free.lock);

	/* wake for POLLOUT */
	wake_up_interruptible(&pcr->user_waiter);

	return retval;
}
#endif

/* this function has to be called from process context */
static int fill_kcop_from_cop(struct kernel_crypt_op *kcop, struct fcrypt *fcr)
{
	struct crypt_op *cop = &kcop->cop;
	struct csession *ses_ptr;
	int rc;
	/* this also enters ses_ptr->sem */
	ses_ptr = crypto_get_session_by_sid(fcr, cop->ses);
	if (unlikely(!ses_ptr)) {
		return -EINVAL;
	}
	kcop->ivlen = cop->iv ? ses_ptr->cdata.ivsize : 0;
	kcop->digestsize = 0; /* will be updated during operation */

	crypto_put_session(ses_ptr);

	kcop->task = current;
	kcop->mm = current->mm;

	if (cop->iv) {
		rc = copy_from_user(kcop->iv, cop->iv, kcop->ivlen);
		if (unlikely(rc)) {
			return -EFAULT;
		}
	}
	return 0;
}

/* this function has to be called from process context */
static int fill_cop_from_kcop(struct kernel_crypt_op *kcop, struct fcrypt *fcr)
{
	int ret;
	if (kcop->digestsize) {
		ret = copy_to_user(kcop->cop.mac,
				kcop->hash_output, kcop->digestsize);
		if (unlikely(ret))
			return -EFAULT;
	}
	if (kcop->ivlen && kcop->cop.flags & COP_FLAG_WRITE_IV) {
		ret = copy_to_user(kcop->cop.iv,
				kcop->iv, kcop->ivlen);
		if (unlikely(ret))
			return -EFAULT;
	}
	return 0;
}

static int kcop_from_user(struct kernel_crypt_op *kcop,
			struct fcrypt *fcr, void __user *arg)
{
	if (unlikely(copy_from_user(&kcop->cop, arg, sizeof(kcop->cop))))
		return -EFAULT;
	return fill_kcop_from_cop(kcop, fcr);
}

static int kcop_to_user(struct kernel_crypt_op *kcop,
			struct fcrypt *fcr, void __user *arg)
{
	int ret;

	ret = fill_cop_from_kcop(kcop, fcr);
	if (unlikely(ret)) {
		return ret;
	}

	if (unlikely(copy_to_user(arg, &kcop->cop, sizeof(kcop->cop)))) {
		return -EFAULT;
	}
	return 0;
}

static inline void tfm_info_to_alg_info(struct alg_info *dst, struct crypto_tfm *tfm)
{
	snprintf(dst->cra_name, CRYPTODEV_MAX_ALG_NAME,
			"%s", crypto_tfm_alg_name(tfm));
	snprintf(dst->cra_driver_name, CRYPTODEV_MAX_ALG_NAME,
			"%s", crypto_tfm_alg_driver_name(tfm));
}

#ifndef CRYPTO_ALG_KERN_DRIVER_ONLY
static unsigned int is_known_accelerated(struct crypto_tfm *tfm)
{
	const char *name = crypto_tfm_alg_driver_name(tfm);

	if (name == NULL)
		return 1; /* assume accelerated */

	/* look for known crypto engine names */
	if (strstr(name, "-talitos")	||
	    !strncmp(name, "mv-", 3)	||
	    !strncmp(name, "atmel-", 6)	||
	    strstr(name, "geode")	||
	    strstr(name, "hifn")	||
	    strstr(name, "-ixp4xx")	||
	    strstr(name, "-omap")	||
	    strstr(name, "-picoxcell")	||
	    strstr(name, "-s5p")	||
	    strstr(name, "-ppc4xx")	||
	    strstr(name, "-caam")	||
	    strstr(name, "-n2"))
		return 1;

	return 0;
}
#endif

static long
cryptodev_ioctl(struct file *filp, unsigned int cmd, unsigned long arg_)
{
	void __user *arg = (void __user *)arg_;
	struct session_op sop;
	struct kernel_crypt_op kcop;
	struct crypt_priv *pcr = filp->private_data;
	struct fcrypt *fcr;
	uint32_t ses;
	int ret;
	if (unlikely(!pcr))
		BUG();

	fcr = &pcr->fcrypt;

	switch (cmd) {
	case CIOCGSESSION:
        if (unlikely(copy_from_user(&sop, arg, sizeof(sop))))
            return -EFAULT;
        ret = crypto_create_session(fcr, &sop);
		if (unlikely(ret))
			return ret;
		ret = copy_to_user(arg, &sop, sizeof(sop));
		if (unlikely(ret)) {
            crypto_finish_session(fcr, sop.ses);
			return -EFAULT;
		}
		return ret;
	case CIOCFSESSION:
		ret = get_user(ses, (uint32_t __user *)arg);
		if (unlikely(ret))
			return ret;
		ret = crypto_finish_session(fcr, ses);
		return ret;
	case CIOCCRYPT:
		if (unlikely(ret = kcop_from_user(&kcop, fcr, arg))) {
//			dwarning(1, "Error copying from user");
			return ret;
		}
		ret = crypto_run(fcr, &kcop);
		if (unlikely(ret)) {
//			dwarning(1, "Error in crypto_run");
			return ret;
		}
		return kcop_to_user(&kcop, fcr, arg);

	default:
		return -ENOTTY;
	}
}


static unsigned int cryptodev_poll(struct file *file, poll_table *wait)
{
	struct crypt_priv *pcr = file->private_data;
	int ret = 0;
	poll_wait(file, &pcr->user_waiter, wait);
	if (!list_empty_careful(&pcr->done.list))
		ret |= POLLIN | POLLRDNORM;
	if (!list_empty_careful(&pcr->free.list) || pcr->itemcount < MAX_COP_RINGSIZE)
		ret |= POLLOUT | POLLWRNORM;
	return ret;
}

static const struct file_operations cryptodev_fops = {
	.owner = THIS_MODULE,
	.open = cryptodev_open,
	.release = cryptodev_release,
	.unlocked_ioctl = cryptodev_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl = cryptodev_compat_ioctl,
#endif /* CONFIG_COMPAT */
	.poll = cryptodev_poll,
};

static struct miscdevice cryptodev = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "crypto",
	.fops = &cryptodev_fops,
	.mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH,
};

static int __init
cryptodev_register(void)
{
	int rc;
	rc = misc_register(&cryptodev);
	if (unlikely(rc)) {
		pr_err(PFX "registration of /dev/crypto failed\n");
		return rc;
	}

	return 0;
}

static void __exit
cryptodev_deregister(void)
{
	misc_deregister(&cryptodev);
}

/* ====== Module init/exit ====== */
static struct ctl_table verbosity_ctl_dir[] = {
	{
		.procname       = "cryptodev_verbosity",
		.data           = &cryptodev_verbosity,
		.maxlen         = sizeof(int),
		.mode           = 0644,
		.proc_handler   = proc_dointvec,
	},
	{0, },
};

static struct ctl_table verbosity_ctl_root[] = {
	{
		.procname       = "ioctl",
		.mode           = 0555,
		.child          = verbosity_ctl_dir,
	},
	{0, },
};
static struct ctl_table_header *verbosity_sysctl_header;
static int __init init_cryptodev(void)
{
	int rc;
	cryptodev_wq = create_workqueue("cryptodev_queue");
	if (unlikely(!cryptodev_wq)) {
		pr_err(PFX "failed to allocate the cryptodev workqueue\n");
		return -EFAULT;
	}
	rc = cryptodev_register();
	if (unlikely(rc)) {
		destroy_workqueue(cryptodev_wq);
		return rc;
	}
	verbosity_sysctl_header = register_sysctl_table(verbosity_ctl_root);
	pr_info(PFX "driver %s loaded.\n", VERSION);
	return 0;
}

static void __exit exit_cryptodev(void)
{
	flush_workqueue(cryptodev_wq);
	destroy_workqueue(cryptodev_wq);
	if (verbosity_sysctl_header)
		unregister_sysctl_table(verbosity_sysctl_header);
	cryptodev_deregister();
	pr_info(PFX "driver unloaded.\n");
}
module_init(init_cryptodev);
module_exit(exit_cryptodev);
