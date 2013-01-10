# I have no idea if this version is right.
cmake_minimum_required(VERSION 2.8)

# First, include LLVM and Clang.
add_subdirectory(llvm)

# This was recommended by the LLVM documentation for "out-of-source" pass
# builds.
set(LLVM_DIR llvm/cmake/modules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/llvm/cmake/modules)
find_package(LLVM)
include(HandleLLVMOptions)  # Set LLVM_ON_UNIX, etc.
include(AddLLVM)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})

# I made this up to make it slightly easier to write CMake files for Clang
# plugins.
macro(add_clang_plugin name)
    add_library( ${name} SHARED ${ARGN} )
    target_link_libraries( ${name} clangFrontend clangAST )
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
add_subdirectory(checker)
add_subdirectory(rt)
