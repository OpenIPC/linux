#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/clk-provider.h>
#include <linux/gpio.h>

#include "ms_platform.h"
#include "ms_types.h"
#include "infinity3/registers.h"
#include "infinity3/irqs.h"
#include <linux/suspend.h>


extern U8 MDrv_GPIO_Pad_Read(U8 u8IndexGPIO);
int detect_gpio=-1;
int from_resend=0;

static irqreturn_t ms_pm_deepsleep_handler(int irq, void *dev_id)
{
#if 0

    struct platform_device *pdev = dev_id;
    struct irq_data *data;


    dev_info(&pdev->dev, "[%s] ++\n", __func__);

    if(detect_gpio == -1)
        return IRQ_NONE;

    if(from_resend == 1)
    {
        from_resend = 0;
        return IRQ_HANDLED;
    }


    data = irq_get_irq_data(irq);


    data = irq_get_irq_data(irq);
    //data->chip->irq_set_type(data, 0);  //set polarity, get interrupt when L->H
    data->chip->irq_set_type(data, 1);  //set polarity, get interrupt when H->L

    //clear interrupt
    SETREG16(BASE_REG_PMGPIO_PA + REG_ID_00, BIT6);
    //unmask interrupt
    CLRREG16(BASE_REG_PMGPIO_PA + REG_ID_00, BIT4);

#ifndef CONFIG_ARCH_INFINITY3 //no need in I3
    //power down eth
    //wriu  -w  0x0032fc  0x0102   // Power-down LDO
    //wriu      0x0032b7  0x17     // Power-down ADC
    //wriu      0x0032cb  0x13     // Power-down BGAP
    //wriu      0x0032cc  0x30     // Power-down ADCPL
    //wriu      0x0032cd  0xd8     // Power-down ADCPL
    //wriu      0x0032d4  0x20     // Power-down LPF_OP
    //wriu      0x0032b9  0x41     // Power-down LPF
    //wriu      0x0032bb  0x84     // Power-down REF
    //wriu  -w  0x00333a  0x03f3   // PD_TX_IDAC, PD_TX_LD
    //wriu      0x0033a1  0x20     // PD_SADC, EN_SAR_LOGIC**
    //wriu      0x0033c5  0x40     // 100gat
    //wriu      0x003330  0x53     // 200gat
    OUTREG16(0x1F0065F8, 0x0102);
    OUTREG8 (0x1F00656D, 0x17);
    OUTREG8 (0x1F006595, 0x13);
    OUTREG8 (0x1F006598, 0x30);
    OUTREG8 (0x1F006599, 0xd8);
    OUTREG8 (0x1F0065A8, 0x20);
    OUTREG8 (0x1F006571, 0x41);
    OUTREG8 (0x1F006575, 0x84);
    OUTREG16(0x1F006674, 0x03f3);
    OUTREG8 (0x1F006741, 0x20);
    OUTREG8 (0x1F006789, 0x40);
    OUTREG8 (0x1F006660, 0x53);

    //Set DVDD_NODIE to 0.95V
    //OUTREG8(BASE_REG_PMSLEEP_PA + REG_ID_62, 0xA0);
#endif

    //Switch reg_ckg_mcu/reg_ckg_spi to xtal, or it will hang when resume
    OUTREG16(BASE_REG_PMSLEEP_PA + REG_ID_20, 0x0);

    OUTREG8(0x1F002E1C, 0x03);     //reg_spi_arb_ctrl[1:0]
    OUTREG16(0x1F002E20, 0x007F);  //reg_non_pm_ack_timeout_len[15:0]
    OUTREG8(0x1F007848, 0x30);     //reg_pwoff_rst0_en, reg_pwoff_rst1_en

    //reg_ckg_pm_sleep
    OUTREG16(0x1F001C88, ((INREG16(0x1F001C88)&(~0x7C00))|0x1000));

    //Enter deepsleep
    SETREG16(BASE_REG_PMSLEEP_PA + REG_ID_09, BIT15);
    OUTREG16(BASE_REG_PMSLEEP_PA + REG_ID_32, 0x9f8e);
    OUTREG16(BASE_REG_PMSLEEP_PA + REG_ID_33, 0x9f8e);
    OUTREG8(BASE_REG_PMSLEEP_PA + REG_ID_37, 0xa5);
    OUTREG8(BASE_REG_PMSLEEP_PA + REG_ID_1C, 0x0c);
    OUTREG16(BASE_REG_PMSLEEP_PA + REG_ID_12, 0xbabe);
    OUTREG8(BASE_REG_PMGPIO_PA + REG_ID_04, 0x10);

    /* we'll never reach here because power down */
#endif
    return IRQ_HANDLED;
}

#ifdef CONFIG_PM
static int ms_pm_suspend(struct platform_device *pdev, pm_message_t state)
{
    dev_info(&pdev->dev, "[%s] ++\n", __func__);
    return 0;
}

static int ms_pm_resume(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "[%s] ++\n", __func__);
    return 0;
}
#endif

static int ms_pm_remove(struct platform_device *pdev)
{
    dev_info(&pdev->dev, "[%s] ++\n", __func__);

    return 0;
}

static int ms_pm_probe(struct platform_device *pdev)
{
    struct irq_data *data;
    int ret;

    data = irq_get_irq_data(platform_get_irq(pdev, 0));
    //data->chip->irq_set_type(data, 1); //get interrupt when H->L
    data->chip->irq_set_type(data, 0); //get interrupt when L->H
    data->chip->irq_ack(data);

    if((ret = of_property_read_u32(pdev->dev.of_node, "detect-gpio", &detect_gpio)))
    {
        detect_gpio=-1;
        return ret;
    }

    if(MDrv_GPIO_Pad_Read(detect_gpio)==0)
        from_resend=1;

    pr_err("gpio_to_irq %d => %d\n", detect_gpio, gpio_to_irq(detect_gpio));

    if((ret = devm_request_threaded_irq(&pdev->dev, data->irq, ms_pm_deepsleep_handler, NULL, 0, "ms_pm", pdev)))
        return ret;

    dev_info(&pdev->dev, "[%s]: irq=%d, detect-gpio=%d\n", __func__, data->irq, detect_gpio);

    from_resend=0;

    return ret;
}


static const struct of_device_id ms_pm_of_match_table[] = {
    { .compatible = "mstar,infinity-pm" },
    {}
};
MODULE_DEVICE_TABLE(of, ms_pm_of_match_table);

static struct platform_driver ms_pm_driver = {
    .remove = ms_pm_remove,
    .probe = ms_pm_probe,
#ifdef CONFIG_PM
    .suspend = ms_pm_suspend,
    .resume = ms_pm_resume,
#endif
    .driver = {
        .name = "ms_pm_driver",
        .owner = THIS_MODULE,
        .of_match_table = ms_pm_of_match_table,
    },
};

module_platform_driver(ms_pm_driver);
MODULE_LICENSE("GPL");
