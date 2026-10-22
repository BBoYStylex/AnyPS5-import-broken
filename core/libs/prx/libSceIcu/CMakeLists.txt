add_library(libSceIcu SHARED EXCLUDE_FROM_ALL PendingExports.cpp Export.cpp)
target_include_directories(libSceIcu PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(libSceIcu)

if(BUILD_TESTING)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME icu_export_nids COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_export_nids.py
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PATCHED_DIR}/libSceIcu.prx)
        set_tests_properties(icu_export_nids PROPERTIES TIMEOUT 20)
    endif()
endif()
