/*
 * Based on arch/arm64/kernel/chipid-sunxi.c
 *
 * Copyright (C) 2015 Allwinnertech Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/sunxi-smc.h>
#include <linux/sunxi-sid.h>
#include <linux/arm-smccc.h>
#include <linux/bitops.h>

#ifndef OPTEE_SMC_STD_CALL_VAL
#define OPTEE_SMC_STD_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_STD_CALL, ARM_SMCCC_SMC_32, \
			   ARM_SMCCC_OWNER_TRUSTED_OS, (func_num))
#endif

#ifndef OPTEE_SMC_FAST_CALL_VAL
#define OPTEE_SMC_FAST_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32, \
			   ARM_SMCCC_OWNER_TRUSTED_OS, (func_num))
#endif
/*
 * Function specified by SMC Calling convention.
 */
#define OPTEE_SMC_FUNCID_READ_REG  (17 | sunxi_smc_call_offset())
#define OPTEE_SMC_READ_REG \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_READ_REG)


#define OPTEE_SMC_FUNCID_WRITE_REG  (18 | sunxi_smc_call_offset())
#define OPTEE_SMC_WRITE_REG \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_WRITE_REG)

#define OPTEE_SMC_FUNCID_COPY_ARISC_PARAS  (19 | sunxi_smc_call_offset())
#define OPTEE_SMC_COPY_ARISC_PARAS \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_COPY_ARISC_PARAS)
#define ARM_SVC_COPY_ARISC_PARAS    OPTEE_SMC_COPY_ARISC_PARAS

#define OPTEE_SMC_FUNCID_GET_TEEADDR_PARAS  (20 | sunxi_smc_call_offset())
#define OPTEE_SMC_GET_TEEADDR_PARAS \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_GET_TEEADDR_PARAS)
#define ARM_SVC_GET_TEEADDR_PARAS    OPTEE_SMC_GET_TEEADDR_PARAS

#define OPTEE_SMC_FUNCID_READ_EFUSE  24
#define OPTEE_SMC_READ_EFUSE \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_READ_EFUSE)


#define OPTEE_SMC_FUNCID_WRITE_EFUSE  25
#define OPTEE_SMC_WRITE_EFUSE \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_WRITE_EFUSE)

#define OPTEE_SMC_FUNCID_SUNXI_TCOFFER_VER (27 | sunxi_smc_call_offset())
#define OPTEE_SMC_SUNXI_TCOFFER_VER \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_SUNXI_TCOFFER_VER)

#ifdef CONFIG_ARM64
/*cmd to call ATF service*/
#define ARM_SVC_EFUSE_PROBE_SECURE_ENABLE    (0xc000fe03)
#define ARM_SVC_READ_SEC_REG                 (0xC000ff05)
#define ARM_SVC_WRITE_SEC_REG                (0xC000ff06)
#define ARM_SVC_EFUSE_READ_AARCH64	(0xc000fe00)
#define ARM_SVC_EFUSE_WRITE_AARCH64	(0xc000fe01)
#define ARM_SVC_EFUSE_READ	ARM_SVC_EFUSE_READ_AARCH64
#define ARM_SVC_EFUSE_WRITE	ARM_SVC_EFUSE_WRITE_AARCH64
#else
/*cmd to call TEE service*/
#define ARM_SVC_READ_SEC_REG        OPTEE_SMC_READ_REG
#define ARM_SVC_WRITE_SEC_REG       OPTEE_SMC_WRITE_REG
#define ARM_SVC_EFUSE_READ	OPTEE_SMC_READ_EFUSE
#define ARM_SVC_EFUSE_WRITE	OPTEE_SMC_WRITE_EFUSE
#endif


/*interface for smc */
u32 sunxi_smc_readl(phys_addr_t addr)
{
#if defined(CONFIG_ARM64)
	return invoke_smc_fn(ARM_SVC_READ_SEC_REG, addr, 0, 0);
#elif defined(CONFIG_TEE)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_READ_SEC_REG, addr, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
#else
	void __iomem *vaddr = ioremap(addr, 4);
	u32 ret;
	ret = readl(vaddr);
	iounmap(vaddr);
	return ret;
#endif
}
EXPORT_SYMBOL_GPL(sunxi_smc_readl);

int sunxi_smc_writel(u32 value, phys_addr_t addr)
{
#if defined(CONFIG_ARM64)
	return invoke_smc_fn(ARM_SVC_WRITE_SEC_REG, addr, value, 0);
#elif defined(CONFIG_TEE)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_WRITE_SEC_REG, addr, value, 0, 0, 0, 0, 0, &res);
	return res.a0;
#else
	void __iomem *vaddr = ioremap(addr, 4);
	writel(value, vaddr);
	iounmap(vaddr);
	return 0;
#endif
}
EXPORT_SYMBOL_GPL(sunxi_smc_writel);

int arm_svc_efuse_write(phys_addr_t key_buf)
{
#ifdef CONFIG_ARM64
	return invoke_smc_fn(ARM_SVC_EFUSE_WRITE,
				(unsigned long)key_buf, 0, 0);
#elif defined(CONFIG_TEE)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_EFUSE_WRITE, (unsigned long)key_buf, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
#else
	return -1;
#endif
}
EXPORT_SYMBOL_GPL(arm_svc_efuse_write);

int arm_svc_efuse_read(phys_addr_t key_buf, phys_addr_t read_buf)
{
#ifdef	CONFIG_ARM64
	return invoke_smc_fn(ARM_SVC_EFUSE_READ,
		(ulong)key_buf, (ulong)read_buf, 0);
#elif defined(CONFIG_TEE)
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_EFUSE_READ, (unsigned long)key_buf, (unsigned long)read_buf, 0, 0, 0, 0, 0, &res);
	return res.a0;
#else
	return -1;
#endif

}
EXPORT_SYMBOL_GPL(arm_svc_efuse_read);

int sunxi_smc_copy_arisc_paras(phys_addr_t dest, phys_addr_t src, u32 len)
{
	struct arm_smccc_res res;
	arm_smccc_smc(ARM_SVC_COPY_ARISC_PARAS, dest, src, len, 0, 0, 0, 0, &res);
	return res.a0;
}

phys_addr_t sunxi_smc_get_teeaddr_paras(phys_addr_t resumeaddr)
{
	struct arm_smccc_res res;

	arm_smccc_smc(ARM_SVC_GET_TEEADDR_PARAS,
		resumeaddr, 0, 0, 0, 0, 0, 0, &res);
	return res.a0;
}
/*optee smc*/
#define ARM_SMCCC_SMC_32		0
#define ARM_SMCCC_SMC_64		1
#define ARM_SMCCC_CALL_CONV_SHIFT	30

#define ARM_SMCCC_OWNER_MASK		0x3F
#define ARM_SMCCC_OWNER_SHIFT		24

#define ARM_SMCCC_FUNC_MASK		0xFFFF
#define ARM_SMCCC_OWNER_TRUSTED_OS	50

#define ARM_SMCCC_IS_FAST_CALL(smc_val)	\
	((smc_val) & (ARM_SMCCC_FAST_CALL << ARM_SMCCC_TYPE_SHIFT))
#define ARM_SMCCC_IS_64(smc_val) \
	((smc_val) & (ARM_SMCCC_SMC_64 << ARM_SMCCC_CALL_CONV_SHIFT))
#define ARM_SMCCC_FUNC_NUM(smc_val)	((smc_val) & ARM_SMCCC_FUNC_MASK)
#define ARM_SMCCC_OWNER_NUM(smc_val) \
	(((smc_val) >> ARM_SMCCC_OWNER_SHIFT) & ARM_SMCCC_OWNER_MASK)

#define ARM_SMCCC_CALL_VAL(type, calling_convention, owner, func_num) \
	(((type) << ARM_SMCCC_TYPE_SHIFT) | \
	((calling_convention) << ARM_SMCCC_CALL_CONV_SHIFT) | \
	(((owner) & ARM_SMCCC_OWNER_MASK) << ARM_SMCCC_OWNER_SHIFT) | \
	((func_num) & ARM_SMCCC_FUNC_MASK))

#define OPTEE_SMC_FAST_CALL_VAL(func_num) \
	ARM_SMCCC_CALL_VAL(ARM_SMCCC_FAST_CALL, ARM_SMCCC_SMC_32, \
			   ARM_SMCCC_OWNER_TRUSTED_OS, (func_num))

#define OPTEE_SMC_FUNCID_CRYPT  16
#define OPTEE_SMC_CRYPT \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_SMC_FUNCID_CRYPT)

#define TEESMC_RSSK_DECRYPT      5
int sunxi_smc_refresh_hdcp(void)
{
	return invoke_smc_fn(OPTEE_SMC_CRYPT, TEESMC_RSSK_DECRYPT, 0, 0);
}
EXPORT_SYMBOL(sunxi_smc_refresh_hdcp);

#define OPTEE_MSG_FUNCID_GET_OS_REVISION	0x0001
#define OPTEE_SMC_CALL_GET_OS_REVISION \
	OPTEE_SMC_FAST_CALL_VAL(OPTEE_MSG_FUNCID_GET_OS_REVISION)

int smc_tee_get_os_revision(uint32_t *major, uint32_t *minor)
{
	struct arm_smccc_res param = { 0 };

	arm_smccc_smc(OPTEE_SMC_CALL_GET_OS_REVISION, 0, 0, 0, 0, 0, 0, 0,
		      &param);
	*major = param.a0;
	*minor = param.a1;
	return 0;
}

int sunxi_smc_call_offset(void)
{
	uint32_t major, minor;

	smc_tee_get_os_revision(&major, &minor);
	if ((major > 3) || ((major == 3) && (minor >= 5))) {
		return SUNXI_OPTEE_SMC_OFFSET;
	} else {
		return 0;
	}
}

int sunxi_smc_get_tcoffer_ver(uint32_t *major, uint32_t *minor, uint32_t *patch)
{
	struct arm_smccc_res param = { 0 };

	arm_smccc_smc(OPTEE_SMC_SUNXI_TCOFFER_VER, 0, 0, 0, 0, 0, 0, 0, &param);

	if (param.a0 == 0xFFFFFFFF) {
		/* optee os not supported, assume t-coffer v1.0.0 */
		*major = 1;
		*minor = 0;
		*patch = 0;
	} else {
		*major = param.a1;
		*minor = param.a2;
		*patch = param.a3;
	}
	return 0;
}
EXPORT_SYMBOL(sunxi_smc_get_tcoffer_ver);
