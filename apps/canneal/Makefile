TARGET := canneal
CFLAGS := -DENABLE_THREADS -pthread

# RUNARGS := 1 15000 2000 2500000.nets 6000  # native
# RUNARGS := 1 15000 2000 400000.nets 128  # simlarge
RUNARGS := 4 15000 2000 200000.nets 64  # simmedium
# RUNARGS := 1 10000 2000 100000.nets 32  # simsmall
# RUNARGS := 1 100 300 100.nets 2  # simdev

CLEANMETOO := output.txt

APP_MK := ../app.mk
include $(APP_MK)
