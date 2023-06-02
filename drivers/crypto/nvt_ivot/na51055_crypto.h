#ifndef _NA51055_CRYPTO_H_
#define _NA51055_CRYPTO_H_

typedef enum {
    NA51055_CRYPTO_MCLK_240MHZ = 0,
    NA51055_CRYPTO_MCLK_320MHZ,
    NA51055_CRYPTO_MCLK_RSVD,
    NA51055_CRYPTO_MCLK_PLL9,
    NA51055_CRYPTO_MCLK_MAX
} NA51055_CRYPTO_MCLK_T;

typedef enum {
    NA51055_CRYPTO_MODE_DES = 0,            ///< Block Size => 64  bits, Key Size => 64  bits, Single Block Cipher
    NA51055_CRYPTO_MODE_3DES,               ///< Block Size => 64  bits, Key Size => 192 bits, Single Block Cipher
    NA51055_CRYPTO_MODE_AES_128,            ///< Block Size => 128 bits, Key Size => 128 bits, Single Block Cipher
    NA51055_CRYPTO_MODE_AES_256,            ///< Block Size => 128 bits, Key Size => 256 bits, Single Block Cipher
    NA51055_CRYPTO_MODE_MAX
} NA51055_CRYPTO_MODE_T;

typedef enum {
    NA51055_CRYPTO_OPMODE_ECB = 0,
    NA51055_CRYPTO_OPMODE_CBC,
    NA51055_CRYPTO_OPMODE_CFB,
    NA51055_CRYPTO_OPMODE_OFB,
    NA51055_CRYPTO_OPMODE_CTR,
    NA51055_CRYPTO_OPMODE_GCM,
    NA51055_CRYPTO_OPMODE_MAX
} NA51055_CRYPTO_OPMODE_T;

typedef enum {
    NA51055_CRYPTO_TYPE_ENCRYPT = 0,
    NA51055_CRYPTO_TYPE_DECRYPT,
    NA51055_CRYPTO_TYPE_MAX
} NA51055_CRYPTO_TYPE_T;

typedef enum {
	NA51055_CRYPTO_CCM_TYPE_NONE = 0,
    NA51055_CRYPTO_CCM_TYPE_ENCRYPT,
    NA51055_CRYPTO_CCM_TYPE_DECRYPT,
    NA51055_CRYPTO_CCM_TYPE_MAX
} NA51055_CRYPTO_CCM_TYPE_T;

typedef enum {
	NA51055_CRYPTO_KEY_SRC_DESC0 = 0,
    NA51055_CRYPTO_KEY_SRC_MANAGAMENT,
    NA51055_CRYPTO_KEY_SRC_MAX
} NA51055_CRYPTO_KEY_SRC_T;

typedef enum {
    NA51055_CRYPTO_DMA_CH_0 = 0,
#ifdef NA51055_CRYPTO_MULTI_DMA_CH_SUPPORT
    NA51055_CRYPTO_DMA_CH_1,
    NA51055_CRYPTO_DMA_CH_2,
    NA51055_CRYPTO_DMA_CH_3,
#endif
    NA51055_CRYPTO_DMA_CH_MAX
} NA51055_CRYPTO_DMA_CH_T;

typedef enum {
    NA51055_CRYPTO_STATE_IDLE = 0,
    NA51055_CRYPTO_STATE_START,
    NA51055_CRYPTO_STATE_BUSY,
    NA51055_CRYPTO_STATE_DONE,
} NA51055_CRYPTO_STATE_T;

#define NA51055_CRYPTO_MAX_IV_SIZE            16             ///< 16  Bytes, 128 bits
#define NA51055_CRYPTO_MAX_KEY_SIZE           32             ///< 32  Bytes, 256 bits
#define NA51055_CRYPTO_MAX_DMA_BLOCK_NUM      32
#define NA51055_CRYPTO_DMA_ALIGN_SIZE         4              ///< dma output length is word alignment

#define NA51055_CRYPTO_CFG_REG                0x00           ///< crypto configuration
#define NA51055_CRYPTO_CTRL_REG               0x04           ///< crypto control
#define NA51055_CRYPTO_INT_ENB_REG            0x08           ///< interrupt enable
#define NA51055_CRYPTO_INT_STS_REG            0x0C           ///< interrupt status
#define NA51055_CRYPTO_KEY0_REG               0x10           ///< key 0   ~ 31  bit
#define NA51055_CRYPTO_KEY1_REG               0x14           ///< key 32  ~ 63  bit
#define NA51055_CRYPTO_KEY2_REG               0x18           ///< key 64  ~ 95  bit
#define NA51055_CRYPTO_KEY3_REG               0x1C           ///< key 96  ~ 127 bit
#define NA51055_CRYPTO_KEY4_REG               0x20           ///< key 128 ~ 159 bit
#define NA51055_CRYPTO_KEY5_REG               0x24           ///< key 160 ~ 191 bit
#define NA51055_CRYPTO_KEY6_REG               0x28           ///< key 192 ~ 223 bit
#define NA51055_CRYPTO_KEY7_REG               0x2C           ///< key 224 ~ 255 bit
#define NA51055_CRYPTO_IN0_REG                0x30           ///< input  data  0  ~ 31  bit
#define NA51055_CRYPTO_IN1_REG                0x34           ///< input  data 32  ~ 63  bit
#define NA51055_CRYPTO_IN2_REG                0x38           ///< input  data 64  ~ 95  bit
#define NA51055_CRYPTO_IN3_REG                0x3C           ///< input  data 96  ~ 127 bit
#define NA51055_CRYPTO_OUT0_REG               0x40           ///< output data  0  ~ 31  bit
#define NA51055_CRYPTO_OUT1_REG               0x44           ///< output data 32  ~ 63  bit
#define NA51055_CRYPTO_OUT2_REG               0x48           ///< output data 64  ~ 95  bit
#define NA51055_CRYPTO_OUT3_REG               0x4C           ///< output data 96  ~ 127 bit
#define NA51055_CRYPTO_DMA0_ADDR_REG          0x50           ///< DMA channel 0 descriptor starting address, DMA mode only
#define NA51055_CRYPTO_DMA1_ADDR_REG          0x54           ///< DMA channel 1 descriptor starting address, DMA mode only
#define NA51055_CRYPTO_DMA2_ADDR_REG          0x58           ///< DMA channel 2 descriptor starting address, DMA mode only
#define NA51055_CRYPTO_DMA3_ADDR_REG          0x5C           ///< DMA channel 0 descriptor starting address, DMA mode only
#define NA51055_CRYPTO_KEY_READ_REG           0x60           ///< crypto key readable control

#endif  /* _NA51055_CRYPTO_H_ */
