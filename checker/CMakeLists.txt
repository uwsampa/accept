set(SOURCES
  EnerCTypeChecker.cpp
)

add_clang_plugin(EnerCTypeChecker ${SOURCES})

include_directories(${CHECKERLIB_SOURCE_DIR} ${Project_SOURCE_DIR}/include)
target_link_libraries(EnerCTypeChecker checkerlib)

