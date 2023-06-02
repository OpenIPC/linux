#ifndef _NVT_EXAMSYS_H
#define _NVT_EXAMSYS_H

#ifdef __cplusplus
# define __NVT_EXAMSYS_EXTERNC extern "C"
#else
# define __NVT_EXAMSYS_EXTERNC extern
#endif
// Also define externC for now - but it is deprecated
#define NVT_EXAMSYS_EXTERNC __NVT_EXAMSYS_EXTERNC

typedef int (*NVT_EXAMSYS_MAIN)(int argc, char **argv);

typedef struct _NVT_EXAMSYS_ENTRY {
    NVT_EXAMSYS_MAIN p_main;
    char name[64];
} NVT_EXAMSYS_ENTRY;

#if defined(__FREERTOS) || defined(_NVT_CONSOLE_)
#define EXAMFUNC_ENTRY(_l,_argc,_argv) \
NVT_EXAMSYS_EXTERNC int _l##_examsys_main(int _argc, char **_argv); \
NVT_EXAMSYS_ENTRY _l##examsys_entry __attribute__ ((section (".examsys.table." #_l))) = {    \
   _l##_examsys_main,                                      \
   #_l							  \
}; \
NVT_EXAMSYS_EXTERNC int _l##_examsys_main(int _argc, char **_argv)
#else
#define EXAMFUNC_ENTRY(_l,_argc,_argv) \
int main(int _argc, char **_argv)
#endif

#if defined(__FREERTOS) || defined(_NVT_CONSOLE_)
#define NVT_EXAMSYS_FGETS(str, n, stream) nvt_examsys_fgets(str, n, stream)
#define NVT_EXAMSYS_GETCHAR() nvt_examsys_getchar()
#else
#define NVT_EXAMSYS_FGETS(str, n, stream) fgets(str, n, stream)
#define NVT_EXAMSYS_GETCHAR() getchar()
#endif

NVT_EXAMSYS_EXTERNC int nvt_examsys_init(void);
NVT_EXAMSYS_EXTERNC int nvt_examsys_runcmd(char *str); ///< run directly
NVT_EXAMSYS_EXTERNC int nvt_examsys_runcmd_bk(char *str); ///< run in background
NVT_EXAMSYS_EXTERNC char *nvt_examsys_fgets(char *str, int n, FILE *stream);
NVT_EXAMSYS_EXTERNC char nvt_examsys_getchar(void);

#endif /* _NVT_EXAMSYS_H */
