#ifndef _NVTMPP_LOG_H
#define _NVTMPP_LOG_H

#if defined __UITRON || defined __ECOS
typedef void (*NVTMPP_DUMP)(char *fmt, ...);
#else
typedef int (*NVTMPP_DUMP)(const char *fmt, ...);
#endif


extern  void nvtmpp_log_open(NVTMPP_DUMP dump);
extern  int  nvtmpp_log_save_str(const char *fmtstr, ...);
extern  void nvtmpp_log_dump(void);
extern  void nvtmpp_log_close(void);

#endif
