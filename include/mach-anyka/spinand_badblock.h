#ifndef _SPINAND_BADBLOCK_H_
#define _SPINAND_BADBLOCK_H_



#include "partition_lib.h"


#define HEAD_SIZE 8               //asa head info size


#define ASA_MAX_BLOCK_TRY 50    //define asa block max try use
#define ASA_BLOCK_COUNT 10       //define asa block max count

#define ASA_FORMAT_NORMAL   0
#define ASA_FORMAT_EWR      1
#define ASA_FORMAT_RESTORE  2

typedef enum
{
//nand flash
    NAND_TYPE_UNKNOWN,
    NAND_TYPE_SAMSUNG,
    NAND_TYPE_HYNIX,
    NAND_TYPE_TOSHIBA,
    NAND_TYPE_TOSHIBA_EXT,
    NAND_TYPE_MICRON,
    NAND_TYPE_ST,
    NAND_TYPE_MICRON_4K,
    NAND_TYPE_MIRA,

//  spi nand flash
    SPINAND_TYPE_MIRA=128,
    SPINAND_TYPE_GD,
}E_NAND_TYPE;




typedef struct {
 unsigned char head_str[8];
 unsigned long verify[2];
 unsigned long item_num;
 unsigned long info_end;
}
T_ASA_HEAD;

typedef struct
{
    unsigned short page_start;
    unsigned short page_count;
    unsigned short info_start;
    unsigned short info_len;
 }T_ASA_ITEM;


 typedef struct
{
    unsigned char file_name[8];
    unsigned long file_length;
    unsigned long start_page;
    unsigned long end_page;
}T_ASA_FILE_INFO;


typedef struct tag_ASA_Param
{
    unsigned short   PagePerBlock;       
    unsigned short   BytesPerPage;
    unsigned short   BlockNum;           //blocks of one chip 
}T_ASA_PARAM;

typedef struct tag_ASA_Block
{
    unsigned char  asa_blocks[ASA_BLOCK_COUNT];   //保存安全区块表的数组
    unsigned char  asa_count;                     //初始化以后用于安全区的block个数，仅指安全区可用块
    unsigned char  asa_head;                      //安全区块数组中数据最新的块的索引 
    unsigned char  asa_block_cnt;                 //安全区所有块个数，包含坏块
    unsigned long write_time;                    //块被擦写次数
}T_ASA_BLOCK;



/************************************************************************
 * NAME:     spinand_babblock_tbl_init
 * FUNCTION  bad block table init
 * PARAM:    [in] unsigned char burn_flag   
                   [in] unsigned long asa_start_block
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
int  spinand_babblock_tbl_init(unsigned char new_burn_mode, unsigned char burn_flag, unsigned long asa_start_block);


/************************************************************************
 * NAME:     spinand_babblock_tbl_creat
 * FUNCTION  bad block table creat
 * PARAM:    [in] unsigned long type
                   [in] unsigned long asa_start_block
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
int spinand_babblock_tbl_creat(unsigned long type, unsigned long asa_start_block);

/************************************************************************
 * NAME:     spinand_set_badblock
 * FUNCTION  set bad block table
 * PARAM:    [in] unsigned long block
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
int  spinand_set_badblock(unsigned long block);

/************************************************************************
 * NAME:     spinand_get_badblock
 * FUNCTION  get bad block table data
 * PARAM:    [in] unsigned long start_block
                   [out] unsigned char pData[]  -------- buffer used to store bad blocks information data
                   [in] unsigned long blk_cnt
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
int  spinand_get_badblock(unsigned long start_block, unsigned char pData[], unsigned long blk_cnt);

/************************************************************************
 * NAME:     spinand_is_badblock
 * FUNCTION  check is or not bad block 
 * PARAM:    [in] unsigned long block
 * RETURN:    is return 0, fail retuen -1
**************************************************************************/
int  spinand_is_badblock(unsigned long block);


/************************************************************************
 * NAME:     spinand_check_babblock
 * FUNCTION  check is or not bad block 
 * PARAM:    [in] unsigned long block
 * RETURN:    is return 0, fail retuen -1
**************************************************************************/
//在没有坏块表的情况下进行判断此块是否坏块
//由此此接口会进行读写擦的操作，
//所以此块只能作为全新烧录时使用，
int spinand_check_babblock(unsigned long block);
int spinand_check_Initial_babblock(unsigned long block);

/************************************************************************
 * NAME:     spinand_read_asa_data
 * FUNCTION  read asa data
 * PARAM:    [in]unsigned char *data, unsigned long data_len
 * RETURN:    is return 0, fail retuen -1
**************************************************************************/
int spinand_read_asa_data(unsigned char *data, unsigned long data_len);


/************************************************************************
 * NAME:     spinand_write_asa_data
 * FUNCTION  write asa data
 * PARAM:    unsigned char *data, unsigned long data_len
 * RETURN:    is return 0, fail retuen -1
**************************************************************************/
int spinand_write_asa_data(unsigned char *data, unsigned long data_len);




#endif

