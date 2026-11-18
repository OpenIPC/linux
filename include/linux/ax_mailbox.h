#ifndef __AX_MAILBOX_H__
#define __AX_MAILBOX_H__

/*mailbox message length is 32 bytes*/
typedef struct {
    unsigned char id;
    unsigned char data[31];
} mbox_msg_t;
typedef int(mbox_callback_t)(mbox_msg_t *data, void *pri_data); /* callback*/
void ax_mailbox_set_callback(mbox_callback_t *callback, void *pri_data);
int ax_mailbox_send_message(unsigned int send_masterid, unsigned int receive_masterid, mbox_msg_t *data);
typedef enum  {
    AX_MAILBOX_MASTERID_ARM0 = 0,/*default in linux driver*/
    AX_MAILBOX_MASTERID_ARM1 = 1,
    AX_MAILBOX_MASTERID_RISCV = 2,
    //ADD YOUR MASTEDID HERE
    AX_MAILBOX_MASTERID_MAX = 16,
} AX_MAILBOX_MASTERID;

#endif