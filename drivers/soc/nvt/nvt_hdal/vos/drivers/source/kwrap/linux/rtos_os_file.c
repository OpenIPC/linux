/*-----------------------------------------------------------------------------*/
/* Include Header Files                                                        */
/*-----------------------------------------------------------------------------*/
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/module.h>
#include <linux/version.h>
#include <kwrap/file.h>

#define __MODULE__    rtos_file
#define __DBGLVL__    8 // 0=FATAL, 1=ERR, 2=WRN, 3=UNIT, 4=FUNC, 5=IND, 6=MSG, 7=VALUE, 8=USER
#define __DBGFLT__    "*"
#include <kwrap/debug.h>

/*-----------------------------------------------------------------------------*/
/* Local Types Declarations                                                    */
/*-----------------------------------------------------------------------------*/
#define RTOS_FILE_INITED_TAG       MAKEFOURCC('R', 'F', 'I', 'L') ///< a key value

/*-----------------------------------------------------------------------------*/
/* Local Global Variables                                                      */
/*-----------------------------------------------------------------------------*/
unsigned int rtos_file_debug_level = NVT_DBG_WRN;

module_param_named(rtos_file_debug_level, rtos_file_debug_level, int, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(rtos_file_debug_level, "Debug message level");

/*-----------------------------------------------------------------------------*/
/* Interface Functions                                                         */
/*-----------------------------------------------------------------------------*/
void rtos_file_init(void *param)
{
}

void rtos_file_exit(void)
{
}

VOS_FILE vos_file_open(const char *pathname, int flags, vos_mode_t mode)
{
	struct file* p_file = NULL;
	mm_segment_t org_fs;

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	p_file = filp_open(pathname, flags, (mode_t)mode);

	set_fs(org_fs);

	if(IS_ERR(p_file)) {
		DBG_ERR("open [%s] failed, ret %ld\r\n", pathname, PTR_ERR(p_file));
		return VOS_FILE_INVALID;
	}

	return (VOS_FILE)p_file;
}

int vos_file_read(VOS_FILE vos_file, void *p_buf, vos_size_t count)
{
	struct file *p_file = (struct file *)vos_file;
	int read_bytes;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
	mm_segment_t org_fs;

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	read_bytes = vfs_read(p_file, p_buf, (size_t)count, &p_file->f_pos);

	set_fs(org_fs);
#else
	read_bytes = kernel_read(p_file, p_buf, (size_t)count, &p_file->f_pos);
#endif
	return read_bytes;
}

int vos_file_write(VOS_FILE vos_file, const void *p_buf, vos_size_t count)
{
	struct file *p_file = (struct file *)vos_file;
	int written_bytes;

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
	mm_segment_t org_fs;

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	written_bytes = vfs_write(p_file, p_buf, (size_t)count, &p_file->f_pos);

	set_fs(org_fs);
#else
	written_bytes = kernel_write(p_file, p_buf, (size_t)count, &p_file->f_pos);
#endif

	return written_bytes;
}

int vos_file_close(VOS_FILE vos_file)
{
	struct file *p_file = (struct file *)vos_file;
	mm_segment_t org_fs;
	int ret;

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	if (0 != vfs_fsync(p_file, 0)) {
		DBG_ERR("vfs_fsync fail, vos_file 0x%lX\r\n", (ULONG)vos_file);
	}

	ret = filp_close(p_file, NULL);

	set_fs(org_fs);

	return ret;
}

vos_off_t vos_file_lseek(VOS_FILE vos_file, vos_off_t offset, int whence)
{
	struct file *p_file = (struct file *)vos_file;
	mm_segment_t org_fs;
	off_t ret_offset;

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	ret_offset = vfs_llseek(p_file, (loff_t)offset, whence);

	set_fs(org_fs);

	return (vos_off_t)ret_offset;
}

int vos_file_fstat(VOS_FILE vos_file, struct vos_stat *p_stat)
{
	struct kstat statbuf = {0};
	struct file *p_file = (struct file *)vos_file;
	mm_segment_t org_fs;
	int ret;

	if (NULL == p_stat) {
		return -1;
	}

	org_fs = get_fs();
	set_fs(KERNEL_DS);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 11, 0))
	ret = vfs_getattr(&p_file->f_path, &statbuf);
#else
	ret = vfs_getattr(&p_file->f_path, &statbuf, STATX_BASIC_STATS, AT_STATX_SYNC_AS_STAT);
#endif

	set_fs(org_fs);

	if (0 == ret) {
		//success, fill data
		p_stat->st_mode = (unsigned int)statbuf.mode;
		p_stat->st_size = (unsigned int)statbuf.size;
	}

	return ret;
}

int vos_file_stat(const char *pathname, struct vos_stat *p_stat)
{
	struct kstat statbuf = {0};
	mm_segment_t org_fs;
	int ret;

	if (NULL == p_stat) {
		return -1;
	}

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_stat(pathname, &statbuf);

	set_fs(org_fs);

	if (0 == ret) {
		//success, fill data
		p_stat->st_mode = (vos_mode_t)statbuf.mode;
		p_stat->st_size = (vos_size_t)statbuf.size;
	}

	return ret;
}

int vos_file_fsync(VOS_FILE vos_file)
{
	struct file *p_file = (struct file *)vos_file;
	mm_segment_t org_fs;
	int ret;

	org_fs = get_fs();
	set_fs(KERNEL_DS);

	ret = vfs_fsync(p_file, 0);

	set_fs(org_fs);

	return ret;
}

EXPORT_SYMBOL(vos_file_open);
EXPORT_SYMBOL(vos_file_read);
EXPORT_SYMBOL(vos_file_write);
EXPORT_SYMBOL(vos_file_close);
EXPORT_SYMBOL(vos_file_lseek);
EXPORT_SYMBOL(vos_file_fstat);
EXPORT_SYMBOL(vos_file_stat);
EXPORT_SYMBOL(vos_file_fsync);
