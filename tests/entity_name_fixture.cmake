file(MAKE_DIRECTORY "${FIXTURE_DIR}")
file(COPY "${NEUTRAL_DIR}/RealityFactory.ini" "${NEUTRAL_DIR}/install"
    "${NEUTRAL_DIR}/bitmaps" "${NEUTRAL_DIR}/named-audio.bsp" DESTINATION "${FIXTURE_DIR}")
file(APPEND "${FIXTURE_DIR}/RealityFactory.ini" "StartLevel=named-audio.bsp\n")
execute_process(COMMAND "${XVFB}" -a -s "-screen 0 640x480x24"
    "${RUNNER}" --boot-stage=4 "--config-dir=${FIXTURE_DIR}"
    RESULT_VARIABLE status OUTPUT_VARIABLE out ERROR_VARIABLE err TIMEOUT 25)
set(log "${out}\n${err}")
file(WRITE "${FIXTURE_DIR}/entity-name.log" "${log}")
# This fixture proves the real audio constructor/registry and teardown path;
# the later, explicit missing-player failure is expected, NOT stage-4 success.
if(NOT status EQUAL 1 OR NOT log MATCHES "entity-schema: AudioSource3D" OR
   NOT log MATCHES "named audio constructed, registry matched, destructor returned" OR
   NOT log MATCHES "PlayerSetup" OR NOT log MATCHES "RF native exit: 1" OR
   log MATCHES "ERROR: (AddressSanitizer|LeakSanitizer)|runtime error:")
    message(FATAL_ERROR "Named audio lifetime failed (${status}): ${log}")
endif()
