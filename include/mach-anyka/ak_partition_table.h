#ifndef        _AK_PARTITION_TABLE_H_
#define        _AK_PARTITION_TABLE_H_

#include <mach-anyka/partition_lib.h> 


#define KERNEL_PARTITION_IDEX  				0
#define A_PARTITION_IDEX  				    1
#define B_PARTITION_IDEX  				    2





#define PARTITION_CNT  				3

typedef struct
{
    unsigned char update_flag[6];
}T_PARTITION_NAME_INFO;

typedef struct
{
    unsigned long partition_cnt;
    T_PARTITION_NAME_INFO partition_name_info[PARTITION_CNT];//kernel, a,b
}T_PARTITION_INFO;



/*****************************************************************
 *@brief:ak_partition_table_sys_create
 *@author:cao_donghua
 *@date:2017-02-22
 *@param *part_tab:partition table sdram  first address
 *@return:int
 *@retval:0:success/ other value:fail
 ******************************************************************/

int ak_partition_table_sys_create(T_PARTITION_TABLE_CONFIG *part_tab);

#endif      //_AK_PARTITION_TABLE_H_


