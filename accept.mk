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

# Paths to components of the ACCEPT toolchain.
ACCEPTDIR := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))
INCLUDEDIR := $(ACCEPTDIR)/include
BUILTDIR := $(ACCEPTDIR)/build/built
CC := $(BUILTDIR)/bin/clang
CXX := $(BUILTDIR)/bin/clang++
LLVMDIS := $(BUILTDIR)/bin/llvm-dis
LLVMLINK := $(BUILTDIR)/bin/llvm-link
LLVMOPT := $(BUILTDIR)/bin/opt
LLVMLLC := $(BUILTDIR)/bin/llc
LLVMLLI := $(BUILTDIR)/bin/lli
RTDIR := $(ACCEPTDIR)/rt
VEDIR := $(ACCEPTDIR)/venv
NNTUNEDIR := $(ACCEPTDIR)/nntune
FANNDIR := $(ACCEPTDIR)/fann-snnap

# Target platform specifics.
ARCH ?= default
RTLIB ?= $(RTDIR)/acceptrt.$(ARCH).bc

# Linked BCs
FANNARGS := -I$(FANNDIR)/src/include -I$(FANNDIR)/src/
FANNLIBSRC := $(FANNDIR)/src/floatfann.c
FANNLIBDST := $(RTDIR)/floatfann.bc
EXTRABC += $(RTLIB) $(FANNLIBDST)

# Virtual Env Python
VEPYTHON := $(VEDIR)/bin/python2.7

# Error Target
MSETARGET := 0.01

# Host platform specifics.
ifeq ($(shell uname -s),Darwin)
	XCODEINCLUDES = $(shell xcrun --show-sdk-path)/usr/include
	ifeq ($(ARCH),default)
		CFLAGS += -I$(XCODEINCLUDES)
		CXXFLAGS += -I$(XCODEINCLUDES)
	endif
	LIBEXT := dylib
else
	# Work around Clang include path search bug on Debian/Ubuntu.
	# https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=697127
	# This manifests as clang being unable to find `bits/c++config.h` when
	# compiling any C++. We manually amend the include path.
	CXXLIBPATH := $(wildcard /usr/include/*-linux-gnu/c++/4.*)
	ifneq ($(CXXLIBPATH),)
		CXXFLAGS += $(CXXLIBPATH:%=-I%)
	endif
	LIBEXT := so
endif
ENERCLIB ?= $(BUILTDIR)/lib/EnerCTypeChecker.$(LIBEXT)
PASSLIB ?= $(BUILTDIR)/lib/enerc.$(LIBEXT)

# General compiler flags.
override CFLAGS += -I$(INCLUDEDIR) -g -fno-use-cxa-atexit
override CXXFLAGS += $(CFLAGS)
LLCARGS += -O2

# Compiler flags to pass to Clang to add the ACCEPT machinery.
ENERCFLAGS :=  -Xclang -load -Xclang $(ENERCLIB) \
	-Xclang -add-plugin -Xclang enerc-type-checker

# SOURCES is a list of source files, *.{c,cpp} by default.
SOURCES ?= $(wildcard *.c) $(wildcard *.cpp)

# Build products.
TARGET ?= app
BCFILES := $(SOURCES:.c=.bc)
BCFILES := $(BCFILES:.cpp=.bc)
LINKEDBC := $(TARGET)_all.bc
LLFILES := $(BCFILES:.bc=.ll)

# Attempt to guess which linker to use.
ifneq ($(filter %.cpp,$(SOURCES)),)
	LINKER ?= $(CXX)
else
	LINKER ?= $(CC)
endif

# Determine which command-line arguments to use depending on whether we are
# "training" (profiling/measuring) or "testing" (performing a final
# evaluation).
ifneq ($(ACCEPT_TEST),)
	ifneq ($(TESTARGS),)
		RUNARGS = $(TESTARGS)
	endif
endif

#################################################################
# The different executable configurations we can build.
CONFIGS := orig opt dummy

BUILD_TARGETS := $(CONFIGS:%=build_%)
RUN_TARGETS := $(CONFIGS:%=run_%)
.PHONY: all setup clean profile $(BUILD_TARGETS) $(RUN_TARGETS)

all: build_orig

# See below for BUILD_TARGETS rule

$(RUN_TARGETS): run_%: $(TARGET).%
	$(RUNSHIM) ./$< $(RUNARGS)

# Blank setup target (for overriding).
setup:
#################################################################

# BUILD_TARGETS rule is here to catch any EXTRADEPS assigned above
$(BUILD_TARGETS): build_%: setup $(EXTRADEPS) $(TARGET).%

# Make LLVM bitcode from C/C++ sources.
%.bc: %.c $(HEADERS)
	$(CC) $(ENERCFLAGS) $(CFLAGS) $(CLANGARGS) -c -emit-llvm -o $@ $<
%.bc: %.cpp $(HEADERS)
	$(CXX) $(ENERCFLAGS) $(CXXFLAGS) $(CLANGARGS) -c -emit-llvm -o $@ $<
%.ll: %.bc
	$(LLVMDIS) $<

# Make the ACCEPT runtime library for the target architecture.
$(RTLIB):
	make -C $(RTDIR) acceptrt.$(ARCH).bc CC="$(CC)" CFLAGS="$(CFLAGS) $(FANNARGS)"

# Compile additional libraries (FANN)
$(FANNLIBDST): $(FANNLIBSRC)
	$(CC) $(CFLAGS) $(FANNARGS) $(CLANGARGS) -c -emit-llvm -o $@ $<

# Link component bitcode files into a single file.
$(LINKEDBC): $(BCFILES) $(EXTRABC)
	$(LLVMLINK) $^ > $@

# For debugging: we can also disassemble to .ll files.
#	for f in $(BCFILES); do \
#		$(LLVMDIS) $$f; \
#	done

# Versions of the amalgamated program.
$(TARGET).orig.bc: $(LINKEDBC)
	$(LLVMOPT) -load $(PASSLIB) -O1 $(OPTARGS) $< -o $@
$(TARGET).opt.bc: $(LINKEDBC) accept_config.txt
	$(LLVMOPT) -load $(PASSLIB) -O1 -accept-relax $(OPTARGS) $< -o $@
$(TARGET).dummy.bc: $(LINKEDBC)
	cp $< $@

# .bc -> .s
$(TARGET).%.s: $(TARGET).%.bc
	$(LLVMLLC) $(LLCARGS) $< > $@

# .s -> executable (assemble and link)
$(TARGET).%: $(TARGET).%.s
	$(LINKER) $(LDFLAGS) -o $@ $< $(LIBS)

# Derive Statistics
train:
	$(VEPYTHON) $(NNTUNEDIR)/nntune.py -train accept_npulog.txt -error_target $(MSETARGET)


clean:
	$(RM) $(TARGET) $(TARGET).s $(BCFILES) $(LLFILES) $(LINKEDBC) \
	accept-globals-info.txt accept_config.txt accept_config_desc.txt \
	accept_log.txt accept_time.txt accept_npulog.txt \
	$(CONFIGS:%=$(TARGET).%.bc) $(CONFIGS:%=$(TARGET).%) \
	accept-approxRetValueFunctions-info.txt accept-npuArrayArgs-info.txt \
	$(CLEANMETOO)
	for SUBDIR in $(SUBDIRS); do make -C "$$SUBDIR" clean; done
