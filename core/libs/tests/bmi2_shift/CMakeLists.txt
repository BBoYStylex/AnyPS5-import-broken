cmake_minimum_required(VERSION 3.20)

if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    project(Bmi2ShiftTests LANGUAGES CXX)
    enable_testing()
    set(ANYPS5_TEST_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/tests")
endif()

if(WIN32)
    add_executable(bmi2_shift_tests ../Bmi2Shift.cpp)
    target_include_directories(bmi2_shift_tests PRIVATE ../..)
    target_compile_features(bmi2_shift_tests PRIVATE cxx_std_20)
    set_target_properties(bmi2_shift_tests PROPERTIES
        CXX_EXTENSIONS OFF
        RUNTIME_OUTPUT_DIRECTORY "$<1:${ANYPS5_TEST_OUTPUT_DIRECTORY}>")
    add_test(NAME bmi2_shift COMMAND bmi2_shift_tests)
    set_tests_properties(bmi2_shift PROPERTIES TIMEOUT 20)
endif()
