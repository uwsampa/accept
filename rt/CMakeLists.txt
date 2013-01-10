set( output enercrt.bc )
set( source enercrt.c )
set( clang ${CMAKE_CURRENT_SOURCE_DIR}/../build/built/bin/clang )

# Explicitly execute clang with an -emit-llvm flag to produce a bytecode file
# (for linking with other bytecode files).
set_source_files_properties( ${output} PROPERTIES GENERATED true )
add_custom_command ( OUTPUT ${output}
    COMMAND ${clang} -g -O0 -c -emit-llvm -o ${output} ${CMAKE_CURRENT_SOURCE_DIR}/${source} )
add_custom_target( enerc_rt ALL
    DEPENDS ${output} )
