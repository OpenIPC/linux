#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef CHIP_TYPE_hi3516a

/********* Here define the function supported by chip *****************/
#define  CIPHER_IRQ_NUMBER                      58
//#define  CIPHER_HASH_SUPPORT
//#define  CIPHER_MHASH_SUPPORT
//#define  CIPHER_RSA_SUPPORT
//#define  CIPHER_RNG_SUPPORT
//#define  CIPHER_CCM_GCM_SUPPORT
//#define  CIPHER_EFUSE_SUPPORT
#define  CIPHER_MULTICIPHER_SUPPORT

/********* Here define the base address of chip ***********************/
#define  REG_BASE_PHY_ADDR_CIPHER            (0x100c0000)

/********* Here define the clcok and reset signal in CRG ***************/
#define  REG_CRG_CLK_PHY_ADDR_CIPHER         (0x2003007C)
#define  CRG_RST_BIT_CIPHER              0
#define  CRG_CLK_BIT_CIPHER              1
#define  CIPHER_READL                    readl
#define  CIPHER_WRITEL                   writel

#else
    #error You need to define a correct chip type!
#endif
#endif

