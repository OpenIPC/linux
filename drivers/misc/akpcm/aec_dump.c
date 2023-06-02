#include <linux/kernel.h>
#include <linux/fs.h>
#include <asm/segment.h>
#include <asm/uaccess.h>
#include <linux/buffer_head.h>
#include <linux/rtc.h>

#include "akpcm_defs.h"
#include "aec_dump.h"

#define CONFIG_AEC_DUMP_FILE_WAV


/////////////////////////////////////////////////////
/* aec debug info */
static char isr_print_buf[1000];
static long in_isr_print_buf = 0;

unsigned char dump_head[20] = "\0AECdump"; // 8 char

/* aec dump file info */
typedef struct S_WAVE_HEADER {
	char    riff[4];        // "RIFF"  0
	long    file_size;       // in bytes 4
	char    wavefmt[8];     // "WAVE" 8
	long    chunk_size;      // in bytes (16 for PCM) 16
	short    format_tag;      // 1=PCM, 2=ADPCM, 3=IEEE float, 6=A-Law, 7=Mu-Law 20
	short    num_chans;       // 1=mono, 2=stereo 22
	long    sample_rate;     // 24
	long    bytes_per_sec;   // 28
	short    bytes_per_samp;  // 2=16-bit mono, 4=16-bit stereo 32
	short    bits_per_samp;   // 34
	char    data[4];         // "data" 36
	long    data_length;     // in bytes 40
} WAVE_HEADER; //size 44

typedef struct _T_AEC_DUMP_HANDLE_
{
	char *file_name;
	struct file *file;
	int cur; // current ptr
	int data_len;
}T_AEC_DUMP_HANDLE;

static T_AEC_DUMP_HANDLE aec_dump_handle[4] = {{"near"},{"far"},{"res"},{"after_eq"}};
static int aec_dump_sn = 0; // serial number

/////////////////////////////////////////////////////
/* aec debug function */
void print_isr_debug(void)
{
    int i = 0;

	if (in_isr_print_buf == 0)
		return;

	for (i=0; i<in_isr_print_buf; i++)
		printk("%c",isr_print_buf[i]);

	in_isr_print_buf = 0;
}

// called in interrupt
void putch_isr(char ch)
{
	isr_print_buf[in_isr_print_buf++] = ch;
}

void putstr_isr_debug(char *str)
{
	int i=0;

	while (str[i] != '\0')
		isr_print_buf[in_isr_print_buf++] = str[i++];
}



/////////////////////////////////////////////////////
/* aec dump file function */
static struct file *file_open(const char *path, int flags, int rights)
{
	struct file *filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
	set_fs(get_ds());
	filp = filp_open(path, flags, rights);
	set_fs(oldfs);
	if (IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}

	return filp;
}

static void file_close(struct file *file)
{
	filp_close(file, NULL);
}

#ifdef CONFIG_AEC_DUMP_FILE_WAV
static int file_read(struct file *file, unsigned long long offset, unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, data, size, &offset);

	set_fs(oldfs);

	return ret;
}
#endif

static int file_write(struct file *file, unsigned long long offset, 
						unsigned char *data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, data, size, &offset);

	set_fs(oldfs);

	return ret;
}

static int file_sync(struct file *file)
{
	vfs_fsync(file, 0);
	return 0;
}

void putstr_isr_dump_file(char *str)
{
	printk(str);
}

static void open_dump_file(char *time_str, struct aec_dump_info *info)
{
	T_AEC_DUMP_HANDLE *hDump = &aec_dump_handle[info->id];
	char file_path[100];
#ifdef CONFIG_AEC_DUMP_FILE_WAV
	WAVE_HEADER WaveHead;
#endif

	sprintf(file_path, ("/tmp/%s_aec_%s"
#ifdef CONFIG_AEC_DUMP_FILE_WAV
		".wav"),
#else
		".pcm"),
#endif
		time_str,
		hDump->file_name);

	/* create file */
	hDump->file = file_open(file_path, O_RDWR|O_CREAT, 0644);
	hDump->cur = 0;
	hDump->data_len = 0;

#ifdef CONFIG_AEC_DUMP_FILE_WAV
	/* write wav head */
	memcpy(WaveHead.riff, "RIFF", 4);
	WaveHead.file_size = sizeof(WAVE_HEADER) - 8;
	memcpy(WaveHead.wavefmt, "WAVEfmt ", 8);
	WaveHead.chunk_size = 16;
	WaveHead.format_tag = 1;
	WaveHead.num_chans = 1;
	WaveHead.sample_rate = info->sample_rate;
	WaveHead.bytes_per_sec = info->sample_rate * 2;
	WaveHead.bytes_per_samp = 2;
	WaveHead.bits_per_samp = 16;
	memcpy(WaveHead.data, "data", 4);
	WaveHead.data_length = 0;

	file_write(hDump->file, 0, (unsigned char *)&WaveHead, sizeof(WaveHead));
	hDump->cur += sizeof(WaveHead);
#endif

	//printk(KERN_ERR "[%s] open id %d, handle %p\n", __FUNCTION__, id, hDump->file);
}

static void start_dump(struct aec_dump_info *info)
{
	struct timeval time;
	unsigned long local_time;
	struct tm tm;
	char time_str[100];

	do_gettimeofday(&time);
	local_time = (u32)(time.tv_sec);
	time_to_tm(local_time, 0, &tm);

	sprintf(time_str, "%04ld-%02d-%02d_%02d-%02d-%02d_%d",
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec, aec_dump_sn);
	ak_pcm_info("aec dump start: %s", time_str);

	if (info->id >= AEC_DUMP_RES) {
	    ++aec_dump_sn;
	}
	open_dump_file(time_str, info);
}

static void dump_complete_file(unsigned char id)
{
	T_AEC_DUMP_HANDLE *hDump = &aec_dump_handle[id];
#ifdef CONFIG_AEC_DUMP_FILE_WAV
	WAVE_HEADER WaveHead;
#endif

	if (hDump->file == NULL)
		return;

#ifdef CONFIG_AEC_DUMP_FILE_WAV
	/* read back wav head */
	file_read(hDump->file, 0, (unsigned char *)&WaveHead, sizeof(WaveHead));

	/* write wav length */
	WaveHead.data_length = hDump->data_len;
	WaveHead.file_size = WaveHead.data_length + (sizeof(WAVE_HEADER) - 8);
	file_write(hDump->file, 0, (unsigned char *)&WaveHead, sizeof(WaveHead));
#endif

	/* close file */
	file_sync(hDump->file);
	file_close(hDump->file);
	hDump->file = NULL;
	hDump->cur = 0;
	hDump->data_len = 0;
}

void aec_dump_complete_all_file(void)
{
	int i = 0;
	ak_pcm_debug("aec dump complete");

	for (i = 0; i < AEC_DUMP_MAX; i ++) {
		dump_complete_file(i);
	}
}

void aec_dump_file(struct aec_dump_info *info)
{
	T_AEC_DUMP_HANDLE *hDump = &aec_dump_handle[info->id];

	if (NULL == hDump->file) {
	    start_dump(info);
	    if (NULL == hDump->file) {
	        return;
	    }
	}

	//printk(KERN_ERR "[%s] dump id %d\n", __FUNCTION__, id);
	file_write(hDump->file, hDump->cur, info->data, info->size);
	hDump->cur += info->size;
	hDump->data_len += info->size;
}

void aec_dump_sync_file(struct akpcm *pcm)
{
	T_AEC_DUMP_HANDLE *hDump = NULL;
	int id;
	int b_switch = 0;
    int aec_dump_max_size = (pcm->cptr_rt == NULL) ? 480000 : pcm->cptr_rt->cfg.rate * 60;// 1 second = samplerate * 2 bytes, save 30 second

	for (id=0; id<AEC_DUMP_MAX; id++)	{
		hDump = &aec_dump_handle[id];

		if (hDump->data_len >= aec_dump_max_size) {
			b_switch = 1;
		}
	}

	if (b_switch) {
		aec_dump_complete_all_file();
	} else if (hDump->file && AEC_DUMP_FILE != pcm->aec_dump_type) {
		aec_dump_complete_all_file();
	}
}

void putstr_isr_null(char *str)
{
	printk(str);
}

void print_isr(int dump_type)
{
	switch (dump_type) {
		case AEC_DUMP_DEBUG :
			print_isr_debug();
			break;
		default :
			break;
	}
}

void putstr_isr(int dump_type, char *str)
{
	switch (dump_type) {
		case AEC_DUMP_FILE :
			putstr_isr_null(str);
			break;
		case AEC_DUMP_DEBUG :
			putstr_isr_debug(str);
			break;
		default :
			putstr_isr_null(str);
			break;
	}
}

void aec_dump_complete(int dump_type)
{
	switch (dump_type) {
		case AEC_DUMP_FILE :
			aec_dump_complete_all_file();
			break;
		default :
			break;
	}
}

void aec_dump(int dump_type, struct aec_dump_info *info)
{
	switch (dump_type) {
		case AEC_DUMP_FILE :
			aec_dump_file(info);
			break;
		case AEC_DUMP_DEBUG :
			break;
		default :
			break;
	}
}

void aec_dump_sync(struct akpcm *pcm)
{
	aec_dump_sync_file(pcm);
}
