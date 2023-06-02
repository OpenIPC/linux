#ifndef _VOS_CDEV_H_
#define _VOS_CDEV_H_
#ifdef __cplusplus
extern "C" {
#endif

#if defined(__LINUX) && defined(__KERNEL__)
#include <linux/cdev.h>
#include <linux/fs.h>

void vos_dev_init(void *param);
void vos_dev_exit(void);

int vos_alloc_chrdev_region(dev_t *ret_dev, unsigned count, const char *name);
void vos_unregister_chrdev_region(dev_t from, unsigned count);

struct cdev* vos_icdev(const struct inode *inode);
dev_t vos_irdev(const struct inode *inode);

#endif

#ifdef __cplusplus
}
#endif

#endif /* _VOS_CDEV_H_ */

