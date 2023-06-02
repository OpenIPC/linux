
#ifndef _MFE_ASIC_H_
#define _MFE_ASIC_H_

#include <mhve_ios.h>

typedef struct mfe_asic mfe_asic;

struct mfe_asic {
    mhve_ios    mios;
    void*       p_base;
    mfe_regs*   p_regs;
};

#define BIT_ENC_DONE    (1<<0)
#define BIT_BUF_FULL    (1<<1)

#endif/*_MFE_ASIC_H_*/
