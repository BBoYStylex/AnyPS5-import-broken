add_library(libSceXml SHARED EXCLUDE_FROM_ALL PendingExports.cpp Export.cpp)
target_include_directories(libSceXml PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(libSceXml)

if(BUILD_TESTING)
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(Python3_Interpreter_FOUND)
        add_test(NAME xml_export_nids COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_CURRENT_SOURCE_DIR}/tests/test_export_nids.py
            ${CMAKE_CURRENT_SOURCE_DIR}
            ${PATCHED_DIR}/libSceXml.prx)
        set_tests_properties(xml_export_nids PROPERTIES TIMEOUT 20)
    endif()
endif()
