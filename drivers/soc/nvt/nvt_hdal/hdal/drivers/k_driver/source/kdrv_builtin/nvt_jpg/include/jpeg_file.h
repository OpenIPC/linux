#ifndef __JPEG_FILE_H_
#define __JPEG_FILE_H__

#include "kwrap/type.h"

#if 1	// FS API wrapper
/**
    File Handle.
*/
typedef UINT32  *FST_FILE;

/**
     @name File system open file flag
*/
//@{
#define FST_OPEN_READ                     0x00000001      ///< open file with read mode
#define FST_OPEN_WRITE                    0x00000002      ///< open file with write mode
#define FST_OPEN_EXISTING                 0x00000004      ///< if exist, open; if not exist, return fail
#define FST_OPEN_ALWAYS                   0x00000008      ///< if exist, open; if not exist, create new
//#define FST_CREATE_NEW                    0x00000010    ///< if exist, return fail; if not exist, create new
#define FST_CREATE_ALWAYS                 0x00000020      ///< if exist, open and truncate it; if not exist, create new
#define FST_SPEEDUP_CONTACCESS            0x00000100      ///< read/write random position access can enable this flag for speed up access speed
//@}

#endif
#define FST_STA_OK                  (0)                   ///<  the status is ok




FST_FILE filesys_openfile(char *filename, UINT32 flag);
INT32 filesys_closefile(FST_FILE pfile);
INT32 filesys_readfile(FST_FILE pfile, UINT8 *pbuf, UINT32 *pbufsize, UINT32 flag, INT32 *CB);
INT32 filesys_readcontfile(FST_FILE pfile, UINT8 *pbuf, UINT32 *pbufsize, UINT32 flag, INT32 *CB, loff_t offset);

#endif