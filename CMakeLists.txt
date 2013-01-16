# I have no idea if this version is right.
cmake_minimum_required(VERSION 2.8)

# This was recommended by the LLVM documentation for "out-of-source" pass
# builds.
set(LLVM_DIR build/built/share/llvm/cmake)
find_package(LLVM)
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# I made this up to make it slightly easier to write CMake files for Clang
# plugins.
macro(add_clang_plugin name)
    add_llvm_loadable_module( ${name} ${ARGN} )
    set_target_properties( ${name} PROPERTIES 
        LINKER_LANGUAGE CXX
        PREFIX ""
        # Necessary because LLVM itself has RTTI turned off by default.
        COMPILE_FLAGS "-fno-rtti"  
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
add_subdirectory(tracer)
