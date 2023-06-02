#ifndef _KDRV_IVE_VERSION_H_
#define _KDRV_IVE_VERSION_H_

#include <kwrap/verinfo.h>

#define major     01
#define minor     01
#define bugfix    2105190
#define ext       0

#define _IVE_STR(a,b,c)          #a"."#b"."#c
#define IVE_STR(a,b,c)           _IVE_STR(a,b,c)
#define KDRV_IVE_IMPL_VERSION    IVE_STR(major, minor, bugfix)//implementation version major.minor.yymmdds

#endif //_KDRV_IVE_VERSION_H_
