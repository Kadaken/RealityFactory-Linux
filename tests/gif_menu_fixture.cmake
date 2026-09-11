# Separate generated animated profile retains the static-menu regression too.
file(MAKE_DIRECTORY "${FIXTURE_DIR}")
file(COPY "${NEUTRAL_DIR}/RealityFactory.ini" "${NEUTRAL_DIR}/install"
    "${NEUTRAL_DIR}/bitmaps" "${NEUTRAL_DIR}/video" DESTINATION "${FIXTURE_DIR}")
file(APPEND "${FIXTURE_DIR}/install/menu.ini"
    "animation=0 neutral.gif\nmenutitle=0 0 16 16 31 31 0 0 0\n")
execute_process(COMMAND "${XVFB}" -a -s "-screen 0 640x480x24"
    "${RUNNER}" "--boot-stage=${STAGE}" "--config-dir=${FIXTURE_DIR}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 25)
set(log "${out}\n${err}")
file(WRITE "${FIXTURE_DIR}/gif-menu.log" "${log}")
if(NOT status EQUAL 0 OR NOT log MATCHES "RF native exit: 0" OR
   log MATCHES "ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:")
    message(FATAL_ERROR "Animated GIF fixture failed (${status}): ${log}")
endif()
if(STAGE EQUAL 2 AND NOT log MATCHES "RF GIF fixture: Active and non-null NextFrame confirmed")
    message(FATAL_ERROR "GIF constructor control not reached: ${log}")
endif()
if(STAGE EQUAL 2 AND NOT log MATCHES "RF GIF fixture: descriptor 1x1 at 0,0; LE16 0x1234 confirmed")
    message(FATAL_ERROR "GIF descriptor/byte-order checks not reached: ${log}")
endif()
if(STAGE EQUAL 3 AND NOT log MATCHES "menu frames=1, quit=1")
    message(FATAL_ERROR "Animated title menu did not render and quit: ${log}")
endif()
