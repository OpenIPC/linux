
#ifndef __NVT_EMU_AFFINE__
#define __NVT_EMU_AFFINE__

#if defined(__FREERTOS)
#include <kwrap/cmdsys.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#else
#include <linux/fs.h>
#include <asm/segment.h>
#include <linux/uaccess.h>
#include <linux/buffer_head.h>
#endif
#include "affine_reg.h"
#include "affine_compatible.h"

#if 1	// FS API wrapper
/**
    File Handle.
*/
typedef UINT32  *FST_FILE;

/**
     @name File system open file flag
*/
//@{
#define FST_OPEN_READ                     0x00000001      ///< open file with read mode
#define FST_OPEN_WRITE                    0x00000002      ///< open file with write mode
#define FST_OPEN_EXISTING                 0x00000004      ///< if exist, open; if not exist, return fail
#define FST_OPEN_ALWAYS                   0x00000008      ///< if exist, open; if not exist, create new
//#define FST_CREATE_NEW                    0x00000010    ///< if exist, return fail; if not exist, create new
#define FST_CREATE_ALWAYS                 0x00000020      ///< if exist, open and truncate it; if not exist, create new
#define FST_SPEEDUP_CONTACCESS            0x00000100      ///< read/write random position access can enable this flag for speed up access speed
//@}

#endif

#if 1	// auto test related

#define AUTOTEST_BUFSIZE_AFFINE    0x00100000

// Auto test result
typedef enum {
	AUTOTEST_RESULT_OK,
	AUTOTEST_RESULT_FAIL,

	ENUM_DUMMY4WORD(AUTOTEST_RESULT)
} AUTOTEST_RESULT;

#define READ_CORRECT    (1)                 // 0: generate pattern, 1: normal compare mode

#define IMG1_WIDTH      0xA0
#define IMG1_HEIGHT     0x70
#define IMG1_LOFF       0xA0
#define IMG2_LOFF       0x70

#define AFFINE_AUTO_FS_MODE   (1)

#if defined(__FREERTOS)
#define auto_msg(msg)               printf msg
#else
#define auto_msg(msg)               printk msg
#endif

#define AFFINE_PI               (3.14159265359)
#endif

extern AUTOTEST_RESULT emu_affine_auto(UINT32 addr, UINT32 size);

#endif
