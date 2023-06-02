////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2006-2011 MStar Semiconductor, Inc.
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

#ifndef _MDRV_SCL_DBG_H
#define _MDRV_SCL_DBG_H
typedef enum
{
    EN_DBGMG_MDRV_CONFIG        = 0,
    EN_DBGMG_IOCTL_CONFIG       = 1,
    EN_DBGMG_HVSP_CONFIG        = 2,      ///< HVSP1
    EN_DBGMG_SCLDMA_CONFIG      = 3,      ///< SCLDMA1
    EN_DBGMG_PNL_CONFIG         = 4,    ///< PNL
    EN_DBGMG_VIP_CONFIG         = 5,     ///< VIP
    EN_DBGMG_DRVPQ_CONFIG       = 6,
    EN_DBGMG_INST_ENTRY_CONFIG  = 7,
    EN_DBGMG_INST_LOCK_CONFIG   = 8,
    EN_DBGMG_INST_FUNC_CONFIG   = 9,
    EN_DBGMG_DRVHVSP_CONFIG     = 10,
    EN_DBGMG_DRVSCLDMA_CONFIG   = 11,
    EN_DBGMG_DRVSCLIRQ_CONFIG   = 12,
    EN_DBGMG_DRVCMDQ_CONFIG     = 13,
    EN_DBGMG_DRVVIP_CONFIG      = 14,
    EN_DBGMG_PRIORITY_CONFIG    = 15,
    EN_DBGMG_NUM_CONFIG         = 16,
}EN_DBGMG_CONFIG_TYPE;
typedef enum
{
    EN_DBGMG_HVSPLEVEL_HVSP1 = 0x1,
    EN_DBGMG_HVSPLEVEL_HVSP2 = 0x2,
    EN_DBGMG_HVSPLEVEL_HVSP3 = 0x4,
    EN_DBGMG_HVSPLEVEL_ELSE  = 0x8,
}EN_DBGMG_HVSPLEVEL_TYPE;
typedef enum
{
    EN_DBGMG_SCLDMALEVEL_SC1FRM         = 0x1,
    EN_DBGMG_SCLDMALEVEL_SC1SNP         = 0x2,
    EN_DBGMG_SCLDMALEVEL_SC2FRM         = 0x4,
    EN_DBGMG_SCLDMALEVEL_SC3FRM         = 0x8,// always high fq
    EN_DBGMG_SCLDMALEVEL_SC1FRMHIGH     = 0x10,
    EN_DBGMG_SCLDMALEVEL_SC1SNPHIGH     = 0x20,
    EN_DBGMG_SCLDMALEVEL_SC2FRMHIGH     = 0x40,
    EN_DBGMG_SCLDMALEVEL_ELSE           = 0x80,
}EN_DBGMG_SCLDMALEVEL_TYPE;
typedef enum
{
    EN_DBGMG_VIPLEVEL_NORMAL     = 0x1,
    EN_DBGMG_VIPLEVEL_VIPLOG     = 0x2,
    EN_DBGMG_VIPLEVEL_VIPSUP     = 0x4,
    EN_DBGMG_VIPLEVEL_ELSE       = 0x8,
}EN_DBGMG_VIPLEVEL_TYPE;
typedef enum
{
    EN_DBGMG_SCLIRQLEVEL_NORMAL     = 0x1,
    EN_DBGMG_SCLIRQLEVEL_SC1RINGA   = 0x2,
    EN_DBGMG_SCLIRQLEVEL_SC1RINGN   = 0x4,
    EN_DBGMG_SCLIRQLEVEL_SC1SINGLE  = 0x8,
    EN_DBGMG_SCLIRQLEVEL_SC2RINGA   = 0x10,
    EN_DBGMG_SCLIRQLEVEL_SC2RINGN   = 0x20,
    EN_DBGMG_SCLIRQLEVEL_SC3SINGLE  = 0x40,
    EN_DBGMG_SCLIRQLEVEL_ELSE       = 0x80,
}EN_DBGMG_SCLIRQLEVEL_TYPE;
typedef enum
{
    EN_DBGMG_CMDQEVEL_LOW     = 0x1,  // by call
    EN_DBGMG_CMDQEVEL_NORMAL  = 0x2,  // by cmd
    EN_DBGMG_CMDQEVEL_HIGH    = 0x4,  // by loop in cmd
    EN_DBGMG_CMDQEVEL_ISR     = 0x8,
}EN_DBGMG_CMDQEVEL_TYPE;
typedef enum
{
    EN_DBGMG_MULTI_SC1  = 0x1,
    EN_DBGMG_MULTI_SC3  = 0x2,
    EN_DBGMG_MULTI_DSP  = 0x4,
    EN_DBGMG_MULTI_ELSE = 0x8,
}EN_DBGMG_MULTI_TYPE;

typedef enum
{
    EN_DBGMG_IOCTLEVEL_SC1          = 0x1,
    EN_DBGMG_IOCTLEVEL_SC2          = 0x2,
    EN_DBGMG_IOCTLEVEL_SC3          = 0x4, //alway high
    EN_DBGMG_IOCTLEVEL_VIP          = 0x8,
    EN_DBGMG_IOCTLEVEL_SC1HLEVEL    = 0x10,
    EN_DBGMG_IOCTLEVEL_SC2HLEVEL    = 0x20,
    EN_DBGMG_IOCTLEVEL_ELSE         = 0x40,
}EN_DBGMG_IOCTLEVEL_TYPE;
typedef enum
{
    EN_DBG_HVSP1_CONFIG         = 0x1,      ///< HVSP1
    EN_DBG_HVSP2_CONFIG         = 0x2,      ///< HVSP2
    EN_DBG_HVSP3_CONFIG         = 0x4,      ///< HVSP3
    EN_DBG_SCLDMA1_CONFIG       = 0x8,      ///< SCLDMA1
    EN_DBG_SCLDMA2_CONFIG       = 0x10,     ///< SCLDMA2
    EN_DBG_SCLDMA3_CONFIG       = 0x20,     ///< SCLDMA3
    EN_DBG_SCLDMA4_CONFIG       = 0x40,     ///< SCLDMA4
    EN_DBG_VIP_CONFIG           = 0x80,     ///< VIP
    EN_DBG_PNL_CONFIG           = 0x100,    ///< PNL
    EN_DBG_SCL_CONFIG           = 0x1FF,
}__attribute__ ((__packed__))EN_DBG_CONFIG_TYPE;
extern unsigned int gu8DNRBufferReadyNum;
extern unsigned int gbProbeAlready;
extern unsigned char gbdbgmessage[EN_DBGMG_NUM_CONFIG];

#define  SCL_IO_DBG_ENABLE 1

#define SCL_DBG_LV_MDRV_IO()          (gbdbgmessage[EN_DBGMG_MDRV_CONFIG])
#define SCL_DBG_LV_IOCTL()            (gbdbgmessage[EN_DBGMG_IOCTL_CONFIG])
#define SCL_DBG_LV_0                1
#define SCL_DBG_LV_HVSP()             (gbdbgmessage[EN_DBGMG_HVSP_CONFIG])
#define SCL_DBG_LV_SCLDMA()           (gbdbgmessage[EN_DBGMG_SCLDMA_CONFIG])
#define SCL_DBG_LV_PNL()              (gbdbgmessage[EN_DBGMG_PNL_CONFIG])
#define SCL_DBG_LV_VIP()              (gbdbgmessage[EN_DBGMG_VIP_CONFIG])
#define SCL_DBG_LV_VIP_LOG()          (gbdbgmessage[EN_DBGMG_VIP_CONFIG]&EN_DBGMG_VIPLEVEL_VIPLOG)

#define SCL_DBG_LV_DRVHVSP()             (gbdbgmessage[EN_DBGMG_DRVHVSP_CONFIG])
#define SCL_DBG_LV_DRVSCLDMA()           (gbdbgmessage[EN_DBGMG_DRVSCLDMA_CONFIG])
#define SCL_DBG_LV_DRVVIP()              (gbdbgmessage[EN_DBGMG_DRVVIP_CONFIG])
#define SCL_DBG_LV_DRVSCLIRQ()           (gbdbgmessage[EN_DBGMG_DRVSCLIRQ_CONFIG])
#define SCL_DBG_LV_DRVCMDQ()           (gbdbgmessage[EN_DBGMG_DRVCMDQ_CONFIG])
#define SCL_DBG_LV_DRVPQ()           (gbdbgmessage[EN_DBGMG_DRVPQ_CONFIG])

#define SCL_DBG_LV_MULTI_INST_ERR   1
#define SCL_DBG_LV_MULTI_INST_ENTRY() (gbdbgmessage[EN_DBGMG_INST_ENTRY_CONFIG])
#define SCL_DBG_LV_MULTI_INST_LOCK()  (gbdbgmessage[EN_DBGMG_INST_LOCK_CONFIG])
#define SCL_DBG_LV_MULTI_INST_FUNC()  (gbdbgmessage[EN_DBGMG_INST_FUNC_CONFIG])
#define SCL_DBG_LV_MULTI_INST_LOCK_LOG  (0)

#define Reset_DBGMG_FLAG()        \
({                                                               \
    unsigned char i;                                            \
    for(i=0;i<EN_DBGMG_NUM_CONFIG;i++)                          \
        gbdbgmessage[i] = 0;                                         \
})

#define Set_DBGMG_FLAG(var,level)        \
    ({                                                               \
        gbdbgmessage[var] = level;                                                         \
    })

#define Get_DBGMG_HVSP(hvspid)        \
    ({                                                               \
    unsigned char bH;\
        if(hvspid == 0)                                                         \
            bH = EN_DBGMG_HVSPLEVEL_HVSP1; \
        else if(hvspid == 1)\
            bH = EN_DBGMG_HVSPLEVEL_HVSP2; \
            else if(hvspid == 2)\
                bH = EN_DBGMG_HVSPLEVEL_HVSP3; \
                else \
                   bH = EN_DBGMG_HVSPLEVEL_ELSE; \
                   bH;\
    })

#define Get_DBGMG_MULTI(multiid)        \
                    ({                                                               \
                    unsigned char bH;\
                        if(multiid == 0)                                                         \
                            bH = EN_DBGMG_MULTI_SC1; \
                        else if(multiid == 1)\
                            bH = EN_DBGMG_MULTI_SC3; \
                            else if(multiid == 2)\
                                bH = EN_DBGMG_MULTI_DSP; \
                                else \
                                   bH = EN_DBGMG_MULTI_ELSE; \
                                   bH;\
                    })


#define Get_DBGMG_SCLDMA(scldmaid,rw,level)        \
                    ({                                                               \
                    unsigned char bH;\
                        if(scldmaid == 0)  \
                        {\
                            if(rw==0)\
                            {\
                                if(level==0)\
                                    bH = EN_DBGMG_SCLDMALEVEL_SC1FRM; \
                                else\
                                    bH = EN_DBGMG_SCLDMALEVEL_SC1FRMHIGH; \
                            }\
                            else if(rw==1)\
                            {\
                                if(level==0)\
                                    bH = EN_DBGMG_SCLDMALEVEL_SC1SNP; \
                                else\
                                    bH = EN_DBGMG_SCLDMALEVEL_SC1SNPHIGH; \
                            }\
                        }\
                        else if(scldmaid == 1)\
                        {\
                            if(level==0)\
                                bH = EN_DBGMG_SCLDMALEVEL_SC2FRM; \
                            else\
                                bH = EN_DBGMG_SCLDMALEVEL_SC2FRMHIGH; \
                        }\
                        else if(scldmaid == 2)\
                        {\
                            bH = EN_DBGMG_SCLDMALEVEL_SC3FRM; \
                        }\
                        else \
                         bH = EN_DBGMG_HVSPLEVEL_ELSE; \
                                   bH;\
                    })

#define Get_DBGMG_SCLIRQclient(scldmacl,level)        \
                                            ({                                                               \
                                            unsigned char bH;\
                                                if(scldmacl == 0)  \
                                                {\
                                                        if(level==0)\
                                                            bH = EN_DBGMG_SCLIRQLEVEL_SC1RINGA; \
                                                        else\
                                                            bH = EN_DBGMG_SCLIRQLEVEL_SC1RINGN; \
                                                }\
                                                else if(scldmacl == 1)\
                                                {\
                                                        bH = EN_DBGMG_SCLIRQLEVEL_SC1SINGLE; \
                                                }\
                                                else if(scldmacl == 3)\
                                                {\
                                                    if(level==0)\
                                                        bH = EN_DBGMG_SCLIRQLEVEL_SC2RINGA; \
                                                    else\
                                                        bH = EN_DBGMG_SCLIRQLEVEL_SC2RINGN; \
                                                }\
                                                    else if(scldmacl == 5||scldmacl == 6)\
                                                    {\
                                                            bH = EN_DBGMG_SCLIRQLEVEL_SC3SINGLE; \
                                                    }\
                                                else \
                                                {\
                                                    if(level==0)\
                                                        bH = EN_DBGMG_SCLIRQLEVEL_NORMAL; \
                                                    else\
                                                        bH = EN_DBGMG_SCLIRQLEVEL_ELSE; \
                                                }\
                                                           bH;\
                                            })

#define Get_DBGMG_SCLDMAclient(scldmacl,level)        \
                                                ({                                                               \
                                                unsigned char bH;\
                                                    if(scldmacl == 0)  \
                                                    {\
                                                            if(level==0)\
                                                                bH = EN_DBGMG_SCLDMALEVEL_SC1FRM; \
                                                            else\
                                                                bH = EN_DBGMG_SCLDMALEVEL_SC1FRMHIGH; \
                                                    }\
                                                    else if(scldmacl == 1)\
                                                    {\
                                                        if(level==0)\
                                                            bH = EN_DBGMG_SCLDMALEVEL_SC1SNP; \
                                                        else\
                                                            bH = EN_DBGMG_SCLDMALEVEL_SC1SNPHIGH; \
                                                    }\
                                                    else if(scldmacl == 3)\
                                                    {\
                                                        if(level==0)\
                                                            bH = EN_DBGMG_SCLDMALEVEL_SC2FRM; \
                                                        else\
                                                            bH = EN_DBGMG_SCLDMALEVEL_SC2FRMHIGH; \
                                                    }\
                                                        else if(scldmacl == 5||scldmacl == 6)\
                                                        {\
                                                            if(level==3)\
                                                                bH = EN_DBGMG_SCLDMALEVEL_ELSE; \
                                                                else\
                                                                    bH = EN_DBGMG_SCLDMALEVEL_SC3FRM;\
                                                        }\
                                                    else \
                                                     bH = EN_DBGMG_SCLDMALEVEL_ELSE; \
                                                               bH;\
                                                })


#ifdef   SCL_IO_DBG_ENABLE
#define SCL_ASSERT(_con)   \
    do {\
        if (!(_con)) {\
            printk(KERN_CRIT "BUG at %s:%d assert(%s)\n",\
                    __FILE__, __LINE__, #_con);\
            BUG();\
        }\
    } while (0)

#define SCL_DBG(dbglv, _fmt, _args...)             \
    do                                             \
    if(dbglv)                                      \
    {                                              \
        if(gbdbgmessage[EN_DBGMG_PRIORITY_CONFIG]) \
            printk(KERN_WARNING _fmt, ## _args);       \
        else \
            printk(KERN_DEBUG _fmt, ## _args);       \
    }while(0)
#define SCL_ERR(_fmt, _args...)       printk(KERN_WARNING _fmt, ## _args)
#define VersionCheckSuccess 0

#else
#define     SCL_ASSERT(arg)
#define     SCL_DBG(dbglv, _fmt, _args...)

#endif




#endif
