/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#include "osal_mailbox.h"
#include <linux/export.h>

#ifdef CONFIG_AXERA_MAILBOX
void AX_OSAL_mailbox_set_callback(mbox_callback_t *callback, void *pri_data)
{
	ax_mailbox_set_callback(callback, pri_data);
}
EXPORT_SYMBOL(AX_OSAL_mailbox_set_callback);

int AX_OSAL_mailbox_send_message(unsigned int send_masterid, unsigned int receive_masterid, mbox_msg_t *data)
{
	int ret;
	ret = ax_mailbox_send_message(send_masterid, receive_masterid, data);
	return ret;
}
EXPORT_SYMBOL(AX_OSAL_mailbox_send_message);
#else
void AX_OSAL_mailbox_set_callback(mbox_callback_t *callback, void *pri_data)
{
}
EXPORT_SYMBOL(AX_OSAL_mailbox_set_callback);

int AX_OSAL_mailbox_send_message(unsigned int send_masterid, unsigned int receive_masterid, mbox_msg_t *data)
{
	return 0;
}
EXPORT_SYMBOL(AX_OSAL_mailbox_send_message);
#endif