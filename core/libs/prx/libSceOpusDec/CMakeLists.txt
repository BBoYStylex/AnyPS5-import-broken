include(${CMAKE_CURRENT_SOURCE_DIR}/OpusLibrary.cmake)
add_sce_opus_library(libSceOpusDec)

if(BUILD_TESTING)
    add_test_executable(guest_opus_dec_tests ${LIBS_INCLUDE_DIR}/tests/GuestOpusDec.cpp)
    target_include_directories(guest_opus_dec_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_opus_dec_tests PRIVATE libSceOpusDec libc)
    configure_windows_unwind(guest_opus_dec_tests)
    add_test(NAME guest_opus_dec COMMAND guest_opus_dec_tests)
    if(WIN32)
        set_tests_properties(guest_opus_dec PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceOpusDec>")
    endif()
endif()
