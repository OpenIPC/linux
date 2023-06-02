#ifndef __HI_UNF_CIPHER_H__
#define __HI_UNF_CIPHER_H__

#include "hi_type.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */
/*************************** Structure Definition ****************************/
/** \addtogroup      CIPHER */
/** @{ */  /** <!-- [CIPHER] */

/** min length of encrypt, unit: byte */
#define HI_UNF_CIPHER_MIN_CRYPT_LEN      1

/** max length of encrypt, unit: byte */
#define HI_UNF_CIPHER_MAX_CRYPT_LEN      0xfffff

#define HI_UNF_CIPHER_MAX_RSA_KEY_LEN    (512)

/** Cipher work mode */
typedef enum hiHI_UNF_CIPHER_WORK_MODE_E
{
    HI_UNF_CIPHER_WORK_MODE_ECB,        /**<Electronic codebook (ECB) mode*/
    HI_UNF_CIPHER_WORK_MODE_CBC,        /**<Cipher block chaining (CBC) mode*/
    HI_UNF_CIPHER_WORK_MODE_CFB,        /**<Cipher feedback (CFB) mode*/
    HI_UNF_CIPHER_WORK_MODE_OFB,        /**<Output feedback (OFB) mode*/
    HI_UNF_CIPHER_WORK_MODE_CTR,        /**<Counter (CTR) mode*/
    HI_UNF_CIPHER_WORK_MODE_CCM,
    HI_UNF_CIPHER_WORK_MODE_GCM,
    HI_UNF_CIPHER_WORK_MODE_CBC_CTS,    /**<Cipher block chaining CipherStealing mode*/
    HI_UNF_CIPHER_WORK_MODE_BUTT    = 0xffffffff
}HI_UNF_CIPHER_WORK_MODE_E;

/** Cipher algorithm */
typedef enum hiHI_UNF_CIPHER_ALG_E
{
    HI_UNF_CIPHER_ALG_DES           = 0x0,  /**< Data encryption standard (DES) algorithm */
    HI_UNF_CIPHER_ALG_3DES          = 0x1,  /**< 3DES algorithm */
    HI_UNF_CIPHER_ALG_AES           = 0x2,  /**< Advanced encryption standard (AES) algorithm */
    HI_UNF_CIPHER_ALG_BUTT          = 0x3
}HI_UNF_CIPHER_ALG_E;

/** Key length */
typedef enum hiHI_UNF_CIPHER_KEY_LENGTH_E
{
    HI_UNF_CIPHER_KEY_AES_128BIT    = 0x0,  /**< 128-bit key for the AES algorithm */
    HI_UNF_CIPHER_KEY_AES_192BIT    = 0x1,  /**< 192-bit key for the AES algorithm */
    HI_UNF_CIPHER_KEY_AES_256BIT    = 0x2,  /**< 256-bit key for the AES algorithm */
    HI_UNF_CIPHER_KEY_DES_3KEY      = 0x2,  /**< Three keys for the DES algorithm */
    HI_UNF_CIPHER_KEY_DES_2KEY      = 0x3,  /**< Two keys for the DES algorithm */
}HI_UNF_CIPHER_KEY_LENGTH_E;

/** Cipher bit width */
typedef enum hiHI_UNF_CIPHER_BIT_WIDTH_E
{
    HI_UNF_CIPHER_BIT_WIDTH_64BIT   = 0x0,  /**< 64-bit width */
    HI_UNF_CIPHER_BIT_WIDTH_8BIT    = 0x1,  /**< 8-bit width */
    HI_UNF_CIPHER_BIT_WIDTH_1BIT    = 0x2,  /**< 1-bit width */
    HI_UNF_CIPHER_BIT_WIDTH_128BIT  = 0x3,  /**< 128-bit width */
}HI_UNF_CIPHER_BIT_WIDTH_E;

/** Cipher control parameters */
typedef struct hiHI_UNF_CIPHER_CTRL_CHANGE_FLAG_S
{
    HI_U32   bit1IV:1;              /**< Initial Vector change or not */
    HI_U32   bitsResv:31;           /**< Reserved */
}HI_UNF_CIPHER_CTRL_CHANGE_FLAG_S;

/** Encryption/Decryption type selecting */
typedef enum hiHI_UNF_CIPHER_KEY_SRC_E
{
    HI_UNF_CIPHER_KEY_SRC_USER       = 0x0,  /**< User Key*/
    HI_UNF_CIPHER_KEY_SRC_EFUSE_0,          /**< Efuse Key 0*/
    HI_UNF_CIPHER_KEY_SRC_EFUSE_1,          /**< Efuse Key 1*/
    HI_UNF_CIPHER_KEY_SRC_EFUSE_2,          /**< Efuse Key 2*/
    HI_UNF_CIPHER_KEY_SRC_EFUSE_3,          /**< Efuse Key 3*/
    HI_UNF_CIPHER_KEY_SRC_BUTT,
}HI_UNF_CIPHER_KEY_SRC_E;

/** Structure of the cipher control information */
typedef struct hiHI_UNF_CIPHER_CTRL_S
{
    HI_U32 u32Key[8];                               /**< Key input */                                                                                                     /**< CNcomment:输入密钥 */
    HI_U32 u32IV[4];                                /**< Initialization vector (IV) */                                                                                    /**< CNcomment:初始向量 */
    HI_UNF_CIPHER_ALG_E enAlg;                      /**< Cipher algorithm */                                                                                              /**< CNcomment:加密算法 */
    HI_UNF_CIPHER_BIT_WIDTH_E enBitWidth;           /**< Bit width for encryption or decryption */                                                                        /**< CNcomment:加密或解密的位宽 */
    HI_UNF_CIPHER_WORK_MODE_E enWorkMode;           /**< Operating mode */                                                                                                /**< CNcomment:工作模式 */
    HI_UNF_CIPHER_KEY_LENGTH_E enKeyLen;            /**< Key length */                                                                                                    /**< CNcomment:密钥长度 */
    HI_UNF_CIPHER_CTRL_CHANGE_FLAG_S stChangeFlags; /**< control information exchange choices, we default all woulde be change except they have been in the choices */    /**< CNcomment:控制信息变更选项，选项中没有标识的项默认全部变更 */
    HI_UNF_CIPHER_KEY_SRC_E enKeySrc;             /**< Key source */
} HI_UNF_CIPHER_CTRL_S;

/** Cipher data */
typedef struct hiHI_UNF_CIPHER_DATA_S
{
    HI_U32 u32SrcPhyAddr;     /**< phy address of the original data */
    HI_U32 u32DestPhyAddr;    /**< phy address of the purpose data */
    HI_U32 u32ByteLength;     /**< cigher data length*/
} HI_UNF_CIPHER_DATA_S;


/** @} */  /** <!-- ==== Structure Definition End ==== */


#define HI_UNF_CIPHER_Open(HI_VOID) HI_UNF_CIPHER_Init(HI_VOID);
#define HI_UNF_CIPHER_Close(HI_VOID) HI_UNF_CIPHER_DeInit(HI_VOID);

/******************************* API Declaration *****************************/
/** \addtogroup      CIPHER */
/** @{ */  /** <!-- [CIPHER] */
/* ---CIPHER---*/
/**
\brief  Init the cipher device.  
\attention \n
This API is used to start the cipher device.
\param N/A                                                                      
\retval ::HI_SUCCESS  Call this API successful.                                 
\retval ::HI_FAILURE  Call this API fails.                                      
\retval ::HI_ERR_CIPHER_FAILED_INIT  The cipher device fails to be initialized. 
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_Init(HI_VOID);

/**
\brief  Deinit the cipher device.
\attention \n
This API is used to stop the cipher device. If this API is called repeatedly, HI_SUCCESS is returned, but only the first operation takes effect.

\param N/A                                                                      
\retval ::HI_SUCCESS  Call this API successful.                                 
\retval ::HI_FAILURE  Call this API fails.                                      
\retval ::HI_ERR_CIPHER_NOT_INIT  The cipher device is not initialized.         
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_DeInit(HI_VOID);

/**
\brief Obtain a cipher handle for encryption and decryption.

\param[in] cipher attributes                                                    
\param[out] phCipher Cipher handle                                              
\retval ::HI_SUCCESS Call this API successful.                                  
\retval ::HI_FAILURE Call this API fails.                                       
\retval ::HI_ERR_CIPHER_NOT_INIT  The cipher device is not initialized.        
\retval ::HI_ERR_CIPHER_INVALID_POINT  The pointer is null.                     
\retval ::HI_ERR_CIPHER_FAILED_GETHANDLE  The cipher handle fails to be obtained, because there are no available cipher handles. 
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_CreateHandle(HI_HANDLE* phCipher);

/**
\brief Destroy the existing cipher handle. 
\attention \n
This API is used to destroy existing cipher handles.

\param[in] hCipher Cipher handle                                                
\retval ::HI_SUCCESS  Call this API successful.                                 
\retval ::HI_FAILURE  Call this API fails.                                      
\retval ::HI_ERR_CIPHER_NOT_INIT  The cipher device is not initialized.         
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_DestroyHandle(HI_HANDLE hCipher);

/**
\brief Configures the cipher control information.
\attention \n
Before encryption or decryption, you must call this API to configure the cipher control information.
The first 64-bit data and the last 64-bit data should not be the same when using TDES algorithm.

\param[in] hCipher Cipher handle.                                               
\param[in] pstCtrl Cipher control information.                                  
\retval ::HI_SUCCESS Call this API successful.                                  
\retval ::HI_FAILURE Call this API fails.                                       
\retval ::HI_ERR_CIPHER_NOT_INIT  The cipher device is not initialized.         
\retval ::HI_ERR_CIPHER_INVALID_POINT  The pointer is null.                     
\retval ::HI_ERR_CIPHER_INVALID_PARA  The parameter is invalid.                 
\retval ::HI_ERR_CIPHER_INVALID_HANDLE  The handle is invalid.                 
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_ConfigHandle(HI_HANDLE hCipher, HI_UNF_CIPHER_CTRL_S* pstCtrl);

/**
\brief Performs encryption.

\attention \n
This API is used to perform encryption by using the cipher module.
The length of the encrypted data should be a multiple of 8 in TDES mode and 16 in AES mode. Besides, the length can not be bigger than 0xFFFFF.After this operation, the result will affect next operation.If you want to remove vector, you need to config IV(config pstCtrl->stChangeFlags.bit1IV with 1) by transfering HI_UNF_CIPHER_ConfigHandle.
\param[in] hCipher Cipher handle                                                
\param[in] u32SrcPhyAddr Physical address of the source data                    
\param[in] u32DestPhyAddr Physical address of the target data                   
\param[in] u32ByteLength   Length of the encrypted data                         
\retval ::HI_SUCCESS  Call this API successful.                                 
\retval ::HI_FAILURE  Call this API fails.                                      
\retval ::HI_ERR_CIPHER_NOT_INIT  The cipher device is not initialized.         
\retval ::HI_ERR_CIPHER_INVALID_PARA  The parameter is invalid.                 
\retval ::HI_ERR_CIPHER_INVALID_HANDLE  The handle is invalid.                  
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_Encrypt(HI_HANDLE hCipher, HI_U32 u32SrcPhyAddr, HI_U32 u32DestPhyAddr, HI_U32 u32ByteLength);

/**
\brief Performs decryption.

\attention \n
This API is used to perform decryption by using the cipher module.
The length of the decrypted data should be a multiple of 8 in TDES mode and 16 in AES mode. Besides, the length can not be bigger than 0xFFFFF.After this operation, the result will affect next operation.If you want to remove vector, you need to config IV(config pstCtrl->stChangeFlags.bit1IV with 1) by transfering HI_UNF_CIPHER_ConfigHandle.
\param[in] hCipher Cipher handle.                                               
\param[in] u32SrcPhyAddr Physical address of the source data.                   
\param[in] u32DestPhyAddr Physical address of the target data.                  
\param[in] u32ByteLength Length of the decrypted data                          
\retval ::HI_SUCCESS Call this API successful.                                  
\retval ::HI_FAILURE Call this API fails.                                       
\retval ::HI_ERR_CIPHER_NOT_INIT  The cipher device is not initialized.        
\retval ::HI_ERR_CIPHER_INVALID_PARA  The parameter is invalid.                 
\retval ::HI_ERR_CIPHER_INVALID_HANDLE  The handle is invalid.                  
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_Decrypt(HI_HANDLE hCipher, HI_U32 u32SrcPhyAddr, HI_U32 u32DestPhyAddr, HI_U32 u32ByteLength);

/**
\brief Encrypt multiple packaged data.
\attention \n
You can not encrypt more than 128 data package one time. When HI_ERR_CIPHER_BUSY return, the data package you send will not be deal, the custmer should decrease the number of data package or run cipher again.Note:When encrypting more than one packaged data, every one package will be calculated using initial vector configured by HI_UNF_CIPHER_ConfigHandle.Previous result will not affect the later result.
\param[in] hCipher cipher handle                                                                  
\param[in] pstDataPkg data package ready for cipher                                               
\param[in] u32DataPkgNum  number of package ready for cipher                                      
\retval ::HI_SUCCESS  Call this API successful.                                                   
\retval ::HI_FAILURE  Call this API fails.                                                        
\retval ::HI_ERR_CIPHER_NOT_INIT  cipher device have not been initialized                         
\retval ::HI_ERR_CIPHER_INVALID_PARA  parameter error                                             
\retval ::HI_ERR_CIPHER_INVALID_HANDLE  handle invalid                                            
\retval ::HI_ERR_CIPHER_BUSY  hardware is busy, it can not deal with all data package once time   
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_EncryptMulti(HI_HANDLE hCipher, HI_UNF_CIPHER_DATA_S *pstDataPkg, HI_U32 u32DataPkgNum);


/**
\brief Decrypt multiple packaged data.
\attention \n
You can not decrypt more than 128 data package one time.When HI_ERR_CIPHER_BUSY return, the data package you send will not be deal, the custmer should decrease the number of data package or run cipher again.Note:When decrypting more than one packaged data, every one package will be calculated using initial vector configured by HI_UNF_CIPHER_ConfigHandle.Previous result will not affect the later result.
\param[in] hCipher cipher handle                                                                 
\param[in] pstDataPkg data package ready for cipher                                             
\param[in] u32DataPkgNum  number of package ready for cipher                                     
\retval ::HI_SUCCESS  Call this API successful.                                                  
\retval ::HI_FAILURE  Call this API fails.                                                     
\retval ::HI_ERR_CIPHER_NOT_INIT  cipher device have not been initialized                        
\retval ::HI_ERR_CIPHER_INVALID_PARA  parameter error                                            
\retval ::HI_ERR_CIPHER_INVALID_HANDLE  handle invalid                                           
\retval ::HI_ERR_CIPHER_BUSY  hardware is busy, it can not deal with all data package once time 
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_DecryptMulti(HI_HANDLE hCipher, HI_UNF_CIPHER_DATA_S *pstDataPkg, HI_U32 u32DataPkgNum);

/**
\brief Encrypt multiple packaged data.
\attention \n
You can not encrypt more than 128 data package one time. When HI_ERR_CIPHER_BUSY return, the data package you send will not be deal, the custmer should decrease the number of data package or run cipher again.Note:When encrypting more than one packaged data, every one package will be calculated using initial vector configured by HI_UNF_CIPHER_ConfigHandle.Previous result will not affect the later result.
\param[in] hCipher cipher handle                                                                  
\param[in] pstCtrl Cipher control information.                                  
\param[in] pstDataPkg data package ready for cipher                                               
\param[in] u32DataPkgNum  number of package ready for cipher                                      
\retval ::HI_SUCCESS  Call this API successful.                                                   
\retval ::HI_FAILURE  Call this API fails.                                                        
\retval ::HI_ERR_CIPHER_NOT_INIT  cipher device have not been initialized                         
\retval ::HI_ERR_CIPHER_INVALID_PARA  parameter error                                             
\retval ::HI_ERR_CIPHER_INVALID_HANDLE  handle invalid                                            
\retval ::HI_ERR_CIPHER_BUSY  hardware is busy, it can not deal with all data package once time   
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_EncryptMultiEx(HI_HANDLE hCipher, HI_UNF_CIPHER_CTRL_S* pstCtrl, HI_UNF_CIPHER_DATA_S *pstDataPkg, HI_U32 u32DataPkgNum);

/**
\brief Decrypt multiple packaged data.
\attention \n
You can not decrypt more than 128 data package one time.When HI_ERR_CIPHER_BUSY return, the data package you send will not be deal, the custmer should decrease the number of data package or run cipher again.Note:When decrypting more than one packaged data, every one package will be calculated using initial vector configured by HI_UNF_CIPHER_ConfigHandle.Previous result will not affect the later result.
\param[in] hCipher cipher handle                                                                
\param[in] pstCtrl Cipher control information.                                  
\param[in] pstDataPkg data package ready for cipher                                              
\param[in] u32DataPkgNum  number of package ready for cipher                                    
\retval ::HI_SUCCESS  Call this API successful.                                                  
\retval ::HI_FAILURE  Call this API fails.                                                       
\retval ::HI_ERR_CIPHER_NOT_INIT  cipher device have not been initialized                        
\retval ::HI_ERR_CIPHER_INVALID_PARA  parameter error                                            
\retval ::HI_ERR_CIPHER_INVALID_HANDLE  handle invalid                                           
\retval ::HI_ERR_CIPHER_BUSY  hardware is busy, it can not deal with all data package once time  
\see \n
N/A
*/
HI_S32 HI_UNF_CIPHER_DecryptMultiEx(HI_HANDLE hCipher, HI_UNF_CIPHER_CTRL_S* pstCtrl, HI_UNF_CIPHER_DATA_S *pstDataPkg, HI_U32 u32DataPkgNum);
/** @} */  /** <!-- ==== API declaration end ==== */


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /* __HI_UNF_ECS_TYPE_H__ */

