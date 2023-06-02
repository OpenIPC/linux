#ifndef _NA51055_RSA_H_
#define _NA51055_RSA_H_

typedef enum {
    NA51055_RSA_MCLK_240MHZ = 0,
    NA51055_RSA_MCLK_320MHZ,
    NA51055_RSA_MCLK_RSVD,
    NA51055_RSA_MCLK_PLL9,
    NA51055_RSA_MCLK_MAX
} NA51055_RSA_MCLK_T;

typedef enum {
    NA51055_RSA_MODE_NORMAL = 0,                          ///< normal mode, encryption or decryption
    NA51055_RSA_MODE_CRC_KEY_N,                           ///< N key check by crc32
    NA51055_RSA_MODE_CRC_KEY_ED,                          ///< E/D key check by crc32
    NA51055_RSA_MODE_MAX
} NA51055_RSA_MODE_T;

typedef enum {
    NA51055_RSA_KEY_256 = 0,                              ///< key width 256  bits
    NA51055_RSA_KEY_512,                                  ///< key width 512  bits
    NA51055_RSA_KEY_1024,                                 ///< key width 1024 bits
    NA51055_RSA_KEY_2048,                                 ///< key width 2048 bits
    NA51055_RSA_KEY_4096,                                 ///< key width 4096 bits, only support in NA51084
    NA51055_RSA_KEY_MAX
} NA51055_RSA_KEY_T;

#define NA51055_RSA_BUFFER_LEN             512            ///< 512 bytes

#define NA51055_RSA_CFG_REG                0x00           ///< configuration
#define NA51055_RSA_CTRL_REG               0x04           ///< control
#define NA51055_RSA_INT_ENB_REG            0x08           ///< interrupt enable
#define NA51055_RSA_INT_STS_REG            0x0C           ///< interrupt status
#define NA51055_RSA_KEY_N_REG              0x10           ///< key N register
#define NA51055_RSA_KEY_N_ADDR_REG         0x14           ///< key N config ram address
#define NA51055_RSA_KEY_ED_REG             0x18           ///< key E/D register
#define NA51055_RSA_KEY_ED_ADDR_REG        0x1C           ///< key E/D config ram address
#define NA51055_RSA_DATA_REG               0x20           ///< input data register
#define NA51055_RSA_DATA_ADDR_REG          0x24           ///< input data config ram address
#define NA51055_RSA_KEY_READ_REG           0x28           ///< rsa key readable control
#define NA51055_RSA_CRC32_DEFAULT_REG      0x30           ///< crc32 default value
#define NA51055_RSA_CRC32_POLY_REG         0x34           ///< crc32 polynomial parameter
#define NA51055_RSA_CRC32_OUTPUT_REG       0x38           ///< rsa key crc32 result

#endif  /* _NA51055_RSA_H_ */
