file(MAKE_DIRECTORY "${FIXTURE_DIR}")
file(COPY "${NEUTRAL_DIR}/" DESTINATION "${FIXTURE_DIR}")
# Both overloads: the root config uses FILE*, camera.ini uses geVFile*.
file(RENAME "${FIXTURE_DIR}/RealityFactory.ini" "${FIXTURE_DIR}/RealityFACTORY.INI")
file(RENAME "${FIXTURE_DIR}/install/camera.ini" "${FIXTURE_DIR}/install/Camera.INI")
if(AMBIGUOUS)
    file(WRITE "${FIXTURE_DIR}/install/CAMERA.ini" "[General]\nfieldofview=2.0\n")
endif()
execute_process(COMMAND "${XVFB}" -a -s "-screen 0 640x480x24"
    "${RUNNER}" --boot-stage=2 WORKING_DIRECTORY "${FIXTURE_DIR}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 25)
set(log "${out}\n${err}")
file(WRITE "${FIXTURE_DIR}/case-boot.log" "${log}")
if(log MATCHES "ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:")
    message(FATAL_ERROR "Case boot sanitizer failure: ${log}")
endif()
if(AMBIGUOUS)
    if(NOT status EQUAL 1 OR NOT log MATCHES "ambiguous component" OR
       NOT log MATCHES "RF fatal: host teardown complete")
        message(FATAL_ERROR "Ambiguous camera was not refused cleanly (${status}): ${log}")
    endif()
    if(SANITIZED AND NOT log MATCHES "RF fatal: explicit LSan check=0")
        message(FATAL_ERROR "No explicit LSan proof: ${log}")
    endif()
else()
    if(NOT status EQUAL 0 OR NOT log MATCHES "case-fallback: route-4095" OR
       NOT log MATCHES "case-fallback: route-9" OR
       NOT log MATCHES "RF boot stage 2: full InitializeCommon returned")
        message(FATAL_ERROR "Wrong-case boot failed (${status}): ${log}")
    endif()
endif()
