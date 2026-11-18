/**************************************************************************************************
 *
 * Copyright (c) 2019-2024 Axera Semiconductor Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor Co., Ltd.
 *
 **************************************************************************************************/

#ifndef __OSAL_DEV_AX__H__
#define __OSAL_DEV_AX__H__

#ifdef __cplusplus
extern "C" {
#endif
#include "osal_type_ax.h"
#include "osal_ax.h"

//device
#define AX_OSAL_L_PTE_VALID     0
#define AX_OSAL_L_PTE_PRESENT   1
#define AX_OSAL_L_PTE_YOUNG     2
#define AX_OSAL_L_PTE_DIRTY     3

#define AX_OSAL_L_PTE_MT_DEV_SHARED     4
#define AX_OSAL_L_PTE_MT_DEV_NONSHARED  5
#define AX_OSAL_L_PTE_MT_DEV_WC         6
#define AX_OSAL_L_PTE_MT_DEV_CACHED     7

#define AX_OSAL_SEEK_SET 0
#define AX_OSAL_SEEK_CUR 1
#define AX_OSAL_SEEK_END 2

#define AXERA_RESOURCE_NAME 32

enum AX_DEVFREQ_TIMER {
        AX_DEVFREQ_TIMER_DEFERRABLE = 0,
        AX_DEVFREQ_TIMER_DELAYED,
        AX_DEVFREQ_TIMER_NUM,
};

struct AX_DEVFREQ_DEV_STATUS {
        /* both since the last measure */
        unsigned long total_time;
        unsigned long busy_time;
        unsigned long current_frequency;
        void *private_data;
};


struct AX_DEVFREQ_DEV_PROFILE {
        unsigned long initial_freq;
        int polling_ms;
        enum AX_DEVFREQ_TIMER timer;
        bool is_cooling_device;

        int (*target)(void *dev,unsigned long rate,unsigned long volt);
        int (*get_dev_status)(void *dev,
                              struct AX_DEVFREQ_DEV_STATUS *stat);
        unsigned long (*get_cur_freq)(void *dev);
        void (*exit)(void *dev);

        unsigned long *freq_table;
        int max_state;
};


struct AXERA_RESOURCE {
    u64 start;
    u64 end;
    unsigned char name[AXERA_RESOURCE_NAME];
    u64 flags;
    u64 desc;
};

struct AX_OF_DEVICE_ID  {
    unsigned char    name[32];
    unsigned char    type[32];
    unsigned char    compatible[128];
    const void *data;
};

struct AX_PLATFORM_DEVICE_DRIVER {
    const unsigned char      *name;
#if defined(CONFIG_OF)
    const void   *of_match_table;
#endif
};


struct AX_PLATFORM_DRIVER {
    int (*probe)(void *);
    int (*remove)(void *);
    void (*shutdown)(void *);
    int (*suspend)(void *);
    int (*resume)(void *);
    int (*suspend_noirq)(void *);
    int (*resume_early)(void *);
    int (*suspend_late)(void *);
    int (*resume_noirq)(void *);

    struct AX_PLATFORM_DEVICE_DRIVER driver;
    void *axera_ptr;
};


typedef struct AX_POLL {
    void *poll_table;
    void *data;
    void *wait; /*AX_WAIT, only support one poll, read or write*/
} AX_POLL_T;

typedef struct AX_DEV {
    char name[48];
    void *dev;
    int minor;
    struct AX_FILEOPS *fops;
    struct AX_PMOPS *osal_pmops;
    void *private_data;
    struct AX_POLL dev_poll;
    struct AX_WAIT dev_wait;
} AX_DEV_T;

typedef struct AX_VM {
    void *vm;
} AX_VM_T;

typedef struct AX_DEV_PRIVATE_DATA {
    struct AX_DEV *dev;
    void *data;
    struct AX_POLL table;
    int f_ref_cnt;
    unsigned int f_flags;		//todo: this parameter will be deleted after the driver modification is completed
    struct file *file;
} AX_DEV_PRIVATE_DATA_T;

#define AX_OSAL_DEV_PRI_DATA(pri_data) ((( struct AX_DEV_PRIVATE_DATA  *)pri_data)->data)

typedef struct AX_FILEOPS {
    int(*open)(void *private_data);
    int(*read)(char *buf, int size, long *offset, void *private_data);
    int(*write)(const char *buf, int size, long *offset, void *private_data);
    long(*llseek)(long offset, int whence, void *private_data);
    int(*release)(void *private_data);
    long(*unlocked_ioctl)(unsigned int cmd, unsigned long arg, void *private_data);
    unsigned int(*poll)(AX_POLL_T *osal_poll, void *private_data);

    /*******RTT not support mmap*************/
    int (*mmap) (AX_VM_T *vm, unsigned long start, unsigned long end, unsigned long vm_pgoff, void *private_data);
} AX_FILEOPS_T;

typedef struct AX_PMOPS {
    int(*pm_suspend)(AX_DEV_T *dev);
    int(*pm_resume)(AX_DEV_T *dev);
    int(*pm_suspend_late)(AX_DEV_T *dev);
    int(*pm_resume_early)(AX_DEV_T *dev);

    /*******RTT not support the other pm ops*************/
    int (*pm_prepare)(AX_DEV_T *dev);
    void (*pm_complete)(AX_DEV_T *dev);
    int (*pm_freeze)(AX_DEV_T *dev);
    int (*pm_thaw)(AX_DEV_T *dev);
    int (*pm_poweroff)(AX_DEV_T *dev);
    int (*pm_restore)(AX_DEV_T *dev);
    int (*pm_freeze_late)(AX_DEV_T *dev);
    int (*pm_thaw_early)(AX_DEV_T *dev);
    int (*pm_poweroff_late)(AX_DEV_T *dev);
    int (*pm_restore_early)(AX_DEV_T *dev);
    int (*pm_suspend_noirq)(AX_DEV_T *dev);
    int (*pm_resume_noirq)(AX_DEV_T *dev);
    int (*pm_freeze_noirq)(AX_DEV_T *dev);
    int (*pm_thaw_noirq)(AX_DEV_T *dev);
    int (*pm_poweroff_noirq)(AX_DEV_T *dev);
    int (*pm_restore_noirq)(AX_DEV_T *dev);
} AX_OSAL_PMOPS_T;

AX_DEV_T *AX_OSAL_DEV_createdev(char *name);
int AX_OSAL_DEV_destroydev(AX_DEV_T *ax_dev);
int AX_OSAL_DEV_device_register(AX_DEV_T *ax_dev);
void AX_OSAL_DEV_device_unregister(AX_DEV_T *ax_dev);
//only for linux kernel
void AX_OSAL_DEV_poll_wait(AX_POLL_T *table, AX_WAIT_T *wait);
//only for linux kernel
void AX_OSAL_DEV_pgprot_noncached(AX_VM_T *vm);
//only for linux kernel
void AX_OSAL_DEV_pgprot_cached(AX_VM_T *vm);
//only for linux kernel
void AX_OSAL_DEV_pgprot_writecombine(AX_VM_T *vm);
//only for linux kernel
void AX_OSAL_DEV_pgprot_stronglyordered(AX_VM_T *vm);
//only for linux kernel
int AX_OSAL_DEV_remap_pfn_range(AX_VM_T *vm, unsigned long addr, unsigned long pfn, unsigned long size);
//only for linux kernel
int AX_OSAL_DEV_io_remap_pfn_range(AX_VM_T *vm, unsigned long addr, unsigned long pfn, unsigned long size);
//only for linux kernel
void *AX_OSAL_DEV_to_dev(AX_DEV_T *ax_dev);

//addr translate
//only for linux kernel
void *AX_OSAL_DEV_ioremap(unsigned long phys_addr, unsigned long size);
//only for linux kernel
void *AX_OSAL_DEV_ioremap_nocache(unsigned long phys_addr, unsigned long size);
//only for linux kernel
void *AX_OSAL_DEV_ioremap_cache(unsigned long phys_addr, unsigned long size);
//only for linux kernel
void AX_OSAL_DEV_iounmap(void *addr);
unsigned long AX_OSAL_DEV_copy_from_user(void *to, const void *from, unsigned long n);
unsigned long AX_OSAL_DEV_copy_to_user(void *to, const void *from, unsigned long n);

unsigned long AX_OSAL_DEV_usr_virt_to_phys(unsigned long virt);
unsigned long AX_OSAL_DEV_kernel_virt_to_phys(void *virt);
//cache api
void AX_OSAL_DEV_invalidate_dcache_area(void  *addr, int size);
void AX_OSAL_DEV_flush_dcache_area(void *kvirt, unsigned long length);
//only for linux kernel
void AX_OSAL_DEV_flush_dcache_all(void);

void AX_OSAL_DEV_outer_dcache_area(u64 phys_addr_start, u64 phys_addr_end);

//interrupt api
typedef int(*AX_IRQ_HANDLER_T)(int, void *);
int AX_OSAL_DEV_request_threaded_irq(unsigned int irq, AX_IRQ_HANDLER_T handler, AX_IRQ_HANDLER_T thread_fn,
                                        const char *name, void *dev);
int AX_OSAL_DEV_request_threaded_irq_ex(unsigned int irq, AX_IRQ_HANDLER_T handler, AX_IRQ_HANDLER_T thread_fn,
                                        unsigned long flags, const char *name, void *dev);

const void *AX_OSAL_DEV_free_irq(unsigned int irq, void *dev);
int AX_OSAL_DEV_in_interrupt(void);

void AX_OSAL_DEV_enable_irq(unsigned int irq);
void AX_OSAL_DEV_disable_irq(unsigned int irq);
void AX_OSAL_DEV_disable_irq_nosync(unsigned int irq);

enum AX_OSAL_irqchip_irq_state {
	AX_OSAL_IRQCHIP_STATE_PENDING,		/* Is interrupt pending? */
	AX_OSAL_IRQCHIP_STATE_ACTIVE,		/* Is interrupt in progress? */
	AX_OSAL_IRQCHIP_STATE_MASKED,		/* Is interrupt masked? */
	AX_OSAL_IRQCHIP_STATE_LINE_LEVEL,	/* Is IRQ line high? */
};

int AX_OSAL_DEV_irq_get_irqchip_state(unsigned int irq, enum AX_OSAL_irqchip_irq_state which,
			  int *state);

int AX_OSAL_DEV_irq_set_irqchip_state(unsigned int irq, enum AX_OSAL_irqchip_irq_state which,
			  int val);


/*device framework(platform) API*/
int AX_OSAL_DEV_platform_driver_register(void *drv);
void AX_OSAL_DEV_platform_driver_unregister(void *drv);
int AX_OSAL_DEV_platform_get_resource_byname(void *dev, unsigned int type, const char *name,struct AXERA_RESOURCE *res);
int AX_OSAL_DEV_platform_get_resource(void *dev, unsigned int type, unsigned int num,struct AXERA_RESOURCE *res);
int AX_OSAL_DEV_platform_get_irq(void *dev, unsigned int num);
int AX_OSAL_DEV_platform_get_irq_byname(void *dev, const char *name);
unsigned long AX_OSAL_DEV_resource_size(const struct AXERA_RESOURCE *res);
void *AX_OSAL_DEV_platform_get_drvdata(void *pdev);
void AX_OSAL_DEV_platform_set_drvdata(void *pdev,void *data);
int AX_OSAL_DEV_platform_irq_count(void *dev);
void  *AX_OSAL_DEV_to_platform_device(void * dev);
void  *AX_OSAL_DEV_to_platform_driver(void * drv);

int AX_OSAL_DEV_of_property_read_string(void *pdev,const char *propname,const char **out_string);
#if 0
const void *AX_OSAL_DEV_of_get_property(void *pdev, const char *name,int *lenp);
int AX_OSAL_DEV_of_property_read_string_array(void *pdev,const char *propname,const char **out_strs,u64 sz);
int AX_OSAL_DEV_of_property_count_strings(void *pdev,const char *propname);
int AX_OSAL_DEV_of_property_read_string_index(void *pdev,const char *propname,int index, const char **output);
#endif
bool AX_OSAL_DEV_of_property_read_bool(void *pdev,const char *propname);
#if 0
int AX_OSAL_DEV_of_property_read_u8(void *pdev,const char *propname,char *out_value);
int AX_OSAL_DEV_of_property_read_u16(void *pdev,const char *propname,AX_U16 *out_value);
#endif
int AX_OSAL_DEV_of_property_read_u32(void *pdev,const char *propname,unsigned int *out_value);
int AX_OSAL_DEV_of_property_read_s32(void *pdev,const char *propname,int *out_value);



void *AX_OSAL_DEV_devm_clk_get(void *pdev, const char *id);
void AX_OSAL_DEV_devm_clk_put(void *pdev,void *pclk);
void AX_OSAL_DEV_clk_disable(void *pclk);
int AX_OSAL_DEV_clk_enable(void *pclk);
bool AX_OSAL_DEV_clk_is_enabled(void * pclk);
int AX_OSAL_DEV_clk_prepare_enable(void *pclk);
int AX_OSAL_DEV_clk_set_rate(void *pclk, unsigned long rate);
unsigned long AX_OSAL_DEV_clk_get_rate(void *pclk);
void AX_OSAL_DEV_clk_disable_unprepare(void *pclk);


int AX_OSAL_DEV_i2c_write(unsigned char i2c_dev, unsigned char dev_addr,
                         unsigned int reg_addr, unsigned int reg_addr_num,
                         unsigned int data, unsigned int data_byte_num);

int AX_OSAL_DEV_i2c_read(unsigned char i2c_dev, unsigned char dev_addr,
                        unsigned int reg_addr, unsigned int reg_addr_num,
                        unsigned int *pRegData, unsigned int data_byte_num);

int AX_OSAL_DEV_i2c_dev_init(void);
void AX_OSAL_DEV_i2c_dev_exit(void);

void *AX_OSAL_DEV_devm_reset_control_get_optional(void *pdev, const char *id, int flag);
int  AX_OSAL_DEV_reset_control_assert(void *rstc);
int  AX_OSAL_DEV_reset_control_deassert(void *rstc);

#include <linux/soc/axera/ax_bw_limiter.h>
int AX_OSAL_DEV_bwlimiter_register_with_clk(unsigned int sub_sys_bw, void *pclk);
int AX_OSAL_DEV_bwlimiter_register_with_val(unsigned int sub_sys_bw, unsigned int clk);
int AX_OSAL_DEV_bwlimiter_unregister(unsigned int sub_sys_bw, void *pclk);
int AX_OSAL_DEV_bwlimiter_refresh_limiter(unsigned int sub_sys_bw);

int AX_OSAL_DEV_pm_opp_of_add_table(void *pdev);
void AX_OSAL_DEV_pm_opp_of_remove_table(void *pdev);
int AX_OSAL_DEV_devm_devfreq_add_device(void *pdev,struct AX_DEVFREQ_DEV_PROFILE *ax_profile,const char *governor_name,void *data);
int AX_OSAL_DEV_pm_opp_of_disable(void * pdev, unsigned long freq);
void AX_OSAL_DEV_pm_opp_of_remove(void * pdev, unsigned long freq);
int AX_OSAL_DEV_pm_opp_of_add(void * pdev, unsigned long freq, unsigned long volt);

#ifdef __cplusplus
}
#endif

#endif /*__OSAL_DEV_AX__H__*/
