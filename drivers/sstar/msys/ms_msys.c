/*
* ms_msys.c- Sigmastar
*
* Copyright (C) 2018 Sigmastar Technology Corp.
*
* Author: karl.xiao <karl.xiao@sigmastar.com.tw>
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/
/*
 * mdrv_system.c
 *
 *  Created on: 2012/9/21
 *      Author: Administrator
 */
#include <asm/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/dma-mapping.h>      /* for dma_alloc_coherent */
#include <linux/list.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/ctype.h>
#include <linux/swap.h>
#include <linux/vmalloc.h>
#include <linux/delay.h>
#include <linux/seq_file.h>
#include <linux/compaction.h>
#include <asm/cacheflush.h>

#include "ms_platform.h"
#include "registers.h"
#include "mdrv_msys_io_st.h"
#include "mdrv_msys_io.h"
#include "platform_msys.h"
#include "mdrv_verchk.h"
#include "mdrv_miu.h"
#include "mdrv_system.h"
#include "cam_os_wrapper.h"
#include "hal_bdma.h"
#include "hal_movedma.h"


#define MSYS_PERF_TEST          0

#if MSYS_PERF_TEST

#if 1
#define dmac_map_area			__glue(_CACHE,_dma_map_area)
#define dmac_unmap_area 		__glue(_CACHE,_dma_unmap_area)
extern void dmac_map_area(const void *, size_t, int);
extern void dmac_unmap_area(const void *, size_t, int);
#else
#define dmac_map_area(a, b, c) {}
#define dmac_unmap_area(a, b, c) {}
#endif


#include <linux/time.h>

#define STR_IMI         "IMI"
#define STR_MIU         "MIU"
#define STR_CACHE       "CACHE"
#define STR_CPUINFO     "CPUINFO"
#define STR_ALL         "ALL"

////////////////////////////////////////////
#define STR_CPUINFO_CA9         "ARMv7 CA9"
#define STR_CPUINFO_CA7         "ARMv7 CA7"
#define STR_CPUINFO_CA53        "ARMv8 CA53"
#define STR_CPUINFO_NULL        "CPU_NULL"
#define IRQ_LIST 1000
#define CPU_PART_CA7            0xC07
#define CPU_PART_CA9            0xC09
#define CPU_PART_CA53           0xD03
#define CPU_PART_NULL           0x000

#define IMI_ADDR_INVALID        0xFFFFFFFF
#define IMI_SIZE_INVALID        0xFFFFFFFF
#endif // #if MSYS_PERF_TEST

#define BENCH_MEMORY_FUNC            0
#define MSYS_DEBUG                   0
#define MINOR_SYS_NUM               128
#define MAJOR_SYS_NUM               233

#define MSYS_MIU_PROTECT        	1

#if MSYS_DEBUG
#define MSYS_PRINT(fmt, args...)    printk("[MSYS] " fmt, ## args)
#else
#define MSYS_PRINT(fmt, args...)
#endif

#define MSYS_ERROR(fmt, args...)    printk(KERN_ERR"MSYS: " fmt, ## args)
#define MSYS_WARN(fmt, args...)     printk(KERN_WARNING"MSYS: " fmt, ## args)


extern void Chip_Flush_Memory(void);

static int msys_open(struct inode *inode, struct file *filp);
static int msys_release(struct inode *inode, struct file *filp);
static long msys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

typedef struct
{
  MSYS_PROC_DEVICE proc_dev;
  void *proc_addr;
  struct proc_dir_entry* proc_entry;
  struct list_head list;
} PROC_INFO_LIST;

static int msys_request_proc_attr(MSYS_PROC_ATTRIBUTE* proc_attr);
static int msys_release_proc_attr(MSYS_PROC_ATTRIBUTE* proc_attr);
static int msys_request_proc_dev(MSYS_PROC_DEVICE* proc_dev);
static int msys_release_proc_dev(MSYS_PROC_DEVICE* proc_dev);

#if BENCH_MEMORY_FUNC==1
static void msys_bench_memory(unsigned int);
#endif

static struct file_operations msys_fops = {
    .owner = THIS_MODULE,
    .open = msys_open,
    .release = msys_release,
    .unlocked_ioctl=msys_ioctl,
};


static struct miscdevice sys_dev = {
    .minor      = MINOR_SYS_NUM,
    .name       = "msys",
    .fops       = &msys_fops,
};

static unsigned char data_part_string[32]={0};
static unsigned char system_part_string[32]={0};

//static unsigned char mstar_property_path[32]="/data";

static u64 sys_dma_mask = 0xffffffffUL;
struct list_head kept_mem_head;
struct list_head fixed_mem_head;
static struct mutex dmem_mutex;
static unsigned char fixed_dmem_enabled=0;
static unsigned char dmem_realloc_enabled=0;
//static unsigned long dmem_lock_flags;

static unsigned int dmem_retry_interval=100; //(ms)
static unsigned int dmem_retry_count=16;

#if defined(CONFIG_MS_BDMA)
static CamOsTsem_t m_stBdmaDoneSem[HAL_BDMA_CH_NUM];
#endif
#if defined(CONFIG_MS_MOVE_DMA)
static CamOsTsem_t m_stMdmaDoneSem;
#endif

struct DMEM_INFO_LIST
{
    struct list_head list;
    MSYS_DMEM_INFO dmem_info;
};


//port from fs/proc/meminfo.c
unsigned int meminfo_free_in_K(void)
{
    struct sysinfo i;
#if 0
    unsigned long committed;
    struct vmalloc_info vmi;
    long cached;
    long available;
    unsigned long pagecache;
    unsigned long wmark_low = 0;
    unsigned long pages[NR_LRU_LISTS];
    struct zone *zone;
    int lru;
#endif
    /*
     * display in kilobytes.
     */
    #define K(x) ((x) << (PAGE_SHIFT - 10))
    si_meminfo(&i);
    si_swapinfo(&i);
#if 0
    committed = percpu_counter_read_positive(&vm_committed_as);
    cached = global_page_state(NR_FILE_PAGES) -
            total_swapcache_pages() - i.bufferram;
    if (cached < 0)
        cached = 0;

    get_vmalloc_info(&vmi);

    for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
        pages[lru] = global_page_state(NR_LRU_BASE + lru);

    for_each_zone(zone)
        wmark_low += zone->watermark[WMARK_LOW];

    /*
     * Estimate the amount of memory available for userspace allocations,
     * without causing swapping.
     *
     * Free memory cannot be taken below the low watermark, before the
     * system starts swapping.
     */
    available = i.freeram - wmark_low;

    /*
     * Not all the page cache can be freed, otherwise the system will
     * start swapping. Assume at least half of the page cache, or the
     * low watermark worth of cache, needs to stay.
     */
    pagecache = pages[LRU_ACTIVE_FILE] + pages[LRU_INACTIVE_FILE];
    pagecache -= min(pagecache / 2, wmark_low);
    available += pagecache;

    /*
     * Part of the reclaimable slab consists of items that are in use,
     * and cannot be freed. Cap this estimate at the low watermark.
     */
    available += global_page_state(NR_SLAB_RECLAIMABLE) -
             min(global_page_state(NR_SLAB_RECLAIMABLE) / 2, wmark_low);

    if (available < 0)
        available = 0;
#endif
    return K(i.freeram);

}
EXPORT_SYMBOL(meminfo_free_in_K);



//static void *mm_mem_virt = NULL; /* virtual address of frame buffer 1 */


static int msys_open(struct inode *inode, struct file *filp)
{
//    printk(KERN_WARNING"%s():\n", __FUNCTION__);
    return 0;
}

static int msys_release(struct inode *inode, struct file *filp)
{
//    MSYS_PRINT(KERN_WARNING"%s():\n", __FUNCTION__);
    return 0;
}


int msys_fix_dmem(char* name)
{
    int err=0;
    struct list_head *ptr;
    struct DMEM_INFO_LIST *entry,*match_entry;
    match_entry=NULL;


    mutex_lock(&dmem_mutex);

    if(name!=NULL && name[0]!=0)
    {
        struct DMEM_INFO_LIST *new=NULL;
        list_for_each(ptr, &fixed_mem_head)
        {
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            if (0==strncmp(entry->dmem_info.name, name,strnlen(name,15)))
            {
                match_entry=entry;
                goto BEACH;
            }
        }


        new=(struct DMEM_INFO_LIST *)kmalloc(sizeof(struct DMEM_INFO_LIST),GFP_KERNEL);
        if(new==NULL)
        {
            MSYS_ERROR("allocate memory for fixed_mem_list entry error\n" ) ;
            err = -ENOMEM;
            goto BEACH;
        }
        memset(new->dmem_info.name,0,16);
        memcpy(new->dmem_info.name,name,strnlen(name,15));
        //memcpy(&new->dmem_info,&mem_info,sizeof(MSYS_DMEM_INFO));

        list_add(&new->list, &fixed_mem_head);
    }

BEACH:
    mutex_unlock(&dmem_mutex);
    return err;
}



int msys_unfix_dmem(char* name)
{

    //MSYS_DMEM_INFO mem_info;
    struct list_head *ptr;
    struct DMEM_INFO_LIST *entry,*match_entry;
    match_entry=NULL;

    mutex_lock(&dmem_mutex);

    if(name!=NULL && name[0]!=0)
    {
        list_for_each(ptr, &fixed_mem_head)
        {
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            if (0==strncmp(entry->dmem_info.name, name,strnlen(name,15)))
            {
                match_entry=entry;
                break;
            }
        }
    }

    if(match_entry!=NULL)
    {
        list_del_init(&match_entry->list);
        kfree(match_entry);
    }

//BEACH:
    mutex_unlock(&dmem_mutex);
    return 0;

}


int msys_find_dmem_by_phys(unsigned long long phys,MSYS_DMEM_INFO *mem_info)
{

    //MSYS_DMEM_INFO mem_info;
    struct list_head *ptr;
    struct DMEM_INFO_LIST *entry;

    int res=-EINVAL;

    mutex_lock(&dmem_mutex);

    if(0!=phys)
    {
        list_for_each(ptr, &kept_mem_head)
        {
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            if ((entry->dmem_info.phys<=phys) && phys<(entry->dmem_info.phys+entry->dmem_info.length))
            {
                memcpy(mem_info,&entry->dmem_info,sizeof(MSYS_DMEM_INFO));
                res=0;
                goto BEACH;
                ;
            }
        }
    }


BEACH:
    mutex_unlock(&dmem_mutex);
    return res;
}

int msys_find_dmem_by_name(const char *name, MSYS_DMEM_INFO *mem_info)
{
    struct list_head *ptr;
    struct DMEM_INFO_LIST *entry, *match_entry=NULL;
    int res=-EINVAL;

    mutex_lock(&dmem_mutex);

    if(name!=NULL && name[0]!=0)
    {
        list_for_each(ptr, &kept_mem_head)
        {
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            res=strncmp(entry->dmem_info.name, name, 16);
            if (0==res)
            {
                //MSYS_ERROR("%s: Find name\n", __func__);
                match_entry=entry;
                break;
            }
        }
    }
    else
    {
        MSYS_ERROR("%s: Invalid name\n", __func__);
    }

    if(match_entry!=NULL)
    {
        memcpy(mem_info, &match_entry->dmem_info, sizeof(MSYS_DMEM_INFO));
    }
    else
    {
        memset(mem_info->name,0,16);
    }

    mutex_unlock(&dmem_mutex);

    return res;
}


int msys_release_dmem(MSYS_DMEM_INFO *mem_info)
{

    //MSYS_DMEM_INFO mem_info;
    struct list_head *ptr;
    struct DMEM_INFO_LIST *entry,*match_entry;

    int dmem_fixed=0;

    mutex_lock(&dmem_mutex);
    match_entry=NULL;

//  MSYS_PRINT("\nFREEING DMEM [%s]\n\n",mem_info->name);
    if(mem_info->name[0]!=0)
    {
        list_for_each(ptr, &kept_mem_head)
        {
            int res=0;
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            res=strncmp(entry->dmem_info.name, mem_info->name,strnlen(mem_info->name,15));
//          MSYS_PRINT("DMEM0 [%s],%s %d\n",entry->dmem_info.name,match_entry->dmem_info.name,res);
            if (0==res)
            {
                match_entry=entry;
                break;
            }
        }
    }


    if(match_entry==NULL && (0!=mem_info->phys))
    {
        MSYS_ERROR("WARNING!! DMEM [%s]@0x%08X can not be found by name, try to find by phys address\n",mem_info->name, (unsigned int)mem_info->phys);
        list_for_each(ptr, &kept_mem_head)
        {
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            if (entry->dmem_info.phys==mem_info->phys)
            {
                match_entry=entry;
                break;
            }
        }

    }


    if(match_entry==NULL)
    {
        MSYS_ERROR("DMEM [%s]@0x%08X not found, skipping release...\n",mem_info->name, (unsigned int)mem_info->phys);
        goto BEACH;
    }

    if(fixed_dmem_enabled)
    {
        //check if entry is fixed
        list_for_each(ptr, &fixed_mem_head)
        {
            int res=0;
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            res=strcmp(entry->dmem_info.name, match_entry->dmem_info.name);
            if (0==res)
            {
                dmem_fixed=1;
                MSYS_PRINT("DMEM [%s]@0x%08X is fixed, skipping release...\n",match_entry->dmem_info.name,(unsigned int)match_entry->dmem_info.phys);
                goto BEACH;
            }
        }
    }


    dma_free_coherent(sys_dev.this_device, PAGE_ALIGN(match_entry->dmem_info.length),(void *)(uintptr_t)match_entry->dmem_info.kvirt,match_entry->dmem_info.phys);

    MSYS_PRINT("DMEM [%s]@0x%08X successfully released\n",match_entry->dmem_info.name,(unsigned int)match_entry->dmem_info.phys);

    list_del_init(&match_entry->list);
    kfree(match_entry);




BEACH:
    mutex_unlock(&dmem_mutex);
    return 0;

}

int msys_request_dmem(MSYS_DMEM_INFO *mem_info)
{
    dma_addr_t phys_addr;
    int err=0;
    int retry=0;

    if(mem_info->name[0]==0||strlen(mem_info->name)>15)
    {
        MSYS_ERROR( "Invalid DMEM name!! Either garbage or empty name!!\n");
        return -EINVAL;
    }

	/*if(mem_info->length<=0)
	{
		MSYS_ERROR( "Invalid DMEM length!! [%s]:0x%08X\n",mem_info->name,(unsigned int)mem_info->length);
		return -EFAULT;
	}*/

    MSYS_ERROR("DMEM request: [%s]:0x%08X\n",mem_info->name,(unsigned int)mem_info->length);

    mutex_lock(&dmem_mutex);
//  if(mem_info->name[0]!=0)
    {
        struct list_head *ptr;
        struct DMEM_INFO_LIST *entry;

        list_for_each(ptr, &kept_mem_head)
        {
            entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
            if (0==strncmp(entry->dmem_info.name, mem_info->name,strnlen(mem_info->name,15)))
            {
                if(dmem_realloc_enabled && (entry->dmem_info.length != mem_info->length))
                {
                    MSYS_ERROR("dmem realloc %s", entry->dmem_info.name);
                    dma_free_coherent(sys_dev.this_device, PAGE_ALIGN(entry->dmem_info.length),(void *)(uintptr_t)entry->dmem_info.kvirt,entry->dmem_info.phys);
                    MSYS_ERROR("DMEM [%s]@0x%08X successfully released\n",entry->dmem_info.name,(unsigned int)entry->dmem_info.phys);
                    list_del_init(&entry->list);
                    break;
                }
                else
                {
                    memcpy(mem_info,&entry->dmem_info,sizeof(MSYS_DMEM_INFO));
                    MSYS_ERROR("DMEM kept entry found: name=%s, phys=0x%08X, length=0x%08X\n",mem_info->name,(unsigned int)mem_info->phys,(unsigned int)mem_info->length);
                    goto BEACH_ENTRY_FOUND;
                }
            }
        }

        //MSYS_PRINT(KERN_WARNING"can not found kept direct requested memory entry name=%s\n",mem_info.name);

    }
//  else
//  {
//      MSYS_PRINT("    !!ERROR!! Anonymous DMEM request is forbidden !!\n");
//      return -EFAULT;
//  }

    while( !(mem_info->kvirt = (u64)(uintptr_t)dma_alloc_coherent(sys_dev.this_device, PAGE_ALIGN(mem_info->length), &phys_addr, GFP_KERNEL)) )
    {
        if(retry >= dmem_retry_count)
        {
            MSYS_ERROR( "unable to allocate direct memory\n");
            err = -ENOMEM;
            goto BEACH_ALLOCATE_FAILED;
        }
        MSYS_ERROR( "retry ALLOC_DMEM %d [%s]:0x%08X\n", retry, mem_info->name, (unsigned int)mem_info->length);
        sysctl_compaction_handler(NULL, 1, NULL, NULL, NULL);
        msleep(1000);
        retry++;
    }

    mem_info->phys=(u64)phys_addr;


    {
        struct DMEM_INFO_LIST *new=(struct DMEM_INFO_LIST *)kmalloc(sizeof(struct DMEM_INFO_LIST),GFP_KERNEL);
        if(new==NULL)
        {
            MSYS_ERROR("allocate memory for mem_list entry error\n" ) ;
            err = -ENOMEM;
            goto BEACH;

        }

        memset(new->dmem_info.name,0,16);
/*
        new->dmem_info.kvirt=mem_info->kvirt;
        new->dmem_info.phys=mem_info->phys;
        new->dmem_info.length=mem_info->length;
        if(mem_info->name!=NULL){
            memcpy(new->dmem_info.name,mem_info->name,strnlen(mem_info->name,15));
        }
*/
        memcpy(&new->dmem_info,mem_info,sizeof(MSYS_DMEM_INFO));

        list_add(&new->list, &kept_mem_head);

    }

    if(retry)
        MSYS_ERROR("DMEM request: [%s]:0x%08X success, @0x%08X (retry=%d)\n",mem_info->name,(unsigned int)mem_info->length, (unsigned int)mem_info->phys, retry);
    else
        MSYS_ERROR("DMEM request: [%s]:0x%08X success, CPU phy:@0x%08X, virt:@0x%08X\n",mem_info->name,(unsigned int)mem_info->length, (unsigned int)mem_info->phys, (unsigned int)mem_info->kvirt);


BEACH:
    if(err==-ENOMEM)
    {
        msys_release_dmem(mem_info);
    }

BEACH_ALLOCATE_FAILED:
BEACH_ENTRY_FOUND:
    if(err)
    {
        MSYS_ERROR("DMEM request: [%s]:0x%08X FAILED!! (retry=%d)\n",mem_info->name,(unsigned int)mem_info->length, retry);
    }

#if 0
    if(0==err){
        memset((void *)((unsigned int)mem_info->kvirt),0,mem_info->length);
        Chip_Flush_CacheAll();
        MSYS_PRINT("DMEM CLEAR!!\n");
    }

#endif

    mutex_unlock(&dmem_mutex);
    return err;

}

unsigned int get_PIU_tick_count(void)
{
    return ( INREG16(0x1F006050) | (INREG16(0x1F006054)<<16) );
}

EXPORT_SYMBOL(get_PIU_tick_count);

int msys_user_to_physical(unsigned long addr,unsigned long *phys)
{

	unsigned long paddr=0;
	struct page *page;
	down_read(&current->mm->mmap_sem);
	//if (get_user_pages(current, current->mm, addr, 1, 1, 0, &page, NULL) <= 0)//3.18
	if (get_user_pages(addr, 1, FOLL_WRITE, &page, NULL) <= 0)
	{
		up_read(&current->mm->mmap_sem);
		printk(KERN_WARNING"ERR!!\n");
		return -EINVAL;
	}
	up_read(&current->mm->mmap_sem);

	paddr= page_to_phys(page);

	*phys=paddr;
//	if(paddr>0x21E00000)
//	{
//		printk(KERN_WARNING"\nKXX:0x%08X,0x%08X\n",(unsigned int)addr,(unsigned int)paddr);
//	}

	return 0;
}

int msys_find_dmem_by_name_verchk(unsigned long arg)
{
    MSYS_DMEM_INFO mem_info;
    int err=0;
	if(copy_from_user((void*)&mem_info, (void __user *)arg, sizeof(MSYS_DMEM_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(mem_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(mem_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                mem_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(mem_info.VerChk_Size), sizeof(MSYS_DMEM_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_DMEM_INFO), (mem_info.VerChk_Size));
                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	if( (err=msys_find_dmem_by_name(mem_info.name, &mem_info)) )
	{
		//return -ENOENT;
	}

	if(copy_to_user((void __user *)arg, (void*)&mem_info, sizeof(MSYS_DMEM_INFO)))
	{
		return -EFAULT;
	}

    return 0;
}

int msys_request_dmem_verchk(unsigned long arg)
{
    MSYS_DMEM_INFO mem_info;
    int err=0;
	if(copy_from_user((void*)&mem_info, (void __user *)arg, sizeof(MSYS_DMEM_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(mem_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(mem_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                mem_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(mem_info.VerChk_Size), sizeof(MSYS_DMEM_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_DMEM_INFO), (mem_info.VerChk_Size));
                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	if( (err=msys_request_dmem(&mem_info)) )
	{
		MSYS_ERROR("request direct memory failed!!\n" );
		return err;
	}

	if(copy_to_user((void __user *)arg, (void*)&mem_info, sizeof(MSYS_DMEM_INFO)))
	{
		return -EFAULT;
	}

    return 0;
}

int msys_release_dmem_verchk(unsigned long arg)
{
    MSYS_DMEM_INFO mem_info;
	if(copy_from_user((void*)&mem_info, (void __user *)arg, sizeof(MSYS_DMEM_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(mem_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(mem_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                mem_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(mem_info.VerChk_Size), sizeof(MSYS_DMEM_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_DMEM_INFO), (mem_info.VerChk_Size));
                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	return msys_release_dmem(&mem_info);

}


int msys_flush_cache(unsigned long arg)
{
	MSYS_DUMMY_INFO info;
	if(copy_from_user((void*)&info, (void __user *)arg, sizeof(MSYS_DUMMY_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(info.VerChk_Size), sizeof(MSYS_DUMMY_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_DUMMY_INFO), (info.VerChk_Size));

                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	Chip_Flush_CacheAll();

    return 0;
}

int msys_addr_translation_verchk(unsigned long arg, bool direction)
{
    MSYS_ADDR_TRANSLATION_INFO addr_info;

	if(copy_from_user((void*)&addr_info,  (void __user *)arg, sizeof(addr_info)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(addr_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(addr_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                addr_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(addr_info.VerChk_Size), sizeof(MSYS_ADDR_TRANSLATION_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_ADDR_TRANSLATION_INFO), (addr_info.VerChk_Size));

                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	if(direction)
		addr_info.addr=Chip_MIU_to_Phys(addr_info.addr);
	else
		addr_info.addr=Chip_Phys_to_MIU(addr_info.addr);

	if(copy_to_user((void __user *)arg, (void*)&addr_info, sizeof(addr_info)))
	{
		return -EFAULT;
	}

    return 0;
}

int msys_get_riu_map_verchk(unsigned long arg)
{
    MSYS_MMIO_INFO mmio_info;
	if( copy_from_user((void*)&mmio_info, (void __user *)arg, sizeof(MSYS_MMIO_INFO)) )
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(mmio_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&mmio_info, IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                mmio_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(mmio_info.VerChk_Size), sizeof(MSYS_MMIO_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_MMIO_INFO), (mmio_info.VerChk_Size));

                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	mmio_info.addr=Chip_Get_RIU_Phys();
	mmio_info.size=Chip_Get_RIU_Size();
	if( copy_to_user((void __user *)arg, (void*)&mmio_info, sizeof(MSYS_MMIO_INFO)) )
	{
		return -EFAULT;
	}

    return 0;
}


int msys_fix_dmem_verchk(unsigned long arg)
{
    MSYS_DMEM_INFO mem_info;
    int err=0;
	if(copy_from_user((void*)&mem_info, (void __user *)arg, sizeof(MSYS_DMEM_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(mem_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(mem_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                mem_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(mem_info.VerChk_Size), sizeof(MSYS_DMEM_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_DMEM_INFO), (mem_info.VerChk_Size));
                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }
	if( (err=msys_fix_dmem(mem_info.name)) )
	{
		MSYS_ERROR("fix direct memory failed!! %s\n", mem_info.name);
		return err;
	}

    return 0;
}

int msys_unfix_dmem_verchk(unsigned long arg)
{
    MSYS_DMEM_INFO mem_info;
    int err=0;
	if(copy_from_user((void*)&mem_info, (void __user *)arg, sizeof(MSYS_DMEM_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(mem_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(mem_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                mem_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(mem_info.VerChk_Size), sizeof(MSYS_DMEM_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_DMEM_INFO), (mem_info.VerChk_Size));
                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	if( (err=msys_unfix_dmem(mem_info.name)) )
	{
		MSYS_ERROR("unfix direct memory failed!! %s\n", mem_info.name);
		return err;
	}

    return 0;
}

int msys_miu_protect_verchk(unsigned long arg)
{
    MSYS_MIU_PROTECT_INFO protect_info;
    u64 miu_addr_start;
    u64 miu_addr_end;
    u32 start_unit, end_unit;
    u8 i=0;

	if(copy_from_user((void*)&protect_info, (void __user *)arg, sizeof(MSYS_MIU_PROTECT_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(protect_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(protect_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                protect_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(protect_info.VerChk_Size), sizeof(MSYS_MIU_PROTECT_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_MIU_PROTECT_INFO), (((MSYS_MIU_PROTECT_INFO __user *)arg)->VerChk_Size));
                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

    miu_addr_start = Chip_Phys_to_MIU(protect_info.phys);
    miu_addr_end = Chip_Phys_to_MIU(protect_info.phys + protect_info.length) - 1;

    if(miu_addr_start & (0x2000-1)) /*check 8KB align*/
    {
        MSYS_WARN("MIU protect start=0x%08X is not 8KB aligned!\n", (u32)miu_addr_start);
    }

    start_unit = (u32)((miu_addr_start & ~(0x2000-1)) >> 13); // 8KB unit

    OUTREG16(BASE_REG_MIU_PA + REG_ID_60, (u16)(start_unit & 0xFFFF));
    OUTREG16(BASE_REG_MIU_PA + REG_ID_68, (INREG16(BASE_REG_MIU_PA + REG_ID_68)) | ((start_unit>>16) & 0x3));

    if( (miu_addr_end & (0x2000-1)) != (0x2000-1) ) /*check 8KB align*/
    {
        MSYS_WARN("MIU protect end=0x%08X is not 8KB aligned!\n", (u32)miu_addr_end);
    }

    end_unit = (u32)((miu_addr_end & ~(0x2000-1)) >> 13); // 8KB unit

    OUTREG16(BASE_REG_MIU_PA + REG_ID_61, (u16)(end_unit & 0xFFFF));
    OUTREG16(BASE_REG_MIU_PA + REG_ID_68, (INREG16(BASE_REG_MIU_PA + REG_ID_68)) | (((end_unit>>16) & 0x3) << 2));

    printk("\n\tMIU protect start=0x%08X\n", start_unit << 13);
    printk("\tMIU protect end=0x%08X\n", ((end_unit+1) << 13) -1);
    printk("\tMIU protect id=");

    do
    {
        OUTREG16(BASE_REG_MIU_PA + REG_ID_17 + (i*2), (protect_info.id[i] & 0x7F) | (protect_info.id[i+1]&0x7F)<<8 );
        printk(" 0x%02X 0x%02X", protect_info.id[i], protect_info.id[i+1]);
        i+=2;
    } while(protect_info.id[i]!=0x00 && i<16);

    printk("\n");

    OUTREG16(BASE_REG_MIU_PA + REG_ID_10, 0xFFFF);  // for test, we set all id enable

    if(protect_info.w_protect)
        SETREG16(BASE_REG_MIU_PA + REG_ID_69, BIT0);
    if(protect_info.r_protect)
        SETREG16(BASE_REG_MIU_PA + REG_ID_69, BIT4);
    if(protect_info.inv_protect)
        SETREG16(BASE_REG_MIU_PA + REG_ID_69, BIT8);

    printk("\tMIU protect W_protect=%d\n", protect_info.w_protect);
    printk("\tMIU protect R_protect=%d\n", protect_info.r_protect);
    printk("\tMIU protect INV_protect=%d\n", protect_info.inv_protect);

    return 0;
}

#if 0 // not test yet
int msys_user_to_physical_verchk(unsigned long arg)
{
    MSYS_ADDR_TRANSLATION_INFO addr_info;
    int res=0;
    struct page *page;
    int addr;

    if ( CHK_VERCHK_HEADER(&(((MSYS_ADDR_TRANSLATION_INFO __user *)arg)->VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(((MSYS_ADDR_TRANSLATION_INFO __user *)arg)->VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                ((MSYS_ADDR_TRANSLATION_INFO __user *)arg)->VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(((MSYS_ADDR_TRANSLATION_INFO __user *)arg)->VerChk_Size), sizeof(MSYS_ADDR_TRANSLATION_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_ADDR_TRANSLATION_INFO), (((MSYS_ADDR_TRANSLATION_INFO __user *)arg)->VerChk_Size));

                return -EINVAL;
            }
            else
            {
                if(copy_from_user((void*)&addr,  (void __user *)arg, sizeof(addr)))
                {
                    return -EFAULT;
                }
                down_read(&current->mm->mmap_sem);
                res = get_user_pages(current, current->mm, addr, 1, 1, 0, &page, NULL);
                if (res <= 0)
                    return -EINVAL;
                up_read(&current->mm->mmap_sem);
                printk("vaddr=0x%08X\n", addr);
                addr = page_to_phys(page);
                printk("paddr=0x%08X\n\n", addr);
                if(copy_to_user((void __user *)arg, (void*)&addr, sizeof(addr)))
                {
                    return -EFAULT;
                }
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }
    return 0;
}
#endif



int msys_string_verchk(unsigned long arg, unsigned int op)
{
/*
	MSYS_STRING_INFO info;

	if(copy_from_user((void*)&info, (void __user *)arg, sizeof(MSYS_STRING_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(info.VerChk_Size), sizeof(MSYS_STRING_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_STRING_INFO), (info.VerChk_Size));

                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EINVAL;
    }

	if(op==0)
	{
		if(copy_to_user(&(((MSYS_STRING_INFO __user *)arg)->str[0]), (void*)system_part_string, sizeof(system_part_string)))
		{
			return -EFAULT;
		}
	}
	else if(op==1)
	{
		if(copy_to_user(&(((MSYS_STRING_INFO __user *)arg)->str[0]), (void*)data_part_string, sizeof(data_part_string)))
		{
			return -EFAULT;
		}
	}
	else if(op==2)
	{
		if(copy_to_user(&(((MSYS_STRING_INFO __user *)arg)->str[0]), (void*)mstar_property_path, sizeof(mstar_property_path)))
		{
			return -EFAULT;
		}
	}
	else if(op==3)
	{
		if(copy_from_user((void*)mstar_property_path,  &(((MSYS_STRING_INFO __user *)arg)->str[0]), sizeof(mstar_property_path)))
		{
			return -EFAULT;
		}
		printk("set mstar_property_path=%s\n", mstar_property_path);
	}
	else
	{
		MSYS_ERROR("[%s] unsupport op=%d!!\n", __FUNCTION__, op);
		return -EINVAL;
	}
*/
    return 0;
}


extern int g_sCurrentTemp;

int msys_get_temp_verchk(unsigned long arg)
{
	int temp;
	MSYS_STRING_INFO info;

	if(copy_from_user((void*)&info, (void __user *)arg, sizeof(MSYS_STRING_INFO)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(info.VerChk_Size), sizeof(MSYS_TEMP_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_TEMP_INFO), (info.VerChk_Size));

                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EINVAL;
    }
	temp = g_sCurrentTemp;
	if(copy_to_user( &(((MSYS_TEMP_INFO __user *)arg)->temp), &temp, sizeof(temp) ))
		return -EFAULT;;

    return 0;
}

int msys_read_uuid(unsigned long long* udid)
{
    CLRREG16(BASE_REG_EFUSE_PA + REG_ID_03, BIT8);  //reg_sel_read_256[8]=0 to read a/b/c/d
    *udid = (u64)INREG16(BASE_REG_EFUSE_PA + REG_ID_16) |
            ((u64)(INREG16(BASE_REG_EFUSE_PA + REG_ID_17)) << 16) |
            ((u64)INREG16(BASE_REG_EFUSE_PA + REG_ID_18) << 32);
    return 0;
}
EXPORT_SYMBOL(msys_read_uuid);

CHIP_VERSION msys_get_chipVersion(void)
{
    CHIP_VERSION eRet = U01;
    eRet = (INREG16(BASE_REG_PMTOP_PA + REG_ID_01)>>8);
    return eRet;
}
EXPORT_SYMBOL(msys_get_chipVersion);

int msys_get_chipVersion_verchk(unsigned long arg)
{
    MSYS_CHIPVER_INFO chipVer_info;
    if(copy_from_user((void*)&chipVer_info,  (void __user *)arg, sizeof(chipVer_info)))
    {
        return -EFAULT;
    }

    if ( CHK_VERCHK_HEADER(&(chipVer_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(chipVer_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                chipVer_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(chipVer_info.VerChk_Size), sizeof(MSYS_CHIPVER_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_CHIPVER_INFO), chipVer_info.VerChk_Size);

                return -EINVAL;
            }
            else
            {
                chipVer_info.chipVersion = msys_get_chipVersion();
                if(copy_to_user((void __user *)arg, (void*)&chipVer_info, sizeof(chipVer_info)))
                {
                    return -EFAULT;
                }
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

    return 0;
}

int msys_get_udid_verchk(unsigned long arg)
{
    MSYS_UDID_INFO udid_info;
	if(copy_from_user((void*)&udid_info,  (void __user *)arg, sizeof(udid_info)))
	{
		return -EFAULT;
	}

    if ( CHK_VERCHK_HEADER(&(udid_info.VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(udid_info.VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                udid_info.VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(udid_info.VerChk_Size), sizeof(MSYS_UDID_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_UDID_INFO), (udid_info.VerChk_Size));

                return -EINVAL;
            }
        }
    }
    else
    {
        VERCHK_ERR("\n\33[1;31m[%s] No verchk header !!!\33[0m\n", __FUNCTION__);
        return -EFAULT;
    }

	msys_read_uuid(&(udid_info.udid));

	if(copy_to_user((void __user *)arg, (void*)&udid_info, sizeof(udid_info)))
	{
		return -EFAULT;
	}

    return 0;
}

static int msys_check_freq_cfg(unsigned long arg)
{
    MSYS_FREQGEN_INFO freq_info;

    if(copy_from_user((void*)&freq_info, (void __user *)arg, sizeof(MSYS_FREQGEN_INFO)))
    {
        return -EFAULT;
    }

    return msys_request_freq(&freq_info);
}

#if 0
#define CHK_NUM_WAITDONE     20000

static int msys_dma_by_BDMA(unsigned long arg)
{
	MSYS_DMA_INFO mem_info;
    U16 u16data;
    U32 u32Timer = 0;

	if ( CHK_VERCHK_HEADER(&(((MSYS_ADDR_TRANSLATION_INFO __user *)arg)->VerChk_Version)) )
	{
		if( CHK_VERCHK_VERSION_LESS(&(((MSYS_DMA_INFO __user *)arg)->VerChk_Version), IOCTL_MSYS_VERSION) )
		{
			VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
				   ((MSYS_DMA_INFO __user *)arg)->VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
			return -EINVAL;
		}
		else
		{
			if( CHK_VERCHK_SIZE(&(((MSYS_DMA_INFO __user *)arg)->VerChk_Size), sizeof(MSYS_DMA_INFO)) == 0 )
			{
				VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
				sizeof(MSYS_DMA_INFO), (((MSYS_DMA_INFO __user *)arg)->VerChk_Size));
				return -EINVAL;
			}

			if(copy_from_user((void*)&mem_info, (void __user *)arg, sizeof(MSYS_DMEM_INFO)))
		   {
			   return -EFAULT;
		   }
			//Set source and destination path
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_00, 0x0000);
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_02, 0x4040);
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_04, (Chip_Phys_to_MIU(mem_info.kphy_src) & 0x0000FFFF));
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_05, (Chip_Phys_to_MIU(mem_info.kphy_src)>>16));
			// Set end address
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_06, (Chip_Phys_to_MIU(mem_info.kphy_des) & 0x0000FFFF));
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_07, (Chip_Phys_to_MIU(mem_info.kphy_des) >> 16));
			//Set Size
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_08, (mem_info.length & 0x0000FFFF));
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_09, (mem_info.length >> 16));
			OUTREG16(BASE_REG_BDMA1_PA + REG_ID_00, 0x1);

			do
			{
				//check done
				u16data = INREG16(BASE_REG_BDMA1_PA + REG_ID_01);
				if(u16data & 0x8)
				{
					//clear done
					OUTREG16(BASE_REG_BDMA1_PA + REG_ID_01, 0x8);
					break;
				}

				if (++u32Timer%1000 == 0)
					cond_resched();
			}while(u32Timer < CHK_NUM_WAITDONE);
			Chip_Flush_Memory();
		}
	}
	return 0;
}


#include "halAESDMA.c"

static int msys_dma_by_ADMA(unsigned long arg)
{
	MSYS_DMA_INFO mem_info;

    if ( CHK_VERCHK_HEADER(&(((MSYS_ADDR_TRANSLATION_INFO __user *)arg)->VerChk_Version)) )
    {
        if( CHK_VERCHK_VERSION_LESS(&(((MSYS_DMA_INFO __user *)arg)->VerChk_Version), IOCTL_MSYS_VERSION) )
        {
            VERCHK_ERR("\n\33[1;31m[%s] verchk version (%04x) < ioctl verision (%04x) !!!\33[0m\n", __FUNCTION__,
                ((MSYS_DMA_INFO __user *)arg)->VerChk_Version & VERCHK_VERSION_MASK, IOCTL_MSYS_VERSION);
            return -EINVAL;
        }
        else
        {
            if( CHK_VERCHK_SIZE(&(((MSYS_DMA_INFO __user *)arg)->VerChk_Size), sizeof(MSYS_DMA_INFO)) == 0 )
            {
                VERCHK_ERR("\n\33[1;31m[%s] struct size(%04x) != verchk size(%04x) !!!\33[0m\n", __FUNCTION__,
                    sizeof(MSYS_DMA_INFO), (((MSYS_DMA_INFO __user *)arg)->VerChk_Size));
                return -EINVAL;
            }


		if(copy_from_user((void*)&mem_info, (void __user *)arg, sizeof(MSYS_DMA_INFO)))
		{
			return -EFAULT;
		}

		OUTREG16(BASE_REG_CLKGEN_PA + REG_ID_61, 0x14);
		HAL_AESDMA_Enable(0);
		HAL_AESDMA_Reset();
		HAL_AESDMA_SetFileinAddr(Chip_Phys_to_MIU(mem_info.kphy_src));
		HAL_AESDMA_SetXIULength(mem_info.length);
		HAL_AESDMA_SetFileoutAddr(Chip_Phys_to_MIU(mem_info.kphy_des),(mem_info.length));
		HAL_AESDMA_FileOutEnable(1);
		HAL_AESDMA_Start(1);

		while((HAL_AESDMA_GetStatus() & AESDMA_CTRL_DMA_DONE) != AESDMA_CTRL_DMA_DONE)
		{
		}

		Chip_Flush_MIU_Pipe();
		HAL_AESDMA_Reset();
		}
	}
	return 0;
}

#endif

static long msys_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
    int err= 0;

    // wrong cmds: return ENOTTY (inappropriate ioctl) before access_ok()
    if (_IOC_TYPE(cmd) != MSYS_IOCTL_MAGIC) return -ENOTTY;
    if (_IOC_NR(cmd) > IOCTL_SYS_MAXNR) return -ENOTTY;

    if (_IOC_DIR(cmd) & _IOC_READ)
    {
        err = !access_ok(VERIFY_WRITE, (void __user *)arg, _IOC_SIZE(cmd));
    }
    else if (_IOC_DIR(cmd) & _IOC_WRITE)
    {
        err =  !access_ok(VERIFY_READ, (void __user *)arg, _IOC_SIZE(cmd));
    }
    if (err)
    {
        return -EFAULT;
    }


    switch(cmd)
    {
        case IOCTL_MSYS_REQUEST_DMEM:
        {
            if((err=msys_request_dmem_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_REQUEST_DMEM error!\n");
        }
        break;

        case IOCTL_MSYS_RELEASE_DMEM:
        {
            if((err=msys_release_dmem_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_RELEASE_DMEM error!\n");
        }
        break;

        case IOCTL_MSYS_FLUSH_CACHE:
        {
            if((err = msys_flush_cache(arg)))
                MSYS_ERROR("IOCTL_MSYS_FLUSH_CACHE error!\n");
        }
        break;

        case IOCTL_MSYS_PHYS_TO_MIU:
        {
            if((err=msys_addr_translation_verchk(arg, 0)))
                MSYS_ERROR("IOCTL_MSYS_PHYS_TO_MIU error!\n");
        }
        break;

        case IOCTL_MSYS_MIU_TO_PHYS:
        {
            if((err=msys_addr_translation_verchk(arg, 1)))
                MSYS_ERROR("IOCTL_MSYS_MIU_TO_PHYS error!\n");
        }
        break;

        case IOCTL_MSYS_GET_RIU_MAP:
        {
            if((err=msys_get_riu_map_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_GET_RIU_MAP error!\n");
        }
        break;

        case IOCTL_MSYS_FIX_DMEM:
        {
            if((err=msys_fix_dmem_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_FIX_DMEM error!\n");
        }
        break;

        case IOCTL_MSYS_UNFIX_DMEM:
        {
            if((err=msys_unfix_dmem_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_UNFIX_DMEM error!\n");
        }
        break;

        case IOCTL_MSYS_FIND_DMEM_BY_NAME:
        {
//            if((err=msys_find_dmem_by_name_verchk(arg)))
//                MSYS_ERROR("IOCTL_MSYS_FIND_DMEM_BY_NAME error!\n");
            msys_find_dmem_by_name_verchk(arg);
            err=0;
        }
        break;

        case IOCTL_MSYS_MIU_PROTECT:
        {
            if((err=msys_miu_protect_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_MIU_PROTECT error!\n");
        }
        break;

        case IOCTL_MSYS_USER_TO_PHYSICAL:
        {
            unsigned long addr,paddr;


            if(copy_from_user((void*)&addr,  (void __user *)arg, sizeof(addr)))
            {
                return -EFAULT;
            }

            if((err=msys_user_to_physical(addr,&paddr)))
                MSYS_ERROR("IOCTL_MSYS_GET_USER_PAGE error!\n");

            if(copy_to_user((void __user *)arg, (void*)&paddr, sizeof(paddr)))
            {
                return -EFAULT;
            }
        }
        break;

        case IOCTL_MSYS_GET_SYSP_STRING:
        {
            if((err=msys_string_verchk(arg, 0)))
                MSYS_ERROR("IOCTL_MSYS_GET_SYSP_STRING error!\n");
        }
        break;

        case IOCTL_MSYS_GET_DATAP_STRING:
        {
            if((err=msys_string_verchk(arg, 1)))
                MSYS_ERROR("IOCTL_MSYS_GET_DATAP_STRING error!\n");
        }
        break;

        case IOCTL_MSYS_GET_PROPERTY_PATH:
        {
            if((err=msys_string_verchk(arg, 2)))
                MSYS_ERROR("IOCTL_MSYS_GET_PROPERTY_PATH error!\n");
        }
        break;

        case IOCTL_MSYS_SET_PROPERTY_PATH:
        {
            if((err=msys_string_verchk(arg, 3)))
                MSYS_ERROR("IOCTL_MSYS_SET_PROPERTY_PATH error!\n");
        }
        break;

        case IOCTL_MSYS_GET_US_TICKS:
        {

//	            u64 us_ticks=Chip_Get_US_Ticks();
//
//	            if(copy_to_user((void __user *)arg, (void*)&us_ticks, sizeof(us_ticks)))
//	            {
//	                return -EFAULT;
//	            }
            return -EPERM;
        }
        break;

        case IOCTL_MSYS_GET_UDID:
        {
            if((err=msys_get_udid_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_GET_UDID error!\n");
        }
        break;

        case IOCTL_MSYS_PRINT_PIU_TIMER_TICKS:
        {
            int id=arg;
            printk(KERN_WARNING"PIU_T:%X#%d#\n",get_PIU_tick_count(),id);
        }
        break;

        case IOCTL_MSYS_BENCH_MEMORY:
        {
#if BENCH_MEMORY_FUNC==1
            int test_mem_size_in_MB=arg;
            msys_bench_memory((unsigned int)test_mem_size_in_MB);
#endif
        }
        break;

        case IOCTL_MSYS_RESET_TO_UBOOT:
        {
            do
            {
                SETREG16(REG_ADDR_STATUS, FORCE_UBOOT_BIT);
            } while(!(INREG16(REG_ADDR_STATUS) & FORCE_UBOOT_BIT));
            OUTREG16(BASE_REG_PMSLEEP_PA + REG_ID_2E, 0x79);
        }
        break;

        case IOCTL_MSYS_READ_PM_TSENSOR:
        {
#if 0
            if((err=msys_get_temp_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_READ_PM_TSENSOR error!\n");
#else
            int temp = g_sCurrentTemp;
            if(copy_to_user( (void __user *)arg, &temp, sizeof(temp) ))
                return -EFAULT;
#endif
        }
        break;

        case IOCTL_MSYS_REQUEST_PROC_DEVICE:
        {
            MSYS_PROC_DEVICE proc_dev;
            if(copy_from_user((void*)&proc_dev, (void __user *)arg, sizeof(MSYS_PROC_DEVICE)))
                BUG();

            if((err = msys_request_proc_dev(&proc_dev)) == -EEXIST) {
                MSYS_PRINT("skip since device %s exist\n" , proc_attr->name);
            } else if(err != 0) {
                MSYS_ERROR("msys_request_proc_dev failed!!\n" );
                break;
            }

            if(copy_to_user((void*)arg, (void*)&proc_dev, sizeof(MSYS_PROC_DEVICE)))
                BUG();
        }
        break;

        case IOCTL_MSYS_RELEASE_PROC_DEVICE:
        {
            MSYS_PROC_DEVICE proc_dev;
            if(copy_from_user((void*)&proc_dev, (void __user *)arg, sizeof(MSYS_PROC_DEVICE)))
                BUG();

            if((err = msys_release_proc_dev(&proc_dev)) != 0) {
                MSYS_ERROR("msys_release_proc_dev failed!!\n" );
                break;
            }

            if(copy_to_user((void*)arg, (void*)&proc_dev, sizeof(MSYS_PROC_DEVICE)))
                BUG();
        }
        break;

        case IOCTL_MSYS_REQUEST_PROC_ATTRIBUTE:
        {
            MSYS_PROC_ATTRIBUTE proc_attr;

            if(copy_from_user((void*)&proc_attr, (void __user *)arg, sizeof(MSYS_PROC_ATTRIBUTE)))
                BUG();
            if((err = msys_request_proc_attr(&proc_attr)) == -EEXIST) {
                MSYS_PRINT("skip since attribute %s exist\n" , proc_attr.name);
            } else if(err != 0) {
                MSYS_ERROR("msys_request_proc_attribute failed!!\n" );
                break;
            }

            if(copy_to_user((void*)arg, (void*)&proc_attr, sizeof(MSYS_PROC_ATTRIBUTE)))
                BUG();
        }
        break;

        case IOCTL_MSYS_RELEASE_PROC_ATTRIBUTE:
        {
            MSYS_PROC_ATTRIBUTE proc_attr;

            if(copy_from_user((void*)&proc_attr, (void __user *)arg, sizeof(MSYS_PROC_ATTRIBUTE)))
                BUG();

            if((err = msys_release_proc_attr(&proc_attr)) != 0) {
                MSYS_ERROR("msys_release_proc_attr failed!!\n" );
                break;
            }

            if(copy_to_user((void*)arg, (void*)&proc_attr, sizeof(MSYS_PROC_ATTRIBUTE)))
                BUG();
        }
        break;

		case IOCTL_MSYS_FLUSH_MEMORY:
        {
			 __cpuc_flush_kern_all();//L1
			 Chip_Flush_Memory();//L3
        }

        case IOCTL_MSYS_REQUEST_FREQUENCY:
        {
            if((err = msys_check_freq_cfg(arg)))
                MSYS_ERROR("IOCTL_MSYS_REQUEST_FREQUENCY error!\n");
        }
        break;

        case IOCTL_MSYS_GET_CHIPVERSION:
        {
            if((err = msys_get_chipVersion_verchk(arg)))
                MSYS_ERROR("IOCTL_MSYS_GET_CHIPVERSION error!\n");
        }
        break;
#if 0
		case IOCTL_MSYS_BDMA:
        {
			if((err = msys_dma_by_BDMA(arg)))
                MSYS_ERROR("IOCTL_MSYS_BDMA error!\n");
        }
        break;

		case IOCTL_MSYS_ADMA:
        {
			if((err = msys_dma_by_ADMA(arg)))
                MSYS_ERROR("IOCTL_MSYS_ADMA error!\n");
        }
        break;
#endif

        default:
            MSYS_ERROR("Unknown IOCTL Command 0x%08X\n", cmd);
            return -ENOTTY;
    }


    return err;
}



#if BENCH_MEMORY_FUNC==1
typedef unsigned int volatile ulv;
typedef unsigned long int volatile ullv;

/******************************************
 * Function prototypes and Global variables
 ******************************************/
//int TEST_SolidBitsComparison(ulv *pSrc, ulv *pDest, unsigned int nCount);


/******************************************
 * Extern
 ******************************************/

/******************************************
 * Functions
 ******************************************/

static int TEST_Memwrite(ulv * pDest, unsigned int nCount)
{
    register unsigned int val = 0xA5A4B5B4;
    ulv *p2 = NULL;
    unsigned int    nTest, i;
    for (nTest = 0; nTest < 10; nTest++)
    {
        p2 = (ulv *) pDest;
        for (i = 0; i < nCount; i++)
            *p2++ = val;
    }
    return nTest;
}

static int TEST_Memread(ulv * pSrc, unsigned int nCount)
{
    register unsigned int val;
    ulv *p1 = NULL;
    unsigned int    nTest, i;
    for (nTest = 0; nTest < 10; nTest++)
    {
        p1 = (ulv *) pSrc;
        for (i = 0; i < nCount; i++)
            val = *p1++;
    }
    return nTest;
}

static int TEST_Memcpy_mips(ulv * pSrc, ulv * pDest, unsigned int nCount)
{
    int nTest = 0;
//  for (nTest = 0; nTest < 10; nTest++)
//      memcpy_MIPS((void*)pDest, (void*)pSrc, nCount*sizeof(unsigned int));
    return nTest;
}

static int TEST_Memcpy(ulv * pSrc, ulv * pDest, unsigned int nCount)
{
    int nTest;
    for (nTest = 0; nTest < 10; nTest++)
        memcpy((void*)pDest, (void*)pSrc, nCount*sizeof(unsigned int));
    return nTest;
}

static int TEST_MemBandWidth_long(ulv * pSrc, ulv * pDest, unsigned int nCount)
{
    ullv *p1 = NULL;
    ullv *p2 = NULL;
    unsigned int    i;
    unsigned int    nTest;

    for (nTest = 0; nTest < 10; nTest++)
    {
        p1 = (ullv *) pSrc;
        p2 = (ullv *) pDest;

        for (i = 0; i < nCount; i++)
            *p2++ = *p1++;
    }

    return nTest;
}

int TEST_MemBandWidth(ulv * pSrc, ulv * pDest, unsigned int nCount)
{
    ulv *p1 = NULL;
    ulv *p2 = NULL;
    unsigned int    i;
    unsigned int    nTest;

    for (nTest = 0; nTest < 10; nTest++)
    {
        p1 = (ulv *) pSrc;
        p2 = (ulv *) pDest;

        for (i = 0; i < nCount; i++)
            *p2++ = *p1++;
    }

    return nTest;
}


int TEST_MemBandWidthRW(ulv * pSrc, ulv * pDest, unsigned int nCount, unsigned int step_size)
{
    ulv *p1 = NULL;
    ulv *p2 = NULL;
    //unsigned int  i;
    unsigned int    nTest;
    int Count;

    for (nTest = 0; nTest < 10 * step_size; nTest++)
    {
        p1 = (ulv *) pSrc;
        p2 = (ulv *) pDest;
        Count = nCount / step_size;
        //memcpy((void*)p2, (void*)p1, nCount*4);
        while(Count--)
        {
            *p2 = *p1;
            p2 += step_size;
            p1 += step_size;
        }
    }

    return nTest;
}

static int TEST_MemBandWidthR(ulv * pSrc, ulv * pDest, unsigned int nCount, unsigned int step_size)
{
    ulv *p1 = NULL;
    ulv *p2 = NULL;
    //unsigned int  i;
    unsigned int    nTest;
    int Count;

    for (nTest = 0; nTest < 10 * step_size; nTest++)
    {
        p1 = (ulv *) pSrc;
        p2 = (ulv *) pDest;
        Count = nCount / step_size;
        //memcpy((void*)p2, (void*)p1, nCount*4);
        while(Count--)
        {
            *p2 = *p1;
            p1 += step_size;
        }
    }

    return nTest;
}

static int TEST_MemBandWidthW(ulv * pSrc, ulv * pDest, unsigned int nCount, unsigned int step_size)
{
    ulv *p1 = NULL;
    ulv *p2 = NULL;
    //unsigned int  i;
    unsigned int    nTest;
    int Count;

    for (nTest = 0; nTest < 10 * step_size; nTest++)
    {
        p1 = (ulv *) pSrc;
        p2 = (ulv *) pDest;
        Count = nCount / step_size;
        //memcpy((void*)p2, (void*)p1, nCount*4);
        while(Count--)
        {
            *p2 = *p1;
            p2 += step_size;
        }
    }

    return nTest;
}



static void msys_bench_memory(unsigned int uMemSize)
{
    unsigned int    nLoop = 0;
    unsigned int    nAllocBytes;
    unsigned int    nBufSize;
    unsigned int    nCount;
    unsigned int    PAGE_MASK1 = 0x0FFF;
    void *pBuf = NULL;
    volatile void *pAlignedBuf = NULL;
    volatile unsigned int *pSrc;
    volatile unsigned int *pDest;
    unsigned int bus_addr;
    struct timespec tss, tse;
    int             nDelay;
    int             nTestCount = 0;
    int             nSize;
    int i = 0;

    nBufSize    = (unsigned int) (uMemSize << 20);
    nAllocBytes = nBufSize + 4096;

    MSYS_WARNING("\n>>>> sys_memory_benchmark0\n");
    pBuf=dma_alloc_coherent(sys_dev.this_device, PAGE_ALIGN(nAllocBytes), &bus_addr, GFP_KERNEL);

    if(pBuf==NULL)
    {
        MSYS_ERROR("error while allocating DMA buffer for benchmark...\n");
        return;
    }

    MSYS_WARNING(" Allocated %d bytes at 0x%08x\n", nAllocBytes, (unsigned int) pBuf);

    if ((unsigned int) pBuf % 4096) {
        pAlignedBuf = (void volatile *) (((unsigned int) pBuf + 4096)
                & PAGE_MASK1);
        MSYS_WARNING(" Aligned at 0x%08x\n", (unsigned int) pAlignedBuf);
    } else {
        pAlignedBuf = pBuf;
    }

    /* Show information */
    nCount = (nBufSize / 2) / sizeof(unsigned int);

    pSrc = (ulv *) pAlignedBuf;
    pDest = (ulv *) ((unsigned int) pAlignedBuf + (nBufSize / 2));

    MSYS_WARNING(" Read from : %p\n", pSrc);
    MSYS_WARNING(" Write to  : %p\n", pDest);

    nSize = nCount * sizeof(unsigned int);

    MSYS_WARNING(" Size : %x\n", nSize);

    MSYS_WARNING("\nMemory read/write test\n");
    nLoop = 0;

    MSYS_WARNING("\n(1) Memory read/write test through 32-bit pointer access\n");

    tss = CURRENT_TIME;
    nTestCount = TEST_MemBandWidth(pSrc, pDest, nCount);
    tse = CURRENT_TIME;

    nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
            - tss.tv_nsec / 1000000;

    MSYS_WARNING("Read/Write %3d: %d times, %8MSYS_WARNINGs, %4d msec => %6d KB/sec\n",
            nLoop, nTestCount, nSize, nDelay,
            (((nSize * nTestCount) / 1024) * 1000) / nDelay);

    MSYS_WARNING("\n(2) Memory read/write test through 32-bit pointer access\n");

    tss = CURRENT_TIME;
    nTestCount = TEST_MemBandWidth_long(pSrc, pDest, nCount);
    tse = CURRENT_TIME;

    nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
            - tss.tv_nsec / 1000000;

    MSYS_WARNING("Read/Write %3d: %d times, %8d bytes, %4d msec => %6d KB/sec\n",
            nLoop, nTestCount, nSize, nDelay,
            (((nSize * nTestCount) / 1024) * 1000) / nDelay);

    MSYS_WARNING("\n(3) Memory read/write test through memcpy()\n");

    tss = CURRENT_TIME;
    nTestCount = TEST_Memcpy(pSrc, pDest, nCount);
    tse = CURRENT_TIME;

    nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
            - tss.tv_nsec / 1000000;

    MSYS_WARNING("Read/Write %3d: %d times, %8d bytes, %4d msec => %6d KB/sec\n",
            nLoop, nTestCount, nSize, nDelay,
            (((nSize * nTestCount) / 1024) * 1000) / nDelay);

    MSYS_WARNING("\n(4) Memory read/write test through memcpy(prefetch version)\n");

    tss = CURRENT_TIME;
    nTestCount = TEST_Memcpy_mips(pSrc, pDest, nCount);
    tse = CURRENT_TIME;

    nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
            - tss.tv_nsec / 1000000;

    MSYS_WARNING("Read/Write %3d: %d times, %8d bytes, %4d msec => %6d KB/sec\n",
            nLoop, nTestCount, nSize, nDelay,
            (((nSize * nTestCount) / 1024) * 1000) / nDelay);

    MSYS_WARNING("\n(5) Memory read test\n");

    tss = CURRENT_TIME;
    nTestCount = TEST_Memread(pSrc, nCount);
    tse = CURRENT_TIME;

    nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
            - tss.tv_nsec / 1000000;

    MSYS_WARNING("Read  %3d: %d times, %8d bytes, %4d msec => %6d KB/sec\n", nLoop,
            nTestCount, nSize, nDelay,
            (((nSize * nTestCount) / 1024) * 1000) / nDelay);

    MSYS_WARNING("\n(6) Memory write test\n");

    tss = CURRENT_TIME;
    nTestCount = TEST_Memwrite(pDest, nCount);
    tse = CURRENT_TIME;

    nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
            - tss.tv_nsec / 1000000;

    MSYS_WARNING("Write %3d: %d times, %8d bytes, %4d msec => %6d KB/sec\n", nLoop,
            nTestCount, nSize, nDelay,
            (((nSize * nTestCount) / 1024) * 1000) / nDelay);

    //=============================

    MSYS_WARNING("\n(7) Memory read/write test\n");

    for (i = 1; i < 513; i = i << 1)
    {
        tss = CURRENT_TIME;

        nTestCount = TEST_MemBandWidthRW(pSrc, pDest, nCount, i);

        tse = CURRENT_TIME;

        nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
                - tss.tv_nsec / 1000000;

        MSYS_WARNING("Read/Write  %8d bytes, skip %4d bytes %4d msec => %6d KB/sec\n",
                nSize, i * 4, nDelay,
                ((((nSize / i) * nTestCount) / 1024) * 1000) / nDelay);
    }

    MSYS_WARNING("\n(8) Memory read test\n");

    for (i = 1; i < 513; i = i << 1)
    {
        tss = CURRENT_TIME;

        nTestCount = TEST_MemBandWidthR(pSrc, pDest, nCount, i);

        tse = CURRENT_TIME;

        nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
                - tss.tv_nsec / 1000000;

        MSYS_WARNING("Read  %8d bytes, skip %4d bytes %4d msec => %6d KB/sec\n",
                nSize, i * 4, nDelay,
                ((((nSize / i) * nTestCount) / 1024) * 1000) / nDelay);
    }

    MSYS_WARNING("\n(9) Memory write test\n");

    for (i = 1; i < 513; i = i << 1)
    {
        tss = CURRENT_TIME;

        nTestCount = TEST_MemBandWidthW(pSrc, pDest, nCount, i);

        tse = CURRENT_TIME;

        nDelay = (tse.tv_sec - tss.tv_sec) * 1000 + tse.tv_nsec / 1000000
                - tss.tv_nsec / 1000000;

        MSYS_WARNING("Write  %8d bytes, skip %4d bytes %4d msec => %6d KB/sec\n",
                nSize, i * 4, nDelay,
                ((((nSize / i) * nTestCount) / 1024) * 1000) / nDelay);
    }


    MSYS_WARNING("\n<<<< sys_memory_benchmark0\n");
    dma_free_coherent(sys_dev.this_device, nAllocBytes,pBuf,bus_addr);
    //  munlock((void *) pBuf, nAllocBytes);
    //  free((void *) pBuf);
}

#endif

static int __init setup_system_part_string(char *arg)
{
    memcpy(system_part_string,(arg+1),strlen(arg)<sizeof(system_part_string)?strlen(arg):(sizeof(system_part_string)-1));
    MSYS_WARN("sysp: %s\n",system_part_string);
    return 0;
}
static int __init setup_data_part_string(char *arg)
{
    memcpy(data_part_string,(arg+1),strlen(arg)<sizeof(data_part_string)?strlen(arg):(sizeof(data_part_string)-1));
    MSYS_WARN("data: %s\n",data_part_string);

    return 0;
}

__setup("sysp",setup_system_part_string);
__setup("datap",setup_data_part_string);


static ssize_t us_ticks_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%llu\n",(u64)Chip_Get_US_Ticks());

    return (str - buf);
}

DEVICE_ATTR(us_ticks, 0444, us_ticks_show, NULL);


static ssize_t alloc_dmem(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        int error = 0;
        char name[15];
        unsigned int size;
        const char *str = buf;
        MSYS_DMEM_INFO mem_info;
        memset(mem_info.name,0,16);
        mem_info.phys=0;
        /* parsing input data */
        sscanf(buf, "%s %d", name, &size);

        printk("%s\n", name);
        printk("%d\n", size);

        while (*str && !isspace(*str)) str++;

        len = str - buf;
        if (!len) return -EINVAL;

        len=(len<15)?len:15;
        memcpy(mem_info.name,buf,len);

        if(0!=size)
        {
            mem_info.length=size;
            error = msys_request_dmem(&mem_info);
        }
        else
        {
            printk("Error size is NULL\n");
            return -EINVAL;
        }
        // for MIU BIST only
        if (strcmp(mem_info.name, "MIU_TEST") == 0)
        {
            CLRREG16(BASE_REG_MIU_PA + REG_ID_69, 0xFF);
            OUTREG16(BASE_REG_MIU_PA + REG_ID_71, (mem_info.phys - 0x20000000) >> 13); // test base address[15:0]
            CLRREG16(BASE_REG_MIU_PA + REG_ID_6F, BIT2|BIT3);   // test base address[17:16]
            OUTREG16(BASE_REG_MIU_PA + REG_ID_72, (mem_info.length >> 4) & 0xFFFF); // test length[15:0]
            OUTREG16(BASE_REG_MIU_PA + REG_ID_73, (mem_info.length >> 20) & 0x0FFF); // test length[27:16]
        }
        return n;
    }
    return n;
}

DEVICE_ATTR(dmem_alloc, 0200, NULL, alloc_dmem);

static ssize_t dmem_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    struct list_head *ptr;
    struct DMEM_INFO_LIST *entry;
    int i=0;
    unsigned int total=0;

    list_for_each(ptr, &kept_mem_head)
    {
        entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
        str += scnprintf(str, end - str, "%04d : 0x%08X@%08X [%s]\n",i,(unsigned int)entry->dmem_info.length,(unsigned int)entry->dmem_info.phys,entry->dmem_info.name);

        total+=(unsigned int)entry->dmem_info.length;
        i++;
    }

    str += scnprintf(str, end - str, "\nTOTAL: 0x%08X\n\n",total);

    return (str - buf);
}

DEVICE_ATTR(dmem, 0444, dmem_show, NULL);


static ssize_t release_dmem_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        int error = 0;
        const char *str = buf;
        MSYS_DMEM_INFO mem_info;
        memset(mem_info.name,0,16);
        mem_info.phys=0;

        while (*str && !isspace(*str)) str++;

        len = str - buf;
        if (!len) return -EINVAL;

        len=(len<15)?len:15;
        memcpy(mem_info.name,buf,len);

        error = msys_release_dmem(&mem_info);
        return n;
    }

    return 0;
}

DEVICE_ATTR(release_dmem, 0200, NULL, release_dmem_store);

static ssize_t dmem_realloc_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", dmem_realloc_enabled);
    return (str - buf);
}

static ssize_t dmem_realloc_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        const char *str = buf;

        while (*str && !isspace(*str)) str++;
        len = str - buf;

        if(1==len)
        {
            if('0'==buf[0])
            {
                dmem_realloc_enabled=0;
                MSYS_PRINT("dmem realloc disabled\n");
            }
            else if('1'==buf[0])
            {
                dmem_realloc_enabled=1;
                MSYS_PRINT("dmem realloc enabled\n");
            }
            return n;
        }
        return -EINVAL;
    }
    return -EINVAL;
}

DEVICE_ATTR(dmem_realloc, 0644, dmem_realloc_show, dmem_realloc_store);

static ssize_t unfix_dmem_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        int error = 0;
        const char *str = buf;
        char nbuf[16]={0};

        while (*str && !isspace(*str)) str++;

        len = str - buf;
        if (!len) return -EINVAL;

        len=(len<15)?len:15;
        memcpy(nbuf,buf,len);

        error = msys_unfix_dmem(nbuf);
        return error ? error : n;
    }

    return 0;
}

DEVICE_ATTR(unfix_dmem, 0200, NULL, unfix_dmem_store);

static ssize_t fixed_dmem_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        int error = 0;
        const char *str = buf;
        char nbuf[16]={0};

        while (*str && !isspace(*str)) str++;

        len = str - buf;
        if (!len) return -EINVAL;

        len=(len<15)?len:15;
        memcpy(nbuf,buf,len);

        if(1==len){
            if('0'==nbuf[0]){
                fixed_dmem_enabled=0;
                MSYS_ERROR("fix_dmem disabled\n" ) ;
            }else if('1'==nbuf[0]){
                fixed_dmem_enabled=1;
                MSYS_ERROR("fix_dmem enabled\n" ) ;
            }
        }

        error = msys_fix_dmem((char *)nbuf);
        return error ? error : n;
    }

    return 0;
}

static ssize_t fixed_dmem_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;
    struct list_head *ptr;
    struct DMEM_INFO_LIST *entry;
    int i=0;

    list_for_each(ptr, &fixed_mem_head)
    {
        entry = list_entry(ptr, struct DMEM_INFO_LIST, list);
        str += scnprintf(str, end - str, "%04d: %s\n",i,entry->dmem_info.name);
        i++;
    }
    if (str > buf)  str--;

    str += scnprintf(str, end - str, "\n");

    return (str - buf);
}

DEVICE_ATTR(fixed_dmem, 0644, fixed_dmem_show, fixed_dmem_store);

static ssize_t PIU_T_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%X\n",get_PIU_tick_count());

    return (str - buf);
}

DEVICE_ATTR(PIU_T, 0444, PIU_T_show, NULL);

static ssize_t TEMP_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Temperature %d\n", g_sCurrentTemp);

    return (str - buf);
}

DEVICE_ATTR(TEMP_R, 0444, TEMP_show, NULL);
static ssize_t ms_dump_chip_version(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "Chip_Version: %d\n", msys_get_chipVersion());

    return (str - buf);
}

DEVICE_ATTR(CHIP_VERSION, 0444, ms_dump_chip_version, NULL);

static ssize_t dmem_retry_interval_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        const char *str = buf;
        while (*str && !isspace(*str)) str++;
        len = str - buf;
        if(len)
        {
            dmem_retry_interval = simple_strtoul(buf, NULL, 10);
            MSYS_ERROR("dmem_retry_interval=%d\n", dmem_retry_interval);
            return n;

            /*
            if('0'==buf[0])
            {
                cma_monitor_enabled=0;
                return n;
            }
            else if('1'==buf[0])
            {
                cma_monitor_enabled=1;
                return n;
            }
            else
            {
                return -EINVAL;
            }*/
        }
        return -EINVAL;
    }
    return -EINVAL;
}

static ssize_t dmem_retry_interval_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", dmem_retry_interval);
    return (str - buf);
}

DEVICE_ATTR(dmem_retry_interval, 0644, dmem_retry_interval_show, dmem_retry_interval_store);

static ssize_t dmem_retry_count_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        const char *str = buf;
        while (*str && !isspace(*str)) str++;
        len = str - buf;
        if(len)
        {
            dmem_retry_count = simple_strtoul(buf, NULL, 10);
            MSYS_ERROR("dmem_retry_count=%d\n", dmem_retry_count);
            return n;
            /*
            if('0'==buf[0])
            {
                cma_monitor_enabled=0;
                return n;
            }
            else if('1'==buf[0])
            {
                cma_monitor_enabled=1;
                return n;
            }
            else
            {
                return -EINVAL;
            }*/
        }
        return -EINVAL;
    }
    return -EINVAL;
}

static ssize_t dmem_retry_count_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    char *end = buf + PAGE_SIZE;

    str += scnprintf(str, end - str, "%d\n", dmem_retry_count);
    return (str - buf);
}
DEVICE_ATTR(dmem_retry_count, 0644, dmem_retry_count_show, dmem_retry_count_store);


#ifdef CONFIG_SS_PROFILING_TIME
extern void recode_timestamp(int mark, const char* name);
extern void recode_show(void);
static ssize_t profiling_booting_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    if(NULL!=buf)
    {
        size_t len;
        const char *str = buf;
        int mark=0;

        while (*str && !isspace(*str)) str++;
        len = str - buf;

        if(len)
        {
            mark = simple_strtoul(buf, NULL, 10);
            recode_timestamp(mark, "timestamp");
            return n;

        }
        return -EINVAL;
    }
    return -EINVAL;
}

static ssize_t profiling_booting_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    recode_show();
    return 0;
}
DEVICE_ATTR(booting_time, 0644, profiling_booting_show, profiling_booting_store);
#endif

#if MSYS_PERF_TEST
static char* _perf_fileBuf = NULL;
static char* _perf_fileptr_write = NULL;
static char* _perf_fileptr_read = NULL;
static struct file *_fp = NULL;
#define FILEBUF_SIZE        8192

//////////////////////////////////////////////////////////////////
/// performance test file buffer
//////////////////////////////////////////////////////////////////
#define PERF_BUF_ALLOC() \
({ \
    _perf_fileptr_read = _perf_fileptr_write = _perf_fileBuf = kzalloc(FILEBUF_SIZE, GFP_KERNEL); \
    (_perf_fileBuf); \
})

#define PERF_BUF_FREE() \
{ \
    if (_perf_fileBuf) \
    { \
        kfree(_perf_fileBuf); \
        _perf_fileptr_read = _perf_fileptr_write = _perf_fileBuf = NULL; \
    } \
}


#if 0

#define PERF_BUF_SYNC() \
{ \
    if ((_perf_fileptr_read) && (_perf_fileptr_read != _perf_fileptr_write) && (_fp)) \
    { \
        _fp->f_op->write(_fp, _perf_fileptr_read, strlen(_perf_fileptr_read), &_fp->f_pos); \
        _perf_fileptr_read = _perf_fileptr_write; \
    } \
}

#else

#define PERF_BUF_SYNC() \
{ \
    if ((_perf_fileptr_read) && (_perf_fileptr_read != _perf_fileptr_write) && (_fp)) \
    { \
        vfs_write(_fp, _perf_fileptr_read, strlen(_perf_fileptr_read), &_fp->f_pos); \
        _perf_fileptr_read = _perf_fileptr_write; \
    } \
}

#endif

#define PERF_BUF_RESET()        { _perf_fileptr_read = _perf_fileptr_write = _perf_fileBuf; memset(_perf_fileBuf, 0, FILEBUF_SIZE); }

#define PERF_BUF_PUT(format, ...) \
{ \
    if (_perf_fileptr_write) \
    { \
        _perf_fileptr_write += sprintf(_perf_fileptr_write, format, ##__VA_ARGS__); \
    } \
}

//////////////////////////////////////////////////////////////////
/// performance test command maker
//////////////////////////////////////////////////////////////////
#define PERF_TEST_CMD_MAKE_CPUINFO(cmdBuf) \
                sprintf((cmdBuf), "%s", STR_CPUINFO)
#define PERF_TEST_CMD_MAKE_CACHE(cmdBuf, size, loop) \
                sprintf((cmdBuf), "%s %d %d", STR_CACHE, (size), (loop))
#define PERF_TEST_CMD_MAKE_MIU(cmdBuf, cache, size, loop, scheme) \
                sprintf((cmdBuf), "%s %d %s %d %d %d %d", STR_MIU, (cache), STR_MIU, (cache), (loop), (size), (scheme))
#define PERF_TEST_CMD_MAKE_IMI(cmdBuf, cache, size, loop, scheme) \
                sprintf((cmdBuf), "%s %d %s %d %d %d %d", STR_IMI, (cache), STR_IMI, (cache), (loop), (size), (scheme))

//////////////////////////////////////////////////////////////////
/// performance test file
//////////////////////////////////////////////////////////////////
#define PERF_TEST_FILENAME_MAKE(prefix, pattern, postfix) \
{ \
    strcat((prefix), "_"); \
    strcat((prefix), (pattern)); \
    strcat((prefix), (postfix)); \
}

#define PERF_TEST_FILE_OPEN(filename) \
({ \
    _fp = filp_open((filename), O_CREAT|O_RDWR|O_TRUNC, 0777); \
    (_fp); \
})

#define PERF_TEST_FILE_CLOSE() \
{ \
    if (_fp) \
    { \
        filp_close(_fp, NULL); \
        _fp = NULL; \
    } \
}

static struct timespec time_start;
static void _time_start(void)
{
    getnstimeofday(&time_start);
}

static unsigned long _time_end(void)
{
    unsigned long delta;
    struct timespec ct;

    getnstimeofday(&ct);
    // delta = ct.tv_nsec + ((time_start.tv_sec != ct.tv_sec) ? NSEC_PER_SEC : 0) - time_start.tv_nsec;
    // delta = (ct.tv_sec - time_start.tv_sec)*1000000 + (ct.tv_nsec - time_start.tv_nsec)/1000;
    delta = (ct.tv_sec - time_start.tv_sec)*1000 + (ct.tv_nsec - time_start.tv_nsec)/1000000;
    // printk("[%s][%d] (start, end, delta) = (%d, %d) (%d, %d) %d\n", __FUNCTION__, __LINE__, (int)time_start.tv_sec, (int)time_start.tv_nsec, (int)ct.tv_sec, (int)ct.tv_nsec, (int)delta);
    return delta;
}

static unsigned long _time_end_ns(void)
{
    unsigned long delta;
    struct timespec ct;

    getnstimeofday(&ct);
    delta = (ct.tv_sec - time_start.tv_sec)*1000000000 + (ct.tv_nsec - time_start.tv_nsec);
    // printk("[%s][%d] (start, end, delta) = (%d, %d) (%d, %d) %d\n", __FUNCTION__, __LINE__, (int)time_start.tv_sec, (int)time_start.tv_nsec, (int)ct.tv_sec, (int)ct.tv_nsec, (int)delta);
    return delta;
}

#if 0
static void hex_dump(char* pBuf, int size)
{
    int i;

    for (i = 0; i < size; i++)
    {
        if ((i&0xF) == 0x0)
        {
            printk("\n");
        }
        printk("%02x ", pBuf[i]);
    }
    printk("\n");
}
#endif

static int perf_test_malloc(void** ppBuf, dma_addr_t* pPhys_addr, int size, int bCache)
{
    void* pBuf = NULL;
    dma_addr_t phys_addr = 0;
    int i;

    for (i = 0; i < dmem_retry_count; i++)
    {
        if (bCache)
        {
            pBuf = (void*)__get_free_pages(GFP_KERNEL, get_order(size));
            phys_addr = __pa(pBuf);
        }
        else
        {
            // pBuf = (void*)dma_alloc_coherent(sys_dev.this_device, PAGE_ALIGN(size), &phys_addr, GFP_DMA);
            pBuf = (void*)dma_alloc_coherent(sys_dev.this_device, PAGE_ALIGN(size), &phys_addr, GFP_KERNEL);
        }
        if (pBuf)
        {
            break;
        }
        sysctl_compaction_handler(NULL, 1, NULL, NULL, NULL);
        msleep(1000);
    }
    if (NULL == pBuf)
    {
        return 0;
    }
    *ppBuf = pBuf;
    *pPhys_addr = phys_addr;
    return 1;
}

static void perf_test_mfree(void* pBuf, dma_addr_t phys_addr, int size, int bCache)
{
    if (NULL == pBuf)
    {
        return;
    }
    if (bCache)
    {
        free_pages((long)pBuf, get_order(size));
    }
    else
    {
        pBuf = (void*)dma_alloc_coherent(sys_dev.this_device, PAGE_ALIGN(size), &phys_addr, GFP_DMA);
        dma_free_coherent(sys_dev.this_device, PAGE_ALIGN(size), pBuf, phys_addr);
    }
}

typedef struct
{
    int bCache;
    int bIMI;
    void* pBuf;
    dma_addr_t phys_addr;
} bufInfo;

void _bench_neon_memcpy(void*, void*, unsigned int);
void _bench_memcpy(void*, void*, unsigned int);
typedef enum {
    PERF_TEST_SCHEME_CRT_MEMCPY,
    PERF_TEST_SCHEME_BENCH_MEMCPY,
    PERF_TEST_SCHEME_BENCH_NEON_MEMCPY,
    PERF_TEST_SCHEME_ASSIGN_WRITE_ONLY,
    PERF_TEST_SCHEME_ASSIGN_READ_ONLY,
} perf_test_scheme_t;

static ssize_t perf_test_memcpy(const char* buf, size_t n)
{
    int i;
    unsigned long duration = 0;
    int bitrate;
    unsigned char dst[16];
    unsigned char src[16];
    int iteration, size;
    unsigned int scheme = 0; // 0: CRT memcpy, 1: _bench_memcpy, 2: _bench_neon_memcpy

    bufInfo bufInfoDst = {0} , bufInfoSrc = {0};

    sscanf(buf, "%s %d %s %d %d %d %d", dst, &bufInfoDst.bCache, src, &bufInfoSrc.bCache, &iteration, &size, &scheme);
    printk("INPUT: %s %d %s %d %d %d %d\n", dst, bufInfoDst.bCache, src, bufInfoSrc.bCache, iteration, size, scheme);

    if (0 == strcasecmp(dst, STR_IMI))
    {
        bufInfoDst.bIMI = 1;
        if ((IMI_ADDR_PHYS_1 == IMI_ADDR_INVALID) || (IMI_SIZE_1 == IMI_SIZE_INVALID))
        {
            printk("[%s][%d] invalid IMI address, size 0x%08x, 0x%08x\n", __FUNCTION__, __LINE__, IMI_ADDR_PHYS_1, IMI_SIZE_1);
            goto jump_fail;
        }
        if (bufInfoDst.bCache)
        {
            bufInfoDst.pBuf = (void*)ioremap_cache(IMI_ADDR_PHYS_1, IMI_SIZE_1);
        }
        else
        {
            bufInfoDst.pBuf = (void*)ioremap_nocache(IMI_ADDR_PHYS_1, IMI_SIZE_1);
        }
    }
    else
    {
        bufInfoDst.bIMI = 0;
        perf_test_malloc(&(bufInfoDst.pBuf), &(bufInfoDst.phys_addr), size, bufInfoDst.bCache);
    }
    if (0 == strcasecmp(src, STR_IMI))
    {
        bufInfoSrc.bIMI = 1;
        if ((IMI_ADDR_PHYS_2 == IMI_ADDR_INVALID) || (IMI_SIZE_2 == IMI_SIZE_INVALID))
        {
            printk("[%s][%d] invalid IMI address, size 0x%08x, 0x%08x\n", __FUNCTION__, __LINE__, IMI_ADDR_PHYS_2, IMI_SIZE_2);
            goto jump_fail;
        }
        if (bufInfoSrc.bCache)
        {
            bufInfoSrc.pBuf = (void*)ioremap_cache(IMI_ADDR_PHYS_2, IMI_SIZE_2);
        }
        else
        {
            bufInfoSrc.pBuf = (void*)ioremap_nocache(IMI_ADDR_PHYS_2, IMI_SIZE_2);
        }
    }
    else
    {
        bufInfoSrc.bIMI = 0;
        perf_test_malloc(&(bufInfoSrc.pBuf), &(bufInfoSrc.phys_addr), size, bufInfoSrc.bCache);
    }
    if ((NULL == bufInfoSrc.pBuf) || (NULL == bufInfoDst.pBuf))
    {
        printk("[%s][%d] alloc/ioremap fail\n", __FUNCTION__, __LINE__);
        goto jump_fail;
    }
    printk("[%s][%d] ==============================================\n", __FUNCTION__, __LINE__);
    printk("[%s][%d] dst (tag, cache, addr) = (%s, %d, 0x%08x)\n", __FUNCTION__, __LINE__, dst, bufInfoDst.bCache, (int)bufInfoDst.pBuf);
    printk("[%s][%d] src (tag, cache, addr) = (%s, %d, 0x%08x)\n", __FUNCTION__, __LINE__, src, bufInfoSrc.bCache, (int)bufInfoSrc.pBuf);
    _time_start();
    for (i= 0; i< iteration; i++)
    {
        switch(scheme) {
            case PERF_TEST_SCHEME_CRT_MEMCPY:
                memcpy((void*)bufInfoDst.pBuf, (void*)bufInfoSrc.pBuf, size);
                break;
            case PERF_TEST_SCHEME_BENCH_MEMCPY:
            // _bench_memcpy((void*)bufInfoDst.pBuf, (void*)bufInfoSrc.pBuf, size);
                break;
            case PERF_TEST_SCHEME_BENCH_NEON_MEMCPY:
            // _bench_neon_memcpy((void*)bufInfoDst.pBuf, (void*)bufInfoSrc.pBuf, size);
                break;
            case PERF_TEST_SCHEME_ASSIGN_WRITE_ONLY:
                {
                    register unsigned int j = 0;
                    unsigned int v = 0x55;
                    for(j=0;j<size/4;j++) {
                        ((unsigned int*)bufInfoDst.pBuf)[j] = v;
                    }
                }
                break;
            case PERF_TEST_SCHEME_ASSIGN_READ_ONLY:
                {
                    register unsigned int j = 0;
                    unsigned int v = 0x55;
                    for(j=0;j<size/4;j++) {
                        v  = ((unsigned int*)bufInfoDst.pBuf)[j] ;
                    }
                }
                break;
            default:
                printk("[%s][%d] invalid scheme(%d)\n", __FUNCTION__, __LINE__, scheme);
                goto jump_fail;
                break;
        }
    }
    duration = _time_end();
    iteration /= 1000;
    printk("[%s][%d] (iteration, iteration*size) = (%d, %d) time = %d\n", __FUNCTION__, __LINE__, iteration, iteration*size, (int)duration);
    bitrate = ((iteration*size)/duration);
    printk("[%s][%d] bit rate = %d\n", __FUNCTION__, __LINE__, bitrate);
    PERF_BUF_PUT("%d,", bitrate);
jump_fail:
    if (bufInfoDst.pBuf)
    {
        if (bufInfoDst.bIMI)
        {
            iounmap(bufInfoDst.pBuf);
        }
        else
        {
            perf_test_mfree(bufInfoDst.pBuf, bufInfoDst.phys_addr, size, bufInfoDst.bCache);
        }
    }
    if (bufInfoSrc.pBuf)
    {
        if (bufInfoSrc.bIMI)
        {
            iounmap(bufInfoSrc.pBuf);
        }
        else
        {
            perf_test_mfree(bufInfoSrc.pBuf, bufInfoSrc.phys_addr, size, bufInfoSrc.bCache);
        }
    }
    return n;
}

#define INNER_CLEAN(buf, size)          dmac_map_area((buf), (size), DMA_TO_DEVICE)
#define INNER_INV(buf, size)            dmac_map_area((buf), (size), DMA_FROM_DEVICE)
// cache 32768 100000 // cache size loop
static ssize_t perf_test_cache(const char* buf, size_t n)
{
    unsigned char t1[16];
    int s32BufSize = 0;
    int max_loop = 10000;
    unsigned long int delta1 = 0;
    unsigned long int delta2 = 0;
    int i;
    void* pBuf;
    dma_addr_t phys_addr;

    sscanf(buf, "%s %d %d", t1, &s32BufSize, &max_loop);
    perf_test_malloc(&pBuf, &phys_addr, s32BufSize, 1);
    if (NULL == pBuf)
    {
        printk("[%s][%d] fail allocating memory with size %d)\n", __FUNCTION__, __LINE__, s32BufSize);
        return n;
    }
    PERF_BUF_PUT("%d,", s32BufSize);
    // invalidate cache without memset
    delta1 = delta2 = 0;
    for (i = 0; i < max_loop; i++)
    {
        _time_start();
        INNER_INV(pBuf, s32BufSize); // dmac_map_area(pBuf, s32BufSize, DMA_FROM_DEVICE);
        delta1 += _time_end_ns();
        _time_start();
        outer_inv_range(phys_addr, phys_addr + s32BufSize);
        delta2 += _time_end_ns();
    }
    printk("[%s][%d] Invalidate without dirty : (delta1, delta2) = (%d, %d)\n", __FUNCTION__, __LINE__, (int)(delta1/max_loop), (int)(delta2/max_loop));
    PERF_BUF_PUT("%d,%d,", (int)(delta1/max_loop), (int)(delta2/max_loop));

    // clean cache without memset
    delta1 = delta2 = 0;
    for (i = 0; i < max_loop; i++)
    {
        _time_start();
        INNER_CLEAN(pBuf, s32BufSize); // dmac_map_area(pBuf, s32BufSize, DMA_TO_DEVICE);
        delta1 += _time_end_ns();
        _time_start();
        outer_clean_range(phys_addr, phys_addr + s32BufSize);
        delta2 += _time_end_ns();
    }
    printk("[%s][%d] clean without dirty : (delta1, delta2) = (%d, %d)\n", __FUNCTION__, __LINE__, (int)(delta1/max_loop), (int)(delta2/max_loop));
    PERF_BUF_PUT("%d,%d,", (int)(delta1/max_loop), (int)(delta2/max_loop));

    // printk("[%s][%d] ==========================================================\n", __FUNCTION__, __LINE__);
    // invalidate cache with memset
    delta1 = delta2 = 0;
    for (i = 0; i < max_loop; i++)
    {
        memset(pBuf, 0x00, s32BufSize);
        _time_start();
        INNER_INV(pBuf, s32BufSize); // dmac_map_area(pBuf, s32BufSize, DMA_FROM_DEVICE);
        delta1 += _time_end_ns();
        _time_start();
        outer_inv_range(phys_addr, phys_addr + s32BufSize);
        delta2 += _time_end_ns();
    }
    printk("[%s][%d] Invalidate with dirty : (delta1, delta2) = (%d, %d)\n", __FUNCTION__, __LINE__, (int)(delta1/max_loop), (int)(delta2/max_loop));
    PERF_BUF_PUT("%d,%d,", (int)(delta1/max_loop), (int)(delta2/max_loop));

    // clean cache with memset
    delta1 = delta2 = 0;
    for (i = 0; i < max_loop; i++)
    {
        memset(pBuf, 0x00, s32BufSize);
        _time_start();
        INNER_CLEAN(pBuf, s32BufSize); // dmac_map_area(pBuf, s32BufSize, DMA_TO_DEVICE);
        delta1 += _time_end_ns();
        _time_start();
        outer_clean_range(phys_addr, phys_addr + s32BufSize);
        delta2 += _time_end_ns();
    }
    printk("[%s][%d] clean with dirty : (delta1, delta2) = (%d, %d)\n", __FUNCTION__, __LINE__, (int)(delta1/max_loop), (int)(delta2/max_loop));
    PERF_BUF_PUT("%d,%d,", (int)(delta1/max_loop), (int)(delta2/max_loop));
    PERF_BUF_PUT("\n");
    perf_test_mfree(pBuf, phys_addr, s32BufSize, 1);
    return n;
}

// MIDR
#define ARM_MIDR_READ() \
    ({ \
        int val; \
        asm("mrc p15, 0, r0, c0, c0, 0\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })
#define ARM_MIDR_WRITE(val) \
    { \
        asm("ldr r0, %[reg]\n" : :[reg]"m"((val))); \
        asm("mcr p15, 0, r0, c0, c0, 0\n"); \
    }

// TLBTR
#define ARM_TLBTR_READ() \
    ({ \
        int val; \
        asm("mrc p15, 0, r0, c0, c0, 3\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })

// MPIDR
#define ARM_MPIDR_READ() \
    ({ \
        int val; \
        asm("mrc p15, 0, r0, c0, c0, 5\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })

// CSSELR
#define ARM_CSSELR_READ() \
    ({ \
        int val; \
        asm("mrc p15, 2, r0, c0, c0, 0\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })
#define ARM_CSSELR_WRITE(val) \
    { \
        asm("ldr r0, %[reg]\n" : :[reg]"m"((val))); \
        asm("mcr p15, 2, r0, c0, c0, 0\n"); \
    }

// CCSIDR_L1_I
#define ARM_CCSIDR_L1_I_READ() \
    ({ \
        int val; \
        val = ARM_CSSELR_READ(); \
        val &= 0xFFFFFFF0; \
        val |= 0x00000001; \
        ARM_CSSELR_WRITE(val); \
        asm("mrc p15, 1, r0, c0, c0, 0\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })

// CCSIDR_L1_D
#define ARM_CCSIDR_L1_D_READ() \
    ({ \
        int val; \
        val = ARM_CSSELR_READ(); \
        val &= 0xFFFFFFF0; \
        val |= 0x00000000; \
        ARM_CSSELR_WRITE(val); \
        asm("mrc p15, 1, r0, c0, c0, 0\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })

// CCSIDR_L2
#define ARM_CCSIDR_L2_READ() \
    ({ \
        int val; \
        val = ARM_CSSELR_READ(); \
        val &= 0xFFFFFFF0; \
        val |= 0x00000002; \
        ARM_CSSELR_WRITE(val); \
        asm("mrc p15, 1, r0, c0, c0, 0\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })

// CLIDR
#define ARM_CLIDR_READ() \
    ({ \
        int val; \
        asm("mrc p15, 1, r0, c0, c0, 1\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })


#define ARM_CTR_READ() \
    ({ \
        int val; \
        asm("mrc p15, 0, r0, c0, c0, 1\n"); \
        asm("str r0, %[reg]\n" : [reg]"=m"(val)); \
        val; \
    })

#define CCSIDR_DECODE(CCSIDR, WT, WB, RA, WA, NumSet, Associate, LineSize) \
    { \
        (WT) = ((CCSIDR) >> 31) & 0x00000001; \
        (WB) = ((CCSIDR) >> 30) & 0x00000001; \
        (RA) = ((CCSIDR) >> 29) & 0x00000001; \
        (WA) = ((CCSIDR) >> 28) & 0x00000001; \
        (NumSet) = ((CCSIDR) >> 13) & 0x00007FFF; \
        (Associate) = ((CCSIDR) >>  3) & 0x000003FF; \
        (LineSize) = ((CCSIDR) >>  0) & 0x00000007; \
    }
#define CACHE_SIZE(NumSet, Associate, LineSize)         (((((NumSet)+1) * (LineSize))*((Associate)+1))>>5)
#define CCSIDR_DUMP(str, WT, WB, RA, WA, NumSet, Associate, LineSize) \
    { \
        printk("[%s][%d] %s\n", __FUNCTION__, __LINE__, (str)); \
        PERF_BUF_PUT("%s\n", (str)); \
        printk("[%s][%d]         (WT, WB, RA, WA) = (%d, %d, %d, %d)\n", __FUNCTION__, __LINE__, (WT), (WB), (RA), (WA)); \
        PERF_BUF_PUT(",Write through, %d\n", (WT)); \
        PERF_BUF_PUT(",Write back, %d\n", (WB)); \
        PERF_BUF_PUT(",Read allocate, %d\n", (RA)); \
        PERF_BUF_PUT(",Write allocate, %d\n", (WA)); \
        printk("[%s][%d]         %d NumSet\n", __FUNCTION__, __LINE__, ((NumSet)+1)); \
        PERF_BUF_PUT(",NumSet, %d\n", ((NumSet)+1)); \
        printk("[%s][%d]         %d ways\n", __FUNCTION__, __LINE__, ((Associate)+1)); \
        PERF_BUF_PUT(",Ways, %d\n", ((Associate)+1)); \
        printk("[%s][%d]         line size = %d bytes\n", __FUNCTION__, __LINE__, ((LineSize)<<5) ); \
        PERF_BUF_PUT(",Line size (Bytes), %d\n", ((LineSize)<<5) ); \
        printk("[%s][%d]         cache size = %d KB\n", __FUNCTION__, __LINE__, CACHE_SIZE((NumSet), (Associate), (LineSize))); \
        PERF_BUF_PUT(",Cache size (KBytes), %d\n", CACHE_SIZE((NumSet), (Associate), (LineSize))); \
    }

u32 msys_l2x0_size = 0;
u32 msys_l2x0_ways = 0;
u32 msys_l2x0_linesize = 32; // constant for PL310

static ssize_t perf_test_cpuinfo(const char* buf, size_t n)
{
    unsigned int MIDR = 0;
    unsigned int MPIDR = 0;
    unsigned int CCSIDR_L1_I = 0;
    unsigned int CCSIDR_L1_D = 0;
    unsigned int CCSIDR_L2 = 0;
    unsigned int CTR = 0;
    unsigned int cpuPart = 0;
    char* strCPU = STR_CPUINFO_NULL;
    int WT, WB, RA, WA, NumSet, Associate, LineSize;
    int cpu;
    int cpuNR = 0;
    unsigned int CLIDR = ARM_CLIDR_READ();

    for_each_online_cpu(cpu)
    {
        cpuNR++;
    }
    MIDR = ARM_MIDR_READ();
    MPIDR = ARM_MPIDR_READ();
    CCSIDR_L1_I = ARM_CCSIDR_L1_I_READ();
    CCSIDR_L1_D = ARM_CCSIDR_L1_D_READ();
    CCSIDR_L2 = 0;
    if (CLIDR & 0x00000038)
        CCSIDR_L2 = ARM_CCSIDR_L2_READ();
    // CCSIDR_L2 = ARM_CCSIDR_L2_READ();
    CTR = ARM_CTR_READ();
    cpuPart = ((MIDR >> 4) & 0x00000FFF);
    switch (cpuPart)
    {
    case CPU_PART_CA7:
        strCPU = STR_CPUINFO_CA7;
        break;
    case CPU_PART_CA9:
        strCPU = STR_CPUINFO_CA9;
        break;
    case CPU_PART_CA53:
        strCPU = STR_CPUINFO_CA53;
        break;
    default:
        break;
    }
    // printk("[%s][%d] (MIDR, MPIDR) = (0x%08x, 0x%08x)\n", __FUNCTION__, __LINE__, MIDR, MPIDR);
    printk("[%s][%d] (CPU type, core) = (%s, %d)\n", __FUNCTION__, __LINE__, strCPU, cpuNR);
    PERF_BUF_PUT("CPU,%s\n", strCPU);
    PERF_BUF_PUT("Core number,%d\n", cpuNR);
    CCSIDR_DECODE(CCSIDR_L1_I, WT, WB, RA, WA, NumSet, Associate, LineSize);
    CCSIDR_DUMP("L1 instruction cache information", WT, WB, RA, WA, NumSet, Associate, LineSize);
    CCSIDR_DECODE(CCSIDR_L1_D, WT, WB, RA, WA, NumSet, Associate, LineSize);
    CCSIDR_DUMP("L1 data cache information", WT, WB, RA, WA, NumSet, Associate, LineSize);
    if (CPU_PART_CA9 != cpuPart)
    {
        CCSIDR_DECODE(CCSIDR_L2, WT, WB, RA, WA, NumSet, Associate, LineSize);
        CCSIDR_DUMP("L2 cache information", WT, WB, RA, WA, NumSet, Associate, LineSize);
    }
    else
    {
        printk("[%s][%d] %s\n", __FUNCTION__, __LINE__, "L2 cache information");
        PERF_BUF_PUT("%s\n", "L2 cache information");
        printk("[%s][%d]         %d ways\n", __FUNCTION__, __LINE__, msys_l2x0_ways);
        PERF_BUF_PUT(",Ways,%d\n", msys_l2x0_ways);
        printk("[%s][%d]         line size = %d bytes\n", __FUNCTION__, __LINE__, msys_l2x0_linesize);
        PERF_BUF_PUT(",Line size (Bytes), %d\n", msys_l2x0_linesize);
        printk("[%s][%d]         cache size = %d KB\n", __FUNCTION__, __LINE__, (msys_l2x0_size >> 10));
        PERF_BUF_PUT(",Cache size(KBytes), %d\n", (msys_l2x0_size >> 10));
    }

    {
        unsigned int CLIDR = ARM_CLIDR_READ();
        printk("[%s][%d]         CLIDR = 0x%08x\n", __FUNCTION__, __LINE__, CLIDR);
    }


    return n;
}


// all /vendor/2222.txt
// cpuinfo
// MIU 1 MIU 1 10000 65536 0
// IMI 1 IMI 1 10000 65536 0
// cache 32768 100000

char cmdBuf[128] = { 0 };

static ssize_t perf_test_all(const char* buf, size_t n)
{
    // char buf[100];
    char temp[8];
    char filename[128];
    char filename_org[128];
    mm_segment_t old_fs;

    if (NULL == PERF_BUF_ALLOC())
    {
        printk("[%s][%d] kzalloc fail with size %d\n", __FUNCTION__, __LINE__, FILEBUF_SIZE);
        return n;
    }
    sscanf(buf, "%s %s", temp, filename_org);
#if 1
    // cpu information
    strcpy(filename, filename_org);
    PERF_TEST_FILENAME_MAKE(filename, STR_CPUINFO, ".csv");
    if (PERF_TEST_FILE_OPEN(filename))
    {
        old_fs = get_fs();
        set_fs(get_ds());
        PERF_TEST_CMD_MAKE_CPUINFO(cmdBuf);
        perf_test_cpuinfo(cmdBuf, strlen(cmdBuf));
        PERF_BUF_SYNC();
        PERF_BUF_RESET();
        PERF_TEST_FILE_CLOSE(); // filp_close(fp, NULL);
        set_fs(old_fs);
    }
    // cache testing
    strcpy(filename, filename_org);
    PERF_TEST_FILENAME_MAKE(filename, STR_CACHE, ".csv");
    if (PERF_TEST_FILE_OPEN(filename))
    {
        int size[] = { 4096, 8192, 16394, 32768, 65536, 131072 };
        int i = 0;
        int iteration = 10000;

        old_fs = get_fs();
        set_fs(get_ds());
        PERF_BUF_PUT("Size,Bytes\n");
        PERF_BUF_PUT("Iteration,%d\n", iteration);
        PERF_BUF_PUT("Time,Nanoseconds\n");
        PERF_BUF_PUT("Size,L1(Inv),L2(Inv),L1(Clean),L2(Clean),L1(Inv dirty),L2(Inv dirty),L1(Clean dirty),L2(Clean dirty)\n");
        for (i = 0 ; i < sizeof(size)/sizeof(size[0]); i++)
        {
            PERF_TEST_CMD_MAKE_CACHE(cmdBuf, size[i], iteration);
            perf_test_cache(cmdBuf, strlen(cmdBuf));
        }
        PERF_BUF_SYNC();
        PERF_BUF_RESET();
        PERF_TEST_FILE_CLOSE(); // filp_close(fp, NULL);
        set_fs(old_fs);
    }
    // MIU testing
    strcpy(filename, filename_org);
    PERF_TEST_FILENAME_MAKE(filename, STR_MIU, ".csv");
    if (PERF_TEST_FILE_OPEN(filename))
    {
        // int size[] = { 32768, 65536, 131072, 262144, 524288, 1048576, 2097152 };
        int size[] = { 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152 };
        int iter[] = { 1000000, 1000000, 100000, 10000, 10000, 10000, 10000, 10000, 10000, 10000};
        int i = 0;
        int iteration = 10000;

        old_fs = get_fs();
        set_fs(get_ds());
        PERF_BUF_PUT("Size,Bytes\n");
        PERF_BUF_PUT("Iteration,%d\n", iteration);
        PERF_BUF_PUT("Bit rate,MBytes/Sec\n\n\n\n");
        // non-cache
        PERF_BUF_PUT("%s noncache\n", STR_MIU);
        PERF_BUF_PUT("Size,");
        for (i = 0 ; i < sizeof(size)/sizeof(size[0]); i++)
        {
            PERF_BUF_PUT("%d,", size[i]);
        }
        PERF_BUF_PUT("\n");
        PERF_BUF_PUT("Bit rate,");
        for (i = 0 ; i < sizeof(size)/sizeof(size[0]); i++)
        {
            PERF_TEST_CMD_MAKE_MIU(cmdBuf, 0, size[i], iteration, 0);
            perf_test_memcpy(cmdBuf, strlen(cmdBuf));
        }
        PERF_BUF_PUT("\n");
        PERF_BUF_PUT("\n");
        // cache
        PERF_BUF_PUT("%s cache\n", STR_MIU);
        PERF_BUF_PUT("Size,");
        for (i = 0 ; i < sizeof(size)/sizeof(size[0]); i++)
        {
            PERF_BUF_PUT("%d,", size[i]);
        }
        PERF_BUF_PUT("\n");
        PERF_BUF_PUT("Bit rate,");
        for (i = 0 ; i < sizeof(size)/sizeof(size[0]); i++)
        {
            PERF_TEST_CMD_MAKE_MIU(cmdBuf, 1, size[i], iter[i], 0);
            perf_test_memcpy(cmdBuf, strlen(cmdBuf));
        }
        PERF_BUF_PUT("\n");
        PERF_BUF_SYNC();
        PERF_BUF_RESET();
        PERF_TEST_FILE_CLOSE(); // filp_close(fp, NULL);
        set_fs(old_fs);
    }
    // IMI testing
    strcpy(filename, filename_org);
    PERF_TEST_FILENAME_MAKE(filename, STR_IMI, ".csv");
    if (PERF_TEST_FILE_OPEN(filename))
    {
        int size[] = { 32768}; // , 65536, 131072, 262144, 524288, 1048576, 2097152 };
        int i = 0;
        int iteration = 10000;

        old_fs = get_fs();
        set_fs(get_ds());

        PERF_BUF_PUT("Size,Bytes\n");
        PERF_BUF_PUT("Iteration,%d\n", iteration);
        PERF_BUF_PUT("Bit rate,MBytes/Sec\n\n\n\n");
        // non-cache
        PERF_BUF_PUT("%s noncache\n", STR_IMI);
        PERF_BUF_PUT("Size,");
        for (i = 0 ; i < sizeof(size)/sizeof(size[0]); i++)
        {
            PERF_BUF_PUT("%d,", size[i]);
        }
        PERF_BUF_PUT("\n");
        PERF_BUF_PUT("Bit rate,");
        for (i = 0 ; i < sizeof(size)/sizeof(size[0]); i++)
        {
            PERF_TEST_CMD_MAKE_IMI(cmdBuf, 0, size[i], 10000, 0);
            perf_test_memcpy(cmdBuf, strlen(cmdBuf));
        }
        PERF_BUF_PUT("\n");
        PERF_BUF_SYNC();
        PERF_BUF_RESET();
        PERF_TEST_FILE_CLOSE(); // filp_close(fp, NULL);
        set_fs(old_fs);
    }
#endif
    PERF_BUF_FREE();
    return n;
}

static ssize_t perf_test_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    int n;

    n = sprintf(str, "[%s][%d] this is a perf_test_show\n", __FUNCTION__, __LINE__);
    return n;
}

static ssize_t perf_test_entry(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned char token[16];
    sscanf(buf, "%s", token);
    if (0 == strcasecmp(token, STR_CPUINFO))
    {
        return perf_test_cpuinfo(buf, n);
    }
    else if (0 == strcasecmp(token, STR_IMI))
    {
        return perf_test_memcpy(buf, n);
    }
    else if (0 == strcasecmp(token, STR_MIU))
    {
        return perf_test_memcpy(buf, n);
    }
    else if (0 == strcasecmp(token, STR_CACHE))
    {
        return perf_test_cache(buf, n);
    }
    else if (0 == strcasecmp(token, STR_ALL))
    {
        return perf_test_all(buf, n);
    }
    return n;
}
DEVICE_ATTR(perf_test, 0644, perf_test_show, perf_test_entry);

#endif // #if MSYS_PERF_TEST

#if MSYS_MIU_PROTECT
static ssize_t miu_protect_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    char *str = buf;
    int n;

    n = sprintf(str, "[%s][%d]: OK!\n", __FUNCTION__, __LINE__);
    return n;
}

static ssize_t miu_protect_entry(struct device *dev, struct device_attribute *attr, const char *buf, size_t n)
{
    unsigned char Result = FALSE;
    unsigned char u8Blockx;
    unsigned short u8ProtectId[16] = {0};
    unsigned long long u64BusStart;
    unsigned long long u64BusEnd;
    unsigned char  bSetFlag;

    unsigned char token[16];

    sscanf(buf, "%s", token);
    if (0 == strcasecmp(token, "set"))
    {
        sscanf(buf, "%s %hhu %hu %llx %llx %hhu", token, &u8Blockx, &u8ProtectId[0], &u64BusStart, &u64BusEnd, &bSetFlag);

        printk("%s(%d) INPUT: (u8Blockx,u8ProtectId,u64BusStart,u64BusEnd,bSetFlag)=(%hhu,%hu,0x%08llx,0x%08llx,%hhu)\n", __FUNCTION__, __LINE__, u8Blockx, u8ProtectId[0], u64BusStart, u64BusEnd, bSetFlag);

        Result = MDrv_MIU_Protect(u8Blockx, &u8ProtectId[0], u64BusStart, u64BusEnd, bSetFlag);

        if(Result == FALSE ) {
            printk("ERR: Result = %d", Result);
        }
    }
    else if (0 == strcasecmp(token, "test"))
    {
        unsigned int* _va;
        sscanf(buf, "%s %llx", token, &u64BusStart);

        _va = ioremap((unsigned int)u64BusStart, 0x1000);
        printk("%s(%d) Write: MIU @ 0x%08llx, VA @ 0x%08x\n", __FUNCTION__, __LINE__, u64BusStart, (unsigned int)_va);

        *(_va) = 0xDEADBEEF;
    }
    else if (0 == strcasecmp(token, "test1"))
    {
        sscanf(buf, "%s %llx", token, &u64BusStart);

        printk("%s(%d) Write: MIU @ 0x%08llx\n", __FUNCTION__, __LINE__, u64BusStart);

        *(unsigned int*)((void*)(unsigned int)u64BusStart) = 0xDEADBEEF;
    }
    else
    {
         printk("%s(%d) Wrong parameter:\n", __FUNCTION__, __LINE__);
         printk("%s(%d) Usage: echo [CMD] [miublk] [client id] [start addr] [end addr] [enable] > miu_protect\n", __FUNCTION__, __LINE__);
         printk("%s(%d) Ex: echo set 0 1 0x20000000 0x20100000 1 > miu_protect\n", __FUNCTION__, __LINE__);
    }

    return n;
}
DEVICE_ATTR(miu_protect, 0644, miu_protect_show, miu_protect_entry);
#endif

struct list_head proc_info_head;
static struct mutex proc_info_mutex;
static struct proc_dir_entry* proc_class=NULL;
static struct proc_dir_entry* proc_zen_kernel=NULL;

struct proc_dir_entry* msys_get_proc_class(void)
{
	return proc_class;
}

struct proc_dir_entry* msys_get_proc_zen_kernel(void)
{
	return proc_zen_kernel;
}

static int msys_seq_show(struct seq_file*m, void *p)
{
  struct inode *inode = (struct inode *)m->private;

  PROC_INFO_LIST *proc_list = proc_get_parent_data(inode);
  MSYS_PROC_ATTRIBUTE* proc_attr = PDE_DATA(inode);

  switch(proc_attr->type)
  {
    case MSYS_PROC_ATTR_CHAR:
      seq_printf(m, "%c\n", *(unsigned int *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_UINT:
      seq_printf(m, "%u\n", *(unsigned int *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_INT:
      seq_printf(m, "%d\n", *(int *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_XINT:
      seq_printf(m, "0x%x\n", *(unsigned int *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_ULONG:
      seq_printf(m, "%lu\n", *(unsigned long *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_LONG:
      seq_printf(m, "%ld\n", *(long *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_XLONG:
      seq_printf(m, "0x%lx\n", *(unsigned long *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_ULLONG:
      seq_printf(m, "%llu\n", *(unsigned long long *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_LLONG:
      seq_printf(m, "%lld\n", *(long long *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_XLLONG:
      seq_printf(m, "0x%llx\n", *(unsigned long long*)(proc_list->proc_addr +  proc_attr->offset));
      break;
    case MSYS_PROC_ATTR_STRING:
      seq_printf(m, "%s\n", (unsigned char *)(proc_list->proc_addr +  proc_attr->offset));
      break;
    default:
      break;
  }

  return 0;
}

static int msys_proc_open(struct inode *inode, struct file *file)
{
     return single_open(file, msys_seq_show, inode);
}

static int msys_proc_mmap(struct file *file, struct vm_area_struct *vma)
{
  int ret = 0;
  struct page *page = NULL;

  struct inode *inode = (struct inode *)(((struct seq_file *)file->private_data)->private);

  PROC_INFO_LIST *proc_list = proc_get_parent_data(inode);

	size_t size = vma->vm_end - vma->vm_start;

  if (size > proc_list->proc_dev.size)
  {
      MSYS_ERROR("msys_proc_mmap - invalid size = %d\n", size);
      return -EINVAL;
  }

  page = virt_to_page((unsigned long)proc_list->proc_addr + (vma->vm_pgoff << PAGE_SHIFT));
  ret = remap_pfn_range(vma, vma->vm_start, page_to_pfn(page), size, vma->vm_page_prot);
  if (ret)
  {
    MSYS_ERROR("msys_proc_mmap - remap_pfn_range failed.\n");
    return ret;
  }
  //vma->vm_start = (unsigned long)info_addr;
	//vma->vm_end = vma->vm_start + PAGE_ALIGN(MAX_LEN);
	//vma->vm_flags |=  VM_SHARED | VM_WRITE | VM_READ;

  //vma->vm_ops = &rpr_vm_ops;

  //if (remap_page_range(start, page, PAGE_SIZE, PAGE_SHARED))
  //                      return -EAGAIN;
	return 0 ;
}

static const struct file_operations msys_proc_fops = {
 .owner = THIS_MODULE,
 .open  = msys_proc_open,
 .read  = seq_read,
 .llseek = seq_lseek,
 .release = single_release,
};

static const struct file_operations msys_proc_mmap_fops = {
 .owner = THIS_MODULE,
 .open = msys_proc_open,
 .mmap = msys_proc_mmap,
 .release = single_release,
};

static PROC_INFO_LIST *msys_get_proc_info(MSYS_PROC_DEVICE* proc_dev)
{
    struct list_head *tmp_proc_entry = NULL;
    PROC_INFO_LIST *tmp_proc_info = NULL;

    list_for_each(tmp_proc_entry, &proc_info_head) {
        tmp_proc_info = list_entry(tmp_proc_entry, PROC_INFO_LIST, list);
        if (tmp_proc_info->proc_dev.parent == proc_dev->parent
            && strcmp(tmp_proc_info->proc_dev.name, proc_dev->name) == 0) {
            //MSYS_ERROR("%s: Find %s handle = %p\n", __func__, proc_dev->name, tmp_proc_info);
            return tmp_proc_info;
        }
    }
    return NULL;
}

static PROC_INFO_LIST *msys_get_child_proc_info(PROC_INFO_LIST *parent_proc_info)
{
    struct list_head *tmp_proc_entry = NULL;
    PROC_INFO_LIST *tmp_proc_info = NULL;

    list_for_each(tmp_proc_entry, &proc_info_head) {
        tmp_proc_info = list_entry(tmp_proc_entry, PROC_INFO_LIST, list);
        if (tmp_proc_info->proc_dev.parent == parent_proc_info) {
            //MSYS_ERROR("%s; Find %s has child %s = %p\n", __func__, parent_proc_info->proc_dev.name, tmp_proc_info->proc_dev.name, tmp_proc_info);
            return tmp_proc_info;
        }
    }
    return NULL;
}

static int msys_request_proc_attr(MSYS_PROC_ATTRIBUTE* proc_attr)
{
    int err = 0;
    struct proc_dir_entry* tmp_proc_entry;
    PROC_INFO_LIST *parent_proc_info;
    MSYS_PROC_ATTRIBUTE *new_proc_attr;

    mutex_lock(&proc_info_mutex);
    if(/*proc_attr->name != NULL &&*/ proc_attr->name[0] != 0) {
        new_proc_attr = (MSYS_PROC_ATTRIBUTE *)kmalloc(sizeof(MSYS_PROC_ATTRIBUTE), GFP_KERNEL);
        if (!new_proc_attr) {
            MSYS_ERROR("kmalloc MSYS_PROC_ATTRIBUTE failed!!\n" );
            BUG();
        }
        *new_proc_attr = *proc_attr; //It will be freed when release device/attributes.

        parent_proc_info = new_proc_attr->handle;
        tmp_proc_entry = proc_create_data(new_proc_attr->name, 0, parent_proc_info->proc_entry, &msys_proc_fops, new_proc_attr);
        if (!tmp_proc_entry) {
            //MSYS_ERROR("Skip since attribute %s exists\n", proc_attr->name);
            err = -EEXIST;
            kfree(new_proc_attr);
        } else {
            //MSYS_ERROR("Set attribute %s handle = %p\n", proc_attr->name, proc_attr->handle);
        }
    }
    mutex_unlock(&proc_info_mutex);
    return err;
}

static int msys_release_proc_attr(MSYS_PROC_ATTRIBUTE* proc_attr)
{
    return 0;
}

static int msys_request_proc_dev(MSYS_PROC_DEVICE* proc_dev)
{
    int err = 0;
    PROC_INFO_LIST *new_proc_info = NULL;

    mutex_lock(&proc_info_mutex);

    if(/*proc_dev->name != NULL && */proc_dev->name[0] != 0) {
        if((proc_dev->handle = msys_get_proc_info(proc_dev)) != NULL) {
            //MSYS_ERROR("Device proc_info %s exist, return original handle = %p\n" , proc_dev->name, proc_dev->handle);
            err = -EEXIST;
            goto GG;
        }

        new_proc_info = (PROC_INFO_LIST *)kmalloc(sizeof(PROC_INFO_LIST), GFP_KERNEL);
        if (!new_proc_info) {
            MSYS_ERROR("kmalloc PROC_INFO_LIST failed!!\n" );
            err = -ENOMEM;
            goto GG;
        }

        new_proc_info->proc_entry = proc_mkdir_data(proc_dev->name, 0,
            (proc_dev->parent)?((PROC_INFO_LIST *)proc_dev->parent)->proc_entry:proc_class, new_proc_info);
        if (!new_proc_info->proc_entry) {
            MSYS_ERROR("Skip since device proc_entry %s exists\n", proc_dev->name);
            err = -EEXIST;
            kfree(new_proc_info);
            goto GG;
        }

        if (proc_dev->parent && proc_dev->size == 0) { //subdevice case
            new_proc_info->proc_addr = ((PROC_INFO_LIST *)proc_dev->parent)->proc_addr;
        }
        else { //device case
            if (proc_dev->size & ~PAGE_MASK) {
                proc_dev->size &= PAGE_MASK;
                proc_dev->size += PAGE_SIZE;
                //MSYS_ERROR("Size not align with %ld, resize to %ld\n", PAGE_SIZE, proc_dev->size);
            }
            if(proc_dev->size > KMALLOC_MAX_SIZE)
            {
                MSYS_ERROR("allocate %lu kernel memory for proc data error\n", proc_dev->size);
                err = -ENOMEM;
                kfree(new_proc_info);
                goto GG;
            }
            new_proc_info->proc_addr = kmalloc(proc_dev->size, GFP_KERNEL);
            if(!new_proc_info->proc_addr) {
                MSYS_ERROR("allocate %lu kernel memory for proc data error\n", proc_dev->size);
                err = -ENOMEM;
                kfree(new_proc_info);
                goto GG;
            }
            proc_create(".mmap", 0, new_proc_info->proc_entry, &msys_proc_mmap_fops); //It will be freed when relealse device.
        }

        proc_dev->handle = new_proc_info;
        new_proc_info->proc_dev = *proc_dev;
        list_add(&new_proc_info->list, &proc_info_head);
        //MSYS_ERROR("Set device %s handle = %p\n", new_proc_info->proc_dev.name, new_proc_info->proc_dev.handle);
    }
GG:
    mutex_unlock(&proc_info_mutex);
    return err;
}

static int msys_release_proc_dev(MSYS_PROC_DEVICE* proc_dev)
{
    int err = 0;
    PROC_INFO_LIST *tmp_proc_info = NULL;
    PROC_INFO_LIST *target_proc_info = NULL;
    PROC_INFO_LIST *parent_proc_info = NULL;
    PROC_INFO_LIST *child_proc_info = NULL;

    target_proc_info = msys_get_proc_info(proc_dev);
    mutex_lock(&proc_info_mutex);
    if(target_proc_info == NULL) {
        MSYS_ERROR("%s: Cannot find handle of %s\n", __func__, proc_dev->name);
        err = -ENODEV;
    } else {
        //Remove proc_entry
        proc_remove(target_proc_info->proc_entry);
        tmp_proc_info = target_proc_info;
        //Find all proc_info's child from proc_info_list and remove proc_info from bottom which doesn't have child.
        do {
            child_proc_info = msys_get_child_proc_info(tmp_proc_info);
            if(child_proc_info == NULL) {
                parent_proc_info = tmp_proc_info->proc_dev.parent;
                //MSYS_ERROR("%s: Free %s handle = %p\n", __func__, tmp_proc_info->proc_dev.name, tmp_proc_info->proc_dev.handle);
                __list_del_entry(&tmp_proc_info->list);
                kfree(tmp_proc_info);
                if(tmp_proc_info != target_proc_info) {
                    tmp_proc_info = parent_proc_info;
                } else {
                    break;
                }
            }else
                tmp_proc_info = child_proc_info;
        } while(1);
    }
    mutex_unlock(&proc_info_mutex);
    return err;
}

static struct class *msys_sysfs_class = NULL;

struct class *msys_get_sysfs_class(void)
{
  if (!msys_sysfs_class)
  {
      msys_sysfs_class = class_create(THIS_MODULE, "mstar");
      if (!msys_sysfs_class)
        MSYS_ERROR("cannot get class for sysfs\n");
  }
  return msys_sysfs_class;
}

static int __init msys_init(void)
{
    int ret;

    //ret = misc_register(&sys_dev);
    ret = register_chrdev(MAJOR_SYS_NUM, "msys", &msys_fops);
    if (ret != 0) {
        MSYS_ERROR("cannot register msys on minor=11 (err=%d)\n", ret);
    }

    sys_dev.this_device = device_create(msys_get_sysfs_class(), NULL,
	    MKDEV(MAJOR_SYS_NUM, MINOR_SYS_NUM), NULL, "msys");

    sys_dev.this_device->dma_mask=&sys_dma_mask;
    sys_dev.this_device->coherent_dma_mask=sys_dma_mask;

    mutex_init(&dmem_mutex);
#if defined(CONFIG_MS_MOVE_DMA)
    CamOsTsemInit(&m_stMdmaDoneSem, 0);

    HalMoveDma_Initialize();
#endif
#if defined(CONFIG_MS_BDMA)
    CamOsTsemInit(&m_stBdmaDoneSem[0], 0);
    CamOsTsemInit(&m_stBdmaDoneSem[1], 0);
    CamOsTsemInit(&m_stBdmaDoneSem[2], 0);
    CamOsTsemInit(&m_stBdmaDoneSem[3], 0);

    //HalBdma_Initialize(0);
    HalBdma_Initialize(1);
    HalBdma_Initialize(2);
    HalBdma_Initialize(3);
#endif
    INIT_LIST_HEAD(&kept_mem_head);
    INIT_LIST_HEAD(&fixed_mem_head);

    device_create_file(sys_dev.this_device, &dev_attr_dmem);
    device_create_file(sys_dev.this_device, &dev_attr_fixed_dmem);
    device_create_file(sys_dev.this_device, &dev_attr_unfix_dmem);
    device_create_file(sys_dev.this_device, &dev_attr_release_dmem);
    device_create_file(sys_dev.this_device, &dev_attr_PIU_T);
    device_create_file(sys_dev.this_device, &dev_attr_dmem_retry_interval);
    device_create_file(sys_dev.this_device, &dev_attr_dmem_retry_count);
    device_create_file(sys_dev.this_device, &dev_attr_us_ticks);
    device_create_file(sys_dev.this_device, &dev_attr_dmem_realloc);
    device_create_file(sys_dev.this_device, &dev_attr_dmem_alloc);
    device_create_file(sys_dev.this_device, &dev_attr_TEMP_R);
    device_create_file(sys_dev.this_device, &dev_attr_CHIP_VERSION);
#ifdef CONFIG_SS_PROFILING_TIME
    device_create_file(sys_dev.this_device, &dev_attr_booting_time);
#endif
#if MSYS_PERF_TEST
    device_create_file(sys_dev.this_device, &dev_attr_perf_test);
#endif // #if MSYS_PERF_TEST
#if MSYS_MIU_PROTECT
    device_create_file(sys_dev.this_device, &dev_attr_miu_protect);
#endif
//    ret = device_create_file(sys_dev.this_device, &dev_attr_dmem);

//    if (ret != 0)printk("Failed to create sysfs files: %d\n", ret);


#if defined(CONFIG_PROC_FS)
    mutex_init(&proc_info_mutex);
    INIT_LIST_HEAD(&proc_info_head);
    proc_class=proc_mkdir("mstar",NULL);
    proc_zen_kernel=proc_mkdir("kernel",proc_class);
#endif

    MSYS_WARN(" INIT DONE. TICK=0x%08X\n",get_PIU_tick_count());

    return 0;
}

//!!!! msys_kfile_* API has not been tested as they are not used. 2016/07/18
struct file* msys_kfile_open(const char* path, int flags, int rights)
{
    struct file* filp = NULL;
    mm_segment_t oldfs;
    int err = 0;

    oldfs = get_fs();
    set_fs(get_ds());
    filp = filp_open(path, flags, rights);
    set_fs(oldfs);
    if(IS_ERR(filp)) {
        err = PTR_ERR(filp);
        return NULL;
    }
    return filp;
}

void msys_kfile_close(struct file* fp)
{
    if(fp)
    {
        filp_close(fp,NULL);
    }
}

int msys_kfile_write(struct file* fp, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret=-EINVAL;

    if(fp)
    {
        oldfs = get_fs();
        set_fs(get_ds());
        ret = vfs_write(fp, data, size, &offset);
        set_fs(oldfs);
    }
    return ret;
}

int msys_kfile_read(struct file* fp, unsigned long long offset, unsigned char* data, unsigned int size)
{
    mm_segment_t oldfs;
    int ret;

    oldfs = get_fs();
    set_fs(get_ds());

    ret = vfs_read(fp, data, size, &offset);

    set_fs(oldfs);
    return ret;
}

#if defined(CONFIG_MS_MOVE_DMA)
void msys_mdma_done(u32 argu)
{
    CamOsTsemUp(&m_stMdmaDoneSem);
}

int msys_dma_blit(MSYS_DMA_BLIT *pstMdmaCfg)
{
    HalMoveDmaParam_t       tMoveDmaParam;
    HalMoveDmaLineOfst_t    tMoveDmaLineOfst;

    tMoveDmaParam.u32SrcAddr    = pstMdmaCfg->phyaddr_src;
    tMoveDmaParam.u32SrcMiuSel  = (pstMdmaCfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (0) : (1);
    tMoveDmaParam.u32DstAddr    = pstMdmaCfg->phyaddr_dst;
    tMoveDmaParam.u32DstMiuSel  = (pstMdmaCfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? (0) : (1);
    tMoveDmaParam.u32Count      = pstMdmaCfg->length;
    tMoveDmaParam.CallBackFunc  = msys_mdma_done;
    tMoveDmaParam.CallBackArg   = 0;

    if (pstMdmaCfg->lineofst_src && pstMdmaCfg->lineofst_dst) {
        tMoveDmaLineOfst.u32SrcWidth    = pstMdmaCfg->width_src;
        tMoveDmaLineOfst.u32SrcOffset   = pstMdmaCfg->lineofst_src;
        tMoveDmaLineOfst.u32DstWidth    = pstMdmaCfg->width_dst;
        tMoveDmaLineOfst.u32DstOffset   = pstMdmaCfg->lineofst_dst;

        tMoveDmaParam.bEnLineOfst       = 1;
        tMoveDmaParam.pstLineOfst       = &tMoveDmaLineOfst;
    }
    else {
        tMoveDmaParam.bEnLineOfst       = 0;
        tMoveDmaParam.pstLineOfst       = NULL;
    }

    if (HAL_MOVEDMA_NO_ERR != HalMoveDma_MoveData(&tMoveDmaParam)) {
        return -1;
    }

    CamOsTsemDownInterruptible(&m_stMdmaDoneSem);

    return 0;
}
EXPORT_SYMBOL(msys_dma_blit);
#endif
#if defined(CONFIG_MS_BDMA)
static void msys_bdma_done(u32 u32DmaCh)
{
    CamOsTsemUp(&m_stBdmaDoneSem[u32DmaCh]);
}

int msys_dma_fill(MSYS_DMA_FILL *pstDmaCfg)
{
    HalBdmaParam_t  tBdmaParam;
    u8              u8DmaCh = HAL_BDMA_CH1;
    tBdmaParam.ePathSel     = (pstDmaCfg->phyaddr < ARM_MIU1_BASE_ADDR) ? (HAL_BDMA_MEM_TO_MIU0) : (HAL_BDMA_MEM_TO_MIU1);
    tBdmaParam.bIntMode     = 1;
    tBdmaParam.eDstAddrMode = HAL_BDMA_ADDR_INC;
    tBdmaParam.u32TxCount   = pstDmaCfg->length;
    tBdmaParam.pSrcAddr     = (void*)0;
    tBdmaParam.pDstAddr     = (pstDmaCfg->phyaddr < ARM_MIU1_BASE_ADDR) ? (void *)((U32)pstDmaCfg->phyaddr) : (void *)((U32)pstDmaCfg->phyaddr - ARM_MIU1_BASE_ADDR);
    tBdmaParam.pfTxCbFunc   = msys_bdma_done;
    tBdmaParam.u32Pattern   = pstDmaCfg->pattern;

    if (HAL_BDMA_PROC_DONE != HalBdma_Transfer(u8DmaCh, &tBdmaParam)) {
        return -1;
    }

    if (tBdmaParam.bIntMode) {
        CamOsTsemDownInterruptible(&m_stBdmaDoneSem[u8DmaCh]);
    }

    return 0;
}
EXPORT_SYMBOL(msys_dma_fill);

int  msys_dma_copy(MSYS_DMA_COPY *cfg)
{
    HalBdmaParam_t  tBdmaParam;
    u8              u8DmaCh = HAL_BDMA_CH2;
    tBdmaParam.ePathSel     = ((U32)cfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (HAL_BDMA_MIU0_TO_MIU0) : (HAL_BDMA_MIU1_TO_MIU0);
    tBdmaParam.ePathSel     = ((U32)cfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? tBdmaParam.ePathSel : tBdmaParam.ePathSel+1;
    tBdmaParam.pSrcAddr     = ((U32)cfg->phyaddr_src < ARM_MIU1_BASE_ADDR) ? (void *)((U32)cfg->phyaddr_src) : (void *)((U32)cfg->phyaddr_src - ARM_MIU1_BASE_ADDR);
    tBdmaParam.pDstAddr     = ((U32)cfg->phyaddr_dst < ARM_MIU1_BASE_ADDR) ? (void *)((U32)cfg->phyaddr_dst) : (void *)((U32)cfg->phyaddr_dst - ARM_MIU1_BASE_ADDR);
    tBdmaParam.bIntMode     = 1;
    tBdmaParam.eDstAddrMode = HAL_BDMA_ADDR_INC;
    tBdmaParam.u32TxCount   = cfg->length;
    tBdmaParam.pfTxCbFunc   = msys_bdma_done;
    tBdmaParam.u32Pattern   = 0;

    if (HAL_BDMA_PROC_DONE != HalBdma_Transfer(u8DmaCh, &tBdmaParam)) {
        return -1;
    }

    if (tBdmaParam.bIntMode) {
        CamOsTsemDownInterruptible(&m_stBdmaDoneSem[u8DmaCh]);
    }

    return 0;
}
EXPORT_SYMBOL(msys_dma_copy);

#endif

int ssys_get_HZ(void)
{
    return HZ;
}
EXPORT_SYMBOL(ssys_get_HZ);


subsys_initcall(msys_init);

EXPORT_SYMBOL(msys_user_to_physical);
EXPORT_SYMBOL(msys_request_dmem);
EXPORT_SYMBOL(msys_release_dmem);
EXPORT_SYMBOL(msys_fix_dmem);
EXPORT_SYMBOL(msys_unfix_dmem);
EXPORT_SYMBOL(msys_find_dmem_by_phys);
EXPORT_SYMBOL(msys_get_proc_class);
EXPORT_SYMBOL(msys_get_sysfs_class);
EXPORT_SYMBOL(msys_kfile_open);
EXPORT_SYMBOL(msys_kfile_write);
EXPORT_SYMBOL(msys_kfile_read);
EXPORT_SYMBOL(msys_kfile_close);


MODULE_AUTHOR("SSTAR");
MODULE_DESCRIPTION("SYSTEM driver");
MODULE_LICENSE("SSTAR");
