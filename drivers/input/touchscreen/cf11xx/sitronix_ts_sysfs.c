#include <linux/ctype.h>
#include "sitronix_ts_custom_func.h"
#include "sitronix_ts.h"

char stinfbuf[16];

static ssize_t st_inform_show(struct device *dev, struct device_attribute *attr,
			      char *buf)
{
	ssize_t sret;
	mutex_lock(&stx_gpts.dev_mutex);
	st_get_touch_info(&stx_gpts);
	mutex_unlock(&stx_gpts.dev_mutex);
	sret = snprintf(buf, 1000,
			"Sitronix touch\n"
			"FW Version = %d\n"
			"FW Revision = %s\n"
			"Channels = %d x %d\n"
			"Max touches = %d\n"
			"res = %d x %d\n"
			"chip = 0x%x\n",
			stx_gpts.ts_dev_info.fw_version[0],
			stx_gpts.ts_dev_info.fw_revision,
			stx_gpts.ts_dev_info.rx_chs,
			stx_gpts.ts_dev_info.tx_chs,
			stx_gpts.ts_dev_info.max_touches,
			stx_gpts.ts_dev_info.x_res, stx_gpts.ts_dev_info.y_res,
			stx_gpts.ts_dev_info.chip_id);
	return sret;
}

static ssize_t st_inform_store(struct device *dev,
			       struct device_attribute *attr, const char *buf,
			       size_t count)
{
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);

	memset(stinfbuf, 0, 16);
	memcpy(stinfbuf, buf, count);
	STX_INFO("%s inbuffer:%s buf:%s ", __func__, stinfbuf, buf);
	mutex_unlock(&stx_gpts.dev_mutex);
	return count;
}

static ssize_t st_glove_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	ssize_t sret;
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);

	/*add function*/
#ifdef ST_GLOVE_SWITCH_MODE
	sret = snprintf(buf, 100, "Sitronix glove_mode = %s\n",
			stx_gpts.glove_mode ? "on" : "off");
#else
	sret = snprintf(buf, 100, "Don't support ST_GLOVE_SWITCH_MODE mode\n");
#endif

	mutex_unlock(&stx_gpts.dev_mutex);
	return sret;
}

static ssize_t st_glove_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);
#ifdef ST_GLOVE_SWITCH_MODE
	if (buf[0] == '1')
		st_enter_glove_mode(&stx_gpts);
	if (buf[0] == '0')
		st_leave_glove_mode(&stx_gpts);
#endif
	/*add function*/
	mutex_unlock(&stx_gpts.dev_mutex);

	return count;
}

static ssize_t st_cases_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	ssize_t sret;
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);

	/*add function*/
#ifdef ST_CASES_SWITCH_MODE
	sret = snprintf(buf, 100, "Sitronix cases_mode = %s\n",
			stx_gpts.cases_mode ? "on" : "off");
#else
	sret = snprintf(buf, 100, "Don't support ST_CASES_SWITCH_MODE mode\n");
#endif

	mutex_unlock(&stx_gpts.dev_mutex);
	return sret;
}

static ssize_t st_cases_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);
#ifdef ST_CASES_SWITCH_MODE
	if (buf[0] == '1')
		stx_gpts.cases_mode = true;
	if (buf[0] == '0')
		stx_gpts.cases_mode = false;
#endif
	/*add function*/
	mutex_unlock(&stx_gpts.dev_mutex);

	return count;
}

static ssize_t st_rawtest_show(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	ssize_t sret;
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);
	/*add function*/
#ifdef ST_TEST_RAW
	sret = snprintf(
		buf, 200,
		"Sitronix Raw Test Result = %d ( 0 == success, >0 failed sensor number, < 0 err) \n",
		stx_gpts.rawTestResult);
#else
	sret = snprintf(buf, 100, "Don't support ST_TEST_RAW mode\n");
#endif

	mutex_unlock(&stx_gpts.dev_mutex);
	return sret;
}

static ssize_t st_rawtest_store(struct device *dev,
				struct device_attribute *attr, const char *buf,
				size_t count)
{
#ifdef ST_TEST_RAW
	int status = 0;
#endif
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);
#ifdef ST_TEST_RAW
	status = st_testraw_invoke();
	stx_gpts.rawTestResult = status;
#endif
	mutex_unlock(&stx_gpts.dev_mutex);
	return count;
}

static int st_char2hex(char data)
{
	int num = 0;

	if ((data >= '0') && (data <= '9')) {
		num = data - '0';
	}
	if ((data >= 'a') && (data <= 'f')) {
		num = data - 'a' + 10;
	}
	if ((data >= 'A') && (data <= 'F')) {
		num = data - 'A' + 10;
	}
	return num;
}

u8 st_reg_data = 0;

static ssize_t st_rwreg_show(struct device *dev, struct device_attribute *attr,
			     char *buf)
{
	ssize_t num_read_chars;
	STX_DEBUG("%s", __func__);

	mutex_lock(&stx_gpts.dev_mutex);

	num_read_chars = snprintf(buf, 128, "0x%x\n", st_reg_data);

	mutex_unlock(&stx_gpts.dev_mutex);

	STX_INFO("%s num_read_chars:%zu buf:%s", __func__, num_read_chars, buf);

	return num_read_chars;
}

static ssize_t st_rwreg_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	ssize_t num_chars = 0;
	int reg_addr;
	int reg_data;
	char buffer[2] = { 0 };

	STX_INFO("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);

	STX_INFO("%s count:%zu buf:%s ", __func__, count, buf);
	num_chars = count - 1;
	if (num_chars == 2) {
		reg_addr = st_char2hex(buf[0]) * 0x10 + st_char2hex(buf[1]);
		STX_INFO("%s reg_addr:0x%x", __func__, reg_addr);
		/*read register and save data to reg_data*/
		stx_i2c_read(stx_gpts.client, &st_reg_data, 1, reg_addr);
	}
	if (num_chars == 4) {
		reg_addr = st_char2hex(buf[0]) * 0x10 + st_char2hex(buf[1]);
		reg_data = st_char2hex(buf[2]) * 0x10 + st_char2hex(buf[3]);
		STX_INFO("%s reg_addr:0x%x reg_data:0x%x", __func__, reg_addr,
			 reg_data);
		/*write register*/
		buffer[0] = reg_addr;
		buffer[1] = reg_data;
		stx_i2c_write(stx_gpts.client, buffer, 2);
	}
	mutex_unlock(&stx_gpts.dev_mutex);

	return count;
}

static ssize_t st_mt_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	ssize_t sret = 0;
	STX_DEBUG("%s", __func__);

	mutex_lock(&stx_gpts.dev_mutex);

	/*add function*/

	mutex_unlock(&stx_gpts.dev_mutex);

	return sret;
}

static ssize_t st_mt_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);

	/*add function*/
#ifdef ST_MONITOR_THREAD
	if (buf[0] == '1')
		sitronix_monitor_start();
	if (buf[0] == '0')
		sitronix_monitor_stop();
#endif

	mutex_unlock(&stx_gpts.dev_mutex);
	return count;
}

static ssize_t st_forceUpg_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	char spath[40] = { 0 };
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);

	sprintf(spath, "%s", buf);
	stx_force_upgrade_fw(spath);

	st_print_version(&stx_gpts);

	mutex_unlock(&stx_gpts.dev_mutex);
	return count;
}

static ssize_t st_manual_rst_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	STX_DEBUG("%s", __func__);

	mutex_lock(&stx_gpts.dev_mutex);
	if (buf[0] == '1') {
		st_reset_ic();
		STX_INFO("TP has manual reset");
	}
	mutex_unlock(&stx_gpts.dev_mutex);

	return count;
}

static ssize_t st_smart_wakeup_switch_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t count)
{
	STX_DEBUG("%s", __func__);
	mutex_lock(&stx_gpts.dev_mutex);

	if (buf[0] == '1')
		stx_gpts.fsmart_wakeup = 1;
	else if (buf[0] == '0')
		stx_gpts.fsmart_wakeup = 0;
	STX_INFO("fsmart_wakeup = %d", stx_gpts.fsmart_wakeup);

	mutex_unlock(&stx_gpts.dev_mutex);

	return count;
}

static void StrToHex(char *pbDest, const char *pbSrc, int nLen)
{
	char h1, h2;
	char s1, s2;
	int i;

	for (i = 0; i < nLen / 2; i++) {
		h1 = pbSrc[2 * i];
		h2 = pbSrc[2 * i + 1];

		s1 = toupper(h1) - 0x30;
		if (s1 > 9)
			s1 -= 7;
		s2 = toupper(h2) - 0x30;
		if (s2 > 9)
			s2 -= 7;

		pbDest[i] = s1 * 16 + s2;
	}
}

static ssize_t st_temporary_set_i2caddr_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	char addr;
	STX_DEBUG("%s", __func__);

	mutex_lock(&stx_gpts.dev_mutex);

	StrToHex(&addr, buf, 2);
	stx_gpts.client->addr = addr;
	STX_INFO("i2caddr = 0x%x ", addr);

	mutex_unlock(&stx_gpts.dev_mutex);

	return count;
}

/****************************************/
/* sysfs */
/**获取tp信息**/
static DEVICE_ATTR(stinform, S_IRUGO | S_IWUSR, st_inform_show,
		   st_inform_store);

/**手套模式：1 ，正常模式：0**/
static DEVICE_ATTR(stglove, S_IRUGO | S_IWUSR, st_glove_show, st_glove_store);

static DEVICE_ATTR(stcases, S_IRUGO | S_IWUSR, st_cases_show, st_cases_store);

/**皮套模式：1 ，正常模式：0**/
static DEVICE_ATTR(strawtest, S_IRUGO | S_IWUSR, st_rawtest_show,
		   st_rawtest_store);

/*read and write register
*read example: echo 88 > strwreg ---read register 0x88
*write example:echo 8807 > strwreg ---write 0x07 into register 0x88
*note:the number of input must be 2 or 4.if it not enough,please fill in the 0.
*/
static DEVICE_ATTR(strwreg, S_IRUGO | S_IWUSR, st_rwreg_show, st_rwreg_store);

static DEVICE_ATTR(stmt, S_IRUGO | S_IWUSR, st_mt_show, st_mt_store);

/**升级touchscreen 固件程序    固件默认存放在request_fw 空间**/
static DEVICE_ATTR(stupgrade, S_IRUGO | S_IWUSR, NULL, st_forceUpg_store);

/**手动拉节点RST,尝试TP 恢复 **/
static DEVICE_ATTR(streset, S_IRUGO | S_IWUSR, NULL, st_manual_rst_store);

/*开关智能唤醒设备节点，上层可通过此节点控制
 *echo 1 > st_smtwakeup  打开
 *echo 0 > st_smtwakeup  关闭*/
static DEVICE_ATTR(st_smtwakeup, S_IRUGO | S_IWUSR, NULL,
		   st_smart_wakeup_switch_store);

static DEVICE_ATTR(st_i2caddr_set, S_IRUGO | S_IWUSR, NULL,
		   st_temporary_set_i2caddr_store);
static struct attribute *st_attributes[] = { &dev_attr_stinform.attr,
					     &dev_attr_stglove.attr,
					     &dev_attr_stcases.attr,
					     &dev_attr_strawtest.attr,
					     &dev_attr_strwreg.attr,
					     &dev_attr_stmt.attr,
					     &dev_attr_stupgrade.attr,
					     &dev_attr_streset.attr,
					     &dev_attr_st_smtwakeup.attr,
					     &dev_attr_st_i2caddr_set.attr,
					     NULL };

static struct attribute_group st_attribute_group = { .attrs = st_attributes

};

int st_create_sysfs(struct i2c_client *client)
{
	int err;
	//kobject_creat_and_add
	err = sysfs_create_group(&client->dev.kobj, &st_attribute_group);
	if (0 != err) {
		STX_ERROR("%s() - ERROR: sysfs_create_group() failed.",
			  __func__);
		sysfs_remove_group(&client->dev.kobj, &st_attribute_group);
		return -EIO;
	} else {
		STX_INFO("sysfs_create_group() ok.");
	}
	return err;
}

int st_remove_sysfs(struct i2c_client *client)
{
	sysfs_remove_group(&client->dev.kobj, &st_attribute_group);
	return 0;
}

#ifdef ST_DEVICE_NODE

#define SITRONIX_I2C_TOUCH_DEV_NAME "sitronixDev"
static struct cdev sitronix_cdev;
static struct class *sitronix_class;
static int sitronix_major = 0;

int sitronix_open(struct inode *inode, struct file *filp)
{
	STX_INFO("Enter stx OPEN");
	stx_gpts.fapp_in = 1;
#ifdef ST_MONITOR_THREAD
	sitronix_monitor_stop();
#endif
	return 0;
}
EXPORT_SYMBOL(sitronix_open);

int sitronix_release(struct inode *inode, struct file *filp)
{
	STX_INFO("Enter stx RELEASE");
	stx_gpts.fapp_in = 0;
#ifdef ST_MONITOR_THREAD
	sitronix_monitor_start();
#endif
	return 0;
}
EXPORT_SYMBOL(sitronix_release);

ssize_t sitronix_write(struct file *file, const char *buf, size_t count,
		       loff_t *ppos)
{
	int ret;
	char *tmp;

	if (!(stx_gpts.client))
		return -EIO;

	if (count > 8192)
		count = 8192;

	tmp = (char *)kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;
	if (copy_from_user(tmp, buf, count)) {
		kfree(tmp);
		return -EFAULT;
	}
	STX_INFO("writing %zu bytes.", count);

	ret = i2c_master_send(stx_gpts.client, tmp, count);
	kfree(tmp);
	return ret;
}
EXPORT_SYMBOL(sitronix_write);

ssize_t sitronix_read(struct file *file, char *buf, size_t count, loff_t *ppos)
{
	char *tmp;
	int ret;

	if (!(stx_gpts.client))
		return -EIO;

	if (count > 8192)
		count = 8192;

	tmp = (char *)kmalloc(count, GFP_KERNEL);
	if (tmp == NULL)
		return -ENOMEM;

	STX_INFO("reading %zu bytes.", count);

	ret = i2c_master_recv(stx_gpts.client, tmp, count);
	if (ret >= 0)
		ret = copy_to_user(buf, tmp, count) ? -EFAULT : ret;
	kfree(tmp);
	return ret;
}
EXPORT_SYMBOL(sitronix_read);

long sitronix_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	//int err = 0;
	int retval = 0;
	//uint8_t temp[4];

	if (!(stx_gpts.client))
		return -EIO;

	return retval;
}
EXPORT_SYMBOL(sitronix_ioctl);

static struct file_operations nc_fops = {
	.owner = THIS_MODULE,
	.write = sitronix_write,
	.read = sitronix_read,
	.open = sitronix_open,
	.unlocked_ioctl = sitronix_ioctl,
	.release = sitronix_release,
};

int st_dev_node_init(void)
{
	int result;
	int err = 0;
	dev_t devno = MKDEV(sitronix_major, 0);

	result = alloc_chrdev_region(&devno, 0, 1, SITRONIX_I2C_TOUCH_DEV_NAME);
	if (result < 0) {
		STX_ERROR("fail to allocate chrdev (%d) ", result);
		return result;
	}
	sitronix_major = MAJOR(devno);
	cdev_init(&sitronix_cdev, &nc_fops);
	sitronix_cdev.owner = THIS_MODULE;
	sitronix_cdev.ops = &nc_fops;

	err = cdev_add(&sitronix_cdev, devno, 1);
	if (err) {
		STX_ERROR("fail to add cdev (%d)", err);
		return err;
	}
	STX_INFO("%s,%d", __FUNCTION__, __LINE__);

	sitronix_class = class_create(THIS_MODULE, SITRONIX_I2C_TOUCH_DEV_NAME);
	if (IS_ERR(sitronix_class)) {
		result = PTR_ERR(sitronix_class);
		unregister_chrdev(sitronix_major, SITRONIX_I2C_TOUCH_DEV_NAME);
		STX_ERROR("fail to create class (%d)", result);
		return result;
	}
	STX_DEBUG("%s,%d", __FUNCTION__, __LINE__);
	device_create(sitronix_class, NULL, MKDEV(sitronix_major, 0), NULL,
		      SITRONIX_I2C_TOUCH_DEV_NAME);

	return 0;
}

void st_dev_node_exit(void)
{
	dev_t dev_id = MKDEV(sitronix_major, 0);
	cdev_del(&sitronix_cdev);
	device_destroy(sitronix_class, dev_id); //delete device node under /dev
	class_destroy(sitronix_class); //delete class created by us
	unregister_chrdev_region(dev_id, 1);
}
#endif
