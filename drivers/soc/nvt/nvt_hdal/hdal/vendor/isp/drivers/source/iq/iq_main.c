#include "iq_alg.h"
#include "iqt_api.h"
#include "isp_dev.h"
#include "isp_uti.h"

#include "iq_dbg.h"
#include "iq_main.h"
#include "iq_proc.h"

//=============================================================================
//Module parameter : Set module parameters when insert the module
//=============================================================================
UINT32 iq_id_list = 0x3; // path 1 + path 2
module_param_named(iq_id_list, iq_id_list, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iq_id_list, "List of iq id");
UINT32 iq_dpc_en = 1;
module_param_named(iq_dpc_en, iq_dpc_en, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iq_dpc_en, "En of iq dpc");
UINT32 iq_ecs_en = 1;
module_param_named(iq_ecs_en, iq_ecs_en, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iq_ecs_en, "En of iq ecs");
UINT32 iq_2dlut_en = 1;
module_param_named(iq_2dlut_en, iq_2dlut_en, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iq_2dlut_en, "En of iq 2dlut");
#ifdef DEBUG
UINT32 iq_debug_level = __DBGLVL__;
module_param_named(iq_debug_level, iq_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(iq_debug_level, "Debug message level");
#endif

//=============================================================================
// function declaration
//=============================================================================

//=============================================================================
// global
//=============================================================================
INT32 iq_ioctl(INT32 cmd, UINT32 arg, UINT32 *buf_addr)
{
	INT32 ret = 0;
	UINT32 size;

	size = iqt_api_get_item_size(_IOC_NR(cmd));

	if ((_IOC_DIR(cmd) == _IOC_READ) || (_IOC_DIR(cmd) == (_IOC_READ|_IOC_WRITE))) {
		ret = copy_from_user(buf_addr, (void __user *)arg, size) ? -EFAULT : 0;
		if (ret < 0) {
			DBG_ERR("copy_from_user ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}

		ret = iqt_api_get_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
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

		ret = iqt_api_set_cmd(_IOC_NR(cmd), (UINT32) buf_addr);
		if (ret < 0) {
			DBG_ERR("set command ERROR, (%d %d) \n", _IOC_DIR(cmd), _IOC_NR(cmd));
			return ret;
		}
	} else {
		printk("only support read/write \n");
	}

	return ret;
}

static INT32 __init iq_module_init(void)
{
	iq_init_module(iq_id_list, iq_dpc_en, iq_ecs_en, iq_2dlut_en);

	iq_proc_create();

	return 0;
}

static void __exit iq_module_exit(void)
{
	iq_proc_remove();

	iq_uninit_module();
}

module_init(iq_module_init);
module_exit(iq_module_exit);

MODULE_AUTHOR("Novatek Corp.");
MODULE_DESCRIPTION("iq module");
MODULE_LICENSE("Proprietary");
