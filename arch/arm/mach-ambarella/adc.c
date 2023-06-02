/*
 * arch/arm/plat-ambarella/generic/ambarella_adc_drv.c
 *
 * Author: Bing-Liang Hu <blhu@ambarella.com>
 *
 * History:
 *	2014/07/21 - [Cao Rongrong] Re-design the mechanism with client
 *
 * Copyright (C) 2014-2019, Ambarella, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <mach/hardware.h>
#include <plat/adc.h>
#include <plat/rct.h>

static DEFINE_MUTEX(client_mutex);
static LIST_HEAD(client_list);
static struct ambadc_host *ambarella_adc;

static ssize_t ambarella_adc_show(struct device *dev,
				  struct device_attribute *attr,
				  char *buf)
{
	ssize_t ret = 0;
	u32 i, data;

	for (i = 0; i < ADC_NUM_CHANNELS; i++) {
		data = ambarella_adc_read_level(i);
		ret += sprintf(&buf[ret], "adc%d=0x%x\n", i, data);
	}

	return ret;
}
static DEVICE_ATTR(adcsys, 0444, ambarella_adc_show, NULL);

static int ambarella_adc_fifo_ctrl(u32 fid, u16 fcid)
{
	u32 reg=0;
	u32 reg_val=0;

	switch(fid) {
	case 0:
		reg = ADC_FIFO_CTRL_0_REG;
		reg_val = ADC_FIFO_OVER_INT_EN
                        | ADC_FIFO_UNDR_INT_EN
                        | ADC_FIFO_TH
			| (fcid << ADC_FIFO_ID_SHIFT)
			| ADC_FIFO_DEPTH;
		break;
	case 1:
		reg = ADC_FIFO_CTRL_1_REG;
		reg_val = ADC_FIFO_OVER_INT_EN
                        | ADC_FIFO_UNDR_INT_EN
                        | ADC_FIFO_TH
			| (fcid << ADC_FIFO_ID_SHIFT)
			| ADC_FIFO_DEPTH;
		break;
	case 2:
		reg = ADC_FIFO_CTRL_2_REG;
		reg_val = ADC_FIFO_OVER_INT_EN
                        | ADC_FIFO_UNDR_INT_EN
                        | ADC_FIFO_TH
			| (fcid << ADC_FIFO_ID_SHIFT)
			| ADC_FIFO_DEPTH;
		break;
		break;
	case 3:
		reg = ADC_FIFO_CTRL_3_REG;
		reg_val = ADC_FIFO_OVER_INT_EN
                        | ADC_FIFO_UNDR_INT_EN
                        | ADC_FIFO_TH
			| (fcid << ADC_FIFO_ID_SHIFT)
			| ADC_FIFO_DEPTH;
		break;
		break;
	default:
		pr_err("%s: invalid fifo NO = %d.\n",
			__func__, fid);
		return -1;
	}

	amba_writel(reg, reg_val);

	amba_writel(ADC_FIFO_CTRL_REG, ADC_FIFO_CONTROL_CLEAR);
	return 0;
}

static void ambarella_adc_enable(void)
{
	/* select adc scaler as clk_adc and power up adc */
	amba_writel(ADC16_CTRL_REG, 0);

#if (ADC_SUPPORT_SLOT == 1)
	/* soft reset adc controller, set slot, and then enable it */
	amba_writel(ADC_CONTROL_REG, ADC_CONTROL_RESET);
	amba_writel(ADC_SLOT_NUM_REG, 0x0);
	amba_writel(ADC_SLOT_PERIOD_REG, 0xffff);
	amba_writel(ADC_SLOT_CTRL_0_REG, 0x0fff);
#endif
        //test fifo read in fifo_0
        if(ambarella_adc->fifo_mode){
                ambarella_adc_fifo_ctrl(0,1);
        }
	amba_setbitsl(ADC_ENABLE_REG, ADC_CONTROL_ENABLE);
}

static void ambarella_adc_disable(void)
{
	amba_clrbitsl(ADC_ENABLE_REG, ADC_CONTROL_ENABLE);
	amba_writel(ADC16_CTRL_REG, ADC_CTRL_SCALER_POWERDOWN | ADC_CTRL_POWERDOWN);
}

static void ambarella_adc_start(void)
{
	amba_setbitsl(ADC_CONTROL_REG, ADC_CONTROL_MODE | ADC_CONTROL_START);
}

static void ambarella_adc_stop(void)
{
	amba_clrbitsl(ADC_CONTROL_REG, ADC_CONTROL_MODE | ADC_CONTROL_START);
}

static void ambarella_adc_reset(void)
{
        ambarella_adc_disable();
        ambarella_adc_enable();
        ambarella_adc_start();
}

struct ambadc_client *ambarella_adc_register_client(struct device *dev,
		u32 mode, ambadc_client_callback callback)
{
	struct ambadc_host *amb_adc = ambarella_adc;
	struct ambadc_client *client;

	if (!dev || (mode == AMBADC_CONTINUOUS && !callback)) {
		dev_err(dev, "dev or callback missing\n");
		return NULL;
	}

	client = kzalloc(sizeof(struct ambadc_client), GFP_KERNEL);
	if (!client) {
		dev_err(dev, "no memory for adc client\n");
		return NULL;
	}

	client->dev = dev;
	client->dev->parent = amb_adc->dev;
	client->mode = mode;
	client->callback = callback;
	client->host = amb_adc;

	mutex_lock(&client_mutex);
	list_add_tail(&client->node, &client_list);
	mutex_unlock(&client_mutex);

	if (mode == AMBADC_CONTINUOUS) {
		amb_adc->keep_start = true;
		ambarella_adc_start();

		if (amb_adc->polling_mode) {
			queue_delayed_work(system_wq,
				&amb_adc->work, msecs_to_jiffies(20));
		}
	}

	return client;
}
EXPORT_SYMBOL(ambarella_adc_register_client);

void ambarella_adc_unregister_client(struct ambadc_client *client)
{
	struct ambadc_host *amb_adc = client->host;
	struct ambadc_client *_client;
	u32 keep_start = 0;

	mutex_lock(&client_mutex);

	list_del(&client->node);
	kfree(client);

	list_for_each_entry(_client, &client_list, node) {
		if (_client->mode == AMBADC_CONTINUOUS) {
			keep_start = 1;
			break;
		}
	}

	mutex_unlock(&client_mutex);

	if (keep_start == 0) {
		amb_adc->keep_start = false;
		ambarella_adc_stop();
	}
}
EXPORT_SYMBOL(ambarella_adc_unregister_client);

int ambarella_adc_read_level(u32 ch)
{
	struct ambadc_host *amb_adc = ambarella_adc;
	int data=0;
        u32 i,count;

	if (ch >= ADC_NUM_CHANNELS)
		return -EINVAL;

	if (amb_adc == NULL)
		return -ENODEV;

	if (!amb_adc->keep_start)
		ambarella_adc_start();

        if(!ambarella_adc->fifo_mode){
                data = amba_readl(ADC_DATA_REG(ch));
        } else {
                count = 0x7ff & amba_readl(ADC_FIFO_STATUS_0_REG);
                if(count > 4)
			count -= 4;

                for(i=0;i<count;i++){
                        data = amba_readl(ADC_FIFO_DATA0_REG + i*4);
                        // printk("call back [0x%x]=0x%x,count=0x%x,0x%x\n",
                        //  (ADC_FIFO_DATA0_REG + i*4),data,count,ch);
                }
                //data = data/count;
        }

	if (!amb_adc->keep_start)
		ambarella_adc_stop();

	return data;
}
EXPORT_SYMBOL(ambarella_adc_read_level);

int ambarella_adc_set_threshold(struct ambadc_client *client,
		u32 ch, u32 low, u32 high)
{
	struct ambadc_client *_client;
	int value, rval = 0;

	if (ch >= ADC_NUM_CHANNELS)
		return -EINVAL;

	if (client->mode != AMBADC_CONTINUOUS) {
		dev_err(client->dev, "Invalid adc mode: %d\n", client->mode);
		return -EINVAL;
	}

	/* interrupt will be happened when level < low OR level > high */
	value = ADC_EN_HI(!!high) | ADC_EN_LO(!!low) |
		    ADC_VAL_HI(high) | ADC_VAL_LO(low);

	mutex_lock(&client_mutex);

	/* it's not allowed that more than one clients want to set different
	 * threshold for the same adc channel. */
	list_for_each_entry(_client, &client_list, node) {
		if (_client == client)
			continue;
		/* check if other clients set the threshold for this channel */
		if (_client->threshold[ch] && (_client->threshold[ch] != value)) {
			if (value == 0) {
				rval = 0;
				goto exit;
			} else {
				rval = -EBUSY;
				goto exit;
			}
		}
	}

        if(!ambarella_adc->fifo_mode)
                amba_writel(ADC_CHAN_INTR_REG(ch), value);

exit:
	if (rval == 0)
		client->threshold[ch] = value;

	mutex_unlock(&client_mutex);

	return rval;
}
EXPORT_SYMBOL(ambarella_adc_set_threshold);

static void ambarella_adc_client_callback(u32 ch)
{
	struct ambadc_client *client;
	u32 data;

	data = ambarella_adc_read_level(ch);

	list_for_each_entry(client, &client_list, node) {
		if (client->callback && client->threshold[ch])
			client->callback(client, ch, data);
	}
}

static void ambarella_adc_polling_work(struct work_struct *work)
{
	struct ambadc_host *amb_adc;
	u32 i;

	amb_adc = container_of(work, struct ambadc_host, work.work);

	for (i = 0; i < ADC_NUM_CHANNELS; i++)
		ambarella_adc_client_callback(i);

	if (amb_adc->keep_start) {
		queue_delayed_work(system_wq,
				&amb_adc->work, msecs_to_jiffies(20));
	}
}

static irqreturn_t ambarella_adc_irq(int irq, void *dev_id)
{
	u32 i, int_src,rval,chanNo;
        struct ambadc_host *amb_adc = dev_id;

        chanNo = 0;
        if(!amb_adc->fifo_mode){
                int_src = amba_readl(ADC_DATA_INTR_TABLE_REG);
                amba_writel(ADC_DATA_INTR_TABLE_REG, int_src);
                chanNo = int_src;
        }else{
                rval = amba_readl(ADC_CTRL_INTR_TABLE_REG);
                if(rval){
                        ambarella_adc_reset();
                        return IRQ_HANDLED;
                }
                rval = amba_readl(ADC_FIFO_INTR_TABLE_REG);
                amba_writel(ADC_FIFO_INTR_TABLE_REG, rval);

                for(i=0;i<ADC_FIFO_NUMBER;i++){
                        if((rval & (1 << i)) == 0)
                                continue;
                        rval = amba_readl(ADC_FIFO_CTRL_X_REG(i));
                        rval = (rval & (0x0000f000)) >> 12;
                        chanNo = 1 << rval;
                }
        }

        for (i = 0; i < ADC_NUM_CHANNELS; i++) {
                if ((chanNo & (1 << i)) == 0)
                        continue;

                ambarella_adc_client_callback(i);
        }

	return IRQ_HANDLED;
}

static int ambarella_adc_probe(struct platform_device *pdev)
{
	struct ambadc_host *amb_adc;
	struct device_node *np = pdev->dev.of_node;
	int ret = 0;

	amb_adc = devm_kzalloc(&pdev->dev,
			sizeof(struct ambadc_host), GFP_KERNEL);
	if (!amb_adc) {
		dev_err(&pdev->dev, "Failed to allocate memory!\n");
		return -ENOMEM;
	}

	ret = of_property_read_u32(np, "clock-frequency", &amb_adc->clk);
	if (ret < 0) {
		dev_err(&pdev->dev, "Get clock-frequency failed!\n");
		return -ENODEV;
	}

        //amb_adc->fifo_mode=1;//test fifo mode

	amb_adc->polling_mode = !!of_find_property(np, "amb,polling-mode", NULL);
	if (amb_adc->polling_mode) {
		INIT_DELAYED_WORK(&amb_adc->work, ambarella_adc_polling_work);
	} else {
		amb_adc->irq = platform_get_irq(pdev, 0);
		if (amb_adc->irq < 0) {
			dev_err(&pdev->dev, "Can not get irq !\n");
			return -ENXIO;
		}

		ret = devm_request_irq(&pdev->dev, amb_adc->irq,
				ambarella_adc_irq, IRQF_TRIGGER_HIGH,
				dev_name(&pdev->dev), amb_adc);
		if (ret < 0) {
			dev_err(&pdev->dev, "Can not request irq %d!\n", amb_adc->irq);
			return -ENXIO;
		}
	}


	ret = clk_set_rate(clk_get(NULL, "gclk_adc"), amb_adc->clk);
	if (ret < 0)
		return ret;

	amb_adc->dev = &pdev->dev;

	ambarella_adc = amb_adc;

	ambarella_adc_enable();
        if(amb_adc->fifo_mode == 1){
		amb_adc->keep_start = true;
		ambarella_adc_start();
	}

	ret = device_create_file(&pdev->dev, &dev_attr_adcsys);
	if (ret != 0) {
		dev_err(&pdev->dev, "can not create file : %d\n", ret);
		return ret;
	}

	platform_set_drvdata(pdev, amb_adc);

	dev_info(&pdev->dev, "Ambarella ADC driver init\n");

	return 0;
}

static int ambarella_adc_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_adcsys);
	ambarella_adc_disable();
	return 0;
}

#ifdef CONFIG_PM
static int ambarella_adc_suspend(struct platform_device *pdev,
				 pm_message_t state)
{
	ambarella_adc_disable();
	return 0;
}

static int ambarella_adc_resume(struct platform_device *pdev)
{
	struct ambadc_host *amb_adc = platform_get_drvdata(pdev);
	struct ambadc_client *client;
	u32 ch;

	clk_set_rate(clk_get(NULL, "gclk_adc"), amb_adc->clk);
	ambarella_adc_enable();

	if (amb_adc->keep_start)
		ambarella_adc_start();

        if(ambarella_adc->fifo_mode)
		goto exit;

	for (ch = 0; ch < ADC_NUM_CHANNELS; ch++) {
		list_for_each_entry(client, &client_list, node) {
			if (client->threshold[ch] == 0)
				continue;
			amba_writel(ADC_CHAN_INTR_REG(ch), client->threshold[ch]);
		}
	}

exit:
	return 0;
}
#endif

static const struct of_device_id ambarella_adc_dt_ids[] = {
	{.compatible = "ambarella,adc", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_adc_dt_ids);

static struct platform_driver ambarella_adc_driver = {
	.driver	= {
		.name	= "ambarella-adc",
		.owner	= THIS_MODULE,
		.of_match_table	= ambarella_adc_dt_ids,
	},
	.probe		= ambarella_adc_probe,
	.remove		= ambarella_adc_remove,
#ifdef CONFIG_PM
	.suspend	= ambarella_adc_suspend,
	.resume		= ambarella_adc_resume,
#endif
};

module_platform_driver(ambarella_adc_driver);
MODULE_AUTHOR("BingLiang Hu <blhu@ambarella.com>");
MODULE_DESCRIPTION("Ambarella ADC Driver");
MODULE_LICENSE("GPL");

