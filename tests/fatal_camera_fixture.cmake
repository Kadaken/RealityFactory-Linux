file(MAKE_DIRECTORY "${FIXTURE_DIR}/install")
file(WRITE "${FIXTURE_DIR}/RealityFactory.ini" "GameName=Neutral Fatal Fixture\r\nWidth=320\r\nHeight=240\r\nFullScreen=false\r\nUseDirectInput=false\r\n")
execute_process(COMMAND "${XVFB}" -a -s "-screen 0 640x480x24"
    "${RUNNER}" --boot-stage=2 "--config-dir=${FIXTURE_DIR}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 25)
file(WRITE "${FIXTURE_DIR}/fatal-camera.log" "${out}\n${err}")
set(err "${out}\n${err}")
if(NOT status EQUAL 1 OR NOT err MATCHES "RF fatal: stage=2 code=-1 exit=1" OR
   NOT err MATCHES "RF fatal: host teardown complete" OR
   NOT err MATCHES "resolved config dir:" OR
   err MATCHES "ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:")
    message(FATAL_ERROR "Missing-camera teardown failed (${status}): ${err}")
endif()
if(SANITIZED AND NOT err MATCHES "RF fatal: explicit LSan check=0")
    message(FATAL_ERROR "Fatal exit did not prove explicit LSan checking: ${err}")
endif()
