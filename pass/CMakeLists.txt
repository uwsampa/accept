add_llvm_loadable_module(enerc
  enerc.cpp
  acceptaa.cpp
  registration.cpp
)
set_target_properties( enerc PROPERTIES 
    COMPILE_FLAGS "-fno-rtti -fvisibility-inlines-hidden"
)
