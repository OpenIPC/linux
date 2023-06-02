#if defined(__FREERTOS)
#if defined(_BSP_NA51089_)
#include "crypto_protected_na51089.h"
#elif defined(_BSP_NA51000_)
#include "crypto_protected_na51000.h"
#endif
#else
#if defined(_BSP_NA51089_)
//#include "efuse_protected_na51055.h"
#endif
#endif
