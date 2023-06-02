#ifndef _NA51089_HASH_H_
#define _NA51089_HASH_H_

typedef enum {
    NA51089_HASH_MCLK_240MHZ = 0,
    NA51089_HASH_MCLK_320MHZ,
    NA51089_HASH_MCLK_RSVD,
    NA51089_HASH_MCLK_PLL9,
    NA51089_HASH_MCLK_MAX
} NA51089_HASH_MCLK_T;

typedef enum {
    NA51089_HASH_MODE_SHA1 = 0,                            ///< block size=> 64 bytes, digest size 20 bytes
    NA51089_HASH_MODE_SHA256,                              ///< block size=> 64 bytes, digest size 32 bytes
    NA51089_HASH_MODE_HMAC_SHA1,                           ///< block size=> 64 bytes, digest size 20 bytes
    NA51089_HASH_MODE_HMAC_SHA256,                         ///< block size=> 64 bytes, digest size 32 bytes
    NA51089_HASH_MODE_MAX
} NA51089_HASH_MODE_T;

typedef enum {
    NA51089_HASH_IV_SEL_DEFAULT = 0,                       ///< HASH IV from algorithm default
    NA51089_HASH_IV_SEL_REGISTER,                          ///< HASH IV from register HASH_IV_0 to HASH_IV_7
    NA51089_HASH_IV_MAX
} NA51089_HASH_IV_SEL_T;

typedef enum {
    NA51089_HASH_ACCMODE_PIO = 0,                          ///< HASH access mode PIO
    NA51089_HASH_ACCMODE_DMA,                              ///< HASH access mode DMA
    NA51089_HASH_ACCMODE_MAX
} NA51089_HASH_ACCMODE_T;

typedef enum {
    NA51089_HASH_DMA_PAD_DISABLE = 0,                      ///< HASH padding disable, DMA mode only
    NA51089_HASH_DMA_PAD_ENABLE,                           ///< HASH padding enable,  DMA mode only
    NA51089_HASH_DMA_PAD_MAX
} NA51089_HASH_DMA_PAD_T;

typedef enum {
    NA51089_HASH_BYPASS_NORMAL = 0,                        ///< HASH engine as normal mode
    NA51089_HASH_BYPASS_HWCOPY,                            ///< HASH engine as HwCopy
    NA51089_HASH_BYPASS_MAX
} NA51089_HASH_BYPASS_T;

typedef enum {
    NA51089_HASH_INIT_DISABLE = 0,                         ///< Set engine as normal  state, PIO mode only
    NA51089_HASH_INIT_ENABLE,                              ///< Set engine as initail state, PIO mode only
    NA51089_HASH_INIT_MAX
} NA51089_HASH_INIT_T;

typedef enum {
    NA51089_HASH_IPAD_DISABLE = 0,                         ///< Set HASH engine as normal state
    NA51089_HASH_IPAD_ENABLE,                              ///< Set HASH engine as internal padding state
    NA51089_HASH_IPAD_MAX
} NA51089_HASH_IPAD_T;

typedef enum {
    NA51089_HASH_OPAD_DISABLE = 0,                         ///< Set HASH engine as normal state
    NA51089_HASH_OPAD_ENABLE,                              ///< Set HASH engine as external padding state
    NA51089_HASH_OPAD_MAX
} NA51089_HASH_OPAD_T;

#define NA51089_HASH_MAX_IV_SIZE            32             ///< 32 Bytes, SHA1=> 20 bytes, SHA256=> 32 bytes
#define NA51089_HASH_MAX_DIGEST_SIZE        32             ///< 32 Bytes, SHA1=> 20 bytes, SHA256=> 32 bytes
#define NA51089_HASH_MAX_KEY_SIZE           64             ///< 64 Bytes

#define NA51089_HASH_CFG_REG                0x00           ///< configuration
#define NA51089_HASH_PADLEN_REG             0x04           ///< message length for DMA mode auto padding used
#define NA51089_HASH_INT_ENB_REG            0x08           ///< interrupt enable
#define NA51089_HASH_INT_STS_REG            0x0C           ///< interrupt status
#define NA51089_HASH_KEY0_REG               0x10           ///< key 0   ~ 31  bit
#define NA51089_HASH_KEY1_REG               0x14           ///< key 32  ~ 63  bit
#define NA51089_HASH_KEY2_REG               0x18           ///< key 64  ~ 95  bit
#define NA51089_HASH_KEY3_REG               0x1C           ///< key 96  ~ 127 bit
#define NA51089_HASH_KEY4_REG               0x20           ///< key 128 ~ 159 bit
#define NA51089_HASH_KEY5_REG               0x24           ///< key 160 ~ 191 bit
#define NA51089_HASH_KEY6_REG               0x28           ///< key 192 ~ 223 bit
#define NA51089_HASH_KEY7_REG               0x2C           ///< key 224 ~ 255 bit
#define NA51089_HASH_KEY8_REG               0x30           ///< key 256 ~ 287 bit
#define NA51089_HASH_KEY9_REG               0x34           ///< key 288 ~ 319 bit
#define NA51089_HASH_KEY10_REG              0x38           ///< key 320 ~ 351 bit
#define NA51089_HASH_KEY11_REG              0x3C           ///< key 352 ~ 383 bit
#define NA51089_HASH_KEY12_REG              0x40           ///< key 384 ~ 415 bit
#define NA51089_HASH_KEY13_REG              0x44           ///< key 416 ~ 447 bit
#define NA51089_HASH_KEY14_REG              0x48           ///< key 448 ~ 479 bit
#define NA51089_HASH_KEY15_REG              0x4C           ///< key 480 ~ 511 bit
#define NA51089_HASH_IV0_REG                0x50           ///< initial vector 0   ~ 31  bit
#define NA51089_HASH_IV1_REG                0x54           ///< initial vector 32  ~ 63  bit
#define NA51089_HASH_IV2_REG                0x58           ///< initial vector 64  ~ 95  bit
#define NA51089_HASH_IV3_REG                0x5c           ///< initial vector 96  ~ 127 bit
#define NA51089_HASH_IV4_REG                0x60           ///< initial vector 128 ~ 159 bit
#define NA51089_HASH_IV5_REG                0x64           ///< initial vector 160 ~ 191 bit
#define NA51089_HASH_IV6_REG                0x68           ///< initial vector 192 ~ 223 bit
#define NA51089_HASH_IV7_REG                0x6c           ///< initial vector 224 ~ 255 bit
#define NA51089_HASH_OUT0_REG               0x70           ///< output data  0   ~ 31  bit
#define NA51089_HASH_OUT1_REG               0x74           ///< output data  32  ~ 63  bit
#define NA51089_HASH_OUT2_REG               0x78           ///< output data  64  ~ 95  bit
#define NA51089_HASH_OUT3_REG               0x7c           ///< output data  96  ~ 127 bit
#define NA51089_HASH_OUT4_REG               0x80           ///< output data  128 ~ 159 bit
#define NA51089_HASH_OUT5_REG               0x84           ///< output data  160 ~ 191 bit
#define NA51089_HASH_OUT6_REG               0x88           ///< output data  192 ~ 223 bit
#define NA51089_HASH_OUT7_REG               0x8c           ///< output data  224 ~ 255 bit
#define NA51089_HASH_PIO_IN_REG             0x90           ///< input  data  0   ~ 31  bit, PIO mode
#define NA51089_HASH_DMA_SRC_REG            0x94           ///< source dma address, word alignment
#define NA51089_HASH_DMA_DST_REG            0x98           ///< destination dma address, word alignment
#define NA51089_HASH_DMA_TX_SIZE_REG        0x9c           ///< hash message size, byte unit and alignment
#define NA51089_HASH_KEY_READ_REG           0xA0           ///< hash key readable control

#endif  /* _NA51089_HASH_H_ */
