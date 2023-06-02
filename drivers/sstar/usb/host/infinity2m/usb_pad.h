#ifndef __USB_PAD_H
#define __USB_PAD_H


int Enable_USB_VBUS(int param);
int Get_USB_VBUS_Pin(struct device_node *np);

#endif //__USB_PAD_H
