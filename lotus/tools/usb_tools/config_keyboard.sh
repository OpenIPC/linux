# SPDX-License-Identifier: GPL-2.0-only

mount -t configfs none /sys/kernel/config

cd /sys/kernel/config/usb_gadget/
mkdir -p hidkbd
cd hidkbd

echo 0x1d6b > idVendor
echo 0x0104 > idProduct
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB

mkdir -p strings/0x409
echo "serialnumber" > strings/0x409/serialnumber
echo "manufacturer" > strings/0x409/manufacturer
echo "product" > strings/0x409/product

mkdir -p configs/c.1/strings/0x409
echo "Configuration" > configs/c.1/strings/0x409/configuration
echo 120 > configs/c.1/MaxPower

mkdir -p functions/hid.usb0
echo 1 > functions/hid.usb0/protocol
echo 0 > functions/hid.usb0/subclass
echo 8 > functions/hid.usb0/report_length

echo -ne '\x05\x01\x09\x06\xA1\x01\x05\x07\x19\xE0\x29\xE7\x15\x00\x25\x01\x75\x01\x95\x08\x81\x02\x95\x01\x75\x08\x81\x03\x95\x05\x75\x01\x05\x08\x19\x01\x29\x05\x91\x02\x95\x01\x75\x03\x91\x03\x95\x06\x75\x08\x15\x00\x25\x65\x05\x07\x19\x00\x29\x65\x81\x00\xC0' > functions/hid.usb0/report_desc

ln -s functions/hid.usb0 configs/c.1/

ls /sys/class/udc > UDC
