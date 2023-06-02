#ifndef __NVT_EMU_TIMER__
#define __NVT_EMU_TIMER__

#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#include "timer_int.h"

// Auto test result
typedef enum {
        AUTOTEST_RESULT_OK,
        AUTOTEST_RESULT_FAIL,

        ENUM_DUMMY4WORD(AUTOTEST_RESULT)
} AUTOTEST_RESULT;


extern AUTOTEST_RESULT emu_timer_auto(void);

#endif
