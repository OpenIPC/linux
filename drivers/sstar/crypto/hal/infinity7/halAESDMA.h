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

#define REG(Reg_Addr)               (*(volatile unsigned short *)(Reg_Addr))
#define SECURE_DEBUG(reg_addr, val) REG((reg_addr)) = (val)
#define SECURE_DEBUG_REG            (CONFIG_RIU_BASE_ADDRESS + (0x100518 << 1))
#define ROM_AUTHEN_REG              (CONFIG_RIU_BASE_ADDRESS + (0x0038E0 << 1))

#define ALIGN_8(_x_)     (((_x_) + 7) & ~7)
#define RIU              ((u16 volatile *)CONFIG_RIU_BASE_ADDRESS)
#define RIU8             ((unsigned char volatile *)CONFIG_RIU_BASE_ADDRESS)
#define XIU              ((unsigned int volatile *)CONFIG_XIU_BASE_ADDRESS)
#define XIU8             ((unsigned char volatile *)CONFIG_XIU_BASE_ADDRESS)
#define MIU_BASE         (0x40000000)
#define VIR_TO_PHY(x)    ((u32)(x)-MIU_BASE)
#define AESDMA_BASE_ADDR (0x112200)
#define SHARNG_BASE_ADDR (0x112200)
#define RSA_BASE_ADDR    (0x112200)
//#define DMA_SECURE_BASE_ADDR (0x113D00)
//#define MAILBOX_BASE_ADDR    (0x103380)
//#define POR_STATUS_BASE_ADDR (0x100500)

#define RSA_SIGNATURE_LEN (256)
//#define SHA256_DIGEST_SIZE                   (32)

#define RSA_A_BASE_ADDR (0x80)
#define RSA_E_BASE_ADDR (0x00)
#define RSA_N_BASE_ADDR (0x40)
#define RSA_Z_BASE_ADDR (0xC0)

#define SECURE_RSA_OUT_DRAM_ADDRESS (0x20109000)
#define SECURE_SHA_OUT_DRAM_ADDRESS (0x20109100)
#define SECURE_RSA_OUT_SRAM_ADDRESS (0x1FC03600)
#define SECURE_SHA_OUT_SRAM_ADDRESS (0x1FC03680)
#define SECURE_WB_FIFO_OUT_ADDRESS  (0x1FC01C00)

#define SHARNG_CTRL_SHA_SEL_SHA256    0x0200
#define SHARNG_CTRL_SHA_CLR           0x0040
#define SHARNG_CTRL_SHA_MSG_BLOCK_NUM 0x0001
#define SHARNG_CTRL_SHA_FIRE_ONCE     0x0001
#define SHARNG_CTRL_SHA_READY         0x0001
#define SHARNG_CTRL_SHA_RST           0x0080
#define SHARNG_CTRL_SHA_BUSY          0x0002
#define SHARNG_CTRL_WORK_MODE_MANUAL  0x4000
#define SHARNG_CTRL_SHA_INIT_HASH     0x2000

#define RSA_INT_CLR                      0x0002
#define RSA_CTRL_RSA_RST                 0x0001
#define RSA_IND32_START                  0x0001
#define RSA_IND32_CTRL_DIRECTION_WRITE   0x0002
#define RSA_IND32_CTRL_ADDR_AUTO_INC     0x0004
#define RSA_IND32_CTRL_ACCESS_AUTO_START 0x0008
#define RSA_EXP_START                    0x0001
#define RSA_STATUS_RSA_BUSY              0x0001
#define RSA_STATUS_RSA_DONE              0x0002
#define RSA_SEL_HW_KEY                   0x0002
#define RSA_SEL_PUBLIC_KEY               0x0004

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

void HAL_AESDMA_DisableXIUSelectCA9(void);
void HAL_AESDMA_WB2DMADisable(void);
void HAL_AESDMA_ShaFromOutput(void);
void HAL_AESDMA_ShaFromInput(void);
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

void HAL_RSA_ClearInt(void);
void HAL_RSA_Reset(void);
void HAL_RSA_Ind32Ctrl(u8 u8dirction);

void HAL_RSA_LoadSignInverse_start(void);
void HAL_RSA_LoadSignInverse_end(void);

void HAL_RSA_LoadSignInverse(u32 *ptr_Sign, u8 u8Signlentgh);
// void HAL_RSA_LoadSignInverse_2byte(u16 *ptr_Sign);
void HAL_RSA_LoadKeyE(u32 *ptr_E, u8 u8Elentgh);
void HAL_RSA_LoadKeyN(u32 *ptr_N, u8 u8Nlentgh);
void HAL_RSA_LoadKeyNInverse(u32 *ptr_N, u8 u8Nlentgh);
void HAL_RSA_LoadKeyEInverse(u32 *ptr_A, u8 u8Alentgh);

void HAL_RSA_SetKeyLength(u16 u16keylen);
void HAL_RSA_SetKeyType(u8 u8hwkey, u8 u8pubkey);
void HAL_RSA_ExponetialStart(void);
u16  HAL_RSA_GetStatus(void);
void HAL_RSA_FileOutStart(void);
void HAL_RSA_FileOutEnd(void);
void HAL_RSA_SetFileOutAddr(u32 u32offset);
u32  HAL_RSA_FileOut(void);
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
