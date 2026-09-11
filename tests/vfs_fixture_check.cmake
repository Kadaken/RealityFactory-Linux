execute_process(COMMAND "${RUNNER}" RESULT_VARIABLE status
    OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 20)
if(NOT status EQUAL 0 OR NOT err MATCHES "encrypted CF00 VFS archives are unsupported" OR
   NOT err MATCHES "RF archive fixture: PASS" OR
   err MATCHES "ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:")
    message(FATAL_ERROR "Archive contract failed (${status}): ${out}${err}")
endif()
