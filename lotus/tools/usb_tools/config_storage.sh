# SPDX-License-Identifier: GPL-2.0-only

mount -t configfs none /sys/kernel/config/

cd /sys/kernel/config/usb_gadget/
mkdir storage
cd storage

mkdir functions/mass_storage.0
echo $MEMORY > functions/mass_storage.0/lun.0/file

echo $VID > idVendor
echo $PID > idProduct
mkdir strings/0x409
echo $MANUFACTURER > strings/0x409/manufacturer
echo $PRODUCT > strings/0x409/product
echo $SERIALNUMBER > strings/0x409/serialnumber

mkdir configs/c.1/
echo "0xC0" > configs/c.1/bmAttributes
echo "1" > configs/c.1/MaxPower
mkdir configs/c.1/strings/0x409/
echo "Mass Storage" > configs/c.1/strings/0x409/configuration
ln -s functions/mass_storage.0/ configs/c.1/

ls /sys/class/udc/ | xargs -i echo {} > UDC
