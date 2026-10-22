add_library(libSceNpCommon SHARED EXCLUDE_FROM_ALL PendingExports.cpp)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/Export.cpp")
    target_sources(libSceNpCommon PRIVATE Export.cpp)
endif()
target_include_directories(libSceNpCommon PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(libSceNpCommon)

if(BUILD_TESTING)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME np_common_export_nids COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_export_nids.py
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PATCHED_DIR}/libSceNpCommon.prx)
        set_tests_properties(np_common_export_nids PROPERTIES TIMEOUT 20)
    endif()
endif()
