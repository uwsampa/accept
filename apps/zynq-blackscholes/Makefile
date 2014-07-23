ARCH := zynq
TARGET := blackscholes
CFLAGS := -pthread
ZYNQBIT := blackscholes_x8_x64.bit
LDFLAGS := -lm -pthread
OPTARGS += -accept-npu-bufsize=896

RUNARGS := 1 in_64K.txt output.txt  # simlarge
TESTARGS := 1 in_64K.txt output.txt  # simlarge

APP_MK := ../app.mk
include $(APP_MK)

#blackscholes.m4.cpp: blackscholes.c c.m4.pthreads
#	m4 c.m4.pthreads $< > $@
