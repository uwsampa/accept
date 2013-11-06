set( output enercrt.bc )
set( source enercrt.c )
set( clang ${CMAKE_CURRENT_SOURCE_DIR}/../build/built/bin/clang )
execute_process(COMMAND xcrun --show-sdk-path OUTPUT_VARIABLE XCODESDKPATH OUTPUT_STRIP_TRAILING_WHITESPACE)

# Explicitly execute clang with an -emit-llvm flag to produce a bytecode file
# (for linking with other bytecode files).
set_source_files_properties( ${output} PROPERTIES GENERATED true )
add_custom_command (
    OUTPUT ${output}
    COMMAND ${clang} -g -O0 -I${XCODESDKPATH}/usr/include -c -emit-llvm -o ${output} ${CMAKE_CURRENT_SOURCE_DIR}/${source} 
    DEPENDS ${source}
)
add_custom_target( enerc_rt ALL
    DEPENDS ${output} )
