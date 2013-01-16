add_llvm_loadable_module(enerc
  enerc.cpp
)
set_target_properties( enerc PROPERTIES 
    COMPILE_FLAGS "-fno-rtti"  
)
