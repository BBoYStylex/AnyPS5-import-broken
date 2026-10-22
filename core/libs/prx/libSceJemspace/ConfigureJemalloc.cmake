get_filename_component(toolDirectory "${AUTOCONF}" DIRECTORY)
if(WIN32)
    set(ENV{PATH} "${toolDirectory};$ENV{PATH}")
    execute_process(COMMAND "${CC}" -dumpmachine OUTPUT_VARIABLE compilerHost
        OUTPUT_STRIP_TRAILING_WHITESPACE COMMAND_ERROR_IS_FATAL ANY)
    if(NOT compilerHost MATCHES "mingw")
        message(FATAL_ERROR "Jemspace requires a MinGW compiler on Windows")
    endif()
    set(configureHost "--host=${compilerHost}")
else()
    set(ENV{PATH} "${toolDirectory}:$ENV{PATH}")
endif()
if(EXISTS "${BINARY}/Makefile")
    execute_process(COMMAND "${MAKE}" clean WORKING_DIRECTORY "${BINARY}" COMMAND_ERROR_IS_FATAL ANY)
endif()
execute_process(
    COMMAND "${BASH}" "${AUTOCONF}" -o "${BINARY}/configure"
    WORKING_DIRECTORY "${SOURCE}"
    COMMAND_ERROR_IS_FATAL ANY
)
execute_process(
    COMMAND "${CMAKE_COMMAND}" -E env "CC=${CC}" "CFLAGS=${CFLAGS}"
        "CPPFLAGS=-DJEMALLOC_MAPS_COALESCE= -DJEMALLOC_PURGE_MADVISE_FREE=" ac_cv_func_sbrk=no
        "${BASH}" "${BINARY}/configure"
        "--srcdir=${SOURCE}"
        ${configureHost}
        --with-version=5.2.0-0-gb0b3e49a54ec29e32636f4577d9d5a896d67fd20
        --with-jemalloc-prefix=anyps5_jemspace_
        --with-private-namespace=anyps5_jemspace_private_
        --with-lg-page=14
        --with-lg-quantum=4
        --disable-cxx
        --disable-shared
        --enable-static
        --disable-initial-exec-tls
        --disable-cache-oblivious
        --disable-fill
        --disable-zone-allocator
        --without-export
        --with-malloc-conf=narenas:1,tcache:false,dirty_decay_ms:0,muzzy_decay_ms:0,retain:false
    WORKING_DIRECTORY "${BINARY}"
    COMMAND_ERROR_IS_FATAL ANY
)
