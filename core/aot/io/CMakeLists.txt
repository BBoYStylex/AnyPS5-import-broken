add_library(aot_io STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/DirectStorageStreaming.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/FileStreamQueue.cpp
)

target_include_directories(aot_io PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(aot_io PUBLIC cxx_std_20)
