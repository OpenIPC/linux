 /**
 *  @file      /driver/mtd/devices/ak_SPIFlash.c
 *  @brief     SPI Flash driver for Anyka AK37 platform.
 *   Copyright C 2012 Anyka CO.,LTD
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *  @author    She Shaohua
 *  @date      2012-03-23
 *  @note      2011-03-20  created
 *  @note      2011-03-23  Debug OK.
 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/mutex.h>
#include <linux/math64.h>
#include <linux/sched.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/partitions.h>
#include <linux/spi/spi.h>
#include <linux/spi/flash.h>
#include <linux/delay.h>
#include <mach-anyka/anyka_types.h>
#include <mach-anyka/partition_init.h> 
#include <mach-anyka/partition_lib.h> 
#include <linux/spinlock.h>
#include <linux/mm.h>

static struct kobject *partition_table_kobj;
spinlock_t	partition_lock;


/**
* @brief	  ak spi flash partition table sys kobject attr show
* 
* ak spi flash partition table sys kobject attr show
* @author SheShaohua
* @date 2012-03-20
* @param[in] kobj: sys kobject	
* @param[in] attr: kobject show
* @param[in] buf: kobject attr parameter show
* @return ssize_t 
* @retval return string len on success
* @retval return zero error code if failed
*/
static ssize_t partition_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned long len = 0;
	unsigned long flags;
	int ret = 0;
	void *handle ;
	T_EX_PARTITION_CONFIG ex_arrt_buf;
	T_PARTITION_TABLE_INFO *phandle = NULL;
	T_BIN_CONFIG ex_binarrt_lenbuf = {0};
	T_FS_CONFIG ex_fsarrt_lenbuf   = {0};
	
	spin_lock_irqsave(&partition_lock, flags);
	
	/* search open the same name partition */
	handle = partition_open((unsigned char *)kobj->name);
	if (handle == NULL) {
		spin_unlock_irqrestore(&partition_lock, flags);
		printk(KERN_ERR "%s, open partition error!\n", __func__);
		return -ENOENT;
	}

	/* force tansmit handle type for get file type */
	phandle = (T_PARTITION_TABLE_INFO *)handle;
	
	/* search the partition ext 16B params */
	ret = partition_get_attr(handle, &ex_arrt_buf);
	if (ret) {
		printk(KERN_ERR "%s, open partition error!\n", __func__);
		goto show_exit;
	}
	
	/* transfer the ext struct format */
	if (phandle->partition_info.type == PART_FS_TYPE) {
		memcpy(&ex_fsarrt_lenbuf, &ex_arrt_buf, sizeof(T_EX_PARTITION_CONFIG));
	}else {
		memcpy(&ex_binarrt_lenbuf, &ex_arrt_buf, sizeof(T_EX_PARTITION_CONFIG));
	}

	/* first step: show partition infor transfer file_length data to char string */
	if (strcmp(attr->attr.name, "type") == 0){
		len = sprintf(buf, "%x:", phandle->partition_info.type);
	}

	if (strcmp(attr->attr.name, "r_w_flag") == 0){
		len = sprintf(buf, "%x:", phandle->partition_info.r_w_flag);
	}

	if (strcmp(attr->attr.name, "hidden_flag") == 0){
		len = sprintf(buf, "%x:", phandle->partition_info.hidden_flag);
	}

	if (strcmp(attr->attr.name, "name") == 0){
		len = sprintf(buf, "%s:", phandle->partition_info.name);
	}

	if (strcmp(attr->attr.name, "ksize") == 0){
		len = sprintf(buf, "%lx:", phandle->partition_info.ksize);
	}

	if (strcmp(attr->attr.name, "start_pos") == 0){
		len = sprintf(buf, "%lx:", phandle->partition_info.start_pos);
	}

	/* second step: show ext partition infor transfer file_length data to char string */
	if (strcmp(attr->attr.name, "file_length") == 0){
		if (phandle->partition_info.type == PART_FS_TYPE) {
			len = sprintf(buf, "%lx:", ex_fsarrt_lenbuf.file_length);
		}else {
			len = sprintf(buf, "%lx:", ex_binarrt_lenbuf.file_length);
		}
	}

	if (strcmp(attr->attr.name, "ld_addr") == 0){
		if (phandle->partition_info.type != PART_FS_TYPE) {
			len = sprintf(buf, "%lx:", ex_binarrt_lenbuf.ld_addr);
		}
	}
	
	if (strcmp(attr->attr.name, "mtd_index") == 0) {
		if (phandle->partition_info.type == PART_FS_TYPE) {
			len = sprintf(buf, "%x:", ex_fsarrt_lenbuf.mtd_idex);
		}else {
			len = sprintf(buf, "%x:", ex_binarrt_lenbuf.mtd_idex);
		}
	}

show_exit:
	/* search open the same name partition */
	ret = partition_close(handle);
	if (ret) {
		spin_unlock_irqrestore(&partition_lock, flags);
		printk(KERN_ERR "%s, close partition error!\n", __func__);
		return -ENOENT;
	}

	spin_unlock_irqrestore(&partition_lock, flags);
	
	
	return len;
}

/**
* @brief	 ak spi flash partition table sys kobject attr store
* 
* ak spi flash partition table sys kobject attr store
* @author SheShaohua
* @date 2012-03-20
* @param[in] kobj: sys kobject	
* @param[in] attr: kobject store
* @param[in] buf: kobject attr parameter store
* @param[in] count: attr parameter store cnt
* @return ssize_t 
* @retval return parameter store cnt on success
* @retval return zero error code if failed
*/
static ssize_t partition_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
	unsigned long file_len =0;
	unsigned long flags;
	int ret = 0;
	void * handle ;
	T_EX_PARTITION_CONFIG ex_arrt_buf;
	T_PARTITION_TABLE_INFO *phandle = NULL;
	T_BIN_CONFIG ex_binarrt_lenbuf;
	T_FS_CONFIG ex_fsarrt_lenbuf;
	
	sscanf(buf, "%lx", &file_len);

	spin_lock_irqsave(&partition_lock, flags);
	
	/* search the same name partition */
	handle = partition_open((unsigned char *)kobj->name);
	if (handle == NULL) {
		spin_unlock_irqrestore(&partition_lock, flags);
		printk(KERN_ERR "%s, open partition error!\n", __func__);
		return -ENOENT;
	}

	phandle = (T_PARTITION_TABLE_INFO *)handle;
	
	/* search the partition ext 16B params */
	ret = partition_get_attr(handle, &ex_arrt_buf);
	if (ret) {
		printk(KERN_ERR "%s, get attr error!\n", __func__);
		goto store_exit;
	}

	/* transfer the ext struct format */
	if (phandle->partition_info.type == PART_FS_TYPE) {
		memcpy(&ex_fsarrt_lenbuf, &ex_arrt_buf, sizeof(T_EX_PARTITION_CONFIG));
	}else {
		memcpy(&ex_binarrt_lenbuf, &ex_arrt_buf, sizeof(T_EX_PARTITION_CONFIG));
	}

	/* transfer file_length data to char string */
	if (strcmp(attr->attr.name, "file_length") == 0){
		printk("cdh:%s, store attr.name:%s!\n", __func__, attr->attr.name);
		if (phandle->partition_info.type == PART_FS_TYPE) {
			ex_fsarrt_lenbuf.file_length = file_len;
			memcpy(&ex_arrt_buf, &ex_fsarrt_lenbuf, sizeof(T_EX_PARTITION_CONFIG));
		}else {
			ex_binarrt_lenbuf.file_length = file_len;
			memcpy(&ex_arrt_buf, &ex_binarrt_lenbuf, sizeof(T_EX_PARTITION_CONFIG));
		}
	}


	/* search the partition ext 16B params */
	ret = partition_set_attr(handle, &ex_arrt_buf);
	if (ret) {
		printk(KERN_ERR "%s, set attr error!\n", __func__);
		goto store_exit;
	}

store_exit:
	/* search open the same name partition */
	ret = partition_close(handle);
	if (ret) {
		spin_unlock_irqrestore(&partition_lock, flags);
		printk(KERN_ERR "%s, close partition error!\n", __func__);
		return -ENOENT;
	}

	spin_unlock_irqrestore(&partition_lock, flags);
	
	return count;
}


static struct kobj_attribute type_attribute = 
	__ATTR(type, 0666, partition_show, NULL);

static struct kobj_attribute r_w_flag_attribute = 
	__ATTR(r_w_flag, 0666, partition_show, NULL);

static struct kobj_attribute hidden_flag_attribute = 
	__ATTR(hidden_flag, 0666, partition_show, NULL);

static struct kobj_attribute name_attribute = 
	__ATTR(name, 0666, partition_show, NULL);

static struct kobj_attribute ksize_attribute = 
	__ATTR(ksize, 0666, partition_show, NULL);

static struct kobj_attribute start_pos_attribute = 
	__ATTR(start_pos, 0666, partition_show, NULL);

static struct kobj_attribute file_length_attribute = 
	__ATTR(file_length, 0666, partition_show, partition_store);

static struct kobj_attribute ld_addr_attribute = 
	__ATTR(ld_addr, 0666, partition_show, NULL);

static struct kobj_attribute mtd_index_attribute = 
	__ATTR(mtd_index, 0666, partition_show, NULL);


static struct attribute *partition_attribute[] = {
	&type_attribute.attr,
	&r_w_flag_attribute.attr,
	&hidden_flag_attribute.attr,
	&name_attribute.attr,
	&ksize_attribute.attr,
	&start_pos_attribute.attr,
	NULL
};

static struct attribute *binfile_attribute[] = {
	&file_length_attribute.attr,
	&ld_addr_attribute.attr,
	&mtd_index_attribute.attr,
	NULL
};

static struct attribute *filefs_attribute[] = {
	&file_length_attribute.attr,
	&mtd_index_attribute.attr,
	NULL
};

static struct attribute_group partition_attr_group = {
	.attrs = partition_attribute,
};

static struct attribute_group binfile_attr_group = {
	.attrs = binfile_attribute,
};

static struct attribute_group filefs_attr_group = {
	.attrs = filefs_attribute,
};


/**
* @brief	  ak spi flash partition table sys kobject create
* 
* create ak spi flash partition table sys kobject for supply interface to app aplication
* @author SheShaohua
* @date 2012-03-20
* @param[in] part_tab: partition tab buffer pointer	   
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
int ak_partition_table_sys_create(T_PARTITION_TABLE_CONFIG *part_tab)
{
	int i;
	int ret = 0;
	int error;
	unsigned long nr_parts;
	T_PARTITION_TABLE_INFO *parts = NULL;
	struct kobject *tmp_kobj;
	unsigned char part_name[PARTITION_NAME_LEN + 1];
	
	nr_parts = *(unsigned long *)part_tab->table;
	
	/* 
	* if no partiton to mount, the buf will be all 0xFF but not constant.
	*  So, it is not safe here. 
	*/
	// printk("nr_parts=0x%lx\n", nr_parts);
	if (nr_parts <= 0 || nr_parts > 15) {
		printk(KERN_ERR "partition count invalid\n");
		ret = -EINVAL;
		goto err_out1;
	}

	parts = (T_PARTITION_TABLE_INFO *)(&part_tab->table[sizeof(unsigned long)]);
	for (i=0; i<nr_parts; i++) {
		 memset(part_name, 0, 7);
		 strncpy(part_name, parts[i].partition_info.name, 6);
		 //printk("mtd_part[%d]:\nname = %s\n", i, part_name);
		 
		/* create sysfs partition table */
		tmp_kobj = kobject_create_and_add(part_name, partition_table_kobj);
		if (!tmp_kobj) {
			ret = -EINVAL;
			printk(KERN_ERR "Create %s kobject error!\n", part_name);
			goto err_out1;
		}else {
			/* create sysfs partition table */
			error = sysfs_create_group(tmp_kobj, &partition_attr_group);
			if (error) {
				ret = -EINVAL;
				printk(KERN_ERR "Create %s kobject attr group error!\n", part_name);
				goto err_out2;
			}

			/* create sysfs partition table */
			if ((parts[i].partition_info.type == PART_DATA_TYPE) || (parts[i].partition_info.type == PART_BIN_TYPE)) {
				error = sysfs_create_group(tmp_kobj, &binfile_attr_group);
				if (error) {
					ret = -EINVAL;
					printk(KERN_ERR "Create %s kobject binfile attr group error!\n", parts[i].partition_info.name);
					goto err_out2;
				}
			}else {
				error = sysfs_create_group(tmp_kobj, &filefs_attr_group);
				if (error) {
					ret = -EINVAL;
					printk(KERN_ERR "Create %s kobject filefs attr group error!\n", part_name);
					goto err_out2;
				}
			}	
		}
		
	}

err_out1:
	return ret;

err_out2:
	kobject_put(partition_table_kobj);
	return ret;
}

EXPORT_SYMBOL(ak_partition_table_sys_create);


/**
* @brief	  ak spi flash partition table module init
* 
* ak spi flash partition table module init
* @author SheShaohua
* @date 2012-03-20
* @param[in] void	   
* @return int return write success or failed
* @retval returns zero on success
* @retval return a non-zero error code if failed
*/
static int __init ak_partition_table_init(void)
{
    printk("Start to init Anyka partition table...\n");

	spin_lock_init(&partition_lock);
    partition_table_kobj = kobject_create_and_add("partition_table", kernel_kobj);
	if (!partition_table_kobj) {
		printk("Create partition table kobject failed\n");
		return -ENOMEM;
	}
	
	return 0;
}


/**
* @brief	  ak spi flash partition table module exit
* 
* ak spi flash partition table module exit
* @author SheShaohua
* @date 2012-03-20
* @param[in] void	   
* @return void
* @retval none
*/
static void __exit ak_partition_table_exit(void)
{
	kobject_put(partition_table_kobj);
}


module_init(ak_partition_table_init);
module_exit(ak_partition_table_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Cao Donghua");
MODULE_DESCRIPTION("MTD Partition Table for Anyka spiflash chips");
