#ifndef _NVTMPP_PLATFORM_H
#define _NVTMPP_PLATFORM_H

extern void nvtmpp_dump_stack_by_pid(UINT32 pid);
extern void nvtmpp_dump_stack(void);
extern void nvtmpp_dumpmem(UINT32 pa, UINT32 length);
extern void nvtmpp_dumpmem2file(UINT32 pa, UINT32 length, char *filename);
extern void *nvtmpp_vmalloc(unsigned long size);
extern void nvtmpp_vfree(void *v_buff);
#endif
