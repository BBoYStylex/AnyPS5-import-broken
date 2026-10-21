add_library(aot_graphics STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ConsoleShaderExtractor.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ShaderTranspiler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/VulkanPipelineWrapper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GraphicsDriver.cpp
)

target_include_directories(aot_graphics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/3rdparty/Vulkan-Headers/include
)

target_compile_features(aot_graphics PUBLIC cxx_std_20)

add_executable(aot_graphics_cli
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Main.cpp
)

target_link_libraries(aot_graphics_cli PRIVATE
    aot_graphics
)

target_compile_features(aot_graphics_cli PUBLIC cxx_std_20)
