#ifndef __NVT_EMU_DDR_ARB__
#define __NVT_EMU_DDR_ARB__

#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include "ddr_arb_int.h"

// Auto test result
typedef enum {
        AUTOTEST_RESULT_OK,
        AUTOTEST_RESULT_FAIL,

        ENUM_DUMMY4WORD(AUTOTEST_RESULT)
} AUTOTEST_RESULT;


extern AUTOTEST_RESULT emu_ddr_arb_auto(void);

#endif
