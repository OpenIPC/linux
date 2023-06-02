/*
 * arch/arm/plat-ambarella/generic/gpio.c
 *
 * History:
 *	2007/11/21 - [Grady Chen] created file
 *	2008/01/08 - [Anthony Ginger] Add GENERIC_GPIO support.
 *	2013/10/19 - [Tzu-Jung Lee] Port to vitrtual GPIO support.
 *
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
#include <linux/init.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/device.h>
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/rpmsg.h>

enum VGPIO_CMD {

	VGPIO_REQUEST,
	VGPIO_RELEASE,
	VGPIO_QUERY,
	VGPIO_CONFIG,
	VGPIO_GET,
	VGPIO_SET,
};

struct rpmsg_vgpio_msg {

	int	cmd;
	int	func;
	u32	offset;
	int	value;
	int	result;
};

struct vgpio_chip
{
	struct gpio_chip	chip;
	struct mutex		mtx;
	struct rpmsg_vgpio_msg	msg;
	struct completion	comp;
	struct rpmsg_channel	*rpdev;
};

extern struct vgpio_chip vgpio_chip;

#define to_vgpio_chip(c) \
	container_of(c, struct vgpio_chip, chip)

void rpmsg_vgpio_do_send(void *data)
{
	struct vgpio_chip *vchip = data;
	struct rpmsg_vgpio_msg *msg = &vchip->msg;

	rpmsg_trysend(vchip->rpdev, msg, sizeof(*msg));

	switch (msg->cmd) {
	case VGPIO_REQUEST:
	case VGPIO_RELEASE:
	case VGPIO_QUERY:
	case VGPIO_CONFIG:
	case VGPIO_GET:
	case VGPIO_SET:
		wait_for_completion(&vchip->comp);
		break;
	default:
		pr_err("%s: invalid vgpio command %d.\n", __func__, msg->cmd);
		break;
	}
}

static void rpmsg_vgpio_cb(struct rpmsg_channel *rpdev, void *data, int len,
			   void *priv, u32 src)
{
	struct vgpio_chip *vchip = priv;
	struct rpmsg_vgpio_msg *msg = data;

	vchip->msg = *msg;

	complete(&vchip->comp);
}

static int rpmsg_vgpio_probe(struct rpmsg_channel *rpdev)
{
	int ret = 0;
	struct rpmsg_ns_msg nsm;

	vgpio_chip.rpdev = rpdev;
	rpdev->ept->priv = &vgpio_chip;

	nsm.addr = rpdev->dst;
	memcpy(nsm.name, rpdev->id.name, RPMSG_NAME_SIZE);
	nsm.flags = 0;

	rpmsg_send(rpdev, &nsm, sizeof(nsm));

	mutex_init(&vgpio_chip.mtx);
	init_completion(&vgpio_chip.comp);

	ret = gpiochip_add(&vgpio_chip.chip);
	if (ret)
		pr_err("%s: gpiochip_add(vgpio)fail %d.\n", __func__, ret);

	return ret;
}

static void rpmsg_vgpio_remove(struct rpmsg_channel *rpdev)
{
}

static struct rpmsg_device_id rpmsg_vgpio_id_table[] = {
	{ .name	= "vgpio_ca9_a_and_arm11", },
	{ .name	= "vgpio_ca9_b_and_arm11", },
	{ },
};
MODULE_DEVICE_TABLE(rpmsg, rpmsg_vgpio_id_table);

struct rpmsg_driver rpmsg_vgpio_driver = {
	.drv.name	= KBUILD_MODNAME,
	.drv.owner	= THIS_MODULE,
	.id_table	= rpmsg_vgpio_id_table,
	.probe		= rpmsg_vgpio_probe,
	.callback	= rpmsg_vgpio_cb,
	.remove		= rpmsg_vgpio_remove,
};

/* ==========================================================================*/

static int __init rpmsg_vgpio_init(void)
{
	return register_rpmsg_driver(&rpmsg_vgpio_driver);
}

static void __exit rpmsg_vgpio_fini(void)
{
	unregister_rpmsg_driver(&rpmsg_vgpio_driver);
}

static int rpmsg_vgpio_request(struct gpio_chip *chip, unsigned offset,
			       int request)
{
	struct vgpio_chip *vchip = to_vgpio_chip(chip);
	struct rpmsg_vgpio_msg *msg = &vchip->msg;
	int retval = 0;

	mutex_lock(&vchip->mtx);

	msg->cmd = request ? VGPIO_REQUEST : VGPIO_RELEASE;
	msg->offset = offset;
	rpmsg_vgpio_do_send(vchip);
	retval = msg->result;

	mutex_unlock(&vchip->mtx);

	return retval;
}

static int rpmsg_vgpio_config(struct gpio_chip *chip, u32 offset, int func)
{
	struct vgpio_chip *vchip = to_vgpio_chip(chip);
	struct rpmsg_vgpio_msg *msg = &vchip->msg;
	int retval = 0;

	mutex_lock(&vchip->mtx);

	msg->cmd = VGPIO_CONFIG;
	msg->offset = offset;
	msg->func = func;
	rpmsg_vgpio_do_send(vchip);
	retval = msg->result;

	mutex_unlock(&vchip->mtx);

	return retval;
}

static int rpmsg_vgpio_query(struct gpio_chip *chip, u32 offset)
{
	struct vgpio_chip *vchip = to_vgpio_chip(chip);
	struct rpmsg_vgpio_msg *msg = &vchip->msg;
	int retval = 0;

	mutex_lock(&vchip->mtx);

	msg->cmd = VGPIO_QUERY;
	msg->offset = offset;
	rpmsg_vgpio_do_send(vchip);

	if (msg->func == GPIO_FUNC_SW_INPUT)
		retval = 1;
	else if (msg->func == GPIO_FUNC_SW_OUTPUT)
		retval = 0;
	else
		retval = -EINVAL;
	mutex_unlock(&vchip->mtx);

	return retval;
}

static int rpmsg_vgpio_set(struct gpio_chip *chip, u32 offset, int value)
{
	struct vgpio_chip *vchip = to_vgpio_chip(chip);
	struct rpmsg_vgpio_msg *msg = &vchip->msg;
	int retval = 0;

	mutex_lock(&vchip->mtx);

	msg->cmd = VGPIO_SET;
	msg->offset = offset;
	msg->value = value;
	rpmsg_vgpio_do_send(vchip);
	retval = msg->result;

	mutex_unlock(&vchip->mtx);

	return retval;
}

static int rpmsg_vgpio_get(struct gpio_chip *chip, u32 offset)
{
	struct vgpio_chip *vchip = to_vgpio_chip(chip);
	struct rpmsg_vgpio_msg *msg = &vchip->msg;
	int retval = 0;

	mutex_lock(&vchip->mtx);

	msg->cmd = VGPIO_GET;
	msg->offset = offset;
	rpmsg_vgpio_do_send(vchip);
	retval = msg->value;

	mutex_unlock(&vchip->mtx);

	return retval;
}

/* ==========================================================================*/

static int rpmsg_vgpio_chip_request(struct gpio_chip *chip, unsigned offset)
{
	return rpmsg_vgpio_request(chip, offset, 1);
}

static void rpmsg_vgpio_chip_free(struct gpio_chip *chip, unsigned offset)
{
	rpmsg_vgpio_request(chip, offset, 0);
}

static int rpmsg_vgpio_chip_get_direction(struct gpio_chip *chip,
		                          unsigned offset)
{
	return rpmsg_vgpio_query(chip, offset);
}

static int rpmsg_vgpio_chip_direction_input(struct gpio_chip *chip,
					    unsigned offset)
{
	return rpmsg_vgpio_config(chip, offset, GPIO_FUNC_SW_INPUT);
}

static int rpmsg_vgpio_chip_get(struct gpio_chip *chip, unsigned offset)
{
	return rpmsg_vgpio_get(chip, offset);
}

static int rpmsg_vgpio_chip_direction_output(struct gpio_chip *chip,
					     unsigned offset, int value)
{
	if (rpmsg_vgpio_config(chip, offset, GPIO_FUNC_SW_OUTPUT) < 0)
		return -1;

	return rpmsg_vgpio_set(chip, offset, value);
}

static void rpmsg_vgpio_chip_set(struct gpio_chip *chip, unsigned offset,
				 int value)
{
	rpmsg_vgpio_set(chip, offset, value);
}

static int rpmsg_vgpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	if ((chip->base + offset) < GPIO_MAX_LINES)
		return GPIO_INT_VEC(chip->base + offset);

	return -EINVAL;
}

static void rpmsg_vgpio_chip_dbg_show(struct seq_file *s,
				      struct gpio_chip *chip)
{
	struct vgpio_chip *vchip = to_vgpio_chip(chip);

	int value;
	int dir;
	int i = 0;

	for (i = 0; i < GPIO_MAX_LINES; i++) {
		dir = rpmsg_vgpio_query(chip, i);
		value = rpmsg_vgpio_get(chip, i);
		seq_printf(s, "%2d:", i);
		if (dir > 0)
			seq_printf(s, "  IN, %d\n", value);
		else if (dir == 0)
			seq_printf(s, " OUT, %d\n", value);
		else
			seq_printf(s, "  HW\n");
	}
}

struct vgpio_chip vgpio_chip = {

	.chip = {
		.label			= "vgpio",
		.owner			= THIS_MODULE,
		.request		= rpmsg_vgpio_chip_request,
		.free			= rpmsg_vgpio_chip_free,
		.get_direction		= rpmsg_vgpio_chip_get_direction,
		.direction_input	= rpmsg_vgpio_chip_direction_input,
		.get			= rpmsg_vgpio_chip_get,
		.direction_output	= rpmsg_vgpio_chip_direction_output,
		.set			= rpmsg_vgpio_chip_set,
		.to_irq			= rpmsg_vgpio_to_irq,
		.dbg_show		= rpmsg_vgpio_chip_dbg_show,
		.base			= 0,
		.ngpio			= AMBGPIO_SIZE,
		.can_sleep		= 0,
		.exported		= 0,
	},
};

/* ==========================================================================*/

void ambarella_gpio_config(int id, int func)
{
	u32 offset = id & 0x1f;

	rpmsg_vgpio_config(&vgpio_chip.chip, offset, func);
}
EXPORT_SYMBOL(ambarella_gpio_config);

void ambarella_gpio_set(int id, int value)
{
	u32 offset = id & 0x1f;

	rpmsg_vgpio_set(&vgpio_chip.chip, offset, value);
}
EXPORT_SYMBOL(ambarella_gpio_set);

int ambarella_gpio_get(int id)
{
	u32 offset = id & 0x1f;

	return rpmsg_vgpio_get(&vgpio_chip.chip, offset);
}
EXPORT_SYMBOL(ambarella_gpio_get);
/* ==========================================================================*/
u32 ambarella_gpio_suspend(u32 level) { return 0; }
u32 ambarella_gpio_resume(u32 level) { return 0; }
/* ==========================================================================*/

static void ambarella_gpio_mwait(struct ambarella_gpio_io_info *pinfo,
				 int can_sleep)
{
	if (can_sleep)
		msleep(pinfo->active_delay);
	else
		mdelay(pinfo->active_delay);

}
int ambarella_set_gpio_output_can_sleep(
	struct ambarella_gpio_io_info *pinfo, u32 on, int can_sleep)
{
	int					retval = 0;

	if (pinfo == NULL) {
		pr_err("%s: pinfo is NULL.\n", __func__);
		retval = -1;
		goto ambarella_set_gpio_output_can_sleep_exit;
	}
	if (pinfo->gpio_id < 0 ) {
		pr_debug("%s: wrong gpio id %d.\n", __func__, pinfo->gpio_id);
		retval = -1;
		goto ambarella_set_gpio_output_can_sleep_exit;
	}

	pr_debug("%s: Gpio[%d] %s, level[%s], delay[%dms].\n", __func__,
		pinfo->gpio_id, on ? "ON" : "OFF",
		pinfo->active_level ? "HIGH" : "LOW",
		pinfo->active_delay);
	if (pinfo->gpio_id >= EXT_GPIO(0)) {
		pr_debug("%s: try expander gpio id %d.\n",
			__func__, pinfo->gpio_id);
		if (on) {
			gpio_direction_output(pinfo->gpio_id,
				pinfo->active_level);
		} else {
			gpio_direction_output(pinfo->gpio_id,
				!pinfo->active_level);
		}
	} else {
		ambarella_gpio_config(pinfo->gpio_id, GPIO_FUNC_SW_OUTPUT);
		if (on) {
			ambarella_gpio_set(pinfo->gpio_id,
				pinfo->active_level);
		} else {
			ambarella_gpio_set(pinfo->gpio_id,
				!pinfo->active_level);
		}
	}
	ambarella_gpio_mwait(pinfo, can_sleep);

ambarella_set_gpio_output_can_sleep_exit:
	return retval;
}
EXPORT_SYMBOL(ambarella_set_gpio_output_can_sleep);

u32 ambarella_get_gpio_input_can_sleep(
	struct ambarella_gpio_io_info *pinfo, int can_sleep)
{
	u32					gpio_value = 0;

	if (pinfo == NULL) {
		pr_err("%s: pinfo is NULL.\n", __func__);
		goto ambarella_get_gpio_input_can_sleep_exit;
	}
	if (pinfo->gpio_id < 0 ) {
		pr_debug("%s: wrong gpio id %d.\n", __func__, pinfo->gpio_id);
		goto ambarella_get_gpio_input_can_sleep_exit;
	}

	if (pinfo->gpio_id >= EXT_GPIO(0)) {
		pr_debug("%s: try expander gpio id %d.\n",
			__func__, pinfo->gpio_id);
		gpio_direction_input(pinfo->gpio_id);
		ambarella_gpio_mwait(pinfo, can_sleep);
		gpio_value = gpio_get_value(pinfo->gpio_id);
	} else {
		ambarella_gpio_config(pinfo->gpio_id, GPIO_FUNC_SW_INPUT);
		ambarella_gpio_mwait(pinfo, can_sleep);
		gpio_value = ambarella_gpio_get(pinfo->gpio_id);
	}

	pr_debug("%s: {gpio[%d], level[%s], delay[%dms]} get[%d].\n",
		__func__, pinfo->gpio_id,
		pinfo->active_level ? "HIGH" : "LOW",
		pinfo->active_delay, gpio_value);

ambarella_get_gpio_input_can_sleep_exit:
	return (gpio_value == pinfo->active_level) ? 1 : 0;
}
EXPORT_SYMBOL(ambarella_get_gpio_input_can_sleep);

int ambarella_set_gpio_reset_can_sleep(
	struct ambarella_gpio_io_info *pinfo, int can_sleep)
{
	int					retval = 0;

	if (pinfo == NULL) {
		pr_err("%s: pinfo is NULL.\n", __func__);
		retval = -1;
		goto ambarella_set_gpio_reset_can_sleep_exit;
	}
	if (pinfo->gpio_id < 0 ) {
		pr_debug("%s: wrong gpio id %d.\n", __func__, pinfo->gpio_id);
		retval = -1;
		goto ambarella_set_gpio_reset_can_sleep_exit;
	}

	pr_debug("%s: Reset gpio[%d], level[%s], delay[%dms].\n",
		__func__, pinfo->gpio_id,
		pinfo->active_level ? "HIGH" : "LOW",
		pinfo->active_delay);
	if (pinfo->gpio_id >= EXT_GPIO(0)) {
		pr_debug("%s: try expander gpio id %d.\n",
			__func__, pinfo->gpio_id);
		gpio_direction_output(pinfo->gpio_id, pinfo->active_level);
		ambarella_gpio_mwait(pinfo, can_sleep);

		gpio_direction_output(pinfo->gpio_id, !pinfo->active_level);
		ambarella_gpio_mwait(pinfo, can_sleep);
	} else {
		ambarella_gpio_config(pinfo->gpio_id, GPIO_FUNC_SW_OUTPUT);
		ambarella_gpio_set(pinfo->gpio_id, pinfo->active_level);
		ambarella_gpio_mwait(pinfo, can_sleep);

		ambarella_gpio_set(pinfo->gpio_id, !pinfo->active_level);
		ambarella_gpio_mwait(pinfo, can_sleep);
	}

ambarella_set_gpio_reset_can_sleep_exit:
	return retval;
}
EXPORT_SYMBOL(ambarella_set_gpio_reset_can_sleep);

/* ==========================================================================*/
int ambarella_set_gpio_output(struct ambarella_gpio_io_info *pinfo, u32 on)
{
	return ambarella_set_gpio_output_can_sleep(pinfo, on, 0);
}
EXPORT_SYMBOL(ambarella_set_gpio_output);

u32 ambarella_get_gpio_input(struct ambarella_gpio_io_info *pinfo)
{
	return ambarella_get_gpio_input_can_sleep(pinfo, 0);
}
EXPORT_SYMBOL(ambarella_get_gpio_input);

int ambarella_set_gpio_reset(struct ambarella_gpio_io_info *pinfo)
{
	return ambarella_set_gpio_reset_can_sleep(pinfo, 0);
}
EXPORT_SYMBOL(ambarella_set_gpio_reset);

int ambarella_is_valid_gpio_irq(struct ambarella_gpio_irq_info *pinfo)
{
	int bvalid = 0;

	if (pinfo == NULL) {
		pr_err("%s: pinfo is NULL.\n", __func__);
		goto ambarella_is_valid_gpio_irq_exit;
	}

	if ((pinfo->irq_gpio < 0 ) || (pinfo->irq_gpio >= ARCH_NR_GPIOS))
		goto ambarella_is_valid_gpio_irq_exit;

	if ((pinfo->irq_type != IRQ_TYPE_EDGE_RISING) &&
		(pinfo->irq_type != IRQ_TYPE_EDGE_FALLING) &&
		(pinfo->irq_type != IRQ_TYPE_EDGE_BOTH) &&
		(pinfo->irq_type != IRQ_TYPE_LEVEL_HIGH) &&
		(pinfo->irq_type != IRQ_TYPE_LEVEL_LOW))
		goto ambarella_is_valid_gpio_irq_exit;

	if ((pinfo->irq_gpio_val != GPIO_HIGH) &&
		(pinfo->irq_gpio_val != GPIO_LOW))
		goto ambarella_is_valid_gpio_irq_exit;

	if ((pinfo->irq_gpio_mode != GPIO_FUNC_SW_INPUT) &&
		(pinfo->irq_gpio_mode != GPIO_FUNC_SW_OUTPUT) &&
		(pinfo->irq_gpio_mode != GPIO_FUNC_HW))
		goto ambarella_is_valid_gpio_irq_exit;

	if ((pinfo->irq_gpio_mode != GPIO_FUNC_HW) &&
		((pinfo->irq_line < GPIO_INT_VEC(0)) ||
		(pinfo->irq_line >= NR_IRQS)))
		goto ambarella_is_valid_gpio_irq_exit;

	bvalid = 1;

ambarella_is_valid_gpio_irq_exit:
	return bvalid;
}
EXPORT_SYMBOL(ambarella_is_valid_gpio_irq);

module_init(rpmsg_vgpio_init);
module_exit(rpmsg_vgpio_fini);

MODULE_DESCRIPTION("RPMSG VGPIO");
