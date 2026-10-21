add_library(aot_runtime STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MemoryBridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/OsSubsystemStubs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/RuntimeRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HostLinkerBridge.cpp
)

target_include_directories(aot_runtime PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(aot_runtime PUBLIC cxx_std_20)
