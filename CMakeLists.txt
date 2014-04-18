# I have no idea if this version is right.
cmake_minimum_required(VERSION 2.8)

# Incantation to get Ninja to give us color output.
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcolor-diagnostics")
endif()
if("${CMAKE_C_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcolor-diagnostics")
endif()

execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE XCODESDKPATH OUTPUT_STRIP_TRAILING_WHITESPACE)

# This was recommended by the LLVM documentation for "out-of-source" pass
# builds.
set(LLVM_DIR build/built/share/llvm/cmake)
find_package(LLVM)
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${XCODESDKPATH}/usr/include)
link_directories(${LLVM_LIBRARY_DIRS})

# I made this up to make it slightly easier to write CMake files for Clang
# plugins.
macro(add_clang_plugin name)
    add_llvm_loadable_module( ${name} ${ARGN} )
    set_target_properties( ${name} PROPERTIES 
        LINKER_LANGUAGE CXX
        PREFIX ""
        # Match the flags of LLVM itself. RTTI is turned off by default and
        # inlines are invisible.
        COMPILE_FLAGS "-fno-rtti -fvisibility-inlines-hidden"
    )
    install(TARGETS ${name}
        LIBRARY DESTINATION lib${LLVM_LIBDIR_SUFFIX}
        ARCHIVE DESTINATION lib${LLVM_LIBDIR_SUFFIX}
    )
endmacro(add_clang_plugin)

# Finally, the subprojects we actually intend to build.
add_subdirectory(pass)
add_subdirectory(checkerlib)
add_subdirectory(checker)
add_subdirectory(rt)
