#ifndef  _GRPH_LL_H
#define  _GRPH_LL_H

//#include "DrvCommon.h"
//#include "grph_lmt.h"


typedef enum {
    GRPH_LL_CMD_NULL = 0x00,
    GRPH_LL_CMD_NEXT_JOB = 0x01,
    GRPH_LL_CMD_NEXT_UPDATE = 0x02,
    GRPH_LL_CMD_UPDATE = 0x04,

    ENUM_DUMMY4WORD(GRPH_LL_CMD)
} GRPH_LL_CMD;

typedef union
{
    UINT64  reg;

    struct {
        UINT64 table_index : 8;
        UINT64 rsv : 53;
        UINT64 cmd : 3;
    } bit;
} T_GE_LL_NULL;

typedef union
{
    UINT64  reg;

    struct {
        UINT64 reg_val : 32;
        UINT64 reg_ofs : 12;
        UINT64 byte_en : 4;
        UINT64 rsv : 13;
        UINT64 cmd : 3;
    } bit;
} T_GE_LL_UPDATE;

typedef union
{
    UINT64  reg;

    struct {
        UINT64 table_index : 8;
        UINT64 next_job_addr : 32;
        UINT64 rsv : 21;
        UINT64 cmd : 3;
    } bit;
} T_GE_LL_NEXT_JOB;

typedef union
{
    UINT64  reg;

    struct {
        UINT64 rsv1 : 8;
        UINT64 next_update_addr : 32;
        UINT64 rsv2 : 21;
        UINT64 cmd : 3;
    } bit;
} T_GE_LL_NEXT_UPDATE;


#endif
