#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <linux/io.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/reboot.h> //for kernel_halt
#include <asm/uaccess.h>
#include <kwrap/type.h>

#define __MODULE__    rtos_debug
#define __DBGLVL__    2
#define __DBGFLT__    "*"
#include <kwrap/debug.h>
#include <kwrap/file.h>

static void (*m_msg_fp)(const char *fmtstr, ...) = NULL;

void vos_debug_halt(void)
{
	kernel_halt();
}

void debug_dumpmem(ULONG addr, ULONG length)
{
	ULONG   offs;
	UINT32  str_len;
	UINT32  cnt;
	CHAR    str_dumpmem[64];
	void*   map_addr = NULL;
	UINT32  u32array[4];
	UINT32  *p_u32;
	CHAR    *p_char;

	if (0 == addr || 0 == length) {
		DBG_DUMP("Invalid addr %ld, length %ld\r\n", addr, length);
		return;
	}
	addr = ALIGN_FLOOR_4(addr); //align to 4 bytes (UINT32)

	if ((addr & 0xF0000000) == 0xF0000000) {
		map_addr = ioremap_nocache(addr, length+16);
		if (NULL == map_addr) {
			DBG_DUMP("ioremap() failed, addr 0x%lx, length %ld\r\n", addr, length);
			return;
		}
		p_u32 = (UINT32 *)map_addr;
	} else {
		p_u32 = (UINT32 *)addr;
	}

	DBG_DUMP("dump va=%08lx, addr=%08lx length=%08lx to console:\r\n", (ULONG)p_u32, addr, length);

	for (offs = 0; offs < length; offs += sizeof(u32array)) {
		u32array[0] = *p_u32++;
		u32array[1] = *p_u32++;
		u32array[2] = *p_u32++;
		u32array[3] = *p_u32++;

		str_len = snprintf(str_dumpmem, sizeof(str_dumpmem), "%08lX : %08X %08X %08X %08X  ",
			(addr + offs), (UINT)u32array[0], (UINT)u32array[1], (UINT)u32array[2], (UINT)u32array[3]);

		p_char = (char *)&u32array[0];
		for (cnt = 0; cnt < sizeof(u32array); cnt++, p_char++) {
			if (*p_char < 0x20 || *p_char >= 0x80)
				str_len += snprintf(str_dumpmem+str_len, 64-str_len, ".");
			else
				str_len += snprintf(str_dumpmem+str_len, 64-str_len, "%c", *p_char);
		}

		DBG_DUMP("%s\r\n", str_dumpmem);
	}
	DBG_DUMP("\r\n\r\n");
	if (NULL != map_addr) {
		iounmap(map_addr);
	}
}
void debug_dumpmem2file(ULONG addr, ULONG length, char *filename)
{
	int write_size;
	VOS_FILE fd;

	DBG_DUMP("dump addr=0x%08lx length=0x%08lx to file %s:\r\n", addr, length, filename);

	fd = vos_file_open(filename, O_CREAT|O_WRONLY|O_SYNC, 0);
	if ((VOS_FILE)(-1) == fd) {
		DBG_DUMP("open %s failed\r\n", filename);
		return;
	}

	write_size = vos_file_write(fd, (void *)addr, length);

	if (-1 == vos_file_close(fd)) {
		DBG_DUMP("close %s failed\r\n", filename);
	}

	if (write_size <= 0) {
		DBG_DUMP("dump file fail\r\n");
	} else {
		DBG_DUMP("dump file success\r\n");
	}
}

static char msg[512] = {0};
void debug_kmsg(char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	vsnprintf((void *)msg, sizeof(msg)-1, fmt, args);
	va_end(args);

	if (m_msg_fp != NULL) {
		m_msg_fp(msg);
    } else {
    	printk(msg);
    }
}

void debug_kmsg_register(void (*fp)(const char *fmtstr, ...))
{
	m_msg_fp = fp;
}

int _IOFUNC_DEBUG_IOCMD_HALT(unsigned long arg)
{
	vos_debug_halt();
	return 0;
}

/*-----------------------------------------------------------------------------*/
/* Kernel Mode Definiton                                                       */
/*-----------------------------------------------------------------------------*/
EXPORT_SYMBOL(vos_debug_halt);
EXPORT_SYMBOL(debug_dumpmem);
EXPORT_SYMBOL(debug_dumpmem2file);
EXPORT_SYMBOL(debug_kmsg);
EXPORT_SYMBOL(debug_kmsg_register);
