/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __EIP130_DRV_H_
#define __EIP130_DRV_H_

#define EIP130TOKEN_OPCODE_NOP					0U
#define EIP130TOKEN_OPCODE_ENCRYPTION			 1U
#define EIP130TOKEN_OPCODE_HASH				   2U
#define EIP130TOKEN_OPCODE_MAC					3U
#define EIP130TOKEN_OPCODE_TRNG				   4U
#define EIP130TOKEN_OPCODE_SPECIALFUNCTIONS	   5U
#define EIP130TOKEN_OPCODE_AESWRAP				6U
#define EIP130TOKEN_OPCODE_ASSETMANAGEMENT		7U
#define EIP130TOKEN_OPCODE_AUTH_UNLOCK			8U
#define EIP130TOKEN_OPCODE_PUBLIC_KEY			 9U
#define EIP130TOKEN_OPCODE_EMMC				   10U
#define EIP130TOKEN_OPCODE_EXT_SERVICE			11U
#define EIP130TOKEN_OPCODE_RESERVED12			 12U
#define EIP130TOKEN_OPCODE_RESERVED13			 13U
#define EIP130TOKEN_OPCODE_SERVICE				14U
#define EIP130TOKEN_OPCODE_SYSTEM				 15U

// Token sub codes
// TRNG operations
#define EIP130TOKEN_SUBCODE_RANDOMNUMBER		  0U
#define EIP130TOKEN_SUBCODE_TRNGCONFIG			1U
#define EIP130TOKEN_SUBCODE_VERIFYDRBG			2U
#define EIP130TOKEN_SUBCODE_VERIFYNRBG			3U
// Asset Management operations
#define EIP130TOKEN_SUBCODE_ASSETSEARCH		   0U
#define EIP130TOKEN_SUBCODE_ASSETCREATE		   1U
#define EIP130TOKEN_SUBCODE_ASSETLOAD			 2U
#define EIP130TOKEN_SUBCODE_ASSETDELETE		   3U
#define EIP130TOKEN_SUBCODE_PUBLICDATA			4U
#define EIP130TOKEN_SUBCODE_MONOTONICREAD		 5U
#define EIP130TOKEN_SUBCODE_MONOTONICINCR		 6U
#define EIP130TOKEN_SUBCODE_OTPDATAWRITE		  7U
#define EIP130TOKEN_SUBCODE_SECURETIMER		   8U
#define EIP130TOKEN_SUBCODE_PROVISIONRANDOMHUK	9U
// KeyWrap and Encrypt vector operations
#define EIP130TOKEN_SUBCODE_AESKEYWRAP			0U
#define EIP130TOKEN_SUBCODE_ENCRYPTVECTOR		 1U
// Special Functions operations
#define EIP130TOKEN_SUBCODE_SF_MILENAGE		   0U
#define EIP130TOKEN_SUBCODE_SF_BLUETOOTH		  1U
#define EIP130TOKEN_SUBCODE_SF_COVERAGE		   4U
// Authenticated Unlock operations
#define EIP130TOKEN_SUBCODE_AUNLOCKSTART		  0U
#define EIP130TOKEN_SUBCODE_AUNLOCKVERIFY		 1U
#define EIP130TOKEN_SUBCODE_SETSECUREDEBUG		2U
// Public key operations
#define EIP130TOKEN_SUBCODE_PK_NOASSETS		   0U
#define EIP130TOKEN_SUBCODE_PK_WITHASSETS		 1U
// eMMC operations
#define EIP130TOKEN_SUBCODE_EMMC_RDREQ			0U
#define EIP130TOKEN_SUBCODE_EMMC_RDVER			1U
#define EIP130TOKEN_SUBCODE_EMMC_RDWRCNTREQ	   2U
#define EIP130TOKEN_SUBCODE_EMMC_RDWRCNTVER	   3U
#define EIP130TOKEN_SUBCODE_EMMC_WRREQ			4U
#define EIP130TOKEN_SUBCODE_EMMC_WRVER			5U
// Service operations
#define EIP130TOKEN_SUBCODE_REGISTERREAD		  0U
#define EIP130TOKEN_SUBCODE_REGISTERWRITE		 1U
#define EIP130TOKEN_SUBCODE_CLOCKSWITCH		   2U
#define EIP130TOKEN_SUBCODE_ZEROOUTMAILBOX		3U
#define EIP130TOKEN_SUBCODE_SELECTOTPZERO		 4U
#define EIP130TOKEN_SUBCODE_ZEROIZEOTP			5U
// System operations
#define EIP130TOKEN_SUBCODE_SYSTEMINFO			0U
#define EIP130TOKEN_SUBCODE_SELFTEST			  1U
#define EIP130TOKEN_SUBCODE_RESET				 2U
#define EIP130TOKEN_SUBCODE_DEFINEUSERS		   3U
#define EIP130TOKEN_SUBCODE_SLEEP				 4U
#define EIP130TOKEN_SUBCODE_RESUMEFROMSLEEP	   5U
#define EIP130TOKEN_SUBCODE_HIBERNATION		   6U
#define EIP130TOKEN_SUBCODE_RESUMEFROMHIBERNATION 7U
#define EIP130TOKEN_SUBCODE_SETTIME			   8U

// Token/HW/algorithm related limits
#define EIP130TOKEN_DMA_MAXLENGTH		   0x001FFFFF  // 2 MB - 1 bytes
#define EIP130TOKEN_DMA_TOKENID_SIZE		4		   // bytes
#define EIP130TOKEN_DMA_ARC4_STATE_BUF_SIZE 256		 // bytes

// DMA data block must be an integer multiple of a work block size (in bytes)
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_HASH 64
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_AES  16
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_DES  8
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_ARC4 4
#define EIP130TOKEN_DMA_ALGO_BLOCKSIZE_NOP  4

#define MASK_1_BIT	  (BIT_1 - 1)
#define MASK_2_BITS	 (BIT_2 - 1)
#define MASK_3_BITS	 (BIT_3 - 1)
#define MASK_4_BITS	 (BIT_4 - 1)
#define MASK_5_BITS	 (BIT_5 - 1)
#define MASK_6_BITS	 (BIT_6 - 1)
#define MASK_7_BITS	 (BIT_7 - 1)
#define MASK_8_BITS	 (BIT_8 - 1)
#define MASK_9_BITS	 (BIT_9 - 1)
#define MASK_10_BITS	(BIT_10 - 1)
#define MASK_11_BITS	(BIT_11 - 1)
#define MASK_12_BITS	(BIT_12 - 1)
#define MASK_13_BITS	(BIT_13 - 1)
#define MASK_14_BITS	(BIT_14 - 1)
#define MASK_15_BITS	(BIT_15 - 1)
#define MASK_16_BITS	(BIT_16 - 1)
#define MASK_17_BITS	(BIT_17 - 1)
#define MASK_18_BITS	(BIT_18 - 1)
#define MASK_19_BITS	(BIT_19 - 1)
#define MASK_20_BITS	(BIT_20 - 1)
#define MASK_21_BITS	(BIT_21 - 1)
#define MASK_22_BITS	(BIT_22 - 1)
#define MASK_23_BITS	(BIT_23 - 1)
#define MASK_24_BITS	(BIT_24 - 1)
#define MASK_25_BITS	(BIT_25 - 1)
#define MASK_26_BITS	(BIT_26 - 1)
#define MASK_27_BITS	(BIT_27 - 1)
#define MASK_28_BITS	(BIT_28 - 1)
#define MASK_29_BITS	(BIT_29 - 1)
#define MASK_30_BITS	(BIT_30 - 1)
#define MASK_31_BITS	(BIT_31 - 1)

#define BIT_0   0x00000001U
#define BIT_1   0x00000002U
#define BIT_2   0x00000004U
#define BIT_3   0x00000008U
#define BIT_4   0x00000010U
#define BIT_5   0x00000020U
#define BIT_6   0x00000040U
#define BIT_7   0x00000080U
#define BIT_8   0x00000100U
#define BIT_9   0x00000200U
#define BIT_10  0x00000400U
#define BIT_11  0x00000800U
#define BIT_12  0x00001000U
#define BIT_13  0x00002000U
#define BIT_14  0x00004000U
#define BIT_15  0x00008000U
#define BIT_16  0x00010000U
#define BIT_17  0x00020000U
#define BIT_18  0x00040000U
#define BIT_19  0x00080000U
#define BIT_20  0x00100000U
#define BIT_21  0x00200000U
#define BIT_22  0x00400000U
#define BIT_23  0x00800000U
#define BIT_24  0x01000000U
#define BIT_25  0x02000000U
#define BIT_26  0x04000000U
#define BIT_27  0x08000000U
#define BIT_28  0x10000000U
#define BIT_29  0x20000000U
#define BIT_30  0x40000000U
#define BIT_31  0x80000000U
#define EIP130TOKEN_COMMAND_WORDS   64
#define EIP130TOKEN_RESULT_WORDS	64
typedef struct {
	uint32_t W[EIP130TOKEN_COMMAND_WORDS];
} eip130_token_command_t;
typedef struct {
	uint32_t W[EIP130TOKEN_RESULT_WORDS];
} eip130_token_result_t;

int ce_init(void *reg_base);
int ce_deinit(void);
int eip130_wait_for_result_token(int chn);
int eip130_physical_token_exchange(uint32_t *command_token, uint32_t *result_token, uint32_t chn);
int eip130_Interrupt_handle(void);
int ce_resume(void);
#endif