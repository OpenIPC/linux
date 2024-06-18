/*
 * Revision History
 * VER1.1 2013/11/27 prevent CVBS jitter for read saradc REG 2 in gm8139.c
 * VER1.2 2013/12/11 printk error message add module name sar_adc_drv.c
 * VER1.3 2013/12/13 support runing mode for 8139(gm8139.c,sar_adc_drv.c,sar_adc_drv.h)
 * VER1.4 2014/05/19 add low and high int limit for gain0
 * VER1.5 2014/06/18 filter gain0 vlaue is zero case;
 * VER1.6 2014/08/05 prevent interrupt happen in init state and add 8136 support
 * VER1.7 2014/08/28 add proc and debug for 8136 and 8139
 * VER1.8 2014/09/09 REMOVE open adda clock in 8139 and 8136
 * VER1.9 2.0 15/04/01 change saradc state machine xgin0 as polling mode and
 *                     set 0x00 saradc reg don't set 0~2 bit and set eoc drop =1 
 * VER2.1 15/09/30 change saradc hw polling mode
  * VER2.2 15/10/12 adjust hw auto interval , gain2 as 0x1(one frame to request)
  * VER2.3 15/10/12 adjust hw auto interval , let sw to switch enable bit.
  * VER2.4.1 16/07/15     
    (1) get value to wakeup kfifo that info app get quickly.
  * VER2.5/08/04     
    (1) set sample clk as 0x09000000
    (2) set run_mode default is channel 0
    (3) When CVBS get 0 for 10 times , will do recovery.
 */

#include "platform.h"
#include "sar_adc_drv.h"
#include "sar_adc_dev.h"
#include "sar_adc_api.h"
#include <linux/delay.h>
#include <asm-generic/gpio.h>
#include <linux/proc_fs.h>

#if (HZ==1000)
#define DEFAULT_SCAN_DURATION       80
#define DEFAULT_REPEAT_DURATION     300
#elif (HZ==100)
#define DEFAULT_SCAN_DURATION       8
#define DEFAULT_REPEAT_DURATION     30
#else
#define DEFAULT_SCAN_DURATION       80
#define DEFAULT_REPEAT_DURATION     300
#endif

#define DEFAULT_CLOCK_DIVIDER       120


/* module parameter */
static unsigned int clk_div = DEFAULT_CLOCK_DIVIDER;
static int g_xgain_num = 0; 
module_param(clk_div, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(clk_div, "clock divider (base freq. is 12MHz)");

static unsigned int run_mode = 0;
module_param(run_mode, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(run_mode, "SARADC run_mode");

unsigned int poll_mode = 0;

module_param(poll_mode, uint, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(poll_mode, "polling mode 0:HW 1:SW");

static unsigned short gain0_lim[2] = {[0 ... (2 - 1)] = 0};
module_param_array(gain0_lim, ushort, NULL, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(gain0_lim, "gain0 min and max interrupt value");

 unsigned short auto_interval[3] = {0x32,0x64,0x1};
	
module_param_array(auto_interval, ushort, NULL, S_IRUGO | S_IWUSR);
	
MODULE_PARM_DESC(auto_interval, "auto request interval");
static unsigned short gain0_int = 0;
module_param(gain0_int, ushort, S_IRUGO|S_IWUSR);
MODULE_PARM_DESC(gain0_int, "SARADC gain0 int mode,default is 0");

/* function for register device */
void device_release(struct device *dev);

/* function for register driver */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
int driver_probe(struct platform_device * pdev);
int __devexit driver_remove(struct platform_device *pdev);
#else
int driver_probe(struct device * dev);
int driver_remove(struct device * dev);
#endif

#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)

//int scu_remove(struct scu_t *p_scu);
/* proc init.
 */
static struct proc_dir_entry *saradc_proc_root = NULL;
static struct proc_dir_entry *saradc_cvbs_info = NULL;
static struct proc_dir_entry *saradc_gain_dbg = NULL;
static struct proc_dir_entry *saradc_force_cvbs = NULL;
static struct proc_dir_entry *saradc_cvbs_trs = NULL;
static struct proc_dir_entry *saradc_polling_tms = NULL;
#endif

u32	g_recovery_cnt;
/* function for register file operation */
static int file_open(struct inode *inode, struct file *filp);
static int file_release(struct inode *inode, struct file *filp);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static int file_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg);
#else
static long file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);
#endif
static ssize_t file_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
static unsigned int file_poll(struct file *filp, poll_table *wait);

static struct resource _dev_resource[] = {
	[0] = {
		.start = DEV_PA_START,
		.end = DEV_PA_END,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DEV_IRQ_START,
		.end = DEV_IRQ_END,
		.flags = IORESOURCE_IRQ,
	},
};

static struct platform_device _device = {
	.name = DEV_NAME,
	.id = -1, /* "-1" to indicate there's only one. */
	#ifdef HARDWARE_IS_ON
	.num_resources = ARRAY_SIZE(_dev_resource),
	.resource = _dev_resource,
	#endif
	.dev  = {
		.release = device_release,
	},
};

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
/*
 * platform_driver
 */
static struct platform_driver _driver = {
    .probe = driver_probe,
    .remove = driver_remove,
    .driver = {
           .owner = THIS_MODULE,
           .name = DEV_NAME,
           .bus = &platform_bus_type, /* verify on GM8210 */
    },
};
#else
/*
 * device_driver
 */
static struct device_driver _driver = {
    .owner = THIS_MODULE,
    .name = DEV_NAME,
    .bus = &platform_bus_type,      /* used to represent busses like PCI, USB, I2C, etc */
    .probe = driver_probe,
    .remove = driver_remove,
};
#endif

static struct file_operations _fops = {
	.owner 			= THIS_MODULE,
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
	.ioctl 			= file_ioctl,
	#else
    .unlocked_ioctl = file_ioctl,
    #endif
	.open 			= file_open,
	.release 		= file_release,
	.read			= file_read,
    .poll           = file_poll,
};


void device_release(struct device *dev)
{
    PRINT_FUNC();
    return;
}


int register_device(struct platform_device *p_device)
{
    int ret = 0;

    if (unlikely((ret = platform_device_register(p_device)) < 0)) 
    {
        printk("%s fails: platform_device_register not OK\n", __FUNCTION__);
    }
    
    return ret;    
}

void unregister_device(struct platform_device *p_device)
{
    
    platform_device_unregister(p_device);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
int register_driver(struct platform_driver *p_driver)
{
    PRINT_FUNC();
	return platform_driver_register(p_driver);
}
#else
int register_driver(struct device_driver* p_driver)
{
	return driver_register(p_driver);
}
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
void unregister_driver(struct platform_driver *p_driver)
{
    platform_driver_unregister(p_driver);
}
#else
void ft2dge_unregister_driver(struct device_driver *p_driver)
{
    driver_unregister(p_driver);	
}
#endif

/*
 * register_cdev
 */ 
int register_cdev(struct dev_data* p_dev_data)
{
    int ret = 0;

    /* alloc chrdev */
    ret = alloc_chrdev_region(&p_dev_data->dev_num, 0, DEV_COUNT, DEV_NAME);
    if (unlikely(ret < 0)) {
        printk(KERN_ERR "%s:alloc_chrdev_region failed\n", __func__);
        goto err1;
    }
    
    cdev_init(&p_dev_data->cdev, &_fops);
    p_dev_data->cdev.owner = THIS_MODULE;

    ret = cdev_add(&p_dev_data->cdev, p_dev_data->dev_num, DEV_COUNT);
    if (unlikely(ret < 0)) {
        PRINT_E(KERN_ERR "%s:cdev_add failed\n", __func__);
        goto err2;
    }

    /* create class */
    p_dev_data->class = class_create(THIS_MODULE, CLS_NAME);
	if (IS_ERR(p_dev_data->class))
	{
        PRINT_E(KERN_ERR "%s:class_create failed\n", __func__);
        goto err3;
	}

    
    /* create a node in /dev */
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
	p_dev_data->p_device = device_create(
        p_dev_data->class,              /* struct class *class */
        NULL,                           /* struct device *parent */
        p_dev_data->cdev.dev,    /* dev_t devt */
	    p_dev_data,                     /* void *drvdata, the same as platform_set_drvdata */
	    DEV_NAME                 /* const char *fmt */
	);
    #else
	class_device_create(
	    p_dev_data->class, 
	    p_dev_data->cdev.dev,
	    NULL, DEV_NAME
	);
    #endif
    
    PRINT_FUNC();
    return 0;
    
    err3:
        cdev_del(&p_dev_data->cdev);
    
    err2:
        unregister_chrdev_region(p_dev_data->dev_num, 1);
        
    err1:
        return ret;
}


/*
 * unregister_cdev
 */ 
void unregister_cdev(struct dev_data* p_dev_data)
{
    PRINT_FUNC();

    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,26))
    device_destroy(p_dev_data->class, p_dev_data->cdev.dev);
    #else
    class_device_destroy(p_dev_data->class, p_dev_data->cdev.dev);
    #endif
    
    class_destroy(p_dev_data->class);
    
    cdev_del(&p_dev_data->cdev);
    
    unregister_chrdev_region(p_dev_data->dev_num, 1);
    
}

static void process_key_adc(struct work_struct *work)
{
    struct delayed_work *delay_work = container_of(work, struct delayed_work, work);
    struct dev_specific_data_t* p_data  = container_of(delay_work, struct dev_specific_data_t, key_work);
    struct dev_data* p_dev_data = container_of(p_data, struct dev_data, dev_specific_data);
    sar_adc_pub_data new_data;
    int len = 0;
	unsigned int 		vbase;

	vbase = (unsigned int)p_dev_data->io_vadr;

   Platform_SAR_ADC_GetData(p_dev_data,&new_data);


	if(new_data.adc_val == 0){        
		return ;
    }

    Platform_Lock_Polling_Mutex();

    len = kfifo_in(
		&p_data->fifo, 
		&new_data,
		sizeof(sar_adc_pub_data)
		);
    
    Platform_UnLock_Polling_Mutex();

	 wake_up(&p_data->wait_queue);
    if(len < sizeof(sar_adc_pub_data)){
        if(Platform_Get_SARADC_Debug())
            printk("SAR_ADC : put data into fifo queue fail \n");
    }
}


static void* dev_data_alloc_specific(struct dev_data* p_dev_data)
{
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;

    spin_lock_init(&p_data->lock);

    p_data->scan_duration = DEFAULT_SCAN_DURATION;
	/* setting fifo */
    p_data->queue_len = 16;
    if (unlikely(kfifo_alloc(
		&p_data->fifo, 
		p_data->queue_len * sizeof(sar_adc_pub_data), 
		GFP_KERNEL))
	)
	{
		panic("SADARC : kfifo_alloc fail in %s\n", __func__);
        goto err0;
	}
    kfifo_reset(&p_data->fifo);
    init_waitqueue_head(&p_data->wait_queue);
	
	/* setting work and workqueue */
    INIT_DELAYED_WORK(&p_data->key_work, process_key_adc);	
    init_MUTEX(&p_data->oper_sem);
	printk(" * dev_data_alloc_specific done\n");
    return p_data;


    err0:
    return NULL;
}

static void* dev_data_alloc(void)
{
    struct dev_data* p_dev_data = NULL;
        
    /* alloc drvdata */
    p_dev_data = kzalloc(sizeof(struct dev_data), GFP_KERNEL);

    if (unlikely(p_dev_data == NULL))
    {
        PRINT_E("%s Failed to allocate p_dev_data\n", __FUNCTION__);        
        goto err0;
    }

	if (unlikely(dev_data_alloc_specific(p_dev_data) == NULL))
    {
        PRINT_E("%s Failed to allocate p_dev_data\n", __FUNCTION__);        
        goto err1;
    }

   	DRV_COUNT_RESET();
	return p_dev_data;
	
	err1:
	    kfree (p_dev_data);
		p_dev_data = NULL;
    err0:
        return p_dev_data;
}

static int dev_data_free_specific(struct dev_data* p_dev_data)
{
    struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;
    kfifo_free(&p_data->fifo);
    return 0;
}


static int dev_data_free(struct dev_data* p_dev_data)
{      
    dev_data_free_specific(p_dev_data);
    /* free drvdata */
    kfree(p_dev_data);
    p_dev_data = NULL;
    
	return 0;
}

static irqreturn_t drv_interrupt(int irq, void *base)
{
	unsigned int 		vbase;
    struct dev_data*	p_dev_data = (struct dev_data*)base;
    struct dev_specific_data_t*	p_data = &p_dev_data->dev_specific_data;
    sar_adc_pub_data new_data;
    int delay = 5 ,repeat_duration = 0;

    if(HZ == 1000)
        delay = 5;
    else if(HZ == 100)
        delay = 1;
    else
        delay = 5;

	vbase = (unsigned int)p_dev_data->io_vadr;

    SAR_ADC_ClearInt(vbase);

    repeat_duration = p_data->scan_duration;
    
    if(jiffies_diff(jiffies, p_data->last_scan_jiffies) < repeat_duration){
    	new_data.status = KEY_REPEAT;
        return IRQ_HANDLED;       
    }
    else
        new_data.status = KEY_IN;

    p_data->last_scan_jiffies = jiffies;
    /*prevent saradc semaphore isn't ok*/
	if(Platform_Get_Polling_Timer_State())
        schedule_delayed_work(&p_data->key_work, delay);

    return IRQ_HANDLED;
}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,19))
irqreturn_t drv_isr(int irq, void *dev)
#else
irqreturn_t drv_isr(int irq, void *dev, struct pt_regs *dummy)
#endif
{

	return 	drv_interrupt(irq, dev);
}

int init_hardware(struct dev_data *p_dev_data)
{
	unsigned int base = (unsigned int)p_dev_data->io_vadr;
    int ret = -1;

    //assing running mode 
    run_mode = (run_mode&SAR_ADC_RUNING_MODE_ALL);

    if(!run_mode)
        p_dev_data->running_mode = SAR_ADC_RUNING_MODE_GAIN0;
    else
        p_dev_data->running_mode = run_mode;

    p_dev_data->dev_specific_data.xgain0_min = gain0_lim[0];
    p_dev_data->dev_specific_data.xgain0_max = gain0_lim[1];
	p_dev_data->dev_specific_data.xgain0_int = gain0_int;
    //setup hardware    
    ret = scu_probe(&p_dev_data->scu);
    if (unlikely(ret != 0)) 
    {
        PRINT_E("%s fails at scu_probe %d\n", __FUNCTION__, ret);
        goto err0;
    }
	
	set_pinmux();

	Platform_Set_Init_Reg(p_dev_data);
	ret = Platform_Init_Pollign_Timer(p_dev_data);
    if (unlikely(ret != 0)) 
    {
        PRINT_E("%s fails at Platform_Init_Pollign_Timer %d\n", __FUNCTION__, ret);
        goto err1;
    }
    
    SAR_ADC_ClearInt(base);

	return 0;
    
    err1:
        scu_remove(&p_dev_data->scu);

	err0:
        
    return ret;
}

void exit_hardware(struct dev_data *p_dev_data)
{

    PRINT_FUNC();
	
	/****************************************************/
	/**** start to implement your exit_hardware here ****/
	/****************************************************/
	
	/* free gpio */

    /* remove pmu */
    scu_remove(&p_dev_data->scu);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
int driver_probe(struct platform_device * pdev)
#else
int driver_probe(struct device * dev)
#endif
{
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
    struct platform_device* pdev = to_platform_device(dev); // for version under 2.6.24 
    #endif
    struct dev_data *p_dev_data = NULL;
    #ifdef HARDWARE_IS_ON
    struct resource *res = NULL;  
    #endif
    struct dev_specific_data_t *p_data = NULL;
  
    int ret = 0;
    PRINT_FUNC();
    
    /* 1. alloc and init device driver_Data */
    p_dev_data = dev_data_alloc();
    if (unlikely(p_dev_data == NULL)) 
    {
        printk("%s fails: kzalloc not OK", __FUNCTION__);
        goto err1;
    }
    p_data = &p_dev_data->dev_specific_data;
	
    /* 2. set device_drirver_data */
    #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
    platform_set_drvdata(pdev, p_dev_data);
    #else
    dev_set_drvdata(dev, p_dev_data);
    #endif

    p_dev_data->id = pdev->id;    
   
    /* 3. request resource of base address */
    #ifdef HARDWARE_IS_ON
    res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

    if (unlikely((!res))) 
    {
        PRINT_E("%s fails: platform_get_resource not OK", __FUNCTION__);
        goto err2;
    }

    p_dev_data->io_size = res->end - res->start + 1;
    p_dev_data->io_padr = (void*)res->start;

    if (unlikely(!request_mem_region(res->start, p_dev_data->io_size, pdev->name))) {
        PRINT_E("%s fails: request_mem_region not OK", __FUNCTION__);
        goto err2;
    }

    p_dev_data->io_vadr = (void*) ioremap((uint32_t)p_dev_data->io_padr, p_dev_data->io_size);

    if (unlikely(p_dev_data->io_vadr == 0)) 
    {
        PRINT_E("%s fails: ioremap_nocache not OK", __FUNCTION__);
        goto err3;
    }

    /* 4. request_irq */
    p_dev_data->irq_no = platform_get_irq(pdev, 0);
    if (unlikely(p_dev_data->irq_no < 0)) 
    {
        PRINT_E("%s fails: platform_get_irq not OK", __FUNCTION__);
        goto err3;
    }
    ret = request_irq(
        p_dev_data->irq_no, 
        drv_isr, 
        #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,24))
        SA_INTERRUPT, 
        #else
        0,
        #endif
        DEV_NAME, 
        p_dev_data
    ); 

    if (unlikely(ret != 0)) 
    {
        PRINT_E("%s fails: request_irq not OK %d\n", __FUNCTION__, ret);
        goto err3;
    }
     
    /* 6. init hardware */
	ret = init_hardware(p_dev_data);
    if (unlikely(ret < 0)) 
    {
        PRINT_E("%s fails: init_hardware not OK\n", __FUNCTION__);
        goto err4;
    }
    #endif
    

    /* 7. register cdev */
    ret = register_cdev(p_dev_data);
    if (unlikely(ret < 0)) 
    {
        PRINT_E("%s fails: sar_adc_register_cdev not OK\n", __FUNCTION__);
        goto err5;
    }


    /* 8. print probe info */
    printk("%s done, io_vadr 0x%08X, io_padr 0x%08X 0x%08X\n", 
    __FUNCTION__, 
    (unsigned int)p_dev_data->io_vadr, 
    (unsigned int)p_dev_data->io_padr, 
    (unsigned int)p_dev_data
    );
    /*print runmod and gain0 min and max value*/
     printk("%s done,runmod:0x%x,0x%x,0x%x,0x%x,poll_mode=%x\n", 
    __FUNCTION__, 
    (unsigned int)run_mode,gain0_int,gain0_lim[0],gain0_lim[1],poll_mode);
    return ret;
    
    err5:
        scu_remove(&p_dev_data->scu);

    #ifdef HARDWARE_IS_ON
    err4:
        free_irq(p_dev_data->irq_no, p_dev_data);
        
    err3:
        release_mem_region((unsigned int)p_dev_data->io_padr, p_dev_data->io_size);

    err2:
        #if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
        platform_set_drvdata(pdev, NULL);
        #else
        dev_set_drvdata(dev, NULL);
        #endif
    #endif
        

    err1:
        return ret;
}
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
static int proc_dump_cvbs_info(char *page, char **start, off_t off, int count
                                , int *eof, void *data)
{
    int tmp_data;
    int len = 0;
    
    tmp_data = Platform_Get_CVBS_Info();

    if(tmp_data == 0)
        len += sprintf(page + len, "cvbs status: NONE\n");
    else if (tmp_data == 0x01)
        len += sprintf(page + len, "cvbs status: Plug in\n");
    else if(tmp_data == 0x02)
        len += sprintf(page + len, "cvbs status: Plug out\n");
    else
        len += sprintf(page + len, "cvbs status: Unknow\n");
        
    return len;
}

static int proc_dump_xgain_dbg(char *page, char **start, off_t off, int count
                                , int *eof, void *data)
{
   return sprintf(page, "xgain_dbg_flag =%d\n", Platform_Get_SARADC_Debug());
}

static int proc_set_xgain_dbg(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set = 0;
    sscanf(buffer, "%d", &mode_set);

    if(mode_set !=0 && mode_set !=1){
        printk("set xgain debug must be 1 or 0 (%d)\n",mode_set);
        return count;
    }

    Platform_Set_SARADC_Debug(mode_set);
    
    return count;
}

static int proc_dump_force_cvbs(char *page, char **start, off_t off, int count
                                , int *eof, void *data)
{
    int len = 0;

    len += sprintf(page + len, "force_cvbs state =%d\n", Platform_Force_Get_ADDA_TVBS());
    len += sprintf(page + len, "0:auto,1:plugin,2:plugout\n");

    return len;
}

static int proc_set_force_cvbs(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int mode_set = 0;
    sscanf(buffer, "%d", &mode_set);

    if(mode_set !=0 && mode_set !=1 && mode_set !=2){
        printk("0:auto,1:plugin,2:plugout (%d)\n",mode_set);
        return count;
    }

    Platform_Force_Set_ADDA_TVBS(mode_set);
    
    return count;
}

static int proc_dump_polling_tms(char *page, char **start, off_t off, int count
                                , int *eof, void *data)
{
    int len = 0; 

	 len += sprintf(page + len, "ver : %s\n",SARADC_VER); 
    len += sprintf(page + len, "Polling time: %d ms\n",Platform_Get_Polling_Time());   

	 len += sprintf(page + len, "Recovery cnt: %d \n",g_recovery_cnt); 
	 
    return len;
}

static int proc_set_polling_tms(struct file *file, const char *buffer,unsigned long count, void *data)
{
    
    u32 time_val = 0;
    
    sscanf(buffer, "%d", &time_val);

    if(time_val ==0){
        printk("can't set polling time as zero\n");
        return count;
    } 

    Platform_Set_Polling_Time(time_val);
    return count;
}

static int proc_dump_cvbs_tsr(char *page, char **start, off_t off, int count
                                , int *eof, void *data)
{
    int len = 0;
    int on_v = 0 ,off_v = 0;
    
    Platform_Get_CVBS_THR_Value(&on_v,&off_v);

    len += sprintf(page + len, "CVBS ON THR: %d\n",on_v);
    len += sprintf(page + len, "CVBS OFF THR: %d\n",off_v);
    len += sprintf(page + len, "CVBS MAX THR: %d\n",OUTPUT_VOL_THR_TOP);
    len += sprintf(page + len, "CVBS MIN THR: %d\n",OUTPUT_VOL_THR_DOWN);
    return len;
}

static int proc_set_cvbs_tsr(struct file *file, const char *buffer,unsigned long count, void *data)
{
    int len = count;
    unsigned char value[20];
    unsigned int on_val = 0,off_val = 0;

    memset(value,0x0,20);
    if(len >= 20-1 ){
        printk("set cvbs threshold fail (len(%d))\n",len);
        return count;
    }
            
    if(copy_from_user(value, buffer, len)){
        printk("set cvbs threshold fai(copy_from_user)\n");
        return count;
    }
    value[len] = '\0';
    sscanf(value, "%d %d\n",&on_val,&off_val);
       
    if(on_val <= OUTPUT_VOL_THR_DOWN || off_val <= OUTPUT_VOL_THR_DOWN 
       || off_val  >= OUTPUT_VOL_THR_TOP || on_val >= OUTPUT_VOL_THR_TOP){
        printk("set cvbs threshold fai(on(%d)off(%d))\n",on_val,off_val);
        return count; 
    }

    if(Platform_Set_CVBS_THR_Value(on_val,off_val) < 0)
        printk("Platform_Set_CVBS_THR_Value fail(on(%d)off(%d))\n",on_val,off_val);  
    
    return count;        

}


static int saradc_proc_init(void)
{
    struct proc_dir_entry *p;

    p = create_proc_entry("saradc", S_IFDIR | S_IRUGO | S_IXUGO, NULL);

	if (p == NULL) {
        panic("Fail to create proc saradc root!\n");
    }

    saradc_proc_root = p;

    /*
     * debug message
     */
    saradc_cvbs_info = create_proc_entry("cvbs_info", S_IRUGO, saradc_proc_root);

	if (saradc_cvbs_info == NULL)
        panic("Fail to create saradc_cvbs_info!\n");
    saradc_cvbs_info->read_proc = (read_proc_t *) proc_dump_cvbs_info;
    saradc_cvbs_info->write_proc = NULL;   

    saradc_gain_dbg = create_proc_entry("xgain_dbg", S_IRUGO, saradc_proc_root);
    
    if (saradc_gain_dbg == NULL)
        panic("Fail to create saradc_gain_dbg!\n");
    saradc_gain_dbg->read_proc = (read_proc_t *) proc_dump_xgain_dbg;    
    saradc_gain_dbg->write_proc =(write_proc_t *)proc_set_xgain_dbg;

    saradc_force_cvbs = create_proc_entry("force_cvbs", S_IRUGO, saradc_proc_root);
    
    if (saradc_force_cvbs == NULL)
        panic("Fail to create saradc_force_cvbs!\n");
    saradc_force_cvbs->read_proc = (read_proc_t *) proc_dump_force_cvbs;    
    saradc_force_cvbs->write_proc =(write_proc_t *)proc_set_force_cvbs;

    saradc_cvbs_trs = create_proc_entry("cvbs_thr", S_IRUGO, saradc_proc_root);
    
    if (saradc_cvbs_trs == NULL)
        panic("Fail to create saradc_cvbs_trs!\n");
    saradc_cvbs_trs->read_proc = (read_proc_t *) proc_dump_cvbs_tsr;    
    saradc_cvbs_trs->write_proc =(write_proc_t *)proc_set_cvbs_tsr;

    saradc_polling_tms = create_proc_entry("polling_time", S_IRUGO, saradc_proc_root);
    if (saradc_polling_tms == NULL)
        panic("Fail to create saradc_cvbs_trs!\n");
    saradc_polling_tms->read_proc = (read_proc_t *) proc_dump_polling_tms;    
    saradc_polling_tms->write_proc =(write_proc_t *)proc_set_polling_tms; 
    return 0;
}

static void saradc_proc_release(void)
{
    if (saradc_cvbs_info != NULL)
        remove_proc_entry(saradc_cvbs_info->name, saradc_proc_root);
    
    if (saradc_gain_dbg != NULL)
        remove_proc_entry(saradc_gain_dbg->name, saradc_proc_root);

    if (saradc_force_cvbs != NULL)
		remove_proc_entry(saradc_force_cvbs->name, saradc_proc_root);

    if (saradc_cvbs_trs != NULL)
		remove_proc_entry(saradc_cvbs_trs->name, saradc_proc_root);

    
    if (saradc_polling_tms != NULL)
            remove_proc_entry(saradc_polling_tms->name, saradc_proc_root);

    if (saradc_proc_root != NULL)
		remove_proc_entry(saradc_proc_root->name, NULL);
    
   
}
#endif


#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
int __devexit driver_remove(struct platform_device *pdev)
#else
int driver_remove(struct device * dev)
#endif
{      
	struct dev_data* p_dev_data = NULL;

    PRINT_FUNC();
	
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	p_dev_data = (struct dev_data *)platform_get_drvdata(pdev);	
	#else
	p_dev_data = (struct dev_data *)dev_get_drvdata(dev);
	#endif
    PRINT_I(" * %s p_dev_data 0x%08X\n", __FUNCTION__, (unsigned int)p_dev_data);

    /* unregister cdev */
    unregister_cdev(p_dev_data);
    
    #ifdef HARDWARE_IS_ON
    /* free irq */
    free_irq(p_dev_data->irq_no, p_dev_data);  
    
    Platform_Del_Pollign_Timer();
    /* remove pmu */
	exit_hardware(p_dev_data);
	
    /* release resource */
    iounmap((void __iomem *)p_dev_data->io_vadr); 
    release_mem_region((u32)p_dev_data->io_padr, p_dev_data->io_size);
    #endif
    
	/* free device memory, and set drv data as NULL	*/
	#if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,24))
	platform_set_drvdata(pdev, NULL);
	#else
	dev_set_drvdata(dev, NULL);
	#endif
    
    /* free device driver_data */
	dev_data_free(p_dev_data);

    
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    saradc_proc_release();
#endif
	return 0;
}



static int __init module_init_func(void)
{
    int ret = 0;  

    printk("\nWelcome to use %s %s\n", DEV_NAME ,SARADC_VER);

    PRINT_FUNC();   
    
	 g_recovery_cnt = 0;
    
    /* register platform device */
    ret = register_device(&_device);
    if (unlikely(ret < 0)) 
    {
        PRINT_E("%s fails: ft2dge_register_device not OK\n", __FUNCTION__);
        goto err1;
    }

    /* register platform driver     
     * probe will be done immediately after platform driver is registered 
     */
    ret = register_driver(&_driver);
    if (unlikely(ret < 0)) 
    {
        PRINT_E("%s fails: register_driver not OK\n", __FUNCTION__);
        goto err2;
    }
    
#if defined(CONFIG_PLATFORM_GM8139) || defined(CONFIG_PLATFORM_GM8136)
    saradc_proc_init();
#endif

   
    return 0;

        
    err2:
        unregister_device(&_device);		

    err1:
    return ret;

}

static void __exit module_exit_func(void)
{

    printk(" ************************************************\n");
    printk(" * Thank you to use %s,%s,goodbye *\n", DEV_NAME,SARADC_VER );
    printk(" ************************************************\n");

    PRINT_FUNC();   
    
    
    /* unregister platform driver */     
    unregister_driver(&_driver);


	/* register platform device */
    unregister_device(&_device);
}

static unsigned int file_poll(struct file *filp, poll_table *wait)
{
    unsigned int mask = 0;
    struct flp_data* p_flp_data = filp->private_data;
    struct dev_data* p_dev_data = p_flp_data->p_dev_data;
    struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;
    unsigned long       cpu_flags;

    if(down_interruptible(&p_data->oper_sem))
        return -ERESTARTSYS;
    poll_wait(filp, &p_data->wait_queue, wait);
    spin_lock_irqsave(&p_data->lock, cpu_flags);
    if(kfifo_len(&p_data->fifo))
        mask |= POLLIN | POLLRDNORM;
    spin_unlock_irqrestore(&p_data->lock, cpu_flags);
    up(&p_data->oper_sem);

    return mask;

}

static int file_open(struct inode *inode, struct file *filp)
{
	int ret_of_sar_adc_driver_open = 0;
	
	struct flp_data* p_flp_data = NULL;	
	struct dev_data* p_dev_data = NULL;
	
    /* set filp */
    p_dev_data = container_of(inode->i_cdev, struct dev_data, cdev);
    
	p_flp_data = kzalloc(sizeof(struct flp_data), GFP_KERNEL);
	p_flp_data->p_dev_data = p_dev_data;

	
    /* increase driver count */	
	DRV_COUNT_INC(); 

	/* assign to private_data */
    filp->private_data = p_flp_data;	


    //PRINT_I("\n");
	if (unlikely(((unsigned int)p_dev_data != (unsigned int)inode->i_cdev)))
	{
    	PRINT_I("%s p_dev_data 0x%08X 0x%08X\n", __FUNCTION__, (unsigned int)p_dev_data, (unsigned int)inode->i_cdev);
    	PRINT_I("%s p_flp_data 0x%08X\n", __FUNCTION__, (unsigned int)p_flp_data);
		ret_of_sar_adc_driver_open = -1;
	}


    return ret_of_sar_adc_driver_open;
}


#if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
static int file_ioctl(struct inode *inode, struct file *filp, unsigned int cmd, unsigned long arg)
#else
static long file_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
#endif
{
    #if (LINUX_VERSION_CODE < KERNEL_VERSION(2,6,36))
	int ret = 0;
    #else
    long ret = 0;
    #endif

	struct flp_data* p_flp_data = filp->private_data;	
	struct dev_data* p_dev_data = p_flp_data->p_dev_data;
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;


    down(&p_data->oper_sem);

    switch(cmd)
    {
        case SAR_ADC_KEY_ADC_DIRECT_READ:
        {
            int adc_val=0;
           
            adc_val = Platform_Direct_Get_XGain_Value((unsigned int)p_dev_data->io_vadr,g_xgain_num);
           
            if(copy_to_user((void *)arg, (void *)&adc_val, sizeof(int)) != 0) {
                ret = -EFAULT;
            } else {
                ret = 0;
            }
            break;
        }
        case SAR_ADC_KEY_SET_REPEAT_DURATION:
        {    
            int  ms;
	        if (unlikely(copy_from_user((void *)&ms, (void *)arg, sizeof(int))))
		    {
			    ret = -EFAULT;
			    break;
	        }
            
            if(ms>0)
                p_data->scan_duration = msecs_to_jiffies(ms);
            else
                ret = -EFAULT;
            break;
        }  
        case SAR_ADC_KEY_SET_XGAIN_NUM:
        {
            int  num;
	        if (unlikely(copy_from_user((void *)&num, (void *)arg, sizeof(int))))
		    {
			    ret = -EFAULT;
			    break;
	        }
         
            if(Platform_Check_Support_XGain(num) == 0)
                g_xgain_num = num;                
            else
                ret = -EFAULT;
            break;        
        }
        default:
            printk("%s cmd(0x%x) no define!\n", __func__, cmd);
            break;
    }

    up(&p_data->oper_sem);

    return ret;
}



static ssize_t file_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
 	unsigned long 		cpu_flags;
    int         ret = 0;
    sar_adc_pub_data data;
	struct flp_data* p_flp_data = filp->private_data;	
	struct dev_data* p_dev_data = p_flp_data->p_dev_data;
	struct dev_specific_data_t* p_data = &p_dev_data->dev_specific_data;


    if (down_interruptible(&p_data->oper_sem))
        return -ERESTARTSYS;

	spin_lock_irqsave(&p_data->lock, cpu_flags);
    if (kfifo_len(&p_data->fifo)==0)
    {
    	ret = 0;
        goto exit;
    }
    
    if (unlikely(
		kfifo_out(
			&p_data->fifo, 
			(unsigned char *)&data,
			sizeof(sar_adc_pub_data)
		) <= 0))
	{
    	ret = -ERESTARTSYS;
        goto exit;
	}

    if (copy_to_user(buf, &data, sizeof(sar_adc_pub_data)))
    {
    	ret = -ERESTARTSYS;
        goto exit;
    }

    ret = sizeof(u8);

	exit:
	spin_unlock_irqrestore(&p_data->lock, cpu_flags);
    up(&p_data->oper_sem);

    return ret;
}

static int file_release(struct inode *inode, struct file *filp)
{
	struct flp_data* p_flp_data = filp->private_data;	

    /* remove this filp from the asynchronously notified filp's */

    kfree(p_flp_data);

    filp->private_data = NULL;
    
    return 0;
}


module_init(module_init_func);
module_exit(module_exit_func);

MODULE_AUTHOR("GM Technology Corp.");
MODULE_DESCRIPTION("GM fasync test");
MODULE_LICENSE("GPL");
