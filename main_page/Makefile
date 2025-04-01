#
# Makefile
#
# ⭐全速编译命令： make clean && make -j$(nproc) RUN_JOBS=-j$(nproc)

#CC ?= gcc
# export CC := gcc
# export CXX := g++
export CC := aarch64-buildroot-linux-gnu-gcc
export CXX := aarch64-buildroot-linux-gnu-g++
export LVGL_DIR_NAME ?= lvgl
export LVGL_DIR ?= ${shell pwd}
export CFLAGS ?= -O3 -g0 -I$(LVGL_DIR)/  -Wall -Wshadow -Wundef -Wmissing-prototypes -Wno-discarded-qualifiers -Wall -Wextra -Wno-unused-function -Wno-error=strict-prototypes -Wpointer-arith -fno-strict-aliasing -Wno-error=cpp -Wuninitialized -Wmaybe-uninitialized -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wno-cast-qual -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wformat-security -Wno-ignored-qualifiers -Wno-error=pedantic -Wno-sign-compare -Wno-error=missing-prototypes -Wdouble-promotion -Wclobbered -Wdeprecated -Wempty-body -Wtype-limits -Wshift-negative-value -Wstack-usage=2048 -Wno-unused-value -Wno-unused-parameter -Wno-missing-field-initializers -Wuninitialized -Wmaybe-uninitialized -Wall -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wtype-limits -Wsizeof-pointer-memaccess -Wno-format-nonliteral -Wpointer-arith -Wno-cast-qual -Wmissing-prototypes -Wunreachable-code -Wno-switch-default -Wreturn-type -Wmultichar -Wno-discarded-qualifiers -Wformat-security -Wno-ignored-qualifiers -Wno-sign-compare \
	 -I /home/yixi/Work/imx6ull/chain/arm-buildroot-linux-musleabihf_sdk-buildroot/arm-buildroot-linux-musleabihf/sysroot/usr/include/dbus-1.0 -I /home/yixi/Work/imx6ull/chain/arm-buildroot-linux-musleabihf_sdk-buildroot/arm-buildroot-linux-musleabihf/sysroot/usr/lib/dbus-1.0/include -I/home/yixi/Work/TSPI/buildroot/buildroot-2025.02/output/host/aarch64-buildroot-linux-gnu/sysroot/usr/include/freetype2
export CPPFLAGS = -O3 -g0 -I$(LVGL_DIR)/$(LVGL_DIR_NAME) -I$(LVGL_DIR)

export LDFLAGS ?= -lm -ldbus-1 -lpthread -lpng -ljpeg -lharfbuzz -lfreetype -lstdc++ -lc
BIN = Main


# 从make传递的参数获取子应用的编译速度设置，示例：make -j16 RUN_JOBS=-j16
RUN_JOBS = 

#Collect the files to compile
MAINSRC = ./main.cpp
# CPPSRCS +=  ./main.cpp
CSRCS += $(shell find -L ./src/  -name "*.c")
CPPSRCS += $(shell find -L ./src/  -name "*.cpp")



include $(LVGL_DIR)/lvgl/lvgl.mk
include $(LVGL_DIR)/lv_drivers/lv_drivers.mk
# include $(LVGL_DIR)/lv_lib_png/lv_lib_png.mk
# include $(LVGL_DIR)/lv_100ask_modules/lv_100ask_modules.mk
#include $(LVGL_DIR)/lv_100ask_app/lv_100ask_app.mk


OBJEXT ?= .o

AOBJS = $(ASRCS:.S=$(OBJEXT))
COBJS = $(CSRCS:.c=$(OBJEXT))
CPPOBJS = $(CPPSRCS:.cpp=$(OBJEXT))

MAINOBJ = $(MAINSRC:.cpp=$(OBJEXT))

SRCS = $(ASRCS) $(CSRCS) $(MAINSRC)
OBJS = $(AOBJS) $(COBJS)

## MAINOBJ -> OBJFILES
# # imx6ull app
# SRC_DIR  +=  app/picture\
# 			 application/imx6ull_set_time\
# 			 application/imx6ull_set_wlan\
# 			 application/picture

#通过 foreach 函数迭代 SRC_DIR 中定义的每一个子目录（或源文件路径），并在这些目录中执行 make 命令
all: default
	$(foreach  dir,$(SRC_DIR),make $(RUN_JOBS) -C $(dir);)
	

%.o: %.c
	$(CC)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"

%.o: %.cpp
	$(CXX)  $(CFLAGS) -c $< -o $@
	@echo "CC $<"
    
default: $(AOBJS) $(COBJS) $(MAINOBJ) $(CPPOBJS)
	$(CXX) -o $(BIN) $(MAINOBJ) $(AOBJS) $(COBJS) $(CPPOBJS) $(LDFLAGS)
	mkdir -p $(LVGL_DIR)/obj $(LVGL_DIR)/bin
	mv *.o $(LVGL_DIR)/obj/
	mv $(BIN) $(LVGL_DIR)/bin/

clean: 
	rm -f $(BIN) $(AOBJS) $(COBJS) $(MAINOBJ) ./bin/* ./obj/*
	$(foreach  dir,$(SRC_DIR),make -C $(dir) clean;)

