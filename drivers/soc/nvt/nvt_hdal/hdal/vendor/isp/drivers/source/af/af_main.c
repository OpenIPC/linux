#include "af_alg.h"
#include "aft_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "af_dbg.h"
#include "af_main.h"
#include "af_proc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
#ifdef DEBUG
UINT32 af_debug_level = __DBGLVL__;
module_param_named(af_debug_level, af_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(af_debug_level, "debug message level");
#endif

//=============================================================================
// global
//=============================================================================
INT32 af_ioctl(INT32 cmd, UINT32 arg, UINT32 *buf_addr)
{
	INT32 ret = 0;
	UINT32 size = aft_api_get_item_size(_IOC_NR(cmd));

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ | _IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%u %u)\n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = aft_api_get_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("get command ERROR, (%u %u)\n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = copy_to_user((void __user *)arg, buf_addr, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_to_user ERROR, (%u %u)\n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else if (_IOC_DIR(cmd) == _IOC_WRITE) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%u %u)\n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = aft_api_set_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("set command ERROR, (%u %u)\n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else {
		DBG_ERR("only support read/write!\n");

		return -ENOIOCTLCMD;
	}

	return ret;
}

static INT32 __init af_module_init(void)
{
	af_init_module();

	af_proc_create();

	return 0;
}

static void __exit af_module_exit(void)
{
	af_proc_remove();

	af_uninit_module();
}

module_init(af_module_init);
module_exit(af_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("AF Module");
MODULE_LICENSE("Proprietary");

