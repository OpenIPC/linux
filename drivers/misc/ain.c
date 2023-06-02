#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <mach-anyka/anyka_types.h>
#include <mach-anyka/fha_asa.h>
#include <mach/map.h>
#include <asm/io.h>
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/string.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/mtd/partitions.h>
#include <linux/gpio.h>
#include <mtd/mtd-abi.h>
#include <plat-anyka/notify.h>
#include <linux/platform_device.h>
#include <linux/proc_fs.h>
#include <mach/adc.h>


#define DBG(fmt...)               //printk(fmt)	
#define EFUSE_CTRL_REG            (AK_VA_SYSCTRL + 0x48)
#define ANALOG_CTRL_REG3          (AK_VA_SYSCTRL + 0x9C)
#define ANALOG_CTRL_REG4          (AK_VA_SYSCTRL + 0xA0)


static ssize_t ain0_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	/*int rTmp = 0x0;*/
	int ain;
	
	/*pull down VP */
	
	//REG32(ANALOG_CTRL_REG3) |= (1 << 3); 
	//REG32(ANALOG_CTRL_REG4) |= ((1 << 0) | (1 << 25));
	#if 0
	msleep(200);                                    //wait for VP pull down, (>150 ms)
	REG32(EFUSE_CTRL_REG) = 0x00000002;             //set read mode		
	REG32(EFUSE_CTRL_REG) |= 0x00000001;            //start to read 
	msleep(15);                                     //wait for read finish ( > 2us ) 
	REG32(EFUSE_CTRL_REG) &= (~0x00000001);         //clear efuse_cfg_rdy bit for next operate
	rTmp = REG32(EFUSE_CTRL_REG); 
	#endif
/* if(((rTmp >> 8)&0xff)< 8)
		DBG("open get ain device failure.:%d\n",rTmp);
   else
	  DBG("open get ain device success:%d.\n",rTmp);
*/
	ain = (int)adc1_read_ain0();
	
	DBG("ad4_value = %d:\n",ain);
  return sprintf(buf, "%d\n", ain);	
}

static ssize_t ain1_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	/*int rTmp = 0x0;*/
	int ain;
	
	/*pull down VP */
	
	//REG32(ANALOG_CTRL_REG3) |= (1 << 3); 
	//REG32(ANALOG_CTRL_REG4) |= ((1 << 0) | (1 << 25));
	#if 0
	msleep(200);                                    //wait for VP pull down, (>150 ms)
	REG32(EFUSE_CTRL_REG) = 0x00000002;             //set read mode		
	REG32(EFUSE_CTRL_REG) |= 0x00000001;            //start to read 
	msleep(15);                                     //wait for read finish ( > 2us ) 
	REG32(EFUSE_CTRL_REG) &= (~0x00000001);         //clear efuse_cfg_rdy bit for next operate
	rTmp = REG32(EFUSE_CTRL_REG); 
	#endif
/* if(((rTmp >> 8)&0xff)< 8)
		DBG("open get ain device failure.:%d\n",rTmp);
   else
	  DBG("open get ain device success:%d.\n",rTmp);
*/
	ain = (int)adc1_read_ain1();
	
	DBG("ad4_value = %d:\n",ain);
  return sprintf(buf, "%d\n", ain);	
}

static ssize_t bat_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	/*int rTmp = 0x0;*/
	int ain;
	
	/*pull down VP */
	
	//REG32(ANALOG_CTRL_REG3) |= (1 << 3); 
	//REG32(ANALOG_CTRL_REG4) |= ((1 << 0) | (1 << 25));
	#if 0
	msleep(200);                                    //wait for VP pull down, (>150 ms)
	REG32(EFUSE_CTRL_REG) = 0x00000002;             //set read mode		
	REG32(EFUSE_CTRL_REG) |= 0x00000001;            //start to read 
	msleep(15);                                     //wait for read finish ( > 2us ) 
	REG32(EFUSE_CTRL_REG) &= (~0x00000001);         //clear efuse_cfg_rdy bit for next operate
	rTmp = REG32(EFUSE_CTRL_REG); 
	#endif
/* if(((rTmp >> 8)&0xff)< 8)
		DBG("open get ain device failure.:%d\n",rTmp);
   else
	  DBG("open get ain device success:%d.\n",rTmp);
*/
	ain = (int)adc1_read_bat();
	
	DBG("ad4_value = %d:\n",ain);
  return sprintf(buf, "%d\n", ain);	
}


static struct kobj_attribute ain0_attribute = 
	__ATTR(ain0, 0666, ain0_show, NULL);

static struct kobj_attribute ain1_attribute = 
	__ATTR(ain1, 0666, ain1_show, NULL);

static struct kobj_attribute bat_attribute = 
	__ATTR(bat, 0666, bat_show, NULL);


static struct attribute *attrain0[] = {
	&ain0_attribute.attr,
	NULL
};
static struct attribute *attrain1[] = {
	&ain1_attribute.attr,
	NULL
};
static struct attribute *attrbat[] = {
	&bat_attribute.attr,
	NULL
};


static struct kobject *ain_kobj;

static int __init ain_init(void)
{
	int ret;

	ain_kobj = kobject_create_and_add("ain", kernel_kobj);
	if (!ain_kobj) {
		printk("Create ain kobject failed\n");
		return -ENOMEM;
	}


	ret = sysfs_create_file(ain_kobj, *attrain0);
	if (ret) {
		printk("Create ain sysfs file failed\n"); 
		kobject_put(ain_kobj);
	}
	ret = sysfs_create_file(ain_kobj, *attrain1);
	if (ret) {
		printk("Create ain sysfs file failed\n"); 
		kobject_put(ain_kobj);
	}	
	ret = sysfs_create_file(ain_kobj, *attrbat);
	if (ret) {
		printk("Create ain sysfs file failed\n"); 
		kobject_put(ain_kobj);
	}	
	return ret;
}

static void __exit ain_exit(void)
{
	kobject_put(ain_kobj);
}

module_init(ain_init);
module_exit(ain_exit);

MODULE_DESCRIPTION("Anyka Device Ain Interface");
MODULE_AUTHOR("Anyka");
MODULE_LICENSE("GPL");
