#if defined(__FREERTOS)
#else
#include <linux/module.h>
#endif
#include "kwrap/type.h"
#include <kwrap/verinfo.h>

//=============================================================================
// AF version
//=============================================================================
VOS_MODULE_VERSION(nvt_af, 1, 00, 000, 00);

//=============================================================================
// AF version
//=============================================================================
#define AF_VER_MAJOR     1
#define AF_VER_MINOR     0
#define AF_VER_MINOR2    0
#define AF_VER_MINOR3    0
#define AF_VER           ((AF_VER_MAJOR << 24) | (AF_VER_MINOR << 16) | (AF_VER_MINOR2 << 8) | AF_VER_MINOR3)


//=============================================================================
// extern functions
//=============================================================================
UINT32 af_get_version(void)
{
	return AF_VER;
}

