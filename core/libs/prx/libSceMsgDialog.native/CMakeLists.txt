add_library(libSceMsgDialog.native SHARED EXCLUDE_FROM_ALL Export.cpp)
target_include_directories(libSceMsgDialog.native PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(libSceMsgDialog.native)

if(BUILD_TESTING)
    add_test_executable(guest_msg_dialog_native_tests ${LIBS_INCLUDE_DIR}/tests/services/GuestMsgDialogNative.cpp)
    target_include_directories(guest_msg_dialog_native_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_msg_dialog_native_tests PRIVATE libSceMsgDialog.native)
    add_library_test(services NAME guest_msg_dialog_native COMMAND guest_msg_dialog_native_tests)
    if(WIN32)
        set_tests_properties(guest_msg_dialog_native PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceMsgDialog.native>")
    endif()
endif()
