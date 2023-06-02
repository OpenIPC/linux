
zreladdr-y		:= $(shell printf "0x%08x" $$(($(CONFIG_PHYS_OFFSET) + $(TEXT_OFFSET))))
params_phys-y	:= $(shell printf "0x%08x" $$(($(CONFIG_PHYS_OFFSET) + 0x100)))

dtb-y	+= gk710xs-evb.dtb

