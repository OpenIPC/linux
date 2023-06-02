
#ifndef _NVT_CMDSYS_H
#define _NVT_CMDSYS_H

#ifdef __cplusplus
# define __NVT_CMDSYS_EXTERNC extern "C"
#else
# define __NVT_CMDSYS_EXTERNC extern
#endif
// Also define externC for now - but it is deprecated
#define NVT_CMDSYS_EXTERNC __NVT_CMDSYS_EXTERNC

typedef int (*NVT_CMDSYS_MAIN)(int argc, char **argv); ///< command entry
typedef int (*NVT_CMDSYS_CB)(char *str); ///< register new domain command sys

typedef struct _NVT_CMDSYS_ENTRY {
    NVT_CMDSYS_MAIN p_main;
    char name[32];
} NVT_CMDSYS_ENTRY;

#if defined(__FREERTOS) || defined(_NVT_CONSOLE_)
#define MAINFUNC_ENTRY(_l,_argc,_argv) \
NVT_CMDSYS_EXTERNC int _l##_cmdsys_main(int _argc, char **_argv); \
NVT_CMDSYS_ENTRY _l##cmdsys_entry __attribute__ ((section (".cmdsys.table." #_l))) = {    \
   _l##_cmdsys_main,                                      \
   #_l							  \
}; \
NVT_CMDSYS_EXTERNC int _l##_cmdsys_main(int _argc, char **_argv)
#else
#define MAINFUNC_ENTRY(_l,_argc,_argv) \
int main(int _argc, char **_argv)
#endif

NVT_CMDSYS_EXTERNC int nvt_cmdsys_init(void);
NVT_CMDSYS_EXTERNC int nvt_cmdsys_runcmd(char *str);
NVT_CMDSYS_EXTERNC int nvt_cmdsys_regsys(char domain, NVT_CMDSYS_CB cb);
NVT_CMDSYS_EXTERNC int nvt_cmdsys_ipc_cmd(int argc, char **argv); ///< only for linux user space


#endif /* _NVT_CMDSYS_H */
