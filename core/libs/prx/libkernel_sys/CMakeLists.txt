<<<<<<< HEAD
add_library(${LIB_NAME} SHARED EXCLUDE_FROM_ALL Export.cpp Export.Discovery.cpp
        PendingExports.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../libkernel/PendingExports.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/../libkernel_web/PendingExports.cpp)
=======
add_library(${LIB_NAME} SHARED EXCLUDE_FROM_ALL Pending.cpp Export.cpp Export.Discovery.cpp)
>>>>>>> local/fix/libkernel_sys-pending-exports
target_include_directories(${LIB_NAME} PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(${LIB_NAME})
