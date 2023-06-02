////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2007 MStar Semiconductor, Inc.
// All rights reserved.
//
// Unless otherwise stipulated in writing, any and all information contained
// herein regardless in any format shall remain the sole proprietary of
// MStar Semiconductor Inc. and be kept in strict confidence
// (?œMStar Confidential Information?? by the recipient.
// Any unauthorized act including without limitation unauthorized disclosure,
// copying, use, reproduction, sale, distribution, modification, disassembling,
// reverse engineering and compiling of the contents of MStar Confidential
// Information is unlawful and strictly prohibited. MStar hereby reserves the
// rights to any and all damages, losses, costs and expenses resulting therefrom.
//
///

#define MS_OS_C

/******************************************************************************/
/*                           Header Files                                     */
/* ****************************************************************************/
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/semaphore.h>
#include <linux/spinlock.h>
#include <linux/kthread.h>
#include <linux/interrupt.h>
#include <linux/jiffies.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/vmalloc.h>          /* seems do not need this */
#include "MsCommon.h"
#include "MsTypes.h"
#include "MsIRQ.h"
#include "MsOS.h"
#include "halCHIP.h"


/********************************************************************************/
/*                           Macro                                              */
/********************************************************************************/
#define MSOS_MUTEX_USE_SEM

#define MSOS_ID_PREFIX              0x76540000
#define MSOS_ID_PREFIX_MASK         0xFFFF0000
#define MSOS_ID_MASK                0x0000FFFF //~MSOS_ID_PREFIX_MASK

#define HAS_FLAG(flag, bit)        ((flag) & (bit))
#define SET_FLAG(flag, bit)        ((flag)|= (bit))
#define RESET_FLAG(flag, bit)      ((flag)&= (~(bit)))


#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif // #ifndef MIN

#ifndef MAX
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

#ifdef MSOS_MEMPOOL_MAX
#undef MSOS_MEMPOOL_MAX
#define MSOS_MEMPOOL_MAX 2
#endif // #ifdef MSOS_MEMPOOL_MAX



/********************************************************************************/
/*                           Constant                                           */
/********************************************************************************/


/******************************************************************************/
/*                           Enum                                             */
/******************************************************************************/


/******************************************************************************/
/*                           Global Variables                                   */
/******************************************************************************/
//
// Mutex
//
typedef struct
{
    MS_BOOL             bUsed;
#ifdef MSOS_MUTEX_USE_SEM
    struct semaphore stMutex;
#else
    spinlock_t       stMutex;
#endif
    MS_U8               u8Name[MAX_MUTEX_NAME_LENGTH];
} MsOS_Mutex_Info;
typedef struct
{
    MS_BOOL             bUsed;
    spinlock_t       stMutex;
    MS_U8               u8Name[MAX_MUTEX_NAME_LENGTH];
} MsOS_Spinlock_Info;

static MsOS_Mutex_Info          _MsOS_Mutex_Info[MSOS_MUTEX_MAX];
static MsOS_Spinlock_Info       _MsOS_Spinlock_Info[MSOS_SPINLOCK_MAX];

static                          DEFINE_SPINLOCK(_MsOS_Mutex_Mutex);
#define MUTEX_MUTEX_LOCK()      spin_lock(&_MsOS_Mutex_Mutex)
#define MUTEX_MUTEX_UNLOCK()    spin_unlock(&_MsOS_Mutex_Mutex)

//
// Task Management
//
typedef struct
{
    MS_BOOL             bUsed;
    struct task_struct* pstThreadInfo;
} MsOS_Task_Info;

static MsOS_Task_Info   _MsOS_Task_Info[MSOS_TASK_MAX];

#ifdef MSOS_MUTEX_USE_SEM
struct semaphore _MsOS_Task_Mutex;
#define MUTEX_TASK_LOCK()    down(&_MsOS_Task_Mutex)
#define MUTEX_TASK_UNLOCK()  up(&_MsOS_Task_Mutex)
#else
static                       DEFINE_SPINLOCK(_MsOS_Task_Mutex);
#define MUTEX_TASK_LOCK()    spin_lock(&_MsOS_Task_Mutex)
#define MUTEX_TASK_UNLOCK()  spin_unlock(&_MsOS_Task_Mutex)
#endif

//
// Event Group
//
typedef struct
{
    MS_BOOL                     bUsed;
    MS_U32                      u32EventGroup;
    spinlock_t                  stMutexEvent;
    wait_queue_head_t           stSemaphore;
    // pthread_cond_t              stSemaphore; // ?????????????
} MsOS_EventGroup_Info;

static MsOS_EventGroup_Info     _MsOS_EventGroup_Info[MSOS_EVENTGROUP_MAX];
static                          DEFINE_SPINLOCK(_MsOS_EventGroup_Mutex);
#define EVENT_MUTEX_LOCK()      spin_lock(&_MsOS_EventGroup_Mutex)
#define EVENT_MUTEX_UNLOCK()    spin_unlock(&_MsOS_EventGroup_Mutex)


// Timer
//
typedef struct
{
    MS_BOOL             bUsed;
    TimerCb             pTimerCb;
    struct timer_list   timer;
    int                 period;
    int                 first;
} MsOS_Timer_Info;
static MsOS_Timer_Info  _MsOS_Timer_Info[MSOS_TIMER_MAX];
static                          DEFINE_SPINLOCK(_MsOS_Timer_Mutex);
#define TIMER_MUTEX_LOCK()      spin_lock(&_MsOS_Timer_Mutex)
#define TIMER_MUTEX_UNLOCK()    spin_unlock(&_MsOS_Timer_Mutex)

/******************************************************************************/
/*                           Local Variables                                    */
/******************************************************************************/

/******************************************************************************/
/*               P r i v a t e    F u n c t i o n s                             */
/******************************************************************************/
void MsOS_SetPollWait(void *filp, void *pWaitQueueHead, void *pstPollQueue)
{
    poll_wait((struct file *)filp, (wait_queue_head_t *)pWaitQueueHead, (struct poll_table_struct *)pstPollQueue);
}
MS_U32 MsOS_GetSystemTime (void)
{
    struct timespec         ts;

    getnstimeofday(&ts);
    return ts.tv_sec* 1000+ ts.tv_nsec/1000000;
}
MS_U64 MsOS_GetSystemTimeStamp (void)
{
    struct timeval         tv;
    MS_U64 u64TimeStamp;
    do_gettimeofday(&tv);
    u64TimeStamp =(MS_U64)tv.tv_sec* 1000000ULL+ (MS_U64)tv.tv_usec;
    return u64TimeStamp;
}

MS_U32 MsOS_Timer_DiffTimeFromNow(MS_U32 u32TaskTimer) //unit = ms
{
    return (MsOS_GetSystemTime() - u32TaskTimer);
}

void MsOS_DelayTask (MS_U32 u32Ms)
{
    //sleep in spinlock will cause deadlock
#ifdef MSOS_MUTEX_USE_SEM
    msleep_interruptible((unsigned int)u32Ms);
#else
    mdelay(u32Ms);
#endif
}


void MsOS_DelayTaskUs (MS_U32 u32Us)
{
    struct timespec TS1, TS2;
    getnstimeofday(&TS1);
    getnstimeofday(&TS2);

    while((TS2.tv_nsec - TS1.tv_nsec)< (u32Us * 1000UL))
    {
        getnstimeofday(&TS2);
    }
#if 0
    struct timespec req, rem;

    req.tv_sec = 0;
    req.tv_nsec = (long) (u32Us*1000UL);

    while(1)
    {
        int err;

        err = nanosleep(&req, &rem);
        if(err==-1)
        {
            #if 0
            switch(errno)
            {
                case EINTR:
                    req.tv_sec = rem.tv_sec;
                    req.tv_nsec = rem.tv_nsec;
                    continue;
                default:
                    printk("nanosleep is interrupted: %d\n", errno);
            }
            #endif
        }

        break;
    }
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//
// Interrupt management
//
//-------------------------------------------------------------------------------------------------
/// Attach the interrupt callback function to interrupt #
/// @param  eIntNum \b IN: Interrupt number in enumerator InterruptNum
/// @param  pIntCb  \b IN: Interrupt callback function
/// @return TRUE : succeed
/// @return FALSE :  fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_AttachInterrupt (InterruptNum eIntNum, InterruptCb pIntCb,unsigned long flags,const char *name)
{
    int i;
    i = request_irq(eIntNum, (irq_handler_t)pIntCb, flags, name, NULL);

    return i;
}

//-------------------------------------------------------------------------------------------------
/// Detach the interrupt callback function from interrupt #
/// @param  eIntNum \b IN: Interrupt number in enumerator InterruptNum
/// @return TRUE : succeed
/// @return FALSE :  fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_DetachInterrupt (InterruptNum eIntNum)
{
    // PTH_RET_CHK(pthread_mutex_lock(&_ISR_Mutex));
    //HAL_IRQ_Detech((int)eIntNum); TODo!!
    // PTH_RET_CHK(pthread_mutex_unlock(&_ISR_Mutex));
    free_irq(eIntNum, NULL);
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Enable (unmask) the interrupt #
/// @param  eIntNum \b IN: Interrupt number in enumerator InterruptNum
/// @return TRUE : succeed
/// @return FALSE :  fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_EnableInterrupt (InterruptNum eIntNum)
{
    enable_irq((int)eIntNum);
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Disable (mask) the interrupt #
/// @param  eIntNum \b IN: Interrupt number in enumerator InterruptNum
/// @return TRUE : succeed
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_DisableInterrupt (InterruptNum eIntNum)
{
    disable_irq((int)eIntNum);
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Disable all interrupts (including timer interrupt), the scheduler is disabled.
/// @return Interrupt register value before all interrupts disable
//-------------------------------------------------------------------------------------------------
MS_U32 MsOS_DisableAllInterrupts(void)
{
#if 0
    return CHIP_DisableAllInterrupt() ;
#else
    return 0;
#endif
}

//-------------------------------------------------------------------------------------------------
/// Restore the interrupts from last MsOS_DisableAllInterrupts.
/// @param  u32OldInterrupts \b IN: Interrupt register value from @ref MsOS_DisableAllInterrupts
/// @return TRUE : succeed
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_RestoreAllInterrupts(MS_U32 u32OldInterrupts)
{
    return TRUE;
}
//-------------------------------------------------------------------------------------------------
/// Enable all CPU interrupts.
/// @return TRUE : succeed
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_EnableAllInterrupts(void)
{
#if 0
    return CHIP_EnableAllInterrupt() ;
#else
    return TRUE;
#endif
}

//-------------------------------------------------------------------------------------------------
/// In Interuupt Context or not
/// @return TRUE : Yes
/// @return FALSE : No
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_In_Interrupt (void)
{
#if 0
    return CHIP_InISRContext();
#else
    return FALSE;
#endif
}

//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------
//-------------------------------------------------------------------------------------------------

//
// Mutex
//
//-------------------------------------------------------------------------------------------------
/// Create a mutex in the unlocked state
/// @param  eAttribute  \b IN: E_MSOS_FIFO: suspended in FIFO order
/// @param  pMutexName  \b IN: mutex name
/// @param  u32Flag  \b IN: process data shared flag
/// @return >=0 : assigned mutex Id
/// @return <0 : fail
/// @note   A mutex has the concept of an owner, whereas a semaphore does not.
///         A mutex provides priority inheritance protocol against proiorty inversion, whereas a binary semaphore does not.
//-------------------------------------------------------------------------------------------------
MS_S32 MsOS_CreateMutex ( MsOSAttribute eAttribute, char *pMutexName1, MS_U32 u32Flag)
{
    MS_S32 s32Id, s32LstUnused = MSOS_MUTEX_MAX;
    MS_U8 pMutexName[MAX_MUTEX_NAME_LENGTH];
    MS_U32 u32MaxLen;

    if (NULL == pMutexName1)
    {
        return -1;
    }
    if (strlen(pMutexName1) >= (MAX_MUTEX_NAME_LENGTH-1))
    {
        printk("%s: Warning strlen(%s) is longer than MAX_MUTEX_NAME_LENGTH(%d). Oversize char will be discard.\n",
        __FUNCTION__,pMutexName1,MAX_MUTEX_NAME_LENGTH);
    }
    if (0 == (u32MaxLen = MIN(strlen(pMutexName1), (MAX_MUTEX_NAME_LENGTH-1))))
    {
        return -1;
    }
    strncpy((char*)pMutexName, (const char*)pMutexName1, u32MaxLen);
    pMutexName[u32MaxLen] = '\0';

    MUTEX_MUTEX_LOCK();
    for(s32Id=0;s32Id<MSOS_MUTEX_MAX;s32Id++)
    {
        // if (PTHREAD_PROCESS_SHARED == s32Prop) // @FIXME: Richard: is the mutex name always used as an id, regardless of process shared/private property?
        {
            if(TRUE == _MsOS_Mutex_Info[s32Id].bUsed)
            {
                if (0== strcmp((const char*)_MsOS_Mutex_Info[s32Id].u8Name, (const char*)pMutexName))
                {
                    break;
                }
            }
        }
        if (FALSE==_MsOS_Mutex_Info[s32Id].bUsed  && MSOS_MUTEX_MAX==s32LstUnused)
        {
            s32LstUnused = s32Id;
        }
    }
    if ((MSOS_MUTEX_MAX==s32Id) && (MSOS_MUTEX_MAX>s32LstUnused))
    {
        _MsOS_Mutex_Info[s32LstUnused].bUsed = TRUE;
        strcpy((char*)_MsOS_Mutex_Info[s32LstUnused].u8Name, (const char*)pMutexName);
#ifdef MSOS_MUTEX_USE_SEM
        sema_init(&_MsOS_Mutex_Info[s32LstUnused].stMutex, 1);
#else
        spin_lock_init(&_MsOS_Mutex_Info[s32LstUnused].stMutex);
#endif
        s32Id = s32LstUnused;
    }
    MUTEX_MUTEX_UNLOCK();

    if(MSOS_MUTEX_MAX <= s32Id)
    {
        return -1;
    }

    s32Id |= MSOS_ID_PREFIX;

    return s32Id;
}
MS_S32 MsOS_CreateSpinlock ( MsOSAttribute eAttribute, char *pMutexName1, MS_U32 u32Flag)
{
    MS_S32 s32Id, s32LstUnused = MSOS_SPINLOCK_MAX;
    MS_U8 pMutexName[MAX_MUTEX_NAME_LENGTH];
    MS_U32 u32MaxLen;

    if (NULL == pMutexName1)
    {
        return -1;
    }
    if (strlen(pMutexName1) >= (MAX_MUTEX_NAME_LENGTH-1))
    {
        printk("%s: Warning strlen(%s) is longer than MAX_MUTEX_NAME_LENGTH(%d). Oversize char will be discard.\n",
        __FUNCTION__,pMutexName1,MAX_MUTEX_NAME_LENGTH);
    }
    if (0 == (u32MaxLen = MIN(strlen(pMutexName1), (MAX_MUTEX_NAME_LENGTH-1))))
    {
        return -1;
    }
    strncpy((char*)pMutexName, (const char*)pMutexName1, u32MaxLen);
    pMutexName[u32MaxLen] = '\0';

    MUTEX_MUTEX_LOCK();
    for(s32Id=0;s32Id<MSOS_SPINLOCK_MAX;s32Id++)
    {
        // if (PTHREAD_PROCESS_SHARED == s32Prop) // @FIXME: Richard: is the mutex name always used as an id, regardless of process shared/private property?
        {
            if(TRUE == _MsOS_Spinlock_Info[s32Id].bUsed)
            {
                if (0== strcmp((const char*)_MsOS_Spinlock_Info[s32Id].u8Name, (const char*)pMutexName))
                {
                    break;
                }
            }
        }
        if (FALSE==_MsOS_Spinlock_Info[s32Id].bUsed  && MSOS_SPINLOCK_MAX==s32LstUnused)
        {
            s32LstUnused = s32Id;
        }
    }
    if ((MSOS_SPINLOCK_MAX==s32Id) && (MSOS_SPINLOCK_MAX>s32LstUnused))
    {
        _MsOS_Spinlock_Info[s32LstUnused].bUsed = TRUE;
        strcpy((char*)_MsOS_Spinlock_Info[s32LstUnused].u8Name, (const char*)pMutexName);
        spin_lock_init(&_MsOS_Spinlock_Info[s32LstUnused].stMutex);
        s32Id = s32LstUnused;
    }
    MUTEX_MUTEX_UNLOCK();

    if(MSOS_SPINLOCK_MAX <= s32Id)
    {
        return -1;
    }

    s32Id |= MSOS_ID_PREFIX;

    return s32Id;
}

//-------------------------------------------------------------------------------------------------
/// Delete the specified mutex
/// @param  s32MutexId  \b IN: mutex ID
/// @return TRUE : succeed
/// @return FALSE : fail
/// @note   It is important that the mutex be in the unlocked state when it is
///            destroyed, or else the behavior is undefined.
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_DeleteMutex (MS_S32 s32MutexId)
{
    if ( (s32MutexId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32MutexId &= MSOS_ID_MASK;
    }
    MUTEX_MUTEX_LOCK();

    MS_ASSERT(_MsOS_Mutex_Info[s32MutexId].bUsed);
    _MsOS_Mutex_Info[s32MutexId].bUsed = FALSE;
    _MsOS_Mutex_Info[s32MutexId].u8Name[0] = '\0';

    MUTEX_MUTEX_UNLOCK();
    return TRUE;
}
MS_BOOL MsOS_DeleteSpinlock (MS_S32 s32MutexId)
{
    if ( (s32MutexId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32MutexId &= MSOS_ID_MASK;
    }
    MUTEX_MUTEX_LOCK();

    MS_ASSERT(_MsOS_Spinlock_Info[s32MutexId].bUsed);
    _MsOS_Spinlock_Info[s32MutexId].bUsed = FALSE;
    _MsOS_Spinlock_Info[s32MutexId].u8Name[0] = '\0';

    MUTEX_MUTEX_UNLOCK();
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Attempt to lock a mutex
/// @param  s32MutexId  \b IN: mutex ID
/// @param  u32WaitMs   \b IN: 0 ~ MSOS_WAIT_FOREVER: suspend time (ms) if the mutex is locked
/// @return TRUE : succeed
/// @return FALSE : fail
//-------------------------------------------------------------------------------------------------
// @FIXME: don't support time-out at this stage
MS_BOOL MsOS_ObtainMutex_IRQ(MS_S32 s32MutexId)
{
    if ( (s32MutexId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32MutexId &= MSOS_ID_MASK;
    }
    spin_lock_irq(&(_MsOS_Spinlock_Info[s32MutexId].stMutex));
    return TRUE;
}
MS_BOOL MsOS_ObtainMutex (MS_S32 s32MutexId, MS_U32 u32WaitMs)
{
    MS_BOOL bRet = FALSE;

    if ( (s32MutexId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32MutexId &= MSOS_ID_MASK;
    }

    if (u32WaitMs==MSOS_WAIT_FOREVER) //blocking wait
    {
#ifdef MSOS_MUTEX_USE_SEM
        down(&(_MsOS_Mutex_Info[s32MutexId].stMutex));
#else
        spin_lock(&(_MsOS_Mutex_Info[s32MutexId].stMutex));
#endif
        bRet = TRUE;
    }
    else if (u32WaitMs==0) //non-blocking
    {
#ifdef MSOS_MUTEX_USE_SEM
        if (!down_trylock(&_MsOS_Mutex_Info[s32MutexId].stMutex))
#else
        if (spin_trylock(&_MsOS_Mutex_Info[s32MutexId].stMutex))
#endif
        {
            bRet = TRUE;
        }
    }
    else //blocking wait with timeout
    {
#ifndef MSOS_MUTEX_USE_SEM
        MS_U32 u32CurTime = MsOS_GetSystemTime();
#endif

        while(1)
        {
#ifdef MSOS_MUTEX_USE_SEM
            if (0 == down_timeout(&_MsOS_Mutex_Info[s32MutexId].stMutex, msecs_to_jiffies(u32WaitMs)))
#else
            if (spin_trylock(&_MsOS_Mutex_Info[s32MutexId].stMutex))
#endif
            {
                bRet = TRUE;
                break;
            }
#ifdef MSOS_MUTEX_USE_SEM
            else
#else
            else if((MsOS_GetSystemTime() - u32CurTime) > u32WaitMs)
#endif
            {
                printf("Mutext TimeOut: ID:%x \n", (int)s32MutexId);
                bRet = FALSE;
                break;
            }

           // msleep(5);
        }

    }
    return bRet;
}

//-------------------------------------------------------------------------------------------------
/// Attempt to unlock a mutex
/// @param  s32MutexId  \b IN: mutex ID
/// @return TRUE : succeed
/// @return FALSE : fail
/// @note   Only the owner thread of the mutex can unlock it.
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_ReleaseMutex_IRQ (MS_S32 s32MutexId)
{
    if ( (s32MutexId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32MutexId &= MSOS_ID_MASK;
    }
    if(spin_is_locked(&(_MsOS_Spinlock_Info[s32MutexId].stMutex)))
    {
        spin_unlock_irq(&(_MsOS_Spinlock_Info[s32MutexId].stMutex));
    }
    return TRUE;

}
MS_BOOL MsOS_ReleaseMutex (MS_S32 s32MutexId)
{
    if ( (s32MutexId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32MutexId &= MSOS_ID_MASK;
    }
#ifdef MSOS_MUTEX_USE_SEM
    up(&(_MsOS_Mutex_Info[s32MutexId].stMutex));
#else
    spin_unlock(&(_MsOS_Mutex_Info[s32MutexId].stMutex));
#endif

    return TRUE;
}

//-------------------------------------------------------------------------------------------------
// Get a mutex informaton
// @param  s32MutexId  \b IN: mutex ID
// @param  peAttribute \b OUT: ptr to suspended mode: E_MSOS_FIFO / E_MSOS_PRIORITY
// @param  pMutexName  \b OUT: ptr to mutex name
// @return TRUE : succeed
// @return FALSE : the mutex has not been created.
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_InfoMutex (MS_S32 s32MutexId, MsOSAttribute *peAttribute, char *pMutexName)
{
    if ( (s32MutexId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32MutexId &= MSOS_ID_MASK;
    }

    if(_MsOS_Mutex_Info[s32MutexId].bUsed == TRUE)
    {
        //ToDo: extend _MsOS_Mutex_Info structure ?
        *peAttribute = E_MSOS_FIFO; //only FIFO for eCos
        // @FIXME: linux porting
        // UTL_strcpy(pMutexName, "ABC");
        strcpy(pMutexName, (const char*)_MsOS_Mutex_Info[s32MutexId].u8Name);
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}



MS_U32 MsOS_PA2KSEG1_1(MS_U32 addr)
{

    // miu0
    if ((0x00000000 <= addr) && (HAL_MIU1_BASE > addr))
    {
        return ((MS_U32)(addr) | (0x40000000));
    }
    // miu1
    if ((HAL_MIU1_BASE <= addr) && (((MS_U64)HAL_MIU1_BASE * 2) > (MS_U64)addr))
    {
        return ((addr & ~(HAL_MIU1_BASE)) | 0xD0000000);
    }
    return 0;
}


void MsOS_FlushMemory(void)
{

}


void MsOS_ReadMemory(void)
{

}

//
// Task
//
//-------------------------------------------------------------------------------------------------
/// Create a task
/// @param  pTaskEntry       \b IN: task entry point
/// @param  u32TaskEntryData \b IN: task entry data: a pointer to some static data, or a
///          small integer, or NULL if the task does not require any additional data.
/// @param  eTaskPriority    \b IN: task priority
/// @param  bAutoStart       \b IN: start immediately or later
/// @param  pStackBase       \b IN: task stack
/// @param  u32StackSize     \b IN: stack size
/// @param  pTaskName        \b IN: task name
/// @return >=0 : assigned Task ID
/// @return < 0 : fail
//-------------------------------------------------------------------------------------------------
MSOS_ST_TASKSTRUCT MsOS_GetTaskinfo(MS_S32 s32Id)
{
    MSOS_ST_TASKSTRUCT stTask;
    stTask.pThread = _MsOS_Task_Info[(s32Id&0xFFFF)].pstThreadInfo;
    return stTask;
}
void MsOS_SetUserNice(MSOS_ST_TASKSTRUCT stTask, long nice)
{
    set_user_nice(stTask.pThread,nice);
}
int MsOS_GetUserNice(MSOS_ST_TASKSTRUCT stTask)
{
    return (int)task_nice(stTask.pThread);
}

MS_S32 MsOS_CreateTask (TaskEntry pTaskEntry,
                     MS_U32 u32TaskEntryData,
                     MS_BOOL bAutoStart,
                     char *pTaskName)
{
    // @FIXME:
    //     (1) eTaskPriority: Task priority is ignored here
    //     (2) pTaskName: is ignored here
    //     (3) Need mutex to protect critical section

    MS_S32 s32Id;

    MUTEX_TASK_LOCK();

    for( s32Id=0; s32Id<MSOS_TASK_MAX; s32Id++)
    {
        if(_MsOS_Task_Info[s32Id].bUsed == FALSE)
        {
            break;
        }
    }
    if( s32Id >= MSOS_TASK_MAX)
    {
        return -1;
    }

    _MsOS_Task_Info[s32Id].bUsed = TRUE;
    //_MsOS_Task_Info[s32Id].pstThreadInfo = kthread_create(((int)(void *)pTaskEntry), (void*)u32TaskEntryData, pTaskName);
    _MsOS_Task_Info[s32Id].pstThreadInfo = kthread_create(pTaskEntry, (void*)u32TaskEntryData, pTaskName);

    MUTEX_TASK_UNLOCK();

    if (bAutoStart)
    {
        wake_up_process(_MsOS_Task_Info[s32Id].pstThreadInfo);
    }
    s32Id |= MSOS_ID_PREFIX;
    return s32Id;
}


//-------------------------------------------------------------------------------------------------
/// Delete a previously created task
/// @param  s32TaskId   \b IN: task ID
/// @return TRUE : succeed
/// @return FALSE : fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_DeleteTask (MS_S32 s32TaskId)
{
#if 1
    if ( (s32TaskId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32TaskId &= MSOS_ID_MASK;
    }

    kthread_stop(_MsOS_Task_Info[s32TaskId].pstThreadInfo);
    _MsOS_Task_Info[s32TaskId].bUsed = FALSE;
#else
    printk("[%s][%d] %s is not supported\n", __FUNCTION__, __LINE__, __FUNCTION__);
#endif
    return TRUE;
}



MS_BOOL MsOS_Init (void)
{
    MS_U32 u32I;
    static MS_BOOL bInit = FALSE;

    if(bInit)
    {
        return TRUE;
    }
    //
    // Task Management
    //
#ifdef MSOS_MUTEX_USE_SEM
    sema_init(&_MsOS_Task_Mutex, 1);
#else
    spin_lock_init(&_MsOS_Task_Mutex);
#endif
    for( u32I=0; u32I<MSOS_TASK_MAX; u32I++)
    {
        _MsOS_Task_Info[u32I].bUsed = FALSE;
    }

    // Even Group
    spin_lock_init(&_MsOS_EventGroup_Mutex);
    for( u32I=0; u32I<MSOS_EVENTGROUP_MAX; u32I++)
    {
        _MsOS_EventGroup_Info[u32I].bUsed = FALSE;
    }

    //
    // Timer
    //
    spin_lock_init(&_MsOS_Timer_Mutex);
    for( u32I=0; u32I<MSOS_TIMER_MAX; u32I++)
    {
        _MsOS_Timer_Info[u32I].bUsed = FALSE;
        _MsOS_Timer_Info[u32I].pTimerCb = NULL;
        _MsOS_Timer_Info[u32I].period = 0;
        _MsOS_Timer_Info[u32I].first = 0;
        init_timer(&(_MsOS_Timer_Info[u32I].timer));
    }

    bInit = TRUE;

    return TRUE;
}


//
// Event management
//
//-------------------------------------------------------------------------------------------------
/// Create an event group
/// @param  pEventName  \b IN: event group name
/// @return >=0 : assigned Event Id
/// @return <0 : fail
//-------------------------------------------------------------------------------------------------
MS_S32 MsOS_CreateEventGroup (char *pEventName)
{
    MS_S32 s32Id;

    EVENT_MUTEX_LOCK();
    for(s32Id=0; s32Id<MSOS_EVENTGROUP_MAX; s32Id++)
    {
        if(_MsOS_EventGroup_Info[s32Id].bUsed == FALSE)
        {
            break;
        }
    }
    if(s32Id < MSOS_EVENTGROUP_MAX)
    {
        _MsOS_EventGroup_Info[s32Id].bUsed = TRUE;
        _MsOS_EventGroup_Info[s32Id].u32EventGroup= 0;
    }
    EVENT_MUTEX_UNLOCK();

    if(s32Id >= MSOS_EVENTGROUP_MAX)
    {
        return -1;
    }
    spin_lock_init(&_MsOS_EventGroup_Info[s32Id].stMutexEvent);
    init_waitqueue_head(&_MsOS_EventGroup_Info[s32Id].stSemaphore);
    s32Id |= MSOS_ID_PREFIX;
    return s32Id;
}

//-------------------------------------------------------------------------------------------------
/// Delete the event group
/// @param  s32EventGroupId \b IN: event group ID
/// @return TRUE : succeed
/// @return FALSE : fail, sb is waiting for the event flag
/// @note event group that are being waited on must not be deleted
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_DeleteEventGroup (MS_S32 s32EventGroupId)
{
    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }

    init_waitqueue_head(&_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore);
    // PTH_RET_CHK(pthread_mutex_destroy(&_MsOS_EventGroup_Info[s32EventGroupId].stMutex));
    EVENT_MUTEX_LOCK();
    _MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup= 0;
    _MsOS_EventGroup_Info[s32EventGroupId].bUsed = FALSE;
    EVENT_MUTEX_UNLOCK();
    return TRUE;
}


//-------------------------------------------------------------------------------------------------
/// Set the event flag (bitwise OR w/ current value) in the specified event group
/// @param  s32EventGroupId \b IN: event group ID
/// @param  u32EventFlag    \b IN: event flag value
/// @return TRUE : succeed
/// @return FALSE : fail
//-------------------------------------------------------------------------------------------------

MS_BOOL MsOS_SetEvent_IRQ (MS_S32 s32EventGroupId, MS_U32 u32EventFlag)
{
    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }

    spin_lock_irq(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    SET_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, u32EventFlag);
    spin_unlock_irq(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    wake_up(&_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore);
    return TRUE;
}
MS_BOOL MsOS_SetEvent (MS_S32 s32EventGroupId, MS_U32 u32EventFlag)
{
    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }

    spin_lock(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    SET_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, u32EventFlag);
    spin_unlock(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    wake_up(&_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore);
    return TRUE;
}

MS_U32 MsOS_GetEvent(MS_S32 s32EventGroupId)
{
    MS_U32 u32Event;
    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }

    u32Event = HAS_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, 0xFFFF);
    return u32Event;
}
//-------------------------------------------------------------------------------------------------
/// Clear the specified event flag (bitwise XOR operation) in the specified event group
/// @param  s32EventGroupId \b IN: event group ID
/// @param  u32EventFlag    \b IN: event flag value
/// @return TRUE : succeed
/// @return FALSE : fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_ClearEvent (MS_S32 s32EventGroupId, MS_U32 u32EventFlag)
{
    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }

    spin_lock(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    RESET_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, u32EventFlag);
    spin_unlock(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    return TRUE;
}
MS_BOOL MsOS_ClearEventIRQ (MS_S32 s32EventGroupId, MS_U32 u32EventFlag)
{
    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }
    spin_lock_irq(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    RESET_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, u32EventFlag);
    spin_unlock_irq(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    return TRUE;
}

//-------------------------------------------------------------------------------------------------
/// Wait for the specified event flag combination from the event group
/// @param  s32EventGroupId     \b IN: event group ID
/// @param  u32WaitEventFlag    \b IN: wait event flag value
/// @param  pu32RetrievedEventFlag \b OUT: retrieved event flag value
/// @param  eWaitMode           \b IN: E_AND/E_OR/E_AND_CLEAR/E_OR_CLEAR
/// @param  u32WaitMs           \b IN: 0 ~ MSOS_WAIT_FOREVER: suspend time (ms) if the event is not ready
/// @return TRUE : succeed
/// @return FALSE : fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_WaitEvent (MS_S32 s32EventGroupId,
                     MS_U32 u32WaitEventFlag,
                     MS_U32 *pu32RetrievedEventFlag,
                     EventWaitMode eWaitMode,
                     MS_U32 u32WaitMs)
{
    MS_BOOL bRet= FALSE;
    MS_BOOL bAnd;
    MS_BOOL bClear;
    MS_BOOL bTimeout=0;

    *pu32RetrievedEventFlag = 0;

    if (!u32WaitEventFlag)
    {
        return FALSE;
    }

    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }

    bClear= ((E_AND_CLEAR== eWaitMode) || (E_OR_CLEAR== eWaitMode))? TRUE: FALSE;
    bAnd= ((E_AND== eWaitMode)|| (E_AND_CLEAR== eWaitMode))? TRUE: FALSE;

    do{
/*
        *pu32RetrievedEventFlag= HAS_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, u32WaitEventFlag);
        if ((bAnd)? (*pu32RetrievedEventFlag== u32WaitEventFlag): (0!= *pu32RetrievedEventFlag))
        {
            break;
        }
*/
        if (u32WaitMs== MSOS_WAIT_FOREVER) //blocking wait
        {
            if (bAnd)
            {
                wait_event(_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore,
                           ((_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup & u32WaitEventFlag) == u32WaitEventFlag));
            }
            else
            {
                //(printf("[DRVSCLDMA]%ld before Wait event:%lx @:%ld \n",u32WaitEventFlag,_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup & u32WaitEventFlag,MsOS_GetSystemTime()));
                wait_event(_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore,
                           ((_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup & u32WaitEventFlag) != 0));
                //(printf("[DRVSCLDMA]%ld After Wait event:%lx @:%ld \n",u32WaitEventFlag,MsOS_GetEvent(s32EventGroupId),MsOS_GetSystemTime()));
            }
        }
        else
        {
            u32WaitMs = msecs_to_jiffies(u32WaitMs);
            if (bAnd)
            {
                bTimeout = wait_event_timeout(_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore,
                                   ((_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup & u32WaitEventFlag) == u32WaitEventFlag),
                                   u32WaitMs);
            }
            else
            {
                bTimeout = wait_event_timeout(_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore,
                                   ((_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup & u32WaitEventFlag) != 0),
                                   u32WaitMs);
            }
        }
        if(!bTimeout)
            *pu32RetrievedEventFlag= HAS_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, u32WaitEventFlag);
        else
            printf("[SCL_MSOS]wait timeout\n");
    } while (0);

    bRet= (bAnd)? (*pu32RetrievedEventFlag== u32WaitEventFlag): (0!= *pu32RetrievedEventFlag);
    if (bRet && bClear)
    {
        spin_lock(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
        RESET_FLAG(_MsOS_EventGroup_Info[s32EventGroupId].u32EventGroup, *pu32RetrievedEventFlag);
        spin_unlock(&_MsOS_EventGroup_Info[s32EventGroupId].stMutexEvent);
    }
    return bRet;
}

wait_queue_head_t* MsOS_GetEventQueue (MS_S32 s32EventGroupId)
{
    if ( (s32EventGroupId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32EventGroupId &= MSOS_ID_MASK;
    }
    return (&_MsOS_EventGroup_Info[s32EventGroupId].stSemaphore);
}


//
// Timer management
//
static void _MsOS_TimerNotify(unsigned long data)
{
    MS_S32 s32Id = (MS_S32) data;

    if (_MsOS_Timer_Info[s32Id].pTimerCb)
    {
        _MsOS_Timer_Info[s32Id].pTimerCb(0, s32Id| MSOS_ID_PREFIX);
    }
    _MsOS_Timer_Info[s32Id].timer.expires = jiffies + HZ*_MsOS_Timer_Info[s32Id].period/1000;
    add_timer(&(_MsOS_Timer_Info[s32Id].timer));
}


//-------------------------------------------------------------------------------------------------
/// Create a Timer
/// @param  pTimerCb        \b IN: timer callback function
/// @param  u32FirstTimeMs  \b IN: first ms for timer expiration
/// @param  u32PeriodTimeMs \b IN: periodic ms for timer expiration after first expiration
///                                0: one shot timer
/// @param  bStartTimer     \b IN: TRUE: activates the timer after it is created
///                                FALSE: leaves the timer disabled after it is created
/// @param  pTimerName      \b IN: Timer name (not used by eCos)
/// @return >=0 : assigned Timer ID
///         <0 : fail
//-------------------------------------------------------------------------------------------------
MS_S32 MsOS_CreateTimer (TimerCb pTimerCb,
                      MS_U32 u32FirstTimeMs,
                      MS_U32 u32PeriodTimeMs,
                      MS_BOOL bStartTimer,
                      char *pTimerName)
{
    MS_S32 s32Id;

    TIMER_MUTEX_LOCK();
    for(s32Id=0;s32Id<MSOS_TIMER_MAX;s32Id++)
    {
        if(_MsOS_Timer_Info[s32Id].bUsed == FALSE)
        {
            break;
        }
    }
    if(s32Id < MSOS_TIMER_MAX)
    {
        _MsOS_Timer_Info[s32Id].bUsed = TRUE;
    }
    TIMER_MUTEX_UNLOCK();

    if(s32Id >= MSOS_TIMER_MAX)
    {
        return -1;
    }

    _MsOS_Timer_Info[s32Id].pTimerCb=   pTimerCb;
    _MsOS_Timer_Info[s32Id].first = u32FirstTimeMs;
    _MsOS_Timer_Info[s32Id].period = u32PeriodTimeMs;
    _MsOS_Timer_Info[s32Id].timer.data = (unsigned long)s32Id;
    _MsOS_Timer_Info[s32Id].timer.expires = jiffies + HZ*u32FirstTimeMs/1000;
    _MsOS_Timer_Info[s32Id].timer.function = _MsOS_TimerNotify;
    if (bStartTimer)
    {
        add_timer(&(_MsOS_Timer_Info[s32Id].timer));
    }
    s32Id |= MSOS_ID_PREFIX;
    return s32Id;
}

//-------------------------------------------------------------------------------------------------
/// Delete the Timer
/// @param  s32TimerId  \b IN: Timer ID
/// @return TRUE : succeed
/// @return FALSE : fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_DeleteTimer (MS_S32 s32TimerId)
{
    //return FALSE;

    if ( (s32TimerId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32TimerId &= MSOS_ID_MASK;
    }

    if( _MsOS_Timer_Info[s32TimerId].bUsed )
    {
        TIMER_MUTEX_LOCK();
        del_timer(&(_MsOS_Timer_Info[s32TimerId].timer));
        _MsOS_Timer_Info[s32TimerId].bUsed = FALSE;

        _MsOS_Timer_Info[s32TimerId].pTimerCb = NULL;
        _MsOS_Timer_Info[s32TimerId].period = 0;
        _MsOS_Timer_Info[s32TimerId].first = 0;
        TIMER_MUTEX_UNLOCK();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//-------------------------------------------------------------------------------------------------
/// Start the Timer
/// @param  s32TimerId  \b IN: Timer ID
/// @return TRUE : succeed
/// @return FALSE : fail
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_StartTimer (MS_S32 s32TimerId)
{
    //return FALSE;

    if ( (s32TimerId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32TimerId &= MSOS_ID_MASK;
    }

    if( _MsOS_Timer_Info[s32TimerId].bUsed )
    {
        _MsOS_Timer_Info[s32TimerId].timer.expires = jiffies + _MsOS_Timer_Info[s32TimerId].period/1000;
        add_timer(&(_MsOS_Timer_Info[s32TimerId].timer));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//-------------------------------------------------------------------------------------------------
/// Stop the Timer
/// @param  s32TimerId  \b IN: Timer ID
/// @return TRUE : succeed
/// @return FALSE : fail
/// @note   MsOS_StopTimer then MsOS_StartTimer => The timer will trigger at the same relative
///             intervals that it would have if it had not been disabled.
//-------------------------------------------------------------------------------------------------
MS_BOOL MsOS_StopTimer (MS_S32 s32TimerId)
{
    //return FALSE;

    if ( (s32TimerId & MSOS_ID_PREFIX_MASK) != MSOS_ID_PREFIX )
    {
        return FALSE;
    }
    else
    {
        s32TimerId &= MSOS_ID_MASK;
    }

    if( _MsOS_Timer_Info[s32TimerId].bUsed )
    {
        del_timer(&(_MsOS_Timer_Info[s32TimerId].timer));
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
void* MsOS_Memalloc(size_t size, gfp_t flags)
{
    return kmalloc(size, flags);
}
void MsOS_MemFree(void *pVirAddr)
{
    kfree(pVirAddr);
}
void* MsOS_VirMemalloc(size_t size)
{
    return vmalloc(size);
}
void MsOS_VirMemFree(void *pVirAddr)
{
    vfree(pVirAddr);
}
void* MsOS_Memcpy(void *pstCfg,const void *pstInformCfg,__kernel_size_t size)
{
    return memcpy(pstCfg, pstInformCfg, size);
}
void* MsOS_Memset(void *pstCfg,int val,__kernel_size_t size)
{
    return memset(pstCfg, val, size);
}
