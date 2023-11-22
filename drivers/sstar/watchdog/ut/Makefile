#
# Copyright (c) [2019~2020] SigmaStar Technology.
#
#
# This software is licensed under the terms of the GNU General Public
# License version 2, as published by the Free Software Foundation, and
# may be copied, distributed, and modified under those terms.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License version 2 for more details.
#

CC  = $(CROSS_COMPILE)gcc
CPP = $(CROSS_COMPILE)g++
AR  = $(CROSS_COMPILE)ar
SAN = /tools/bin/clang-13/clang-format

ifeq (arm,$(ARCH))
COM_FLAGS = -Wall -O2 -fPIC -mcpu=cortex-a7 -mfpu=neon-fp16 -mfloat-abi=hard -mthumb-interwork -marm -g
endif
C_FLAGS   = $(COM_FLAGS) -std=c11 -D_DEFAULT_SOURCE
CPP_FLAGS = $(COM_FLAGS) -std=c++11

#INCLUDES  = -I.
#LIB_PATH  = -L.

TARGET_NAME  = wdt_ut

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
	@if [ -e $(SAN) ]; then \
		echo "sanitize $<"; \
		$(SAN) -i $<; \
	fi;
	@echo "compile $@"
	@$(CC) $(C_FLAGS) $(INCLUDES) -c $< -o $@

%.cpp.o : %.cpp
	@echo "compile $@"
	@$(CPP) $(CPP_FLAGS) $(INCLUDES) -c $< -o $@


