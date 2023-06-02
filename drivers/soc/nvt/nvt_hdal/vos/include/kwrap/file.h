#ifndef _VOS_FILE_H_
#define _VOS_FILE_H_
#ifdef __cplusplus
extern "C" {
#endif

/*-----------------------------------------------------------------------------*/
/* Including Files                                                                                                                 */
/*-----------------------------------------------------------------------------*/
#include <kwrap/nvt_type.h>

#if defined(__FREERTOS)
#include <fcntl.h>

#elif defined(__LINUX) && defined(__KERNEL__)
#include <asm/fcntl.h>

#else
#include <fcntl.h>
#include <sys/stat.h>
#endif

#if VOS_LEGACY_ARCH32
typedef int VOS_FILE;
#else
typedef unsigned long VOS_FILE;
#endif

typedef unsigned int vos_mode_t;
typedef unsigned long vos_size_t;
typedef unsigned long vos_off_t;

#define VOS_FILE_INVALID ((VOS_FILE)-1)

struct vos_stat {
	vos_mode_t st_mode;
	vos_size_t st_size;
};

//! Init/Exit flag
void  rtos_file_init(void *param);
void  rtos_file_exit(void);

//! Common api
VOS_FILE vos_file_open(const char *pathname, int flags, vos_mode_t mode); //return (VOS_FILE)(-1) if failed

int vos_file_read(VOS_FILE vos_file, void *p_buf, vos_size_t count);
int vos_file_write(VOS_FILE vos_file, const void *p_buf, vos_size_t count);
vos_off_t vos_file_lseek(VOS_FILE vos_file, vos_off_t offset, int whence);

int vos_file_fsync(VOS_FILE vos_file);
int vos_file_fstat(VOS_FILE vos_file, struct vos_stat *p_stat);

int vos_file_close(VOS_FILE vos_file);

int vos_file_stat(const char *pathname, struct vos_stat *p_stat);

#ifdef __cplusplus
}
#endif

#endif /* _VOS_FILE_H_ */

