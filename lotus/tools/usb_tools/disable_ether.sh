# SPDX-License-Identifier: GPL-2.0-only

cd /sys/kernel/config/usb_gadget/ether
echo > UDC
rm configs/c.1/ecm.usb0
rm configs/c.2/rndis.usb0
rmdir configs/c.1/strings/0x409
rmdir configs/c.2/strings/0x409
rmdir functions/ecm.usb0
rmdir functions/rndis.usb0
rm os_desc/c.2
rmdir configs/c.1
rmdir configs/c.2
rmdir strings/0x409
cd ../
rmdir ether
cd /root/
umount /sys/kernel/config/
