BUILD := build
BUILT := $(BUILD)/built
LLVM_VERSION := 3.2

# Program names.
CMAKE := cmake
VIRTUALENV := virtualenv

# Location of the Python virtual environment.
VENV := venv

# CMake options for building LLVM and the ACCEPT pass.
CMAKE_FLAGS := -G Ninja -DCMAKE_INSTALL_PREFIX:PATH=$(shell pwd)/$(BUILT)
ifeq ($(RELEASE),1)
CMAKE_FLAGS += -DCMAKE_BUILD_TYPE:STRING=Release
else
CMAKE_FLAGS += -DCMAKE_BUILD_TYPE:STRING=Debug
endif

ifeq ($(shell uname -s),Darwin)
        LIBEXT := dylib
else
        LIBEXT := so
endif

# Automatically use a binary called "ninja-build", if it's available. Some
# package managers call it this to avoid naming conflicts.
ifeq ($(shell which ninja-build >/dev/null 2>&1 ; echo $$?),0)
	NINJA := ninja-build
else
	NINJA := ninja
endif

# LLVM 3.2 has some trouble building against libc++, which seems to be the
# default standard library on recent OS X dev tools. Presumably this is fixed
# in later versions of LLVM, but for now, we force the compiler to use GNU
# libstdc++.
ifeq ($(LLVM_VERSION),3.2)
ifneq ($(shell c++ --version | grep clang),)
	CMAKE_FLAGS += '-DCMAKE_CXX_FLAGS:STRING=-stdlib=libstdc++ -std=gnu++98'
endif
endif

# On platforms that ship Python 3 as `python`, force Python 2 to be used in
# CMake. I don't think CMake itself has a problem with py3k, but LLVM's
# scripts do.
ifeq ($(shell which python2 >/dev/null 2>&1 ; echo $$?),0)
	CMAKE_FLAGS += -DPYTHON_EXECUTABLE:PATH=$(shell which python2)
endif


# Actually building stuff.

.PHONY: accept llvm

accept: check_cmake check_ninja
	mkdir -p $(BUILD)/enerc
	cd $(BUILD)/enerc ; $(CMAKE) $(CMAKE_FLAGS) ../..
	cd $(BUILD)/enerc ; $(NINJA) install

llvm: llvm/CMakeLists.txt llvm/tools/clang check_cmake check_ninja
	mkdir -p $(BUILD)/llvm
	cd $(BUILD)/llvm ; $(CMAKE) $(CMAKE_FLAGS) ../../llvm
	cd $(BUILD)/llvm ; $(NINJA) install


# Convenience targets.

.PHONY: setup test clean

setup: llvm accept driver

test:
	$(BUILT)/bin/llvm-lit -v test

clean:
	rm -rf $(BUILD)


# Fetching and extracting LLVM.

.INTERMEDIATE: llvm-$(LLVM_VERSION).src.tar.gz
llvm-$(LLVM_VERSION).src.tar.gz:
	curl -O http://llvm.org/releases/$(LLVM_VERSION)/$@

llvm/CMakeLists.txt: llvm-$(LLVM_VERSION).src.tar.gz
	tar -xf $<
	mv llvm-$(LLVM_VERSION).src llvm

# Symlink our modified Clang source into the LLVM tree. This way, building the
# "llvm" directory will build both LLVM and Clang. (In fact, this is the only
# way to build Clang at all as far as I know.)
llvm/tools/clang: llvm/CMakeLists.txt
	cd llvm/tools ; ln -s ../../clang .


# Friendly error messages when tools don't exist.

.PHONY: check_cmake check_ninja check_virtualenv

check_cmake:
	@if ! $(CMAKE) --version > /dev/null ; then \
		echo "Please install CMake to build LLVM and ACCEPT."; \
		echo "http://www.cmake.org"; \
		exit 2; \
	else true; fi

check_ninja:
	@if ! $(NINJA) --version > /dev/null ; then \
		echo "Please install Ninja to build LLVM and ACCEPT."; \
		echo "http://martine.github.io/ninja/"; \
		exit 2; \
	else true; fi

check_virtualenv:
	@if ! $(VIRTUALENV) --version > /dev/null ; then \
		echo "Please install Virtualenv to use the ACCEPT driver."; \
		echo "http://www.virtualenv.org/"; \
		exit 2; \
	else true; fi


# Python driver installation.

.PHONY: driver

driver:
	# Make a virtualenv and install all our dependencies there. This avoids
	# needing to clutter the system Python libraries (and possibly requiring
	# sudo).
	[ -e $(VENV)/bin/pip ] || virtualenv $(VENV)
	./$(VENV)/bin/pip install -r requirements.txt


# Documentation.

.PHONY: docs cleandocs deploy

docs:
	mkdocs build

cleandocs:
	rm -rf site

# Upload the documentation to the Web server.
CSEHOST := bicycle.cs.washington.edu
CSEPATH := /cse/www2/sampa/accept
deploy: cleandocs docs
	rsync --compress --recursive --checksum --itemize-changes --delete -e ssh site/ $(CSEHOST):$(CSEPATH)
	ssh $(CSEHOST) "echo -e 'authtype csenetid\\nrequire valid-user' > $(CSEPATH)/.htaccess"


# Experiments for the paper.

.PHONY: exp exp_setup

APPS := streamcluster sobel canneal fluidanimate x264
APPSDIR := apps
EXP_PY_DEPS := munkres pillow
PARSEC_INPUTS := \
	apps/canneal/40000.nets \
	apps/canneal/2500000.nets \
	apps/fluidanimate/in_100K.fluid \
	apps/fluidanimate/in_300K.fluid \
	apps/x264/eledream_640x360_32.y4m \
	apps/x264/eledream_640x360_128.y4m

# Reduce reps with MINI=1.
ifneq ($(MINI),)
ACCEPT_ARGS := -r1 -R1
else
ACCEPT_ARGS := -r2 -R5
endif

# Get a non-timing run with NOTIME=1. Disables forcing.
ifeq ($(NOTIME),)
ACCEPT_ARGS += -f
EXP_ARGS += -t
endif

# Run on cluster with CLUSTER=1.
ifneq ($(CLUSTER),)
ACCEPT_ARGS += -c
endif

exp:
	./bin/accept $(ACCEPT_ARGS) -v exp -j $(EXP_ARGS) $(APPS:%=$(APPSDIR)/%)

exp_setup:
	./$(VENV)/bin/pip install $(EXP_PY_DEPS)
ifeq ($(CLUSTER),1)
	./$(VENV)/bin/pip install git+git://github.com/sampsyo/cluster-workers.git
endif
