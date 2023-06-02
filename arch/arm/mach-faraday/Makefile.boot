ifeq ($(CONFIG_FARADAY_HIGH_PHYS_OFFSET),y)
# memory not remapped
   zreladdr-y	+= 0x10008000
params_phys-y	:= 0x10000100
initrd_phys-y	:= 0x10800000
else
   zreladdr-y	+= 0x00008000
params_phys-y	:= 0x00000100
initrd_phys-y	:= 0x00800000
endif
