TARGET := blackscholes
LDFLAGS := -lm -pthread
CFLAGS := -pthread -DENABLE_THREADS

SOURCES := blackscholes.m4.cpp

RUNARGS := 1 in_64K.txt output.txt  # simlarge
TESTARGS := 1 in_10M.txt output.txt  # native
# RUNARGS := 4 in_1M.txt output.txt  # in between

CLEANMETOO := output.txt blackscholes.m4.cpp

APP_MK := ../app.mk
include $(APP_MK)

blackscholes.m4.cpp: blackscholes.c c.m4.pthreads
	m4 c.m4.pthreads $< > $@
