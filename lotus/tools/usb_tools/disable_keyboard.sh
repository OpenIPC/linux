# SPDX-License-Identifier: GPL-2.0-only

cd /sys/kernel/config/usb_gadget/hidkbd
echo > UDC

rm configs/c.1/hid.usb0/
rmdir configs/c.1/strings/0x409
rmdir functions/hid.usb0/
rmdir configs/c.1/
rmdir strings/0x409/

cd ../
rmdir mykbd/

cd /root/
umount /sys/kernel/config/