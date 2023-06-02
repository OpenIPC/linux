#include <linux/random.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include "otp_proc.h"
#include "otp_dbg.h"
#include "otp_main.h"
//#include "comm/timer.h"
//#include "kwrap/flag.h"
//#include "kwrap/task.h"
#include <plat/efuse_protected.h>
#include <mach/fmem.h>
#include <linux/scatterlist.h>
#include <crypto/skcipher.h>

//============================================================================
// Define
//============================================================================
#define MAX_CMD_LENGTH 30
#define MAX_ARG_NUM     6

#define DBG_TEST_EN	(0)

//============================================================================
// Declaration
//============================================================================
typedef struct proc_cmd {
	char cmd[MAX_CMD_LENGTH];
	int (*execute)(PMODULE_INFO pdrv, unsigned char argc, char **argv);
} PROC_CMD, *PPROC_CMD;

//============================================================================
// Global variable
//============================================================================
POTP_DRV_INFO pdrv_otp_info_data;

//============================================================================
// Function define
//============================================================================


//=============================================================================
// proc "Custom Command" file operation functions
//=============================================================================
static int nvt_otp_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "\nUsage\n");
	seq_printf(sfile, "\necho [command] > efuse_op\n\n");
	seq_printf(sfile, "[command]    =>\n");
	seq_printf(sfile, "             => trim (driver's trim data)\n");
	seq_printf(sfile, "             => uniqueid (chip's unique ID)\n");
	seq_printf(sfile, "             => keyset No (key transfer to secure engine)\n");
	seq_printf(sfile, "             => nvt_write_key No (write specific key into specific key set field)\n");
	seq_printf(sfile, "             => nvt_read_key No ( read specific key from specific key set field)\n");
	seq_printf(sfile, "             => nvt_read_key_lock No (Read lock specific key set)\n");
	seq_printf(sfile, "             => encrypt_key_set No (Encrypt/decrypt via specific key set)\n");
	seq_printf(sfile, "             => version (knlPkg.a version)\n");
	return 0;
}

static int nvt_otp_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_otp_proc_show, NULL);
}

static ssize_t nvt_otp_proc_cmd_read(struct file *fp, char __user *ubuf, size_t cnt, loff_t *ppos)
{
	int ret = 0;
	int len;
	char kbuf[32];
	UINT32 h, l;
//	BOOL    avl = FALSE;

	//pr_info("nvt_check_available_proc_cmd_read cnt = [%d]\r\n", cnt);

	if (!*ppos) {
//		pr_info("Check linrary nane [%s]\r\n", library_name);
		//avl = efuse_check_available(library_name);
		h=0;
		l=0;
		if(efuse_get_unique_id(&l, &h) < 0) {
			pr_err("unique ID[0x%08x][0x%08x] error\r\n", (int)h, (int)l);
			h = 0;
			l = 0;
			sprintf(kbuf, "%08x%08x\n", h, l);
		} else {
			sprintf(kbuf, "%08x%08x\n", h, l);
		}

		len = strlen(kbuf);
		if (clear_user((void *)ubuf, cnt)) {
			printk(KERN_ERR "clear error\n");
			return -EIO;
		}
		//ret = simple_read_from_buffer((char __user *)ubuf,cnt,ppos,kbuf,strlen(kbuf));
		ret = copy_to_user(ubuf, kbuf, len);
//		pr_info("efuse_unid %s ubuf %s len = %d\r\n", kbuf, ubuf, len);
		*ppos += len;
        return len;

	}

	return ret;
}

#if 0
//Crypto framework encrypt sample code start
struct tcrypt_result {
    struct completion completion;
    int err;
};

/* tie all data structures together */
struct skcipher_def {
    struct scatterlist sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct tcrypt_result result;
};

/* Callback function */
static void test_skcipher_cb(struct crypto_async_request *req, int error)
{
    struct tcrypt_result *result = req->data;

    if (error == -EINPROGRESS)
        return;
    result->err = error;
    complete(&result->completion);
    pr_info("Encryption finished successfully\n");
}

/* Perform cipher operation */
static unsigned int test_skcipher_encdec(struct skcipher_def *sk, int enc)
{
    int rc = 0;

    if (enc)
        rc = crypto_skcipher_encrypt(sk->req);
    else
        rc = crypto_skcipher_decrypt(sk->req);

    switch (rc) {
    case 0:
        break;
    case -EINPROGRESS:
    case -EBUSY:
        rc = wait_for_completion_interruptible(
            &sk->result.completion);
        if (!rc && !sk->result.err) {
            reinit_completion(&sk->result.completion);
            break;
        }
    default:
        pr_info("skcipher encrypt returned with %d result %d\n",
            rc, sk->result.err);
        break;
    }
    init_completion(&sk->result.completion);

    return rc;
}
#endif

/******************************************************************
 NA51068 efuse key section 640bit => 20 word
 =============================================================
 key#   offset  word[0]  word[1]  word[2]  word[3]
 =============================================================
 key#0  [ 0]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#1  [ 4]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#2  [ 8]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#3  [12]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
 key#4  [16]    xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx  => 128bit
*******************************************************************/
#define AES_ECB_KEY_SIZE    16
static UINT8 key_sample1[16] = {0x24, 0xaa, 0x0d, 0x9b, 0xf1, 0xae, 0x31, 0xb4, 0x28, 0x51, 0xe4, 0xc4, 0xd1, 0x71, 0x1d, 0x1e};
static UINT8 key_sample2[16] = {0x95, 0x4d, 0x81, 0xc5, 0x5a, 0xcc, 0x2b, 0xe5, 0xdd, 0xc8, 0x74, 0xc3, 0x9f, 0xaf, 0xcf, 0x5c};
static UINT8 key_sample3[16] = {0x04, 0x03, 0x02, 0x01, 0x08, 0x07, 0x06, 0x05, 0x12, 0x11, 0x10, 0x09, 0x16, 0x15, 0x14, 0x13};
//static UINT8 key_sample_crypto[16] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x9, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16};

//Here is specific key magic number to let linux crypto framework encrypt / decrypt via key manager
#define OTP_KEY_MANAGER_IDENTIFY_OFS			4
#define OTP_KEY_MANAGER_HDR_SIZE				8
const unsigned char OTP_KEY_MANAGER_HDR[OTP_KEY_MANAGER_HDR_SIZE]= {0x65, 0x6B, 0x65, 0x79, 0x00, 0x00, 0x00, 0x00};  ///< byte[0...3] as magic tag, byte[4] as key offset (word unit)


static ssize_t nvt_otp_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
    int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

//	struct skcipher_def sk;
//    struct crypto_skcipher *skcipher = NULL;
//    struct skcipher_request *req = NULL;
//    char *scratchpad = NULL;
//    char *ivdata = NULL;
//  unsigned char key[16];
//  int ret = -EFAULT;



	/*check command length*/
	if ((!len) || (len > (MAX_CMD_LENGTH - 1))) {
		pr_err("Command length is too long or 0!\n");
		goto ERR_OUT;
	}

	/*copy command string from user space*/
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	printk("CMD:%s\n", cmd_line);

	/*parse command string*/
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	if (ucargc < 1) {
		pr_err("NULL command error\n");
		goto ERR_OUT;
	}

	if (!strcmp(argv[0], "trim")) {
//#if defined(CONFIG_NVT_IVOT_PLAT_NA51055)
		UINT16  data;
		if (efuse_readParamOps(EFUSE_ETHERNET_TRIM_DATA, &data) != E_OK) {
			pr_err("[0] data = NULL\n");
		} else {
			pr_info("[0] data = 0x%08x\r\n", data);
		}
		if (efuse_readParamOps(EFUSE_USBC_TRIM_DATA, &data) != E_OK) {
			pr_err("[1] data = NULL\n");
		} else {
			pr_info("[1] data = 0x%08x\r\n", data);
		}
		if (efuse_readParamOps(EFUSE_DDRP_H_TRIM_DATA, &data) != E_OK) {
			pr_err("[2] data = NULL\n");
		} else {
			pr_info("[2] data = 0x%08x\r\n", data);
		}

		if (efuse_readParamOps(EFUSE_VER_PKG_UID, &data) != E_OK) {
			pr_err("[4] data = NULL\n");
		} else {
			pr_info("[4] data = 0x%08x\r\n", data);
		}

		if (efuse_readParamOps(EFUSE_DDRP_V_TRIM_DATA, &data) != E_OK) {
			pr_err("[3] data = NULL\n");
		} else {
			pr_info("[3] data = 0x%08x\r\n", data);
		}

		if (efuse_readParamOps(EFUSE_DDRP_ZQ_TRIM_DATA, &data) != E_OK) {
			pr_err("[5] data = NULL\n");
		} else {
			pr_info("[5] data = 0x%08x\r\n", data);
		}

		if (efuse_readParamOps(EFUSE_THERMAL_TRIM_DATA, &data) != E_OK) {
			pr_err("[6] data = NULL\n");
		} else {
			pr_info("[6] data = 0x%08x\r\n", data);
		}

		if (efuse_readParamOps(EFUSE_IDDQ_TRIM_DATA, &data) != E_OK) {
			pr_err("[7] data = NULL\n");
		} else {
			pr_info("[7] data = 0x%08x\r\n", data);
		}
//#endif
		return size;
	} else if(!strcmp(argv[0], "uniqueid")) {
		UINT32 h, l;
		h=0;
		l=0;
		if(efuse_get_unique_id(&l, &h) >= 0)
			pr_info("unique ID[0x%08x][0x%08x] success\r\n", (int)h, (int)l);
		else
			pr_err("unique ID[0x%08x][0x%08x] error\r\n", (int)h, (int)l);
	} else if(!strcmp(argv[0], "nvt_write_key")) {
			UINT8  	pKey[16];
			INT32	result;
			UINT32	key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
			if(!strcmp(argv[1], "0")) {
				key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
				memcpy((void *)pKey, (void *)key_sample3, 16);
			} else if(!strcmp(argv[1], "1")) {
				key_set = EFUSE_OTP_2ND_KEY_SET_FIELD;
				memcpy((void *)pKey, (void *)key_sample1, 16);
			} else if(!strcmp(argv[1], "2")) {
				key_set = EFUSE_OTP_3RD_KEY_SET_FIELD;
				memcpy((void *)pKey, (void *)key_sample2, 16);
			} else if(!strcmp(argv[1], "3")) {
				key_set = EFUSE_OTP_4TH_KEY_SET_FIELD;
				memcpy((void *)pKey, (void *)key_sample3, 16);
			} else if(!strcmp(argv[1], "4")) {
				key_set = EFUSE_OTP_5TH_KEY_SET_FIELD;
				memcpy((void *)pKey, (void *)key_sample3, 16);
			}
			pr_info("nvt_write_key => [%s]", argv[1]);
			result = otp_write_key(key_set, pKey);
			if (result < 0) {
				pr_info(" => fail [%d] \r\n", result);
				return size;
			} else {
				pr_info(" => success\r\n");
			}

	} else if(!strcmp(argv[0], "nvt_read_key")) {
			UINT8  		pKey[16];
			INT32		result;
			UINT32	key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
			if(!strcmp(argv[1], "0")) {
				key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "1")) {
				key_set = EFUSE_OTP_2ND_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "2")) {
				key_set = EFUSE_OTP_3RD_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "3")) {
				key_set = EFUSE_OTP_4TH_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "4")) {
				key_set = EFUSE_OTP_5TH_KEY_SET_FIELD;
			}
			pr_info("nvt_read_key => [%s]", argv[1]);

			result = otp_read_key(key_set, pKey);
			if (result < 0) {
				pr_info(" => fail [%d] \r\n", result);
				return size;
			} else {
				pr_info(" => success\r\n");
				pr_info(" => value:%x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x\r\n",
				pKey[0],pKey[1],pKey[2],pKey[3],
				pKey[4],pKey[5],pKey[6],pKey[7],
				pKey[8],pKey[9],pKey[10],pKey[11],
				pKey[12],pKey[13],pKey[14],pKey[15]);
			}
	} else if(!strcmp(argv[0], "nvt_read_key_lock")) {
			INT32	result;
			UINT32	key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
			if(!strcmp(argv[1], "0")) {
				key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "1")) {
				key_set = EFUSE_OTP_2ND_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "2")) {
				key_set = EFUSE_OTP_3RD_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "3")) {
				key_set = EFUSE_OTP_4TH_KEY_SET_FIELD;
			} else if(!strcmp(argv[1], "4")) {
				key_set = EFUSE_OTP_5TH_KEY_SET_FIELD;
			}
			pr_info("nvt_read_key_lock => [%s]", argv[1]);

			result = otp_set_key_read_lock(key_set);
			if (result < 0) {
				pr_info(" => fail [%d] \r\n", result);
				return size;
			} else {
				pr_info(" => success\r\n");
			}
	} else if(!strcmp(argv[0], "version")) {
#if defined(CONFIG_NVT_IVOT_PLAT_NA51089)
		otp_version();
#else
		pr_info("knlpkg.a 1.00.004\r\n");
#endif
	} else if(!strcmp(argv[0], "jtag_disable")) {
		if(is_JTAG_DISABLE_en() == TRUE)
			pr_info("Disable JTAG success\r\n");
		else
			pr_info("Disable JTAG fail\r\n");
	} else if(!strcmp(argv[0], "secure_enable")) {
		if(otp_secure_en() == TRUE)
			pr_info("Secure enable success\r\n");
		else
			pr_info("Secure enable fail\r\n");
	} else if(!strcmp(argv[0], "data_area_encrypt")) {
		if(otp_data_area_encrypt_en() == TRUE)
			pr_info("Data area encrypt enable success\r\n");
		else
			pr_info("Data area encrypt enable fail\r\n");
	} else if(!strcmp(argv[0], "signature_rsa_enable")) {
		if(otp_signature_rsa_en() == TRUE)
			pr_info("RSA signature enable success\r\n");
		else
			pr_info("RSA signature enable fail\r\n");
	} else if(!strcmp(argv[0], "signature_rsa_chksum_enable")) {
		if(otp_signature_rsa_chksum_en() == TRUE)
			pr_info("RSA signature checksum enable success\r\n");
		else
			pr_info("RSA signature checksum enable fail\r\n");
	} else if(!strcmp(argv[0], "trigger_key_set")) {
		UINT32	key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
		void __iomem *IOADDR_CRYPTO_REG_BASE;
		if(!strcmp(argv[1], "0")) {
			key_set = EFUSE_OTP_1ST_KEY_SET_FIELD;
		} else if(!strcmp(argv[1], "1")) {
			key_set = EFUSE_OTP_2ND_KEY_SET_FIELD;
		} else if(!strcmp(argv[1], "2")) {
			key_set = EFUSE_OTP_3RD_KEY_SET_FIELD;
		} else if(!strcmp(argv[1], "3")) {
			key_set = EFUSE_OTP_4TH_KEY_SET_FIELD;
		} else if(!strcmp(argv[1], "4")) {
			key_set = EFUSE_OTP_5TH_KEY_SET_FIELD;
		}
		pr_info("encrypt_key_set => @key [%s]", argv[1]);

		trigger_efuse_key(EFUSE_KEY_MANAGER_CRYPTO, key_set * 4, 4);
		if(!IOADDR_CRYPTO_REG_BASE) {
			IOADDR_CRYPTO_REG_BASE = ioremap_nocache(0xF0620000, 0x100);
		}

		pr_info("0x10 0x%08x 0x%08x 0x%08x 0x%08x\r\n", nvt_readl(IOADDR_CRYPTO_REG_BASE+0x10), nvt_readl(IOADDR_CRYPTO_REG_BASE+0x14), nvt_readl(IOADDR_CRYPTO_REG_BASE+0x18), nvt_readl(IOADDR_CRYPTO_REG_BASE+0x1C));
		pr_info("0x20 0x%08x 0x%08x 0x%08x 0x%08x\r\n", nvt_readl(IOADDR_CRYPTO_REG_BASE+0x20), nvt_readl(IOADDR_CRYPTO_REG_BASE+0x24), nvt_readl(IOADDR_CRYPTO_REG_BASE+0x28), nvt_readl(IOADDR_CRYPTO_REG_BASE+0x2C));
	} else if(!strcmp(argv[0], "quary")) {
		pr_info("=>[quary]\r\n");
		pr_info("             is_secure_enable()=%d\r\n", is_secure_enable());
		pr_info("       is_data_area_encrypted()=%d\r\n", is_data_area_encrypted());
		pr_info("             is_signature_rsa()=%d\r\n", is_signature_rsa());
		pr_info("is_signature_rsa_chsum_enable()=%d\r\n", is_signature_rsa_chsum_enable());
		pr_info("           is_JTAG_DISABLE_en()=%d\r\n", is_JTAG_DISABLE_en());
		pr_info("        is_1st_key_programmed()=%d\r\n", is_1st_key_programmed());
		pr_info("        is_2nd_key_programmed()=%d\r\n", is_2nd_key_programmed());
		pr_info("        is_3rd_key_programmed()=%d\r\n", is_3rd_key_programmed());
		pr_info("        is_4th_key_programmed()=%d\r\n", is_4th_key_programmed());
		pr_info("        is_5th_key_programmed()=%d\r\n", is_5th_key_programmed());
		pr_info("         is_1st_key_read_lock()=%d\r\n", is_1st_key_read_lock());
		pr_info("         is_2nd_key_read_lock()=%d\r\n", is_2nd_key_read_lock());
		pr_info("         is_3rd_key_read_lock()=%d\r\n", is_3rd_key_read_lock());
		pr_info("         is_4th_key_read_lock()=%d\r\n", is_4th_key_read_lock());
		pr_info("         is_5th_key_read_lock()=%d\r\n", is_5th_key_read_lock());
#if defined(CONFIG_NVT_IVOT_PLAT_NA51089)
		pr_info("              is_new_key_rule()=%d\r\n", is_new_key_rule());
#endif
	}else {
		pr_info("\nUsage\n");
		pr_info("\necho [command] > otp_trim\n\n");
		pr_info("[command]    =>\n");
		pr_info("             => trim\n");
		pr_info("             => uniqueid (chip's unique ID)\n");
		pr_info("             =>     nvt_write_key No (write specific key into specific key set field)\n");
		pr_info("             =>      nvt_read_key No ( read specific key from specific key set field)\n");
		pr_info("             =>   trigger_key_set No (                         trigger key set field)\n");
		pr_info("             => nvt_read_key_lock No (Read lock specific key set field => can not readable)\n");
		pr_info("             =>               jtag_disable (Disable JTAG interface persistent)\n");
		pr_info("             =>              secure_enable (ROM treat loader as secure enable)\n");
		pr_info("             =>           data_area_encrypt(ROM treat loader data area as cypher text)\n");
		pr_info("             =>        signature_rsa_enable(ROM treat loader by use RSA as loader's signature)o	\n");
		pr_info("             => signature_rsa_chksum_enable(Once use RSA as signature, enable RSA public checksum(use SHA256) enable)\n");
		pr_info("             => quary(Query all option)\n");
		pr_info("             => version (knlPkg.a version)\n");
	}
ERR_OUT:

	return size;
}

static struct file_operations proc_otp_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_otp_proc_help_open,
	.release = single_release,
	.read   = nvt_otp_proc_cmd_read,
	.llseek = seq_lseek,
    .write   = nvt_otp_proc_cmd_write
};


static int nvt_avl_proc_show(struct seq_file *sfile, void *v)
{
	seq_printf(sfile, "\nUsage\n");
	seq_printf(sfile, "\necho [command] > avl\n\n");
	seq_printf(sfile, "[command]    =>\n");
	seq_printf(sfile, "             => showavl (show available list)\n");
	seq_printf(sfile, "             => version (knlPkg.a version)\n");
	return 0;
}

static int nvt_avl_proc_help_open(struct inode *inode, struct file *file)
{
	return single_open(file, nvt_avl_proc_show, NULL);
}

static ssize_t nvt_avl_proc_cmd_write(struct file *file, const char __user *buf, size_t size, loff_t *off)
{
    int len = size;
	char cmd_line[MAX_CMD_LENGTH];
	char *cmdstr = cmd_line;
	const char delimiters[] = {' ', 0x0A, 0x0D, '\0'};
	char *argv[MAX_ARG_NUM] = {0};
	unsigned char ucargc = 0;

	/*check command length*/
	if ((!len) || (len > (MAX_CMD_LENGTH - 1))) {
		pr_err("Command length is too long or 0!\n");
		goto ERR_OUT;
	}

	/*copy command string from user space*/
	if (copy_from_user(cmd_line, buf, len)) {
		goto ERR_OUT;
	}

	cmd_line[len - 1] = '\0';

	printk("CMD:%s\n", cmd_line);

	/*parse command string*/
	for (ucargc = 0; ucargc < MAX_ARG_NUM; ucargc++) {
		argv[ucargc] = strsep(&cmdstr, delimiters);

		if (argv[ucargc] == NULL) {
			break;
		}
	}

	if (ucargc < 1) {
		pr_err("NULL command error\n");
		goto ERR_OUT;
	}

	if (!strcmp(argv[0], "showavl")) {
		cmd_efuse_show_avl();
		return size;
	} else if(!strcmp(argv[0], "version")) {
		pr_info("knlpkg.a 1.00.004\r\n");
	}
ERR_OUT:

	return size;
}

static struct file_operations proc_avl_fops = {
	.owner  = THIS_MODULE,
	.open   = nvt_avl_proc_help_open,
	.release = single_release,
	.read   = seq_read,
	.llseek = seq_lseek,
    .write   = nvt_avl_proc_cmd_write
};

int nvt_otp_proc_init(POTP_DRV_INFO pdrv_info)
{
	int ret = 0;
	struct proc_dir_entry *pmodule_root = NULL;
	struct proc_dir_entry *pentry = NULL;

	pmodule_root = proc_mkdir("nvt_otp_op", NULL);
	if (pmodule_root == NULL) {
		nvt_dbg(ERR, "failed to create Module root\n");
		ret = -EINVAL;
		goto remove_root;
	}

	pdrv_info->pproc_otp_root = pmodule_root;


	pentry = proc_create("otp_trim", S_IRUGO | S_IXUGO, pmodule_root, &proc_otp_fops);

  	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc otp!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_otp_entry = pentry;

	pentry = proc_create("avl", S_IRUGO | S_IXUGO, pmodule_root, &proc_avl_fops);

  	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc avl!\n");
		ret = -EINVAL;
		goto remove_cmd;
	}
	pdrv_info->pproc_avl_entry = pentry;

	return ret;
#if 0

	pentry = proc_create("dram2_info", S_IRUGO | S_IXUGO, pmodule_root, &proc_dram2_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc dram2!\n");
		ret = -EINVAL;
		goto remove_dram2_proc;
	}
	pdrv_info->pproc_dram2_entry = pentry;

	pentry = proc_create("dram1_heavyload", S_IRUGO | S_IXUGO, pmodule_root, &proc_dram_heavyload_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc dram1!\n");
		ret = -EINVAL;
		goto remove_dram1_heavyload_proc;
	}
	pdrv_info->pproc_dram1_heavyload_entry = pentry;


	pentry = proc_create("dram2_heavyload", S_IRUGO | S_IXUGO, pmodule_root, &proc_dram2_heavyload_fops);
	if (pentry == NULL) {
		nvt_dbg(ERR, "failed to create proc dram2!\n");
		ret = -EINVAL;
		goto remove_dram2_heavyload_proc;
	}
	pdrv_info->pproc_dram2_heavyload_entry = pentry;

	nvt_ddr_proc_cfg.timer_id = TIMER_INVALID;
	nvt_dram2_proc_cfg.timer_id = TIMER_INVALID;
	OS_CONFIG_FLAG(nvt_ddr_proc_flag_id);
	OS_CONFIG_FLAG(nvt_dram1_heavyload_flag_id);
	OS_CONFIG_FLAG(nvt_dram2_heavyload_flag_id);
	THREAD_CREATE(nvt_ddr_proc_tsk_id, dbgut_tsk, NULL, "nvt_ddr_proc_tsk");
	THREAD_CREATE(nvt_dram1_heavyload_tsk_id, heavyload_tsk, NULL, "nvt_dram1_heavyload_tsk");
	THREAD_CREATE(nvt_dram2_heavyload_tsk_id, heavyload_tsk2, NULL, "nvt_dram2_heavyload_tsk");
	THREAD_RESUME(nvt_ddr_proc_tsk_id);
	THREAD_RESUME(nvt_dram1_heavyload_tsk_id);
	THREAD_RESUME(nvt_dram2_heavyload_tsk_id);



#if (DBG_TEST_EN == 1)
	pentry = proc_create("cmd", S_IRUGO | S_IXUGO, pmodule_root, &proc_cmd_fops);
        if (pentry == NULL) {
                nvt_dbg(ERR, "failed to create proc cmd!\n");
                ret = -EINVAL;
                goto remove_cmd;
        }
        pdrv_info->pproc_cmd_entry = pentry;
#endif

	return ret;

remove_dram2_heavyload_proc:
	proc_remove(pdrv_info->pproc_dram2_heavyload_entry);

remove_dram1_heavyload_proc:
	proc_remove(pdrv_info->pproc_dram1_heavyload_entry);

remove_dram2_proc:
	proc_remove(pdrv_info->pproc_dram2_entry);
#endif
remove_cmd:
	proc_remove(pdrv_info->pproc_otp_entry);
	proc_remove(pdrv_info->pproc_avl_entry);

remove_root:
	proc_remove(pdrv_info->pproc_otp_root);
	return ret;
}

int nvt_otp_proc_remove(POTP_DRV_INFO pdrv_info)
{
#if 0
printk("%s: rm thread 0x%x\r\n", __func__, (UINT32)nvt_ddr_proc_tsk_id);
	set_flg(nvt_ddr_proc_flag_id, FLGDBGUT_QUIT);
	set_flg(nvt_dram1_heavyload_flag_id, FLGDBGUT_QUIT_HVY);
	set_flg(nvt_dram2_heavyload_flag_id, FLGDBGUT_QUIT_HVY2);
	THREAD_DESTROY(nvt_ddr_proc_tsk_id);
	THREAD_DESTROY(nvt_dram1_heavyload_tsk_id);
	THREAD_DESTROY(nvt_dram2_heavyload_tsk_id);
printk("%s: rm flag\r\n", __func__);
	rel_flg(nvt_ddr_proc_flag_id);
	rel_flg(nvt_dram1_heavyload_flag_id);
	rel_flg(nvt_dram2_heavyload_flag_id);
printk("%s: rm proc\r\n", __func__);
	proc_remove(pdrv_info->pproc_dram2_heavyload_entry);
	proc_remove(pdrv_info->pproc_dram1_heavyload_entry);
	proc_remove(pdrv_info->pproc_dram2_entry);
	proc_remove(pdrv_info->pproc_dram1_entry);
	proc_remove(pdrv_info->pproc_ddr_root);
#endif
	return 0;
}
