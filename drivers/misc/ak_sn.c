#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <mach-anyka/anyka_types.h>
#include <mach-anyka/partition_lib.h> 
#include <mach/map.h>
#include <asm/io.h>
#include <linux/delay.h>

#define BAR_CODE_FILE_NAME "BAR"
#define SN_FILE_NAME	"SEQ"
#define SN_MAX_LEN	64


#define EFUSE_CFG_REG							(AK_VA_SYSCTRL + 0x0000008C)     // EFUSE CFG
#define EFUSE_CFG_REG1						(AK_VA_SYSCTRL + 0x00000090)     // CFG35
#define EFUSE_CFG_REG2						(AK_VA_SYSCTRL + 0x00000094)     // CFG36
#define EFUSE_CFG_REG4		 				(AK_VA_SYSCTRL + 0x0000009C)     // CODEC CFG1
#define EFUSE_CFG_REG5						(AK_VA_SYSCTRL + 0x000000A0)     // CODEC CFG2



#define DBG(fmt, args...) 	 			//printk(fmt, ##args)


/**
 * @brief: read vendor id and serial number from efuse area
 * @param: vid, pointer to 14bit vendor id
 * @param: sid, pointer to 32bit serial number
 * @retval: int
 */
static int efuse_read_id(unsigned long *vid, unsigned long *sid)
{
    unsigned long regval;

	 //enable resistor dirve 1.5v
	regval = __raw_readl(EFUSE_CFG_REG5);
  regval |= (1<<27);
  __raw_writel(regval, (EFUSE_CFG_REG5));  

	//power on Avcc25_sel and efuse_vp 
	regval = __raw_readl(EFUSE_CFG_REG4) ; 
	regval	|= ((1<<2)|(1<<1));
	 __raw_writel(regval, (EFUSE_CFG_REG4));
	
	//power on bias generator and VCM3
	regval = __raw_readl(EFUSE_CFG_REG4) ; 
	regval 	&=~((1<<0)|(1<<3));
	__raw_writel(regval, (EFUSE_CFG_REG4));	
	
	 // not used vcm3 pull down
	regval = __raw_readl(EFUSE_CFG_REG5);    
	regval &=~(1<<0);
	__raw_writel(regval, (EFUSE_CFG_REG5));	

    msleep(300);  
  
	while(__raw_readl(EFUSE_CFG_REG) & (1<<31));
  
	//enable efuse read
	regval = __raw_readl(EFUSE_CFG_REG);
	regval |= (1<<31);
	 __raw_writel(regval, (EFUSE_CFG_REG));
  
	//wait read finished
	while(__raw_readl(EFUSE_CFG_REG) & (1<<31));

	//bit[31:0]
	regval = __raw_readl(EFUSE_CFG_REG1);
	*vid = regval&0xffff;  
	*sid = (regval>>16) & 0xffff;

	//bit[47:32]
	regval = __raw_readl(EFUSE_CFG_REG2);  
	*sid |= ((regval<<16) & 0xffff0000);

	return 0; 
}

/**
 * @brief: test efuse has been locked or not
 * @retval: bool, locked return true
 */
static bool efuse_locked(void)
{
    unsigned long regval,sid=0;
  
	 //enable resistor dirve 1.5v
	regval = __raw_readl(EFUSE_CFG_REG5);
  regval |= (1<<27);
  __raw_writel(regval, (EFUSE_CFG_REG5));  

	//power on Avcc25_sel and efuse_vp 
	regval = __raw_readl(EFUSE_CFG_REG4) ; 
	regval	|= ((1<<2)|(1<<1));
	 __raw_writel(regval, (EFUSE_CFG_REG4));
	
	//power on bias generator and VCM3
	regval = __raw_readl(EFUSE_CFG_REG4) ; 
	regval 	&=~((1<<0)|(1<<3));
	__raw_writel(regval, (EFUSE_CFG_REG4));	
	
	 // not used vcm3 pull down
	regval = __raw_readl(EFUSE_CFG_REG5);    
	regval &=~(1<<0);
	__raw_writel(regval, (EFUSE_CFG_REG5));	 

    msleep(300); 
   
    //enable efuse read
	regval = __raw_readl(EFUSE_CFG_REG);
	regval |= (1<<31);
	  __raw_writel(regval, (EFUSE_CFG_REG));
	
	//wait read finished
	while(__raw_readl(EFUSE_CFG_REG) & (1<<31));

/*		
	//bit[47:32]
	regval = __raw_readl(AK_VA_SYSCTRL+0x94); 
	
	if ((regval & (1<<15)) == 1)
	    return true; //locked
	else
	    return false; //unlocked
*/
	regval = __raw_readl(EFUSE_CFG_REG1);
	sid = (regval>>16) & 0xffff;
	//bit[47:32]
	regval = __raw_readl(EFUSE_CFG_REG2);  
	sid |= ((regval<<16) & 0xffff0000);
	DBG("lock sid %lx\n", sid);
	if (sid > 0)
	    return true; //locked
	else
	    return false; //unlocked

}

/*
 * @brief: burn efuse bit, the last bit is lock bit
 * @param: cell: 0 ~ 5
 * @param: bit : 0 ~ 7
 */
static int efuse_burn_bit(unsigned char cell, unsigned char bit)
{  
	 
	unsigned long regval = 0;
	
 	DBG("cell:%d,bit:%x\n",cell,bit);

  DBG("\n===========efuse write operation============\n");	

  while(__raw_readl(EFUSE_CFG_REG) & (1<<30) ) //wait bit[30]=0 ,ready to configure write operation
 		{};
  DBG("****EFUSE_CFG_REG = %x \n", __raw_readl(EFUSE_CFG_REG));
	
	regval = __raw_readl(EFUSE_CFG_REG);          //clear macro value and macro cell select	
	regval &= ~(0x7ff <<0);
	__raw_writel(regval, EFUSE_CFG_REG); 
  
	regval =  __raw_readl(EFUSE_CFG_REG);         // set new macro value and select macro cell
	regval |= ( bit |(cell <<8));
	__raw_writel(regval, EFUSE_CFG_REG); 			  


	regval = __raw_readl(EFUSE_CFG_REG);          // start efuse write operation
	regval |= (1<<30);
	__raw_writel(regval, EFUSE_CFG_REG);			 

	DBG("****Configure efuse write finished , EFUSE_CFG_REG = %x \n", __raw_readl(EFUSE_CFG_REG));      
	while(__raw_readl(EFUSE_CFG_REG) & (1<<30) )        //wait bit[30]=0 ,one cell write finished 
		{};
	DBG("****macro cell %d write finished , EFUSE_CFG_REG = %x \n",cell, __raw_readl(EFUSE_CFG_REG));  
	   
    return 0;
}

static ssize_t barcode_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    unsigned long vid=0, sid=0, len=0;
    
	efuse_read_id(&vid, &sid);

  DBG("sid:%lx\n",sid);
	len = sprintf(buf, "%lx:", vid);
	len += sprintf(buf+len, "%lx", sid);
	len += sprintf(buf+len, "\n");

	return len;
    
}

ssize_t barcode_store(struct kobject *kobj, struct kobj_attribute *attr,
		 const char *buf, size_t count)
{
	unsigned long vid=0, sid=0,sidbk=0;
	unsigned char cell, bit, i;

	if (efuse_locked())
    {
        printk(KERN_WARNING "barcode number has been locked!!\n");
        return -EACCES;
    }
   
	sscanf(buf, "%lx", &sid);
	DBG("sid:%lx\n",sid);
	sidbk = sid;
	for(i=0; i<4; i++)
	{
         
		cell = i+2;
		DBG("sidbk:%lx\n",sidbk);
		bit = sidbk & 0xff;
//		if(i >= 3)
//			bit |= 0x8;   //不处理最高lock  bit
			
		DBG("bit:%x\n",bit);	
		efuse_burn_bit(cell, bit);
		sidbk = sidbk >> 8;
		DBG("sidbk11:%lx\n",sidbk);
	}

    //readback and show
	efuse_read_id(&vid, &sid);
	DBG("barcode: %lx\n", sid);

	return count;
}


static ssize_t sn_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	unsigned char sn[SN_MAX_LEN] = {0};
	void * handle ;
	int ret = 0;
	
	/* search the same name partition */
	handle = partition_open(SN_FILE_NAME);
	if (handle == NULL) {
		printk(KERN_ERR "%s, open partition error!\n", __func__);
		goto out;
	}

	ret = partition_read(handle , sn, SN_MAX_LEN);
	if (ret == -1) {
		printk(KERN_ERR "%s, write partition error!\n", __func__);
		memset(sn, 0, SN_MAX_LEN);
	}

	/* search open the same name partition */
	ret = partition_close(handle);
	if (ret == -1) {
		printk(KERN_ERR "%s, close partition error!\n", __func__);
		memset(sn, 0, SN_MAX_LEN);
		goto out;
	}
out:
	return sprintf(buf, "%s\n", sn + 4);
}


static struct kobj_attribute sn_attribute = 
	__ATTR(sn, 0666, sn_show, NULL);

static struct kobj_attribute barcode_attribute = 
	__ATTR(barcode, 0666, barcode_show, barcode_store);

static struct attribute *attrs[] = {
	&sn_attribute.attr,
	NULL
};

static struct attribute *attrsbarcode[] = {
	&barcode_attribute.attr,
	NULL
};


static struct kobject *sn_kobj;

static int __init serial_number_init(void)
{
	int ret;

	sn_kobj = kobject_create_and_add("serial_number", kernel_kobj);
	if (!sn_kobj) {
		printk("Create serial number kobject failed\n");
		return -ENOMEM;
	}

	ret = sysfs_create_file(sn_kobj, *attrs);
	if (ret) {
		printk("Create serial number sysfs file failed\n"); 
		kobject_put(sn_kobj);
	}

	ret = sysfs_create_file(sn_kobj, *attrsbarcode);
	if (ret) {
		printk("Create serial number sysfs file failed\n"); 
		kobject_put(sn_kobj);
	}

	return ret;
}

static void __exit serial_number_exit(void)
{
	kobject_put(sn_kobj);
}

module_init(serial_number_init);
module_exit(serial_number_exit);

MODULE_DESCRIPTION("Anyka Device Serial Number Interface");
MODULE_AUTHOR("Anyka");
MODULE_LICENSE("GPL");
