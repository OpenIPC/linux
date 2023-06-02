/*!
********************************************************************************
*  \file    ErrorNo.h
*  \brief
*  \project vos
*  \chip
*  \author Anderson
********************************************************************************
*/
#ifndef _VOS_ERROR_NO_H_
#define _VOS_ERROR_NO_H_

#ifdef __cplusplus
extern "C" {
#endif


/*-----------------------------------------------------------------------------*/
/* Including Files                                                                                                                 */
/*-----------------------------------------------------------------------------*/
/*-----------------------------------------------------------------------------*/
/* Constant Definitions                                                                                                         */
/*-----------------------------------------------------------------------------*/
#define E_OK                0           ///< Normal completion
#define E_SYS               (-5)        ///< System error
#define E_NOMEM             (-10)       ///< Insufficient memory
#define E_NOSPT             (-17)       ///< Feature not supported
#define E_INOSPT            (-18)       ///< Feature not supported by ITRON/FILE specification
#define E_RSFN              (-20)       ///< Reserved function code number
#define E_RSATR             (-24)       ///< Reserved attribute
#define E_PAR               (-33)       ///< Parameter error
#define E_ID                (-35)       ///< Invalid ID number
#define E_NOEXS             (-52)       ///< Object does not yet exist
#define E_OBJ               (-63)       ///< Invalid object state
#define E_MACV              (-65)       ///< Memory access disabled or memory access violation
#define E_OACV              (-66)       ///< Object access violation
#define E_CTX               (-69)       ///< Context error
#define E_QOVR              (-73)       ///< Queuing or nesting overflow
#define E_DLT               (-81)       ///< Object being waited for was deleted
#define E_TMOUT             (-85)       ///< Polling failure or timeout exceeded
#define E_RLWAI             (-86)       ///< WAIT state was forcibly released

/*-----------------------------------------------------------------------------*/
/* Linux-style pointer error code                                              */
/*-----------------------------------------------------------------------------*/
#define VOS_MAX_ERRNO           4095
#define VOS_IS_ERR_VALUE(x)     ((unsigned long)(void *)(x) >= (unsigned long)-VOS_MAX_ERRNO)
#define VOS_ERR_PTR(e)          ((void *)(e))
#define VOS_PTR_ERR(ptr)        ((long)ptr)
#define VOS_IS_ERR(ptr)         VOS_IS_ERR_VALUE((unsigned long)ptr)
#define VOS_IS_ERR_OR_NULL(ptr) ((!ptr) || VOS_IS_ERR_VALUE((unsigned long)ptr))

#ifdef __cplusplus
}
#endif

#endif /* _VOS_ERROR_NO_H_ */

