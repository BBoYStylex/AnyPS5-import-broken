cmake_minimum_required(VERSION 3.20)
project(AnyPS5_AOT CXX)
enable_testing()

add_subdirectory(compiler)
add_subdirectory(graphics)
add_subdirectory(runtime)
add_subdirectory(io)
add_subdirectory(tests)
