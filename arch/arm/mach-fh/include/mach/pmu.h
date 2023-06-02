
#ifndef _FH_PMU_H_
#define _FH_PMU_H_

#include <linux/types.h>

void fh_pmu_set_reg(u32 offset, u32 data);
u32 fh_pmu_get_reg(u32 offset);
int fh_pmu_init(void);

void fh_pmu_stop(void);

void fh_pae_set_reg(u32 offset, u32 data);

#endif /* _FH_PMU_H_ */
