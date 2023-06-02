/*
 * Copyright (c) Hisilicon Technologies Co., Ltd. 2017-2019. All rights reserved.
 * Description   : head file for cipher drv osal configuration to hi3519av100.
 * Author        : Hisilicon multimedia software group
 * Create        : 2017-10-20
 */

#ifndef __DRV_OSAL_HI3519AV100_H__
#define __DRV_OSAL_HI3519AV100_H__

/* the total cipher hard channel which we can used. */
#define CIPHER_HARD_CHANNEL_CNT         0x07
/* mask which cipher channel we can used, bit0 means channel 0. */
#define CIPHER_HARD_CHANNEL_MASK        0xFE

/* the total hash hard channel which we can used. */
#define HASH_HARD_CHANNEL_CNT           0x01

#ifdef __HuaweiLite__
/* liteos resource config */
/* mask which cipher channel we can used, bit0 means channel 0. */
#define HASH_HARD_CHANNEL_MASK          0x04
#define HASH_HARD_CHANNEL               0x02
#else
/* mask which cipher channel we can used, bit0 means channel 0. */
#define HASH_HARD_CHANNEL_MASK          0x02
#define HASH_HARD_CHANNEL               0x01
#endif

/* the total cipher hard key channel which we can used. */
#define CIPHER_HARD_KEY_CHANNEL_CNT     0x04

/* mask which cipher hard key channel we can used, bit0 means channel 0. */
#define CIPHER_HARD_KEY_CHANNEL_MASK    0xF0

/* support read IRQ number from DTS */
#define IRQ_DTS_SUPPORT

/* support OTP load key */
#define OTP_SUPPORT

#ifndef __HuaweiLite__
/* support interrupt. */
#define CRYPTO_OS_INT_SUPPORT
#endif

/* the hardware version */
#define CHIP_SYMC_VER_V200
#define CHIP_HASH_VER_V200
#define CHIP_IFEP_RSA_VER_V100

/* support des */
#define CHIP_DES_SUPPORT

/* support 3des */
#define CHIP_3DES_SUPPORT

/* the hardware capacity */
#define CHIP_AES_CCM_GCM_SUPPORT

/* moudle unsupport, we need set the table. */
#define BASE_TABLE_NULL { \
.reset_valid = 0,  \
.clk_valid = 0, \
.phy_valid = 0, \
.crg_valid = 0, \
.ver_valid = 0, \
.int_valid = 0, \
}

#if defined(ARCH_TYPE_amp) && !defined(AMP_NONSECURE_VERSION) && !defined(__HuaweiLite__)

/* linux of AMP use non-secure config */
#define HARD_INFO_TRNG                BASE_TABLE_NULL

#define HARD_INFO_CIPHER { \
.name = "nonsec_cipher",  \
.reset_valid = 1,  \
.clk_valid = 1, \
.phy_valid = 1, \
.crg_valid = 1, \
.ver_valid = 1, \
.int_valid = 1, \
.int_num = 126, \
.reset_bit = 0, \
.clk_bit = 1, \
.version_reg = 0x308, \
.version_val = 0x20170609, \
.reg_addr_phy = 0x04060000, \
.reg_addr_size = 0x4000,    \
.crg_addr_phy = 0x0451016c, \
}

/* define initial value of struct sys_arch_boot_dts for cipher. */
#define HARD_INFO_HASH { \
.name = "nonsec_hash",  \
.reset_valid = 0,  \
.clk_valid = 0, \
.phy_valid = 1, \
.crg_valid = 0, \
.ver_valid = 1, \
.int_valid = 1, \
.int_num = 126, \
.reset_bit = 0, \
.clk_bit = 1, \
.version_reg = 0x308, \
.version_val = 0x20170609, \
.reg_addr_phy = 0x04060000, \
.reg_addr_size = 0x4000, \
.crg_addr_phy = 0x0451016c, \
}

#define HARD_INFO_IFEP_RSA { \
.name = "nonsec_rsa",  \
.reset_valid = 1,  \
.clk_valid = 1, \
.phy_valid = 1, \
.crg_valid = 1, \
.ver_valid = 1, \
.int_valid = 1, \
.reg_addr_phy = 0x04080000,  \
.reg_addr_size = 0x1000, \
.crg_addr_phy = 0x04510194, \
.reset_bit = 14, \
.clk_bit = 15, \
.int_num = 117, \
.version_reg = 0x90, \
.version_val = 0x20160907, \
}
#else
/* SMP, liteos of secure AMP, linux of non-secure AMP use secure config */
#define CRYPTO_SEC_CPU

#define CHIP_TRNG_VER_V200

/* define initial value of struct sys_arch_boot_dts for cipher. */
#define HARD_INFO_CIPHER { \
.name = "cipher",  \
.reset_valid = 1,  \
.clk_valid = 1, \
.phy_valid = 1, \
.crg_valid = 1, \
.ver_valid = 1, \
.int_valid = 1, \
.int_num = 125, \
.reset_bit = 0, \
.clk_bit = 1, \
.version_reg = 0x308, \
.version_val = 0x20170609, \
.reg_addr_phy = 0x04060000, \
.reg_addr_size = 0x4000,    \
.crg_addr_phy = 0x0451016c, \
}

/* define initial value of struct sys_arch_boot_dts for cipher. */
#define HARD_INFO_HASH { \
.name = "hash",  \
.reset_valid = 0,  \
.clk_valid = 0, \
.phy_valid = 1, \
.crg_valid = 0, \
.ver_valid = 1, \
.int_valid = 1, \
.int_num = 125, \
.reset_bit = 0, \
.clk_bit = 1, \
.version_reg = 0x308, \
.version_val = 0x20170609, \
.reg_addr_phy = 0x04060000, \
.reg_addr_size = 0x4000, \
.crg_addr_phy = 0x0451016c, \
}

/* linux use rsa0, liteos use rsa1 */
#define HARD_INFO_IFEP_RSA { \
.name = "rsa",  \
.reset_valid = 1,  \
.clk_valid = 1, \
.phy_valid = 1, \
.crg_valid = 1, \
.ver_valid = 1, \
.int_valid = 1, \
.reg_addr_phy = 0x04088000,  \
.reg_addr_size = 0x1000, \
.crg_addr_phy = 0x04510194, \
.reset_bit = 22, \
.clk_bit = 23, \
.int_num = 127, \
.version_reg = 0x90, \
.version_val = 0x20160907, \
}

/* define initial value of struct sys_arch_boot_dts for HASH. */
#define HARD_INFO_TRNG { \
.name = "trng",  \
.reset_valid = 1,  \
.clk_valid = 1, \
.phy_valid = 1, \
.crg_valid = 1, \
.ver_valid = 0, \
.int_valid = 0, \
.reset_bit = 12, \
.clk_bit = 13, \
.reg_addr_phy = 0x04090200,  \
.reg_addr_size = 0x100,   \
.crg_addr_phy = 0x04510194, \
}
#endif

#define KLAD_REG_BASE_ADDR_PHY          0x04070000
#define OTP_REG_BASE_ADDR_PHY           0x040A0000
#define KLAD_CRG_ADDR_PHY               0x04510194
#define REG_SYS_OTP_CLK_ADDR_PHY        0x04510194
#define SPACC_PROT_BYPASS_N             0x04C50004

#define SPACC_PROT_BYPASS_N_BIT         (0x01 << 12)
#define RSA1_PROT_BYPASS_N_BIT          (0x01 << 18)

#define OTP_CRG_CLOCK_BIT               (0x01 << 7)

#define KLAD_CRG_CLOCK_BIT              (0x01 << 11)
#define KLAD_CRG_RESET_BIT              (0x01 << 10)

/* support smmu.
 * CRYPTO_SMMU_SUPPORT
 *
 * RSA rand mask.
 * RSA_RAND_MASK
 *
 * support chip sm2.
 * CHIP_SM2_VER_V100
 *
 * supoort odd key.
 * CHIP_SYMC_ODD_KEY_SUPPORT
 *
 * supoort SM1
 * CHIP_SYMC_SM1_SUPPORT
 *
 *
 * the software capacity.
 * SOFT_AES_SUPPORT
 * SOFT_TDES_SUPPORT
 * SOFT_AES_CCM_GCM_SUPPORT
 * SOFT_SHA1_SUPPORT
 * SOFT_SHA256_SUPPORT
 * SOFT_SHA512_SUPPORT
 * SOFT_SM2_SUPPORT
 * SOFT_SM3_SUPPORT
 * SOFT_ECC_SUPPORT
 * SOFT_AES_CTS_SUPPORT
 */
#define HARD_INFO_SMMU                BASE_TABLE_NULL
#define HARD_INFO_SIC_RSA             BASE_TABLE_NULL
#define HARD_INFO_CIPHER_KEY          BASE_TABLE_NULL
#define HARD_INFO_SM4                 BASE_TABLE_NULL
#define HARD_INFO_SM2                 BASE_TABLE_NULL

/*
 * supoort switch cpu
 * CRYPTO_SWITCH_CPU
 */
#define NSEC_HARD_INFO_CIPHER              BASE_TABLE_NULL
#define NSEC_HARD_INFO_HASH                BASE_TABLE_NULL
#define NSEC_HARD_INFO_IFEP_RSA            BASE_TABLE_NULL
#define NSEC_HARD_INFO_SMMU                BASE_TABLE_NULL
#define NSEC_HARD_INFO_SIC_RSA             BASE_TABLE_NULL
#define NSEC_HARD_INFO_CIPHER_KEY          BASE_TABLE_NULL
#define NSEC_HARD_INFO_SM4                 BASE_TABLE_NULL
#define NSEC_HARD_INFO_SM2                 BASE_TABLE_NULL
#define NSEC_HARD_INFO_TRNG                BASE_TABLE_NULL

#endif /* __DRV_OSAL_HI3519AV100_H__ */
