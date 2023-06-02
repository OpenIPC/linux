/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/of.h>

#define __MODULE__    rtos_dev
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/dev.h>
#include <kwrap/perf.h>

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define VOS_DEV_BASE_MINOR      0

#define VOS_DTS_NAME_DEVNUM     "hd_devnum"
#define VOS_DTS_PATH_MINOR      "/hd_devnum/minor"
#define VOS_DTS_MAX_ELEMENT     16
#define VOS_DTS_DEBUG           0 //0: disable, 1: enable
/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_dev_debug_level = NVT_DBG_WRN;

static dev_t g_devid = 0;
static unsigned int g_cur_major = 0;
static unsigned int g_next_minor = VOS_DEV_BASE_MINOR;
static struct semaphore g_dev_sem = __SEMAPHORE_INITIALIZER(g_dev_sem, 1);

module_param_named(rtos_dev_debug_level, rtos_dev_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rtos_dev_debug_level, "Debug message level");

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void vos_dev_init(void *param)
{
}

void vos_dev_exit(void)
{
}

static void vos_dev_lock(void)
{
	down(&g_dev_sem);
}


static void vos_dev_unlock(void)
{
	up(&g_dev_sem);
}

static void vos_dts_dump_array(const char* name, u32 *p_array, int elem_cnt)
{
	int elem_idx;

	DBG_WRN("[%s] data:\r\n", name);

	for (elem_idx = 0; elem_idx < elem_cnt; elem_idx++) {
		DBG_WRN("%d\r\n", p_array[elem_idx]);
	}
}

static int vos_dts_reg_all(int *p_major, int *p_max_minor)
{
	struct device_node *node_devnum = NULL, *node_minor = NULL;
	struct property *prop_minor;
	u32 major = 0, max_minor = VOS_DEV_BASE_MINOR;
	u32 minor_array[VOS_DTS_MAX_ELEMENT] = {0};
	int elem_idx;
	int elem_cnt = 0;

	node_devnum = of_find_node_by_name(NULL, VOS_DTS_NAME_DEVNUM);
	if (NULL == node_devnum) {
		DBG_IND("node %s not found\r\n", VOS_DTS_NAME_DEVNUM);
		return -1;
	}

	if (0 != of_property_read_u32(node_devnum, "major", &major)) {
		//if dts node is defined, but no major value.
		//there must be something wrong
		DBG_ERR("prop /%s:major not found\r\n", VOS_DTS_NAME_DEVNUM);
		of_node_put(node_devnum);
		return -1;
	}

	node_minor = of_find_node_by_name(node_devnum, "minor");
	if (NULL == node_minor) {
		//only major, no minor
		goto vos_dev_reg_all_dts_exit;
	}

	for_each_property_of_node(node_minor, prop_minor) {
		if (!strcmp(prop_minor->name, "name")) {
			//the last property name would be "name", key word "unflatten"
			//just skip this property to the next, in case it is not the last
			continue;
		}

		//calc elem_cnt and make sure the buffer size is enough
		elem_cnt = prop_minor->length / sizeof(u32);
		if (elem_cnt > VOS_DTS_MAX_ELEMENT) {
			DBG_ERR("%s elem_cnt %d > max %d\r\n", prop_minor->name, elem_cnt, VOS_DTS_MAX_ELEMENT);
			continue;
		}

		//read minor property
		if (0 != of_property_read_u32_array(node_minor, prop_minor->name, minor_array, elem_cnt)) {
			DBG_ERR("%s read array failed\r\n", prop_minor->name);
			continue;
		}

		//check values of the property is continuous
		for (elem_idx = 1; elem_idx < elem_cnt; elem_idx++) {
			if ((minor_array[elem_idx - 1] + 1) != minor_array[elem_idx]) {
				DBG_ERR("%s not continuous\r\n", prop_minor->name);
				vos_dts_dump_array(prop_minor->name, minor_array, elem_cnt);
				continue;
			}
		}

		//register dev to system
		if (0 != register_chrdev_region(MKDEV(major, minor_array[0]), elem_cnt, prop_minor->name)) {
			DBG_ERR("register_chrdev_region failed, %s %d %d %d\r\n",
				prop_minor->name, major, minor_array[0], elem_cnt);
			continue;
		}

		DBG_IND("[%s] major %d minor %d count %d\r\n", prop_minor->name, major, minor_array[0], elem_cnt);

		//update last_minor for return
		if (minor_array[elem_cnt - 1] > max_minor) {
			max_minor = minor_array[elem_cnt - 1];
		}
	}
	of_node_put(node_minor);

vos_dev_reg_all_dts_exit:
	*p_major = major;
	*p_max_minor = max_minor;
	return 0;
}

static int vos_dts_get_minor(const char *name, int *p_minor, int *p_count)
{
	static struct device_node *node_minor = NULL;
	//NOTE: use static here to reduce process time
	//Finding a node from the beginning will take about 5x us

	struct property *prop_minor;
	u32 major = 0;
	u32 minor_array[VOS_DTS_MAX_ELEMENT] = {0};
	int elem_idx;
	int elem_cnt = 0;

	if (NULL == node_minor) {
		node_minor = of_find_node_by_path(VOS_DTS_PATH_MINOR);
		if (NULL == node_minor) {
			DBG_IND("node %s not found\r\n", VOS_DTS_PATH_MINOR);
			return -1;
		}
	}

	for_each_property_of_node(node_minor, prop_minor) {
		if (0 == strcmp(name, prop_minor->name)) {
			//the property name of minor is found
			elem_cnt = prop_minor->length / sizeof(u32);
			break;
		}
	}

	if (elem_cnt <= 0) {
		//not found
		//of_node_put(node_minor);
		return -1;
	}

	if (elem_cnt > VOS_DTS_MAX_ELEMENT) {
		DBG_ERR("%s elem_cnt %d > max %d\r\n", prop_minor->name, elem_cnt, VOS_DTS_MAX_ELEMENT);
		//of_node_put(node_minor);
		return -1;
	}

	if (0 != of_property_read_u32_array(node_minor, prop_minor->name, minor_array, elem_cnt)) {
		DBG_ERR("%s read array failed\r\n", prop_minor->name);
		//of_node_put(node_minor);
		return -1;
	}

	//error check
	for (elem_idx = 1; elem_idx < elem_cnt; elem_idx++) {
		if ((minor_array[elem_idx - 1] + 1) != minor_array[elem_idx]) {
			DBG_ERR("%s not continuous\r\n", prop_minor->name);
			vos_dts_dump_array(prop_minor->name, minor_array, elem_cnt);
			//of_node_put(node_minor);
			return -1;
		}
	}

	//of_node_put(node_minor);

	//success here
	DBG_IND("[%s] major %d minor %d count %d\r\n", prop_minor->name, major, minor_array[0], elem_cnt);

	if (p_minor) {
		*p_minor = minor_array[0];
	}
	if (p_count) {
		*p_count = elem_cnt;
	}

	return 0;
}

int vos_alloc_chrdev_region(dev_t *ret_dev, unsigned count, const char *name)
{
	unsigned old_next_minor;
	int ret = 0;
	int dts_major, dts_minor, dts_max_minor, dts_count;

	vos_dev_lock();

	old_next_minor = g_next_minor;

	//try to reg all nodes once. if succeeded, update global major and minor
	if (0 == g_cur_major) {
		if (0 == vos_dts_reg_all(&dts_major, &dts_max_minor)) {
			g_cur_major = dts_major;
			g_next_minor = dts_max_minor + 1;
		}
	}

	//try to get from dts
	if (0 == vos_dts_get_minor(name, &dts_minor, &dts_count)) {
		if (dts_count == count) {
			*ret_dev = MKDEV(g_cur_major, dts_minor);
			ret = 0;
		} else {
			DBG_ERR("%s dts_count(%d) != count(%d)\r\n", name, dts_count, count);
			ret = -1;
		}
		goto vos_alloc_chrdev_region_end;
	}

	//normal path (if no dts)
	if (0 == g_cur_major) {
		//get a new major and minors
		ret = alloc_chrdev_region(&g_devid, VOS_DEV_BASE_MINOR, count, name);
		if (0 != ret) {
			DBG_ERR("alloc_chrdev_region vos_cdev failed\r\n");
			goto vos_alloc_chrdev_region_end;
		}
		g_cur_major = MAJOR(g_devid);
	} else {
		//use the old major and get minors
		ret = register_chrdev_region(MKDEV(g_cur_major, g_next_minor), count, name);
		if (0 != ret) {
			DBG_ERR("register_chrdev_region failed\r\n");
			goto vos_alloc_chrdev_region_end;
		}
	}

	*ret_dev = MKDEV(g_cur_major, g_next_minor);
	g_next_minor += count;
	DBG_IND("vos_alloc_chrdev_region MAJOR %d, MINOR %d ~ %d, %s\r\n", g_cur_major, old_next_minor, g_next_minor - 1, name);

vos_alloc_chrdev_region_end:
	vos_dev_unlock();
	return ret;
}

void vos_unregister_chrdev_region(dev_t from, unsigned count)
{
	if (MAJOR(from) != g_cur_major) {
		DBG_ERR("invalid MAJOR(from) %d, cur %d\r\n", MAJOR(from), g_cur_major);
		return;
	}
	unregister_chrdev_region(from, count);
}

struct cdev* vos_icdev(const struct inode *inode)
{
	return inode->i_cdev;
}

dev_t vos_irdev(const struct inode *inode)
{
	return inode->i_rdev;
}

EXPORT_SYMBOL(vos_alloc_chrdev_region);
EXPORT_SYMBOL(vos_unregister_chrdev_region);
EXPORT_SYMBOL(vos_icdev);
EXPORT_SYMBOL(vos_irdev);
