#include "kwrap/type.h"
#include <kwrap/verinfo.h>

//=============================================================================
// IQ version
//=============================================================================
VOS_MODULE_VERSION(nvt_iq, 1, 23, 010, 00);

#define IQ_VER_MAJOR       1
#define IQ_VER_MINOR       0
#define IQ_VER_MINOR2      1
#define IQ_VER_MINOR3      0
#define IQ_VER             ((IQ_VER_MAJOR << 24) | (IQ_VER_MINOR << 16) | (IQ_VER_MINOR2 << 8) | IQ_VER_MINOR3)

//=============================================================================
// extern functions
//=============================================================================
UINT32 iq_get_version(void)
{
	return IQ_VER;
}

