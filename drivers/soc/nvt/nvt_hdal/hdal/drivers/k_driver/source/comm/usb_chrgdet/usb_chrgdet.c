#include <kwrap/util.h>
#include <linux/gpio.h>
#include <plat/nvt-gpio.h>

#include "comm/usb_chrgdet.h"
#include "usb_chrgdet_reg.h"
#include "usb_chrgdet_drv.h"
#include "usb_chrgdet_main.h"
#include "usb_chrgdet_dbg.h"

extern UINT32 _USB_CHRGDET_REG_BASE_ADDR[MODULE_REG_NUM];
extern PUSB_CHRGDET_DRV_INFO pusb_chrgdet_drv_info_data;

#define USB_SETREG(ofs, value)	writel((value), (volatile void __iomem *)(0xFD600000+(ofs)))
#define USB_GETREG(ofs)	readl((volatile void __iomem *)(0xFD600000+(ofs)))

#define USB2PHY_SETREG(ofs, value)	writel((value), (volatile void __iomem *)(0xFD601000+((ofs)<<2)))
#define USB2PHY_GETREG(ofs)	readl((volatile void __iomem *)(0xFD601000+((ofs)<<2)))


static BOOL usb_chrgdet_standard[1];

#if 1

static BOOL usb_chrgdet_check_linestate(USB_CHRGDET_PORT usb_port)
{
	UINT32 temp;

	if (usb_port == USB_CHRGDET_PORT_USB30) {
		printk("NO U3 port\n");
		return TRUE;
	} else {
		USB2PHY_SETREG(0x0B, 0x20);
		USB2PHY_SETREG(0x0C, 0x20);
		vos_util_delay_us_polling(20);
		temp = USB2PHY_GETREG(0x33) >> 6;
		USB2PHY_SETREG(0x0B, 0x00);
		USB2PHY_SETREG(0x0C, 0x00);
	}

	if (temp == 0x01) {
		/* PC */
		return TRUE;
	}

	/* NOT-PC / Charger */
	return FALSE;
}

static UINT32 usb_chrgdet_check_vbusi(USB_CHRGDET_PORT usb_port)
{
	int gpio_pin;

	if (usb_port == USB_CHRGDET_PORT_USB30) {
		printk("NO U3 port\n");
		return 0;
	} else {
		gpio_pin = 7;
	}

	gpio_direction_input(D_GPIO(gpio_pin));
	return (UINT32) gpio_get_value(D_GPIO(gpio_pin));
}



static BOOL usb_chrgdet_check_data_contact(USB_CHRGDET_PORT usb_port)
{
	BOOL contact = FALSE;
	UINT32 phyreg_0b, phyreg_0c;
	T_USB3APB_CHARGER_CONTROL_REG   charger_ctrlreg;

	if (usb_port == USB_CHRGDET_PORT_USB30) {
		printk("NO U3 port\n");
		return FALSE;
	} else if (usb_port == USB_CHRGDET_PORT_USB20) {


		// Set DP/DM Pull-Down resist as SW control
		phyreg_0b = USB2PHY_GETREG(0x0B);
		USB2PHY_SETREG(0x0B, phyreg_0b | 0x3);

		// Enable DM Pull-Down resist
		phyreg_0c = USB2PHY_GETREG(0x0C);
		USB2PHY_SETREG(0x0C, phyreg_0c | 0x1);

		charger_ctrlreg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
		charger_ctrlreg.bit.DCD_DETECT_EN = 1;
		USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, charger_ctrlreg.reg);

		vos_util_delay_ms(11);

		charger_ctrlreg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
		if (charger_ctrlreg.bit.DCD_DETECT_EN == 1) {
			contact = FALSE;
		} else {
			contact = TRUE;
		}

		charger_ctrlreg.bit.DCD_DETECT_EN = 0;
		USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, charger_ctrlreg.reg);

		// Disable DM Pull-Down resist
		USB2PHY_SETREG(0x0C, phyreg_0c);

		// Restore DP/DM Pull-Down resist as HW control
		USB2PHY_SETREG(0x0B, phyreg_0b);

	}


	return contact;
}

static USB_CHRGDET_STS usb_chrgdet_check_charger(USB_CHRGDET_PORT usb_port)
{
	USB_CHRGDET_STS ret = USB_CHRGDET_STS_DISCONNECT;
	unsigned char ucloop;

	// TODO: Add UTMI Suspend/Exit


	if (usb_port == USB_CHRGDET_PORT_USB30) {
		printk("NO U3 port\n");
		return USB_CHRGDET_STS_DISCONNECT;
	} else if (usb_port == USB_CHRGDET_PORT_USB20) {

		ucloop = 0;

		if ((!usb_chrgdet_standard[ucloop]) && (usb_chrgdet_check_data_contact(usb_port) == FALSE)
			&& usb_chrgdet_check_vbusi(usb_port)) {

			/* If support non-standard charger, VBUS detect, but no D+/D- not correctly contacted
			   Then it might be D+/D- open type charger */
			ret = USB_CHRGDET_STS_CHARGER;

		} else {
			T_USB3APB_CHARGER_CONTROL_REG   charger_ctrlreg;
			T_USB3APB_BUS_STATUS_REG        bus_stsreg;

			/* If D+/D- is contacted, follow standard charger detection flow */

			charger_ctrlreg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
			charger_ctrlreg.bit.CHG_DETECT_EN = 1;
			USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, charger_ctrlreg.reg);

			vos_util_delay_ms(40);

			bus_stsreg.reg = USB_GETREG(USB_BUS_STATUS_REG_OFS);
			charger_ctrlreg.bit.CHG_DETECT_EN = 0;
			USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, charger_ctrlreg.reg);

			if (bus_stsreg.bit.CHG_DET == 0) {

				/* standard downstream port. i.e. PC */
				if (usb_chrgdet_standard[ucloop]) {
					ret = USB_CHRGDET_STS_NONE;
				} else {

					if (usb_chrgdet_check_linestate(usb_port) == TRUE) {

					if ((usb_chrgdet_check_vbusi(usb_port) == 0) ||
						(usb_chrgdet_check_data_contact(usb_port) == FALSE)) {
						/* Double confirm USB plug status */
						nvt_dbg(IND, "line sts check: disconnect\r\n");
						ret = USB_CHRGDET_STS_DISCONNECT;
					} else {
						nvt_dbg(IND, "line sts check: PC\r\n");
						ret = USB_CHRGDET_STS_NONE;
					}

					} else {
					    nvt_dbg(IND, "line sts check: charger\r\n");
					    ret = USB_CHRGDET_STS_CHARGER;
					}
				}

			} else {
				/* charger or charging downstream port */

				charger_ctrlreg.reg = USB_GETREG(USB_CHARGER_CONTROL_REG_OFS);
				charger_ctrlreg.bit.SECOND_CHG_DETECT_EN = 1;
				USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, charger_ctrlreg.reg);

				vos_util_delay_ms(40);

				bus_stsreg.reg = USB_GETREG(USB_BUS_STATUS_REG_OFS);
				charger_ctrlreg.bit.SECOND_CHG_DETECT_EN = 0;
				USB_SETREG(USB_CHARGER_CONTROL_REG_OFS, charger_ctrlreg.reg);


				if (bus_stsreg.bit.CHG_DET == 0) {
					/* charging downstream port */

					if (usb_chrgdet_standard[ucloop]) {
						ret = USB_CHRGDET_STS_CHARGING_DOWNSTREAM_PORT;
					} else {

					if (usb_chrgdet_check_linestate(usb_port) == TRUE) {

					if ((usb_chrgdet_check_vbusi(usb_port) == 0) ||
						(usb_chrgdet_check_data_contact(usb_port) == FALSE)) {
						// Double confirm USB plug status
						nvt_dbg(IND, "line sts check2: disconnect\r\n");
						ret = USB_CHRGDET_STS_DISCONNECT;
					} else {
						nvt_dbg(IND, "line sts check2: charge PC\r\n");
						ret = USB_CHRGDET_STS_CHARGING_DOWNSTREAM_PORT;
					}
					} else {
					    nvt_dbg(IND, "line sts check2: charger\r\n");
					    ret = USB_CHRGDET_STS_CHARGER;
					}

					}

				} else {
					/* dedicated charging port */
					ret = USB_CHRGDET_STS_CHARGER;
				}
			}
		}


	}

	return ret;
}


#endif


INT32 __usb_chrgdet_set(USB_CHRGDET_PORT usb_port, USB_CHRGDET_PARAM_ID param_id, VOID *p_param)
{
	UINT32 *ret;
	unsigned char ucloop;

	ret = (UINT32 *)p_param;

	if (usb_port == USB_CHRGDET_PORT_USB30) {
		printk("NO U3 port\n");
		return -1;
	}

	ucloop = 0;

	switch (param_id) {
	case USB_CHRGDET_TYPE_STD: {
		usb_chrgdet_standard[ucloop] = (BOOL)p_param;
	} break;

	default:
		return -1;
	}

	return 0;
}


INT32 __usb_chrgdet_get(USB_CHRGDET_PORT usb_port, USB_CHRGDET_PARAM_ID param_id, VOID *p_param)
{
	UINT32 *ret;
	USB_CHRGDET_STS *ret_sts;
	unsigned char ucloop;

	ret = (UINT32 *)p_param;
	ret_sts = (USB_CHRGDET_STS *)p_param;

	if (usb_port == USB_CHRGDET_PORT_USB30) {
		printk("NO U3 port\n");
		return -1;
	}

	ucloop = 0;

	switch (param_id) {
	case USB_CHRGDET_PLUG: {

		*ret = usb_chrgdet_check_vbusi(usb_port);

		if (*ret && usb_chrgdet_standard[ucloop]) {
			if (!request_mem_region(pusb_chrgdet_drv_info_data->presource[ucloop]->start, resource_size(pusb_chrgdet_drv_info_data->presource[ucloop]), "nvt,nvt_usb_chrgdet")) {
				nvt_dbg(IND, "ehci/fotg200 driver already loaded\n");
				*ret = 1;
			} else {
				release_mem_region(pusb_chrgdet_drv_info_data->presource[ucloop]->start, resource_size(pusb_chrgdet_drv_info_data->presource[ucloop]));
				*ret = usb_chrgdet_check_data_contact(usb_port);
			}
		}

	} break;

	case USB_CHRGDET_STATUS:

		if (!request_mem_region(pusb_chrgdet_drv_info_data->presource[ucloop]->start, resource_size(pusb_chrgdet_drv_info_data->presource[ucloop]), "nvt,nvt_usb_chrgdet")) {
			nvt_dbg(IND, "ehci/fotg200 driver already loaded\n");
			*ret_sts = USB_CHRGDET_STS_DISCONNECT;
		} else {
			release_mem_region(pusb_chrgdet_drv_info_data->presource[ucloop]->start, resource_size(pusb_chrgdet_drv_info_data->presource[ucloop]));
			*ret_sts = usb_chrgdet_check_charger(usb_port);
		}

		break;
	default:
		return -1;
	}

	return 0;
}


#ifdef __KERNEL__
EXPORT_SYMBOL(__usb_chrgdet_set);
EXPORT_SYMBOL(__usb_chrgdet_get);
#endif


