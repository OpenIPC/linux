#ifndef __FH_RTC_PLAT_H__
#define __FH_RTC_PLAT_H__

struct fh_rtc_plat_data {
    int lut_cof;
    int lut_offset;
    int tsensor_cp_default_out;
    char *clk_name;
};

#endif
