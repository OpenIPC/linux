#ifndef        _PARTITION_INIT_H_
#define        _PARTITION_INIT_H_

#include "nand_list.h"


typedef struct
{
    unsigned long    chip_id;
    unsigned long    total_size;             ///< flash total size in bytes
    unsigned long	 page_size;       ///< total bytes per page
    unsigned long	 program_size;    ///< program size at 02h command
    unsigned long	 erase_size;      ///< erase size at d8h command 
    unsigned long	 clock;           ///< spi clock, 0 means use default clock 
    
    //chip character bits:
    //bit 0: under_protect flag, the serial flash under protection or not when power on
    //bit 1: fast read flag    
    unsigned char    flag;            ///< chip character bits
    unsigned char	 protect_mask;    ///< protect mask bits in status register:BIT2:BP0, 
                             //BIT3:BP1, BIT4:BP2, BIT5:BP3, BIT7:BPL
    unsigned char    reserved1;
    unsigned char    reserved2;
    unsigned char    des_str[32];		   //ÃèÊö·û                                    
}T_SFLASH_INFO;


/************************************************************************
 * NAME:     FHA_Erase
 * FUNCTION  callback function, medium erase
 * PARAM:    [in] nChip--meidum chip
 *           [in] nPage--medium page
 * RETURN:   success return 0, fail retuen -1
**************************************************************************/
typedef  int (*FHA_Erase)(unsigned long nChip,  unsigned long block); 

/************************************************************************
 * NAME:     FHA_Write
 * FUNCTION  callback function, medium write
 * PARAM:    [in] nChip-----meidum chip
 *           [in] nPage-----medium page
 *           [in] pData-----need to write data pointer addr
 *           [in] nDataLen--need to write data length
 *                          nand(unit byte)
 *                          SD(unit sector count(1sec = 512byte))
 *                          SPI(unit page count, page size in platform define, generally is 256bytes)
 *           [in] pOob------Spare area£ºOut Of Band, only nand use
 *           [in] nOobLen---Spare area length
 *           [in] eDataType-burn medium data type
 *                          nand -- E_FHA_DATA_TYPE
 *                          SD----- MEDIUM_EMMC
 *                          SPI---- MEDIUM_SPIFLASH
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
typedef int (*FHA_Write)(unsigned long nChip, unsigned long nPage, const unsigned char *pData, unsigned long nDataLen,  unsigned char *pOob, unsigned long nOobLen, unsigned long eDataType);

/************************************************************************
 * NAME:     FHA_Read
 * FUNCTION  callback function, medium read
 * PARAM:    [in] nChip-----meidum chip
 *           [in] nPage-----medium page
 *           [out]pData-----need to read data pointer addr
 *           [in] nDataLen--need to ren data length
 *                          nand(unit byte)
 *                          SD(unit sector count(1sec = 512byte))
 *                          SPI(unit page count, page size in platform define, generally is 256bytes)
 *           [out]pOob------Spare area£ºOut Of Band, only nand use
 *           [in] nOobLen---Spare area length
 *           [in] eDataType-burn medium data type
 *                          nand -- E_FHA_DATA_TYPE
 *                          SD----- MEDIUM_EMMC
 *                          SPI---- MEDIUM_SPIFLASH
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
typedef int (*FHA_Read)(unsigned long nChip,  unsigned long nPage, unsigned char *pData, unsigned long nDataLen,  unsigned char *pOob, unsigned long nOobLen , unsigned long eDataType);

/************************************************************************
 * NAME:     FHA_ReadNandBytes
 * FUNCTION  callback function, nand read no ECC
 * PARAM:    [in] nChip-------meidum chip
 *           [in] rowAddr-----nand physical row addr
 *           [in] columnAddr--nand physical cloumn addr
 *           [out] pData------need to read data pointer addr
 *           [in] nDataLen----need to ren data length
 *                            nand(unit byte)
 * RETURN:    success return 0, fail retuen -1
**************************************************************************/
typedef int (*FHA_ReadNandBytes)(unsigned long nChip, unsigned long rowAddr, unsigned long columnAddr, unsigned char *pData, unsigned long nDataLen);

typedef void *(*FHA_RamAlloc)(unsigned long size);
typedef void *(*FHA_RamFree)(void * var);
typedef void *(*FHA_MemSet)(void * pBuf, signed long value, unsigned long count);
typedef void *(*FHA_MemCpy)(void * dst, const void * src, unsigned long count);
typedef signed long   (*FHA_MemCmp)(const void * pbuf1, const void * pbuf2, unsigned long count);
typedef void *(*FHA_MemMov)(void * dst, const void * src, unsigned long count);
typedef signed long    (*FHA_Printf)(const signed char *s, ...);

typedef struct tag_FHA_LibCallback
{
    FHA_Erase Erase;
    FHA_Write Write;
    FHA_Read  Read;
    FHA_ReadNandBytes ReadNandBytes;
    FHA_RamAlloc RamAlloc;
    FHA_RamFree  RamFree;
    FHA_MemSet   MemSet;
    FHA_MemCpy   MemCpy;
    FHA_MemCmp   MemCmp;
    FHA_MemMov   MemMov;
    FHA_Printf   Printf;
}T_FHA_LIB_CALLBACK, *T_PFHA_LIB_CALLBACK;



/************************************************************************
 * NAME:     Sflash_Init
 * FUNCTION  partition init
 * PARAM:    [in] T_pVOID SpiInfo
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
int partition_init(T_PFHA_LIB_CALLBACK pCB, void *pPhyInfo, unsigned long table_page);

#endif      //_FHA_BINBURN_H_


