add_llvm_loadable_module(enerc
  # Scaffolding.
  transform.cpp
  registration.cpp
  approxinfo.cpp
  log.cpp

  # Optimizations.
  loopperf.cpp
  acceptaa.cpp
  desync.cpp
  npu.cpp
  error.cpp
  snnap.cpp
)
set_target_properties( enerc PROPERTIES 
    COMPILE_FLAGS "-fno-rtti -fvisibility-inlines-hidden"
)
