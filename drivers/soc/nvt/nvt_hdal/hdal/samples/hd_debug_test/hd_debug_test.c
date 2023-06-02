/**
        @brief Sample code of hd_logger and hd_debug.\n

        @file hd_debug_test.c

        @author Niven Cho

        @ingroup mhdal

        @note Nothing.

        Copyright Novatek Microelectronics Corp. 2018.  All rights reserved.
*/
#include <hd_logger.h>
#include <hd_debug.h>
#include <stdarg.h>
#include <string.h>
#include <kwrap/examsys.h>
#include <hdal.h>

#define HD_MODULE_NAME HD_AUDIOCAPTURE
#define DBG_ERR(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _ERR)("\033[1;31m" fmtstr "\033[0m", ##args)
#define DBG_WRN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _WRN)("\033[1;33m" fmtstr "\033[0m", ##args)
#define DBG_IND(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _IND)(fmtstr, ##args)
#define DBG_DUMP(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _MSG)(fmtstr, ##args)
#define DBG_FUNC_BEGIN(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("BEGIN: " fmtstr, ##args)
#define DBG_FUNC_END(fmtstr, args...) HD_LOG_BIND(HD_MODULE_NAME, _FUNC)("END: " fmtstr, ##args)

EXAMFUNC_ENTRY(hd_debug_test, argc, argv)
{
	//test debug message
	if (hd_common_init(0) != HD_OK) {
		return 0;
	}
	DBG_FUNC_BEGIN("with init val=%d\n", 0);
	DBG_ERR("this is error message, er=%d\n", -1);
	DBG_WRN("this is warning message, wr=%d\n", -2);
	DBG_IND("[class1] this is debug message, got val=%d\n", 3);
	DBG_IND("[class2] this is debug message, got val=%d\n", 3);
	DBG_DUMP("this is normal message without function name, module name and line number.\n");
	DBG_FUNC_END("with end val=%d\n", 99);

	//run debug menu
	hd_debug_run_menu();

	//show message after debug menu disable some message
	DBG_FUNC_BEGIN("with init val=%d\n", 0);
	DBG_ERR("this is error message, er=%d\n", -1);
	DBG_WRN("this is warning message, wr=%d\n", -2);
	DBG_IND("[class1] this is debug message, got val=%d\n", 3);
	DBG_IND("[class2] this is debug message, got val=%d\n", 3);
	DBG_DUMP("this is normal message without function name, module name and line number.\n");
	DBG_FUNC_END("with end val=%d\n", 99);
        return 0;
}

