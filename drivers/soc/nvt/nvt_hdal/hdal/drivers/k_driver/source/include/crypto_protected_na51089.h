/*
    Novatek protected header file of NT98520's driver.

    The header file for Novatek protected APIs of NT98520's driver.

    @file       efuse_protected.h
    @ingroup    mIDriver
    @note       For Novatek internal reference, don't export to agent or customer.

    Copyright   Novatek Microelectronics Corp. 2012.  All rights reserved.
*/

#ifndef _NVT_CRYPTO_PROTECTED_H
#define _NVT_CRYPTO_PROTECTED_H

#include <kwrap/error_no.h>
#include <kwrap/nvt_type.h>
#include "plat/crypto.h"
#include "efuse_protected.h"

extern INT32 crypto_data_operation(EFUSE_OTP_KEY_SET_FIELD key_set, CRYPT_OP crypt_op_param);

#endif  /* _NVT_CRYPTO_PROTECTED_H */

