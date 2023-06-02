#ifndef _KER_DUMP_H_
#define _KER_DUMP_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                             */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>
#include <kwrap/type.h>

typedef struct _OSINIT_PARAM {
	//Regions
	MEM_RANGE total; //for kernel all regions
	MEM_RANGE resv; //for kernel stack & isr stack
	MEM_RANGE code; //for text (ro-code), data (rw-data), ro-data,
	MEM_RANGE zi;   //for zero-init-data
	MEM_RANGE os_heap;   //for os heap
	MEM_RANGE libc_heap; //for libc heap
}
OSINIT_PARAM;

extern void ker_dump_os_init(OSINIT_PARAM *init_parm);

#ifdef __cplusplus
}
#endif

#endif /* _KER_DUMP_H_ */

