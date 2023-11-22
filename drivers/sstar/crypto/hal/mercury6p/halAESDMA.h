/*
 * halAESDMA.h- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#ifndef _HAL_AESDMA_H_
#define _HAL_AESDMA_H_

#define CONFIG_SRAM_DUMMY_ACCESS_RSA
//#define CONFIG_SRAM_DUMMY_ACCESS_SHA

#define CONFIG_MS_CRYPTO_SUPPORT_AES256 1

#ifdef CONFIG_ARM64
//#define CONFIG_RIU_BASE_ADDRESS     0xFFFFFF80FD000000
#define CONFIG_RIU_BASE_ADDRESS (MS_IO_OFFSET + 0x1f000000UL)

#else
#define CONFIG_RIU_BASE_ADDRESS 0xFD000000
#endif

#define CONFIG_XIU_BASE_ADDRESS  0x1F600000
#define CONFIG_EMMC_BASE_ADDRESS 0x1FC00000

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define PKA_AESDMA_DEBUG (0)
#if (PKA_AESDMA_DEBUG == 1)
#define PKA_AESDMA_DBG(fmt, arg...) printk(KERN_ALERT fmt, ##arg) // KERN_DEBUG KERN_ALERT KERN_WARNING
#else
#define PKA_AESDMA_DBG(fmt, arg...)
#endif

#define REG(Reg_Addr)               (*(volatile unsigned short *)(Reg_Addr))
#define SECURE_DEBUG(reg_addr, val) REG((reg_addr)) = (val)
#define SECURE_DEBUG_REG            (CONFIG_RIU_BASE_ADDRESS + (0x100518 << 1))
#define ROM_AUTHEN_REG              (CONFIG_RIU_BASE_ADDRESS + (0x0038E0 << 1))

#define ALIGN_8(_x_)     (((_x_) + 7) & ~7)
#define RIU              ((unsigned short volatile *)CONFIG_RIU_BASE_ADDRESS)
#define RIU8             ((unsigned char volatile *)CONFIG_RIU_BASE_ADDRESS)
#define XIU              ((unsigned int volatile *)CONFIG_XIU_BASE_ADDRESS)
#define XIU8             ((unsigned char volatile *)CONFIG_XIU_BASE_ADDRESS)
#define MIU_BASE         (0x40000000)
#define VIR_TO_PHY(x)    ((u32)(x)-MIU_BASE)
#define AESDMA_BASE_ADDR (0x112200)
#define SHARNG_BASE_ADDR (0x112200)
#define RSA_BASE_ADDR    (0x112200)

#define PKA0_AESDMA_BASE (CONFIG_RIU_BASE_ADDRESS + (0x1AC000UL << 1))
#define PKA1_AESDMA_BASE (CONFIG_RIU_BASE_ADDRESS + (0x1AC100UL << 1))
#define PKA2_AESDMA_BASE (CONFIG_RIU_BASE_ADDRESS + (0x1AC200UL << 1))

#define PKA0_AESDMA_REG32_READ(x) (*((volatile unsigned long *)(PKA0_AESDMA_BASE + ((x) << 2))))
#define PKA1_AESDMA_REG32_READ(x) (*((volatile unsigned long *)(PKA1_AESDMA_BASE + ((x) << 2))))
#define PKA2_AESDMA_REG32_READ(x) (*((volatile unsigned long *)(PKA2_AESDMA_BASE + ((x) << 2))))

#define PKA0_AESDMA_REG32_WRITEE(x, value)                                                               \
    {                                                                                                    \
        (*((volatile unsigned long *)(PKA0_AESDMA_BASE + ((x) << 2)))) = (value);                        \
        PKA_AESDMA_DBG("Set 0x%lx Long 0x%lx\n", (PKA0_AESDMA_BASE + (x << 2)), (unsigned long)(value)); \
    }
#define PKA1_AESDMA_REG32_WRITEE(x, value)                                                               \
    {                                                                                                    \
        (*((volatile unsigned long *)(PKA1_AESDMA_BASE + ((x) << 2)))) = (value);                        \
        PKA_AESDMA_DBG("Set 0x%lx Long 0x%lx\n", (PKA1_AESDMA_BASE + (x << 2)), (unsigned long)(value)); \
    }
#define PKA2_AESDMA_REG32_WRITEE(x, value)                                                               \
    {                                                                                                    \
        (*((volatile unsigned long *)(PKA2_AESDMA_BASE + ((x) << 2)))) = (value);                        \
        PKA_AESDMA_DBG("Set 0x%lx Long 0x%lx\n", (PKA2_AESDMA_BASE + (x << 2)), (unsigned long)(value)); \
    }

#define PKA_BANK_MAIN_CTL     0x00
#define PKA_BANK_ENTRY_PIONT  0x01
#define PKA_BANK_ISR_STATUS   0x02
#define PKA_BANK_BUILD_CONFIG 0x03
#define PKA_BANK_STACK_PIONT  0x04
#define PKA_BANK_INSTR_SINGE  0x05
#define PKA_BANK_CONFIG       0x07
#define PKA_BANK_STAT         0x08
#define PKA_BANK_FLAGS        0x09
#define PKA_BANK_IRQ_EN       0x10
#define PKA_BANK_PROBABILITY  0x11
#define PKA_BANK_FROB         0x12

#define PKA2_RSA_SELECT_CTRL     0x00UL // control register = 0x000~0x05c
#define PKA2_RSA_SELECT_A_RAM    0x01UL // A_ram = 0x400~0x7ff
#define PKA2_RSA_SELECT_B_RAM    0x02UL // B_ram = 0x800~0xbff
#define PKA2_RSA_SELECT_C_RAM    0x03UL // C_ram = 0xc00~0xfff
#define PKA2_RSA_SELECT_D0D1_RAM 0x04UL // D0、D1 = 0x1000~0x13ff
#define PKA2_RSA_SELECT_D2D3_RAM 0x05UL // D2、D3 = 0x1400~0x17ff

#define PKA_RSA_OPERAND_4096 0x600UL // Start CLUE operation for 4096-bits
#define PKA_RSA_OPERAND_2048 0x500UL // Start CLUE operation for 2048-bits
#define PKA_RSA_OPERAND_1024 0x400UL // Start CLUE operation for 1024-bits
#define PKA_RSA_OPERAND_512  0x300UL // Start CLUE operation for 512-bits
#define PKA_RSA_OPERAND_NULL 0x000UL // Error Operation

#ifndef ELP_CLUE_C_ENTRY_POINT_FILE
#define ELP_CLUE_C_ENTRY_POINT_FILE

#define ELP_CLUE_ENTRY_BIT_SERIAL_MOD       0x14
#define ELP_CLUE_ENTRY_BIT_SERIAL_MOD_DP    0x13
#define ELP_CLUE_ENTRY_C25519_PMULT         0x2e
#define ELP_CLUE_ENTRY_CALC_MP              0x10
#define ELP_CLUE_ENTRY_CALC_R_INV           0x11
#define ELP_CLUE_ENTRY_CALC_R_SQR           0x12
#define ELP_CLUE_ENTRY_CED25519_MODEXP      0x30
#define ELP_CLUE_ENTRY_CED25519_MODINV      0x34
#define ELP_CLUE_ENTRY_CED25519_MODMULT     0x31
#define ELP_CLUE_ENTRY_CED25519_MODSQR      0x32
#define ELP_CLUE_ENTRY_CRT                  0x18
#define ELP_CLUE_ENTRY_CRT_KEY_SETUP        0x17
#define ELP_CLUE_ENTRY_ED25519_OBP_MONTMULT 0x33
#define ELP_CLUE_ENTRY_ED25519_PADD         0x2c
#define ELP_CLUE_ENTRY_ED25519_PDBL         0x2d
#define ELP_CLUE_ENTRY_ED25519_PMULT        0x2b
#define ELP_CLUE_ENTRY_ED25519_PVER         0x2f
#define ELP_CLUE_ENTRY_ED25519_SHAMIR       0x36
#define ELP_CLUE_ENTRY_ED25519_XRECOVER     0x35
#define ELP_CLUE_ENTRY_IS_A_M3              0x22
#define ELP_CLUE_ENTRY_IS_P_EQUAL_Q         0x20
#define ELP_CLUE_ENTRY_IS_P_REFLECT_Q       0x21
#define ELP_CLUE_ENTRY_MODADD               0xb
#define ELP_CLUE_ENTRY_MODDIV               0xd
#define ELP_CLUE_ENTRY_MODEXP               0x16
#define ELP_CLUE_ENTRY_MODINV               0xe
#define ELP_CLUE_ENTRY_MODMULT              0xa
#define ELP_CLUE_ENTRY_MODMULT_521          0x29
#define ELP_CLUE_ENTRY_MODSUB               0xc
#define ELP_CLUE_ENTRY_MULT                 0x15
#define ELP_CLUE_ENTRY_M_521_MONTMULT       0x28
#define ELP_CLUE_ENTRY_PADD                 0x1c
#define ELP_CLUE_ENTRY_PADD_521             0x26
#define ELP_CLUE_ENTRY_PADD_STD_PRJ         0x1d
#define ELP_CLUE_ENTRY_PDBL                 0x1a
#define ELP_CLUE_ENTRY_PDBL_521             0x25
#define ELP_CLUE_ENTRY_PDBL_STD_PRJ         0x1b
#define ELP_CLUE_ENTRY_PMULT                0x19
#define ELP_CLUE_ENTRY_PMULT_521            0x24
#define ELP_CLUE_ENTRY_PVER                 0x1e
#define ELP_CLUE_ENTRY_PVER_521             0x27
#define ELP_CLUE_ENTRY_REDUCE               0xf
#define ELP_CLUE_ENTRY_SHAMIR               0x23
#define ELP_CLUE_ENTRY_SHAMIR_521           0x2a
#define ELP_CLUE_ENTRY_STD_PRJ_TO_AFFINE    0x1f

#endif // ELP_CLUE_C_ENTRY_POINT_FILE

#define SHARNG_CTRL_SHA_SEL_SHA256    0x0200
#define SHARNG_CTRL_SHA_CLR           0x0040
#define SHARNG_CTRL_SHA_MSG_BLOCK_NUM 0x0001
#define SHARNG_CTRL_SHA_FIRE_ONCE     0x0001
#define SHARNG_CTRL_SHA_READY         0x0001
#define SHARNG_CTRL_SHA_RST           0x0080
#define SHARNG_CTRL_SHA_BUSY          0x0002
#define SHARNG_CTRL_WORK_MODE_MANUAL  0x4000
#define SHARNG_CTRL_SHA_INIT_HASH     0x2000

#define DMA_SECURE_CTRL_AES_SECURE_PROTECT 0x0001
#define DMA_SECURE_CTRL_AES_SECRET_KEY1    0x0020
#define DMA_SECURE_CTRL_AES_SECRET_KEY2    0x0040
#define DMA_SECURE_CTRL_WB2DMA_R_EN        0x0200
#define DMA_SECURE_CTRL_WB2DMA_W_EN        0x0100

#define AESDMA_CTRL_XIU_SEL_CA9  0x1000
#define AESDMA_CTRL_MIU_SEL_12M  0x8000
#define AESDMA_CTRL_SHA_FROM_IN  0x0100
#define AESDMA_CTRL_SHA_FROM_OUT 0x0200
#define AESDMA_CTRL_DMA_DONE     0x0001

#define AESDMA_CTRL_FILE_ST        0x0001
#define AESDMA_CTRL_FOUT_EN        0x0100
#define AESDMA_CTRL_CBC_MODE       0x2000
#define AESDMA_CTRL_CIPHER_DECRYPT 0x0200
#define AESDMA_CTRL_AES_EN         0x0100
#define AESDMA_CTRL_DES_EN         0x0004
#define AESDMA_CTRL_TDES_EN        0x0008
#define AESDMA_CTRL_CHAINMODE_ECB  0x0000
#define AESDMA_CTRL_CHAINMODE_CTR  (0x0001 << 12)
#define AESDMA_CTRL_CHAINMODE_CBC  (0x0001 << 13)
#define AESDMA_CTRL_CHAINMODE_CLEAR \
    (~(AESDMA_CTRL_CHAINMODE_ECB | AESDMA_CTRL_CHAINMODE_CTR | AESDMA_CTRL_CHAINMODE_CBC))
#define AESDMA_CTRL_SW_RST (0x0001 << 7)
//#define AESDMA_CTRL_USE_SECRET_KEY          0x1000
#define AESDMA_USE_SECRET_KEY_MASK (0x0003 << 5)
#define AESDMA_USE_CIPHER_KEY      0x0000
#define AESDMA_USE_EFUSE_KEY       (0x0001 << 5)
#define AESDMA_USE_HW_KEY          (0x0002 << 5)

#define AESDMA_OTP_KEY_MAX            (8)
#define AESDMA_USE_OTP_KEY(id)        (id << 5)
#define AESDMA_KEY_OTP_EFUSE_KEY8     8
#define AESDMA_KEY_OTP_EFUSE_USE_KEY8 (0x0001 << 10)

#define XIU_STATUS_W_RDY 0x0001
#define XIU_STATUS_W_ERR 0x0008
#define XIU_STATUS_W_LEN 0x00F0
#define XIU_STATUS_R_RDY 0x0100
#define XIU_STATUS_R_ERR 0x0800
#define XIU_STATUS_R_LEN 0xF000

void HAL_AESDMA_SetXIULength(u64 u32Size);
void HAL_AESDMA_UseHwKey(u16 u32KeySel);
bool HAL_AESDMA_CheckAesKey(u16 u32KeySel);
void HAL_AESDMA_UseEfuseKey(void);
void HAL_AESDMA_UseCipherKey(void);
void HAL_AESDMA_CipherEncrypt(void);
void HAL_AESDMA_CipherDecrypt(void);
void HAL_AESDMA_Enable(u16 engine);
void HAL_AESDMA_Disable(void);
void HAL_AESDMA_FileOutEnable(u8 u8FileOutEnable);
void HAL_AESDMA_SetFileinAddr(u64 u32addr);
void HAL_AESDMA_SetFileoutAddr(u64 u32addr, u32 u32Size);
void HAL_AESDMA_SetCipherKey(u16 *pu16Key);
void HAL_AESDMA_SetCipherKey2(u16 *pu16Key, u32 len);
void HAL_AESDMA_SetIV(u16 *pu16IV);
void HAL_AESDMA_SetChainModeECB(void);
void HAL_AESDMA_SetChainModeCTR(void);
void HAL_AESDMA_SetChainModeCBC(void);
void HAL_AESDMA_Reset(void);
void HAL_AESDMA_Start(u8 u8AESDMAStart);
u16  HAL_AESDMA_GetStatus(void);
void HAL_AESDMA_INTMASK(void);
void HAL_AESDMA_INTDISABLE(void);

void HAL_RSA_Reset(u8 u8lentgh, u32 operation);
void HAL_RSA_LoadSignInverse_start(void);
void HAL_RSA_LoadSignInverse(u32 *ptr_Sign, u32 Signlentgh, u32 position);
void HAL_RSA_LoadKeyNInverse(u32 *ptr_N, u8 u8Nlentgh);
void HAL_RSA_LoadKeyE(u32 *ptr_E, u8 u8Elentgh, u32 operation);
void HAL_RSA_LoadKeyEInverse(u32 *ptr_E, u8 u8Elentgh, u32 operation);
void HAL_RSA_FileOut_Start(void);
u32  HAL_RSA_FileOut(u32 i);
u32  HAL_RSA_WaitDone(u32 reason_way);
void HAL_RSA_Init_Status(u32 piont, u32 operation);

void HAL_SHA_Reset(void);
void HAL_SHA_SetAddress(u64 u32Address);
void HAL_SHA_SetLength(u32 u32Size);
void HAL_SHA_SelMode(u8 u8sha256);
void HAL_SHA_ManualMode(u8 bManualMode);
u16  HAL_SHA_GetStatus(void);
void HAL_SHA_Clear(void);
void HAL_SHA_Start(void);
void HAL_SHA_Out(u32 *u32Buf);
void HAL_SHA_ReadOut(u32 *u32Buf);
void HAL_SHA_SetInitHashMode(u8 uMode);
void HAL_SHA_Write_InitValue(u32 *u32Buf);
void HAL_SHA_Write_InitValue_BE(u32 *u32Buf);
u32  HAL_SHA_ReadWordCnt(void);
void HAL_SHA_WriteWordCnt(u32 u32Val);
void HAL_MCM(u32 u32Val);
u16  HAL_RNG_Read(void);

#endif
