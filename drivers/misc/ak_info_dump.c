#include <linux/module.h>
#include <linux/types.h>
#include <linux/kobject.h>

int aksensor_get_sensor_id(void);
int aksensor_get_sensor_if(char *if_str);
int akmci_get_tf_err_count(void);
void akmci_set_tf_err_count(int count);

static ssize_t reserved_mem_size_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%x\n", CONFIG_VIDEO_RESERVED_MEM_SIZE);
}

static ssize_t sensor_id_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "0x%x\n", aksensor_get_sensor_id());
}

static ssize_t sensor_if_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int sensor_if;
	char if_str[16];

	if_str[0] = '\0';
	sensor_if = aksensor_get_sensor_if(if_str);

	return sprintf(buf, "%s\n", if_str);
}

static ssize_t tf_err_count_read(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d\n", akmci_get_tf_err_count());
}

static ssize_t tf_err_count_write(struct kobject *kobj, struct kobj_attribute *attr,
		 const char *buf, size_t count)
{
	unsigned int tf_err_count = 0;
	sscanf(buf, "%d", &tf_err_count);
	akmci_set_tf_err_count(tf_err_count);
	return count;
}

static struct kobj_attribute reserved_mem_size_obj =__ATTR(reserved_mem_size, S_IRUGO, reserved_mem_size_read, NULL);
static struct kobj_attribute sensor_id_obj =__ATTR(sensor_id, S_IRUGO, sensor_id_read, NULL);
static struct kobj_attribute sensor_if_obj =__ATTR(sensor_if, S_IRUGO, sensor_if_read, NULL);
static struct kobj_attribute tf_err_count_obj =__ATTR(tf_err_count, 0666, tf_err_count_read, tf_err_count_write);

static struct attribute *ak_info_dump_attrs[] = {
	&reserved_mem_size_obj.attr,
	&sensor_id_obj.attr,
	&sensor_if_obj.attr,
	&tf_err_count_obj.attr,
	NULL,
};


static struct attribute_group ak_info_dump_group = {
	.attrs = ak_info_dump_attrs,
};

struct kobject *ak_info_dump_obj = NULL;
int ak_info_dump_init(void)
{
	int ret = 0;
	printk("%s\n", __func__);

	ak_info_dump_obj = kobject_create_and_add("ak_info_dump", NULL);
	if (!ak_info_dump_obj)
		goto err_board_obj;

	ret = sysfs_create_group(ak_info_dump_obj, &ak_info_dump_group);
	if (ret)
		goto err_sysfs_create;

	return 0;

err_sysfs_create:
	//sysfs_remove_group(ak_info_dump_obj, &ak_info_dump_group);
	kobject_put(ak_info_dump_obj);
	printk("\nsysfs_create_group ERROR : %s\n",__func__);
	return 0;
err_board_obj:
	printk("\nobject_create_and_add ERROR : %s\n",__func__);
	return 0;
}

void ak_info_dump_exit(void)
{
	printk("%s\n", __func__);
	sysfs_remove_group(ak_info_dump_obj, &ak_info_dump_group);
	kobject_put(ak_info_dump_obj);
}


MODULE_AUTHOR("ye_guohong");
MODULE_LICENSE("GPL");
module_init(ak_info_dump_init);
module_exit(ak_info_dump_exit);


