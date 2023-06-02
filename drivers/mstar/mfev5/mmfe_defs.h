
#ifndef _MMFE_DEFS_H_
#define _MMFE_DEFS_H_

#include <mhve_ios.h>
#include <mhve_ops.h>
#include <rqct_ops.h>

/* options */
#define MMFE_TIMER_SIZE         2048

typedef MSYS_DMEM_INFO  msys_mem;
typedef struct mmfe_ctx mmfe_ctx;
typedef struct mmfe_rqc mmfe_rqc;
typedef struct mmfe_dev mmfe_dev;

struct mmfe_ctx
{
    atomic_t        i_refcnt;
    void (*release) (void* );
    void (*adduser) (void* );
    struct mutex    m_stream;
    struct mutex    m_encode;
    mmfe_dev*       p_device;
    void*           p_handle;
    /* CTX status */
#define MMFE_CTX_STATE_NULL  -1
#define MMFE_CTX_STATE_IDLE   0
#define MMFE_CTX_STATE_BUSY   1
    int             i_state;
    int             i_score;
    int             i_index;
    /* direct memory resource */
    int             i_strid;        // stream id set from user
    short           i_max_w;
    short           i_max_h;
    /* user data buffer */
#define MMFE_USER_DATA_SIZE  (4*1024)
    unsigned char*  p_usrdt;
    short           i_usrsz;
    short           i_usrcn;
    /* user mode output */
    unsigned char*  p_ovptr;
    unsigned int    u_ophys;
    int             i_osize;
    int             i_ormdr;
    /* output cpbs(coded picture buffer) */
#define MMFE_CPBS_NR          1
    mhve_cpb        m_mcpbs[MMFE_CPBS_NR];
    /* output buffer mode */
#define MMFE_OMODE_MMAP       0
#define MMFE_OMODE_USER       1
#define MMFE_OMODE_PIPE       2
    short           i_omode;
    /* input buffer mode */
#define MMFE_IMODE_PURE       0
#define MMFE_IMODE_PLUS       1     /* use input buffer as reconstruct buffer */
    short           i_imode;
    /* direct memory resource */
#define MMFE_DMEM_NR          4
    int             i_dmems;
    msys_mem        m_dmems[MMFE_DMEM_NR];
#if MMFE_TIMER_SIZE
    int             i_numbr;
    struct
    {
        unsigned char   tm_dur[4];
        unsigned int    tm_cycles;
    } *p_timer;
#endif
};

mmfe_ctx* mmfectx_acquire(mmfe_dev*);
     long mmfectx_actions(mmfe_ctx*, unsigned int, void*);

struct mmfe_rqc
{
    struct mutex*       p_mutex;
    struct cdev         m_cdev;
    struct mmfe_ctx*    p_mctx;
};

long   mmferqc_ioctls(void*, unsigned int, void*);
size_t mmferqc_lprint(void*, char*, size_t);

struct mmfe_dev
{
    int     i_major;
    int     i_minor;
    struct cdev         m_cdev; // character device
    struct device       m_dev;  // device node
    struct device*      p_dev;  // platform device
    struct mutex        m_mutex;
    struct semaphore    m_sem;
    int                 i_irq;
    wait_queue_head_t   m_wqh;
#define MMFE_CLOCKS_NR        4
    struct clk*         p_clocks[MMFE_CLOCKS_NR];
#define MMFE_STREAM_NR        4
    struct mmfe_rqc     m_regrqc[MMFE_STREAM_NR];
  /* ... */
    void*   p_asicip;
#define MMFE_DEV_STATE_NULL  -1
#define MMFE_DEV_STATE_IDLE   0
#define MMFE_DEV_STATE_BUSY   1
    int     i_state;
    int     i_users;
    int     i_score;
    /* statistic */
    int     i_counts[MMFE_STREAM_NR][5];
    int     i_thresh;
    /* clock[0]  */
    int     i_clkidx;
    int     i_ratehz;
    /* rate-controller */
    int     i_rctidx;
};

int mmfedev_register(mmfe_dev*, mmfe_ctx*);
int mmfedev_unregister(mmfe_dev*, mmfe_ctx*);
int mmfedev_poweron(mmfe_dev*, int);
int mmfedev_suspend(mmfe_dev*);
int mmfedev_resume(mmfe_dev*);
int mmfedev_pushjob(mmfe_dev*, mmfe_ctx*);
int mmfedev_isr_fnx(mmfe_dev*);

#endif//_MMFE_DEFS_H_

