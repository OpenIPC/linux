/******************************************************************************

  Copyright (C), 2001-2014, Fullhan Tech. Co., Ltd.

 ******************************************************************************
  File Name     : fh_error_mpi.h
  Version       : Initial Draft
  Author        : Fullhan multimedia software group
  Created       : 2008/04/24
  Description   : error code of MPI
  History       :
  1.Date        : 2008/04/24
    Author      : q46153
    Modification: Created file

******************************************************************************/
/** 
\file
\brief Describes the information about common error codes. 
*/
/* add include here */
#ifndef __FH_UNF_MPI_ERRORCODE_H__
#define __FH_UNF_MPI_ERRORCODE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* End of #ifdef __cplusplus */

/***************************** Macro Definition ******************************/
/** \addtogroup      MSP_ERRCODE */
/** @{ */  /** <!--  [MSP_ERRCODE] */

#define  FH_ERR_CIPHER_NOT_INIT                     (FH_S32)(0x804D0001)
#define  FH_ERR_CIPHER_INVALID_HANDLE               (FH_S32)(0x804D0002)
#define  FH_ERR_CIPHER_INVALID_POINT                (FH_S32)(0x804D0003)
#define  FH_ERR_CIPHER_INVALID_PARA                 (FH_S32)(0x804D0004)
#define  FH_ERR_CIPHER_FAILED_INIT                  (FH_S32)(0x804D0005)
#define  FH_ERR_CIPHER_FAILED_GETHANDLE             (FH_S32)(0x804D0006)
#define  FH_ERR_CIPHER_FAILED_RELEASEHANDLE         (FH_S32)(0x804D0007)
#define  FH_ERR_CIPHER_FAILED_CONFIGAES             (FH_S32)(0x804D0008)
#define  FH_ERR_CIPHER_FAILED_CONFIGDES             (FH_S32)(0x804D0009)
#define  FH_ERR_CIPHER_FAILED_ENCRYPT               (FH_S32)(0x804D000A)
#define  FH_ERR_CIPHER_FAILED_DECRYPT               (FH_S32)(0x804D000B)
#define  FH_ERR_CIPHER_BUSY                         (FH_S32)(0x804D000C)
#define  FH_ERR_CIPHER_NO_AVAILABLE_RNG             (FH_S32)(0x804D000D)

#define  FH_ERR_CIPHER_FAILED_CONFIGKEY             (FH_S32)(0x804D000E)
#define  FH_ERR_CIPHER_FAILED_CONFIGIV              (FH_S32)(0x804D000F)
#define  FH_ERR_CIPHER_FAILED_CONFIGINS             (FH_S32)(0x804D0010)
#define	 FH_ERR_CIPHER_FAILED_CHECKSTAGE            (FH_S32)(0x804D0011)
#define	 FH_ERR_CIPHER_FAILED_CHECKST               (FH_S32)(0x804D0012)
#define	 FH_ERR_CIPHER_FAILED_CHECKALG              (FH_S32)(0x804D0013)

/** @} */  /*! <!-- Macro Definition end */



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif /* End of #ifndef __FH_UNF_MPI_ERRORCODE_H__ */

