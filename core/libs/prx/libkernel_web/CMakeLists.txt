<<<<<<< HEAD
add_library(${LIB_NAME} SHARED EXCLUDE_FROM_ALL
        Export.cpp
        PendingExports.cpp)
=======
add_library(${LIB_NAME} SHARED EXCLUDE_FROM_ALL Export.cpp Pending.cpp)
>>>>>>> local/fix/libkernel_web-pending-exports
target_include_directories(${LIB_NAME} PRIVATE ${LIBS_INCLUDE_DIR})
configure_windows_unwind(${LIB_NAME})
