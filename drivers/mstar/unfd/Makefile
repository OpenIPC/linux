
obj-$(CONFIG_MS_NAND) := kdrv_unfd.o
kdrv_unfd-y = drvNAND.o \
	./src/api/drvNAND_prg.o \
	./src/common/drvNAND_hal.o \
	./src/common/drvNAND_hal_v5.o \
	./src/common/drvNAND_ReadRetry_v5.o \
	./src/common/drvNAND_utl.o \
	./src/config/drvNAND_platform.o

#include unfd header files
EXTRA_CFLAGS += -Idrivers/mstar/include
EXTRA_CFLAGS += -I$(src)/inc/api/
EXTRA_CFLAGS += -I$(src)/inc/common
EXTRA_CFLAGS += -I$(src)/inc/config/


