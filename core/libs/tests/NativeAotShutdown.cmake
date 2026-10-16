execute_process(COMMAND "${PROGRAM}" RESULT_VARIABLE status OUTPUT_VARIABLE output ERROR_VARIABLE error)
if(NOT status STREQUAL "0" OR NOT error MATCHES "native bundle shutdown completed")
    message(FATAL_ERROR "Native bundle shutdown failed (${status}): ${output}${error}")
endif()
