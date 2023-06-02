#ifndef _VDODISPID_H
#define _VDODISPID_H

#include "kwrap/type.h"
#include "kwrap/flag.h"
#include "kwrap/semaphore.h"
#include "kwrap/task.h"


extern THREAD_HANDLE VDODISP_TSK_ID_0; ///< task id
extern THREAD_HANDLE VDODISP_IDE_ID_0; ///< task id
extern ID VDODISP_FLG_ID_0; ///< flag id
extern SEM_HANDLE VDODISP_SEM_ID_0; ///< semophore id
extern THREAD_HANDLE VDODISP_TSK_ID_1; ///< task id
extern THREAD_HANDLE VDODISP_IDE_ID_1; ///< task id
extern ID VDODISP_FLG_ID_1; ///< flag id
extern SEM_HANDLE VDODISP_SEM_ID_1; ///< semophore id
#define PRI_VDODISP             3


#endif //_VDODISPID_H
