/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _MEDIA_FH8626_SUBDEV_H
#define _MEDIA_FH8626_SUBDEV_H

#include <linux/kref.h>

/* FH supplier storage retained by bridge nodes and inherited controls. */
struct fh8626_subdev_lifetime {
	struct kref *ref;
	void (*release)(struct kref *ref);
};

#endif
