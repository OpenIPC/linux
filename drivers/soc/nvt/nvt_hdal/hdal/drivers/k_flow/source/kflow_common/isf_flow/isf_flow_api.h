#ifndef __ISF_FLOW_API_H
#define __ISF_FLOW_API_H

#include "isf_flow_int.h"

///////////////////////////////////////////////////////////////////////////////
extern ISF_RV isf_init(UINT32 h_isf, UINT32 p0, UINT32 p1, UINT32 p2);
extern ISF_RV isf_exit(UINT32 h_isf, UINT32 p0, UINT32 p1, UINT32 p2);
extern ISF_RV isf_abort(UINT32 h_isf);
extern ISF_RV isf_cmd(UINT32 h_isf, UINT32 cmd, UINT32 p0, UINT32 p1, UINT32 p2);

///////////////////////////////////////////////////////////////////////////////
extern ISF_RV isf_unit_set_param(UINT32 unit_nport, UINT32 param, UINT32 value);
extern ISF_RV isf_unit_get_param(UINT32 unit_nport, UINT32 param, UINT32* p_value);
extern ISF_RV isf_unit_set_struct(UINT32 unit_nport, UINT32 param, UINT32* p_struct, UINT32 size);
extern ISF_RV isf_unit_get_struct(UINT32 unit_nport, UINT32 param, UINT32* p_struct, UINT32 size);

///////////////////////////////////////////////////////////////////////////////
extern ISF_RV isf_unit_set_bind(UINT32 this_oport, UINT32 dest_iport);
extern ISF_RV isf_unit_get_bind(UINT32 this_oport, UINT32* dest_iport);
extern ISF_RV isf_unit_set_state(UINT32 this_oport, UINT32 state);
extern ISF_RV isf_unit_get_state(UINT32 this_oport, UINT32* state);

///////////////////////////////////////////////////////////////////////////////
// data operation
extern UINT32 isf_unit_new_data(UINT32 size, ISF_DATA *p_data);
extern ISF_RV isf_unit_add_data(ISF_DATA *p_data);
extern ISF_RV isf_unit_release_data(UINT32 unit_nport, ISF_DATA *p_data);
extern BOOL isf_unit_is_allow_push(UINT32 unit_nport);
extern ISF_RV isf_unit_push_data(UINT32 unit_nport, ISF_DATA *p_data, INT32 wait_ms);
extern BOOL isf_unit_is_allow_pull(UINT32 unit_nport);
extern ISF_RV isf_unit_pull_data(UINT32 unit_nport, ISF_DATA *p_data, INT32 wait_ms);
extern BOOL isf_unit_is_allow_notify(UINT32 unit_nport);
extern ISF_RV isf_unit_notify_data(UINT32 unit_nport, ISF_DATA* p_syncdata, INT32 wait_ms);
extern ISF_RV isf_unit_send_event(UINT32 unit_nport, UINT32 event, UINT32 param1, UINT32 param2, UINT32 param3);


#endif
