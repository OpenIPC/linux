/*
 * drivers/input/misc/ambarella_input_adc.c
 *
 * Author: Qiao Wang <qwang@ambarella.com>
 * Copyright (C) 2004-2009, Ambarella, Inc.
 *
 *  @History        ::
 *      Date        Name             Comments
 *      07/16/2014  Bing-Liang Hu    irq or polling depend on adc irq support
 *      07/21/2014  Cao Rongrong     re-design the mechanism with ADC
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
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <plat/adc.h>

struct ambadc_keymap {
	u32 key_code;
	u32 channel : 4;
	u32 low_level : 12;
	u32 high_level : 12;
};

struct ambarella_adckey {
	struct input_dev *input;
	struct ambadc_client *adc_client;

	struct ambadc_keymap *keymap;
	u32 key_num;
	u32 key_saved[ADC_NUM_CHANNELS]; /* save the key currently pressed */
	u32 print_key : 1;
};

static void ambarella_adckey_filter_out(struct ambadc_client *client,
		struct ambadc_keymap *keymap)
{
	/* we expect the adc level which is out of current key's range. */
	ambarella_adc_set_threshold(client,
		keymap->channel, keymap->low_level, keymap->high_level);
}

static int ambarella_adckey_callback(struct ambadc_client *client,
		u32 ch, u32 level)
{
	struct ambarella_adckey *adckey;
	struct ambadc_keymap *keymap;
	struct input_dev *input;
	u32 i;

	adckey = dev_get_drvdata(client->dev);
	if(adckey == NULL)
		return -EAGAIN;

	keymap = adckey->keymap;
	input = adckey->input;

	for (i = 0; i < adckey->key_num; i++, keymap++) {
		if (ch != keymap->channel
			|| level < keymap->low_level
			|| level > keymap->high_level)
			continue;

		if (adckey->key_saved[ch] == KEY_RESERVED
			&& keymap->key_code != KEY_RESERVED) {
			input_report_key(input, keymap->key_code, 1);
			input_sync(input);
			adckey->key_saved[ch] = keymap->key_code;
			ambarella_adckey_filter_out(client, keymap);

			if (adckey->print_key) {
				dev_info(&input->dev, "key[%d:%d] pressed %d\n",
					ch, adckey->key_saved[ch], level);
			}
			break;
		} else if (adckey->key_saved[ch] != KEY_RESERVED
			&& keymap->key_code == KEY_RESERVED) {
			input_report_key(input, adckey->key_saved[ch], 0);
			input_sync(input);
			adckey->key_saved[ch] = KEY_RESERVED;
			ambarella_adckey_filter_out(client, keymap);

			if (adckey->print_key) {
				dev_info(&input->dev, "key[%d:%d] released %d\n",
					ch, adckey->key_saved[ch], level);
			}
			break;
		}
	}

	return 0;
}

static int ambarella_adckey_of_parse(struct platform_device *pdev,
		struct ambarella_adckey *adckey)
{
	struct device_node *np = pdev->dev.of_node;
	struct ambadc_keymap *keymap;
	const __be32 *prop;
	u32 propval, i, size;

	adckey->print_key = !!of_find_property(np, "amb,print-key", NULL);

	prop = of_get_property(np, "amb,keymap", &size);
	if (!prop || size % (sizeof(__be32) * 2)) {
		dev_err(&pdev->dev, "Invalid keymap!\n");
		return -ENOENT;
	}

	/* cells is 2 for each keymap */
	size /= sizeof(__be32) * 2;
	adckey->key_num = size;

	adckey->keymap = devm_kzalloc(&pdev->dev,
			sizeof(struct ambadc_keymap) * size, GFP_KERNEL);
	if (adckey->keymap == NULL){
		dev_err(&pdev->dev, "No memory for keymap!\n");
		return -ENOMEM;
	}

	keymap = adckey->keymap;

	for (i = 0; i < adckey->key_num; i++, keymap++) {
		propval = be32_to_cpup(prop + i * 2);
		keymap->low_level = propval & 0xfff;
		keymap->high_level = (propval >> 16) & 0xfff;
		keymap->channel = propval >> 28;
		if (keymap->channel >= ADC_NUM_CHANNELS) {
			dev_err(&pdev->dev, "Invalid channel: %d\n", keymap->channel);
			return -EINVAL;
		}

		propval = be32_to_cpup(prop + i * 2 + 1);
		keymap->key_code = propval;

		if (keymap->key_code == KEY_RESERVED)
			ambarella_adckey_filter_out(adckey->adc_client, keymap);

		input_set_capability(adckey->input, EV_KEY, keymap->key_code);
	}

	for (i = 0; i < ADC_NUM_CHANNELS; i++)
		adckey->key_saved[i] = KEY_RESERVED;

	return 0;
}

static int ambarella_adckey_probe(struct platform_device *pdev)
{
	struct ambarella_adckey *adckey;
	struct input_dev *input;
	int rval = 0;

	adckey = devm_kzalloc(&pdev->dev,
			     sizeof(struct ambarella_adckey),
			     GFP_KERNEL);
	if (!adckey) {
		dev_err(&pdev->dev, "Failed to allocate adckey!\n");
		return -ENOMEM;
	}

	input = input_allocate_device();
	if (!input) {
		dev_err(&pdev->dev, "input_allocate_device fail!\n");
		return -ENOMEM;
	}

	input->name = "AmbADCkey";
	input->phys = "ambadckey/input0";
	input->id.bustype = BUS_HOST;
	input->dev.parent = &pdev->dev;

	rval = input_register_device(input);
	if (rval < 0) {
		dev_err(&pdev->dev, "Register input_dev failed!\n");
		goto adckey_probe_err0;
	}

	adckey->input = input;

	adckey->adc_client = ambarella_adc_register_client(&pdev->dev,
					AMBADC_CONTINUOUS,
					ambarella_adckey_callback);
	if (!adckey->adc_client) {
		dev_err(&pdev->dev, "Register adc client failed!\n");
		goto adckey_probe_err1;
	}

	rval = ambarella_adckey_of_parse(pdev, adckey);
	if (rval < 0)
		goto adckey_probe_err2;

	platform_set_drvdata(pdev, adckey);

	dev_info(&pdev->dev, "ADC key input driver probed!\n");

	return 0;

adckey_probe_err2:
	ambarella_adc_unregister_client(adckey->adc_client);
adckey_probe_err1:
	input_unregister_device(adckey->input);
adckey_probe_err0:
	input_free_device(input);
	return rval;
}

static int ambarella_adckey_remove(struct platform_device *pdev)
{
	struct ambarella_adckey *adckey;
	int rval = 0;

	adckey = platform_get_drvdata(pdev);

	ambarella_adc_unregister_client(adckey->adc_client);
	input_unregister_device(adckey->input);
	input_free_device(adckey->input);

	dev_info(&pdev->dev, "Remove Ambarella ADC Key driver.\n");

	return rval;
}

#ifdef CONFIG_PM
static int ambarella_adckey_suspend(struct platform_device *pdev,
		pm_message_t state)
{
	return 0;
}

static int ambarella_adckey_resume(struct platform_device *pdev)
{
	return 0;
}
#endif

static const struct of_device_id ambarella_adckey_dt_ids[] = {
	{.compatible = "ambarella,input_adckey", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_adckey_dt_ids);

static struct platform_driver ambarella_adckey_driver = {
	.probe		= ambarella_adckey_probe,
	.remove		= ambarella_adckey_remove,
#ifdef CONFIG_PM
	.suspend	= ambarella_adckey_suspend,
	.resume		= ambarella_adckey_resume,
#endif
	.driver		= {
		.name	= "ambarella-adckey",
		.owner	= THIS_MODULE,
		.of_match_table = ambarella_adckey_dt_ids,
	},
};

module_platform_driver(ambarella_adckey_driver);

MODULE_AUTHOR("Bing-Liang Hu <blhu@ambarella.com>");
MODULE_DESCRIPTION("Ambarella ADC key Driver");
MODULE_LICENSE("GPL");

