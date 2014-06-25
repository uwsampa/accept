TARGET := fluidanimate

RUNARGS := 1 5 in_100K.fluid out.fluid  # simmedium
TESTARGS := 4 5 in_300K.fluid out.fluid  # simlarge

SOURCES := pthreads.cpp cellpool.cpp parsec_barrier.cpp
HEADERS := cellpool.hpp fluid.hpp parsec_barrier.hpp
LDFLAGS := -lpthread
CXXFLAGS += -pthread -D_GNU_SOURCE -D__XOPEN_SOURCE=600 -Wno-invalid-offsetof
CLEANMETOO := fluidcmp out.fluid

APP_MK := ../app.mk
include $(APP_MK)

# Output evaluation tool. Not approximated.
fluidcmp: fluidcmp.cpp
	$(CXX) -I$(ENERCDIR)/include $< -o $@

# Compile the evaluation tool before the workflow begins.
setup: fluidcmp
