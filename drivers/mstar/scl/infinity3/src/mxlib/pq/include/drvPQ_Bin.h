////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2008-2009 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// ("MStar Confidential Information") by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
////////////////////////////////////////////////////////////////////////////////
#ifndef _DRV_PQ_BIN_H_
#define _DRV_PQ_BIN_H_


//-------------------------------------------------------------------------------------------------
//  Macro and Define
//-------------------------------------------------------------------------------------------------

#define PQ_BIN_MAX_IP       256
#define PQ_BIN_MAX_PNL      30

#define PQ_BIN_IP_ALL       0xFFFF
#define PQ_BIN_IP_NULL      0xFFFF
#define PQ_BIN_IP_COMM      0xFFFE
#define PQ_BIN_ADDR_NULL    0xFFFFFFFF

#define PQ_BIN_BANK_SIZE    1
#define PQ_BIN_ADDR_SIZE    1
#define PQ_BIN_MASK_SIZE    1

#define PQ_BIN_HEADER_LEN   16
#define PQ_BIN_VERSION_LEN  16
#define PQ_BIN_DUMMY        7

#define PQ_BIN_HEADER_LEN_1   17
#define PQ_BIN_VERSION_LEN_1  17
#define PQ_BIN_DUMMY_1        9

#define PQ_TEXT_BIN_HEADER_LEN   16
#define PQ_TEXT_BIN_VERSION_LEN  16
#define PQ_TEXT_BIN_DUMMY        8


#define PQ_BIN_ENABLE_CHECK 0

//Size of each table
#define PQ_BIN_GRULE_INFO_SIZE              14
#define PQ_BIN_GRULE_LEVEL_INFO_SIZE        6
#define PQ_BIN_XRULE_INFO_SIZE              12
#define PQ_BIN_SKIPRULE_INFO_SIZE           6
#define PQ_BIN_IP_COMM_INFO_SIZE            16
#define PQ_BIN_SOURCELUT_INFO_SIZE          8


// Item Offset of each table
// GRule
#define PQ_BIN_GRULE_HEADER_LEN             (16)
#define PQ_BIN_GRULE_RULE_NUM_LEN           (2)
#define PQ_BIN_GRULE_PANEL_NUM_LEN          (2)

#define PQ_BIN_GRULE_IP_NUM_OFFSET          (0)
#define PQ_BIN_GRULE_SOURCE_NUM_OFFSET      (PQ_BIN_GRULE_IP_NUM_OFFSET+2)
#define PQ_BIN_GRULE_GROUP_NUM_OFFSET       (PQ_BIN_GRULE_SOURCE_NUM_OFFSET+2)
#define PQ_BIN_GRULE_IPOFFSET_OFFSET        (PQ_BIN_GRULE_GROUP_NUM_OFFSET+2)
#define PQ_BIN_GRULE_RULEOFFSET_OFFSET      (PQ_BIN_GRULE_IPOFFSET_OFFSET+4)

#define PQ_BIN_GRULE_LEVEL_LVL_NUM_OFFSET   (0)
#define PQ_BIN_GRULE_LEVEL_OFFSET_OFFSET    (PQ_BIN_GRULE_LEVEL_LVL_NUM_OFFSET+2)


// XRule
#define PQ_BIN_XRULE_HEADER_LEN             (16)
#define PQ_BIN_XRULE_RULE_NUM_LEN           (2)

#define PQ_BIN_XRULE_IP_NUM_OFFSET          (0)
#define PQ_BIN_XRULE_GROUP_NUM_OFFSET       (PQ_BIN_XRULE_IP_NUM_OFFSET+2)
#define PQ_BIN_XRULE_IPOFFSET_OFFSET        (PQ_BIN_XRULE_GROUP_NUM_OFFSET+2)
#define PQ_BIN_XRULE_GROUPOFFSET_OFFSET     (PQ_BIN_XRULE_IPOFFSET_OFFSET+4)


// SkipRule
#define PQ_BIN_SKIPRULE_HEADER_LEN          (16)
#define PQ_BIN_SKIPRULE_IP_NUM_OFFSET       (0)
#define PQ_BIN_SKIPRULE_OFFSET_OFFSET       (PQ_BIN_SKIPRULE_IP_NUM_OFFSET+2)


// IP & Common
#define PQ_BIN_IP_COMM_HEADER_LEN           (16)
#define PQ_BIN_IP_COMM_IP_NUM_LEN           (2)
#define PQ_BIN_IP_COMM_TABLE_TYPE_OFFSET    (0)
#define PQ_BIN_IP_COMM_COMM_REG_NUM_OFFSET  (PQ_BIN_IP_COMM_TABLE_TYPE_OFFSET+2)
#define PQ_BIN_IP_COMM_IP_REG_NUM_OFFSET    (PQ_BIN_IP_COMM_COMM_REG_NUM_OFFSET+2)
#define PQ_BIN_IP_COMM_IP_GROUP_NUM_OFFSET  (PQ_BIN_IP_COMM_IP_REG_NUM_OFFSET+2)
#define PQ_BIN_IP_COMM_COMMOFFSET_OFFSET    (PQ_BIN_IP_COMM_IP_GROUP_NUM_OFFSET+2)
#define PQ_BIN_IP_COMM_IPOFFSET_OFFSET      (PQ_BIN_IP_COMM_COMMOFFSET_OFFSET+4)


// SourceLUT
#define PQ_BIN_SOURCELUT_HEADER_LEN         (16)
#define PQ_BIN_SOURCELUT_PANEL_NUM_LEN      (2)

#define PQ_BIN_SOURCELUT_SOURCE_NUM_OFFSET  (0)
#define PQ_BIN_SOURCELUT_IP_NUM_OFFSET      (PQ_BIN_SOURCELUT_SOURCE_NUM_OFFSET+2)
#define PQ_BIN_SOURCELUT_OFFSET_OFFSET      (PQ_BIN_SOURCELUT_IP_NUM_OFFSET+2)



#define PQ_BIN_SOURCELUT_DATA_LEN           2 // 2 bytes
#define PQ_BIN_IP_COMM_DATA_LEN             1 // 1 byte


// Text Bin
//Size of each table
#define PQ_TEXT_BIN_SRCTYPE_INFO_SIZE       6
#define PQ_TEXT_BIN_IPNAME_INFO_SIZE        6
#define PQ_TEXT_BIN_GROUPNAME_INFO_SIZE     6

// SrcType
#define PQ_TEXT_BIN_SRCTYPE_HEADER_LEN      (16)
#define PQ_TEXT_BIN_SRCTYPE_SRCNUM_LEN      (2)
#define PQ_TEXT_BIN_SRCTYPE_SRC_NUM_OFFSET  (0)
#define PQ_TEXT_BIN_SRCTYPE_OFFSET_OFFSET   (PQ_TEXT_BIN_SRCTYPE_SRC_NUM_OFFSET+2)


// IPName
#define PQ_TEXT_BIN_IPNAME_HEADER_LEN       (16)
#define PQ_TEXT_BIN_IPNAME_IPNUM_LEN        (2)
#define PQ_TEXT_BIN_IPNAME_IPNUM_OFFSET     (0)
#define PQ_TEXT_BIN_IPNAME_OFFSET_OFFSET    (PQ_TEXT_BIN_IPNAME_IPNUM_OFFSET+2)

// GroupName
#define PQ_TEXT_BIN_GROUPNAME_HEADER_LEN                (16)
#define PQ_TEXT_BIN_GROUPNAME_IPNUM_LEN                 (2)
#define PQ_TEXT_BIN_GROUPNAME_IPGROUP_GROUPNUM_OFFSET   (0)
#define PQ_TEXT_BIN_GROUPNAME_IPGROUP_OFFSET_OFFSET     (PQ_TEXT_BIN_GROUPNAME_IPGROUP_GROUPNUM_OFFSET+2)

#define PQ_TEXT_BIN_GROUPNAME_GROUPOFFSET_LEN           (4)
#define PQ_TEXT_BIN_GROUPNAME_GROUPLENGTH_LEN           (2)
#define PQ_TEXT_BIN_GROUPNAME_GROUPOFFSET_OFFSET        (0)

//-------------------------------------------------------------------------------------------------
//  enum
//-------------------------------------------------------------------------------------------------

typedef enum
{
    PQ_BIN_DISPLAY_ONE = 0,
    PQ_BIN_DISPLAY_PIP,
    PQ_BIN_DISPLAY_POP,
    PQ_BIN_DISPLAY_NUM,
}PQ_BIN_DISPLAY_TYPE;


//-------------------------------------------------------------------------------------------------
//  Type and Structure
//-------------------------------------------------------------------------------------------------

typedef struct
{
    MS_U16 u16IPNum;
    MS_U16 u16SourceNum;
    MS_U16 u16GroupNum;
    MS_U32 u32IPOffset;
    MS_U32 u32RuleOffset;
}MS_PQBin_GRule_Info;

typedef struct
{
    MS_U16 u16LvlNum;
    MS_U32 u32Offset;
}MS_PQBin_GRule_Level_Info;

typedef struct
{
    MS_U16 u16IPNum;
    MS_U16 u16GroupNum;
    MS_U32 u32IPOffset;
    MS_U32 u32GroupOffset;
}MS_PQBin_XRule_Info;


typedef struct
{
    MS_U16 u16IPNum;
    MS_U32 u32Offset;
}MS_PQBin_SkipRule_Info;


typedef struct
{
    MS_U16 u16SourceNum;
    MS_U16 u16IPNum;
    MS_U32 u32Offset;
}MS_PQBin_SourceLUT_Info;


typedef struct
{
    MS_U16 u16TableType;
    MS_U16 u16CommRegNum;
    MS_U16 u16IPRegNum;
    MS_U16 u16IPGroupNum;
    MS_U32 u32CommOffset;
    MS_U32 u32IPOffset;
}MS_PQBin_IP_Common_Info;

typedef struct
{
    MS_U8 u8BinID;
    MS_U32 u32BinStartAddress;
    char  u8Header[PQ_BIN_HEADER_LEN_1];
    char  u8Version[PQ_BIN_VERSION_LEN_1];
    MS_U32 u32StartOffset;
    MS_U32 u32EndOffset;
    MS_U8  u8Dummy[PQ_BIN_DUMMY_1];
    MS_U8  u8PQID;

    // GRule
    char   u8GRuleHader[PQ_BIN_HEADER_LEN_1];
    MS_U16 u16GRule_RuleNum;
    MS_U16 u16GRule_PnlNum;
    MS_U32 u32GRule_Offset;
    MS_U32 u32GRule_Pitch;
    MS_U32 u32GRule_Lvl_Offset;
    MS_U32 u32GRule_Lvl_Pitch;

    // XRule
    char   u8XRuleHader[PQ_BIN_HEADER_LEN_1];
    MS_U16 u16XRuleNum;
    MS_U32 u32XRule_Offset;
    MS_U32 u32XRUle_Pitch;

    // SkipRule
    char   u8SkipRuleHader[PQ_BIN_HEADER_LEN_1];
    MS_U16 u16SkipRule_IPNum;
    MS_U32 u32SkipRule_Offset;

    // IP & Common
    char   u8IP_Comm_Header[PQ_BIN_HEADER_LEN_1];
    MS_U16 u16IP_Comm_Num;
    MS_U32 u32IP_Comm_Offset;
    MS_U32 u32IP_Comm_Pitch;


    // SourceLUT
    MS_U8  u8SourceLUT_Header[PQ_BIN_HEADER_LEN_1];
    MS_U16 u16SourceLUT_PnlNum;
    MS_U32 u32SourceLUT_Offset;
    MS_U32 u32SourceLUT_Pitch;
}MS_PQBin_Header_Info;

typedef struct
{
    MS_U8 u8BinID;
    MS_U32 u32BinStartAddress;
    char  u8Header[PQ_TEXT_BIN_HEADER_LEN];
    char  u8Version[PQ_TEXT_BIN_HEADER_LEN];
    MS_U32 u32StartOffset;
    MS_U32 u32EndOffset;
    MS_U8  u8Dummy[PQ_TEXT_BIN_HEADER_LEN];

    //SourceType
    char   u8SrcTypeHeader[PQ_TEXT_BIN_HEADER_LEN];
    MS_U16 u16SrcNum;
    MS_U32 u32SrcType_Offset;
    MS_U32 u32SrcType_Pitch;

    //IPName
    char   u8IPNameHeader[PQ_TEXT_BIN_HEADER_LEN];
    MS_U16 u16IPNum;
    MS_U32 u32IPName_Offset;
    MS_U32 u32IPName_Pitch;

    //GroupName
    char   u8GroupNameHeader[PQ_TEXT_BIN_HEADER_LEN];
    MS_U16 u16GroupIPNum;
    MS_U32 u32IP_Group_Offset;
    MS_U32 u32IP_Group_pitch;
}MS_PQTextBin_Header_Info;

typedef struct
{
    MS_U32 u32TabOffset;
    MS_U16 u16RegNum;
    MS_U16 u16TableType;
    MS_U16 u16GroupNum;
    MS_U16 u16GroupIdx;
    PQ_WIN ePQWin;
}MS_PQBin_IP_Table_Info;




void MDrv_PQBin_Parsing(MS_PQBin_Header_Info *pPQBinHeader);
void MDrv_PQBin_LoadCommTable(MS_U16 u16PnlIdx, MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetIPNum(MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetCurrentTableIndex(MS_U16 u16PnlIdx, MS_U16 u16PQIPIdx, MS_U8 u8BinID);
MS_U16 MDrv_PQBin_GetTableNum(MS_U16 u16PQIPIdx, MS_PQBin_Header_Info *pPQBinHeader);
void MDrv_PQBin_LoadTableData(MS_U16 u16PnlIdx, MS_U16 u16TabIdx, MS_U16 u16PQIPIdx, MS_PQBin_Header_Info *pPQBinHeader, MS_U8 *pTable, MS_U16 u16TableSize);
void MDrv_PQBin_LoadTable(MS_U16 u16PnlIdx, MS_U16 u16TabIdx, MS_U16 u16PQIPIdx, MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetTableIndex(MS_U16 u16PQSrcType, MS_U16 u16PQIPIdx, MS_U16 u16PQPnlIdx, MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetSkipRule(MS_U16 u16PQIPIdx, MS_PQBin_Header_Info *pPQBinHeader);
void MDrv_PQBin_LoadTableBySrcType(MS_U16 u16PQSrcType, MS_U16 u16PQIPIdx, MS_U16 u16PQPnlIdx, MS_PQBin_Header_Info *pPQBinHeader);


MS_U16 MDrv_PQBin_GetXRuleTableIndex(MS_U16 u16XRuleType, MS_U16 u16XRuleIdx, MS_U16 u16XRuleIP, MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetXRuleIPIndex(MS_U16 u16XRuleType, MS_U16 u16XRuleIP, MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetXRuleIPNum(MS_U16 u16XRuleType, MS_PQBin_Header_Info *pPQBinHeader);


MS_U16 MDrv_PQBin_GetGRule_LevelIndex(MS_U16 u16PnlIdx, MS_U16 u16GRuleType, MS_U16 u16GRuleLvlIndex, MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetGRule_IPIndex(MS_U16 u16GRuleType, MS_U16 u16GRuleIPIndex, MS_PQBin_Header_Info *pPQBinHeader);
MS_U16 MDrv_PQBin_GetGRule_TableIndex(MS_U16 u16GRuleType, MS_U16 u16PQSrcType, MS_U16 u16PQ_NRIdx, MS_U16 u16GRuleIPIndex, MS_PQBin_Header_Info *pPQBinHeader);

void MDrv_PQBin_SetDisplayType(PQ_BIN_DISPLAY_TYPE enDisplayType, PQ_WIN ePQWin);
PQ_BIN_DISPLAY_TYPE MDrv_PQBin_GetDisplayType(PQ_WIN ePQWin);

void  MDrv_PQBin_SetPanelID(MS_U8 u8PnlID, PQ_WIN ePQWin);
MS_U8 MDrv_PQBin_GetPanelIdx(PQ_WIN ePQWin);

void MDrv_PQBin_Set_MLoadEn(MS_BOOL bEn);

void MDrv_PQBin_CheckCommTable(MS_U16 u16PnlIdx, MS_PQBin_Header_Info * pPQBinHeader);
void MDrv_PQBin_CheckTableBySrcType(MS_U16 u16PQSrcType, MS_U16 u16PQIPIdx, MS_U16 u16PQPnlIdx, MS_PQBin_Header_Info * pPQBinHeader);


MS_BOOL MDrv_PQTextBin_Parsing(MS_PQTextBin_Header_Info *pPQTextBinHeader);
MS_U32 MDrv_PQTextBin_GetSrcType(MS_U16 u16SrcIdx, MS_PQTextBin_Header_Info *pPQTextBinHeader);
MS_U32 MDrv_PQTextBin_GetIPName(MS_U16 u16PQIPIdx, MS_PQTextBin_Header_Info *pPQTextBinHeader);
MS_U32 MDrv_PQTextBin_GetTableName(MS_U16 u16PQIPIdx, MS_U16 u16TabIdx, MS_PQTextBin_Header_Info *pPQTextBinHeader);
PQ_WIN MDrv_PQBin_TransToPQWin(MS_U8 u8BinID);
MS_BOOL MDrv_PQBin_LoadPictureSetting(MS_U16 u16PnlIdx, MS_U16 u16TabIdx, MS_U16 u16PQIPIdx, MS_PQBin_Header_Info *pPQBinHeader, void *pTable, MS_U16 u16TableSize);
void MDrv_PQBin_Clear_SRAM_Table_Index(void);

#endif
