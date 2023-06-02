include Makefile

CSCOPE_FIND_FLAGS = -regex '\(Makefile\|makefile\|Kbuild\|Kconfig\|.*\.\(c\|h\|s\|S\|mk\|mak\)\)'

CSCOPE_ROOT = $(shell pwd)
CSCOPE_ARCH_DIR = arch
CSCOPE_COMMON_DIR = common

CSCOPE_ARCH_MACHINE_PREFIX = mach-
CSCOPE_ARCH_PLATFORM_PREFIX = plat-

include $(CSCOPE_ROOT)/$(CSCOPE_ARCH_DIR)/$(ARCH)/Makefile

CSCOPE_TOP_SRC_PATH = init ipc virt samples drivers include block crypto mm security fs sound net scripts lib kernel firmware
CSCOPE_SRC_PATH = $(foreach d, $(CSCOPE_TOP_SRC_PATH), $(CSCOPE_ROOT)/$(d))
CSCOPE_MACHDIRS = $(CSCOPE_ROOT)/$(shell echo $(machdirs) | cut -d' ' -f1 )
CSCOPE_ARCHDIRS = $(CSCOPE_ROOT)/$(CSCOPE_ARCH_DIR)/$(ARCH)

CSCOPE_NAMEFILE = .cscope.files

gen_db:
	@echo
	@echo Clean up ...
	@rm -f $(CSCOPE_NAMEFILE)
	@echo
	@echo List files ... option:$(CSCOPE_FIND_FLAGS)
	@echo +++ $(CSCOPE_ARCHDIRS)
	@echo --- $(CSCOPE_ARCHDIRS)/$(CSCOPE_ARCH_MACHINE_PREFIX)
	@echo --- $(CSCOPE_ARCHDIRS)/$(CSCOPE_ARCH_PLATFORM_PREFIX)
	@find $(CSCOPE_ARCHDIRS) $(CSCOPE_FIND_FLAGS) -type f | grep -v $(CSCOPE_ARCHDIRS)/$(CSCOPE_ARCH_MACHINE_PREFIX) | grep -v $(CSCOPE_ARCHDIRS)/$(CSCOPE_ARCH_PLATFORM_PREFIX) > $(CSCOPE_NAMEFILE)
	@echo +++ $(CSCOPE_MACHDIRS)
	@find $(CSCOPE_MACHDIRS) $(CSCOPE_FIND_FLAGS) -type f >> $(CSCOPE_NAMEFILE)
	@echo +++ $(CSCOPE_SRC_PATH)
	@find $(CSCOPE_SRC_PATH) $(CSCOPE_FIND_FLAGS) -type f >> $(CSCOPE_NAMEFILE)
	@echo
	@echo Generate cscope db ...
	@cscope -i $(CSCOPE_NAMEFILE) -bqkR
	@echo
