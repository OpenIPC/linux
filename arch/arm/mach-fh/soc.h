#ifndef _SOC_H_
#define _SOC_H_

void fh_pmu_set_reg(u32 offset, u32 data);
u32 fh_pmu_get_reg(u32 offset);
void fh_pmu_set_reg_m(u32 offset, u32 data, u32 mask);
int fh_pmu_init(void);

#endif /* _SOC_H_ */
