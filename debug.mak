BINARYDIR := Debug

COMPILER_PATH :=
COMPILER_LIB_PATH :=

# Cortex-A7编译器
#COMPILER_PATH += /opt/a7_toolchain/gcc-linaro-arm-linux-gnueabihf-4.9-2014.09_linux/bin/arm-linux-gnueabihf-
#COMPILER_PATH += /opt/gcc-linaro-4.9.4-2017.01-x86_64_arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
#COMPILER_LIB_PATH +=

# 龙芯2K1000编译器
#COMPILER_PATH += /opt/output/host/bin/mips64el-linux-
#COMPILER_LIB_PATH +=

# RK3568编译器
#COMPILER_PATH += /opt/m3568-sdk-v1.0.0-ga/host/bin/aarch64-linux-
#COMPILER_LIB_PATH += /opt/m3568-sdk-v1.0.0-ga/gcc-buildroot-9.3.0-2020.03-x86_64_aarch64-rockchip-linux-gnu/lib

# TQ3568编译器
COMPILER_PATH += /opt/TQ3568/aarch64-linux-gcc-v9.3/bin/aarch64-linux-
COMPILER_LIB_PATH += /opt/TQ3568/aarch64-linux-gcc-v9.3/lib

CC := $(COMPILER_PATH)gcc
CXX := $(COMPILER_PATH)g++
LD := $(CXX)
AR := $(COMPILER_PATH)ar
OBJCOPY := $(COMPILER_PATH)objcopy

PREPROCESSOR_MACROS := DEBUG
INCLUDE_DIRS := ./include
LIBRARY_DIRS := $(COMPILER_LIB_PATH)
LIBRARY_DIRS += ./ArtcLib
LIBRARY_NAMES := pthread iconv msc_rk dl
ADDITIONAL_LINKER_INPUTS :=
MACOS_FRAMEWORKS :=

CFLAGS := -ffunction-sections -O0 #-Wall -Wextra
CXXFLAGS := -ffunction-sections -O0 #-Wall -Wextra

# 去除函数未使用报警信息
CFLAGS += -Wno-unused-function
CXXFLAGS += -Wno-unused-function

# 去除未使用变量的报警信息
CFLAGS += -Wno-unused-parameter
CXXFLAGS += -Wno-unused-parameter

# 遇到第一个错误就停止，减少排查时间
CFLAGS += -Wfatal-errors
CXXFLAGS += -Wfatal-errors

# 设置报警级别
CFLAGS += -Wimplicit-fallthrough=0
CXXFLAGS += -Wimplicit-fallthrough=0

ASFLAGS :=
LDFLAGS := -Wl,-gc-sections
COMMONFLAGS :=

START_GROUP := -Wl,--start-group
END_GROUP := -Wl,--end-group

IS_LINUX_PROJECT := 1
