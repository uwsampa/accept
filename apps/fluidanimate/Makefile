TARGET := fluidanimate

# RUNARGS := 1 5 in_100K.fluid out.fluid  # simmedium
RUNARGS := 1 5 in_300K.fluid out.fluid  # simlarge

SOURCES := pthreads.cpp cellpool.cpp parsec_barrier.cpp
LDFLAGS := -lpthread
CXXFLAGS += -pthread -D_GNU_SOURCE -D__XOPEN_SOURCE=600 -Wno-invalid-offsetof
CLEANMETOO := fluidcmp out.fluid

# Output evaluation tool. Not approximated.
fluidcmp: fluidcmp.cpp
	$(CXX) $< -o $@
BUILD_EXTRA := fluidcmp

APP_MK := ../app.mk
include $(APP_MK)
