#cd /tmp
#busybox tftp -gr mmu_ut.ko 192.168.4.99

#. disalbe MIU protect
#riu_w 1012 69 0700
#rmmod mmu_ut; insmod mmu_ut.ko ut=1
#rmmod mmu_ut; insmod mmu_ut.ko ut=2
#rmmod mmu_ut; insmod mmu_ut.ko ut=3
rmmod sata_ut; insmod sata_ut.ko port=1 phy_mode=0
rmmod sata_ut; insmod sata_ut.ko port=1 phy_mode=1
rmmod sata_ut; insmod sata_ut.ko port=1 phy_mode=2


