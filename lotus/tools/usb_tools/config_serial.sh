# SPDX-License-Identifier: GPL-2.0-only

export MANUFACTURER="The Linux Foundation"
export PRODUCT="Gadget Serial v2.4"
export SERIALNUMBER="1234567895"

mount -t configfs none /sys/kernel/config/
cd /sys/kernel/config/usb_gadget/
mkdir acm
cd acm

mkdir functions/acm.0
echo "0x1d6b" > idVendor
echo "0xa4a7" > idProduct
echo "2" > bDeviceClass
mkdir strings/0x409
echo $MANUFACTURER > strings/0x409/manufacturer
echo $PRODUCT > strings/0x409/product
echo $SERIALNUMBER > strings/0x409/serialnumber

mkdir configs/c.1/
echo "1" > configs/c.1/MaxPower
echo "0xC0" > configs/c.1/bmAttributes
mkdir configs/c.1/strings/0x409/
echo "CDC ACM config" > configs/c.1/strings/0x409/configuration
ln -s functions/acm.0/ configs/c.1/
ls /sys/class/udc/ | xargs -i echo {} > UDC
