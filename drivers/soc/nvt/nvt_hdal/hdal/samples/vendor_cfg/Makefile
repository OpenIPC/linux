MODULE_NAME = appfs
# DIRs
# source folder
SENSOR_CFG_DIR = $(NVT_HDAL_DIR)/ext_devices/sensor/configs
AD_CFG_DIR = $(NVT_HDAL_DIR)/ext_devices/ad/configs
MOTOR_CFG_DIR = $(NVT_HDAL_DIR)/ext_devices/motor/configs
ISP_CFG_DIR = $(NVT_HDAL_DIR)/vendor/isp/configs
# dtb includes
DTB_INCLUDE = \
	-I$(SENSOR_CFG_DIR)/dtsi \
	-I$(AD_CFG_DIR)/dtsi \
	-I$(MOTOR_CFG_DIR)/dtsi \
	-I$(ISP_CFG_DIR)/dtsi \
	-I$(KERNELDIR)/include \
	-I$(KERNELDIR)/arch/arm/boot/dts/include \
	-I$(CONFIG_DIR)/include \
	-I$(LINUX_BUILD_TOP)/base/linux-BSP/linux-kernel/include

# appfs folder
SENSOR_APPFS_DIR = $(MODULE_NAME)/sensor
MOTOR_APPFS_DIR = $(MODULE_NAME)/motor
ISP_APPFS_DIR = $(MODULE_NAME)/isp

.PHONY: all clean

###############################################################################
# Linux Makefile                                                              #
###############################################################################
ifeq ($(NVT_PRJCFG_CFG),Linux)
OUTPUT_NAND = $(MODULE_NAME).ubifs.nand.bin
OUTPUT_NOR = $(MODULE_NAME).jffs2.nor.bin
OUTPUT_NOR_SQUASHFS = $(MODULE_NAME).squash.nor.bin
OUTPUT_EMMC = $(MODULE_NAME).ext4.emmc.bin

#--------- ENVIRONMENT SETTING --------------------
# disable python tool generate __pycache__ directory
export PYTHONDONTWRITEBYTECODE=1
# MTD configs
include $(NVT_PRJCFG_MODEL_CFG)
NVT_FS_MTD_CFG_DEFAULT := ${ROOTFS_DIR}/mtd_cfg.txt
NVT_FS_MTD_CFG := ${CONFIG_DIR}/cfg_gen/mtd_cfg.txt
-include $(NVT_FS_MTD_CFG_DEFAULT)
-include $(NVT_FS_MTD_CFG)
# UBIFS app partition label name
PARTITION_LABEL_UBIFS = app
# system fdt name
DTBNAME = nvt-evb.dtb
#--------- END OF ENVIRONMENT SETTING -------------
SENSOR_DTS = sensor.dts
MOTOR_DTS = motor.dts
ISP_DTS = isp.dts
SENSOR_CFG_SRC = $(wildcard $(SENSOR_CFG_DIR)/cfg/*.cfg)
AD_CFG_SRC = $(wildcard $(AD_CFG_DIR)/cfg/*.cfg)
MOTOR_CFG_SRC = $(wildcard $(MOTOR_CFG_DIR)/cfg/*.cfg)
ISP_CFG_SRC = $(wildcard $(ISP_CFG_DIR)/cfg/*.cfg) $(wildcard $(ISP_CFG_DIR)/bin/*.bin)

DTS_SRC = $(SENSOR_DTS) $(MOTOR_DTS) $(ISP_DTS)
DTB = $(DTS_SRC:.dts=.dtb)

ifneq ($(ROOTFS_APPFS_EN),OFF)
all: $(DTB) $(OUTPUT_NAND) $(OUTPUT_NOR) $(OUTPUT_NOR_SQUASHFS) $(OUTPUT_EMMC)
else
all:
	@echo "nothing to be done for vendor_cfg"
endif

$(DTB): $(wildcard *.dtsi) $(wildcard *.dts)

%.dtb: %.dts
	@echo Compiling $<
	@cpp -nostdinc $(DTB_INCLUDE) -undef -x assembler-with-cpp $< > $<.tmp \
	&& dtc -O dtb -b 0 -o $@ $<.tmp \
	&& rm $<.tmp

$(SENSOR_APPFS_DIR): $(SENSOR_CFG_SRC) $(SENSOR_DTS:.dts=.dtb)
	@echo creating $@
	@mkdir -p $@ && \
	cp -arf -t $@ $^

$(MOTOR_APPFS_DIR): $(MOTOR_CFG_SRC) $(MOTOR_DTS:.dts=.dtb)
	@echo creating $@
	@mkdir -p $@ && \
	cp -arf -t $@ $^

$(ISP_APPFS_DIR): $(ISP_CFG_SRC) $(ISP_DTS:.dts=.dtb)
	@echo creating $@
	@mkdir -p $@ && \
	cp -arf -t $@ $^

$(OUTPUT_NAND) : $(SENSOR_APPFS_DIR) $(MOTOR_APPFS_DIR) $(ISP_APPFS_DIR)
	@echo "creating $@ ";
ifeq ($(EMBMEM), EMBMEM_SPI_NAND)
	@$(ROOTFS_DIR)/verify_leb.py -f $(dir $(NVT_PRJCFG_MODEL_CFG))$(DTBNAME) -l $(PARTITION_LABEL_UBIFS) -b $(ROOTFS_UBI_APP_MAX_LEB_COUNT)
endif
	@if [ -e bin ]; then \
		rm -rf bin; \
	fi
	@mkdir bin;
	@mkfs.ubifs --squash-uids -x $(ROOTFS_UBI_COMPRESS_MODE) -m $(ROOTFS_UBI_PAGE_SIZE) -e $(ROOTFS_UBI_ERASE_BLK_SIZE) -c $(ROOTFS_UBI_APP_MAX_LEB_COUNT) -o bin/$(OUTPUT_NAND).raw -r $(MODULE_NAME);
	@echo -e "[ubifs]\nmode=ubi\nimage=bin/$(OUTPUT_NAND).raw\nvol_id=0" > bin/ubinize.cfg;
	@echo -e "vol_size=$(shell echo $(ROOTFS_UBI_APP_MAX_LEB_COUNT)\*$(shell echo $(ROOTFS_UBI_ERASE_BLK_SIZE))|bc)" >> bin/ubinize.cfg;
	@echo -e "vol_type=dynamic\nvol_name=app" >> bin/ubinize.cfg;
	@ubinize -m $(ROOTFS_UBI_PAGE_SIZE) -p $(ROOTFS_UBI_BLK_SIZE) -s $(ROOTFS_UBI_PAGE_SIZE) -o bin/$(OUTPUT_NAND).tmp.raw bin/ubinize.cfg;
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$(OUTPUT_NAND).tmp.raw --packsum-dst=bin/$(OUTPUT_NAND) --packsum-type=0x9;
	@cp bin/$(OUTPUT_NAND) $(OUTPUT_NAND)
$(OUTPUT_NOR) : $(SENSOR_APPFS_DIR) $(MOTOR_APPFS_DIR) $(ISP_APPFS_DIR)
	@echo "creating $@ ";
	@mkfs.jffs2 -n -s $(ROOTFS_JFFS2_PAGE_SIZE) -e $(ROOTFS_JFFS2_BLK_SIZE) -r $(MODULE_NAME) -U -X $(ROOTFS_JFFS2_COMPRESS_MODE) -p $(ROOTFS_JFFS2_APP_NOR_SIZE) -o bin/$(OUTPUT_NOR).raw;
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$@.raw --packsum-dst=bin/$@ --packsum-type=0x9;
	@cp bin/$@ $@
$(OUTPUT_NOR_SQUASHFS) : $(SENSOR_APPFS_DIR) $(MOTOR_APPFS_DIR) $(ISP_APPFS_DIR)
	@echo "creating $@ ";
	@mksquashfs $(MODULE_NAME) bin/$@.raw -comp $(ROOTFS_SQ_COMPRESS_MODE) -b $(ROOTFS_SQ_BLK_SIZE) -root-owned;
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=bin/$@.raw --packsum-dst=bin/$@ --packsum-type=0x9;
	@cp bin/$@ $@
$(OUTPUT_EMMC) : $(SENSOR_APPFS_DIR) $(MOTOR_APPFS_DIR) $(ISP_APPFS_DIR)
	@echo "creating $@ ";
	@make_ext4fs -s -l $(ROOTFS_EXT4_APP_SIZE) $@.raw $(MODULE_NAME);
	@$(BUILD_DIR)/nvt-tools/nvt-ld-op --packsum-src=$@.raw --packsum-dst=$@ --packsum-type=0x9; \
	rm $@.raw;

clean:
	@rm -f $(DTB) $(OUTPUT_NAND) $(OUTPUT_NOR) $(OUTPUT_NOR_SQUASHFS) $(OUTPUT_EMMC)
	@rm -rf $(MODULE_NAME)
	@rm -rf bin

ifneq ($(ROOTFS_APPFS_EN),OFF)
install:
	@cp -arf -t $(LINUX_BUILD_TOP)/output $(OUTPUT_NAND) $(OUTPUT_NOR) $(OUTPUT_NOR_SQUASHFS) $(OUTPUT_EMMC)
	@RAW_IMAGE_LIST=`ls bin/*.bin.raw 2>/dev/null`; \
	for file in $$RAW_IMAGE_LIST; do \
		mkdir -p $(LINUX_BUILD_TOP)/output/raw; \
		if [ -f $$file ]; then cp $$file $(LINUX_BUILD_TOP)/output/raw; fi; \
	done;
else
install:
	@echo "nothing to be done for vendor_cfg"
endif
###############################################################################
# rtos Makefile                                                               #
###############################################################################
else ifeq ($(NVT_PRJCFG_CFG),rtos)

DTS_SRC = isp.dts
DTB = $(DTS_SRC:.dts=.dtb)

all: $(DTB)

$(DTB): $(wildcard *.dtsi) $(wildcard *.dts)

%.dtb: %.dts
	@echo Compiling $<
	@cpp -nostdinc $(DTB_INCLUDE) -undef -x assembler-with-cpp $< > $<.tmp \
	&& dtc -O dtb -b 0 -o $@ $<.tmp \
	&& rm $<.tmp

clean:
	@rm -f $(DTB)

install:

endif
