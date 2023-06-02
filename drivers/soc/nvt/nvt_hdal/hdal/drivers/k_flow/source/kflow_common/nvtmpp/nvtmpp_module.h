#ifndef _NVTMPP_MODULE_H
#define _NVTMPP_MODULE_H

#define NVTMPP_MODULE_MAX_NUM          20
#define USER_MODULE                    MAKE_NVTMPP_MODULE('u', 's', 'e', 'r', 0, 0, 0, 0)

typedef enum _NVTMPP_MODULE_ER {
	NVTMPP_MODULE_ER_GET_BLK_FAIL      = 0,
	NVTMPP_MODULE_ER_LOCK_BLK_FAIL     = 1,
	NVTMPP_MODULE_ER_UNLOCK_BLK_FAIL   = 2,
	NVTMPP_MODULE_ER_MAX,
	ENUM_DUMMY4WORD(NVTMPP_MODULE_ER)
} NVTMPP_MODULE_ER;

extern void      nvtmpp_vb_module_init(void);
extern NVTMPP_ER nvtmpp_vb_module_add_one(NVTMPP_MODULE module);
extern UINT32    nvtmpp_vb_get_module_count(void);
extern INT32     nvtmpp_vb_module_to_index(NVTMPP_MODULE module);
extern NVTMPP_MODULE nvtmpp_vb_index_to_module(UINT32 module_id);
extern INT32     nvtmpp_vb_get_all_modules_list_string(CHAR *buff, UINT32 buff_len, UINT32 *module_strlen_list, BOOL align_max_str);
extern void      nvtmpp_vb_module_add_err_cnt(UINT32 module_id, NVTMPP_MODULE_ER err_type);
extern UINT32    nvtmpp_vb_module_get_err_cnt(UINT32 module_id, NVTMPP_MODULE_ER err_type);

#endif
