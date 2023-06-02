#ifndef _SIM_VDOCDC_FILE_H_
#define _SIM_VDOCDC_FILE_H_


#include "kwrap/type.h"
#include <kwrap/file.h>

#if defined(__LINUX)
/**
    File Handle.
*/
typedef VOS_FILE FST_FILE;

/**
     @name File system open file flag
*/
//@{
#define FST_SEEK_SET					  0x00000000
#define FST_OPEN_READ                     0x00000001      ///< open file with read mode
#define FST_OPEN_WRITE                    0x00000002      ///< open file with write mode
#define FST_OPEN_EXISTING                 0x00000004      ///< if exist, open; if not exist, return fail
#define FST_OPEN_ALWAYS                   0x00000008      ///< if exist, open; if not exist, create new
//#define FST_CREATE_NEW                    0x00000010    ///< if exist, return fail; if not exist, create new
#define FST_CREATE_ALWAYS                 0x00000020      ///< if exist, open and truncate it; if not exist, create new
#define FST_SPEEDUP_CONTACCESS            0x00000100      ///< read/write random position access can enable this flag for speed up access speed
//@}

#define FST_STA_OK                  (0)                   ///<  the status is ok
#else
#include "FileSysTsk.h"
#endif

typedef struct _H26XFile{
    char   FileName[64];
    UINT32 StartOfs;

	FST_FILE    fileHdl;
}H26XFile;

void h26xFileOpen(H26XFile *pH26XFile,char *string, UINT32 flag);
int h26xFileRead(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr);
void h26xFileSeek(H26XFile *pH26XFile,UINT32 Size);
int h26xFileWrite(H26XFile *pH26XFile,UINT32 Size,UINT32 BufAddr);
void h26xFileClose(H26XFile *pH26XFile);

#endif // _SIM_VDOCDC_MEM_H_
