#ifndef _AD_UITL_INT_H_
#define _AD_UITL_INT_H_

#define _AD_TOKEN_BIND(a, b)  a ## _ ## b
#define AD_TOKEN_BIND(a, b)  _AD_TOKEN_BIND(a, b)

#define _AD_TOKEN_STR(a)  #a
#define AD_TOKEN_STR(a)  _AD_TOKEN_STR(a)

#define AD_UTIL_DECLARE_FUNC(name) AD_TOKEN_BIND(name, _MODEL_NAME)
#define AD_UTIL_STR(name) AD_TOKEN_STR(AD_TOKEN_BIND(name, _MODEL_NAME))

#define AD_UTIL_GEN_I2C_ID_NAME(idx) AD_UTIL_STR(ad) "." AD_TOKEN_STR(idx)    //ad_xxxxx_idx
#define AD_UTIL_GEN_COMPAT_NAME(idx) "nvt," AD_UTIL_STR(ad) "." AD_TOKEN_STR(idx)    //nvt,ad_xxxxx_idx

#define AD_SXCMD_BEGIN(tbl,desc) SXCMD_BEGIN(tbl,desc)
#define AD_SXCMD_ITEM(cmd,func,desc) SXCMD_ITEM(cmd,func,desc)
#define AD_SXCMD_END() SXCMD_END()
#define AD_MAINFUNC_ENTRY(cmd, argc, argv)  MAINFUNC_ENTRY(cmd, argc, argv)
#endif //_AD_UITL_INT_H_
