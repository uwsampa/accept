project(CHECKERLIB)

set(SOURCES
    TyperConsumer.cpp
    NodeTyper.cpp
)
set(HEADERS
    NodeTyper.h
    TyperConsumer.h
    TyperVisitor.h
)

add_library(checkerlib STATIC ${SOURCES} ${HEADERS})
set_target_properties(checkerlib PROPERTIES 
    COMPILE_FLAGS "-fno-rtti -fvisibility-inlines-hidden -fPIC"
    PREFIX ""  # Call it "checkerlib", not "libcheckerlib".
)
