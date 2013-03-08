###
# Generic Makefile for enerc benchmark apps.
###
# To provide arguments to the program when profiling, set RUNARGS.
# To provide extra arguments to clang, set CLANGARGS.
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
ENERCLIB=$(BUILTDIR)/lib/EnerCTypeChecker.$(LIBEXT)
PASSLIB=$(BUILTDIR)/lib/enerc.$(LIBEXT)
PROFLIB=$(BUILTDIR)/../enerc/rt/enercrt.bc

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
LLFILES := $(BCFILES:.bc=.ll)

# attempt to guess which linker to use
ifneq ($(filter %.cpp,$(SOURCES)),)
	LINKER ?= $(CXX)
else
	LINKER ?= $(CC)
endif

#################################################################
.PHONY: all build profile clean

all: build profile

build: $(BCFILES) $(LLFILES)

profile: build $(TARGET)
	./$(TARGET) $(RUNARGS) || true
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

# make the final executable $(TARGET)
# .INTERMEDIATE: $(TARGET).o # XXX leave this .o file around?
$(TARGET).o: $(BCFILES)
	$(LLVMLINK) $(PROFLIB) $^ | \
		$(LLVMOPT) -strip | \
		$(LLVMLLC) -filetype=obj > $@
$(TARGET): $(TARGET).o
	$(LINKER) $(LDFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET) $(TARGET).o $(BCFILES) $(LLFILES) \
	enerc_static.txt enerc_dynamic.txt
