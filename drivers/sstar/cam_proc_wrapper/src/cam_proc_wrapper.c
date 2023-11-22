/*
 * cam_proc_wrapper.c- Sigmastar
 *
 * Copyright (c) [2019~2020] SigmaStar Technology.
 *
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 2 for more details.
 *
 */

#if defined(__KERNEL__)
#define CAM_OS_LINUX_KERNEL
#endif

#ifdef CAM_OS_RTK
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "sys_sys_isw_cli.h"
#elif defined(CAM_OS_LINUX_KERNEL)
#include <linux/uaccess.h>
#include <linux/proc_fs.h>
#endif

#include "cam_os_wrapper.h"
#include "cam_proc_common.h"
#include "cam_proc_wrapper.h"

struct CamOsListHead_t gProcRootList = {&gProcRootList, &gProcRootList};
CamOsMutex_t           gProcMutex;
u8                     gbIsProcInit;

#ifdef CAM_OS_RTK
#define _CAMPROC_IS_WHITE(x) ((x) == ' ' || (x) == '\t' || (x) == '\\' || (x) == '/')
#define _CAMPROC_IS_SPACE(x) _CAMPROC_IS_WHITE(x)
#define _CAMPROC_EAT_WHITE(x)       \
    while (_CAMPROC_IS_WHITE(*(x))) \
        x++;
#define _CAMPROC_EAT_NON_WHITE(x)            \
    while (*(x) && !_CAMPROC_IS_WHITE(*(x))) \
        x++;

static CamProcEntryPriv_t *_CamProcFindEntryByName(struct CamOsListHead_t *pSrcList, const char *name)
{
    struct CamOsListHead_t *l;
    CamProcEntryPriv_t *    pTmpListEntry;

    CAM_OS_LIST_FOR_EACH(l, pSrcList)
    {
        pTmpListEntry = CAM_OS_LIST_ENTRY(l, CamProcEntryPriv_t, link);
        if (strcmp(pTmpListEntry->aEntryName, name) == 0)
        {
            return pTmpListEntry;
        }
    }
    return NULL;
}

static void _CamProcListNodesPerEntryEx(struct CamOsListHead_t *pSrcList, char *pPathBuf, int nPathBufLen)
{
    struct CamOsListHead_t *l;
    CamProcEntryPriv_t *    pTmpListEntry;

    CAM_OS_LIST_FOR_EACH(l, pSrcList)
    {
        pTmpListEntry = CAM_OS_LIST_ENTRY(l, CamProcEntryPriv_t, link);
        if (pTmpListEntry->pfReadCb || pTmpListEntry->pfWriteCb)
        {
            // it's a node
            CamOsPrintf("/proc%s%s\n", pPathBuf, pTmpListEntry->aEntryName);
        }
    }

    CAM_OS_LIST_FOR_EACH(l, pSrcList)
    {
        pTmpListEntry = CAM_OS_LIST_ENTRY(l, CamProcEntryPriv_t, link);
        if (pTmpListEntry->pfReadCb == NULL && pTmpListEntry->pfWriteCb == NULL)
        {
            char *pTemp;

            if (strlen(pPathBuf) + strlen(pTmpListEntry->aEntryName) >= nPathBufLen)
            {
                CamOsPrintf("%s : Path buffer is not enough!\n", __FUNCTION__);
                return;
            }

            pTemp = pPathBuf + strlen(pPathBuf);
            sprintf(pTemp, "%s/", pTmpListEntry->aEntryName);
            CamOsPrintf("/proc%s\n", pPathBuf);

            // it's a directory entry, recursively search
            _CamProcListNodesPerEntryEx(&pTmpListEntry->sSubNodeList, pPathBuf, nPathBufLen);

            *pTemp = 0;
        }
    }
}

char *_CamProcParsingToken(char *pSourceStr, char *pOutBuf, int nBufLen)
{
    int   i        = 0;
    char *pTmpChar = pSourceStr;
    u32   nSourceRemainLen;

    _CAMPROC_EAT_WHITE(pTmpChar)
    nSourceRemainLen = strlen(pTmpChar);

    while (i < nBufLen && nSourceRemainLen)
    {
        if (_CAMPROC_IS_SPACE(*pTmpChar))
        {
            break;
        }

        *(pOutBuf + i) = *pTmpChar;
        pTmpChar++;
        i++;
        nSourceRemainLen--;
    }

    *(pOutBuf + i) = 0;
    return pTmpChar;
}

void CamProcExecRead(char *name, CamProcSeqFile_t *pSeq)
{
    CamProcEntryPriv_t *    pTargetEntry = NULL;
    struct CamOsListHead_t *pSourceList  = NULL;
    char                    aTargetName[32];
    char *                  pTmpChar = NULL;

    pTargetEntry   = NULL;
    pSourceList    = &gProcRootList;
    aTargetName[0] = 0;
    pTmpChar       = name;

    CamOsMutexLock(&gProcMutex);

    while (1)
    {
        pTmpChar = _CamProcParsingToken(pTmpChar, aTargetName, sizeof(aTargetName));
        if (aTargetName[0] == 0)
            break;
        pTargetEntry = _CamProcFindEntryByName(pSourceList, aTargetName);

        if (pTargetEntry)
            pSourceList = &pTargetEntry->sSubNodeList;
        else
            break;
    }

    if (pTargetEntry && pTargetEntry->pfReadCb)
    {
        pTargetEntry->pfReadCb(pSeq, pTargetEntry->pPrivData);
    }

    CamOsMutexUnlock(&gProcMutex);
}

void CamProcExecWrite(char *name, char *pBuf)
{
    CamProcEntryPriv_t *    pTargetEntry = NULL;
    struct CamOsListHead_t *pSourceList  = NULL;
    char                    aTargetName[32];
    char *                  pTmpChar = NULL;

    pTargetEntry   = NULL;
    pSourceList    = &gProcRootList;
    aTargetName[0] = 0;
    pTmpChar       = name;

    CamOsMutexLock(&gProcMutex);

    while (1)
    {
        pTmpChar = _CamProcParsingToken(pTmpChar, aTargetName, sizeof(aTargetName));
        if (aTargetName[0] == 0)
            break;
        pTargetEntry = _CamProcFindEntryByName(pSourceList, aTargetName);

        if (pTargetEntry)
            pSourceList = &pTargetEntry->sSubNodeList;
        else
            break;
    }

    if (pTargetEntry && pTargetEntry->pfWriteCb)
    {
        pTargetEntry->pfWriteCb(pBuf, strlen(pBuf), pTargetEntry->pPrivData);
    }

    CamOsMutexUnlock(&gProcMutex);
}

void CamProcListEntry(char *name)
{
    CamProcEntryPriv_t *    pTargetEntry;
    struct CamOsListHead_t *pSourceList;
    char                    aTargetName[128]; // including path buffer
    char *                  pTmpChar;

    pTargetEntry   = NULL;
    pSourceList    = &gProcRootList;
    aTargetName[0] = 0;
    pTmpChar       = name;

    CamOsMutexLock(&gProcMutex);

    // find the root directory entry in path which is stored in pTmpChar
    while (1)
    {
        pTmpChar = _CamProcParsingToken(pTmpChar, aTargetName, sizeof(aTargetName));
        if (aTargetName[0] == 0)
            break;
        pTargetEntry = _CamProcFindEntryByName(pSourceList, aTargetName);

        if (pTargetEntry)
            pSourceList = &pTargetEntry->sSubNodeList;
        else
            break;
    }

    if (pSourceList != &gProcRootList)
    {
        CamOsPrintf("%s/\n", pTargetEntry->aEntryName);
        sprintf(aTargetName, "%s/", pTargetEntry->aEntryName);
    }
    else
    {
        aTargetName[0] = '/';
        aTargetName[1] = 0;
    }

    _CamProcListNodesPerEntryEx(pSourceList, aTargetName, sizeof(aTargetName));

    CamOsMutexUnlock(&gProcMutex);
}
#elif defined(CAM_OS_LINUX_KERNEL)
static int _CamProcShowLinux(struct seq_file *m, void *v)
{
    CamProcNodeOp_t *op = m->private;

    if (op->cat)
        op->cat((CamProcSeqFile_t *)m, op->pPrivData);

    return 0;
}

static int _CamProcOpenLinux(struct inode *inode, struct file *file)
{
    return single_open(file, _CamProcShowLinux, PDE_DATA(inode));
}

static ssize_t _CamProcWriteLinux(struct file *file, const char __user *buf, size_t count, loff_t *ppos)
{
    CamProcNodeOp_t *op = ((struct seq_file *)file->private_data)->private;

    if (op->echo)
    {
        u32   n;
        void *tmp_buf;
        tmp_buf = CamOsMemAlloc(count);
        if (!tmp_buf)
        {
            printk(KERN_ERR "%s %d No available memory\n", __FUNCTION__, __LINE__);
            return count;
        }
        n = copy_from_user(tmp_buf, buf, count);
        if (count)
        {
            *((u8 *)tmp_buf + count - 1) = 0;
        }
        op->echo(tmp_buf, count, op->pPrivData);
        CamOsMemRelease(tmp_buf);
    }

    return count;
}
#endif

static CamProcEntryPriv_t *_CamProcFindEntry(struct CamOsListHead_t *pSrcList, CamProcEntryPriv_t *pDstEntry)
{
    CamProcEntryPriv_t *    pSubEntry = NULL;
    struct CamOsListHead_t *l;
    CamProcEntryPriv_t *    pTmpListEntry;

    CAM_OS_LIST_FOR_EACH(l, pSrcList)
    {
        pTmpListEntry = CAM_OS_LIST_ENTRY(l, CamProcEntryPriv_t, link);
        if (pTmpListEntry == pDstEntry)
        {
            return pTmpListEntry;
        }
        else
        {
            pSubEntry = _CamProcFindEntry(&pTmpListEntry->sSubNodeList, pDstEntry);
            if (pSubEntry)
                return pSubEntry;
        }
    }

    return NULL;
}

static CamProcEntryPriv_t *_CamProcFindEntryByNameEx(struct CamOsListHead_t *pSrcList, const char *name)
{
    CamProcEntryPriv_t *    pFoundSubEntry = NULL;
    struct CamOsListHead_t *l;
    CamProcEntryPriv_t *    pTmpListEntry;

    CAM_OS_LIST_FOR_EACH(l, pSrcList)
    {
        pTmpListEntry = CAM_OS_LIST_ENTRY(l, CamProcEntryPriv_t, link);

        if (strcmp(pTmpListEntry->aEntryName, name) == 0)
        {
            return pTmpListEntry;
        }
        else
        {
            pFoundSubEntry = _CamProcFindEntryByNameEx(&pTmpListEntry->sSubNodeList, name);
            if (pFoundSubEntry)
                return pFoundSubEntry;
        }
    }
    return NULL;
}

static CamProcEntry_t *_CamProcCreateEntryNode(const char *name, CamProcEntryPriv_t *pParentEntry,
                                               CamProcReadCb pReadCb, CamProcWriteCb pWriteCb, void *pPrivData)
{
#ifdef CAM_OS_RTK
    int                     i;
    CamProcEntryPriv_t *    pFoundEntry    = NULL;
    struct CamOsListHead_t *pRootEntryList = NULL;
    CamProcEntryPriv_t *    pNewEntry      = NULL;

    pNewEntry = CamOsMemAlloc(sizeof(CamProcEntryPriv_t));

    if (!pNewEntry)
        return NULL;

    if (!gbIsProcInit)
    {
        gbIsProcInit = 1;
        CamOsMutexInit(&gProcMutex);
    }

    CAM_OS_INIT_LIST_HEAD(&pNewEntry->link);
    CAM_OS_INIT_LIST_HEAD(&pNewEntry->sSubNodeList);
    for (i = 0; i < strlen(name) && i < sizeof(pNewEntry->aEntryName) - 1; i++)
    {
        pNewEntry->aEntryName[i] = *(name + i);
    }
    pNewEntry->aEntryName[i] = 0;
    pNewEntry->pfReadCb      = pReadCb;
    pNewEntry->pfWriteCb     = pWriteCb;
    pNewEntry->pPrivData     = pPrivData;

    CamOsMutexLock(&gProcMutex);

    if (pParentEntry)
    {
        pFoundEntry = _CamProcFindEntry(&gProcRootList, (CamProcEntryPriv_t *)pParentEntry);
    }

    if (pFoundEntry)
    {
        pRootEntryList = &pFoundEntry->sSubNodeList;
    }
    else
    {
        pRootEntryList = &gProcRootList;
    }

    CAM_OS_LIST_ADD_TAIL(&pNewEntry->link, pRootEntryList);

    CamOsMutexUnlock(&gProcMutex);

    return (void *)pNewEntry;
#elif defined(CAM_OS_LINUX_KERNEL)
    int                     i;
    CamProcEntryPriv_t *    pFoundEntry    = NULL;
    struct CamOsListHead_t *pRootEntryList = NULL;
    CamProcEntryPriv_t *    pNewEntry      = NULL;
    struct proc_dir_entry * pNewPde        = NULL;
    CamProcLinuxPrivate_t * pNewPrivate    = NULL;

    if (pReadCb || pWriteCb)
    {
        pNewPrivate = CamOsMemAlloc(sizeof(CamProcLinuxPrivate_t));
        if (!pNewPrivate)
            return NULL;

        pNewPrivate->sProcOps.cat       = pReadCb;
        pNewPrivate->sProcOps.echo      = pWriteCb;
        pNewPrivate->sProcOps.pPrivData = pPrivData;

        memset((void *)&pNewPrivate->pFileOps, 0, sizeof(struct proc_ops));
        pNewPrivate->pFileOps.proc_open    = _CamProcOpenLinux;
        pNewPrivate->pFileOps.proc_write   = _CamProcWriteLinux;
        pNewPrivate->pFileOps.proc_read    = seq_read;
        pNewPrivate->pFileOps.proc_lseek   = seq_lseek;
        pNewPrivate->pFileOps.proc_release = single_release;

        if (pParentEntry)
            pNewPde =
                proc_create_data(name, S_IRUGO, pParentEntry->pde, &pNewPrivate->pFileOps, &pNewPrivate->sProcOps);
        else
            pNewPde = proc_create_data(name, S_IRUGO, NULL, &pNewPrivate->pFileOps, &pNewPrivate->sProcOps);
    }
    else
    {
        if (pParentEntry)
            pNewPde = proc_mkdir(name, pParentEntry->pde);
        else
            pNewPde = proc_mkdir(name, NULL);
    }

    if (!pNewPde)
    {
        return NULL;
    }

    pNewEntry = CamOsMemAlloc(sizeof(CamProcEntryPriv_t));

    if (!pNewEntry)
        return NULL;

    if (!gbIsProcInit)
    {
        gbIsProcInit = 1;
        CamOsMutexInit(&gProcMutex);
    }

    CAM_OS_INIT_LIST_HEAD(&pNewEntry->link);
    CAM_OS_INIT_LIST_HEAD(&pNewEntry->sSubNodeList);

    for (i = 0; i < strlen(name) && i < sizeof(pNewEntry->aEntryName) - 1; i++)
    {
        pNewEntry->aEntryName[i] = *(name + i);
    }
    pNewEntry->aEntryName[i] = 0;
    pNewEntry->pde           = pNewPde;
    pNewEntry->pdata         = pNewPrivate;

    CamOsMutexLock(&gProcMutex);

    if (pParentEntry)
    {
        pFoundEntry = _CamProcFindEntry(&gProcRootList, (CamProcEntryPriv_t *)pParentEntry);
    }

    if (pFoundEntry)
    {
        pRootEntryList = &pFoundEntry->sSubNodeList;
    }
    else
    {
        pRootEntryList = &gProcRootList;
    }

    CAM_OS_LIST_ADD_TAIL(&pNewEntry->link, pRootEntryList);

    CamOsMutexUnlock(&gProcMutex);

    return (void *)pNewEntry;
#endif
}

static void _CamProcRemoveAllSubNode(CamProcEntryPriv_t *pParentEntry, CamProcEntryPriv_t *pCurrentEntry)
{
    struct CamOsListHead_t *l;
    CamProcEntryPriv_t *    pSubNodeEntry;

    // CAM_OS_LIST_FOR_EACH(pos, head) : for (pos = (head)->pNext; pos != (head); pos = pos->pNext)
    CAM_OS_LIST_FOR_EACH(l, &pCurrentEntry->sSubNodeList)
    {
        pSubNodeEntry = CAM_OS_LIST_ENTRY(l, CamProcEntryPriv_t, link);
        _CamProcRemoveAllSubNode(pCurrentEntry, pSubNodeEntry);
        l = &pCurrentEntry->sSubNodeList;
    }

#ifdef CAM_OS_LINUX_KERNEL
    if (pParentEntry == pCurrentEntry || pParentEntry == NULL)
        remove_proc_entry(pCurrentEntry->aEntryName, NULL);
    else
        remove_proc_entry(pCurrentEntry->aEntryName, pParentEntry->pde);
    if (pCurrentEntry->pdata)
        CamOsMemRelease((void *)pCurrentEntry->pdata);
#endif

    CAM_OS_LIST_DEL(&pCurrentEntry->link);
    CamOsMemRelease((void *)pCurrentEntry);
}

static void _CamProcRemoveEntryNode(const char *name, CamProcEntryPriv_t *pParentEntry)
{
    CamProcEntryPriv_t *    pFoundEntry    = NULL;
    struct CamOsListHead_t *pRootEntryList = NULL;
    CamProcEntryPriv_t *    pRmEntry       = NULL;

    if (!gbIsProcInit)
    {
        gbIsProcInit = 1;
        CamOsMutexInit(&gProcMutex);
    }

    CamOsMutexLock(&gProcMutex);

    if (pParentEntry)
    {
        pFoundEntry = _CamProcFindEntry(&gProcRootList, (CamProcEntryPriv_t *)pParentEntry);
    }

    if (pFoundEntry)
    {
        pRootEntryList = &pFoundEntry->sSubNodeList;
    }
    else
    {
        pRootEntryList = &gProcRootList;
    }

    pRmEntry = _CamProcFindEntryByNameEx(pRootEntryList, name);

    if (pRmEntry)
    {
        _CamProcRemoveAllSubNode(pParentEntry, pRmEntry);
    }

    CamOsMutexUnlock(&gProcMutex);
}

CamProcEntry_t *CamProcMkdir(const char *name, CamProcEntry_t *pParentEntry)
{
    return _CamProcCreateEntryNode(name, (CamProcEntryPriv_t *)pParentEntry, NULL, NULL, NULL);
}

CamProcEntry_t *CamProcCreate(const char *name, CamProcEntry_t *pParentEntry, CamProcReadCb pReadCb,
                              CamProcWriteCb pWriteCb, void *pPrivData)
{
    return _CamProcCreateEntryNode(name, (CamProcEntryPriv_t *)pParentEntry, pReadCb, pWriteCb, pPrivData);
}

void CamProcRemoveEntry(const char *name, CamProcEntry_t *pEntry)
{
    _CamProcRemoveEntryNode(name, pEntry);
}

void CamProcSeqPrintf(CamProcSeqFile_t *pSeq, const char *fmt, ...)
{
#ifdef CAM_OS_RTK
    va_list args;

    if (pSeq)
    {
        va_start(args, fmt);
        pSeq->used += vsnprintf(pSeq->buf + pSeq->used, pSeq->size - pSeq->used, fmt, args);
        va_end(args);
    }
#elif defined(CAM_OS_LINUX_KERNEL)
    va_list args;
    va_start(args, fmt);
    seq_vprintf(pSeq, fmt, args);
    va_end(args);
#endif
}

#if defined(__KERNEL__)
EXPORT_SYMBOL(CamProcMkdir);
EXPORT_SYMBOL(CamProcCreate);
EXPORT_SYMBOL(CamProcRemoveEntry);
EXPORT_SYMBOL(CamProcSeqPrintf);
#endif
