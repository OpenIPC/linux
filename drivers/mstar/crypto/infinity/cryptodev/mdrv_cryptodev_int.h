/* cipher stuff */
#ifndef CRYPTODEV_INT_H
# define CRYPTODEV_INT_H

#include <linux/version.h>
#include <linux/init.h>
#include <linux/sched.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/fdtable.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/scatterlist.h>
#include <mdrv_crypto_io_st.h>
#include <mdrv_crypto_io.h>
#include <crypto/aead.h>


#define PFX "cryptodev: "

extern int cryptodev_verbosity;

#define AESDMA_DEBUG (1)
#if (AESDMA_DEBUG == 1)
#define AESDMA_DBG(fmt, arg...) printk(KERN_DEBUG fmt, ##arg)//KERN_DEBUG KERN_ALERT KERN_WARNING
#else
#define AESDMA_DBG(fmt, arg...)
#endif

struct fcrypt {
	struct list_head list;
	struct mutex sem;
};

/* kernel-internal extension to struct crypt_op */
struct kernel_crypt_op {
	struct crypt_op cop;

	int ivlen;
	__u8 iv[EALG_MAX_BLOCK_LEN];

	int digestsize;
	uint8_t hash_output[AALG_MAX_RESULT_LEN];

	struct task_struct *task;
	struct mm_struct *mm;
};

int crypto_run(struct fcrypt *fcr, struct kernel_crypt_op *kcop);

#include <mdrv_cryptlib.h>

/* other internal structs */
struct csession {
	struct list_head entry;
	struct mutex sem;
	struct cipher_data cdata;
	struct hash_data hdata;
	uint32_t sid;
	uint32_t alignmask;

	unsigned int array_size;
	unsigned int used_pages; /* the number of pages that are used */
	/* the number of pages marked as NOT-writable; they preceed writeables */
	unsigned int readonly_pages;
	struct page **pages;
	struct scatterlist *sg;
};

struct csession *crypto_get_session_by_sid(struct fcrypt *fcr, uint32_t sid);

static inline void crypto_put_session(struct csession *ses_ptr)
{
	mutex_unlock(&ses_ptr->sem);
}
int adjust_sg_array(struct csession *ses, int pagecount);

#endif /* CRYPTODEV_INT_H */
