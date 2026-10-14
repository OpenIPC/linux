# SPDX-License-Identifier: GPL-2.0-only

cd /sys/kernel/config/usb_gadget/storage
echo > UDC
rm configs/c.1/mass_storage.0
rmdir configs/c.1/strings/0x409
rmdir functions/mass_storage.0
rmdir configs/c.1
rmdir strings/0x409
cd ../
rmdir storage
cd /root/
umount /sys/kernel/config/
