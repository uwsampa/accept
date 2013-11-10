TARGET := streamcluster

RUNARGS := 10 20 128 1000000 200000 5000 none output.txt 4  # native
#RUNARGS := 10 20 128 16384 16384 1000 none output.txt 4  # simlarge

LDFLAGS := -lpthread
CFLAGS := -DENABLE_THREADS
CLEANMETOO := output.txt

APP_MK := ../app.mk
include $(APP_MK)
