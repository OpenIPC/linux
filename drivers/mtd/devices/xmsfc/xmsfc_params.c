#include <linux/mtd/mtd.h>                  
#include "xmsfc.h"

const struct xmsfc_params xmsfc_params_table[] = { 
	{"W25Q64CV", 0xef4017, _8M,  _64K,  0xD8,  0x02, 0x03, 0},  
	//{"W25Q64CV", 0xef4017, _8M,  _64K,  0xD8,  0x32, 0x6B, FLAG_QUAD_ONE_ADDR},  
	{"MX25L6405D", 0xc22017, _8M,  _64K,  0xD8,  0x02, 0x03, 0},  
};
const struct xmsfc_params xmsfc_params_default = { 
	"DEFAULT", 0x000000, _8M,  _64K,  0xD8,  0x02, 0x03, 0 
};
