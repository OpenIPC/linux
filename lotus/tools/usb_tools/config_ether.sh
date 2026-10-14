# SPDX-License-Identifier: GPL-2.0-only

mount -t configfs none /sys/kernel/config/
cd /sys/kernel/config/usb_gadget/
mkdir ether
cd ether/
echo "0x0200" > bcdUSB
echo "0xef" > bDeviceClass
echo "2" > bDeviceSubClass
echo "0x1d6b" > idVendor
echo "0xA4A2" > idProduct
echo "0x3000" > bcdDevice
echo "0x01" > bDeviceProtocol
mkdir strings/0x409
echo $MANUFACTURER > strings/0x409/manufacturer
echo $PRODUCT > strings/0x409/product
echo $SERIALNUMBER > strings/0x409/serialnumber

mkdir configs/c.1/
echo "0xC0" > configs/c.1/bmAttributes
echo "1" > configs/c.1/MaxPower
mkdir configs/c.1/strings/0x409/
echo "CDC" > configs/c.1/strings/0x409/configuration

mkdir configs/c.2
echo "0xC0" > configs/c.2/bmAttributes
echo "1" > configs/c.2/MaxPower
mkdir configs/c.2/strings/0x409/
echo "RNDIS" > configs/c.2/strings/0x409/configuration
echo "1" > os_desc/use
echo "0xcd" > os_desc/b_vendor_code
echo "MSFT100" > os_desc/qw_sign

mkdir functions/rndis.usb0
echo "RNDIS" > functions/rndis.usb0/os_desc/interface.rndis/compatible_id
echo "5162001" > functions/rndis.usb0/os_desc/interface.rndis/sub_compatible_id

mkdir functions/ecm.usb0

ln -s functions/ecm.usb0  configs/c.1
ln -s functions/rndis.usb0 configs/c.2
ln -s configs/c.2 os_desc
ls /sys/class/udc/ | xargs -i echo {} > UDC
