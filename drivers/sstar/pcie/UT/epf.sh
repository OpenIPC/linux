# insert epf-test driver
#insmod /data/ut/edma.ko
insmod /data/ut/pci-epf-test.ko

#mount -t configfs none /sys/kernel/config
#cd /sys/kernel/config/pci_ep/
#mkdir functions/pci_epf_test/func1
#cd functions/pci_epf_test/func1
#echo 0x104c > vendorid
#echo 0xb500 > deviceid
#echo 16 > msi_interrupts
#echo 8 > msix_interrupts
#cd ../../../
# bind epc & epf
#ln -s functions/pci_epf_test/func1 controllers/1f351000.pcie/
# start link
#echo 1 > controllers/1f351000.pcie/start

# remove EP device
echo EP remove process
echo 1 > /sys/bus/pci/devices/0000\:01\:00.0/remove
# RC rescan Process
echo rescan RC_process
echo 1 > /sys/bus/pci/rescan

# insert rc side test driver
insmod /data/ut/pci_endpoint_test.ko
chmod 775 /data/ut/pcitest

./pcitest -b 0
./pcitest -b 1
./pcitest -i 2
./pcitest -x 1
./pcitest -x 2
./pcitest -x 4
./pcitest -x 8
./pcitest -i 1
./pcitest -m 1
./pcitest -m 2
./pcitest -m 4
./pcitest -m 8
./pcitest -r -s 102400
./pcitest -w -s 102400
./pcitest -c -s 102400
./pcitest -r -s 102400 -d
./pcitest -w -s 102400 -d

insmod /data/ut/pcie_loopback.ko












