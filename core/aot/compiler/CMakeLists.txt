add_library(aot_compiler STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ElfBinaryParser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ElfRelocationResolver.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/LlvmLifter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/X86LifterContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SyscallRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/SyscallReplacementPass.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AotCompilerDriver.cpp
)

target_include_directories(aot_compiler PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(aot_compiler PUBLIC cxx_std_20)

add_executable(aot_compiler_cli
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
)

target_link_libraries(aot_compiler_cli PRIVATE
    aot_compiler
)

target_compile_features(aot_compiler_cli PUBLIC cxx_std_20)
