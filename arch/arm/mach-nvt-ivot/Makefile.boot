#
# To define loading/entry address for NVT platform
#
ifdef CONFIG_PHYS_OFFSET
 zreladdr-y	:= $(shell perl -e 'printf "0x%08x", $(CONFIG_PHYS_OFFSET) + $(TEXT_OFFSET)')
else
 zreladdr-y	:= 0x00208000
endif
