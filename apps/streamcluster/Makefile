RUNARGS := 10 20 64 8192 8192 1000 none output.txt 4  # simmedium
#RUNARGS := 10 20 128 16384 16384 1000 none output.txt 4  # simlarge
TESTARGS := 10 20 128 1000000 200000 5000 none output.txt 4  # native

LDFLAGS := -lpthread
CFLAGS := -DENABLE_THREADS
CLEANMETOO := output.txt

APP_MK := ../app.mk
include $(APP_MK)
