#ifndef _FH_PMU_H_
#define _FH_PMU_H_

#include <linux/types.h>
#include <mach/fh_chipid.h>

#include "chip.h"

void fh_get_chipid(unsigned int *plat_id, unsigned int *chip_id);
unsigned int fh_pmu_get_ptsl(void);
unsigned int fh_pmu_get_ptsh(void);
unsigned long long fh_get_pts64(void);
void fh_pmu_wdt_pause(void);
void fh_pmu_wdt_resume(void);
void fh_pmu_usb_utmi_rst(void);
void fh_pmu_usb_phy_rst(void);
void fh_pmu_usb_resume(void);

unsigned int fh_pmu_get_tsensor_init_data(void);
unsigned int fh_pmu_get_ddrsize(void);
void fh_pmu_set_sdc1_funcsel(unsigned int val);

void fh_pmu_sdc_reset(int slot_id);
void fh_pmu_mipi_reset(void);
void fh_pmu_enc_reset(void);
void fh_pmu_dwi2s_set_clk(unsigned int div_i2s, unsigned int div_mclk);
void fh_pmu_eth_set_speed(unsigned int speed);
void fh_pmu_eth_reset(void);
void fh_pmu_restart(void);

int fh_pmu_internal_ephy_reset(void);
void fh_pmu_ephy_sel(__u32 phy_sel);
void fh_pmu_arxc_write_A625_INT_RAWSTAT(unsigned int val);
unsigned int fh_pmu_arxc_read_ARM_INT_RAWSTAT(void);
void fh_pmu_arxc_write_ARM_INT_RAWSTAT(unsigned int val);
unsigned int fh_pmu_arxc_read_ARM_INT_STAT(void);
void fh_pmu_arxc_reset(unsigned long phy_addr);
void fh_pmu_arxc_kickoff(void);

int fh_pmu_init(void);


#endif /* _FH_PMU_H_ */
