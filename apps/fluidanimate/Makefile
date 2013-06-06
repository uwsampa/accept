TARGET := fluidanimate

RUNARGS := 1 5 in_100K.fluid out.fluid  # simmedium

LDFLAGS := -lpthread
CXXFLAGS += -pthread -D_GNU_SOURCE -D__XOPEN_SOURCE=600

APP_MK := ../app.mk
include $(APP_MK)
