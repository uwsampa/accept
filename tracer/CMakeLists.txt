set(LLVM_LINK_COMPONENTS mcjit jit interpreter nativecodegen bitreader asmparser selectiondag native)

set(LLVM_BUILD_TOOLS true)
add_llvm_tool(enerci
  enerci.cpp
)
set_target_properties(enerci PROPERTIES 
    COMPILE_FLAGS "-fno-rtti"  
)
