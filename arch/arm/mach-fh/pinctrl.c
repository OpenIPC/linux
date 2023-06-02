#ifdef CONFIG_MACH_FH8830
#ifdef CONFIG_MACH_FH8830_QFN
#include <mach/fh8830_iopad_qfn.h>
#else
#include <mach/fh8830_iopad_bga.h>
#endif
#endif

#ifdef CONFIG_MACH_FH8833
#include <mach/fh8833_iopad_mipi.h>
#endif

#include <mach/pinctrl.h>
#include <linux/module.h>

/* #define  FH_PINCTRL_DEBUG */
#ifdef FH_PINCTRL_DEBUG
#define PRINT_DBG(fmt,args...)  OS_PRINT(fmt,##args)
#else
#define PRINT_DBG(fmt,args...)  do{} while(0)
#endif

static PinCtrl_Object pinctrl_obj;
OS_LIST fh_pinctrl_devices = OS_LIST_INIT(fh_pinctrl_devices);

static int fh_pinctrl_func_select(PinCtrl_Pin *pin, unsigned int flag)
{
    unsigned int reg;

    if(!pin)
    {
        OS_PRINT("ERROR: pin is null\n\n");
        return -1;
    }

    if(flag & NEED_CHECK_PINLIST)
    {
        if(pinctrl_obj.pinlist[pin->pad_id])
        {
            OS_PRINT("ERROR: pad %d has already been set\n\n", pin->pad_id);
            return -2;
        }
    }

    reg = GET_REG(pinctrl_obj.vbase + pin->reg_offset);

    pin->reg = (PinCtrl_Register *)&reg;

    pin->reg->bit.mfs = pin->func_sel;

    if(pin->pullup_pulldown == PUPD_DOWN)
    {
	    pin->reg->bit.pdn = 0;
    }
    else if(pin->pullup_pulldown == PUPD_UP)
    {
	    pin->reg->bit.pun = 0;
    }
    else
    {
	    pin->reg->bit.pdn = 1;
	    pin->reg->bit.pun = 1;
    }

    pin->reg->bit.ie = 1;

    SET_REG(pinctrl_obj.vbase + pin->reg_offset, pin->reg->dw);

    pinctrl_obj.pinlist[pin->pad_id] = pin;

    return 0;
}

static int fh_pinctrl_mux_switch(PinCtrl_Mux *mux, unsigned int flag)
{
    if(mux->cur_pin > MUX_NUM)
    {
        OS_PRINT("ERROR: selected function is not exist, sel_func=%d\n\n", mux->cur_pin);
        return -3;
    }

    if(!mux->mux_pin[mux->cur_pin])
    {
        OS_PRINT("ERROR: mux->mux_pin[%d] has no pin\n\n", mux->cur_pin);
        return -4;
    }

    PRINT_DBG("\t%s[%d]\n", mux->mux_pin[mux->cur_pin]->func_name, mux->cur_pin);
    return fh_pinctrl_func_select(mux->mux_pin[mux->cur_pin], flag);
}


static int fh_pinctrl_device_switch(PinCtrl_Device *dev, unsigned int flag)
{
    int i, ret;
    for(i=0; i<dev->mux_count; i++)
    {
        unsigned int *mux_addr = (unsigned int *)((unsigned int)dev
                + sizeof(*dev) - 4 + i*4);
        PinCtrl_Mux *mux = (PinCtrl_Mux *)(*mux_addr);

        ret = fh_pinctrl_mux_switch(mux, flag);
        if(ret)
        {
            return ret;
        }
    }

    return 0;
}

static PinCtrl_Device * fh_pinctrl_get_device_by_name(char *name)
{
    PinCtrl_Device *dev = OS_NULL;

    list_for_each_entry(dev, &fh_pinctrl_devices, list)
    {
        if(!strcmp(name, dev->dev_name))
        {
            return dev;
        }
    }

    return 0;
}

int fh_pinctrl_check_pinlist(void)
{
    int i;
    for(i=0; i<PAD_NUM; i++)
    {
        if(!pinctrl_obj.pinlist[i])
        {
        	PRINT_DBG("ERROR: pad %d is still empty\n", i);
        }
    }

    return 0;
}

static int fh_pinctrl_init_devices(char** devlist, int listsize, unsigned int flag)
{
    int i, ret;
    PinCtrl_Device *dev;

    memset(pinctrl_obj.pinlist, 0, sizeof(pinctrl_obj.pinlist));

    for(i=0; i<listsize; i++)
    {
        dev = fh_pinctrl_get_device_by_name(devlist[i]);

        if(!dev)
        {
            OS_PRINT("ERROR: cannot find device %s\n", devlist[i]);
            return -5;
        }

        PRINT_DBG("%s:\n", dev->dev_name);
        ret = fh_pinctrl_device_switch(dev, flag);
        PRINT_DBG("\n");
        if(ret)
        {
            return ret;
        }

    }

    fh_pinctrl_check_pinlist();

    return 0;

}

static void fh_pinctrl_init_pin(void)
{
    int i;

    for(i=0; i<PAD_NUM; i++)
    {
        PinCtrl_Pin *pin = pinctrl_obj.pinlist[i];
        if(!pin)
        {
        	unsigned int reg;
        	PRINT_DBG("ERROR: pad %d is empty\n", i);
        	reg = GET_REG(pinctrl_obj.vbase + i * 4);
        	reg &= ~(0x1000);
        	SET_REG(pinctrl_obj.vbase + i * 4, reg);
        	continue;
        }
        pin->reg->dw = GET_REG(pinctrl_obj.vbase +
                pin->reg_offset);

        pin->input_enable = pin->reg->bit.ie;
        pin->output_enable = pin->reg->bit.oe;
    }
}


void fh_pinctrl_init(unsigned int base)
{
    pinctrl_obj.vbase = pinctrl_obj.pbase = (void *)base;

    fh_pinctrl_init_devicelist(&fh_pinctrl_devices);
    fh_pinctrl_init_devices(fh_pinctrl_selected_devices,
            ARRAY_SIZE(fh_pinctrl_selected_devices),
            NEED_CHECK_PINLIST);
    fh_pinctrl_init_pin();
}

void fh_pinctrl_prt(struct seq_file *sfile)
{
    int i;
    seq_printf(sfile, "%2s\t%8s\t%4s\t%8s\t%4s\t%4s\t%4s\t%4s\n",
		    "id", "name", "addr", "reg", "sel", "ie", "oe", "pupd");
    for(i=0; i<PAD_NUM; i++)
    {
        if(!pinctrl_obj.pinlist[i])
        {
                OS_PRINT("ERROR: pad %d is empty\n", i);
		continue;
        }
        seq_printf(sfile, "%02d\t%8s\t0x%08x\t0x%08x\t%04d\t%04d\t%04d\t%04d\n",
                pinctrl_obj.pinlist[i]->pad_id,
                pinctrl_obj.pinlist[i]->func_name,
                pinctrl_obj.pinlist[i]->reg_offset + 0xf0000080,
                GET_REG(pinctrl_obj.vbase + pinctrl_obj.pinlist[i]->reg_offset),
                pinctrl_obj.pinlist[i]->func_sel,
                pinctrl_obj.pinlist[i]->input_enable,
                pinctrl_obj.pinlist[i]->output_enable,
                pinctrl_obj.pinlist[i]->pullup_pulldown);
    }

}


int fh_pinctrl_smux(char *devname, char* muxname, int muxsel, unsigned int flag)
{
    PinCtrl_Device *dev;
    int i, ret;

    dev = fh_pinctrl_get_device_by_name(devname);

    if(!dev)
    {
        OS_PRINT("ERROR: cannot find device %s\n", devname);
        return -4;
    }

    for(i=0; i<dev->mux_count; i++)
    {
        unsigned int *mux_addr = (unsigned int *)((unsigned int)dev
                + sizeof(*dev) - 4 + i*4);
        PinCtrl_Mux *mux = (PinCtrl_Mux *)(*mux_addr);

        if(!strcmp(muxname, mux->mux_pin[0]->func_name))
        {
            mux->cur_pin = muxsel;
            ret = fh_pinctrl_mux_switch(mux, flag);
            return ret;
        }
    }

    if(i == dev->mux_count)
    {
        OS_PRINT("ERROR: cannot find mux %s of device %s\n", muxname, devname);
        return -6;
    }

    fh_pinctrl_check_pinlist();

    return 0;
}
EXPORT_SYMBOL(fh_pinctrl_smux);

int fh_pinctrl_sdev(char *devname, unsigned int flag)
{
    PinCtrl_Device *dev;
    int ret;

    dev = fh_pinctrl_get_device_by_name(devname);
    if(!dev)
    {
        OS_PRINT("ERROR: cannot find device %s\n", devname);
        return -7;
    }

    OS_PRINT("%s:\n", dev->dev_name);
    ret = fh_pinctrl_device_switch(dev, flag);
    OS_PRINT("\n");
    if(ret)
    {
        return ret;
    }

    fh_pinctrl_check_pinlist();

    return 0;
}
EXPORT_SYMBOL(fh_pinctrl_sdev);
