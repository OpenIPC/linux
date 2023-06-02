#ifdef __KERNEL__
#include <linux/syscalls.h>
#endif
#include "sen_common.h"
#include "sen_cfg.h"

//=============================================================================
// routines
//=============================================================================
#ifndef __KERNEL__
static UINT32 sen_simple_guess_base(const char *cp)
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

static UINT32 sen_strtoul(const char *cp, char **endp, UINT32 base)
{
	UINT32 result = 0;

	if (!base) {
		base = sen_simple_guess_base(cp);
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

INT32 sen_simple_strtol(char *str, char **endp, UINT32 base)
{
	if (*str == '-') {
		return -sen_simple_strtoul(str+1, endp, base);
	}

	return sen_simple_strtoul(str, endp, base);
}

UINT32 sen_simple_strtoul(char *str, char **endp, UINT32 base)
{
	return sen_strtoul(str, endp, base);
}
#endif

static INT32 sen_cfg_is_delimeter(const INT8 c)
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

static INT32 sen_cfg_find_section_string(char *sec_name, CFG_FILE_FMT *cfile, char *line)
{
	char *s1, *s2;

	while (sen_cfg_read_line(cfile, line, LINE_LEN) >= 0) {
		s1 = strchr(line, CHAR_LSQR);
		if (s1 == NULL) {
			continue;  // '[' not found
		}

		s2 = strchr(line, CHAR_RSQR);
		if (s2 != NULL) {
			*s2 = CHAR_NULL;  // ']' found, replace by NULL char
		}

		// compare the section name
		if (strcmp(++s1, sec_name)) {
			continue;  // section name does not match
		}

		return TRUE;  // section name match
	}

	return FALSE;  // specified section name is not found
}

static char *sen_cfg_find_key_string(const char *key_name, CFG_FILE_FMT *cfile, char *line)
{
	char *s1, *s2;
	INT32 key_len = strlen(key_name);

	while (sen_cfg_read_line(cfile, line, LINE_LEN) >= 0) {
		// found next section with '['
		s1 = strchr(line, CHAR_LSQR);
		if (s1 != NULL) {
			return (char *)NULL;
		}

		// search the key_name in this line
		if ((s1 = strstr(line, key_name)) == NULL) {
			continue;  // key_name is not a substring of this line
		}

		// determine the end of the candidate key_name (s1)
		if (sen_cfg_is_delimeter(s1[key_len]) == FALSE) {
			continue;  // s1 is a super set of key_name, not exactly match
		}

		// key_name matched, looking for the equal sign '=' after key_name
		if ((s2 = strchr((char *)(s1+key_len), CHAR_EQUAL)) != NULL) {
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
inline CFG_FILE_FMT *sen_cfg_open(const INT8 *file_name, const INT32 flag)
{
	CFG_FILE_FMT *cfile;

#ifdef __KERNEL__
	cfile = (CFG_FILE_FMT *)kmalloc(sizeof(CFG_FILE_FMT), GFP_KERNEL);
#else
	cfile = (CFG_FILE_FMT *)malloc(sizeof(CFG_FILE_FMT));
#endif

	if (cfile != NULL) {
		cfile->offset = 0;
		cfile->filp = vos_file_open((char *)file_name, flag, 0x777);

		if ((INT32)cfile->filp == -1) {
#ifdef __KERNEL__
			kfree(cfile);
#else
			free(cfile);
#endif

			return NULL;
		}

		return cfile;
	} else {
		return NULL;
	}
}

void sen_cfg_close(CFG_FILE_FMT *cfile)
{
	do {
		vos_file_close(cfile->filp);
#ifdef __KERNEL__
		kfree(cfile);
#else
		free(cfile);
#endif
	} while (0);
}

// get a line from a file / limition : there must be a newline charactor within line length 'size'
INT32 sen_cfg_read_line(CFG_FILE_FMT *cfile, char *line, const INT32 size)
{
	char *p1, *p2;
	INT32 n;
#ifdef __KERNEL__
	((struct file *)cfile->filp)->f_pos = cfile->offset;
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
		*p2 = CHAR_NULL;  // replace the 'return' char with NULL
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

INT32 sen_cfg_get_field_str(char *section, const INT8 *key_name, INT8 *buf, INT32 size, CFG_FILE_FMT *cfile)
{
	char *key_str, *line = cfile->line_buf;
	INT8 *buf_ptr = buf;
	INT8 lastc;
	INT32 buf_size = size, cp_size, n = 0;

	cfile->offset = vos_file_lseek(cfile->filp, 0, SEEK_SET);

	if (sen_cfg_find_section_string(section, cfile, line) == FALSE) {
		return -1;  // section not found
	}

	key_str = sen_cfg_find_key_string((char *)key_name, cfile, line);
	if (key_str == NULL) {
		return -1;  // key_name not found
	}

	cp_size = strnlen(key_str, LINE_LEN);
	if (cp_size > buf_size) {
		cp_size = buf_size;
	}
	memcpy(buf_ptr, key_str, cp_size);

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
		if (sen_cfg_read_line(cfile, line, LINE_LEN) < 0) {
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

INT32 sen_cfg_str2tab_u8(INT8 *str, UINT8 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (UINT8)(sen_simple_strtoul(token, NULL, 0) & 0xFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 sen_cfg_str2tab_s8(INT8 *str, INT8 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (INT8)(sen_simple_strtol(token, NULL, 0) & 0xFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 sen_cfg_str2tab_u16(INT8 *str, UINT16 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (UINT16)(sen_simple_strtoul(token, NULL, 0) & 0xFFFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 sen_cfg_str2tab_s16(INT8 *str, INT16 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = (INT16)(sen_simple_strtol(token, NULL, 0) & 0xFFFF);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 sen_cfg_str2tab_u32(INT8 *str, UINT32 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = sen_simple_strtoul(token, NULL, 0);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

INT32 sen_cfg_str2tab_s32(INT8 *str, INT32 *tab, INT32 count)
{
	char *string = (char *)str;
	char seps[] = ", ";
	char *token;
	INT32 i = 0;

	do {
		token = strsep(&string, seps);
		if ((token != NULL) && (*token != CHAR_NULL)) {
			tab[i++] = sen_simple_strtol(token, NULL, 0);
		}
	} while ((token != NULL) && (i < count));

	return i;
}

