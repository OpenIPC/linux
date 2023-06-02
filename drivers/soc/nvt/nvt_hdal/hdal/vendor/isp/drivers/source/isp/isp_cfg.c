#if defined(__FREERTOS)
#include <stdio.h>
#include <string.h>
#else
#include <linux/syscalls.h>
#endif
#include "isp_cfg.h"
#include "isp_uti.h"
#include "isp_lib.h"

//=============================================================================
// routines
//=============================================================================
#ifndef __KERNEL__
static UINT32 isp_simple_guess_base(const char *cp)
{
	if (cp[0] == '0') {
		if ((TOLOWER(cp[1]) == 'x') && (IS_DIGIT(cp[2]))) {
			return 16;
		} else {
			return 8;
		}
	} else {
		return 10;
	}
}

static UINT32 isp_strtoul(const char *cp, char **endp, UINT32 base)
{
	UINT32 result = 0;

	if (!base) {
		base = isp_simple_guess_base(cp);
	}

	if ((base == 16) && (cp[0] == '0') && (TOLOWER(cp[1]) == 'x')) {
		cp += 2;
	}

	while (IS_DIGIT(*cp) || IS_HEX(*cp)) {
		UINT32 value;

		value = IS_DIGIT(*cp) ? (*cp - '0') : (TOLOWER(*cp) - 'a' + 10);
		if (value >= base) {
			break;
		}

		result = result * base + value;
		cp++;
	}

	if (endp) {
		*endp = (char *)cp;
	}

	return result;
}

INT32 isp_simple_strtol(char *str, char **endp, UINT32 base)
{
	if (*str == '-') {
		return -isp_simple_strtoul(str+1, endp, base);
	}

	return isp_simple_strtoul(str, endp, base);
}

UINT32 isp_simple_strtoul(char *str, char **endp, UINT32 base)
{
	return isp_strtoul(str, endp, base);
}
#endif

static INT32 isp_cfg_is_delimeter(const INT8 c)
{
	switch (c) {
	case CHAR_RET:
	case CHAR_NULL:
	case CHAR_COMMENT2:
	case CHAR_COMMENT1:
	case CHAR_NEWLINE:
	case CHAR_TAB:
	case CHAR_EQUAL:
	case CHAR_SPACE:
		return TRUE;
	}

	return FALSE;
}

static INT32 isp_cfg_find_section_string(const SECTION_MAP *sec_map, char *sec_name, UINT32 *sec_pos, UINT32 *sec_len)
{
	UINT32 i;
	INT32 cmp_match = FALSE;

	for (i = 0; i < MAX_SECTION_MAP_NUM; i++) {
		if (sec_map[i].sec_name[0] == CHAR_NULL) {
			break;
		}

		cmp_match = TRUE;

		// compare the section name
		if (strcmp((char *)sec_map[i].sec_name, sec_name)) {
			cmp_match = FALSE;  // section name does not match
			continue;
		}

		if (cmp_match == TRUE) {
			*sec_pos = sec_map[i].start_pos;
			*sec_len = sec_map[i].sec_len;

			break;
		}
	}

	return cmp_match;
}

static char *isp_cfg_find_key_string(const char *key_name, CFG_FILE_FMT *cfile, char *line, const UINT32 sec_pos, const UINT32 sec_len)
{
	char *s1, *s2;
	INT32 key_len = strlen(key_name);

	cfile->offset = (vos_off_t)sec_pos;

	while ((isp_cfg_read_line(cfile, line, LINE_LEN) >= 0) && (cfile->offset <= (vos_off_t)(sec_pos + sec_len))) {
		// search the key_name in this line
		s1 = strstr(line, key_name);
		if (s1 == NULL) {
			continue;  // key_name is not a substring of this line
		}

		// determine the end of the candidate key_name (s1)
		if ((isp_cfg_is_delimeter(s1[key_len]) == FALSE) || (((UINT32)s1 > (UINT32)line) && (isp_cfg_is_delimeter(*(s1-1)) == FALSE))) {
			continue;  // s1 is a super set of key_name, not exactly match
		}

		// key_name matched, looking for the equal sign '=' after key_name
		s2 = strchr((char *)(s1+key_len), CHAR_EQUAL);
		if (s2 != NULL) {
			for (++s2; IS_SPACE(*s2); ++s2) {
				;  // skip space charactors next to '='
			}
		}

		return s2;  // return the value string
	}

	return (char *)NULL;
}

//=============================================================================
// external functions
//=============================================================================
inline CFG_FILE_FMT *isp_cfg_open(const INT8 *file_name, const INT32 flag)
{
	CFG_FILE_FMT *cfile;

	cfile = (CFG_FILE_FMT *)isp_uti_mem_alloc(sizeof(CFG_FILE_FMT));

	if (cfile != NULL) {
		cfile->offset = 0;
		cfile->filp = vos_file_open((char *)file_name, flag, 0x777);

		if ((INT32)cfile->filp == -1) {
			isp_uti_mem_free(cfile);
			cfile = NULL;
			return NULL;
		}

		return cfile;
	} else {
		return NULL;
	}
}

void isp_cfg_close(CFG_FILE_FMT *cfile)
{
	do {
		vos_file_close(cfile->filp);
		isp_uti_mem_free(cfile);
		cfile = NULL;
	} while (0);
}

UINT32 isp_cfg_build_section_map(CFG_FILE_FMT *pcfg_file, char *line, SECTION_MAP *isp_cfg_sec_map)
{
	char *tmp, *s1, *s2;
	UINT32 i, sec_name_len, sec_idx = 0, sec_num;
	struct vos_stat file_stat;

	// clear section map data
	memset(&isp_cfg_sec_map[0], 0, MAX_SECTION_MAP_NUM*sizeof(SECTION_MAP));

	for (i = 0; i < MAX_SECTION_MAP_NUM; i++) {
		isp_cfg_sec_map[i].sec_name[0] = CHAR_NULL;
	}

	pcfg_file->offset = vos_file_lseek(pcfg_file->filp, 0, SEEK_SET);

	while ((isp_cfg_read_line(pcfg_file, line, LINE_LEN) >= 0) && (sec_idx < MAX_SECTION_MAP_NUM)) {
		tmp = s1 = strchr(line, CHAR_LSQR);
		if (tmp == NULL) {
			continue;  // '[' is not found
		}

		s2 = strchr(line, CHAR_RSQR);
		if (s2 != NULL) {
			*s2 = CHAR_NULL;  // ']' is found, replace by NULL char

			sec_name_len = MIN(SECTION_LENGTH, (s2 - s1 - 1));

			memcpy(isp_cfg_sec_map[sec_idx].sec_name, ++tmp, sec_name_len);
			isp_cfg_sec_map[sec_idx].name_len = sec_name_len;
			isp_cfg_sec_map[sec_idx].start_pos = (UINT32)pcfg_file->offset;

			sec_idx++;
		}
	}

	sec_num = MIN(sec_idx, MAX_SECTION_MAP_NUM);

	// calculate section total string length
	if (sec_num >= 1) {
		for (i = 0; i < (sec_num-1); i++) {
			isp_cfg_sec_map[i].sec_len = isp_cfg_sec_map[i+1].start_pos - isp_cfg_sec_map[i].start_pos;
		}

		//isp_cfg_sec_map[sec_num-1].sec_len = (UINT32)((struct file *)pcfg_file->filp)->f_path.dentry->d_inode->i_size - isp_cfg_sec_map[sec_num-1].start_pos + 1;
		if (vos_file_fstat(pcfg_file->filp, &file_stat) == 0) {
			isp_cfg_sec_map[sec_num-1].sec_len = file_stat.st_size - isp_cfg_sec_map[sec_num-1].start_pos + 1;
		} else {
			isp_cfg_sec_map[sec_num-1].sec_len = 0;
		}
	}

	#if 0
	// print out section map
	for (i = 0; i < sec_num; i++) {
		DBG_DUMP("%s (%u) @ 0x%08X w 0x%08X\n", isp_cfg_sec_map[i].sec_name, (unsigned int)isp_cfg_sec_map[i].name_len, (unsigned int)isp_cfg_sec_map[i].start_pos, (unsigned int)isp_cfg_sec_map[i].sec_len);
	}
	#endif

	return sec_num;
}


// get a line from a file / limition : there must be a newline charactor within line length 'size'
INT32 isp_cfg_read_line(CFG_FILE_FMT *cfile, char *line, const INT32 size)
{
	char *p1, *p2;
	INT32 n;
#ifdef __KERNEL__
	((struct file *)cfile->filp)->f_pos = cfile->offset;
#else
	if (vos_file_lseek(cfile->filp, cfile->offset, SEEK_SET) != cfile->offset) {
		return -1;	// seek failure
	}
#endif

	if (vos_file_read(cfile->filp, line, size) < 1) {
		return -1;  // end of file
	}

	line[size-1] = '\0';

	// n = (INT32) line + size;
	p1 = strchr(line, CHAR_NEWLINE);
	if (p1 != NULL) {
		*p1 = CHAR_NULL;  // replace newline with NULL char
	} else {
		p1 = (char *)(line + size);  // newline char is not found
	}

	p2 = strchr(line, CHAR_RET);
	if (p2 != NULL) {
		*p2 = CHAR_NULL;   // replace the 'return' char with NULL
	} else {
		p2 = (char *)(line + size);  // 'return' char not found
	}

	if ((((char *)(line + size)) == p1) && (((char *)(line + size)) == p2)) {
		return -1;  // neither 'newline' nor 'return' char is found
	}

	n = (INT32)((p1 < p2) ? p1 : p2) - (INT32)line;  // calculate length of the line
	cfile->offset += (n + 1);  // record & move current

	if (vos_file_lseek(cfile->filp, cfile->offset, SEEK_SET) == cfile->offset) {
		// discard comments
		p1 = strchr(line, CHAR_COMMENT1);
		if (p1 != NULL) {
			*p1 = CHAR_NULL;  // discard words behind '#'
		}
		p1 = strstr(line, STR_COMMENT);
		if (p1 != NULL) {
			*p1 = CHAR_NULL;  // discard words behind '//'
		}

		return n;
	} else {
		return -1;  // file seek failure
	}
}

INT32 isp_cfg_get_field_str(const SECTION_MAP *section_map, const INT8 *section, const INT8 *key_name, INT8 *buf, CFG_FILE_FMT *cfile, const INT32 rcd_last_pos)
{
	char *key_str, *line = cfile->line_buf;
	INT8 *buf_ptr = buf;
	INT8 lastc;
	UINT32 sec_pos, sec_len;
	INT32 buf_size = LINE_LEN - 1, cp_size, n = 0;
	static UINT32 rcd_acc_ofst = 0;

	if (isp_cfg_find_section_string(section_map, (char *)section, &sec_pos, &sec_len) == FALSE) {
		return -1;  // section is not found
	}

	if (rcd_last_pos == 0) {
		key_str = isp_cfg_find_key_string((char *)key_name, cfile, line, sec_pos, sec_len);
		if (key_str == NULL) {
			return -1;  // key_name not found
		}

		rcd_acc_ofst = 0;
	} else {
		key_str = isp_cfg_find_key_string((char *)key_name, cfile, line, sec_pos + rcd_acc_ofst, sec_len - rcd_acc_ofst);
		if (key_str == NULL) {
			key_str = isp_cfg_find_key_string((char *)key_name, cfile, line, sec_pos, sec_len);
			if (key_str == NULL) {
				return -1;  // key_name not found
			}

			rcd_acc_ofst = 0;
		}
	}

	cp_size = strnlen(key_str, LINE_LEN);
	if (cp_size > buf_size) {
		cp_size = buf_size;
	}
	memcpy(buf_ptr, key_str, cp_size);

	rcd_acc_ofst += cp_size;

	// check if the last character is '\\'
	lastc = buf_ptr[cp_size-1];
	if (lastc != '\\') {
		if (buf_size) {
			buf_ptr[cp_size] = CHAR_NULL;
		}

		return cp_size;
	}

	buf_ptr += cp_size;
	buf_size -= cp_size;
	n += cp_size;

	do {
		buf_ptr--;
		buf_size++;
		n--;

		// read next line
		if (isp_cfg_read_line(cfile, line, LINE_LEN) < 0) {
			break;
		}

		cp_size = strnlen(line, LINE_LEN);
		if (cp_size > buf_size) {
			cp_size = buf_size;
		}
		memcpy(buf_ptr, line, cp_size);

		lastc = buf_ptr[cp_size-1];
		buf_ptr += cp_size;
		buf_size -= cp_size;
		n += cp_size;
	} while ((buf_size > 0) && (lastc == '\\'));

	if (buf_size) {
		*buf_ptr = CHAR_NULL;
	}

	return n;
}

INT32 isp_cfg_str2tab_char(INT8 *str, CHAR *tab, INT32 count)
{
	char *string = (char *)str;
	char *path;
	UINT32 str_len;

	path = strsep(&string, "\0");
	if (path != NULL) {
		if ((str_len = strlen(path)) != 0) {
			memcpy(tab, path, str_len);

			return 1;
		}
	}

	return 0;
}

INT32 isp_cfg_str2tab_u8(INT8 *str, UINT8 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (UINT8)(isp_simple_strtoul(token, NULL, 0) & 0xFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 isp_cfg_str2tab_s8(INT8 *str, INT8 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (INT8)(isp_simple_strtol(token, NULL, 0) & 0xFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 isp_cfg_str2tab_u16(INT8 *str, UINT16 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (UINT16)(isp_simple_strtoul(token, NULL, 0) & 0xFFFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 isp_cfg_str2tab_s16(INT8 *str, INT16 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (INT16)(isp_simple_strtol(token, NULL, 0) & 0xFFFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 isp_cfg_str2tab_u32(INT8 *str, UINT32 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = isp_simple_strtoul(token, NULL, 0);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 isp_cfg_str2tab_s32(INT8 *str, INT32 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = isp_simple_strtol(token, NULL, 0);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

#ifdef __KERNEL__
EXPORT_SYMBOL(isp_cfg_open);
EXPORT_SYMBOL(isp_cfg_close);
EXPORT_SYMBOL(isp_cfg_build_section_map);
EXPORT_SYMBOL(isp_cfg_read_line);
EXPORT_SYMBOL(isp_cfg_get_field_str);
EXPORT_SYMBOL(isp_cfg_str2tab_char);
EXPORT_SYMBOL(isp_cfg_str2tab_u8);
EXPORT_SYMBOL(isp_cfg_str2tab_s8);
EXPORT_SYMBOL(isp_cfg_str2tab_u16);
EXPORT_SYMBOL(isp_cfg_str2tab_s16);
EXPORT_SYMBOL(isp_cfg_str2tab_u32);
EXPORT_SYMBOL(isp_cfg_str2tab_s32);
#endif

