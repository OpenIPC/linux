
#ifndef _MFE_ASIC_H_
#define _MFE_ASIC_H_

typedef struct mfe_asic {
    struct mutex* lock;
    int     i_ctx_count;
    void*   p_iobase;
    void*   p_pmbase;
} mfe_asic;

#endif//_MFE_ASIC_H_

