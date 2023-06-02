#ifndef _VENDOR_IVE_VERSION_H_
#define _VENDOR_IVE_VERSION_H_

#define ive_major     01
#define ive_minor     01
#define ive_bugfix    2105190
#define ive_ext       0

#define _IVE_STR(a,b,c)          #a"."#b"."#c
#define IVE_STR(a,b,c)           _IVE_STR(a,b,c)
#define VENDOR_IVE_IMPL_VERSION    IVE_STR(ive_major, ive_minor, ive_bugfix)//implementation version major.minor.yymmdds


#endif //_VENDOR_IVE_VERSION_H_
