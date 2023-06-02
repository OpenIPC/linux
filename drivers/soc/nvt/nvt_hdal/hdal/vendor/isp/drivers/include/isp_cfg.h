#ifndef _ISP_CFG_H_
#define _ISP_CFG_H_

#ifdef __KERNEL__
#include <linux/slab.h>
#endif
#include "kwrap/file.h"

//=============================================================================
// struct & definition
//=============================================================================
#define MAX_PATH_NAME_LENGTH   256
// set line length in configuration files
#define LINE_LEN               768    // depending on max. string length at one line
#define SECTION_LENGTH         32
#define KEY_LENGTH             32
#define MAX_SECTION_MAP_NUM    300    // depending on your section number

#define CHAR_0                 '0'
#define CHAR_9                 '9'
#define CHAR_SPACE             ' '
#define CHAR_NEWLINE           '\n'
#define CHAR_CR                '\n'
#define CHAR_RET               '\r'
#define CHAR_NULL              '\0'
#define CHAR_TAB               '\t'
#define CHAR_EQUAL             '='
#define CHAR_LSQR              '['
#define CHAR_RSQR              ']'
#define CHAR_COMMENT1          '#'
#define CHAR_COMMENT2          '/'
#define STR_COMMENT            "//"

// utilitiy macros
#define IS_DIGIT(c)            (((c) >= CHAR_0) && ((c) <= CHAR_9))
#define IS_HEX(c)              ((((c) >= 0x41) && ((c) <= 0x46)) || (((c) >= 0x61) && ((c) <= 0x66)))
#define IS_NOT_DIGIT(c)        (((c) < CHAR_0) || ((c) > CHAR_9))
#define IS_SPACE(c)            ((c == CHAR_SPACE))
#define TOLOWER(c)             ((((c) >= 'A') && ((c) <= 'Z')) ? ((c) + 0x20) : (c))

#ifdef __KERNEL__
#define isp_simple_strtoul     simple_strtoul
#define isp_simple_strtol      simple_strtol
#endif
#define CFG_STR2INT(str)       isp_simple_strtol(str, NULL, 0)     // convert string to INT32
#define CFG_STR2UINT(str)      isp_simple_strtoul(str, NULL, 0)    // convert string to UINT32

typedef struct _CFG_FILE_FMT {
	VOS_FILE filp;
	vos_off_t offset;
	char line_buf[LINE_LEN];
} CFG_FILE_FMT;

typedef struct _SECTION_MAP{
	INT8 sec_name[SECTION_LENGTH];
	UINT32 name_len;
	UINT32 start_pos;
	UINT32 sec_len;
} SECTION_MAP;


//=============================================================================
// extern functions
//=============================================================================
extern CFG_FILE_FMT *isp_cfg_open(const INT8 *file_name, const INT32 flag);
extern void isp_cfg_close(CFG_FILE_FMT *cfile);
extern UINT32 isp_cfg_build_section_map(CFG_FILE_FMT *pcfg_file, char *line, SECTION_MAP *isp_cfg_sec_map);
extern INT32 isp_cfg_read_line(CFG_FILE_FMT *cfile, char *line, const INT32 size);
// read the value of key name in string form
extern INT32 isp_cfg_get_field_str(const SECTION_MAP *section_map, const INT8 *section, const INT8 *key_name, INT8 *buf, CFG_FILE_FMT *cfile, const INT32 rcd_last_pos);
extern INT32 isp_cfg_str2tab_char(INT8 *str, CHAR *tab, INT32 count);     // convert string to CHAR array
extern INT32 isp_cfg_str2tab_u8(INT8 *str, UINT8 *tab, INT32 count);      // convert string to UINT8 array
extern INT32 isp_cfg_str2tab_s8(INT8 *str, INT8 *tab, INT32 count);       // convert string to INT8 array
extern INT32 isp_cfg_str2tab_u16(INT8 *str, UINT16 *tab, INT32 count);    // convert string to UINT16 array
extern INT32 isp_cfg_str2tab_s16(INT8 *str, INT16 *tab, INT32 count);     // convert string to INT16 array
extern INT32 isp_cfg_str2tab_u32(INT8 *str, UINT32 *tab, INT32 count);    // convert string to UINT32 array
extern INT32 isp_cfg_str2tab_s32(INT8 *str, INT32 *tab, INT32 count);     // convert string to INT32 array

#ifndef __KERNEL__
extern INT32 isp_simple_strtol(char *str, char **endp, UINT32 base);
extern UINT32 isp_simple_strtoul(char *str, char **endp, UINT32 base);
#endif

#endif

