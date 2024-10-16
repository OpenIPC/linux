
SRC_DIR  := src

EXTRA_CFLAGS += -DBUILD_PLATFORM_DEFAULT=1 \
		-DBUILD_ZHOUYI_V1=1 \
		-I$(src)/$(SRC_DIR) \
		-I$(src)/$(SRC_DIR)/aipu \
		-I$(src)/$(SRC_DIR)/aipu/zhouyi/z1 \
		-I$(src)/$(SRC_DIR)/aipu/zhouyi \
		-I$(src)/$(SRC_DIR)/aipu/soc \
		-I$(src)/include/uapi/misc \
		-DBUILD_DEBUG_VERSION=0 \

COMM_OBJ := $(SRC_DIR)/aipu/zhouyi/zhouyi.o \
		$(SRC_DIR)/aipu/aipu_core.o \
		$(SRC_DIR)/aipu_priv.o \
		$(SRC_DIR)/aipu_io.o \
		$(SRC_DIR)/aipu_irq.o \
		$(SRC_DIR)/aipu_mm.o \
		$(SRC_DIR)/aipu_job_manager.o

PMU_OBJ  := $(SRC_DIR)/aipu_pmu.o

AIPU_OBJ := $(SRC_DIR)/aipu/zhouyi/z1/z1.o

INIT_OBJ := $(SRC_DIR)/aipu.o
OBJS     := $(COMM_OBJ) $(PMU_OBJ) $(AIPU_OBJ)

obj-$(CONFIG_AIPU) += aipu.o
aipu-objs += $(INIT_OBJ)
aipu-objs += $(OBJS)
