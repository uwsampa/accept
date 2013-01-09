set(SOURCES
  EnerCTypeChecker.cpp
  NodeTyper.h
  TyperConsumer.cpp
  TyperConsumer.h
  TyperVisitor.h
)

add_clang_plugin(EnerCTypeChecker ${SOURCES})
