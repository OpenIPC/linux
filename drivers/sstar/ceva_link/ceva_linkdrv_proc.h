#ifndef CEVA_LINKDRV_PROC_H_
#define CEVA_LINKDRV_PROC_H_

#define HOST_LOG_ENABLE 1
#define HOST_LOG_BUF_MEM_SIZE   (1 * 4096)
#define INVALID_DSP_LOG_ADDR    0xFFFFFFFF

#define HOST_LOG_MAX_LINE 30
#define HOST_LOG_LINE_SIZE 128
#define DSP_LOG_MAX_LINE 30
#define DSP_LOG_LINE_SIZE 128

#define HOST_LOG_TITLE   "[%s] "
#define HOST_LOG_FUNC    __func__

#if HOST_LOG_ENABLE
#define HOST_LOG(_fmt, _args...)                          \
        host_log_add(HOST_LOG_TITLE  _fmt, HOST_LOG_FUNC, ## _args);
#else
#define HOST_LOG(_fmt, _args...)
#endif

typedef struct {
    u32 dsp_log_head;
    u32 dsp_log_tail;
    u32 dsp_log_size;
    u32 dsp_log_index;
    u8 dsp_log_buf[DSP_LOG_MAX_LINE * DSP_LOG_LINE_SIZE];
} dsp_info_buf;

extern int host_log_init(void);
extern void host_log_add(const char *fmt, ...);
extern int dsp_log_init(phys_addr_t phy_addr, u32 size);
extern int ceva_linkdrv_proc_init(struct xm6_dev_data *p_data);
extern void ceva_linkdrv_proc_exit(void);

#endif /* CEVA_LINKDRV_PROC_H_ */
