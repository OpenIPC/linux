#include "kwrap/type.h"
#include <kwrap/verinfo.h>

//=============================================================================
// ISP version
//=============================================================================
VOS_MODULE_VERSION(nvt_isp, 1, 05, 000, 00);

#define ISP_VER_MAJOR       1
#define ISP_VER_MINOR       0
#define ISP_VER_MINOR2      0
#define ISP_VER_MINOR3      0
#define ISP_VER             ((ISP_VER_MAJOR << 24) | (ISP_VER_MINOR << 16) | (ISP_VER_MINOR2 << 8) | ISP_VER_MINOR3)

//=============================================================================
// extern functions
//=============================================================================
UINT32 isp_get_version(void)
{
	return ISP_VER;
}

