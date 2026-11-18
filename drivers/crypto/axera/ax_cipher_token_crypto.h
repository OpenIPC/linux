/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __AX_CIPHER_TOKEN_CRYPTO_H__
#define __AX_CIPHER_TOKEN_CRYPTO_H__
#define CE_POLICY_SHA1					   0x0000000000000001ULL
#define CE_POLICY_SHA224					 0x0000000000000002ULL
#define CE_POLICY_SHA256					 0x0000000000000004ULL
#define CE_POLICY_SHA384					 0x0000000000000008ULL
#define CE_POLICY_SHA512					 0x0000000000000010ULL
#define CE_POLICY_CMAC					   0x0000000000000020ULL
#define CE_POLICY_POLY1305				   0x0000000000000040ULL

/** Asset policies related to symmetric cipher algorithms */
#define CE_POLICY_ALGO_CIPHER_MASK		   0x0000000000000300ULL
#define CE_POLICY_ALGO_CIPHER_AES			0x0000000000000100ULL
#define CE_POLICY_ALGO_CIPHER_TRIPLE_DES	 0x0000000000000200ULL
#define CE_POLICY_ALGO_CIPHER_CHACHA20	   0x0000000000002000ULL
#define CE_POLICY_ALGO_CIPHER_SM4			0x0000000000004000ULL
#define CE_POLICY_ALGO_CIPHER_ARIA		   0x0000000000008000ULL

/** Asset policies related to symmetric cipher modes */
#define CE_POLICY_MODE1					  0x0000000000010000ULL
#define CE_POLICY_MODE2					  0x0000000000020000ULL
#define CE_POLICY_MODE3					  0x0000000000040000ULL
#define CE_POLICY_MODE4					  0x0000000000080000ULL
#define CE_POLICY_MODE5					  0x0000000000100000ULL
#define CE_POLICY_MODE6					  0x0000000000200000ULL
#define CE_POLICY_MODE7					  0x0000000000400000ULL
#define CE_POLICY_MODE8					  0x0000000000800000ULL
#define CE_POLICY_MODE9					  0x0000000001000000ULL
#define CE_POLICY_MODE10					 0x0000000002000000ULL

/** Asset policies specialized per symmetric cipher algorithm */
#define CE_POLICY_AES_MODE_ECB			   (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE1)
#define CE_POLICY_AES_MODE_CBC			   (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE2)
#define CE_POLICY_AES_MODE_CTR			   (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE4)
#define CE_POLICY_AES_MODE_CTR32			 (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE4)
#define CE_POLICY_AES_MODE_ICM			   (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE5)
#define CE_POLICY_AES_MODE_CCM			   (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE7|CE_POLICY_CMAC)
#define CE_POLICY_AES_MODE_F8				(CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE8)
#define CE_POLICY_AES_MODE_XTS			   (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE9)
#define CE_POLICY_AES_MODE_GCM			   (CE_POLICY_ALGO_CIPHER_AES|CE_POLICY_MODE10)

#define CE_POLICY_3DES_MODE_ECB			  (CE_POLICY_ALGO_CIPHER_TRIPLE_DES|CE_POLICY_MODE1)
#define CE_POLICY_3DES_MODE_CBC			  (CE_POLICY_ALGO_CIPHER_TRIPLE_DES|CE_POLICY_MODE2)

#define CE_POLICY_CHACHA20_ENCRYPT		   (CE_POLICY_ALGO_CIPHER_CHACHA20)
#define CE_POLICY_CHACHA20_AEAD			  (CE_POLICY_ALGO_CIPHER_CHACHA20|CE_POLICY_POLY1305)

#define CE_POLICY_SM4_MODE_ECB			   (CE_POLICY_ALGO_CIPHER_SM4|CE_POLICY_MODE1)
#define CE_POLICY_SM4_MODE_CBC			   (CE_POLICY_ALGO_CIPHER_SM4|CE_POLICY_MODE2)
#define CE_POLICY_SM4_MODE_CTR			   (CE_POLICY_ALGO_CIPHER_SM4|CE_POLICY_MODE4)

#define CE_POLICY_ARIA_MODE_ECB			  (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE1)
#define CE_POLICY_ARIA_MODE_CBC			  (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE2)
#define CE_POLICY_ARIA_MODE_CTR			  (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE4)
#define CE_POLICY_ARIA_MODE_CTR32			(CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE4)
#define CE_POLICY_ARIA_MODE_ICM			  (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE5)
#define CE_POLICY_ARIA_MODE_CCM			  (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE7|CE_POLICY_CMAC)
#define CE_POLICY_ARIA_MODE_GCM			  (CE_POLICY_ALGO_CIPHER_ARIA|CE_POLICY_MODE10)

/** Asset policies related to algorithm/cipher/MAC operations */
#define CE_POLICY_MAC_GENERATE			   0x0000000004000000ULL
#define CE_POLICY_MAC_VERIFY				 0x0000000008000000ULL
#define CE_POLICY_ENCRYPT					0x0000000010000000ULL
#define CE_POLICY_DECRYPT					0x0000000020000000ULL

/** Asset policies related to temporary values
 *  Note that the CE_POLICY_TEMP_MAC should be used for intermediate
 *  hash digest as well. */
#define CE_POLICY_TEMP_IV					0x0001000000000000ULL
#define CE_POLICY_TEMP_COUNTER			   0x0002000000000000ULL
#define CE_POLICY_TEMP_MAC				   0x0004000000000000ULL
#define CE_POLICY_TEMP_AUTH_STATE			0x0010000000000000ULL

/** Asset policy related to monotonic counter */
#define CE_POLICY_MONOTONIC				  0x0000000100000000ULL

/** Asset policies related to key derive functionality */
#define CE_POLICY_TRUSTED_ROOT_KEY		   0x0000000200000000ULL
#define CE_POLICY_TRUSTED_KEY_DERIVE		 0x0000000400000000ULL
#define CE_POLICY_KEY_DERIVE				 0x0000000800000000ULL

/** Asset policies related to AES key wrap functionality\n
 *  Note: Must be combined with operations bits */
#define CE_POLICY_TRUSTED_WRAP			   0x0000001000000000ULL
#define CE_POLICY_AES_WRAP				   0x0000002000000000ULL

/** Asset policies related to PK operations */
#define CE_POLICY_PUBLIC_KEY				 0x0000000080000000ULL
#define CE_POLICY_PK_RSA_OAEP_WRAP		   0x0000004000000000ULL
#define CE_POLICY_PK_RSA_PKCS1_WRAP		  0x0000010000000000ULL
#define CE_POLICY_PK_RSA_PKCS1_SIGN		  0x0000020000000000ULL
#define CE_POLICY_PK_RSA_PSS_SIGN			0x0000040000000000ULL
#define CE_POLICY_PK_DSA_SIGN				0x0000080000000000ULL
#define CE_POLICY_PK_ECC_ECDSA_SIGN		  0x0000100000000000ULL
#define CE_POLICY_PK_DH_KEY				  0x0000200000000000ULL
#define CE_POLICY_PK_ECDH_KEY				0x0000400000000000ULL
#define CE_POLICY_PUBLIC_KEY_PARAM		   0x0000800000000000ULL

#define CE_POLICY_PK_ECC_ELGAMAL_ENC		 (CE_POLICY_PK_ECC_ECDSA_SIGN|CE_POLICY_PK_ECDH_KEY)

/** Asset policies related to Authentication */
#define CE_POLICY_EMMC_AUTH_KEY			  0x0400000000000000ULL
#define CE_POLICY_AUTH_KEY				   0x8000000000000000ULL

/** Asset policies related to the domain */
#define CE_POLICY_SOURCE_NON_SECURE		  0x0100000000000000ULL
#define CE_POLICY_CROSS_DOMAIN			   0x0200000000000000ULL

/** Asset policies related to general purpose data that can or must be used
 *  in an operation */
#define CE_POLICY_PRIVATE_DATA			   0x0800000000000000ULL
#define CE_POLICY_PUBLIC_DATA				0x1000000000000000ULL

/** Asset policies related to export functionality */
#define CE_POLICY_EXPORT					 0x2000000000000000ULL
#define CE_POLICY_TRUSTED_EXPORT			 0x4000000000000000ULL

#define CETOKEN_PKASSET_RSA_PKCS1V1_5_SIGN 8
#define CETOKEN_PKASSET_RSA_PKCS1V1_5_VERIFY 9
#define CETOKEN_PKASSET_RSA_PSS_SIGN 0xc
#define CETOKEN_PKASSET_RSA_PSS_VERIFY 0xd

typedef enum
{
	EIP130TOKEN_CRYPTO_ALGO_AES	  = 0,
	EIP130TOKEN_CRYPTO_ALGO_DES	  = 1,
	EIP130TOKEN_CRYPTO_ALGO_3DES	 = 2,
	EIP130TOKEN_CRYPTO_ALGO_CHACHA20 = 7,
	EIP130TOKEN_CRYPTO_ALGO_SM4	  = 8,
	EIP130TOKEN_CRYPTO_ALGO_ARIA	 = 9,
}EIP130_TOKEN_CRYPTO_ALGO_E;
typedef enum
{
	TOKEN_MODE_CIPHER_ECB = 0,
	TOKEN_MODE_CIPHER_CBC,
	TOKEN_MODE_CIPHER_CTR,
	TOKEN_MODE_CIPHER_ICM,
	TOKEN_MODE_CIPHER_F8,
	TOKEN_MODE_CIPHER_CCM,
	TOKEN_MODE_CIPHER_XTS,
	TOKEN_MODE_CIPHER_GCM,
	TOKEN_MODE_CIPHER_CHACHA20_ENC = 0,
	TOKEN_MODE_CIPHER_CHACHA20_AEAD,
} AX_CIPHER_TokenModeCipher;
enum
{
	EIP130TOKEN_HASH_ALGORITHM_SHA1 = 1,
	EIP130TOKEN_HASH_ALGORITHM_SHA224,
	EIP130TOKEN_HASH_ALGORITHM_SHA256,
	EIP130TOKEN_HASH_ALGORITHM_SHA384,
	EIP130TOKEN_HASH_ALGORITHM_SHA512,
};
enum
{
	EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA1 = 1,
	EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA224,
	EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA256,
	EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA384,
	EIP130TOKEN_MAC_ALGORITHM_HMAC_SHA512,
};
enum
{
	EIP130TOKEN_PK_CMD_NUMLOAD = 0x01,
	EIP130TOKEN_PK_CMD_NUMSETN = 0x03,
	EIP130TOKEN_PK_CMD_MODEXPE = 0x04,
	EIP130TOKEN_PK_CMD_MODEXPD,
	EIP130TOKEN_PK_CMD_MODEXPCRT,
	EIP130TOKEN_PK_CMD_ECMONTMUL = 0x0A,
	EIP130TOKEN_PK_CMD_ECCMUL,
	EIP130TOKEN_PK_CMD_ECCADD,
	EIP130TOKEN_PK_CMD_DSA_SIGN,
	EIP130TOKEN_PK_CMD_DSA_VERIFY,
	EIP130TOKEN_PK_CMD_ECDSA_SIGN,
	EIP130TOKEN_PK_CMD_ECDSA_VERIFY,
};
typedef enum
{
	VEXTOKEN_PK_OP_NUMLOAD = 0x01,
	VEXTOKEN_PK_OP_NUMSETN = 0x03,
	VEXTOKEN_PK_OP_MODEXPE = 0x04,
	VEXTOKEN_PK_OP_MODEXPD,
	VEXTOKEN_PK_OP_MODEXPCRT,
	VEXTOKEN_PK_OP_ECMONTMUL = 0x0A,
	VEXTOKEN_PK_OP_ECCMUL,
	VEXTOKEN_PK_OP_ECCADD,
	VEXTOKEN_PK_OP_DSASIGN,
	VEXTOKEN_PK_OP_DSAVERIFY,
	VEXTOKEN_PK_OP_ECDSASIGN,
	VEXTOKEN_PK_OP_ECDSAVERIFY,
} VexTokenPkOperation_t;
static void Eip130Token_Command_WriteByteArray(
		eip130_token_command_t * const command_token,
		unsigned int StartWord,
		const uint8_t * Data,
		const unsigned int DataLenInBytes)
{
	const uint8_t * const Stop = Data + DataLenInBytes;

	if (command_token == 0 || Data == 0)
	{
		return;
	}
	while (Data < Stop)
	{
		uint32_t W;
		if (StartWord >= EIP130TOKEN_RESULT_WORDS)
		{
			return;
		}
		// LSB-first
		W = (uint32_t)(*Data++);
		if (Data < Stop)
		{
			W |= (uint32_t)((*Data++) << 8);
			if (Data < Stop)
			{
				W |= (uint32_t)((*Data++) << 16);
				if (Data < Stop)
				{
					W |= (uint32_t)((*Data++) << 24);
				}
			}
		}
		// Write word
		command_token->W[StartWord++] = W;
	}
}
static void
Eip130Token_Result_ReadByteArray(
		const eip130_token_result_t * const ResultToken_p,
		unsigned int StartWord,
		unsigned int DestLenOutBytes,
		uint8_t * Dest)
{
	uint8_t * const Stop = Dest + DestLenOutBytes;

	if (ResultToken_p == 0 || Dest == 0)
	{
		return;
	}
	while (Dest < Stop)
	{
		uint32_t W;

		if (StartWord >= EIP130TOKEN_RESULT_WORDS)
		{
			return;
		}
		// Read word
		W = ResultToken_p->W[StartWord++];

		// LSB-first
		*Dest++ = (uint8_t)W;
		if (Dest < Stop)
		{
			W >>= 8;
			*Dest++ = (uint8_t)W;
			if (Dest)
			{
				W >>= 8;
				*Dest++ = (uint8_t)W;
				if (Dest < Stop)
				{
					W >>= 8;
					*Dest++ = (uint8_t)W;
				}
			}
		}
	}
}

static inline void Eip130Token_Command_Crypto_Operation(
		eip130_token_command_t * const command_token,
		const uint8_t algorithm,
		const uint8_t Mode,
		const ax_bool_ce fEncrypt,
		const uint32_t DataLengthInBytes)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_ENCRYPTION << 24);
	command_token->W[2] = DataLengthInBytes;

	// algorithm, Mode and direction
	command_token->W[11] = (MASK_4_BITS & algorithm) + ((MASK_4_BITS & Mode) << 4);
	if (fEncrypt)
	{
		command_token->W[11] |= BIT_15;
	}
}
static inline void Eip130Token_Command_Crypto_SetDataAddresses(
		eip130_token_command_t * const command_token,
		const uint64_t InputDataAddress,
		const uint32_t InputDataLengthInBytes,
		const uint64_t OutputDataAddress,
		const uint32_t OutputDataLengthInBytes)
{
	command_token->W[3] = (uint32_t)(InputDataAddress);
	command_token->W[4] = (uint32_t)(InputDataAddress >> 32);
	command_token->W[5] = InputDataLengthInBytes;
	command_token->W[6] = (uint32_t)(OutputDataAddress);
	command_token->W[7] = (uint32_t)(OutputDataAddress >> 32);
	command_token->W[8] = OutputDataLengthInBytes;
}
static inline void Eip130Token_Command_Crypto_CopyKey(
		eip130_token_command_t * const command_token,
		const uint8_t * const Key_p,
		const uint32_t KeyLengthInBytes)
{
	Eip130Token_Command_WriteByteArray(command_token, 17, Key_p, KeyLengthInBytes);
}
static inline void Eip130Token_Command_Crypto_SetKeyLength(
		eip130_token_command_t * const command_token,
		const uint32_t KeyLengthInBytes)
{
	uint32_t CodedKeyLen = 0;
	// Coded key length only needed for AES and ARIA
	switch (KeyLengthInBytes)
	{
	case (128 / 8):
		CodedKeyLen = 1;
		break;

	case (192 / 8):
		CodedKeyLen = 2;
		break;

	case (256 / 8):
		CodedKeyLen = 3;
		break;

	default:
		break;
	}
	command_token->W[11] |= (CodedKeyLen << 16);
}
static inline void
Eip130Token_Command_Crypto_CopyIV(
		eip130_token_command_t * const command_token,
		const uint8_t * const IV_p)
{
	Eip130Token_Command_WriteByteArray(command_token, 13, IV_p, 16);
}

static inline void
Eip130Token_Command_Hash(
		eip130_token_command_t * const command_token,
		const uint8_t HashAlgo,
		const ax_bool_ce fInit_with_default,
		const ax_bool_ce fFinalize,
		const uint64_t InputDataAddress,
		const uint32_t InputDataLengthInBytes)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_HASH << 24);
	command_token->W[2] = InputDataLengthInBytes;
	command_token->W[3] = (uint32_t)(InputDataAddress);
	command_token->W[4] = (uint32_t)(InputDataAddress >> 32);
	command_token->W[5] = InputDataLengthInBytes;
	command_token->W[6] = (MASK_4_BITS & HashAlgo);
	if (!fInit_with_default)
	{
		command_token->W[6] |= BIT_4;
	}
	if (!fFinalize)
	{
		command_token->W[6] |= BIT_5;
	}
}

static inline void
Eip130Token_Command_Hash_SetTempDigestASID(eip130_token_command_t * const command_token, const uint32_t asset_id)
{
	command_token->W[7] = asset_id;
}

static inline void
Eip130Token_Command_Hash_SetTotalMessageLength(
		eip130_token_command_t * const command_token,
		const uint64_t TotalMessageLengthInBytes)
{
	command_token->W[24] = (uint32_t)(TotalMessageLengthInBytes);
	command_token->W[25] = (uint32_t)(TotalMessageLengthInBytes >> 32);
}


static inline void
Eip130Token_Command_Hash_CopyDigest(
		eip130_token_command_t * const command_token,
		const uint8_t * const Digest_p,
		const uint32_t DigestLenInBytes)
{
	Eip130Token_Command_WriteByteArray(command_token, 8, Digest_p, DigestLenInBytes);
}


static inline void
Eip130Token_Result_Hash_CopyDigest(
		eip130_token_result_t * const result_token,
		const uint32_t DigestLenInBytes,
		uint8_t * Digest)
{
	Eip130Token_Result_ReadByteArray(result_token, 2,
									 DigestLenInBytes, Digest);
}

static inline void
Eip130Token_Command_Mac(
		eip130_token_command_t * const command_token,
		const uint8_t MacAlgo,
		const ax_bool_ce fInit,
		const ax_bool_ce fFinalize,
		const uint64_t InputDataAddress,
		const uint32_t InputDataLengthInBytes)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_MAC << 24);
	command_token->W[2] = InputDataLengthInBytes;
	command_token->W[3] = (uint32_t)(InputDataAddress);
	command_token->W[4] = (uint32_t)(InputDataAddress >> 32);
	command_token->W[5] = (uint32_t)((InputDataLengthInBytes + 3) & (uint32_t)~3);
	command_token->W[6] = (MASK_4_BITS & MacAlgo);
	if (!fInit)
	{
		command_token->W[6] |= BIT_4;
	}
	if (!fFinalize)
	{
		command_token->W[6] |= BIT_5;
	}
}
static inline void
Eip130Token_Command_Mac_SetTotalMessageLength(
		eip130_token_command_t * const command_token,
		const uint64_t TotalMessageLengthInBytes)
{
	command_token->W[24] = (uint32_t)(TotalMessageLengthInBytes);
	command_token->W[25] = (uint32_t)(TotalMessageLengthInBytes >> 32);
}
static inline void
Eip130Token_Command_Mac_SetASLoadKey(
		eip130_token_command_t * const command_token,
		const uint32_t asset_id)
{
	command_token->W[6] |= BIT_8;
	command_token->W[28] = asset_id;
}
static inline void
Eip130Token_Command_Mac_SetASLoadMAC(
		eip130_token_command_t * const command_token,
		const uint32_t asset_id)
{
	command_token->W[6] |= BIT_9;
	command_token->W[8] = asset_id;
}
static inline void
Eip130Token_Command_Mac_CopyKey(
		eip130_token_command_t * const command_token,
		const uint8_t * const Key_p,
		const uint32_t KeyLengthInBytes)
{
	command_token->W[6] |= ((MASK_8_BITS & KeyLengthInBytes) << 16);
	Eip130Token_Command_WriteByteArray(command_token, 28, Key_p, KeyLengthInBytes);
}
static inline void
Eip130Token_Command_Mac_CopyMAC(
		eip130_token_command_t * const command_token,
		const uint8_t * const MAC_p,
		const uint32_t MACLenInBytes)
{
	Eip130Token_Command_WriteByteArray(command_token, 8,
									   MAC_p, MACLenInBytes);
}
static inline void
Eip130Token_Result_Mac_CopyMAC(
		eip130_token_result_t * const ResultToken_p,
		const uint32_t MACLenInBytes,
		uint8_t * MAC_p)
{
	Eip130Token_Result_ReadByteArray(ResultToken_p, 2, MACLenInBytes, MAC_p);
}
static inline void
Eip130Token_Command_RandomNumber_Generate(
		eip130_token_command_t * const command_token,
		const uint16_t NumberLengthInBytes,
		const uint64_t OutputDataAddress)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_TRNG << 24) |
						   (EIP130TOKEN_SUBCODE_RANDOMNUMBER << 28);
	command_token->W[2] = NumberLengthInBytes;
	command_token->W[3] = (uint32_t)(OutputDataAddress);
	command_token->W[4] = (uint32_t)(OutputDataAddress >> 32);
}
static inline void
Eip130Token_Command_TRNG_Configure(
		eip130_token_command_t * const command_token,
		const uint8_t  auto_seed,
		const uint16_t sample_cycles,
		const uint8_t  sample_div,
		const uint8_t  noise_blocks)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_TRNG << 24) |
						   (EIP130TOKEN_SUBCODE_TRNGCONFIG << 28);
	command_token->W[2] = (uint32_t)((uint32_t)(auto_seed << 8) | BIT_0);
	command_token->W[3] = (uint32_t)((sample_cycles << 16) |
									  ((sample_div & 0x0F) << 8) |
									  noise_blocks);
}
static inline void
Eip130Token_Command_AssetCreate(
		eip130_token_command_t * const command_token,
		const uint64_t Policy,
		const uint32_t LengthInBytes)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
						   (EIP130TOKEN_SUBCODE_ASSETCREATE << 28);
	command_token->W[2] = (uint32_t)(Policy & 0xffffffff);
	command_token->W[3] = (uint32_t)(Policy >> 32);
	command_token->W[4] = (LengthInBytes & MASK_10_BITS) | BIT_28;
	command_token->W[5] = 0;
	command_token->W[6] = 0;
}
static inline void
Eip130Token_Command_AssetDelete(
		eip130_token_command_t * const command_token,
		const uint32_t asset_id)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
						   (EIP130TOKEN_SUBCODE_ASSETDELETE << 28);
	command_token->W[2] = asset_id;
}
static inline void
Eip130Token_Command_AssetLoad_Plaintext(
		eip130_token_command_t * const command_token,
		const uint32_t asset_id)
{
	command_token->W[0] = (EIP130TOKEN_OPCODE_ASSETMANAGEMENT << 24) |
						   (EIP130TOKEN_SUBCODE_ASSETLOAD << 28);
	command_token->W[2] = asset_id;
	command_token->W[3] = BIT_27;	 // Plaintext
	command_token->W[4] = 0;
	command_token->W[5] = 0;
	command_token->W[6] = 0;
	command_token->W[7] = 0;
	command_token->W[8] = 0;
}
static inline void
Eip130Token_Command_Pk_Asset_Command(
		eip130_token_command_t * const command_token,
		const uint8_t Command,
		const uint8_t Nwords,
		const uint8_t Mwords,
		const uint8_t OtherLen,
		const uint32_t KeyAssetId,
		const uint32_t ParamAssetId,
		const uint32_t IOAssetId,
		const uint64_t InputDataAddress,
		const uint16_t InputDataLengthInBytes,
		const uint64_t OutputDataAddress,	   // or Signature address
		const uint16_t OutputDataLengthInBytes) // or Signature length
{
	command_token->W[0]  = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
							(EIP130TOKEN_SUBCODE_PK_WITHASSETS << 28);
	command_token->W[2]  = (uint32_t)(Command | // PK operation to perform
									   (Nwords << 16) |
									   (Mwords << 24));
	command_token->W[3]  = (uint32_t)(OtherLen << 8);
	command_token->W[4]  = KeyAssetId; // asset containing x and y coordinates of pk
	command_token->W[5]  = ParamAssetId; // public key parameters:
										  // p, a, b, n, base x, base y[, h]
	command_token->W[6]  = IOAssetId;
	command_token->W[7]  = ((MASK_12_BITS & OutputDataLengthInBytes) << 16 ) |
							 (MASK_12_BITS & InputDataLengthInBytes);
	command_token->W[8]  = (uint32_t)(InputDataAddress);
	command_token->W[9]  = (uint32_t)(InputDataAddress >> 32);
	command_token->W[10] = (uint32_t)(OutputDataAddress);
	command_token->W[11] = (uint32_t)(OutputDataAddress >> 32);
}
static inline void
Eip130Token_Command_AssetLoad_SetInput(
		eip130_token_command_t * const command_token,
		const uint64_t DataAddress,
		const uint32_t DataLengthInBytes)
{
	command_token->W[3] |= (DataLengthInBytes & MASK_10_BITS);
	command_token->W[4]  = (uint32_t)(DataAddress);
	command_token->W[5]  = (uint32_t)(DataAddress >> 32);
}
static inline void
Eip130Token_Command_Pk_Asset_SetAdditionalLength(
		eip130_token_command_t * const command_token,
		const uint64_t AddLength)
{
	uint32_t offset = ((command_token->W[3] & 0xFF) + 3) & (uint32_t)~3;
	command_token->W[3] &= (uint32_t)~0xFF;
	command_token->W[3] |= (offset + (2 * (uint32_t)sizeof(uint32_t)));
	command_token->W[12 + (offset / sizeof(uint32_t))] = (uint32_t)(AddLength);
	command_token->W[13 + (offset / sizeof(uint32_t))] = (uint32_t)(AddLength >> 32);
}
static inline void
Eip130Token_Command_Pk_Claim(
		eip130_token_command_t * const CommandToken_p,
		const uint8_t Nwords,
		const uint8_t Mwords,
		const uint8_t Mmask)
{
	CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
						   (EIP130TOKEN_SUBCODE_PK_NOASSETS << 28);
	CommandToken_p->W[2] = (uint32_t)(EIP130TOKEN_PK_CMD_NUMSETN |
									  (Mmask << 8) |
									  (Nwords << 16) |
									  (Mwords << 24));
}
static inline void
Eip130Token_Command_SetTokenID(
		eip130_token_command_t * const CommandToken_p,
		const uint16_t TokenIDValue,
		const ax_bool_ce fWriteTokenID)
{
	// Replace TokenID field (word 0, lowest 16 bits) with TokenIDValue aand
	// reset Write Token ID indication
	CommandToken_p->W[0] &= ((MASK_16_BITS << 16) - BIT_18);
	CommandToken_p->W[0] |= TokenIDValue;
	if (fWriteTokenID)
	{
		// Set Write Token ID field (word 0, bit 18)
		CommandToken_p->W[0] |= BIT_18;
	}
}
static inline void
Eip130Token_Command_Pk_NumLoad(
		eip130_token_command_t * const CommandToken_p,
		const uint8_t Index,
		const uint64_t InputDataAddress,
		const uint32_t InputDataLengthInBytes)
{
	CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
						   (EIP130TOKEN_SUBCODE_PK_NOASSETS << 28);
	CommandToken_p->W[2] = EIP130TOKEN_PK_CMD_NUMLOAD |
						  ((MASK_4_BITS & Index) << 24);
	CommandToken_p->W[5] = (MASK_12_BITS & InputDataLengthInBytes);
	CommandToken_p->W[6] = (uint32_t)(InputDataAddress);
	CommandToken_p->W[7] = (uint32_t)(InputDataAddress >> 32);
}
static inline void
Eip130Token_Command_Pk_Operation(
		eip130_token_command_t * const CommandToken_p,
		const uint8_t Command,
		const uint32_t PublicExponent,
		const uint64_t InputDataAddress,
		const uint32_t InputDataLengthInBytes,
		const uint64_t OutputDataAddress,
		const uint32_t OutputDataLengthInBytes)
{
	CommandToken_p->W[0] = (EIP130TOKEN_OPCODE_PUBLIC_KEY << 24) |
						   (EIP130TOKEN_SUBCODE_PK_NOASSETS << 28);
	CommandToken_p->W[2] = (MASK_5_BITS & Command); // PK operation to perform
	CommandToken_p->W[3] = PublicExponent;
	CommandToken_p->W[5] = ((OutputDataLengthInBytes & MASK_12_BITS) << 16) |
						   (InputDataLengthInBytes & MASK_12_BITS);
	CommandToken_p->W[6] = (uint32_t)(InputDataAddress);
	CommandToken_p->W[7] = (uint32_t)(InputDataAddress >> 32);
	CommandToken_p->W[8] = (uint32_t)(OutputDataAddress);
	CommandToken_p->W[9] = (uint32_t)(OutputDataAddress >> 32);
}

#endif