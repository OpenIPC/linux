#ifndef __FH_EFUSE_PLAT_H__
#define __FH_EFUSE_PLAT_H__

struct fh_efuse_platform_data {
	u32 efuse_support_flag;
	void *optee_uuid;
};

#endif