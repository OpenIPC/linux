/*
 *  ip6303_battery.c
 *  fuel-gauge systems for lithium-ion (Li+) batteries
 *
 *  Copyright (C) 2019 SStar
 *  lei.qin <lei.qin@sigmastar.com.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/err.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/power_supply.h>
#include <linux/ip6303_battery.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/interrupt.h>
//#include <../../../drivers/sstar/include/infinity6e/irqs.h>
#include <linux/gpio.h>
//#include <../../../drivers/sstar/include/infinity6e/gpio.h>

#define IP6303_VCELL_MSB	0x02
#define IP6303_VCELL_LSB	0x03
#define IP6303_SOC_MSB	0x04
#define IP6303_SOC_LSB	0x05
#define IP6303_MODE_MSB	0x06
#define IP6303_MODE_LSB	0x07
#define IP6303_VER_MSB	0x08
#define IP6303_VER_LSB	0x09
#define IP6303_RCOMP_MSB	0x0C
#define IP6303_RCOMP_LSB	0x0D
#define IP6303_CMD_MSB	0xFE
#define IP6303_CMD_LSB	0xFF

#define IP6303_DELAY		1000
//#define IP6303_POWEROFF_DELAY		500

#define IP6303_BATTERY_FULL	95

#define IP6303_MAX_V        4300
#define IP6303_PF_NOTIFY    4000    //poweroff notify
#define IP6303_MIN_V        3600
#define IP6303_POFF_PERC      50    //less than 40%
#define IP6303_CHK_CNT      2       //above 2 times in less than 40%

struct ip6303_chip {
	struct i2c_client		*client;
	struct delayed_work		work;
	struct power_supply		*battery;
	struct ip6303_platform_data	*pdata;
    unsigned int            irq;            // IRQ number

	/* State Of Connect */
	int online;
	/* battery voltage */
	int vcell;
	/* battery capacity */
	int soc;
	/* State Of Charge */
	int status;
    /* State Of Charge Enable */
    int chargEn;
    /* check Of battery exit */
    int assembleBat;
    /* force to update all status */
    int updateAll;
    /* long press status */
    int longPress;

    int BatLowLoopCnt;
};

struct ip6303_chip *g_chip;

static int ip6303_write_reg(struct i2c_client *client, int reg, u8 value)
{
	int ret;

	ret = i2c_smbus_write_byte_data(client, reg, value);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static int ip6303_read_reg(struct i2c_client *client, int reg)
{
	int ret;

	ret = i2c_smbus_read_byte_data(client, reg);

	if (ret < 0)
		dev_err(&client->dev, "%s: err %d\n", __func__, ret);

	return ret;
}

static void ip6303_reset(struct i2c_client *client)
{
#if defined(IP_PMU_WATCH_DOG_ENABLE) && (IP_PMU_WATCH_DOG_ENABLE)
    int returnvalue;
    IP6303WDogCtl_U uWDogCtl;

	uWDogCtl.uData = (unsigned int)ip6303_read_reg(client,WDOG_CTL);

	uWDogCtl.tFlag.bIsWDogEn = 0;
	uWDogCtl.tFlag.bTimerClr = 1;
	ip6303_write_reg(client,WDOG_CTL, (u8)uWDogCtl.uData);

	uWDogCtl.tFlag.uTimerType = 0;
	uWDogCtl.tFlag.bTimerClr = 0;
	uWDogCtl.tFlag.bIsWDogEn = 1;
	returnvalue = ip6303_write_reg(client,WDOG_CTL, (u8)uWDogCtl.uData);
    printk(KERN_NOTICE "%s: uWDogCtl.uData= %d\n", __func__, uWDogCtl.uData);

#endif
}

void IP6303_Initialize(struct i2c_client *client)
{
	IpIntsCtl_U 	uIntsCtl = {0};
	IpIntrMask0_U	uIntrMask0 = {0};
	IpChgDigCtl3_U  uChgDigCtl3 = {0};
	IpAdcAnaCtl0_U  uAdcAnaCtl0 = {0};
    unsigned int    stu0,stu1,stu2;
    u8              u8PwrEn, u8val, i;
    int             err;
    u32             val;
    char            nam[16]={0};
    struct ip6303_chip *chip = i2c_get_clientdata(client);

    stu0 = ip6303_read_reg(client,PWROFF_REC0);
    stu1 = ip6303_read_reg(client,PWROFF_REC1);
    stu2 = ip6303_read_reg(client,PWROFF_REC2);
    printk(KERN_NOTICE "%s,PWROFF_REC[0x%02x,0x%02x,0x%02x]\n", __func__,stu0,stu1,stu2);

    //OK_SET=4.3V,VBUSOCH_SET>=2A
    ip6303_write_reg(client,PROTECT_CTL4, 0x3C);

    uIntsCtl.uData = (unsigned int)ip6303_read_reg(client,INTS_CTL);
	uIntsCtl.tFlag.bIrqPolHighValid = PMU_IRQ_ACT_LEVEL;
    ip6303_write_reg(client,INTS_CTL, uIntsCtl.uData);

	//Intr Cfg -- INTR MASK
	uIntrMask0.uData = 0xFF;
	uIntrMask0.tFlag.bShortPressOnOff = IP_PMU_INTR_UNMASK;
	uIntrMask0.tFlag.bLongPressOnOff = IP_PMU_INTR_UNMASK;
    uIntrMask0.tFlag.bVBusIn = IP_PMU_INTR_UNMASK;
	uIntrMask0.tFlag.bVBusOut = IP_PMU_INTR_UNMASK;
    uIntrMask0.tFlag.bLowBat = IP_PMU_INTR_UNMASK;
    ip6303_write_reg(client,INTR_MASK_0, uIntrMask0.uData);

    for(i = 1; i <= 3; i++){
        sprintf(nam,"DC%d_VSET",i);
        err = of_property_read_u32(client->dev.of_node, nam, &val);
        if((err < 0) && (val < 3600) && (val > 600)){
            dev_err(&client->dev, "dts can't find %s\n",nam);
        }else{
            u8val = (u8)((val-600)*10/125 & 0xff);//0.6V~3.6V=>0000_0000~1111_0000,step=12.5mv
            printk(KERN_NOTICE "dts find %s=%dmV,0x%02x\n",nam,val,u8val);
            ip6303_write_reg(client,DC1_VSET+(i-1)*5, u8val);
        }
    }

    memset(nam,0,sizeof(nam));
    for(i = 2; i <= 5; i++){
        sprintf(nam,"LDO%d_VSET",i);
        err = of_property_read_u32(client->dev.of_node, nam, &val);
        if((err < 0) && (val < 3400) && (val > 700)){
            dev_err(&client->dev, "dts can't find %s\n",nam);
        }else{
            u8val = (u8)(((val-700)/25) & 0xff);//0.7V~3.4V=>0000_0000~0110_1100,step=25mv
            printk(KERN_NOTICE "dts find %s=%dmV,0x%02x\n",nam,val,u8val);
            ip6303_write_reg(client,LDO2_VSET+i-2, u8val);
            u8PwrEn |= (1 << i);
        }
   }
    //dev_err(&client->dev, "LDO_EN =0x%02x\n",u8PwrEn);
    //ENABLE:LDO2&LDO3&LDO4&LDO5
    ip6303_write_reg(client,LDO_EN, u8PwrEn);

	ip6303_write_reg(client,CHG_ANA_CTL0, 0X21);    //fast charge:28mV  EN_ISTOP=enable
    ip6303_write_reg(client,PSTATE_SET, 0X80);      //S2S3_DELAY=8mS

	//clear all irq status
    ip6303_write_reg(client,INTR_FLAG_0, 0xFF);

	//enable voltage ADC
    uAdcAnaCtl0.uData = (unsigned int)ip6303_read_reg(client,ADC_ANA_CTL0);
	uAdcAnaCtl0.tFlag.bEnVBatAdc = 1;
    ip6303_write_reg(client,ADC_ANA_CTL0, (u8)uAdcAnaCtl0.uData);
    msleep(10);

	//enable charging
    uChgDigCtl3.uData = (unsigned int)ip6303_read_reg(client,CHG_DIG_CTL3);
	uChgDigCtl3.tFlag.bEnChg = 1;
    chip->chargEn = 1;
    ip6303_write_reg(client,CHG_DIG_CTL3, (u8)uChgDigCtl3.uData);

	#if defined(IP_PMU_WATCH_DOG_ENABLE) && (IP_PMU_WATCH_DOG_ENABLE)
	{
		//打开看门狗
		IP6303WDogCtl_U uWDogCtl = {0};
		//returnvalue = IP_IP6303_ReadReg(WDOG_CTL, (MMP_USHORT*)&(uWDogCtl.uData));
        uWDogCtl.uData = (unsigned int)ip6303_read_reg(client,WDOG_CTL);
		uWDogCtl.tFlag.uTimerType = 3;
		uWDogCtl.tFlag.bIsWDogEn = 1;
		//returnvalue = IP_IP6303_WriteReg(WDOG_CTL, uWDogCtl.uData);
        ip6303_write_reg(client,WDOG_CTL, (u8)uWDogCtl.uData);
	}
	#endif
}

static void ip6303_charge_en(struct i2c_client *client,bool en)
{
	struct ip6303_chip *chip = i2c_get_clientdata(client);
    IpChgDigCtl3_U  uChgDigCtl3 = {0};

    chip->chargEn = en;
    //enable charging
    uChgDigCtl3.uData = (unsigned int)ip6303_read_reg(client,CHG_DIG_CTL3);
	uChgDigCtl3.tFlag.bEnChg = en;
    ip6303_write_reg(client,CHG_DIG_CTL3, (u8)uChgDigCtl3.uData);
}

static void ip6303_power_off(struct i2c_client *client)
{
    IPPStateCtl0_U  uPStateCtl0 = {0};

    ip6303_charge_en(client,0);//disable charging

	uPStateCtl0.uData = ip6303_read_reg(client,PSTATE_CTL0);
    uPStateCtl0.tFlag.bIrqWkEn = 1;
	uPStateCtl0.tFlag.bSOnoffWkEn = 1;  //DR0010短按不开机, 短按不开机会导致停车监控无法开机
	uPStateCtl0.tFlag.bLOnoffWkEn = 1;
	uPStateCtl0.tFlag.bPwrOffEn = 0;
    uPStateCtl0.tFlag.bVBusWkEn = 0;
    ip6303_write_reg(client,PSTATE_CTL0, (u8)uPStateCtl0.uData);
    msleep(2);
	uPStateCtl0.tFlag.bPwrOffEn = 1;

	while(1)
	{
	    printk(KERN_NOTICE "********IP6303:SHUT DOWN*******\n\n");
		ip6303_write_reg(client,PSTATE_CTL0, (u8)uPStateCtl0.uData);
		msleep(2);
	}
}

static void ip6303_poweroff_enter(void)
{
    ip6303_power_off(g_chip->client);
}

static void ip6303_get_vcell(struct i2c_client *client)
{
	struct ip6303_chip *chip = i2c_get_clientdata(client);
    u32 ubValue, uVoltage;

	//Get VBAT,Uint:mV
    ubValue = (u32)ip6303_read_reg(client, ADC_DATA_VBAT);
	uVoltage = ubValue*15625 +500000+ 15625/2;
    uVoltage /= 1000;
    printk(KERN_INFO "\n%s dev-reg[%x,0x64],V:%dmV\n", __func__,client->addr,uVoltage);
    chip->vcell = uVoltage;
}

static void ip6303_get_soc(struct i2c_client *client)
{
    struct ip6303_chip *chip = i2c_get_clientdata(client);
    int charging = chip->chargEn;
    IpChgDigCtl3_U  uChgDigCtl3 = {0};

    if(chip->vcell < IP6303_MIN_V){
        chip->soc = 0;
        chip->BatLowLoopCnt ++;
    }
    else if (chip->vcell > IP6303_MAX_V){
        chip->soc = 100;
        chip->chargEn = 0;
        chip->BatLowLoopCnt = 0;
    }
    else{
        chip->soc = (chip->vcell - IP6303_MIN_V) *100 / (IP6303_MAX_V - IP6303_MIN_V);
        if(chip->soc < IP6303_POFF_PERC){
            chip->BatLowLoopCnt ++;
        }
        else{
            (chip->BatLowLoopCnt > 0)? (chip->BatLowLoopCnt--):(chip->BatLowLoopCnt = 0);
        }
    }

    if(charging != chip->chargEn){
        printk(KERN_INFO "%s chargEn=%d\n", __func__,chip->chargEn);//

        uChgDigCtl3.uData = (unsigned int)ip6303_read_reg(client,CHG_DIG_CTL3);
    	uChgDigCtl3.tFlag.bEnChg = (chip->chargEn)? 1:0;
        ip6303_write_reg(client,CHG_DIG_CTL3, (u8)uChgDigCtl3.uData);
    }
}

static void ip6303_get_version(struct i2c_client *client)
{
	u8 msb;
	u8 lsb;

	msb = 0x11;
	lsb = 0x22;
	printk(KERN_INFO "TODO::IP6303 Fuel-Gauge Ver %d%d\n", msb, lsb);
}

static void ip6303_get_battery_assemble(struct i2c_client *client)
{
    int RegIntFlag0;
    struct ip6303_chip *chip = i2c_get_clientdata(client);

    //enable to check
    RegIntFlag0 = ip6303_read_reg(client, PSTATE_CTL3);
    ip6303_write_reg(client, PSTATE_CTL3,RegIntFlag0|BIT0);

	//Get BAT ext status
	RegIntFlag0 = ip6303_read_reg(client,CHG_DIG_CTL2);
	if(RegIntFlag0 & BIT3)
        chip->assembleBat = 1;
}

static void ip6303_get_init_online(struct i2c_client *client)
{
    int RegIntFlag0;
    struct ip6303_chip *chip = i2c_get_clientdata(client);

    RegIntFlag0 = ip6303_read_reg(client, INTR_FLAG_0);
    ip6303_write_reg(client, INTR_FLAG_0,RegIntFlag0);  //clear flag
    //printk(KERN_INFO "TODO::%s RegIntFlag0=0x%x\n", __func__, RegIntFlag0);

    if(!(RegIntFlag0 & (BIT0|BIT1))){         //long press && short press issue
        chip->online = RegIntFlag0 & BIT3;  //plug in
    }

    //REF:IP6303_SoftwareApplication_User's_Guide_Vxxx.PDF
    //How to judge VBUS status.
    RegIntFlag0 = ip6303_read_reg(client,CHG_DIG_CTL1);
    printk(KERN_INFO "TODO::%s reg0x54=0x%x\n", __func__, RegIntFlag0);
    if(RegIntFlag0)
        chip->online = 1;

    #if 0
    else{
		RegIntFlag0 = ip6303_read_reg(client,ADC_ANA_CTL0);
		RegIntFlag0 |= BIT2;                //enable current check
		ip6303_write_reg(client, ADC_ANA_CTL0,RegIntFlag0);
        RegIntFlag0 = ip6303_read_reg(client,ADC_DATA_ICHG);
		RegIntFlag0 = (RegIntFlag0*15625-750000+15625/2)/3;
        chip->online = 1;
    }
    #endif
}

static void ip6303_get_online(struct i2c_client *client)
{
    int RegIntFlag0;
    struct ip6303_chip *chip = i2c_get_clientdata(client);

    RegIntFlag0 = ip6303_read_reg(client, INTR_FLAG_0);
    ip6303_write_reg(client, INTR_FLAG_0,RegIntFlag0);  //clear flag

    chip->online = RegIntFlag0 & BIT3;//plug in
    if(chip->online){
        printk(KERN_INFO "TODO::%s online***\n", __func__);
    }

    chip->longPress = RegIntFlag0 & (BIT1|BIT0);    //power-key:short press & long press
}

static void ip6303_get_status(struct i2c_client *client)
{
	struct ip6303_chip *chip = i2c_get_clientdata(client);

	if(chip->online) {
		if (chip->chargEn)
			chip->status = POWER_SUPPLY_STATUS_CHARGING;
		else
			chip->status = POWER_SUPPLY_STATUS_NOT_CHARGING;
	} else {
		chip->status = POWER_SUPPLY_STATUS_DISCHARGING;
	}

	if(chip->soc > IP6303_BATTERY_FULL)
		chip->status = POWER_SUPPLY_STATUS_FULL;

}

static void ip6303_get_all(struct i2c_client *client)
{
	ip6303_get_vcell(client);
	ip6303_get_soc(client);
	ip6303_get_online(client);
	ip6303_get_status(client);
}

static int ip6303_get_property(struct power_supply *psy,
			    enum power_supply_property psp,
			    union power_supply_propval *val)
{
	struct ip6303_chip *chip = power_supply_get_drvdata(psy);

    if(chip->updateAll){
        chip->updateAll = 0;
        ip6303_get_all(chip->client);

        if((chip->vcell < IP6303_PF_NOTIFY) || (chip->longPress)){
            chip->longPress = 0;
            kernel_power_off();
        }
    }

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = chip->status;
		break;
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = chip->online;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = chip->vcell;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = chip->soc;
		break;
	default:
		return -EINVAL;
	}

    printk(KERN_NOTICE "%s,psp:%d,val:%d\n",__func__,psp,val->intval);
	return 0;
}

static irqreturn_t ip6303_isr(int irq, void* data)
{
    struct ip6303_chip *chip = (struct ip6303_chip *)data;
    //struct i2c_client *client = chip->client;

    printk(KERN_INFO "TODO::%s[%d,0x%08x]**********\n", __func__,irq,(u32)data);
    chip->updateAll = 1;
    power_supply_changed(chip->battery);

    return IRQ_HANDLED;
}

static void ip6303_work(struct work_struct *work)
{
	struct ip6303_chip *chip;
    struct i2c_client *client;
    int oldsoc, oldchargEn,oldonline;

	chip = container_of(work, struct ip6303_chip, work.work);
    client = chip->client;
    oldsoc = chip->soc;
    oldchargEn = chip->chargEn;
    oldonline = chip->online;

	ip6303_get_vcell(client);
	ip6303_get_soc(client);
	//ip6303_get_online(client);
	ip6303_get_status(client);

    if((oldsoc != chip->soc)||(oldchargEn != chip->chargEn)||(oldonline != chip->online))
        power_supply_changed(chip->battery);

    if((chip->soc < IP6303_POFF_PERC) && (chip->BatLowLoopCnt > IP6303_CHK_CNT)){
        printk(KERN_INFO "********TODO:NOTIFY SYSTEM,POWEROFF*******\n\n");
        kernel_power_off();
    }

	queue_delayed_work(system_power_efficient_wq, &chip->work,IP6303_DELAY);
}

static u32 ip6303_getValue(struct device *dev,int reg)
{
    int regValue=0;
    u32 value=0;
    struct i2c_client *client = to_i2c_client(dev);
    switch(reg)
    {
        case DC1_VSET:
            regValue=ip6303_read_reg(client, DC1_VSET);
            value=(((regValue&0xff)*125/10)+600);
            break;
         case DC2_VSET:
            regValue=ip6303_read_reg(client, DC2_VSET);
            value=(((regValue&0xff)*125/10)+600);
            break;
         case DC3_VSET:
            regValue=ip6303_read_reg(client, DC3_VSET);
            value=(((regValue&0xff)*125/10)+600);
            break;
         case LDO2_VSET:
            regValue=ip6303_read_reg(client, LDO2_VSET);
            value=(((regValue&0xff)*25)+700);
            break;
        case LDO3_VSET:
            regValue=ip6303_read_reg(client, LDO3_VSET);
            value=(((regValue&0xff)*25)+700);
            break;
        case LDO4_VSET:
            regValue=ip6303_read_reg(client, LDO4_VSET);
            value=(((regValue&0xff)*25)+700);
            break;
        case LDO5_VSET:
            regValue=ip6303_read_reg(client, LDO5_VSET);
            value=(((regValue&0xff)*25)+700);
            break;
        default:
            printk("reg error!!\n");
    }

    return value;
}
static ssize_t ip6303_battery_DC1_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret;
    u32 value=0;
    value=ip6303_getValue(dev,DC1_VSET);    
    ret = sprintf(buf, "%d\n", value);
    return ret;
}
static ssize_t ip6303_battery_DC1_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    u8 u8val;
    int value,ret;
    struct i2c_client *client = to_i2c_client(dev);
    ret=kstrtoint(buf, 10, &value);
    if((value > 3600) && (value < 600)){
        printk( "%d is out of range!!\n",value);
    }else{
        u8val = (u8)((value-600)*10/125 & 0xff);//0.6V~3.6V=>0000_0000~1111_0000,step=12.5mv
        ip6303_write_reg(client,DC1_VSET, u8val);
    }
    return count;   
}
static ssize_t ip6303_battery_DC2_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret;
    u32 value=0;
    value=ip6303_getValue(dev,DC2_VSET);    
    ret = sprintf(buf, "%d\n", value);
    return ret;
}
static ssize_t ip6303_battery_DC2_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    u8 u8val;
    int value,ret;
    struct i2c_client *client = to_i2c_client(dev);
    ret=kstrtoint(buf, 10, &value);
    if((value > 3600) && (value < 600)){
        printk( "%d is out of range!!\n",value);
    }else{
        u8val = (u8)((value-600)*10/125 & 0xff);//0.6V~3.6V=>0000_0000~1111_0000,step=12.5mv
        ip6303_write_reg(client,DC2_VSET, u8val);
    }
    return count;   
}
static ssize_t ip6303_battery_DC3_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret;
    u32 value=0;
    value=ip6303_getValue(dev,DC3_VSET);    
    ret = sprintf(buf, "%d\n", value);
    return ret;
}
static ssize_t ip6303_battery_DC3_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    u8 u8val;
    int value,ret;
    struct i2c_client *client = to_i2c_client(dev);
    ret=kstrtoint(buf, 10, &value);
    if((value > 3600) && (value < 600)){
        printk( "%d is out of range!!\n",value);
    }else{
        u8val = (u8)((value-600)*10/125 & 0xff);//0.6V~3.6V=>0000_0000~1111_0000,step=12.5mv
        ip6303_write_reg(client,DC3_VSET, u8val);
    }
    return count;   
}
static ssize_t ip6303_battery_LDO2_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret;
    u32 value=0;
    value=ip6303_getValue(dev,LDO2_VSET);    
    ret = sprintf(buf, "%d\n", value);
    return ret;
}
static ssize_t ip6303_battery_LDO2_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    u8 u8val;
    int value,ret;
    struct i2c_client *client = to_i2c_client(dev);
    ret=kstrtoint(buf, 10, &value);
    if((value > 3400) && (value < 700)){
        printk( "%d is out of range!!\n",value);
    }else{
        u8val = (u8)(((value-700)/25) & 0xff);//0.7V~3.4V=>0000_0000~0110_1100,step=25mv
        ip6303_write_reg(client,LDO2_VSET, u8val);
    }
    return count;   
}
static ssize_t ip6303_battery_LDO3_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret;
    u32 value=0;
    value=ip6303_getValue(dev,LDO3_VSET);    
    ret = sprintf(buf, "%d\n", value);
    return ret;
}
static ssize_t ip6303_battery_LDO3_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    u8 u8val;
    int value,ret;
    struct i2c_client *client = to_i2c_client(dev);
    ret=kstrtoint(buf, 10, &value);
    if((value > 3400) && (value < 700)){
        printk( "%d is out of range!!\n",value);
    }else{
        u8val = (u8)(((value-700)/25) & 0xff);//0.7V~3.4V=>0000_0000~0110_1100,step=25mv
        ip6303_write_reg(client,LDO3_VSET, u8val);
    }
    return count;   
}

static ssize_t ip6303_battery_LDO4_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret;
    u32 value=0;
    value=ip6303_getValue(dev,LDO4_VSET);    
    ret = sprintf(buf, "%d\n", value);
    return ret;
}
static ssize_t ip6303_battery_LDO4_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    u8 u8val;
    int value,ret;
    struct i2c_client *client = to_i2c_client(dev);
    ret=kstrtoint(buf, 10, &value);
    if((value > 3400) && (value < 700)){
        printk( "%d is out of range!!\n",value);
    }else{
        u8val = (u8)(((value-700)/25) & 0xff);//0.7V~3.4V=>0000_0000~0110_1100,step=25mv
        ip6303_write_reg(client,LDO4_VSET, u8val);
    }
    return count;   
}

static ssize_t ip6303_battery_LDO5_show(struct device *dev,struct device_attribute *attr, char *buf)
{
    int ret;
    u32 value=0;
    value=ip6303_getValue(dev,LDO5_VSET);    
    ret = sprintf(buf, "%d\n", value);
    return ret;
}
static ssize_t ip6303_battery_LDO5_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    u8 u8val;
    int value,ret;
    struct i2c_client *client = to_i2c_client(dev);
    ret=kstrtoint(buf, 10, &value);
    if((value > 3400) && (value < 700)){
        printk( "%d is out of range!!\n",value);
    }else{
        u8val = (u8)(((value-700)/25) & 0xff);//0.7V~3.4V=>0000_0000~0110_1100,step=25mv
        ip6303_write_reg(client,LDO5_VSET, u8val);
    }
    return count;   
}


static DEVICE_ATTR(DC1_VSET,                0660,  ip6303_battery_DC1_show,             ip6303_battery_DC1_store);
static DEVICE_ATTR(DC2_VSET,                0660,  ip6303_battery_DC2_show,             ip6303_battery_DC2_store);
static DEVICE_ATTR(DC3_VSET,                0660,  ip6303_battery_DC3_show,             ip6303_battery_DC3_store);
static DEVICE_ATTR(LDO2_VSET,               0660,  ip6303_battery_LDO2_show,            ip6303_battery_LDO2_store);
static DEVICE_ATTR(LDO3_VSET,               0660,  ip6303_battery_LDO3_show,            ip6303_battery_LDO3_store);
static DEVICE_ATTR(LDO4_VSET,               0660,  ip6303_battery_LDO4_show,            ip6303_battery_LDO4_store);
static DEVICE_ATTR(LDO5_VSET,               0660,  ip6303_battery_LDO5_show,            ip6303_battery_LDO5_store);

static struct attribute *ip6303_attributes[] = {
    &dev_attr_DC1_VSET.attr,
    &dev_attr_DC2_VSET.attr,
    &dev_attr_DC3_VSET.attr,
    &dev_attr_LDO2_VSET.attr,
    &dev_attr_LDO3_VSET.attr,
    &dev_attr_LDO4_VSET.attr,
    &dev_attr_LDO5_VSET.attr,
    NULL
};

static const struct attribute_group ip6303_attr_group = {
    .name   = "ip6303",
    .attrs  = ip6303_attributes,
};


static enum power_supply_property ip6303_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CAPACITY,
};

static const struct power_supply_desc ip6303_battery_desc = {
	.name		= "battery",
	.type		= POWER_SUPPLY_TYPE_BATTERY,
	.get_property = ip6303_get_property,
	.properties	= ip6303_battery_props,
	.num_properties	= ARRAY_SIZE(ip6303_battery_props),
};

static int ip6303_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
    int err;
	struct i2c_adapter *adapter = to_i2c_adapter(client->dev.parent);
	struct power_supply_config psy_cfg = {};
	struct ip6303_chip *chip;

    printk(KERN_NOTICE "%s,%d,iic dev:%s\n",__func__,__LINE__,client->name);

	if(!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE))
		return -EIO;

	chip = devm_kzalloc(&client->dev, sizeof(*chip), GFP_KERNEL);
	if(!chip)
		return -ENOMEM;

	chip->client = client;
	chip->pdata = client->dev.platform_data;

	i2c_set_clientdata(client, chip);
	psy_cfg.drv_data = chip;

	chip->battery = power_supply_register(&client->dev,&ip6303_battery_desc, &psy_cfg);
	if(IS_ERR(chip->battery)) {
		dev_err(&client->dev, "failed: power supply register\n");
		return PTR_ERR(chip->battery);
	}

	ip6303_reset(client);
	ip6303_get_version(client);
    IP6303_Initialize(client);
    ip6303_get_battery_assemble(client);
    ip6303_get_init_online(client);

    // Retrieve IRQ
    of_property_read_u32(client->dev.of_node, "pmu_irq_gpio", &chip->irq);
    if (chip->irq <= 0) {
        dev_err(&client->dev, "dts can't find IRQ, PAD_PM_LED0 is by default\n");
        err = -ENODEV;
        //chip->irq = PAD_FUART_CTS;//for debug
    }
    else
    {
        int irq_num = 0;
        printk(KERN_INFO "pmu_irq_gpio[%d] ...\n", chip->irq);
        if (gpio_direction_input(chip->irq) < 0) {
            dev_err(&client->dev, "gpio_direction_input[%d] failed...\n", chip->irq);
        }
        // Register a ISR
        irq_num = gpio_to_irq(chip->irq);
        err = request_irq(irq_num, ip6303_isr, IRQF_TRIGGER_HIGH/*IRQF_SHARED*/, "pmu isr", chip);
        if (err != 0) {
            dev_err(&client->dev, "request pmu isr failed (irq: %d, errno:%d)\n", chip->irq, err);
            err = -ENODEV;
        }
    }

    /* Sys Attribute Register */
    err = sysfs_create_group(&client->dev.kobj, &ip6303_attr_group);
    if (!err) {
        dev_err(&client->dev,"create device file failed!\n");
    }

    g_chip = chip;
    pm_power_off = ip6303_poweroff_enter;

	INIT_DEFERRABLE_WORK(&chip->work, ip6303_work);
	queue_delayed_work(system_power_efficient_wq, &chip->work, IP6303_DELAY);

	return 0;
}

static int ip6303_remove(struct i2c_client *client)
{
	struct ip6303_chip *chip = i2c_get_clientdata(client);
	power_supply_unregister(chip->battery);
	cancel_delayed_work(&chip->work);
    sysfs_remove_group(&client->dev.kobj, &ip6303_attr_group);
	return 0;
}

#ifdef CONFIG_PM_SLEEP

static int ip6303_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ip6303_chip *chip = i2c_get_clientdata(client);

	cancel_delayed_work(&chip->work);
	return 0;
}

static int ip6303_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct ip6303_chip *chip = i2c_get_clientdata(client);

	queue_delayed_work(system_power_efficient_wq, &chip->work,  IP6303_DELAY);
	return 0;
}

static SIMPLE_DEV_PM_OPS(ip6303_pm_ops, ip6303_suspend, ip6303_resume);
#define IP6303_PM_OPS (&ip6303_pm_ops)

#else

#define IP6303_PM_OPS NULL

#endif /* CONFIG_PM_SLEEP */

static const struct i2c_device_id ip6303_id[] = {
	{ "ip6303" },
	{ }
};
MODULE_DEVICE_TABLE(i2c, ip6303_id);

static const struct of_device_id Injoinic_of_match[] = {
    { .compatible = "Injoinic,ip6303", },
    { }
};

static struct i2c_driver ip6303_i2c_driver = {
    .class = I2C_CLASS_HWMON,
	.driver	= {
		.name	= "ip6303",
		.pm	= IP6303_PM_OPS,
		.of_match_table = Injoinic_of_match,
	},
	.probe		= ip6303_probe,
	.remove		= ip6303_remove,
	.id_table	= ip6303_id,
};
module_i2c_driver(ip6303_i2c_driver);

MODULE_AUTHOR("QinLei <lei.qin@sigmastar.com.cn>");
MODULE_DESCRIPTION("IP6303 Fuel Gauge");
MODULE_LICENSE("GPL");
