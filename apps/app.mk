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
ENERCDIR := $(shell pwd)/$(dir $(lastword $(MAKEFILE_LIST)))/..
INCLUDEDIR := $(ENERCDIR)/include
BUILTDIR := $(ENERCDIR)/build/built
CC := $(BUILTDIR)/bin/clang
CXX := $(BUILTDIR)/bin/clang++
LLVMDIS := $(BUILTDIR)/bin/llvm-dis
LLVMLINK := $(BUILTDIR)/bin/llvm-link
LLVMOPT := $(BUILTDIR)/bin/opt
LLVMLLC := $(BUILTDIR)/bin/llc
LLVMLLI := $(BUILTDIR)/bin/lli

# Platform specifics.
ifeq ($(shell uname -s),Darwin)
	ifeq ($(shell uname -r | sed -e 's/\..*//'),13) # OS X 10.9
		XCODEINCLUDES = $(shell xcrun --show-sdk-path)/usr/include
		CFLAGS += -I$(XCODEINCLUDES)
		CXXFLAGS += -I$(XCODEINCLUDES)
	endif
	LIBEXT := dylib
else
	LIBEXT := so
endif
ENERCLIB ?= $(BUILTDIR)/lib/EnerCTypeChecker.$(LIBEXT)
PASSLIB ?= $(BUILTDIR)/lib/enerc.$(LIBEXT)
PROFLIB ?= $(BUILTDIR)/../enerc/rt/enercrt.bc
LIBPROFILERT := $(BUILTDIR)/lib/libprofile_rt.$(LIBEXT)

# Compiler flags to pass to Clang to add the ACCEPT machinery.
override CFLAGS += -Xclang -load -Xclang $(ENERCLIB) \
	-Xclang -add-plugin -Xclang enerc-type-checker \
	-g -fno-use-cxa-atexit \
	-I$(INCLUDEDIR) -emit-llvm
override CXXFLAGS += $(CFLAGS)

# SOURCES is a list of source files, *.{c,cpp} by default.
SOURCES ?= $(wildcard *.c) $(wildcard *.cpp)

# Build products.
TARGET := app
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

# The different executable configurations we can build.
CONFIGS := orig opt prof

# Determine which command-line arguments to use depending on whether we are
# "training" (profiling/measuring) or "testing" (performing a final
# evaluation).
ifneq ($(ACCEPT_TEST),)
	ifneq ($(TESTARGS),)
		RUNARGS = $(TESTARGS)
	endif
endif

#################################################################
BUILD_TARGETS := $(CONFIGS:%=build_%)
RUN_TARGETS := $(CONFIGS:%=run_%)
.PHONY: all clean profile $(BUILD_TARGETS) $(RUN_TARGETS)

all: build_orig

$(BUILD_TARGETS): build_%: $(TARGET).%

$(RUN_TARGETS): run_%: $(TARGET).%
	$(RUNSHIM) ./$< $(RUNARGS)
#################################################################

# make LLVM bitcode from C/C++ sources
%.bc: %.c $(HEADERS)
	$(CC) $(CFLAGS) $(CLANGARGS) -c -o $@ $<
%.bc: %.cpp $(HEADERS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(CLANGARGS) -c -o $@ $<
%.ll: %.bc
	$(LLVMDIS) $<

# Link component bitcode files into a single file.
ifeq ($(ARCH),msp430)
$(LINKEDBC): $(BCFILES)
else
$(LINKEDBC): $(BCFILES) $(PROFLIB)
endif
	$(LLVMLINK) $^ > $@
	for f in $(BCFILES); do \
		$(LLVMDIS) $$f; \
	done

# Three different transformations of the amalgamated program.
$(TARGET).prof.bc: $(LINKEDBC)
	$(LLVMOPT) -insert-edge-profiling $< -o $@
$(TARGET).orig.bc: $(LINKEDBC)
	$(LLVMOPT) -load $(PASSLIB) -O3 $< -o $@
$(TARGET).opt.bc: $(LINKEDBC) accept_config.txt
	$(LLVMOPT) -load $(PASSLIB) -accept-relax -O3 $< -o $@

# .bc -> .o
ifeq ($(ARCH),msp430)
# llc cannot generate object code for msp430, so emit assembly
.INTERMEDIATE: $(TARGET).%.s
$(TARGET).%.s: $(TARGET).%.bc
	$(LLVMOPT) -strip $< | \
	$(LLVMLLC) -march=msp430 > $@
$(TARGET).%.o: $(TARGET).%.s
	msp430-gcc $(MSPGCC_CFLAGS) -c $<
else
$(TARGET).%.o: $(TARGET).%.bc
	$(LLVMOPT) -strip $< | \
	$(LLVMLLC) -filetype=obj > $@
endif

# .o -> executable
$(TARGET).%: $(TARGET).%.o
	$(LINKER) $(LDFLAGS) -o $@ $<

clean:
	$(RM) $(TARGET) $(TARGET).o $(BCFILES) $(LLFILES) $(LINKEDBC) \
	accept-globals-info.txt accept_config.txt accept_config_desc.txt \
	accept_log.txt accept_time.txt \
	$(CONFIGS:%=$(TARGET).%.bc) $(CONFIGS:%=$(TARGET).%) \
	$(CLEANMETOO)
