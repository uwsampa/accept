###
# Generic Makefile for enerc benchmark apps.
###
# To provide arguments to the program when profiling, set RUNARGS.
# To provide extra arguments to clang, set CLANGARGS.
# To run the executable through something else (e.g., a simulator), set RUNSHIM
###
# Don't use this file directly!  Include it from a Makefile in an app's
# subdirectory or else the paths to various tools will be wrong.
###

ENERCDIR := ../..
INCLUDEDIR := $(ENERCDIR)/include
BUILTDIR := $(ENERCDIR)/build/built
CC := $(BUILTDIR)/bin/clang
CXX := $(BUILTDIR)/bin/clang++
LLVMDIS := $(BUILTDIR)/bin/llvm-dis
LLVMLINK := $(BUILTDIR)/bin/llvm-link
LLVMOPT := $(BUILTDIR)/bin/opt
LLVMLLC := $(BUILTDIR)/bin/llc
SUMMARY := $(ENERCDIR)/bin/summary.py

ifeq ($(shell uname -s),Darwin)
	LIBEXT := dylib
else
	LIBEXT := so
endif
ENERCLIB ?= $(BUILTDIR)/lib/EnerCTypeChecker.$(LIBEXT)
PASSLIB ?= $(BUILTDIR)/lib/enerc.$(LIBEXT)
PROFLIB ?= $(BUILTDIR)/../enerc/rt/enercrt.bc

override CFLAGS += -Xclang -load -Xclang $(ENERCLIB) \
	-Xclang -add-plugin -Xclang enerc-type-checker \
	-Xclang -load -Xclang $(PASSLIB) \
	-g -fno-use-cxa-atexit \
	-I$(INCLUDEDIR) -emit-llvm
override CXXFLAGS += $(CFLAGS)

# SOURCES is a list of source files, *.{c,cpp} by default
SOURCES ?= $(wildcard *.c) $(wildcard *.cpp)

BCFILES := $(SOURCES:.c=.bc)
BCFILES := $(BCFILES:.cpp=.bc)
LINKEDBC := $(TARGET)_all.bc
LLFILES := $(BCFILES:.bc=.ll)

# attempt to guess which linker to use
ifneq ($(filter %.cpp,$(SOURCES)),)
	LINKER ?= $(CXX)
else
	LINKER ?= $(CC)
endif

#################################################################
.PHONY: all build profile clean run

all: build profile

build: $(LINKEDBC)

run: build $(TARGET)
	$(RUNSHIM) ./$(TARGET) $(RUNARGS) || true

profile: run
	$(SUMMARY)
#################################################################

# make LLVM bitcode from C/C++ sources
.SUFFIXES: .bc .ll
.c.bc:
	$(CC) $(CFLAGS) $(CLANGARGS) -c -o $@ $<
.cpp.bc:
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLANGARGS) -c -o $@ $<
.bc.ll:
	$(LLVMDIS) $<

# Link component bitcode files into a single file.
$(LINKEDBC): $(BCFILES) $(PROFLIB)
	$(LLVMLINK) $^ > $@

ifeq ($(ARCH),msp430)
# llc cannot generate object code for msp430, so emit assembly
.INTERMEDIATE: $(TARGET).s
$(TARGET).s: $(LINKEDBC)
	$(LLVMOPT) -strip $^ | \
	$(LLVMLLC) -march=msp430 > $@
$(TARGET).o: $(TARGET).s
	msp430-gcc $(MSPGCC_CFLAGS) -c $<
else
$(TARGET).o: $(LINKEDBC)
	$(LLVMOPT) -strip $^ | \
	$(LLVMLLC) -filetype=obj > $@
endif

# make the final executable $(TARGET)
$(TARGET): $(TARGET).o
	$(LINKER) $(LDFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET) $(TARGET).o $(BCFILES) $(LLFILES) $(LINKEDBC) \
	enerc_static.txt enerc_dynamic.txt \
	accept_config.txt accept_config_desc.txt accept_log.txt \
	$(CLEANMETOO)
