cmake_minimum_required(VERSION 2.8)  # I have no idea if this is right.

set(LLVM_DIR llvm-build/share/llvm/cmake)
find_package(LLVM)

# Define add_llvm_* macro's.
include(AddLLVM)

message(STATUS ${LLVM_INCLUDE_DIRS})

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

add_subdirectory(enerc-pass)
