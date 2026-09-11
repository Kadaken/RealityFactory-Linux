file(MAKE_DIRECTORY "${FIXTURE_DIR}")
file(COPY "${NEUTRAL_DIR}/RealityFactory.ini" "${NEUTRAL_DIR}/install"
    "${NEUTRAL_DIR}/bitmaps" DESTINATION "${FIXTURE_DIR}")
file(APPEND "${FIXTURE_DIR}/install/menu.ini" "music=neutral-stream.wav\n")
execute_process(COMMAND "${XVFB}" -a -s "-screen 0 640x480x24"
    "${RUNNER}" "--boot-stage=${STAGE}" "--config-dir=${FIXTURE_DIR}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 25)
set(log "${out}\n${err}")
file(WRITE "${FIXTURE_DIR}/menu-stream.log" "${log}")
if(NOT status EQUAL 0 OR NOT log MATCHES "RF boot stage ${STAGE}: full InitializeCommon returned" OR
   NOT log MATCHES "RF native exit: 0" OR
   log MATCHES "ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:")
    message(FATAL_ERROR "Menu stream lifetime failed (${status}): ${log}")
endif()
if(STAGE EQUAL 3 AND NOT log MATCHES "menu frames=1, quit=1")
    message(FATAL_ERROR "Menu quit path not completed: ${log}")
endif()
if(CONTROL)
    if(NOT log MATCHES "RF fixture-only: stream Create success control")
        message(FATAL_ERROR "Success-branch control was not reached: ${log}")
    endif()
elseif(log MATCHES "RF fixture-only:")
    message(FATAL_ERROR "Production runner contains a test-only control: ${log}")
endif()
