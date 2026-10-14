# SPDX-License-Identifier: GPL-2.0-only

cd /sys/kernel/config/usb_gadget/acm
echo > UDC
rm configs/c.1/acm.0
rmdir configs/c.1/strings/0x409
rmdir functions/acm.0
rmdir configs/c.1
rmdir strings/0x409
cd ../
rmdir acm
cd /root/
umount /sys/kernel/config/
