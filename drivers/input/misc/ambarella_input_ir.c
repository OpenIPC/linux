/*
 * drivers/input/misc/ambarella_input_ir.c
 *
 * Author: Qiao Wang <qwang@ambarella.com>
 * Copyright (C) 2004-2009, Ambarella, Inc.
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
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <plat/ir.h>

#define MAX_IR_BUFFER			512
#define HW_FIFO_BUFFER			48

struct ambarella_ir_keymap {
	u32 key_code;
	u32 key_raw;
};

struct ambarella_ir_frame_info {
	u32				frame_head_size;
	u32				frame_data_size;
	u32				frame_end_size;
	u32				frame_repeat_head_size;
};

struct ambarella_ir_info {
	struct input_dev		*input;
	unsigned char __iomem 		*regbase;
	unsigned int			irq;

	int (*ir_parse)(struct ambarella_ir_info *pirinfo, u32 *uid);
	int				ir_pread;
	int				ir_pwrite;
	u16				tick_buf[MAX_IR_BUFFER];
	struct ambarella_ir_frame_info 	frame_info;
	u32				frame_data_to_received;


	struct ambarella_ir_keymap	*keymap;
	u32				key_num;

	u32				protocol;
	u32				print_key : 1;
};

/* ========================================================================= */
static u16 ambarella_ir_read_data(
	struct ambarella_ir_info *pirinfo, int pointer)
{
	BUG_ON(pointer < 0);
	BUG_ON(pointer >= MAX_IR_BUFFER);
	BUG_ON(pointer == pirinfo->ir_pwrite);

	return pirinfo->tick_buf[pointer];
}

static int ambarella_ir_get_tick_size(struct ambarella_ir_info *pirinfo)
{
	int				size = 0;

	if (pirinfo->ir_pread > pirinfo->ir_pwrite)
		size = MAX_IR_BUFFER - pirinfo->ir_pread + pirinfo->ir_pwrite;
	else
		size = pirinfo->ir_pwrite - pirinfo->ir_pread;

	return size;
}

void ambarella_ir_inc_read_ptr(struct ambarella_ir_info *pirinfo)
{
	BUG_ON(pirinfo->ir_pread == pirinfo->ir_pwrite);

	pirinfo->ir_pread++;
	if (pirinfo->ir_pread >= MAX_IR_BUFFER)
		pirinfo->ir_pread = 0;
}

void ambarella_ir_move_read_ptr(struct ambarella_ir_info *pirinfo, int offset)
{
	for (; offset > 0; offset--) {
		ambarella_ir_inc_read_ptr(pirinfo);
	}
}

/* ========================================================================= */
#include "ambarella_ir_nec.c"
#include "ambarella_ir_sony.c"
#include "ambarella_ir_philips.c"
#include "ambarella_ir_panasonic.c"

/* ========================================================================= */
static int ambarella_input_report_ir(struct ambarella_ir_info *pirinfo, u32 uid)
{
	struct ambarella_ir_keymap *keymap = pirinfo->keymap;
	struct input_dev *input = pirinfo->input;
	int i;

	for (i = 0; i < pirinfo->key_num; i++, keymap++) {
		if (keymap->key_raw == uid) {
			input_report_key(input, keymap->key_code, 1);
			input_sync(input);
			input_report_key(input, keymap->key_code, 0);
			input_sync(input);
			if(pirinfo->print_key)
				dev_info(&input->dev, "IR_KEY [%d]\n", keymap->key_code);
			break;
		}
	}

	return 0;
}

static inline void ambarella_ir_write_data(struct ambarella_ir_info *pirinfo,
	u16 val)
{
	BUG_ON(pirinfo->ir_pwrite < 0);
	BUG_ON(pirinfo->ir_pwrite >= MAX_IR_BUFFER);

	pirinfo->tick_buf[pirinfo->ir_pwrite] = val;

	pirinfo->ir_pwrite++;

	if (pirinfo->ir_pwrite >= MAX_IR_BUFFER)
		pirinfo->ir_pwrite = 0;
}

static inline int ambarella_ir_update_buffer(struct ambarella_ir_info *pirinfo)
{
	int				count;
	int				size;

	count = amba_readl(pirinfo->regbase + IR_STATUS_OFFSET);
	dev_dbg(&pirinfo->input->dev, "size we got is [%d]\n", count);
	for (; count > 0; count--) {
		ambarella_ir_write_data(pirinfo,
			amba_readl(pirinfo->regbase + IR_DATA_OFFSET));
	}
	size = ambarella_ir_get_tick_size(pirinfo);

	return size;
}

static irqreturn_t ambarella_ir_irq(int irq, void *devid)
{
	struct ambarella_ir_info *pirinfo;
	u32 ctrl_val, uid, edges;
	int rval;

	pirinfo = (struct ambarella_ir_info *)devid;

	BUG_ON(pirinfo->ir_pread < 0);
	BUG_ON(pirinfo->ir_pread >= MAX_IR_BUFFER);

	ctrl_val = amba_readl(pirinfo->regbase + IR_CONTROL_OFFSET);
	if (ctrl_val & IR_CONTROL_FIFO_OV) {
		while (amba_readl(pirinfo->regbase + IR_STATUS_OFFSET) > 0)
			amba_readl(pirinfo->regbase + IR_DATA_OFFSET);

		amba_setbitsl(pirinfo->regbase + IR_CONTROL_OFFSET,
					IR_CONTROL_FIFO_OV);

		dev_err(&pirinfo->input->dev,
			"IR_CONTROL_FIFO_OV overflow\n");

		goto ambarella_ir_irq_exit;
	}

	ambarella_ir_update_buffer(pirinfo);

	if(!pirinfo->ir_parse)
		goto ambarella_ir_irq_exit;

	rval = pirinfo->ir_parse(pirinfo, &uid);

	if (rval == 0) {
		/* yes, we find the key */
		if(pirinfo->print_key)
			dev_info(&pirinfo->input->dev, "uid = 0x%08x\n", uid);
		ambarella_input_report_ir(pirinfo, uid);
	}

	pirinfo->frame_data_to_received = pirinfo->frame_info.frame_data_size +
		pirinfo->frame_info.frame_head_size;
	pirinfo->frame_data_to_received -= ambarella_ir_get_tick_size(pirinfo);

	if (pirinfo->frame_data_to_received <= HW_FIFO_BUFFER) {
		edges = pirinfo->frame_data_to_received;
		pirinfo->frame_data_to_received = 0;
	} else {// > HW_FIFO_BUFFER
		edges = HW_FIFO_BUFFER;
		pirinfo->frame_data_to_received -= HW_FIFO_BUFFER;
	}

	dev_dbg(&pirinfo->input->dev,
		"line[%d],frame_data_to_received[%d]\n", __LINE__, edges);
	amba_clrbitsl(pirinfo->regbase + IR_CONTROL_OFFSET,
		IR_CONTROL_INTLEV(0x3F));
	amba_setbitsl(pirinfo->regbase + IR_CONTROL_OFFSET,
		IR_CONTROL_INTLEV(edges));

ambarella_ir_irq_exit:
	amba_writel(pirinfo->regbase + IR_CONTROL_OFFSET,
		(amba_readl(pirinfo->regbase + IR_CONTROL_OFFSET) |
		IR_CONTROL_LEVINT));

	return IRQ_HANDLED;
}

void ambarella_ir_enable(struct ambarella_ir_info *pirinfo)
{
	u32 edges;

	pirinfo->frame_data_to_received = pirinfo->frame_info.frame_head_size
		+ pirinfo->frame_info.frame_data_size;

	BUG_ON(pirinfo->frame_data_to_received > MAX_IR_BUFFER);

	if (pirinfo->frame_data_to_received > HW_FIFO_BUFFER) {
		edges = HW_FIFO_BUFFER;
		pirinfo->frame_data_to_received -= HW_FIFO_BUFFER;
	} else {
		edges = pirinfo->frame_data_to_received;
		pirinfo->frame_data_to_received = 0;
	}
	amba_writel(pirinfo->regbase + IR_CONTROL_OFFSET, IR_CONTROL_RESET);
	amba_setbitsl(pirinfo->regbase + IR_CONTROL_OFFSET,
		IR_CONTROL_ENB | IR_CONTROL_INTLEV(edges) | IR_CONTROL_INTENB);

	enable_irq(pirinfo->irq);
}

void ambarella_ir_disable(struct ambarella_ir_info *pirinfo)
{
	disable_irq(pirinfo->irq);
}

void ambarella_ir_set_protocol(struct ambarella_ir_info *pirinfo,
	enum ambarella_ir_protocol protocol_id)
{
	memset(pirinfo->tick_buf, 0x0, sizeof(pirinfo->tick_buf));
	pirinfo->ir_pread  = 0;
	pirinfo->ir_pwrite = 0;

	switch (protocol_id) {
	case AMBA_IR_PROTOCOL_NEC:
		dev_notice(&pirinfo->input->dev,
			"Protocol NEC[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_nec_parse;
		ambarella_ir_get_nec_info(&pirinfo->frame_info);
		break;
	case AMBA_IR_PROTOCOL_PANASONIC:
		dev_notice(&pirinfo->input->dev,
			"Protocol PANASONIC[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_panasonic_parse;
		ambarella_ir_get_panasonic_info(&pirinfo->frame_info);
		break;
	case AMBA_IR_PROTOCOL_SONY:
		dev_notice(&pirinfo->input->dev,
			"Protocol SONY[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_sony_parse;
		ambarella_ir_get_sony_info(&pirinfo->frame_info);
		break;
	case AMBA_IR_PROTOCOL_PHILIPS:
		dev_notice(&pirinfo->input->dev,
			"Protocol PHILIPS[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_philips_parse;
		ambarella_ir_get_philips_info(&pirinfo->frame_info);
		break;
	default:
		dev_notice(&pirinfo->input->dev,
			"Protocol default NEC[%d]\n", protocol_id);
		pirinfo->ir_parse = ambarella_ir_nec_parse;
		ambarella_ir_get_nec_info(&pirinfo->frame_info);
		break;
	}
}

static void ambarella_ir_init(struct ambarella_ir_info *pirinfo)
{
	ambarella_ir_disable(pirinfo);

	clk_set_rate(clk_get(NULL, "gclk_ir"), 13000);

	ambarella_ir_set_protocol(pirinfo, pirinfo->protocol);

	ambarella_ir_enable(pirinfo);
}

static int ambarella_ir_of_parse(struct platform_device *pdev,
					struct ambarella_ir_info *pirinfo)
{
	struct device_node *np = pdev->dev.of_node;
	struct ambarella_ir_keymap *keymap;
	const __be32 *prop;
	int rval, i, size;

	rval = of_property_read_u32(np, "amb,protocol", &pirinfo->protocol);
	if (rval < 0 || pirinfo->protocol >= AMBA_IR_PROTOCOL_END)
		pirinfo->protocol = AMBA_IR_PROTOCOL_NEC;

	pirinfo->print_key = !!of_find_property(np, "amb,print-key", NULL);

	prop = of_get_property(np, "amb,keymap", &size);
	if (!prop || size % (sizeof(__be32) * 2)) {
		dev_err(&pdev->dev, "Invalid keymap!\n");
		return -ENOENT;
	}

	/* cells is 2 for each keymap */
	size /= sizeof(__be32) * 2;

	pirinfo->key_num = size;
	pirinfo->keymap = devm_kzalloc(&pdev->dev,
			sizeof(struct ambarella_ir_keymap) * size, GFP_KERNEL);
	if (pirinfo->keymap == NULL){
		dev_err(&pdev->dev, "No memory!\n");
		return -ENOMEM;
	}

	keymap = pirinfo->keymap;
	for (i = 0; i < size; i++) {
		keymap->key_raw = be32_to_cpup(prop + i * 2);
		keymap->key_code = be32_to_cpup(prop + i * 2 + 1);
		input_set_capability(pirinfo->input, EV_KEY, keymap->key_code);
		keymap++;
	}

	return 0;
}

static int ambarella_ir_probe(struct platform_device *pdev)
{
	struct ambarella_ir_info *pirinfo;
	struct input_dev *input;
	struct resource *mem;
	int retval;

	pirinfo = devm_kzalloc(&pdev->dev,
			sizeof(struct ambarella_ir_info), GFP_KERNEL);
	if (!pirinfo) {
		dev_err(&pdev->dev, "Failed to allocate pirinfo!\n");
		return -ENOMEM;
	}

	mem = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (mem == NULL) {
		dev_err(&pdev->dev, "Get mem resource failed!\n");
		return -ENXIO;
	}

	pirinfo->regbase = devm_ioremap(&pdev->dev,
					mem->start, resource_size(mem));
	if (!pirinfo->regbase) {
		dev_err(&pdev->dev, "devm_ioremap() failed\n");
		return -ENOMEM;
	}

	pirinfo->irq = platform_get_irq(pdev, 0);
	if (pirinfo->irq < 0) {
		dev_err(&pdev->dev, "Get irq failed!\n");
		return -ENXIO;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "input_allocate_device fail!\n");
		return -ENOMEM;
	}

	input->name = "AmbIR";
	input->phys = "ambir/input0";
	input->id.bustype = BUS_HOST;

	retval = input_register_device(input);
	if (retval) {
		dev_err(&pdev->dev, "Register input_dev failed!\n");
		goto ir_err0;
	}

	pirinfo->input = input;

	retval = ambarella_ir_of_parse(pdev, pirinfo);
	if (retval)
		goto ir_err1;

	platform_set_drvdata(pdev, pirinfo);

	retval = devm_request_irq(&pdev->dev, pirinfo->irq,
			ambarella_ir_irq, IRQF_TRIGGER_HIGH,
			dev_name(&pdev->dev), pirinfo);
	if (retval < 0) {
		dev_err(&pdev->dev, "Request IRQ failed!\n");
		goto ir_err1;
	}

	ambarella_ir_init(pirinfo);

	dev_notice(&pdev->dev, "IR Host Controller probed!\n");

	return 0;

ir_err1:
	input_unregister_device(input);
ir_err0:
	input_free_device(input);
	return retval;
}

static int ambarella_ir_remove(struct platform_device *pdev)
{
	struct ambarella_ir_info *pirinfo;
	int retval = 0;

	pirinfo = platform_get_drvdata(pdev);

	input_unregister_device(pirinfo->input);
	input_free_device(pirinfo->input);

	dev_notice(&pdev->dev, "Remove Ambarella IR Host Controller.\n");

	return retval;
}

#if (defined CONFIG_PM)
static int ambarella_ir_suspend(struct platform_device *pdev,
	pm_message_t state)
{
	int					retval = 0;
	struct ambarella_ir_info		*pirinfo;

	pirinfo = platform_get_drvdata(pdev);

	disable_irq(pirinfo->irq);
	amba_clrbitsl(pirinfo->regbase + IR_CONTROL_OFFSET, IR_CONTROL_INTENB);

	dev_dbg(&pdev->dev, "%s exit with %d @ %d\n",
		__func__, retval, state.event);
	return retval;
}

static int ambarella_ir_resume(struct platform_device *pdev)
{
	int					retval = 0;
	struct ambarella_ir_info		*pirinfo;

	pirinfo = platform_get_drvdata(pdev);

	clk_set_rate(clk_get(NULL, "gclk_ir"), 13000);
	ambarella_ir_enable(pirinfo);

	dev_dbg(&pdev->dev, "%s exit with %d\n", __func__, retval);

	return retval;
}
#endif

static const struct of_device_id ambarella_ir_dt_ids[] = {
	{.compatible = "ambarella,ir", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_ir_dt_ids);

static struct platform_driver ambarella_ir_driver = {
	.probe		= ambarella_ir_probe,
	.remove		= ambarella_ir_remove,
#if (defined CONFIG_PM)
	.suspend	= ambarella_ir_suspend,
	.resume		= ambarella_ir_resume,
#endif
	.driver		= {
		.name	= "ambarella-ir",
		.owner	= THIS_MODULE,
		.of_match_table = ambarella_ir_dt_ids,
	},
};

module_platform_driver(ambarella_ir_driver);

MODULE_AUTHOR("Cao Rongrong <rrcao@ambarella.com>");
MODULE_DESCRIPTION("Ambarella IR Input Driver");
MODULE_LICENSE("GPL");

