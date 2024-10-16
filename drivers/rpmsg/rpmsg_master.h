#ifndef RPMSG_MASTER_H_
#define RPMSG_MASTER_H_

#include <uapi/linux/rpmsg.h>
#include "rpmsg_internal.h"

#define RPMSG_DEV_MAX	(MINORMASK + 1)

#define RPMSG_ACK_OK                0x13131411

#define RPMSG_ACK_FAILED            0x13131412
#define RPMSG_ACK_NOLISTEN          0x13131413
#define RPMSG_ACK_BUSY              0x13131414
#define RPMSG_ACK_NOMEM             0x13131415
#define RPMSG_ACK_NOENT             0x13131416

#define RPMSG_CREATE_CLIENT         0x13141413
#define RPMSG_CLOSE_CLIENT          0x13141414 /* host release */
#define RPMSG_RELEASE_CLIENT        0x13141415 /* client release */

#define __pack					__attribute__((__packed__))

struct rpmsg_ctrl_msg {
	char name[32];
	uint32_t id;
	uint32_t ctrl_id;
	uint32_t cmd;
} __pack;

struct rpmsg_ctrl_msg_ack {
	uint32_t id;
	uint32_t ack;
} __pack;

extern struct class *rpmsg_ctrldev_get_class(void);
extern dev_t rpmsg_ctrldev_get_devt(void);
extern void rpmsg_ctrldev_put_devt(dev_t devt);
extern int rpmsg_ctrldev_notify(int ctrl_id, int id);

#endif
