function(add_sce_opus_library target)
    set(opusDir ${CMAKE_CURRENT_FUNCTION_LIST_DIR})
    add_library(${target} SHARED EXCLUDE_FROM_ALL
            ${CMAKE_CURRENT_SOURCE_DIR}/Export.cpp
            ${opusDir}/src/Decoder.cpp
    )
    target_include_directories(${target} PRIVATE ${LIBS_INCLUDE_DIR})
    target_link_libraries(${target} PRIVATE ffmpeg)
    configure_windows_unwind(${target})
endfunction()
