# SPDX-License-Identifier: GPL-2.0-only

mount -t configfs none /sys/kernel/config

cd /sys/kernel/config/usb_gadget/
mkdir -p hid
cd hid

echo 0x0483 > idVendor
echo 0x5710 > idProduct
echo 0x0215 > bcdDevice
echo 0x0200 > bcdUSB

mkdir -p strings/0x409
echo "serialnumber" > strings/0x409/serialnumber
echo "manufacturer" > strings/0x409/manufacturer
echo "product" > strings/0x409/product

mkdir -p configs/c.1/strings/0x409
echo "HID_CONFIG" > configs/c.1/strings/0x409/configuration
echo 2 > configs/c.1/MaxPower

mkdir -p functions/hid.usb0
echo 0 > functions/hid.usb0/protocol
echo 0 > functions/hid.usb0/subclass
echo 64 > functions/hid.usb0/report_length

echo -ne '\x05\x01\x09\x02\xA1\x01\x09\x01\xA1\x00\x05\x09\x19\x01\x29\x03\x15\x00\x25\x01\x95\x03\x75\x01\x81\x02\x95\x01\x75\x05\x81\x03\x05\x01\x09\x30\x09\x31\x15\x81\x25\x7F\x75\x08\x95\x02\x81\x06\xC0\xC0' > functions/hid.usb0/report_desc

ln -s functions/hid.usb0 configs/c.1/

ls /sys/class/udc > UDC
