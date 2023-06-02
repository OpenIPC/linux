
#ifndef _MST_DEF_H_
#define _MST_DEF_H_

typedef struct mst_dev {
    struct cdev* cdev;  // character device
    struct device ldev; // device node
    struct device* pdev;// platform device
    struct mutex mutex;
    void* dev_data;
} mst_dev;

typedef struct mst_ctx {
    mst_dev* mdev;
    void* drv_data;
} mst_ctx;

#endif//_MST_DEV_H_

