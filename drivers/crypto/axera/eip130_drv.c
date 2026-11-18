/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#if defined(CONFIG_AX_CIPHER_CRYPTO) || defined(CONFIG_AX_CIPHER_MISC)

#include <linux/atomic.h>
#include <linux/sched.h>
#include <linux/wait.h>
#include <linux/jiffies.h>
#include "ax_cipher_adapt.h"
#include "eip130_drv.h"
#include "eip130_fw.h"
//#include "ax_printk.h"

#define BOND0_REG (0x550000C)
#define SECURE_BOOT_EN (1 << 26)
#define HASH_BLK_NUM 8
#define EIP130_REG_BASE (0xA00000)
#define EIP130_REG_SIZE (0x20000)
#define EIP130_MAILBOX_IN_BASE (0x0000)
#define EIP130_MAILBOX_OUT_BASE				 (0x0000)
#define EIP130_MAILBOX_SPACING_BYTES			(0x0400)
#define EIP130_FIRMWARE_RAM_BASE				(0x4000)
#define EIP130_AIC_POL_CTRL					 (0x3E00)
#define EIP130_AIC_TYPE_CTRL					(0x3E04)
#define EIP130_AIC_ENABLE_CTRL				  (0x3E08)
#define EIP130_AIC_ENABLE_SET				   (0x3E0C)
#define EIP130_AIC_RAW_STAT					 (0x3E0C)
#define EIP130_AIC_ENABLED_STAT				 (0x3E10)
#define EIP130_AIC_ACK						  (0x3E10)
#define EIP130_AIC_ENABLE_CLR				   (0x3E14)
#define EIP130_AIC_OPTIONS					  (0x3E18)
#define EIP130_AIC_VERSION					  (0x3E1C)
#define EIP130_MAILBOX_STAT					 (0x3F00)
#define EIP130_MAILBOX_CTRL					 (0x3F00)
#define EIP130_MAILBOX_LOCKOUT				  (0x3F10)
#define EIP130_MODULE_STATUS					(0x3FE0)
#define EIP130_WAIT_TIMEOUT					 (10000000)
#define EIP130_RAM_SIZE						 (0x1C000)
#define EIP130_CRC24_BUSY					   (1 << 8)
#define EIP130_CRC24_OK						 (1 << 9)
#define EIP130_FIRMWARE_WRITTEN				 (1 << 20)
#define EIP130_FIRMWARE_CHECKS_DONE			 (1 << 22)
#define EIP130_FIRMWARE_ACCEPTED				(1 << 23)
#define EIP130_FATAL_ERROR					  (1 << 31)
#define CRYPTO_OFFICER_ID					   0x4F5A3647
#define CE_CLK_NAME							 "ce_core"

#define CE_TIMEOUT							  (3000) // ms
#define EIP130_MAILBOX_NR						   (4)
struct ax_ce_dev {
	void *base;
	int mailbox_used;
	int result_token[EIP130_MAILBOX_NR][64];
	wait_queue_head_t ce_wtask[EIP130_MAILBOX_NR];
	atomic_t   e_event[EIP130_MAILBOX_NR];
};
static struct ax_ce_dev g_cipher_dev;
static int g_ce_init = 0;
DEFINE_MUTEX(g_cipher_mutex);
uint32_t trng_configed = 0;

static int eip130_mailbox_link(const uint8_t mailbox_nr)
{
	uint32_t set_val;
	uint32_t get_val;
	if (mailbox_nr < 1 || mailbox_nr > 8) {
		return AX_CIPHER_OPERATION_FAILED;
	}
	set_val = 4 << ((mailbox_nr - 1) * 4);
	ax_cipher_regwrite(set_val, (g_cipher_dev.base + EIP130_MAILBOX_CTRL));

	get_val = ax_cipher_regread(g_cipher_dev.base + EIP130_MAILBOX_STAT);
	if ((get_val & set_val) != set_val) {
		return AX_CIPHER_OPERATION_FAILED;
	}
	return AX_CIPHER_SUCCESS;
}
static uint32_t eip130_read_module_status(void)
{
	return ax_cipher_regread(g_cipher_dev.base + EIP130_MODULE_STATUS);
}
static void eip130_write_module_status(uint32_t value)
{
	ax_cipher_regwrite(value, g_cipher_dev.base + EIP130_MODULE_STATUS);
}
static int eip130_mailbox_unlink(int mailbox_nr)
{
	uint32_t set_value;
	uint32_t get_value;

	if (mailbox_nr < 1 || mailbox_nr > 8) {
		return AX_CIPHER_OPERATION_FAILED;
	}
	set_value = 8 << ((mailbox_nr - 1) * 4);
	ax_cipher_regwrite(set_value, g_cipher_dev.base + EIP130_MAILBOX_CTRL);
	get_value = ax_cipher_regread(g_cipher_dev.base + EIP130_MAILBOX_STAT);
	set_value >>= 1;
	if ((get_value & set_value) != 0) {
		return AX_CIPHER_OPERATION_FAILED;
	}
	return 0;
}
static int eip130_mailbox_can_write_token(int mailbox_nr)
{
	int val;
	uint32_t bit;
	if (mailbox_nr < 1 || mailbox_nr > 8) {
		return -1;
	}
	bit = 1 << ((mailbox_nr - 1) * 4);
	val = ax_cipher_regread((g_cipher_dev.base + EIP130_MAILBOX_STAT));
	if ((val & bit) == 0) {
		return 1;
	}
	return 0;
}
static int eip130_mailbox_can_read_token(int mailbox_nr)
{
	int val;
	int bit;
	bit = 2 << ((mailbox_nr - 1) * 4);
	val = ax_cipher_regread((g_cipher_dev.base + EIP130_MAILBOX_STAT));
	if ((val & bit) == 0) {
		return 0;
	}
	return 1;
}

static void eip130_write_u32array(void *addr, uint32_t *data, uint32_t cnt, int mailbox_nr)
{
	uint32_t *ptr = (uint32_t *)addr;
	int i;
	for (i = 0; i < cnt; i++) {
		ptr[i] = data[i];
	}
}

static void eip130_register_write_mailbox_control(uint32_t val, int mailbox_nr)
{
	ax_cipher_regwrite(val << (mailbox_nr -1) * 4, (g_cipher_dev.base + EIP130_MAILBOX_CTRL));
}

static int eip130_mailbox_write_and_submit_token(uint32_t *command_token, int mailbox_nr, int size)
{
	void *mailboxAddr = g_cipher_dev.base + EIP130_MAILBOX_IN_BASE;
	if (mailbox_nr < 1 || mailbox_nr > 4) {
		return -1;
	}

	mailboxAddr += (EIP130_MAILBOX_SPACING_BYTES * (mailbox_nr - 1));
	eip130_write_u32array(mailboxAddr, command_token, size, mailbox_nr);
	eip130_register_write_mailbox_control(1, mailbox_nr);
	return AX_CIPHER_SUCCESS;
}
static int eip130_register_write_mailbox_lockout(uint32_t val)
{
	ax_cipher_regwrite(val, (g_cipher_dev.base + EIP130_MAILBOX_LOCKOUT));
	return AX_CIPHER_SUCCESS;
}

static int eip130_mailbox_read_token(uint32_t *result_token, int mailbox_nr)
{
	void *mailbox;
	if (!eip130_mailbox_can_read_token(mailbox_nr)) {
		return -3;
	}
	mailbox = g_cipher_dev.base + EIP130_MAILBOX_IN_BASE + (mailbox_nr - 1) * EIP130_MAILBOX_SPACING_BYTES;
	eip130_write_u32array((void *) result_token, (uint32_t *)mailbox, 64, mailbox_nr);
	eip130_register_write_mailbox_control(2, mailbox_nr);
	return 0;
}

int eip130_wait_for_result_token(int mailbox_nr)
{
	int i = 0;
	// Poll for output token available with sleep
	while (i < EIP130_WAIT_TIMEOUT) {
		if (eip130_mailbox_can_read_token(mailbox_nr)) {
			return AX_CIPHER_SUCCESS;
		}
		i++;
	}
	return AX_CIPHER_OPERATION_FAILED;
}

int eip130_physical_token_exchange(uint32_t *command_token,
								   uint32_t *result_token,
								   uint32_t mailbox_nr)
{
	int ret,tmp;
	int event_count = 0;
	// Set identity in token if not the Provision Random HUK token
	if ((command_token[0] & (0xff << 24)) !=
		(uint32_t)((EIP130TOKEN_OPCODE_ASSETMANAGEMENT	 << 24) |
				 (EIP130TOKEN_SUBCODE_PROVISIONRANDOMHUK << 28))) {
		command_token[1] = CRYPTO_OFFICER_ID;
	}
	tmp = mailbox_nr-1;
	eip130_mailbox_write_and_submit_token(command_token, mailbox_nr, 64);
	do {
		event_count = atomic_read(&g_cipher_dev.e_event[mailbox_nr - 1]);
		if (event_count == 0) {
			ret = wait_event_timeout(g_cipher_dev.ce_wtask[mailbox_nr - 1],
					atomic_read(&g_cipher_dev.e_event[tmp]),
					msecs_to_jiffies(CE_TIMEOUT));
			if (ret <= 0) {
				CE_LOG_PR(CE_ERR_LEVEL,"timeout\n");
				return AX_CIPHER_OPERATION_FAILED;
			}
		}
		atomic_dec_return(&g_cipher_dev.e_event[mailbox_nr - 1]);
		// Read the result token from the OUT mailbox
		memcpy(result_token, g_cipher_dev.result_token[mailbox_nr - 1], 256);
	} while ((command_token[0] & 0xffff) != (result_token[0] & 0xffff));
	if (result_token[0] & (0x1 << 31)) {
		CE_LOG_PR(CE_ERR_LEVEL, "result_token error result_token[0] = 0x%x\n", result_token[0]);
		return AX_CIPHER_OPERATION_FAILED;
	}
	return 0;
}

int eip130_Interrupt_handle(void)
{
	int reg;
	int i;
	int bit;

	reg = ax_cipher_regread(g_cipher_dev.base + EIP130_AIC_RAW_STAT);
	for (i = 0; i < EIP130_MAILBOX_NR; i++) {
		bit = i * 2 + 1;
		if (reg & (1 << bit)) {
			/*clear token_done interrupt*/
			ax_cipher_regwrite(1 << bit, g_cipher_dev.base + EIP130_AIC_ACK);
			if (i + 1 == OCCUPY_CE_MAILBOX_NR) {
				eip130_mailbox_read_token(g_cipher_dev.result_token[i], i + 1);
				atomic_inc_return(&g_cipher_dev.e_event[i]);
				wake_up(&g_cipher_dev.ce_wtask[i]);
			}
		}
	}
	return 0;
}
static int eip130_firmware_check(void)
{
	uint32_t value = 0;
	do {
		value = eip130_read_module_status();
	} while ((value & EIP130_CRC24_BUSY) != 0);
	if (((value & EIP130_CRC24_OK) == 0) ||
		((value & EIP130_FATAL_ERROR) != 0)) {
		return -3;
	}
	if ((value & EIP130_FIRMWARE_WRITTEN) == 0) {
		return 0;
	}
	// - Check if firmware checks are done & accepted
	if ((value & EIP130_FIRMWARE_CHECKS_DONE) == 0) {
		return 1;
	} else if ((value & EIP130_FIRMWARE_ACCEPTED) != 0) {
		return 2;
	}
	return 0;
}
static int eip130_firmware_load(void *fw_addr, int size)
{
	int value;
	int retries = 3;
	int rc;
	int nretries;
	int mailbox_nr = 1;
	for (; retries > 0; retries--) {
		rc = eip130_firmware_check();
		if (rc < 0) {
			CE_LOG_PR(CE_ERR_LEVEL,"firmware check fail %x\n", rc);
			return AX_CIPHER_INTERNAL_ERROR;
		}
		if (rc == 2) {
			CE_LOG_PR(CE_INFO_LEVEL, "firmware had OK %x\n", rc);
			return 0;
		}
		if (rc != 1) {
			rc = eip130_mailbox_link(mailbox_nr);
			if (rc != AX_CIPHER_SUCCESS) {
				CE_LOG_PR(CE_ERR_LEVEL,"MailboxLink fail, ret = %x\n", rc);
				return AX_CIPHER_INTERNAL_ERROR;
			}
			rc = eip130_mailbox_write_and_submit_token((uint32_t *)fw_addr, mailbox_nr, 64);
			eip130_mailbox_unlink(mailbox_nr);
			if (rc < 0) {
				CE_LOG_PR(CE_ERR_LEVEL,"mailbox write fail %x\n", rc);
				return AX_CIPHER_INTERNAL_ERROR;
			}
			eip130_write_u32array((void *)g_cipher_dev.base + EIP130_FIRMWARE_RAM_BASE, (uint32_t *)(fw_addr + 256), (size - 256) / 4,
								1);
			eip130_write_module_status(EIP130_FIRMWARE_WRITTEN);
		}
		value = eip130_read_module_status();
		if (((value & EIP130_CRC24_OK) == 0) ||
			((value & EIP130_FATAL_ERROR) != 0)) {
			CE_LOG_PR(CE_ERR_LEVEL,"check fail %x\n", value);
			return AX_CIPHER_INTERNAL_ERROR;
		}
		if ((value & EIP130_FIRMWARE_WRITTEN) == 0) {
			CE_LOG_PR(CE_ERR_LEVEL,"FW Writen fail %x\n", value);
			return AX_CIPHER_INTERNAL_ERROR;
		}
		for (nretries = 0x7FFFFFF; nretries && ((value & EIP130_FIRMWARE_CHECKS_DONE) == 0); nretries--) {
			value = eip130_read_module_status();
		}
		if ((value & EIP130_FIRMWARE_CHECKS_DONE) == 0) {
			CE_LOG_PR(CE_ERR_LEVEL,"FW check fail %x\n", value);
			return AX_CIPHER_INTERNAL_ERROR;
		}
		if ((value & EIP130_FIRMWARE_ACCEPTED) == EIP130_FIRMWARE_ACCEPTED) {
			CE_LOG_PR(CE_ERR_LEVEL, "FW Accepted %x\n", value);
			return 0;
		}
		CE_LOG_PR(CE_ERR_LEVEL, "FW Not Accepted %x\n", value);
	}
	return AX_CIPHER_INTERNAL_ERROR;
}
static void eip130_aic_init(void *regBase)
{
	/*configure high level interrupt*/
	ax_cipher_regwrite(0x1ff, regBase + EIP130_AIC_POL_CTRL);
	/*configure interrupt level mode*/
	ax_cipher_regwrite(0, regBase + EIP130_AIC_TYPE_CTRL);
	/*mbox1/2/3/4_token_done interrupt enable*/
	ax_cipher_regwrite(0xa8, regBase + EIP130_AIC_ENABLE_CTRL);
}
static void eip130_aic_deinit(void *regBase)
{
	/*mbox1/2/3/4_token_done interrupt disable*/
	ax_cipher_regwrite(0, regBase + EIP130_AIC_ENABLE_CTRL);
}
static int eip130_init(unsigned long fw_addr, int size)
{
	int ret;
#if 0
	int i;
	for (i = 0; i < EIP130_MAILBOX_NR; i++) {
		ret = eip130_mailbox_link(i + 1);
		if (ret != 0) {
			return AX_CIPHER_INTERNAL_ERROR;
		}
	}
#endif
	ret = eip130_firmware_load((void *)fw_addr, size);
	if (ret != 0) {
		CE_LOG_PR(CE_ERR_LEVEL,"FW Load fail, ret = %x\n", ret);
		return ret;
	}
	ret = eip130_mailbox_link(OCCUPY_CE_MAILBOX_NR);
	if (ret != AX_CIPHER_SUCCESS) {
		CE_LOG_PR(CE_ERR_LEVEL,"MailboxLink fail, ret = %x\n", ret);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	ret = eip130_mailbox_can_write_token(OCCUPY_CE_MAILBOX_NR);
	if (ret != 1) {
		CE_LOG_PR(CE_ERR_LEVEL,"can't writetoken, ret = %x\n", ret);
		return AX_CIPHER_OPERATION_FAILED;
	}
	if (eip130_register_write_mailbox_lockout(0) != AX_CIPHER_SUCCESS) {
		return AX_CIPHER_OPERATION_FAILED;
	}
	return AX_CIPHER_SUCCESS;
}

static int ce_enable(void)
{
	return AX_CIPHER_SUCCESS;
}
static int ce_disable(void)
{
	return AX_CIPHER_SUCCESS;
}

int ce_init(void *reg_base)
{
	int ret;
	int i;
	if (g_ce_init) {
		return AX_CIPHER_SUCCESS;
	}
	ret = ce_enable();
	if (ret < 0) {
		CE_LOG_PR(CE_ERR_LEVEL,"ret = %x\n", ret);
		return ret;
	}
	g_cipher_dev.base = reg_base;
	if (ret < 0) {
		CE_LOG_PR(CE_ERR_LEVEL,"ret = %x\n", ret);
		return ret;
	}
	ret = eip130_init((unsigned long)eip130_firmware, sizeof(eip130_firmware));
	if (ret != AX_CIPHER_SUCCESS) {
		CE_LOG_PR(CE_ERR_LEVEL,"ret = %x\n", ret);
		return -1;
	}
	for (i = 0; i < EIP130_MAILBOX_NR; i++) {
		init_waitqueue_head(&g_cipher_dev.ce_wtask[i]);
		atomic_set(&g_cipher_dev.e_event[i], 0);
	}
	g_ce_init = 1;
	eip130_aic_init(reg_base);
	return 0;
}

int ce_resume(void)
{
	int ret;
	void *reg_base = g_cipher_dev.base;
	ret = eip130_init((unsigned long)eip130_firmware, sizeof(eip130_firmware));
	if (ret != AX_CIPHER_SUCCESS) {
		CE_LOG_PR(CE_ERR_LEVEL,"ret = %x\n", ret);
		return -1;
	}
	ret = eip130_mailbox_link(1);
	if (ret != AX_CIPHER_SUCCESS) {
		CE_LOG_PR(CE_ERR_LEVEL,"MailboxLink fail, ret = %x\n", ret);
		return AX_CIPHER_INTERNAL_ERROR;
	}
	eip130_aic_init(reg_base);
	return 0;
}

int ce_deinit(void)
{
	if (g_ce_init) {
		ce_disable();
	}
	g_ce_init = 0;
	eip130_aic_deinit(g_cipher_dev.base);
	return ce_disable();
}
#endif
