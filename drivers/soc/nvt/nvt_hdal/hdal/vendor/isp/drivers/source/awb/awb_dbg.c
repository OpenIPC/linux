#include "kwrap/type.h"
#include "awb_dbg.h"

UINT32 dbg_mode[AWB_ID_MAX_NUM] = {0};
UINT32 dbg_freq[AWB_ID_MAX_NUM] = {60, 60, 60, 60, 60};

UINT32 awb_get_dbg_mode(AWB_ID id)
{
	return dbg_mode[id];
}

UINT32 awb_get_dbg_freq(AWB_ID id)
{
	return dbg_freq[id];
}

void awb_set_dbg_mode(AWB_ID id, UINT32 levle, UINT32 freq)
{
	dbg_mode[id] = levle;
	dbg_freq[id] = AWB_CLAMP(freq, 1, 4096);
}

