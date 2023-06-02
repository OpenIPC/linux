
#ifndef _MVHE_DEFS_H_
#define _MVHE_DEFS_H_

#include <mhve_ios.h>
#include <mhve_ops.h>
#include <rqct_ops.h>

/* options */
#define MVHE_TIMER_SIZE         2048

typedef MSYS_DMEM_INFO  msys_mem;
typedef struct mvhe_rqc mvhe_rqc;
typedef struct mvhe_ctx mvhe_ctx;
typedef struct mvhe_dev mvhe_dev;

struct mvhe_ctx {
    void (*release) (void* );
    void (*adduser) (void* );
    atomic_t        i_refcnt;
    struct mutex    m_stream;
    struct mutex    m_encode;
    mvhe_dev*       p_device;
    void*           p_handle;
  #define MVHE_CTX_STATE_NULL  -1
  #define MVHE_CTX_STATE_IDLE   0
  #define MVHE_CTX_STATE_BUSY   1
    int             i_state;
    int             i_score;
    int             i_index;
  /* direct memory resource */
    int             i_strid;        // stream id set from user
    short           i_max_w;
    short           i_max_h;
  /* user data buffer */
  #define MVHE_USER_DATA_SIZE  (4*1024)
    unsigned char*  p_usrdt;
    short           i_usrsz;
    short           i_usrcn;
  /* user mode output */
    unsigned char*  p_ovptr;
    unsigned int    u_ophys;
    int             i_osize;
    int             i_ormdr;
  /* output cpbs */
  #define MVHE_CPBS_NR          1
    mhve_cpb        m_mcpbs[MVHE_CPBS_NR];
  #define MVHE_OMODE_MMAP       0
  #define MVHE_OMODE_USER       1
  #define MVHE_OMODE_PIPE       2
    short           i_omode;
  #define MVHE_IMODE_PURE       0
  #define MVHE_IMODE_PLUS       1
    short           i_imode;
  #define MVHE_DMEM_NR          4
    int             i_dmems;
    msys_mem        m_dmems[MVHE_DMEM_NR];
#if MVHE_TIMER_SIZE
    int             i_numbr;
    struct {
    unsigned char   tm_dur[4];
    unsigned int    tm_cycles;
    } *p_timer;
#endif
};

mvhe_ctx* mvhectx_acquire(mvhe_dev*);
long      mvhectx_actions(mvhe_ctx*, unsigned int, void*);

#define MVHE_IRQ_ENABLE

struct mvhe_rqc {
    struct mutex*       p_mutex;
    struct cdev         m_cdev;
    struct mvhe_ctx*    p_mctx;
};

long   mvherqc_ioctls(void*, unsigned int, void*);
size_t mvherqc_lprint(void*, char*, size_t);

struct mvhe_dev {
    int     i_major;
    int     i_minor;
    struct cdev         m_cdev;
    struct device       m_dev;
    struct device*      p_dev;
    struct mutex        m_mutex;
    struct semaphore    m_sem;
    int                 i_irq;
    wait_queue_head_t   m_wqh;
  #define MVHE_CLOCKS_NR        4
    struct clk*         p_clocks[MVHE_CLOCKS_NR];
  #define MVHE_STREAM_NR        4
    struct mvhe_rqc     m_regrqc[MVHE_STREAM_NR];
  /* ... */
    void*   p_asicip;
  #define MVHE_DEV_STATE_NULL  -1
  #define MVHE_DEV_STATE_IDLE   0
  #define MVHE_DEV_STATE_BUSY   1
    int     i_state;
    int     i_users;
    int     i_score;
    /* statistic */
    int     i_counts[MVHE_STREAM_NR][5];
    int     i_thresh;
    /* clocks[0] */
    int     i_clkidx;
    int     i_ratehz;
    /* rate-controller */
    int     i_rctidx;
};

int mvhedev_register(mvhe_dev*, mvhe_ctx*);
int mvhedev_unregister(mvhe_dev*, mvhe_ctx*);
int mvhedev_poweron(mvhe_dev*, int);
int mvhedev_suspend(mvhe_dev*);
int mvhedev_resume(mvhe_dev*);
int mvhedev_pushjob(mvhe_dev*, mvhe_ctx*);
int mvhedev_isr_fnx(mvhe_dev*);

#endif/*_MVHE_DEFS_H_*/

