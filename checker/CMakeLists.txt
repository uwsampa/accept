set(SOURCES
  EnerCTypeChecker.cpp
)

add_clang_plugin(EnerCTypeChecker ${SOURCES})

# Link to checkerlib.
include_directories(${CHECKERLIB_SOURCE_DIR})
target_link_libraries(EnerCTypeChecker checkerlib)

