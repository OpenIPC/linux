/*
 * fh_aes.h
 *
 *  Created on: 3.12.2015
 *      Author: duobao
 */

#ifndef FH_AES_H_
#define FH_AES_H_

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/clk.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>
#include <linux/crypto.h>
#include <linux/interrupt.h>
#include <crypto/algapi.h>
#include <crypto/aes.h>
#include <crypto/des.h>
#include <crypto/ctr.h>

struct fh_aes_reg {
	u32 encrypt_control;		/*0*/
	u32 reserved_4_8;			/*4*/
	u32 fifo_status;			/*8*/
	u32 parity_error;			/*c*/
	u32 security_key0;			/*10*/
	u32 security_key1;			/*14*/
	u32 security_key2;			/*18*/
	u32 security_key3;			/*1c*/
	u32 security_key4;			/*20*/
	u32 security_key5;			/*24*/
	u32 security_key6;			/*28*/
	u32 security_key7;			/*2c*/
	u32 initial_vector0;		/*30*/
	u32 initial_vector1;		/*34*/
	u32 initial_vector2;		/*38*/
	u32 initial_vector3;		/*3c*/
	u32 reserved_40_44;			/*40*/
	u32 reserved_44_48;			/*44*/
	u32 dma_src_add;			/*48*/
	u32 dma_dst_add;			/*4c*/
	u32 dma_trans_size;			/*50*/
	u32 dma_control;			/*54*/
	u32 fifo_threshold;			/*58*/
	u32 intr_enable;			/*5c*/
	u32 intr_src;				/*60*/
	u32 mask_intr_status;		/*64*/
	u32 intr_clear_status;		/*68*/
	u32 reserved_6c_70;			/*6c*/
	u32 revision;				/*70*/
	u32 feature;				/*74*/
	u32 reserved_78_7c;			/*78*/
	u32 reserved_7c_80;			/*7c*/
	u32 last_initial_vector0;	/*80*/
	u32 last_initial_vector1;	/*84*/
	u32 last_initial_vector2;	/*88*/
	u32 last_initial_vector3;	/*8c*/
};

/*requst ctx.....*/
struct fh_aes_reqctx {
	unsigned long mode;
};
/*aes ctx....*/
struct fh_aes_ctx {
	struct fh_aes_dev *dev;              /*bind to aes dev..*/
	uint8_t aes_key[AES_MAX_KEY_SIZE];		/*rec key value..*/
	int keylen;		/*rec key len.*/
};

struct fh_aes_dev {
	/*common driver paras..*/
	void *regs;
	struct device *dev;	/*bind to the platform dev...*/
	struct clk *clk;
	bool busy;		/*software sync the hardware....*/
	spinlock_t lock;	/*just lock...*/
	u32 irq_no;		/*board info...*/
	u32 en_isr;		/*software rec the isr src*/
	bool iv_flag;
	u32 control_reg;
	/*crypto need below...*/
	struct fh_aes_ctx *ctx;		/*bind to the aes ctx...*/
	struct fh_aes_reqctx *reqctx;	/*bind to the req ctx..*/
	struct scatterlist *sg_src;	/*rec the src data need to be handled*/
	struct scatterlist *sg_dst;	/*rec the dst data need to be handled*/
	struct tasklet_struct tasklet;	/*async process the crypto*/
	struct ablkcipher_request *req;	/*active req...*/
	struct crypto_queue queue;
	unsigned char *ctl_src_xbuf;
	unsigned char *ctl_dst_xbuf;
	unsigned char *ctl_raw_src_xbuf;
	unsigned char *ctl_raw_dst_xbuf;
	struct scatterlist src_sg[1];
	struct scatterlist dst_sg[1];
};


/*#define FH_AES_SELF_TEST*/
/*#define FH_AES_DEBUG*/
#ifdef FH_AES_DEBUG
#define AES_DBG(fmt, args...)  printk(fmt, ## args)
#else
#define AES_DBG(fmt, args...)  do { } while (0)
#endif

#define AES_PRINT_RESULT(fmt, args...)  printk(fmt, ## args)

#endif /* fh_AES_H_ */


