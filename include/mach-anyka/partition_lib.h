#ifndef        _PARTITION_LIB_H_
#define        _PARTITION_LIB_H_


#ifndef	NULL
#define	NULL	((void *) 0)
#endif

#ifdef CONFIG_MTD_AK_SPIFLASH
#define SUPPORT_SPIFLASH
#endif

#ifdef CONFIG_MTD_AK_SPINAND
#define SUPPORT_SPINAND
#endif


#define     PARTITION_NAME_LEN                  6

typedef enum
{
   PART_NO_HIDDEN = 0,
   PART_HIDDEN,
}PART_HIDDEN_TYPE;


typedef enum
{
   PART_ONLY_READ = 0,
   PART_READ_WRITE,
}PART_R_OR_W_TYPE;


typedef enum
{
   PART_DATA_TYPE = 0,
   PART_BIN_TYPE,
   PART_FS_TYPE,
}PART_TYPE;

typedef enum
{
   FS_SQSH4 = 0,
   FS_JFFS2,
   FS_YAFFS2,
}FS_TYPE;


typedef enum
{
   MEDIUM_SPINOR = 0,
   MEDIUM_SPINAND,
}MEDIUM_TYPE;

typedef enum
{
   PART_SPIFLASH = 0,
   PART_SPINAND,
   PART_SPINOR_SPINAND,
}PART_MEDIUM;




typedef struct
{
    unsigned long page_size; /*spi page size*/
    unsigned long pages_per_block;/*page per block*/
    unsigned long total_size;/*spiflash size*/
}T_SPI_INIT_INFO, *T_PSPI_INIT_INFO;

typedef struct
{
	unsigned char  name[PARTITION_NAME_LEN];	//盘符名
	unsigned long  Size;                     //分区大小
    unsigned long  ld_addr;	                //动行地址
    unsigned char  type;				        //分区属性类型：data , bin, fs
    unsigned char  r_w_flag;			        //	r_w or onlyread
    unsigned char  hidden_flag;			    //	hidden_flag or no hidden_flag
    unsigned char  backup;				    //是否备份
	unsigned char  check;                   //比较
	unsigned char  mtd_idex;                //mtd的idex
    unsigned char  fs_type;                 //文件系统类型
    unsigned char  medium_flag:1;           //0 -> not need medium_tpye,1->need medium_type
	unsigned char  medium_type:1;           //0->spi, 1->spinand
	unsigned char  rev1:6;
    unsigned long  flash_startpos;          //此分区在flash
}T_CREAT_PARTITION_INFO;


typedef struct
{
    unsigned long   file_length;    //bin:file_length  fs:未定      
    unsigned long   ld_addr;        //bin: ld_addr     fs:未定
    unsigned long   backup_pos;     //bin:backup_page  fs:未定     //字节为单位
    unsigned char   check;          //bin:check          fs:未定  
    unsigned char   mtd_idex;
    unsigned char   medium_flag:1;  //浠嬭川鏄惁鍖哄垎鐨勬爣蹇楋紝0 -銆媙ot need medium_tpye,1->need medium_type
	unsigned char   medium_type:1; //0琛ㄧずspi, 1琛ㄧずspinand
	unsigned char   rev1:6;
    unsigned char   rev2;
}T_BIN_CONFIG;

typedef struct
{
    unsigned long   file_length;    //  文件系统的镜像文件的有效长度，目前回读功能有用到    
    unsigned char   check;          //  是否比较
    unsigned char   mtd_idex;       //mtd的idex
    unsigned char   fs_type;        //文件系统的类类型
    unsigned char   medium_flag:1;  //浠嬭川鏄惁鍖哄垎鐨勬爣蹇楋紝0 -銆媙ot need medium_tpye,1->need medium_type
	unsigned char   medium_type:1;  //0琛ㄧずspi, 1琛ㄧずspinand
	unsigned char   rev1:6;
    unsigned long   rev2;          //
    unsigned long   rev3;          //  
}T_FS_CONFIG;

typedef struct
{
    unsigned long   pos;    //      
    unsigned char   table[512];
}T_PARTITION_TABLE_CONFIG;


typedef struct
{
    unsigned long  parameter1;    //bin:file_length  fs:未定      
    unsigned long  parameter2;    //bin: ld_addr     fs:未定
    unsigned long  parameter3;    //bin:backup_page  fs:未定     
    unsigned long  parameter4;    //bin:check          fs:未定     
}T_EX_PARTITION_CONFIG;

typedef struct
{
    unsigned char           type;                     //data,/bin/fs  ,  E_PARTITION_TYPE
    volatile unsigned char  r_w_flag:4;               //only read or write
    unsigned char           hidden_flag:4;            //hidden or no hidden
    unsigned char           name[PARTITION_NAME_LEN]; //分区名
    unsigned long           ksize;                    //分区大小，K为单位
    unsigned long           start_pos;                //分区的开始位置，字节为单位     
}T_PARTITION_CONFIG;


typedef struct
{
    T_PARTITION_CONFIG        partition_info;
    T_EX_PARTITION_CONFIG     ex_partition_info;
}T_PARTITION_TABLE_INFO;



/************************************************************************
 * NAME:     partition_getversion
 * FUNCTION  get partition vesion
 * PARAM:    void
 * RETURN:   version
**************************************************************************/
unsigned char *partition_getversion(void);

/************************************************************************
 * NAME:     Sflash_Creat
 * FUNCTION  partition creat
 * PARAM:    [in] T_CREAT_PARTITION_INFO *partition
 * RETURN:   success handle, fail retuen null
**************************************************************************/
void *partition_creat(T_CREAT_PARTITION_INFO *partition);


/************************************************************************
 * NAME:     Sflash_Close
 * FUNCTION  partition close
 * PARAM:    [in] T_PARTITION_TABLE_INFO *pFile
 * RETURN:   success return 0, fail retuen -1
**************************************************************************/
int partition_close(void *handle);

/************************************************************************
 * NAME:     Sflash_Open
 * FUNCTION  partition open
 * PARAM:    [in] unsigned char *partition_name
 * RETURN:   success return T_PARTITION_TABLE_INFO *pFile, fail retuen AK_NULL
**************************************************************************/
void *partition_open(unsigned char *name);

/************************************************************************
 * NAME:     Sflash_Write
 * FUNCTION  partition write
 * PARAM:    [in] T_PARTITION_TABLE_INFO *pFile, 
                   [in] unsigned char *data, 
                   [in]  unsigned LONG data_len
 * RETURN:   success handle, fail retuen null
**************************************************************************/
int partition_write(void *handle, unsigned char *data, unsigned long data_len);


/************************************************************************
 * NAME:     Sflash_Read
 * FUNCTION  partition read
 * PARAM:    [in] T_PARTITION_TABLE_INFO *pFile, 
                   out] unsigned char *data, 
                   [in]  unsigned LONG data_len
 * RETURN:   success return 0, fail retuen -1
**************************************************************************/
int partition_read(void *handle, unsigned char *data, unsigned long data_len);


/************************************************************************
 * NAME:     Partition_Get_Attr
 * FUNCTION  get partition ex_attr 
 * PARAM:   [in] T_U32pFile, 
                  [out]  T_pVOID ex_arrt
 * RETURN:   success return 0, fail retuen -1
**************************************************************************/

int partition_get_attr(void *handle, T_EX_PARTITION_CONFIG *ex_arrt);


/************************************************************************
 * NAME:     Partition_Set_Attr
 * FUNCTION  set partition ex_attr 
 * PARAM:    [in] T_U32pFile, 
                   [out]  T_pVOID ex_arrt
 * RETURN:   success return 0, fail retuen -1
**************************************************************************/
int partition_set_attr(void *handle, T_EX_PARTITION_CONFIG *ex_arrt);


/************************************************************************
 * NAME:     Partition_Get_Partition_Table
 * FUNCTION get the partition table
 * PARAM:    T_PARTITION_TABLE_CONFIG *part_talbe
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
//spiflash和spinand的page size is diffrent
int partition_get_partition_table(T_PARTITION_TABLE_CONFIG *part_talbe, PART_MEDIUM medium_type);
int partition_get_partition_backup_table(T_PARTITION_TABLE_CONFIG *part_talbe, PART_MEDIUM medium_type);


/************************************************************************
 * NAME:     partition_get_data_size
 * FUNCTION get the partition data size
 * PARAM:    T_PARTITION_TABLE_CONFIG *part_talbe
 * RETURN:    success return data_size, fail retuen 0
**************************************************************************/
unsigned long partition_get_data_size(void *handle);


/************************************************************************
 * NAME:     partition_get_ksize
 * FUNCTION get partition size ,K
 * PARAM:    void *handle
 * RETURN:    success return ksize, fail retuen 0
**************************************************************************/

unsigned long partition_get_ksize(void *handle);


///////////////////////////////////////////////////////////////////////////////////////
//此接口是提供给烧录使用，
//由于全新烧录时，会读一下mac地址回来
//但此时boot的大小如果和之前不一样，那么分区表的位置就不一样。
//所以在读mac地址时，需要设置旧的分区表，读完后，重新设回新的分区表
void partition_set_partition_startpage(unsigned long start_page, PART_MEDIUM medium_type);
void partition_set_partition_startpage_spinand(unsigned long *start_page, unsigned long *backup_start_page);


//这二个接口只应用于spiflash的
//由于优化性能的问题，烧录时partition表进行多次擦写导致占用一些时间，现在修正在烧录时，只有最后完成时，再进行把这些数据写下去
int partition_burn_partitiontab_info_init_spiflash(unsigned char burn_mode);
int partition_burn_partitiontab_info_end_spiflash(void);



#endif      //_FHA_BINBURN_H_


