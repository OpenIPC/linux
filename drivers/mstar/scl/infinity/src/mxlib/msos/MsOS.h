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

///////////////////////////////////////////////////////////////////////////////////////////////////
///
/// @file   MsOS.h
/// @brief  MStar OS Wrapper
/// @author MStar Semiconductor Inc.
///////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef _MS_OS_H_
#define _MS_OS_H_

#ifdef __cplusplus
extern "C"
{
#endif

//-------------------------------------------------------------------------------------------------
// Defines
//-------------------------------------------------------------------------------------------------
#define MSIF_MSOS_LIB_CODE              {'M','S','O','S'}    //Lib code
#define MSIF_MSOS_LIBVER                {'0','1'}            //LIB version
#define MSIF_MSOS_BUILDNUM              {'0','1'}            //Build Number
#define MSIF_MSOS_CHANGELIST            {'0','0','0','0','0','0','0','0'} //P4 ChangeList Number

#define MSOS_DRV_VERSION                /* Character String for DRV/API version             */  \
    MSIF_TAG,                           /* 'MSIF'                                           */  \
    MSIF_CLASS,                         /* '00'                                             */  \
    MSIF_CUS,                           /* 0x0000                                           */  \
    MSIF_MOD,                           /* 0x0000                                           */  \
    MSIF_CHIP,                                                                                  \
    MSIF_CPU,                                                                                   \
    MSIF_MSOS_LIB_CODE,                  /* IP__                                             */  \
    MSIF_MSOS_LIBVER,                    /* 0.0 ~ Z.Z                                        */  \
    MSIF_MSOS_BUILDNUM,                  /* 00 ~ 99                                          */  \
    MSIF_MSOS_CHANGELIST,                /* CL#                                              */  \
    MSIF_OS

#define MSOS_TASK_MAX           32//(32+120)
#define MSOS_MEMPOOL_MAX        (8+64)
#define MSOS_FIXSIZE_MEMPOOL_MAX  (8)
#define MSOS_SEMAPHORE_MAX      (32+150)
#define MSOS_MUTEX_MAX          32//(64+240)
#define MSOS_SPINLOCK_MAX       8
#define MSOS_EVENTGROUP_MAX     32//(64)
#define MSOS_TIMER_MAX          (32)
#define MSOS_QUEUE_MAX          (16+60)

#define MSOS_OS_MALLOC          (0x7654FFFF)
#define MSOS_MALLOC_ID          (0x0000FFFF)

//-------------------------------------------------------------------------------------------------
// Macros
//-------------------------------------------------------------------------------------------------
//time and clock macros
#define TICK_PER_ONE_MS     (1) //Note: confirm Kernel fisrt
#define MSOS_WAIT_FOREVER   (0xffffff00/TICK_PER_ONE_MS)

#ifdef  MSOS_PERF_DEBUG
#define MSOS_PERF_PROFILE_DECL()  MS_U32 u32time1=0, u32time2=0
#define MSOS_PERF_PROFILE_ON()    u32time1 = MsOS_GetSystemTime()
#define MSOS_PERF_PROFILE_OFF()   u32time2 = MsOS_GetSystemTime(); \
                                  printf("[MSOS_DBG]%s:%d takes %6dms\n",__FILE__, __LINE__, u32time2-u32time1)
#else
#define MSOS_PERF_PROFILE_DECL()
#define MSOS_PERF_PROFILE_ON()
#define MSOS_PERF_PROFILE_OFF()
#endif

//-------------------------------------------------------------------------------------------------
// Type and Structure Declaration
//-------------------------------------------------------------------------------------------------
#if defined (MSOS_TYPE_ECOS) || defined (MSOS_TYPE_LINUX) || defined (MSOS_TYPE_NOS) || defined (MSOS_TYPE_UCOS) || defined (MSOS_TYPE_CE)

//compatible with Nucleus's task_entry
typedef void ( *TaskEntry ) (MS_U32 argc, void *argv); ///< Task entry function  argc: pass additional data to task entry; argv: not used by eCos
typedef void ( *InterruptCb ) (InterruptNum eIntNum);               ///< Interrupt callback function
typedef void ( *SignalCb ) (MS_U32 u32Signals);        ///< Signal callback function
typedef void ( *TimerCb ) (MS_U32 u32StTimer, MS_U32 u32TimerID);  ///< Timer callback function  u32StTimer: not used; u32TimerID: Timer ID

typedef struct {
    volatile MS_S32 s32Value;
} MsOS_Atomic;


#ifdef MSOS_TYPE_UCOS
/// Task priority
typedef enum
{
    E_TASK_PRI_SYS      = 0,    ///< System priority task   ( interrupt level driver, e.g. TSP, SMART )
    E_TASK_PRI_HIGHEST  = 16,   ///< Highest priority task  ( background monitor driver, e.g. DVBC, HDMI )
    E_TASK_PRI_HIGH     = 32,   ///< High priority task     ( service task )
    E_TASK_PRI_MEDIUM   = 48,   ///< Medium priority task   ( application task )
    E_TASK_PRI_LOW      = 64,   ///< Low priority task      ( nonbusy application task )
    E_TASK_PRI_LOWEST   = 96,   ///< Lowest priority task   ( idle application task )
} TaskPriority;
#else
/// Task priority
typedef enum
{
    E_TASK_PRI_SYS      = 0,    ///< System priority task   ( interrupt level driver, e.g. TSP, SMART )
    E_TASK_PRI_HIGHEST  = 4,    ///< Highest priority task  ( background monitor driver, e.g. DVBC, HDMI )
    E_TASK_PRI_HIGH     = 8,    ///< High priority task     ( service task )
    E_TASK_PRI_MEDIUM   = 12,   ///< Medium priority task   ( application task )
    E_TASK_PRI_LOW      = 16,   ///< Low priority task      ( nonbusy application task )
    E_TASK_PRI_LOWEST   = 24,   ///< Lowest priority task   ( idle application task )
} TaskPriority;
#endif


/// Suspend type
typedef enum
{
    E_MSOS_PRIORITY,            ///< Priority-order suspension
    E_MSOS_FIFO,                ///< FIFO-order suspension
} MsOSAttribute;

/// Message size type
typedef enum
{
    E_MSG_FIXED_SIZE,           ///< Fixed size message
    E_MSG_VAR_SIZE,             ///< Variable size message
} MessageType;

/// Event mode
typedef enum
{
    E_AND,                      ///< Specify all of the requested events are require.
    E_OR,                       ///< Specify any of the requested events are require.
    E_AND_CLEAR,                ///< Specify all of the requested events are require. If the request are successful, clear the event.
    E_OR_CLEAR,                 ///< Specify any of the requested events are require. If the request are successful, clear the event.
} EventWaitMode;

typedef struct
{
    MS_S32                          iId;
    MS_U32                          uPoolSize;
    MS_U32                          u32MinAllocation;
    MS_U32                          u32Addr;
    MsOSAttribute                   eAttribute;
    char                            szName[16];
} MemoryPool_Info, *PMemoryPool_Info;

typedef struct
{
    MS_S32                          iId;
    TaskPriority                    ePriority;
    void                            *pStack;
    MS_U32                          u32StackSize;
    char szName[16];
} Task_Info, *PTask_Info;

typedef struct
{
        unsigned int                LX_MEM_ADDR;
        unsigned int                LX_MEM_LENGTH;
        unsigned int                LX_MEM2_ADDR;
        unsigned int                LX_MEM2_LENGTH;
        unsigned int                EMAC_ADDR;
        unsigned int                EMAC_LENGTH;
        unsigned int                DRAM_ADDR;
        unsigned int                DRAM_LENGTH;
        unsigned int                BB_ADDR;
        unsigned int                BB_LENGTH;
        unsigned int                MPOOL_MEM_ADDR;
        unsigned int                MPOOL_MEM_LENGTH;
        unsigned int                G3D_MEM0_ADDR;
        unsigned int                G3D_MEM0_LENGTH;
        unsigned int                G3D_MEM1_ADDR;
        unsigned int                G3D_MEM1_LENGTH;
        unsigned int                G3D_CMDQ_ADDR;
        unsigned int                G3D_CMDQ_LENGTH;
}IO_Sys_Info_t;

#elif defined (MSOS_TYPE_LINUX_KERNEL)

//#include <linux/kernel.h>
//#include <linux/interrupt.h>

//typedef void ( *TaskEntry ) (MS_U32 argc, void *argv); ///< Task entry function  argc: pass additional data to task entry; argv: not used by eCos
typedef int (*TaskEntry)(void *argv);
//typedef void ( *InterruptCb ) (InterruptNum eIntNum);               ///< Interrupt callback function
typedef irqreturn_t ( *InterruptCb ) (InterruptNum eIntNum, void* dev_id);
typedef void ( *SignalCb ) (MS_U32 u32Signals);        ///< Signal callback function
typedef void ( *TimerCb ) (MS_U32 u32StTimer, MS_U32 u32TimerID);  ///< Timer callback function  u32StTimer: not used; u32TimerID: Timer ID

typedef struct {
    volatile MS_S32 s32Value;
} MsOS_Atomic;

/// Task priority
typedef enum
{
    E_TASK_PRI_SYS      = 0,    ///< System priority task   ( interrupt level driver, e.g. TSP, SMART )
    E_TASK_PRI_HIGHEST  = 4,    ///< Highest priority task  ( background monitor driver, e.g. DVBC, HDMI )
    E_TASK_PRI_HIGH     = 8,    ///< High priority task     ( service task )
    E_TASK_PRI_MEDIUM   = 12,   ///< Medium priority task   ( application task )
    E_TASK_PRI_LOW      = 16,   ///< Low priority task      ( nonbusy application task )
    E_TASK_PRI_LOWEST   = 24,   ///< Lowest priority task   ( idle application task )
} TaskPriority;

/// Suspend type
typedef enum
{
    E_MSOS_PRIORITY,            ///< Priority-order suspension
    E_MSOS_FIFO,                ///< FIFO-order suspension
} MsOSAttribute;

/// Message size type
typedef enum
{
    E_MSG_FIXED_SIZE,           ///< Fixed size message
    E_MSG_VAR_SIZE,             ///< Variable size message
} MessageType;

/// Event mode
typedef enum
{
    E_AND,                      ///< Specify all of the requested events are require.
    E_OR,                       ///< Specify any of the requested events are require.
    E_AND_CLEAR,                ///< Specify all of the requested events are require. If the request are successful, clear the event.
    E_OR_CLEAR,                 ///< Specify any of the requested events are require. If the request are successful, clear the event.
} EventWaitMode;

typedef struct
{
    MS_S32                          iId;
    MS_U32                          uPoolSize;
    MS_U32                          u32MinAllocation;
    MS_U32                          u32Addr;
    MsOSAttribute                   eAttribute;
    char                            szName[16];
} MemoryPool_Info, *PMemoryPool_Info;

typedef struct
{
    MS_S32                          iId;
    TaskPriority                    ePriority;
    void                            *pStack;
    MS_U32                          u32StackSize;
    char szName[16];
} Task_Info, *PTask_Info;

typedef struct
{
        unsigned int                LX_MEM_ADDR;
        unsigned int                LX_MEM_LENGTH;
        unsigned int                LX_MEM2_ADDR;
        unsigned int                LX_MEM2_LENGTH;
        unsigned int                EMAC_ADDR;
        unsigned int                EMAC_LENGTH;
        unsigned int                DRAM_ADDR;
        unsigned int                DRAM_LENGTH;
        unsigned int                BB_ADDR;
        unsigned int                BB_LENGTH;
        unsigned int                MPOOL_MEM_ADDR;
        unsigned int                MPOOL_MEM_LENGTH;
        unsigned int                G3D_MEM0_ADDR;
        unsigned int                G3D_MEM0_LENGTH;
        unsigned int                G3D_MEM1_ADDR;
        unsigned int                G3D_MEM1_LENGTH;
        unsigned int                G3D_CMDQ_ADDR;
        unsigned int                G3D_CMDQ_LENGTH;
}IO_Sys_Info_t;

#elif defined (MSOS_TYPE_XXX)

#error "The OS is not supported now ..."

#endif


///////////paul include for RTOS and LINUX
typedef struct
{
    struct task_struct *pThread;
}MSOS_ST_TASKSTRUCT;
typedef struct clk MSOS_ST_CLK;

//-------------------------------------------------------------------------------------------------
// Extern Functions
//-------------------------------------------------------------------------------------------------
//
// Init
//
MS_BOOL MsOS_Init (void);


//
// Memory management
//
MS_S32 MsOS_CreateMemoryPool (MS_U32 u32PoolSize,
                              MS_U32 u32MinAllocation,
                              void * pPoolAddr,
                              MsOSAttribute eAttribute,
                              char *pPoolName);

MS_BOOL MsOS_DeleteMemoryPool (MS_S32 s32PoolId);

MS_BOOL MsOS_InfoMemoryPool (MS_S32 s32PoolId,
                             void **pPoolAddr,
                             MS_U32 *pu32PoolSize,
                             MS_U32 *pu32FreeSize,
                             MS_U32 *pu32LargestFreeBlockSize);

void * MsOS_AllocateMemory (MS_U32 u32Size, MS_S32 s32PoolId);

//void * MsOS_AllocateAlignedMemory (MS_U32 u32Size, MS_U32 u32AlignedByte, MS_S32 s32PoolId);

void * MsOS_ReallocateMemory (void *pOrgAddress, MS_U32 u32NewSize, MS_S32 s32PoolId);

MS_BOOL MsOS_FreeMemory (void *pAddress, MS_S32 s32PoolId);

MS_S32 MsOS_CreateFixSizeMemoryPool (MS_U32 u32PoolSize,
                                     MS_U32 u32BlockSize,
                                     void * pPoolAddr,
                                     MsOSAttribute eAttribute,
                                     char *pPoolName);

MS_BOOL MsOS_DeleteFixSizeMemoryPool (MS_S32 s32PoolId);

MS_BOOL MsOS_InfoFixSizeMemoryPool (MS_S32 s32PoolId,
                                    void **pPoolAddr,
                                    MS_U32 *pu32PoolSize,
                                    MS_U32 *pu32FreeSize,
                                    MS_U32 *pu32LargestFreeBlockSize);

void * MsOS_AllocateFixSizeMemory (MS_S32 s32PoolId);

MS_BOOL MsOS_FreeFixSizeMemory (void *pAddress, MS_S32 s32PoolId);


//
// Task
//
MS_S32 MsOS_CreateTask (TaskEntry pTaskEntry,
                        MS_U32 u32TaskEntryData,
                        MS_BOOL bAutoStart,
                        char *pTaskName);
MSOS_ST_TASKSTRUCT MsOS_GetTaskinfo(MS_S32 s32Id);
int MsOS_GetUserNice(MSOS_ST_TASKSTRUCT stTask);
void MsOS_SetUserNice(MSOS_ST_TASKSTRUCT stTask, long nice);
MS_BOOL MsOS_DeleteTask (MS_S32 s32TaskId);

void MsOS_YieldTask (void);

void MsOS_DelayTask (MS_U32 u32Ms);

void MsOS_DelayTaskUs (MS_U32 u32Us);

MS_BOOL MsOS_ResumeTask (MS_S32 s32TaskId);

MS_BOOL MsOS_SuspendTask (MS_S32 s32TaskId);

MS_S32 MsOS_InfoTaskID (void);
//-------------------------------------------------------------------------------------------------
/// Get thread ID of current thread/process in OS
/// @return : current thread ID
//-------------------------------------------------------------------------------------------------
MS_S32 MsOS_GetOSThreadID (void);

//
// Mutex
//
#define MSOS_PROCESS_PRIVATE    0x00000000
#define MSOS_PROCESS_SHARED     0x00000001
#define MAX_MUTEX_NAME_LENGTH   16
MS_S32 MsOS_CreateMutex ( MsOSAttribute eAttribute, char *pMutexName, MS_U32 u32Flag);
MS_S32 MsOS_CreateSpinlock ( MsOSAttribute eAttribute, char *pMutexName1, MS_U32 u32Flag);

MS_BOOL MsOS_DeleteMutex (MS_S32 s32MutexId);
MS_BOOL MsOS_DeleteSpinlock (MS_S32 s32MutexId);
MS_BOOL MsOS_ObtainMutex_IRQ(MS_S32 s32MutexId);
MS_BOOL MsOS_ObtainMutex (MS_S32 s32MutexId, MS_U32 u32WaitMs);
MS_BOOL MsOS_ReleaseMutex_IRQ (MS_S32 s32MutexId);
MS_BOOL MsOS_ReleaseMutex (MS_S32 s32MutexId);

MS_BOOL MsOS_InfoMutex (MS_S32 s32MutexId, MsOSAttribute *peAttribute, char *pMutexName);


//
// Semaphore
//
MS_S32 MsOS_CreateSemaphore (MS_U32 u32InitCnt,
                             MsOSAttribute eAttribute,
                             char *pName);

MS_BOOL MsOS_DeleteSemaphore (MS_S32 s32SemaphoreId);

MS_BOOL MsOS_ObtainSemaphore (MS_S32 s32SemaphoreId, MS_U32 u32WaitMs);

MS_BOOL MsOS_ReleaseSemaphore (MS_S32 s32SemaphoreId);

MS_BOOL MsOS_InfoSemaphore (MS_S32 s32SemaphoreId, MS_U32 *pu32Cnt, MsOSAttribute *peAttribute, char *pSemaphoreName);


//
// Event management
//
MS_S32 MsOS_CreateEventGroup (char *pName);

MS_BOOL MsOS_DeleteEventGroup (MS_S32 s32EventGroupId);
MS_BOOL MsOS_SetEvent_IRQ (MS_S32 s32EventGroupId, MS_U32 u32EventFlag);

MS_BOOL MsOS_SetEvent (MS_S32 s32EventGroupId, MS_U32 u32EventFlag);
MS_U32 MsOS_GetEvent(MS_S32 s32EventGroupId);
MS_BOOL MsOS_ClearEventIRQ (MS_S32 s32EventGroupId, MS_U32 u32EventFlag);
MS_BOOL MsOS_ClearEvent (MS_S32 s32EventGroupId, MS_U32 u32EventFlag);
MS_BOOL MsOS_WaitEvent (MS_S32  s32EventGroupId,
                        MS_U32  u32WaitEventFlag,
                        MS_U32  *pu32RetrievedEventFlag,
                        EventWaitMode eWaitMode,
                        MS_U32  u32WaitMs);
wait_queue_head_t* MsOS_GetEventQueue (MS_S32 s32EventGroupId);

/*
//
// Signal management
//
MS_BOOL MsOS_CreateSignal (SignalCb pSignalCb);

MS_BOOL MsOS_ControlSignals (MS_U32 u32SignalMask);

MS_BOOL MsOS_SendSignals (MS_S32 s32TaskId, MS_U32 u32Signal);

MS_U32 MsOS_ReceiveSignals (void);
*/

//
// Timer management
//
MS_S32 MsOS_CreateTimer (TimerCb    pTimerCb,
                         MS_U32     u32FirstTimeMs,
                         MS_U32     u32PeriodTimeMs,
                         MS_BOOL    bStartTimer,
                         char       *pName);

MS_BOOL MsOS_DeleteTimer (MS_S32 s32TimerId);

MS_BOOL MsOS_StartTimer (MS_S32 s32TimerId);

MS_BOOL MsOS_StopTimer (MS_S32 s32TimerId);

MS_BOOL MsOS_ResetTimer (MS_S32     s32TimerId,
                         MS_U32     u32FirstTimeMs,
                         MS_U32     u32PeriodTimeMs,
                         MS_BOOL    bStartTimer);
void* MsOS_Memalloc(size_t size, gfp_t flags);
void MsOS_MemFree(void *pVirAddr);
void* MsOS_VirMemalloc(size_t size);
void MsOS_VirMemFree(void *pVirAddr);
void* MsOS_Memcpy(void *pstCfg,const void *pstInformCfg,__kernel_size_t size);
void* MsOS_Memset(void *pstCfg,int val,__kernel_size_t size);
//
// System time
//
//MS_U32 MsOS_GetSystemTick (void);

MS_U32 MsOS_GetSystemTime (void);
void MsOS_SetPollWait(void *filp,void *pWaitQueueHead,void *pstPollQueue);
MS_U64 MsOS_GetSystemTimeStamp (void);


MS_U32 MsOS_Timer_DiffTimeFromNow(MS_U32 u32TaskTimer); ///[OBSOLETE]

MS_U32 MsOS_Timer_DiffTime(MS_U32 u32Timer, MS_U32 u32TaskTimer); ///[OBSOLETE]
//MS_BOOL MsOS_SetSystemTime (MS_U32 u32SystemTime);


//
// Queue
//
MS_S32 MsOS_CreateQueue (void           *pStartAddr,
                         MS_U32         u32QueueSize,
                         MessageType    eMessageType,
                         MS_U32         u32MessageSize,
                         MsOSAttribute  eAttribute,
                         char           *pQueueName);

MS_BOOL MsOS_DeleteQueue (MS_S32 s32QueueId);

MS_BOOL MsOS_SendToQueue (MS_S32 s32QueueId, MS_U8 *pu8Message, MS_U32 u32Size, MS_U32 u32WaitMs);

MS_BOOL MsOS_RecvFromQueue (MS_S32 s32QueueId, MS_U8 *pu8Message, MS_U32 u32IntendedSize, MS_U32 *pu32ActualSize, MS_U32 u32WaitMs);

MS_BOOL MsOS_PeekFromQueue (MS_S32 s32QueueId, MS_U8 *pu8Message, MS_U32 u32IntendedSize, MS_U32 *pu32ActualSize);

//
// Atomic operation
//
void MsOS_AtomicSet(MsOS_Atomic *pAtomic, MS_S32 s32Value);
MS_S32 MsOS_AtomicRead(const MsOS_Atomic *pAtomic);

void MsOS_AtomicAdd(MsOS_Atomic *pAtomic, MS_S32 s32Value);
void MsOS_AtomicSub(MsOS_Atomic *pAtomic, MS_S32 s32Value);

MS_S32 MsOS_AtomicAddReturn(MsOS_Atomic *pAtomic, MS_S32 s32Value);
MS_S32 MsOS_AtomicSubReturn(MsOS_Atomic *pAtomic, MS_S32 s32Value);

//
// Per-thread data
//
MS_BOOL MsOS_CreateThreadDataIndex(MS_U32 *pu32Index);
MS_BOOL MsOS_DeleteThreadDataIndex(MS_U32 u32Index);
MS_BOOL MsOS_SetThreadData(MS_U32 u32Index, MS_U32 u32Data);
MS_BOOL MsOS_GetThreadData(MS_U32 u32Index, MS_U32 *pu32Data);


//
// Interrupt management
//
MS_BOOL MsOS_AttachInterrupt (InterruptNum eIntNum, InterruptCb pIntCb,unsigned long flags,const char *name);

MS_BOOL MsOS_DetachInterrupt (InterruptNum eIntNum);

MS_BOOL MsOS_EnableInterrupt (InterruptNum eIntNum);

MS_BOOL MsOS_DisableInterrupt (InterruptNum eIntNum);

MS_BOOL MsOS_In_Interrupt (void);

MS_U32  MsOS_DisableAllInterrupts(void);

MS_BOOL MsOS_RestoreAllInterrupts(MS_U32 u32OldInterrupts);

MS_BOOL MsOS_EnableAllInterrupts(void);



#if defined(__aeon__)
typedef enum {
    E_EXCEPTION_BUS_ERROR = 2,
    E_EXCEPTION_DATA_PAGE_FAULT,
    E_EXCEPTION_INSTRUCTION_PAGE_FAULT,
    E_EXCEPTION_TICK_TIMER,             ///< tick timer, do not use directly
    E_EXCEPTION_UNALIGNED_ACCESS,
    E_EXCEPTION_ILLEGAL_INSTRUCTION,
    E_EXCEPTION_EXTERNAL_INTERRUPT,     ///< external interrupt, do not use directly
    E_EXCEPTION_DTLB_MISS,
    E_EXCEPTION_ITLB_MISS,
    E_EXCEPTION_RANGE,
    E_EXCEPTION_SYSCALL,                ///< caused by l.sys
    E_EXCEPTION_RESERVED,
    E_EXCEPTION_TRAP,                   ///< caused by l.trap
    E_EXCEPTION_MAX = E_EXCEPTION_TRAP,
} MHAL_EXCEPTION_TYPE;

typedef enum {
    E_INTERRUPT_TICK_TIMER, //< risc32 builtin tick timer
    E_INTERRUPT_00 = 1,     //< PIC interrupt start from 1 for handler performance
    E_INTERRUPT_01,
    E_INTERRUPT_02,
    E_INTERRUPT_03,
    E_INTERRUPT_04,
    E_INTERRUPT_05,
    E_INTERRUPT_06,
    E_INTERRUPT_07,
    E_INTERRUPT_08,
    E_INTERRUPT_09,
    E_INTERRUPT_10,
    E_INTERRUPT_11,
    E_INTERRUPT_12,
    E_INTERRUPT_13,
    E_INTERRUPT_14,
    E_INTERRUPT_15,
    E_INTERRUPT_16,
    E_INTERRUPT_17,
    E_INTERRUPT_18,
    E_INTERRUPT_19,
    E_INTERRUPT_20,
    E_INTERRUPT_21,
    E_INTERRUPT_22,
    E_INTERRUPT_23,
    E_INTERRUPT_24,
    E_INTERRUPT_25,
    E_INTERRUPT_26,
    E_INTERRUPT_27,
    E_INTERRUPT_28,
    E_INTERRUPT_29,
    E_INTERRUPT_30,
    E_INTERRUPT_31,
} MHAL_INTERRUPT_TYPE;

// Aliases for interrupt number
#define E_INTERRUPT_FIQ         E_INTERRUPT_02
#define E_INTERRUPT_IRQ         E_INTERRUPT_03
#define E_INTERRUPT_UART        E_INTERRUPT_19
#define E_INTERRUPT_MAX         E_INTERRUPT_31

typedef struct
{
    unsigned long   r[32];          ///< GPR registers
#ifdef __AEONR2__
    unsigned long   machi2;         // Highest 32-bits of new 32x32=64 multiplier
#endif
    unsigned long   machi;          // High and low words of
    unsigned long   maclo;          //   multiply/accumulate reg

    // These are only saved for exceptions and interrupts
    int             vector;         ///< vector number
    int             sr;             ///< status register
    unsigned long   pc;             ///< program counter

    // Saved only for exceptions, and not restored when continued:
    // Effective address of instruction/data access that caused exception
    unsigned long   eear;           ///< exception effective address
} MHAL_SavedRegisters;
#else
typedef enum {
    E_EXCEPTION_DATA_TLBERROR_ACCESS = 1,    // TLB modification exception
    E_EXCEPTION_DATA_TLBMISS_ACCESS,         // TLB miss (Load or IFetch)
    E_EXCEPTION_DATA_TLBMISS_WRITE,          // TLB miss (Store)
    E_EXCEPTION_DATA_UNALIGNED_ACCESS,       // Address error (Load or Ifetch)
    E_EXCEPTION_DATA_UNALIGNED_WRITE,        // Address error (store)
    E_EXCEPTION_CODE_ACCESS,                 // Bus error (Ifetch)
    E_EXCEPTION_DATA_ACCESS,                 // Bus error (data load or store)
    E_EXCEPTION_SYSTEM_CALL,                 // System call
    E_EXCEPTION_INSTRUCTION_BP,              // Break point
    E_EXCEPTION_ILLEGAL_INSTRUCTION,         // Reserved instruction
    E_EXCEPTION_COPROCESSOR,                 // Coprocessor unusable
    E_EXCEPTION_OVERFLOW,                    // Arithmetic overflow
    E_EXCEPTION_RESERVED_13,                 // Reserved
    E_EXCEPTION_DIV_BY_ZERO,                 // Division-by-zero [reserved vector]
    E_EXCEPTION_FPU,                         // Floating point exception
    E_EXCEPTION_MAX = E_EXCEPTION_FPU,
} MHAL_EXCEPTION_TYPE;

typedef enum {
    E_INTERRUPT_02 = 0,
    E_INTERRUPT_03,
    E_INTERRUPT_04,
    E_INTERRUPT_05,
    E_INTERRUPT_06,
    E_INTERRUPT_07,
} MHAL_INTERRUPT_TYPE;

// Aliases for interrupt number
#define E_INTERRUPT_FIQ         E_INTERRUPT_03
#define E_INTERRUPT_IRQ         E_INTERRUPT_02
#define E_INTERRUPT_TICK_TIMER  E_INTERRUPT_07
#define E_INTERRUPT_MAX         E_INTERRUPT_07
#define E_EXCEPTION_TRAP        E_EXCEPTION_RESERVED_13

typedef struct
{
    // These are common to all saved states
    unsigned long    d[32];          /* Data regs                    */
    unsigned long    hi;             /* hi word of mpy/div reg       */
    unsigned long    lo;             /* lo word of mpy/div reg       */

    // The status register contains the interrupt-enable bit which needs
    // to be preserved across context switches.
    unsigned long    sr;             /* Status Reg                   */

    // These are only saved for exceptions and interrupts
    unsigned long    vector;         /* Vector number                */
    unsigned long    pc;             /* Program Counter              */

    // These are only saved for exceptions, and are not restored
    // when continued.
    unsigned long    cause;          /* Exception cause register     */
    unsigned long    badvr;          /* Bad virtual address reg      */

} MHAL_SavedRegisters;
#endif

#if defined (__arm__)
typedef void (*mhal_isr_t)(void);
#else
typedef void (*mhal_isr_t)(MHAL_SavedRegisters *regs, MS_U32 vector);
#endif

MS_U32 MsOS_CPU_DisableInterrupt (void);

MS_BOOL MsOS_CPU_EnableInterrupt (void);

MS_BOOL MsOS_CPU_RestoreInterrupt (MS_U32 u32OldInterrupts);

MS_BOOL MsOS_CPU_MaskAllInterrupt (void);

MS_BOOL MsOS_CPU_MaskInterrupt (MHAL_INTERRUPT_TYPE intr_num);

MS_BOOL MsOS_CPU_UnMaskInterrupt (MHAL_INTERRUPT_TYPE intr_num);

MS_BOOL MsOS_CPU_LockInterrupt (void);

MS_BOOL MsOS_CPU_UnLockInterrupt (void);

MS_BOOL MsOS_CPU_AttachInterrupt (MHAL_INTERRUPT_TYPE intr_num, mhal_isr_t isr, MS_U32 dat);

MS_BOOL MsOS_CPU_DetachInterrupt (MHAL_INTERRUPT_TYPE intr_num);

MS_BOOL MsOS_CPU_AttachException (MHAL_EXCEPTION_TYPE expt_num, mhal_isr_t isr, MS_U32 dat);

MS_BOOL MsOS_CPU_DetachExceptiont (MHAL_EXCEPTION_TYPE expt_num);

MS_BOOL MsOS_CPU_SetEBASE (MS_U32 u32Addr);




//
// Cache Opertation
//
MS_BOOL MsOS_Dcache_Flush( MS_U32 u32Start , MS_U32 u32Size );

MS_BOOL MsOS_Dcache_Invalidate( MS_U32 u32Start , MS_U32 u32Size );

MS_BOOL MsOS_Dcache_Writeback( MS_U32 u32Start , MS_U32 u32Size );

#if defined(CHIP_T12) || defined(CHIP_T8) || defined(CHIP_J2) || defined(CHIP_A2) || defined(CHIP_A5) || defined(CHIP_A3)
MS_BOOL MsOS_L2Cache_Flush(void);
MS_BOOL MsOS_L2Cache_Read(void);
#endif

//
// CPU relative Operation
//
void MsOS_Sync(void);

typedef enum //_MsOSMPool_DbgLevel
{
    E_MsOSMPool_DBG_Release = 0,
    E_MsOSMPool_DBG_L1, // display error msg
} MsOSMPool_DbgLevel;

// Kernel related information
MS_BOOL MDrv_SYS_Info(IO_Sys_Info_t* SysInfo);
// MPool Operation
void  MsOS_MPool_SetDbgLevel(MsOSMPool_DbgLevel DbgLevel);
MS_BOOL MsOS_MPool_Init(void);
MS_BOOL MsOS_MPool_Get(void** pAddrVirt, MS_U32* pu32AddrPhys, MS_U32* pu32Size, MS_BOOL bNonCache);
MS_BOOL MsOS_MPool_Close(void);
MS_U32 MsOS_MPool_VA2PA(MS_U32 pAddrVirt);
MS_U32 MsOS_MPool_PA2KSEG1(MS_U32 pAddrPhys);
MS_U32 MsOS_MPool_PA2KSEG0(MS_U32 pAddrPhys);
MS_BOOL MsOS_MPool_Dcache_Flush(MS_U32 pAddrVirt, MS_U32 u32Size); // the input address should be user mode cacheable address
MS_BOOL MsOS_MPool_Mapping(MS_U8 u8MiuSel, MS_U32 u32Offset, MS_U32 u32MapSize, MS_BOOL bNonCache);
MS_BOOL MsOS_MPool_Kernel_Detect(MS_U32 *lx_addr, MS_U32 *lx_size, MS_U32 *lx2_addr, MS_U32 *lx2_size);

#define MsOS_MPool_PA2VA MsOS_MPool_PA2KSEG1

// Share memory operation
#define MAX_CLIENT_NAME_LENGTH  50
#define MSOS_SHM_QUERY          0x00000000
#define MSOS_SHM_CREATE         0x00000001

MS_BOOL MsOS_SHM_Init(void);
MS_BOOL MsOS_SHM_GetId(MS_U8* pu8ClientName, MS_U32 u32BufSize, MS_U32* pu32ShmId, MS_U32* pu32Addr, MS_U32* pu32BufSize, MS_U32 u32Flag);
MS_BOOL MsOS_SHM_FreeId(MS_U8* pu8ClientName, MS_U32 u32ShmId);

//
// OS Dependent Macro
//

// Worldwide thread safe macro
// Usage:
//     MS_S32 os_X_MutexID;
//     os_X_MutexID = OS_CREATE_MUTEX(_M_);
//     if (os_X_MutexID < 0) {  return FALSE; }
//     if (OS_OBTAIN_MUTEX(os_X_MutexID, 1000) == FALSE) { return FALSE; }
//     ...
//     OS_RELEASE_MUTEX(os_X_MutexID);
//     return X;
//

#define OS_CREATE_MUTEX(_M_)        MsOS_CreateMutex(E_MSOS_FIFO, "OS_"#_M_"_Mutex", MSOS_PROCESS_SHARED)
#define OS_OBTAIN_MUTEX(_mx, _tm)   MsOS_ObtainMutex(_mx, _tm)
#define OS_RELEASE_MUTEX(_mx)       MsOS_ReleaseMutex(_mx)
#define OS_DELETE_MUTEX(_mx)        MsOS_DeleteMutex(_mx)
#define OS_DELAY_TASK(_msec)        MsOS_DelayTask(_msec)
#define OS_SYSTEM_TIME()            MsOS_GetSystemTime()
#define OS_ENTER_CRITICAL()         MsOS_DisableAllInterrupts();
#define OS_EXIT_CRITICAL()          MsOS_EnableAllInterrupts();

//-------------------------------------------------------------------------------------------------
// Virutal/Physial address operation
//-------------------------------------------------------------------------------------------------
MS_U32 MsOS_VA2PA(MS_U32 addr);
MS_U32 MsOS_PA2KSEG0(MS_U32 addr);
MS_U32 MsOS_PA2KSEG1_1(MS_U32 addr);
void   MsOS_FlushMemory(void);
void   MsOS_ReadMemory(void);

#define MS_VA2PA(_addr_)                (MS_U32)MsOS_VA2PA((_addr_))
#define MS_PA2KSEG0(_addr_)             (MS_U32)MsOS_PA2KSEG0((_addr_))
#define MS_PA2KSEG1(_addr_)             (MS_U32)MsOS_PA2KSEG1_1((_addr_))

//-------------------------------------------------------------------------------------------------
// Debug message
//-------------------------------------------------------------------------------------------------
#define MS_CRITICAL_MSG(x)      x       // for dump critical message
#define MS_FATAL_MSG(fmt,...) printf( "[MS_FATAL]: %s: %d \n"  fmt, __FUNCTION__, __LINE__, ## __VA_ARGS__)


#if defined (MSOS_TYPE_LINUX_KERNEL)
#define printf(_fmt, _args...)        printk(KERN_WARNING _fmt, ## _args)
//#define printf printk
#elif defined (MSOS_TYPE_CE)

typedef const wchar_t* LPCWSTR;
extern void NKDbgPrintfW(LPCWSTR lpszFmt, ...);
#define TXT(quote) L##quote
#define printf(fmt, ...) NKDbgPrintfW(TXT(fmt), __VA_ARGS__)

#endif
#if defined (MS_DEBUG)
  #define MS_DEBUG_MSG(x)       x
#elif defined (MS_OPTIMIZE)
  #define MS_DEBUG_MSG(x)               // retail version remove debug message
#endif

//-------------------------------------------------------------------------------------------------
// debug
//-------------------------------------------------------------------------------------------------
extern void MsOS_RegMyDbg(void);                                        ///< MsOS debug register itself debug

typedef MS_BOOL (*UartDbg_IP_CallBack) (int argc, char *argv[]);        ///< MsOS debug call back function prototype

///< define your own name, help, callback mapping here
typedef struct
{
    const char *Func_Name;
    const char *Func_Help;
    UartDbg_IP_CallBack pCallBack;
} MS_DBG_LINK;

///< for application to pass debug command into MsOS debug module
extern MS_BOOL MsOS_Dbg_ParseCmd(char *Cmd, MS_U32 u32CmdLen);

///< register your main menu here
extern MS_BOOL MsOS_Dbg_Regist(const char *Func_Name, const char *Func_Help, MS_DBG_LINK *pAryDbgLink);

///< for user to handle their own sub menu
extern MS_BOOL MsOS_Dbg_ExecuteSubCB(const char *Func_Name, int argc, char *argv[], MS_DBG_LINK *pAryDbgLink);

#ifdef __cplusplus
}
#endif

#endif // _MS_OS_H_
