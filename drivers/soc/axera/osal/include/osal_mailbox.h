/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_MAILBOX_H__
#define __OSAL_MAILBOX_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <linux/ax_mailbox.h>

void AX_OSAL_mailbox_set_callback(mbox_callback_t *callback, void *pri_data);
int AX_OSAL_mailbox_send_message(unsigned int send_masterid, unsigned int receive_masterid, mbox_msg_t *data);

#ifdef __cplusplus
}
#endif
#endif