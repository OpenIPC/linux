#ifndef _EMU_H26X_MEM_H_
#define _EMU_H26X_MEM_H_

#include "kwrap/type.h"


//#if (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)
#define WP_DDR_NUM 2
#define WP_CHN_NUM 6  // 3

typedef struct _h26x_mem_t_{
	UINT32 start;
	UINT32 addr;
	UINT32 size;
}h26x_mem_t;
void h26x_enable_dram2(void);
void h26x_enable_wp(void);
unsigned int h26x_mem_malloc(h26x_mem_t *p_mem, unsigned int size);
void h26x_test_wp(UINT8 wp_idx, UINT32 start_addr, UINT32 size, UINT32 level);
void h26x_disable_wp(void);
void h26x_test_wp_2(UINT8 wp_idx, UINT32 start_addr, UINT32 size, UINT32 level);

//#endif // (EMU_H26X == ENABLE || AUTOTEST_H26X == ENABLE)

#endif
