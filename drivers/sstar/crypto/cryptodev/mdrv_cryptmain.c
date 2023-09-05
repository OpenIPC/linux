/*
* mdrv_cryptmain.c- Sigmastar
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
#include <mdrv_crypto_io_st.h>
#include <mdrv_crypto_io.h>
#include <crypto/scatterwalk.h>
#include <linux/scatterlist.h>
#include "mdrv_cryptodev_int.h"
#include "mdrv_cryptzc.h"
#include "mdrv_cryptlib.h"
#include "mdrv_cryptversion.h"

/* This file contains the traditional operations of encryption
 * and hashing of /dev/crypto.
 */

static int hash_n_crypt(struct csession *ses_ptr, struct crypt_op *cop,
		struct scatterlist *src_sg, struct scatterlist *dst_sg,
		uint32_t len)
{
	int ret;
	/* Always hash before encryption and after decryption. Maybe
	 * we should introduce a flag to switch... TBD later on.
	 */
	if (cop->op == COP_ENCRYPT) {
		if (ses_ptr->hdata.init != 0) {
			ret = cryptodev_hash_update(&ses_ptr->hdata,
							src_sg, len);
			if (unlikely(ret))
				goto out_err;
		}
		if (ses_ptr->cdata.init != 0) {
			ret = cryptodev_cipher_encrypt(&ses_ptr->cdata,
							src_sg, dst_sg, len);

			if (unlikely(ret))
				goto out_err;
		}
	} else {
		if (ses_ptr->cdata.init != 0) {
			ret = cryptodev_cipher_decrypt(&ses_ptr->cdata,
							src_sg, dst_sg, len);

			if (unlikely(ret))
				goto out_err;
		}

		if (ses_ptr->hdata.init != 0) {
			ret = cryptodev_hash_update(&ses_ptr->hdata,
								dst_sg, len);
			if (unlikely(ret))
				goto out_err;
		}
	}
	return 0;
out_err:
//	derr(0, "CryptoAPI failure: %d", ret);
	return ret;
}

/* This is the main crypto function - feed it with plaintext
   and get a ciphertext (or vice versa ) */
static int
__crypto_run_std(struct csession *ses_ptr, struct crypt_op *cop)
{   char *data;
    char __user *src, *dst;
	struct scatterlist sg;
	size_t nbytes, bufsize;
	int ret = 0;

	nbytes = cop->len;
	data = (char *)__get_free_page(GFP_KERNEL);

	if (unlikely(!data)) {
		return -ENOMEM;
	}

	bufsize = PAGE_SIZE < nbytes ? PAGE_SIZE : nbytes;

	src = cop->src;
	dst = cop->dst;

	while (nbytes > 0) {
		size_t current_len = nbytes > bufsize ? bufsize : nbytes;

		if (unlikely(copy_from_user(data, src, current_len))) {
            ret = -EFAULT;
			break;
		}

		sg_init_one(&sg, data, current_len);
		ret = hash_n_crypt(ses_ptr, cop, &sg, &sg, current_len);
		if (unlikely(ret)) {
			break;
		}

		if (ses_ptr->cdata.init != 0) {
			if (unlikely(copy_to_user(dst, data, current_len))) {
//			        derr(1, "could not copy to user.");
				ret = -EFAULT;
				break;
			}
		}

		dst += current_len;
		nbytes -= current_len;
		src += current_len;
	}

	free_page((unsigned long)data);
	return ret;
}



/* This is the main crypto function - zero-copy edition */
static int
__crypto_run_zc(struct csession *ses_ptr, struct kernel_crypt_op *kcop)
{
	struct scatterlist *src_sg, *dst_sg;
	struct crypt_op *cop = &kcop->cop;
	int ret = 0;
    AESDMA_DBG("%s %d\n",__FUNCTION__,__LINE__);
    AESDMA_DBG("%s %d\n",__FUNCTION__,cop->len);
	ret = get_userbuf(ses_ptr, cop->src, cop->len, cop->dst, cop->len,
	                  kcop->task, kcop->mm, &src_sg, &dst_sg);
	if (unlikely(ret)) {
//		derr(1, "Error getting user pages. Falling back to non zero copy.");
		return __crypto_run_std(ses_ptr, cop);
	}

	ret = hash_n_crypt(ses_ptr, cop, src_sg, dst_sg, cop->len);

	release_user_pages(ses_ptr);
	return ret;
}

int crypto_run(struct fcrypt *fcr, struct kernel_crypt_op *kcop)
{
    struct csession *ses_ptr;
	struct crypt_op *cop = &kcop->cop;
	int ret = 0;
    AESDMA_DBG("%s %d\n",__FUNCTION__,__LINE__);
	if (unlikely(cop->op != COP_ENCRYPT && cop->op != COP_DECRYPT)) {
		return -EINVAL;
	}

	/* this also enters ses_ptr->sem */
	ses_ptr = crypto_get_session_by_sid(fcr, cop->ses);
	if (unlikely(!ses_ptr)) {
//		derr(1, "invalid session ID=0x%08X", cop->ses);
		return -EINVAL;
	}
	if (ses_ptr->hdata.init != 0 && (cop->flags == 0 || cop->flags & COP_FLAG_RESET)) {
        ret = cryptodev_hash_reset(&ses_ptr->hdata);
        if (unlikely(ret)) {
//			derr(1, "error in cryptodev_hash_reset()");
			goto out_unlock;
		}
	}
	if (ses_ptr->cdata.init != 0) {
		int blocksize = ses_ptr->cdata.blocksize;

		if (unlikely(cop->len % blocksize)) {
//			derr(1, "data size (%u) isn't a multiple of block size (%u)",
//				cop->len, blocksize);
			ret = -EINVAL;
			goto out_unlock;
		}

		cryptodev_cipher_set_iv(&ses_ptr->cdata, kcop->iv,
				min(ses_ptr->cdata.ivsize, kcop->ivlen));
	}

	if (likely(cop->len)) {
        if (cop->flags & COP_FLAG_NO_ZC) {
            if (unlikely(ses_ptr->alignmask && !IS_ALIGNED((unsigned long)cop->src, ses_ptr->alignmask))) {

				cop->flags &= ~COP_FLAG_NO_ZC;
			}

			if (unlikely(ses_ptr->alignmask && !IS_ALIGNED((unsigned long)cop->dst, ses_ptr->alignmask))) {

				cop->flags &= ~COP_FLAG_NO_ZC;
			}
		}

		if (cop->flags & COP_FLAG_NO_ZC)
			ret = __crypto_run_std(ses_ptr, &kcop->cop);
		else
			ret = __crypto_run_zc(ses_ptr, kcop);
		if (unlikely(ret))
			goto out_unlock;
	}

	if (ses_ptr->cdata.init != 0) {
		cryptodev_cipher_get_iv(&ses_ptr->cdata, kcop->iv,
				min(ses_ptr->cdata.ivsize, kcop->ivlen));
	}

	if (ses_ptr->hdata.init != 0 &&
		((cop->flags & COP_FLAG_FINAL) ||
		   (!(cop->flags & COP_FLAG_UPDATE) || cop->len == 0))) {
		ret = cryptodev_hash_final(&ses_ptr->hdata, kcop->hash_output);
		if (unlikely(ret)) {
//			derr(0, "CryptoAPI failure: %d", ret);
			goto out_unlock;
		}
		kcop->digestsize = ses_ptr->hdata.digestsize;
	}

out_unlock:
	crypto_put_session(ses_ptr);
	return ret;
}
