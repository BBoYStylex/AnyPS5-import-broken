include(${CMAKE_CURRENT_SOURCE_DIR}/../libSceOpusDec/OpusLibrary.cmake)
add_sce_opus_library(libSceOpusCeltDec)

if(BUILD_TESTING)
    add_test_executable(guest_opus_celt_dec_tests ${LIBS_INCLUDE_DIR}/tests/GuestOpusCeltDec.cpp)
    target_include_directories(guest_opus_celt_dec_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_opus_celt_dec_tests PRIVATE libSceOpusCeltDec libc)
    configure_windows_unwind(guest_opus_celt_dec_tests)
    add_test(NAME guest_opus_celt_dec COMMAND guest_opus_celt_dec_tests)
    if(WIN32)
        set_tests_properties(guest_opus_celt_dec PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceOpusCeltDec>")
    endif()
endif()
