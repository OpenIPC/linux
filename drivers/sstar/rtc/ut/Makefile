CROSS_COMPILE ?=arm-linux-gnueabihf-
CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar

ifeq (arm,$(ARCH))
COM_FLAGS = -Wall -O2 -fPIC -mcpu=cortex-a7 -mfpu=neon-fp16 -mfloat-abi=hard -mthumb-interwork -marm -g
endif
C_FLAGS   = $(COM_FLAGS) -std=c11
CPP_FLAGS = $(COM_FLAGS) -std=c++11


#INCLUDES  = -I. -I../../item/ssd222_dev/project/release/include
#LIB_PATH  = -L../../item/ssd222_dev/project/release/dispcam/p3/common/glibc/9.1.0/mi_libs/dynamic

TARGET_NAME  = rtc_ut

CPP_SRCS  =  $(wildcard *.cpp */*.cpp)
C_SRCS    =  $(wildcard *.c */*.c)

CPP_OBJS  = $(patsubst %.cpp, %.cpp.o, $(CPP_SRCS))
C_OBJS    = $(patsubst %.c, %.c.o, $(C_SRCS))

#LIB_NAME  = -lmi_sys -lmi_common

.PHONY: all prepare clean

all: prepare $(TARGET_NAME) finish

prepare:
	@echo 
	@echo "TARGET_NAME = $(TARGET_NAME)"
	@echo


clean:
	@rm -rf $(CPP_OBJS)
	@rm -ff $(C_OBJS)
	@rm -rf $(TARGET_NAME)

finish:
	@echo "make done"
	@echo

$(TARGET_NAME): $(CPP_OBJS) $(CPP_SRCS) $(C_OBJS) $(C_SRCS)
	@echo "generate $@"
	@$(CPP) -o $@ $(C_OBJS) $(CPP_OBJS) $(LIB_PATH) $(LIB_NAME) -ldl -lm -lpthread

%.c.o : %.c
	@echo "compile $@"
	@$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) $(CPP_FLAGS) $(INCLUDES) -c $< -o $@

