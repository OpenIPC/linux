#include "kwrap/type.h"
#include <kwrap/verinfo.h>

//=============================================================================
// AWB version
//=============================================================================
VOS_MODULE_VERSION(nvt_awb, 1, 00, 000, 00);

//=============================================================================
// AWB version
//=============================================================================
#define AWB_VER_MAJOR       1
#define AWB_VER_MINOR       0
#define AWB_VER_MINOR2      0
#define AWB_VER_MINOR3      0
#define AWB_VER             ((AWB_VER_MAJOR << 24) | (AWB_VER_MINOR << 16) | (AWB_VER_MINOR2 << 8) | AWB_VER_MINOR3)

//=============================================================================
// extern functions
//=============================================================================
UINT32 awb_get_version(void)
{
	return AWB_VER;
}

