#ifndef __CONFIG_H__
#define __CONFIG_H__

#ifdef CHIP_TYPE_hi3518e

/********* Here define the function supported by chip *****************/
#define  CIPHER_IRQ_NUMBER                      13
#define  CIPHER_HASH_SUPPORT
#define  CIPHER_MHASH_SUPPORT
#define  CIPHER_RSA_SUPPORT
#define  CIPHER_RNG_SUPPORT
#define  CIPHER_RNG_VERSION_1
//#define  CIPHER_CCM_GCM_SUPPORT
#define  CIPHER_MULTICIPHER_SUPPORT
//#define  CIPHER_KLAD_SUPPORT
#define  CIPHER_EFUSE_SUPPORT

/********* Here define the base address of chip ***********************/
#define  REG_BASE_PHY_ADDR_CIPHER            (0x100C0000)
#define  REG_BASE_PHY_ADDR_SHA               (0x10070000)
#define  REG_BASE_PHY_ADDR_RSA               (0x20260000) 
#define  REG_BASE_PHY_ADDR_RNG 		         (0x20280200)
#define  REG_BASE_PHY_ADDR_EFUSE 		     (0x20100800)

/********* Here define the clcok and reset signal in CRG ***************/
#define  REG_CRG_CLK_PHY_ADDR_CIPHER     (0x2003007C)   
#define  REG_CRG_CLK_PHY_ADDR_SHA        (0x2003006C)   
#define  REG_CRG_CLK_PHY_ADDR_RSA		 (0x20030100)  
#define  REG_CRG_CLK_PHY_ADDR_RNG		 (0x00)      // Not config
#define  CRG_RST_BIT_CIPHER              0
#define  CRG_CLK_BIT_CIPHER              1
#define  CRG_RST_BIT_SHA                 2
#define  CRG_CLK_BIT_SHA                 3
#define  CRG_RST_BIT_RSA                 4
#define  CRG_CLK_BIT_RSA                 5
#define  CRG_RST_BIT_RNG                 0
#define  CRG_CLK_BIT_RNG                 0
#define  CIPHER_READL                    readl
#define  CIPHER_WRITEL                   writel

#else
    #error You need to define a correct chip type!
#endif
#endif

