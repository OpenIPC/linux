#include "awb_alg.h"
#include "awbt_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "awb_dbg.h"
#include "awb_main.h"
#include "awb_proc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
UINT32 awb_id_list = 0x3; // path 1 + path 2
module_param_named(awb_id_list, awb_id_list, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(awb_id_list, "List of iq id");
#ifdef DEBUG
UINT32 awb_debug_level = __DBGLVL__;
module_param_named(awb_debug_level, awb_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(awb_debug_level, "Debug message level");
#endif

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// global
//=============================================================================
INT32 awb_ioctl(INT32 cmd, UINT32 arg, UINT32 *buf_addr)
{
	INT32 ret = 0;
	UINT32 size;

	size = awbt_api_get_item_size(_IOC_NR(cmd));

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = awbt_api_get_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
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

		ret = awbt_api_set_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("set command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else {
		printk("only support read/write \n");
	}

	return ret;
}

static INT32 __init awb_module_init(void)
{
	awb_init_module(awb_id_list);

	awb_proc_create();

	return 0;
}

static void __exit awb_module_exit(void)
{
	awb_proc_remove();

	awb_uninit_module();
}

module_init(awb_module_init);
module_exit(awb_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("awb module");
MODULE_LICENSE("Proprietary");
