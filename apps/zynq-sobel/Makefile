ARCH := zynq
TARGET := sobel
ZYNQBIT := sobel_x8_x64.bit
LDFLAGS := -lm
OPTARGS += -accept-npu-bufsize=1152

RUNARGS := lena.dat
TESTARGS := baboon.dat

APP_MK := ../app.mk
include $(APP_MK)
