include(ExternalProject)
enable_language(C)

find_program(JEMSPACE_AUTOCONF NAMES autoconf HINTS "C:/msys64/usr/bin" REQUIRED)
find_program(JEMSPACE_BASH NAMES bash HINTS "C:/msys64/usr/bin" REQUIRED)
find_program(JEMSPACE_MAKE NAMES gmake make mingw32-make HINTS "C:/msys64/usr/bin" REQUIRED)

get_filename_component(jemspaceSource "${CMAKE_CURRENT_LIST_DIR}/../../../../3rdparty/jemalloc" ABSOLUTE)
set(jemspaceBuild "${CMAKE_CURRENT_BINARY_DIR}/jemalloc")
set(jemspaceFlags "${CMAKE_C_FLAGS} -O2 -fvisibility=hidden")
if(NOT WIN32)
    string(APPEND jemspaceFlags " -fPIC")
endif()
if(APPLE)
    foreach(arch IN LISTS CMAKE_OSX_ARCHITECTURES)
        string(APPEND jemspaceFlags " -arch ${arch}")
    endforeach()
endif()
if(ANYPS5_ENABLE_ASAN)
    string(APPEND jemspaceFlags " -fsanitize=address -fno-omit-frame-pointer")
endif()

file(MAKE_DIRECTORY "${jemspaceBuild}/include")
ExternalProject_Add(jemspace_allocator_build
    SOURCE_DIR "${jemspaceSource}"
    BINARY_DIR "${jemspaceBuild}"
    DOWNLOAD_COMMAND ""
    UPDATE_COMMAND ""
    CONFIGURE_COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE=${jemspaceSource}"
        "-DBINARY=${jemspaceBuild}"
        "-DAUTOCONF=${JEMSPACE_AUTOCONF}"
        "-DBASH=${JEMSPACE_BASH}"
        "-DCC=${CMAKE_C_COMPILER}"
        "-DCFLAGS=${jemspaceFlags}"
        "-DMAKE=${JEMSPACE_MAKE}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/ConfigureJemalloc.cmake"
    BUILD_COMMAND "${JEMSPACE_MAKE}" "C_SRCS=$(filter-out $(srcroot)src/pages.c $(srcroot)src/zone.c,$(wildcard $(srcroot)src/*.c))" lib/libjemalloc.a
    BUILD_BYPRODUCTS "${jemspaceBuild}/lib/libjemalloc.a"
    INSTALL_COMMAND ""
    LOG_CONFIGURE ON
    LOG_BUILD ON
    LOG_OUTPUT_ON_FAILURE ON
)
ExternalProject_Add_StepDependencies(jemspace_allocator_build configure
    "${CMAKE_CURRENT_SOURCE_DIR}/ConfigureJemalloc.cmake")
add_library(jemspace_allocator STATIC IMPORTED)
set_target_properties(jemspace_allocator PROPERTIES
    IMPORTED_LOCATION "${jemspaceBuild}/lib/libjemalloc.a"
    INTERFACE_INCLUDE_DIRECTORIES "${jemspaceBuild}/include"
)
add_dependencies(jemspace_allocator jemspace_allocator_build)
find_package(Threads REQUIRED)
target_link_libraries(jemspace_allocator INTERFACE Threads::Threads ${CMAKE_DL_LIBS})
