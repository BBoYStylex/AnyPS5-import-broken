add_library(libSceFreeType SHARED EXCLUDE_FROM_ALL
        Export.cpp
        src/GuestObjects.cpp
)
target_include_directories(libSceFreeType PRIVATE ${LIBS_INCLUDE_DIR})
target_link_libraries(libSceFreeType PRIVATE freetype)
if(MINGW OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_options(libSceFreeType PRIVATE LINKER:--exclude-libs,ALL)
elseif(APPLE)
    target_link_options(libSceFreeType PRIVATE "LINKER:-exported_symbol,*_nid_*")
endif()
configure_windows_unwind(libSceFreeType)

if(BUILD_TESTING)
    add_test_executable(guest_freetype_tests ${LIBS_INCLUDE_DIR}/tests/GuestFreeType.cpp)
    target_include_directories(guest_freetype_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_freetype_tests PRIVATE libSceFreeType freetype libc)
    configure_windows_unwind(guest_freetype_tests)
    add_test(NAME guest_freetype COMMAND guest_freetype_tests)
    add_test_executable(guest_freetype_charmap_tests ${LIBS_INCLUDE_DIR}/tests/GuestFreeTypeCharmap.cpp)
    target_include_directories(guest_freetype_charmap_tests PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(guest_freetype_charmap_tests PRIVATE libSceFreeType freetype libc)
    configure_windows_unwind(guest_freetype_charmap_tests)
    add_test(NAME guest_freetype_charmap COMMAND guest_freetype_charmap_tests)
    if(APPLE)
        add_test_executable(guest_freetype_imports_tests
                ${LIBS_INCLUDE_DIR}/tests/GuestFreeTypeImports.cpp
                ${LIBS_INCLUDE_DIR}/nid/src/NidCompute.cpp
                ${LIBS_INCLUDE_DIR}/nid/src/Sha1.cpp
        )
        target_include_directories(guest_freetype_imports_tests PRIVATE ${LIBS_INCLUDE_DIR} ${LIBS_INCLUDE_DIR}/nid/include)
        target_link_libraries(guest_freetype_imports_tests PRIVATE freetype)
        add_dependencies(guest_freetype_imports_tests libs)
        add_test(NAME guest_freetype_imports_unpatched COMMAND guest_freetype_imports_tests $<TARGET_FILE:libSceFreeType> unpatched)
        add_test(NAME guest_freetype_imports_patched COMMAND guest_freetype_imports_tests ${PATCHED_DIR}/libSceFreeType.prx patched)
        set_tests_properties(guest_freetype_imports_unpatched guest_freetype_imports_patched PROPERTIES TIMEOUT 20)
    endif()
    if(WIN32)
        set_tests_properties(guest_freetype PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceFreeType>")
        set_tests_properties(guest_freetype_charmap PROPERTIES ENVIRONMENT_MODIFICATION
            "PATH=path_list_prepend:$<TARGET_FILE_DIR:libSceFreeType>")
    endif()
endif()
