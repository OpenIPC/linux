#include "ae_alg.h"
#include "aet_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "ae_dbg.h"
#include "ae_main.h"
#include "ae_proc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
UINT32 ae_id_list = 0x3; // path 1 + path 2
module_param_named(ae_id_list, ae_id_list, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ae_id_list, "List of ae id");
#ifdef DEBUG
UINT32 ae_debug_level = __DBGLVL__;
module_param_named(ae_debug_level, ae_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(ae_debug_level, "Debug message level");
#endif

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// global
//=============================================================================
INT32 ae_ioctl(INT32 cmd, UINT32 arg, UINT32 *buf_addr)
{
	INT32 ret = 0;
	UINT32 size;

	size = aet_api_get_item_size(_IOC_NR(cmd));

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
		ret = aet_api_get_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("get command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = copy_to_user((void __user *)arg, buf_addr, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_to_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = aet_api_set_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("set command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else {
		printk("only support read/write \n");
	}

	return ret;
}

static INT32 __init ae_module_init(void)
{
	ae_init_module(ae_id_list);

	ae_proc_create();

	return 0;
}

static void __exit ae_module_exit(void)
{
	ae_proc_remove();

	ae_uninit_module();
}

module_init(ae_module_init);
module_exit(ae_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("ae module");
MODULE_LICENSE("Proprietary");
