add_executable(test_aot_compiler
    TestAotCompiler.cpp
)
target_link_libraries(test_aot_compiler PRIVATE aot_compiler)
target_compile_features(test_aot_compiler PUBLIC cxx_std_20)

add_executable(test_aot_runtime
    TestAotRuntime.cpp
)
target_link_libraries(test_aot_runtime PRIVATE aot_runtime)
target_compile_features(test_aot_runtime PUBLIC cxx_std_20)

add_executable(test_aot_graphics
    TestAotGraphics.cpp
)
target_link_libraries(test_aot_graphics PRIVATE aot_graphics)
target_compile_features(test_aot_graphics PUBLIC cxx_std_20)

add_executable(test_aot_io
    TestAotIo.cpp
)
target_link_libraries(test_aot_io PRIVATE aot_io)
target_compile_features(test_aot_io PUBLIC cxx_std_20)
