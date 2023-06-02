/*
 * rpmsg read from nfs server driver
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/rpmsg.h>
#include <linux/err.h>
#include <linux/remoteproc.h>

#include <linux/poll.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/kfifo.h>

#include <linux/virtio_ring.h>
#include <linux/sched.h>
#include <linux/delay.h>

#define DEVICE_NAME "rnfs"


typedef int redev_nfs_FILE;
typedef struct nfs_fread_info_s {
  int *addr;
  int size;
  int count;
  redev_nfs_FILE *file;
}
nfs_fread_info_t;

struct rnfs_struct {
  struct kfifo    fifo;
  spinlock_t    lock;
  struct rpmsg_channel  *rpdev;
  struct miscdevice fdev;
  nfs_fread_info_t unfinished;
};

static struct rnfs_struct g_rnfs;

static int device_open(struct inode *inode, struct file *filp)
{
  filp->private_data = &g_rnfs;

  return 0;
}

static ssize_t device_write(struct file *filp, const char *buf,
                            size_t count, loff_t *off)
{
  struct rnfs_struct *rnfs = filp->private_data;
  //struct rpmsg_channel *rpdev = rnfs->rpdev;
  unsigned int ret;
  unsigned int avail;
  unsigned int unfinished = count;
  unsigned int cpsize;
  unsigned int cpsize_total;
  char *rp = buf;

  //printk("device_write: rp 0x%x unfinished %x\n", rp, unfinished);

  cpsize_total = 0;
  while(unfinished > 0) {
    avail = kfifo_avail(&rnfs->fifo);
    if(avail == 0) {
      //printk("rpmsg_rnfs_cb: msleep(1000);");
      msleep(5);
      cpsize = 0;
    } else if(avail > unfinished){
      cpsize = unfinished;
    } else if(avail <= unfinished) {
      cpsize = avail;
    }

    if(cpsize > 0) {
      ret = kfifo_in_locked(&rnfs->fifo, (char *)rp, cpsize, &rnfs->lock);
      rp += cpsize;
      unfinished -= cpsize;
      cpsize_total += cpsize;
    }
    //printk("device_write: avail %x rp 0x%x unfinished %x\n", avail, rp, unfinished);
  }

  if(cpsize_total != count)
    printk("rnfs device_write: cpsize_total != count\n");

  return cpsize_total;
}

int rnfs_test_counter = 0;
static ssize_t device_read(struct file *filp, char *buf,
                           size_t len, loff_t *offset)
{
  struct rnfs_struct *rnfs = filp->private_data;
  int bytes_read = 0;
  // char c;
  char *vaddr;
  nfs_fread_info_t nfs_fread_info;
  int copy_size;
  int unfinished_size;
  int ret;
  int len_remain = len;

  //printk(" ====================== device_read %d ======================\n", rnfs_test_counter);

//rnfs_test_counter ++;
  if(rnfs_test_counter > 2000) {
    printk("device_read: force 0\n");
    rnfs_test_counter = 0;
    return 0;
  }

//  while (kfifo_out_locked(&rnfs->fifo, &c, sizeof(c), &rnfs->lock)) {
//
//    *buf++ = c;
//    bytes_read++;
//  }

//         blocking = filp->??

  while(len_remain > 0) {
    switch (rnfs->unfinished.count) {
      case 0:
        //reload
//                          while(1) {
        ret = kfifo_out_locked(&rnfs->fifo, &nfs_fread_info, sizeof(nfs_fread_info_t), &rnfs->lock);
        //printk("reload: info %d/%d\n", ret, sizeof(nfs_fread_info_t));
//          //if ret==not ready, sleep
//          if(ret < sizeof(nfs_fread_info_t))
//            if(blocking)
//              sleep();
//            else return -EAGAIN;
//          else
//            break;
//        }
        if(ret < sizeof(nfs_fread_info_t)) {
          msleep(5);
          //printk("reload: not ready %d\n", ret);
          break;
        } else {
          if(nfs_fread_info.size == 0xdeadbeef) {
            printk("nfs_fread_info.size == 0xdeadbeef");
            rnfs_test_counter = 2000000;
            rpmsg_sendto(rnfs->rpdev, &bytes_read, sizeof(int), rnfs->rpdev->dst); //ack to release fread of iTRON side
            return 0;
          }
          memcpy(&(rnfs->unfinished), &nfs_fread_info, sizeof(nfs_fread_info_t));
          //printk("reload: x%x %d %d\n", rnfs->unfinished.addr, rnfs->unfinished.size, rnfs->unfinished.count);
        }

      default:
        //fill-up buf
        unfinished_size = rnfs->unfinished.size * rnfs->unfinished.count;
        copy_size = (len_remain > unfinished_size) ? unfinished_size : len_remain;
        if(copy_size>0) {
          vaddr = (void *)ambarella_phys_to_virt((unsigned long)rnfs->unfinished.addr);
          memcpy(buf, vaddr, copy_size);
          bytes_read += copy_size;
          buf += copy_size;
          unfinished_size -= copy_size;

          if(unfinished_size > 0) {
            rnfs->unfinished.count = unfinished_size / rnfs->unfinished.size;
            rnfs->unfinished.addr = (int *)((unsigned int)rnfs->unfinished.addr + copy_size);
          } else {
            memset(&(rnfs->unfinished), 0, sizeof(nfs_fread_info_t));
            rpmsg_sendto(rnfs->rpdev, &bytes_read, sizeof(int), rnfs->rpdev->dst); //ack to release fread of iTRON side
            //printk("ack\n");
          }
          len_remain -= copy_size;
        }
        //printk("fillup: %d/%d \n", bytes_read, len);
    }
  }

//  while (kfifo_out_locked(&rnfs->fifo, &nfs_fread_info, sizeof(nfs_fread_info_t), &rnfs->lock)) {
// //     vaddr = ambarella_phys_to_virt(nfs_fread_info.addr);
// //                 bytes_read += nfs_fread_info.size*nfs_fread_info.count;
// //                 memcpy(buf, vaddr, nfs_fread_info.size*nfs_fread_info.count);
// //                 buf += bytes_read;
//    //printk("device_read: x%x x%x\n", vaddr, bytes_read);
//    bytes_read = len;
//  }

  //rpmsg_sendto(g_rnfs.rpdev, &bytes_read, sizeof(int), g_rnfs.rpdev->dst);
  //printk("device_read: final bytes_read %d/%d \n", bytes_read, len);

  return bytes_read;
}

static int device_release(struct inode *inode, struct file *file)
{
  return 0;
}

static struct file_operations fops = {
  .read = device_read,
  .write = device_write,
  .open = device_open,
  .release = device_release
};

static int rnfs_init(struct rnfs_struct *rnfs)
{
  int ret;

  spin_lock_init(&rnfs->lock);

  ret = kfifo_alloc(&rnfs->fifo, 1024*1024*2, GFP_KERNEL);
  if (ret)
    return ret;

  printk("RPMSG Read from NFS Server [ARM11] now is ready at /dev/%s\n", DEVICE_NAME);

  rnfs->fdev.name = DEVICE_NAME;
  rnfs->fdev.fops = &fops;
  rnfs->fdev.minor = MISC_DYNAMIC_MINOR;
  memset(&(rnfs->unfinished), 0, sizeof(nfs_fread_info_t));
  misc_register(&rnfs->fdev);

  return 0;
}

static void rpmsg_rnfs_cb(struct rpmsg_channel *rpdev, void *data,
                          int len, void *priv, u32 src)
{
  struct rnfs_struct *rnfs = priv;
  unsigned int ret;
  unsigned int avail;
  unsigned int unfinished = len;
  unsigned int cpsize;
  unsigned int cpsize_total;
  char *wp;
  nfs_fread_info_t *info = data;
#define EOF_TIMEOUT (20)
  unsigned int timeout = EOF_TIMEOUT;

  cpsize_total = 0;
  unfinished = info->size*info->count;
  printk("rpmsg_rnfs_cb: p 0x%x v 0x%x %x %x\n", info->addr, wp, info->size, info->count);
  wp = (void *)ambarella_phys_to_virt((unsigned long)info->addr);
  printk("rpmsg_rnfs_cb: p 0x%x v 0x%x %x %x\n", info->addr, wp, info->size, info->count);

  while(unfinished > 0) {
    avail = kfifo_len(&rnfs->fifo);
    if(avail == 0) {
      //printk("rpmsg_rnfs_cb: msleep(1000);");
      msleep(5);
      cpsize = 0;
      timeout--;
      if(timeout == 0) {
        printk("rpmsg_rnfs_cb: could be eof\n");
	break;
      }
    } else if(avail > unfinished){
      cpsize = unfinished;
    } else if(avail <= unfinished) {
      cpsize = avail;
    }

    if(cpsize > 0) {
      ret = kfifo_out_locked(&rnfs->fifo, wp, cpsize, &rnfs->lock);
      wp += cpsize;
      unfinished -= cpsize;
      cpsize_total += cpsize;
    }
    printk("rpmsg_rnfs_cb: avail %x wp %x, unfinished %x\n", avail, wp, unfinished);
  }

  if(cpsize_total != info->size*info->count)
    printk("rnfs rpmsg_rnfs_cb: cpsize_total != count\n");

  //ack to release fread of iTRON side
  rpmsg_sendto(rnfs->rpdev, &cpsize_total, sizeof(unsigned int), rnfs->rpdev->dst);
  printk("rnfs rpmsg_rnfs_cb: ack iTRON\n");

}

static int rpmsg_rnfs_probe(struct rpmsg_channel *rpdev)
{
  int ret = 0;
  struct rpmsg_ns_msg nsm;
  struct rnfs_struct *rnfs = &g_rnfs;

  nsm.addr = rpdev->dst;
  memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
  nsm.flags = 0;

  rnfs_init(rnfs);
  rnfs->rpdev = rpdev;

  rpdev->ept->priv = rnfs;

  rpmsg_sendto(rpdev, &nsm, sizeof(nsm), rpdev->dst);

  return ret;
}

static void rpmsg_rnfs_remove(struct rpmsg_channel *rpdev)
{
  struct rnfs_struct *rnfs = &g_rnfs;
  misc_deregister(&rnfs->fdev);
}

static struct rpmsg_device_id rpmsg_rnfs_id_table[] = {
  { .name = "rnfs_arm11", },
  { },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_rnfs_id_table);

static struct rpmsg_driver rpmsg_rnfs_driver = {
  .drv.name = KBUILD_MODNAME,
  .drv.owner  = THIS_MODULE,
  .id_table = rpmsg_rnfs_id_table,
  .probe    = rpmsg_rnfs_probe,
  .callback = rpmsg_rnfs_cb,
  .remove   = rpmsg_rnfs_remove,
};

static int __init rpmsg_rnfs_init(void)
{
  return register_rpmsg_driver(&rpmsg_rnfs_driver);
}

static void __exit rpmsg_rnfs_fini(void)
{
  unregister_rpmsg_driver(&rpmsg_rnfs_driver);
}

module_init(rpmsg_rnfs_init);
module_exit(rpmsg_rnfs_fini);

MODULE_DESCRIPTION("RPMSG Read from NFS Server");
