
// mstar hardware video encoder I/F (mhveif)

#ifndef _MHVE_IOS_H_
#define _MHVE_IOS_H_

typedef struct mhve_ios mhve_ios;
typedef struct mhve_reg mhve_reg;
typedef struct mhve_job mhve_job;

struct mhve_ios {
    void     (*release)(void*);                 //! release this object.
    int     (*set_bank)(mhve_ios*, mhve_reg*);  //! assign register base pointer.
    int     (*enc_fire)(mhve_ios*, mhve_job*);  //! process and trigger encoder job.
    int     (*enc_poll)(mhve_ios*);             //! polling encode-done.
    int     (*isr_func)(mhve_ios*, int);        //! interrupt service routine.
    int     (*irq_mask)(mhve_ios*, int);        //! masking irq.
};

struct mhve_reg {
    int     i_id;
    void*   base;
    int     size;
};

#define MHVEJOB_OKAY        0
#define MHVEJOB_ENC_DONE    0
#define MHVEJOB_BUF_FULL    1
#define MHVEJOB_ENC_FAIL   -1
#define MHVEJOB_TIME_OUT   -2

struct mhve_job {
    int     i_code;     // store encoder result as MHVEJOB_ENC_DONE/MHVEJOB_BUF_FULL/MHVEJOB_ENC_FAIL/MHVEJOB_TIME_OUT
    int     i_tick;     // count hw tick
    int     i_bits;
};

#endif//_MHVE_IOS_H_
