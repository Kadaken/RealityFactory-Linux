cmake_minimum_required(VERSION 3.16)
project(RealityFactoryLinuxClosure LANGUAGES C CXX)
include(CTest)

find_package(PkgConfig REQUIRED)
find_package(OpenGL REQUIRED)
pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
find_library(FREEIMAGE_LIBRARY NAMES freeimage REQUIRED)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Keep the measured inherited graph at C++98. Only the bounded native boot
# controller and its watchdog fixture require the C++11 thread/clock library.
set_source_files_properties(adapters/native_main.cpp tests/boot_watchdog_fixture.cpp
    PROPERTIES COMPILE_OPTIONS "-std=c++11")

get_filename_component(DEFAULT_GENESIS3D_ROOT
    "${CMAKE_CURRENT_SOURCE_DIR}/../Genesis3D_Project" ABSOLUTE)
set(GENESIS3D_ROOT "${DEFAULT_GENESIS3D_ROOT}" CACHE PATH
    "Sibling Genesis3D native Linux source tree")
set(GENESIS3D_LIBRARY
    "${GENESIS3D_ROOT}/build/libgenesis3d_render.so.1" CACHE FILEPATH
    "Native Genesis3D shared library")

# The VS2005 project is the last upstream project graph. It contains 157
# C/C++ units. The older VC6 DSP lists two additional entries: one source that
# is absent from the canonical tree and one generated parser that skParser.cpp
# includes directly. Neither is a separate compilable unit in the actual tree.
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/upstream/RealityFactory.vcproj"
    RF_PROJECT_LINES REGEX "RelativePath=\"[^\"]+\\.(c|cpp)\"")
set(RF_RUNTIME_SOURCES)
foreach(RF_PROJECT_LINE IN LISTS RF_PROJECT_LINES)
    string(REGEX REPLACE ".*RelativePath=\"([^\"]+)\".*" "\\1"
        RF_RELATIVE_SOURCE "${RF_PROJECT_LINE}")
    string(REPLACE "\\" "/" RF_RELATIVE_SOURCE "${RF_RELATIVE_SOURCE}")
    if(RF_RELATIVE_SOURCE STREQUAL "drawbbox.c")
        set(RF_RELATIVE_SOURCE "Drawbbox.c")
    endif()
    list(APPEND RF_RUNTIME_SOURCES
        "${CMAKE_CURRENT_SOURCE_DIR}/upstream/${RF_RELATIVE_SOURCE}")
endforeach()
list(REMOVE_DUPLICATES RF_RUNTIME_SOURCES)
list(LENGTH RF_RUNTIME_SOURCES RF_RUNTIME_SOURCE_COUNT)
if(NOT RF_RUNTIME_SOURCE_COUNT EQUAL 157)
    message(FATAL_ERROR
        "Expected 157 VS2005 runtime units, found ${RF_RUNTIME_SOURCE_COUNT}")
endif()

# Faithful 0.68A subsystem recovery for legacy NonPlayerCharacter maps. Keep
# these sources visibly separate from the unmodified 0.76 project graph.
list(APPEND RF_RUNTIME_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream-0.68a/LegacyNPCUtilities.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream-0.68a/Track.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream-0.68a/CNPCPathPoint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream-0.68a/CNPC.cpp")

file(GLOB_RECURSE RF_PRIVATE_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream/*.H"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream-0.68a/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream-0.68a/*.H")
set(RF_INCLUDE_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/adapters"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream"
    "${CMAKE_CURRENT_SOURCE_DIR}/upstream-0.68a")
foreach(RF_HEADER IN LISTS RF_PRIVATE_HEADERS)
    get_filename_component(RF_HEADER_DIR "${RF_HEADER}" DIRECTORY)
    list(APPEND RF_INCLUDE_DIRS "${RF_HEADER_DIR}")
endforeach()

file(GLOB_RECURSE GENESIS3D_HEADERS CONFIGURE_DEPENDS
    "${GENESIS3D_ROOT}/*.h" "${GENESIS3D_ROOT}/*.H")
foreach(GENESIS3D_HEADER IN LISTS GENESIS3D_HEADERS)
    get_filename_component(GENESIS3D_HEADER_DIR "${GENESIS3D_HEADER}" DIRECTORY)
    list(APPEND RF_INCLUDE_DIRS "${GENESIS3D_HEADER_DIR}")
endforeach()
list(REMOVE_DUPLICATES RF_INCLUDE_DIRS)

# Preserve the measured 157-unit upstream graph and the bounded recovered
# subsystem alongside native adapters.
add_library(rf_compile_closure OBJECT EXCLUDE_FROM_ALL ${RF_RUNTIME_SOURCES})
target_include_directories(rf_compile_closure PRIVATE ${RF_INCLUDE_DIRS})
# Match the engine's build configuration: its RAM API is NDEBUG-sensitive.
target_compile_options(rf_compile_closure PRIVATE
    "$<$<COMPILE_LANGUAGE:C>:-include${CMAKE_CURRENT_SOURCE_DIR}/adapters/rf_platform_compat.h>"
    "$<$<COMPILE_LANGUAGE:CXX>:-include${CMAKE_CURRENT_SOURCE_DIR}/adapters/rf_platform_compat.h>"
)

add_library(rf_joystick_adapter STATIC EXCLUDE_FROM_ALL
    adapters/joystick.cpp)
target_include_directories(rf_joystick_adapter PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/adapters")
target_link_libraries(rf_joystick_adapter PUBLIC PkgConfig::SDL2)

add_library(rf_input_adapter STATIC EXCLUDE_FROM_ALL
    adapters/input_compat.cpp adapters/host_window.cpp)
target_include_directories(rf_input_adapter PUBLIC
    ${RF_INCLUDE_DIRS})
target_link_libraries(rf_input_adapter PUBLIC PkgConfig::SDL2 genesis3d_render dl)

if(BUILD_TESTING)
    add_executable(rf_electric_gauss_test tests/electric_gauss_fixture.cpp)
    target_include_directories(rf_electric_gauss_test PRIVATE adapters)
    add_test(NAME rf_electric_gauss_fixture COMMAND rf_electric_gauss_test)
    add_test(NAME rf_human_launcher_contract
        COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tools/launch_human.sh" --dry-run)
    add_executable(rf_boot_watchdog_test tests/boot_watchdog_fixture.cpp)
    target_include_directories(rf_boot_watchdog_test PRIVATE adapters)
    target_link_libraries(rf_boot_watchdog_test PRIVATE pthread)
    add_test(NAME rf_boot_watchdog_fixture COMMAND rf_boot_watchdog_test)
    set_tests_properties(rf_boot_watchdog_fixture PROPERTIES TIMEOUT 5)
    add_executable(rf_joystick_neutral_test tests/joystick_neutral_fixture.cpp)
    target_link_libraries(rf_joystick_neutral_test PRIVATE rf_joystick_adapter)
    add_test(NAME rf_joystick_neutral_fixture COMMAND rf_joystick_neutral_test)
    set_tests_properties(rf_joystick_neutral_fixture PROPERTIES TIMEOUT 5
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
    add_executable(rf_header_packing_test tests/header_packing_fixture.cpp)
    target_include_directories(rf_header_packing_test PRIVATE ${RF_INCLUDE_DIRS})
    add_test(NAME rf_header_packing_fixture COMMAND rf_header_packing_test)
    add_executable(rf_joystick_zero_test tests/joystick_zero_devices.cpp)
    target_link_libraries(rf_joystick_zero_test PRIVATE rf_joystick_adapter)
    add_test(NAME rf_joystick_zero_devices COMMAND rf_joystick_zero_test)
    set_tests_properties(rf_joystick_zero_devices PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")

    add_executable(rf_texture_trace_test
        tests/texture_trace_fixture.cpp
        adapters/texture_trace.cpp)
    target_include_directories(rf_texture_trace_test PRIVATE ${RF_INCLUDE_DIRS})
    add_test(NAME rf_texture_trace_fixture COMMAND rf_texture_trace_test)

    add_executable(rf_runtime_timer_test
        tests/runtime_timer_fixture.cpp
        adapters/runtime_compat.cpp)
    target_include_directories(rf_runtime_timer_test PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_runtime_timer_test PRIVATE genesis3d_render pthread)
    add_test(NAME rf_runtime_timer_fixture COMMAND rf_runtime_timer_test)

    add_executable(rf_stdio_case_test tests/stdio_case_fixture.cpp adapters/runtime_compat.cpp)
    target_include_directories(rf_stdio_case_test PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_stdio_case_test PRIVATE genesis3d_render pthread)
    add_test(NAME rf_stdio_case_fixture COMMAND rf_stdio_case_test)
    add_executable(rf_vfs_archive_test tests/vfs_archive_fixture.c
        tests/vfs_adapter_bridge.cpp adapters/runtime_compat.cpp)
    target_include_directories(rf_vfs_archive_test PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_vfs_archive_test PRIVATE genesis3d_render pthread)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vfs-fixture")
    add_test(NAME rf_vfs_archive_fixture COMMAND "${CMAKE_COMMAND}"
        "-DRUNNER=$<TARGET_FILE:rf_vfs_archive_test>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/vfs_fixture_check.cmake")
    set_tests_properties(rf_vfs_archive_fixture PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vfs-fixture")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vfs-lines")
    add_test(NAME rf_vfs_line_progress COMMAND rf_vfs_archive_test --line-progress)
    set_tests_properties(rf_vfs_line_progress PROPERTIES TIMEOUT 10
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/vfs-lines")
    add_executable(rf_fatal_boundary_test tests/fatal_boundary_fixture.cpp adapters/runtime_compat.cpp)
    target_include_directories(rf_fatal_boundary_test PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_fatal_boundary_test PRIVATE genesis3d_render pthread)
    set(RF_FATAL_MODES clean reentry)
    set(RF_SANITIZED OFF)
    if(CMAKE_CXX_FLAGS MATCHES "sanitize")
        set(RF_SANITIZED ON)
        list(APPEND RF_FATAL_MODES leak)
    endif()
    foreach(mode IN LISTS RF_FATAL_MODES)
        add_test(NAME rf_fatal_boundary_${mode} COMMAND "${CMAKE_COMMAND}"
            "-DRUNNER=$<TARGET_FILE:rf_fatal_boundary_test>" "-DMODE=${mode}"
            "-DSANITIZED=${RF_SANITIZED}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/fatal_boundary_fixture.cmake")
    endforeach()

    add_executable(rf_autoselect_sdl_test
        tests/autoselect_sdl_fixture.c
        upstream/AutoSelect.c
        adapters/runtime_compat.cpp)
    target_include_directories(rf_autoselect_sdl_test PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_autoselect_sdl_test PRIVATE
        genesis3d_render PkgConfig::SDL2 pthread)
    add_test(NAME rf_autoselect_sdl_fixture COMMAND rf_autoselect_sdl_test)
    set_tests_properties(rf_autoselect_sdl_fixture PROPERTIES
        ENVIRONMENT "SDL_VIDEODRIVER=dummy;SDL_AUDIODRIVER=dummy")
endif()

add_library(genesis3d_render SHARED IMPORTED GLOBAL)
set_target_properties(genesis3d_render PROPERTIES
    IMPORTED_LOCATION "${GENESIS3D_LIBRARY}")

# Every inherited runtime object is linked into the real native entry point.
# --neutral-frame runs a hidden render-only startup; normal visibility is opt-in.
add_executable(realityfactory_linux
    adapters/native_main.cpp
    adapters/runtime_compat.cpp
    adapters/texture_trace.cpp
    adapters/entity_schema.cpp
    $<TARGET_OBJECTS:rf_compile_closure>)
target_include_directories(realityfactory_linux PRIVATE ${RF_INCLUDE_DIRS})
target_link_libraries(realityfactory_linux PRIVATE
    genesis3d_render
    OpenGL::GL
    rf_input_adapter
    rf_joystick_adapter
    PkgConfig::SDL2
    "${FREEIMAGE_LIBRARY}"
    dl
    m
    pthread)

if(BUILD_TESTING)
    add_executable(rf_neutral_assets tests/neutral_boot_assets.c)
    add_executable(rf_track_iterator_test tests/track_iterator_fixture.cpp
        adapters/runtime_compat.cpp adapters/texture_trace.cpp adapters/entity_schema.cpp
        $<TARGET_OBJECTS:rf_compile_closure>)
    target_include_directories(rf_track_iterator_test PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_track_iterator_test PRIVATE genesis3d_render OpenGL::GL
        rf_input_adapter rf_joystick_adapter PkgConfig::SDL2 "${FREEIMAGE_LIBRARY}" dl m pthread)
    add_test(NAME rf_track_iterator_fixture COMMAND rf_track_iterator_test)
    # Reuse the actual runner graph, adding measurement-only clock wrappers.
    get_target_property(RF_RUNNER_SOURCES realityfactory_linux SOURCES)
    get_target_property(RF_RUNNER_LIBRARIES realityfactory_linux LINK_LIBRARIES)
    add_executable(rf_frame_clock_probe EXCLUDE_FROM_ALL ${RF_RUNNER_SOURCES} tests/frame_clock_probe.cpp)
    target_include_directories(rf_frame_clock_probe PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_frame_clock_probe PRIVATE ${RF_RUNNER_LIBRARIES})
    target_link_options(rf_frame_clock_probe PRIVATE -no-pie
        "-Wl,--wrap=_Z20RF_BootGameBeginTickv" "-Wl,--wrap=_Z18RF_BootGameEndTickv"
        "-Wl,--wrap=timeGetTime" "-Wl,--wrap=time" "-Wl,--wrap=clock"
        "-Wl,--wrap=_ZN11CCommonData18FreeRunningCounterEv"
        "-Wl,--wrap=_ZN11CCommonData20FreeRunningCounter_FEv")
    add_executable(rf_round15_probe EXCLUDE_FROM_ALL ${RF_RUNNER_SOURCES}
        tests/round15_runtime_probe.cpp)
    target_include_directories(rf_round15_probe PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_round15_probe PRIVATE ${RF_RUNNER_LIBRARIES})
    set_source_files_properties(tests/round15_runtime_probe.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-access-control;-std=gnu++11")
    target_link_options(rf_round15_probe PRIVATE
        "-Wl,--wrap=_ZN11CCommonData15HandleGameInputEv"
        "-Wl,--wrap=_ZN6CInput13GetFirstInputEv"
        "-Wl,--wrap=_ZN6CInput12GetNextInputEv"
        "-Wl,--wrap=_Z19RF_BootGameEndFramebi"
        "-Wl,--wrap=_Z18RF_BootGameCapturev"
        "-Wl,--wrap=geWorld_SetActorFlags")
    find_package(Python3 REQUIRED COMPONENTS Interpreter)
    add_test(NAME rf_entity_sizes_generated COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_entity_sizes.py" --check)
    add_test(NAME rf_entity_schema_generated COMMAND "${Python3_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools/gen_entity_schema.py" --check)
    add_executable(rf_gif_control adapters/native_main.cpp
        adapters/runtime_compat.cpp adapters/texture_trace.cpp adapters/entity_schema.cpp
        tests/gif_contract_fixture.cpp $<TARGET_OBJECTS:rf_compile_closure>)
    target_include_directories(rf_gif_control PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_gif_control PRIVATE genesis3d_render OpenGL::GL
        rf_input_adapter rf_joystick_adapter PkgConfig::SDL2 "${FREEIMAGE_LIBRARY}" dl m pthread)
    target_link_options(rf_gif_control PRIVATE "-Wl,--wrap=_ZN8CAnimGifC1EPKci"
        "-Wl,--wrap=geVFile_Read")
    # Identical runtime objects, but a test-only linker wrapper forces the
    # stream-success ownership branch. No production adapter is replaced.
    add_executable(rf_menu_stream_control adapters/native_main.cpp
        adapters/runtime_compat.cpp adapters/texture_trace.cpp adapters/entity_schema.cpp
        tests/menu_stream_success_control.cpp $<TARGET_OBJECTS:rf_compile_closure>)
    target_include_directories(rf_menu_stream_control PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_menu_stream_control PRIVATE genesis3d_render OpenGL::GL
        rf_input_adapter rf_joystick_adapter PkgConfig::SDL2 "${FREEIMAGE_LIBRARY}" dl m pthread)
    target_link_options(rf_menu_stream_control PRIVATE "-Wl,--wrap=_ZN14StreamingAudio6CreateEPc")
    add_executable(rf_entity_name_control adapters/native_main.cpp
        adapters/runtime_compat.cpp adapters/texture_trace.cpp adapters/entity_schema.cpp
        tests/entity_name_control.cpp $<TARGET_OBJECTS:rf_compile_closure>)
    target_include_directories(rf_entity_name_control PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_entity_name_control PRIVATE genesis3d_render OpenGL::GL
        rf_input_adapter rf_joystick_adapter PkgConfig::SDL2 "${FREEIMAGE_LIBRARY}" dl m pthread)
    target_link_options(rf_entity_name_control PRIVATE "-Wl,--wrap=_ZN11CCommonData15InitializeLevelEPKc")
    add_executable(rf_attribute_ownership_control adapters/native_main.cpp
        adapters/runtime_compat.cpp adapters/texture_trace.cpp adapters/entity_schema.cpp
        tests/attribute_ownership_fixture.cpp $<TARGET_OBJECTS:rf_compile_closure>)
    target_include_directories(rf_attribute_ownership_control PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_attribute_ownership_control PRIVATE genesis3d_render OpenGL::GL
        rf_input_adapter rf_joystick_adapter PkgConfig::SDL2 "${FREEIMAGE_LIBRARY}" dl m pthread)
    target_link_options(rf_attribute_ownership_control PRIVATE "-Wl,--wrap=RF_RunBootStage")
    add_executable(rf_weapon_sentinel_control adapters/native_main.cpp
        adapters/runtime_compat.cpp adapters/texture_trace.cpp adapters/entity_schema.cpp
        tests/weapon_sentinel_control.cpp $<TARGET_OBJECTS:rf_compile_closure>)
    target_include_directories(rf_weapon_sentinel_control PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_weapon_sentinel_control PRIVATE genesis3d_render OpenGL::GL
        rf_input_adapter rf_joystick_adapter PkgConfig::SDL2 "${FREEIMAGE_LIBRARY}" dl m pthread)
    set_source_files_properties(tests/weapon_sentinel_control.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-access-control")
    target_link_options(rf_weapon_sentinel_control PRIVATE "-Wl,--wrap=RF_RunBootStage"
        "-Wl,--wrap=geWorld_GetEntitySet" "-Wl,--wrap=_ZN13CActorManager9GetMotionEPK7geActor"
        "-Wl,--wrap=_ZN13CActorManager9SetMotionEPK7geActorPKci")
    add_executable(rf_inventory_schema_control adapters/native_main.cpp
        adapters/runtime_compat.cpp adapters/texture_trace.cpp adapters/entity_schema.cpp
        tests/inventory_schema_control.cpp $<TARGET_OBJECTS:rf_compile_closure>)
    target_include_directories(rf_inventory_schema_control PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_inventory_schema_control PRIVATE genesis3d_render OpenGL::GL
        rf_input_adapter rf_joystick_adapter PkgConfig::SDL2 "${FREEIMAGE_LIBRARY}" dl m pthread)
    # White-box paging setup only; production objects and class layout are unchanged.
    set_source_files_properties(tests/inventory_schema_control.cpp PROPERTIES
        COMPILE_OPTIONS "-fno-access-control")
    target_link_options(rf_inventory_schema_control PRIVATE "-Wl,--wrap=RF_RunBootStage"
        "-Wl,--wrap=geEngine_DrawBitmap")
    target_include_directories(rf_neutral_assets PRIVATE ${RF_INCLUDE_DIRS}
        "${GENESIS3D_ROOT}/tests")
    target_link_libraries(rf_neutral_assets PRIVATE genesis3d_render)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot/install")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot/bitmaps/menu")
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot/video/menu")
    add_test(NAME rf_neutral_asset_setup COMMAND rf_neutral_assets)
    set_tests_properties(rf_neutral_asset_setup PROPERTIES
        WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
        FIXTURES_SETUP neutral_assets)
    find_program(XVFB_RUN_EXECUTABLE xvfb-run)
    add_executable(rf_host_input_test tests/host_input_fixture.cpp)
    target_link_libraries(rf_host_input_test PRIVATE rf_input_adapter)
    add_executable(rf_menu_cursor_layer_test tests/menu_cursor_layer_fixture.c)
    target_include_directories(rf_menu_cursor_layer_test PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_menu_cursor_layer_test PRIVATE genesis3d_render
        OpenGL::GL PkgConfig::SDL2)
    add_executable(rf_human_host_test tests/human_host_fixture.cpp tests/human_host_main.cpp)
    target_link_libraries(rf_human_host_test PRIVATE rf_input_adapter)
    add_executable(rf_human_launch_control ${RF_RUNNER_SOURCES}
        tests/human_host_fixture.cpp tests/human_launch_control.cpp)
    target_include_directories(rf_human_launch_control PRIVATE ${RF_INCLUDE_DIRS})
    target_link_libraries(rf_human_launch_control PRIVATE ${RF_RUNNER_LIBRARIES})
    target_link_options(rf_human_launch_control PRIVATE "-Wl,--wrap=_Z7WinMainPvS_Pci")
    foreach(symbol IN ITEMS SDL_CreateWindow SDL_ShowWindow SDL_GetKeyboardFocus SDL_GetMouseFocus
        SDL_SetRelativeMouseMode SDL_GetNumVideoDisplays SDL_GetDisplayBounds SDL_GetGlobalMouseState
        SDL_GetMouseState SDL_ShowCursor)
        target_link_options(rf_human_host_test PRIVATE "-Wl,--wrap=${symbol}")
        target_link_options(rf_human_launch_control PRIVATE "-Wl,--wrap=${symbol}")
    endforeach()
    if(XVFB_RUN_EXECUTABLE)
        foreach(profile IN ITEMS placement hidden menu pointer --human-launch)
            string(REPLACE "--" "" test_name "${profile}")
            set(host_runner rf_human_host_test)
            if(profile STREQUAL "--human-launch")
                set(host_runner rf_human_launch_control)
            endif()
            add_test(NAME rf_human_host_${test_name} COMMAND "${XVFB_RUN_EXECUTABLE}" -a
                -s "-screen 0 640x480x24" $<TARGET_FILE:${host_runner}> ${profile})
            set_tests_properties(rf_human_host_${test_name} PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
                ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;SDL_AUDIODRIVER=dummy")
        endforeach()
        add_test(NAME rf_menu_cursor_layer COMMAND "${XVFB_RUN_EXECUTABLE}" -a
            -s "-screen 0 640x480x24" $<TARGET_FILE:rf_menu_cursor_layer_test>
            "${GENESIS3D_ROOT}/build")
        set_tests_properties(rf_menu_cursor_layer PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
            ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;SDL_AUDIODRIVER=dummy")
        foreach(profile IN ITEMS absent partial valid arrowr arrowrhighlight arrowl arrowlhighlight arrowralpha arrowlalpha)
            add_test(NAME rf_inventory_schema_${profile} COMMAND "${CMAKE_COMMAND}"
                "-DXVFB=${XVFB_RUN_EXECUTABLE}" "-DRUNNER=$<TARGET_FILE:rf_inventory_schema_control>"
                "-DNEUTRAL_DIR=${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
                "-DFIXTURE_DIR=${CMAKE_CURRENT_BINARY_DIR}/inventory-${profile}"
                "-DPROFILE=${profile}" "-DSANITIZED=${RF_SANITIZED}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/inventory_schema_fixture.cmake")
            set_tests_properties(rf_inventory_schema_${profile} PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
                FIXTURES_REQUIRED neutral_assets
                ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        endforeach()
        add_test(NAME rf_weapon_sentinel COMMAND "${XVFB_RUN_EXECUTABLE}" -a
            -s "-screen 0 640x480x24" $<TARGET_FILE:rf_weapon_sentinel_control> --boot-stage=2)
        set_tests_properties(rf_weapon_sentinel PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
            FIXTURES_REQUIRED neutral_assets
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
            ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        add_test(NAME rf_attribute_ownership COMMAND "${XVFB_RUN_EXECUTABLE}" -a
            -s "-screen 0 640x480x24" $<TARGET_FILE:rf_attribute_ownership_control> --boot-stage=2)
        set_tests_properties(rf_attribute_ownership PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
            FIXTURES_REQUIRED neutral_assets
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
            ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        foreach(gif_stage IN ITEMS 2 3)
            set(gif_runner realityfactory_linux)
            if(gif_stage EQUAL 2)
                set(gif_runner rf_gif_control)
            endif()
            add_test(NAME rf_gif_stage_${gif_stage} COMMAND "${CMAKE_COMMAND}"
                "-DXVFB=${XVFB_RUN_EXECUTABLE}" "-DRUNNER=$<TARGET_FILE:${gif_runner}>"
                "-DNEUTRAL_DIR=${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
                "-DFIXTURE_DIR=${CMAKE_CURRENT_BINARY_DIR}/gif-stage-${gif_stage}"
                "-DSTAGE=${gif_stage}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/gif_menu_fixture.cmake")
            set_tests_properties(rf_gif_stage_${gif_stage} PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
                FIXTURES_REQUIRED neutral_assets
                ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        endforeach()
        foreach(profile IN ITEMS stage2 stage3 success3)
            set(stream_stage 3)
            set(stream_control OFF)
            set(stream_runner realityfactory_linux)
            if(profile STREQUAL "stage2")
                set(stream_stage 2)
            elseif(profile STREQUAL "success3")
                set(stream_control ON)
                set(stream_runner rf_menu_stream_control)
            endif()
            add_test(NAME rf_menu_stream_${profile} COMMAND "${CMAKE_COMMAND}"
                "-DXVFB=${XVFB_RUN_EXECUTABLE}" "-DRUNNER=$<TARGET_FILE:${stream_runner}>"
                "-DNEUTRAL_DIR=${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
                "-DFIXTURE_DIR=${CMAKE_CURRENT_BINARY_DIR}/menu-stream-${profile}"
                "-DSTAGE=${stream_stage}" "-DCONTROL=${stream_control}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/menu_stream_fixture.cmake")
            set_tests_properties(rf_menu_stream_${profile} PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
                FIXTURES_REQUIRED neutral_assets
                ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        endforeach()
        foreach(rejected IN ITEMS OFF ON)
            add_test(NAME rf_vfs_boot_${rejected} COMMAND "${CMAKE_COMMAND}"
                "-DXVFB=${XVFB_RUN_EXECUTABLE}" "-DRUNNER=$<TARGET_FILE:realityfactory_linux>"
                "-DNEUTRAL_DIR=${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
                "-DFIXTURE_DIR=${CMAKE_CURRENT_BINARY_DIR}/vfs-boot-${rejected}"
                "-DREJECTED=${rejected}" "-DSANITIZED=${RF_SANITIZED}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/vfs_boot_fixture.cmake")
            set_tests_properties(rf_vfs_boot_${rejected} PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
                FIXTURES_REQUIRED neutral_assets
                ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        endforeach()
        add_test(NAME rf_fatal_camera_fixture COMMAND "${CMAKE_COMMAND}"
            "-DXVFB=${XVFB_RUN_EXECUTABLE}"
            "-DRUNNER=$<TARGET_FILE:realityfactory_linux>"
            "-DFIXTURE_DIR=${CMAKE_CURRENT_BINARY_DIR}/fatal-camera"
            "-DSANITIZED=${RF_SANITIZED}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/fatal_camera_fixture.cmake")
        set_tests_properties(rf_fatal_camera_fixture PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
            ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        foreach(ambiguous IN ITEMS OFF ON)
            add_test(NAME rf_case_boot_${ambiguous} COMMAND "${CMAKE_COMMAND}"
                "-DXVFB=${XVFB_RUN_EXECUTABLE}"
                "-DRUNNER=$<TARGET_FILE:realityfactory_linux>"
                "-DNEUTRAL_DIR=${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
                "-DFIXTURE_DIR=${CMAKE_CURRENT_BINARY_DIR}/case-boot-${ambiguous}"
                "-DAMBIGUOUS=${ambiguous}" "-DSANITIZED=${RF_SANITIZED}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/case_boot_fixture.cmake")
            set_tests_properties(rf_case_boot_${ambiguous} PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
                FIXTURES_REQUIRED neutral_assets
                ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        endforeach()
        add_test(NAME rf_entity_name_lifetime COMMAND "${CMAKE_COMMAND}"
            "-DXVFB=${XVFB_RUN_EXECUTABLE}" "-DRUNNER=$<TARGET_FILE:rf_entity_name_control>"
            "-DNEUTRAL_DIR=${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
            "-DFIXTURE_DIR=${CMAKE_CURRENT_BINARY_DIR}/entity-name"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/entity_name_fixture.cmake")
        set_tests_properties(rf_entity_name_lifetime PROPERTIES TIMEOUT 30 RUN_SERIAL TRUE
            FIXTURES_REQUIRED neutral_assets
            ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        foreach(stage RANGE 1 5)
            add_test(NAME rf_boot_stage_${stage}
                COMMAND "${XVFB_RUN_EXECUTABLE}" -a -s "-screen 0 640x480x24"
                    $<TARGET_FILE:realityfactory_linux> --boot-stage=${stage})
            set_tests_properties(rf_boot_stage_${stage} PROPERTIES
                WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
                FIXTURES_REQUIRED neutral_assets TIMEOUT 30
                RUN_SERIAL TRUE
                ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
        endforeach()
        add_test(NAME rf_host_input_fixture COMMAND "${XVFB_RUN_EXECUTABLE}" -a
            -s "-screen 0 640x480x24" $<TARGET_FILE:rf_host_input_test>)
        set_tests_properties(rf_host_input_fixture PROPERTIES TIMEOUT 30
            ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1")
        add_test(NAME rf_neutral_first_frame
            COMMAND "${XVFB_RUN_EXECUTABLE}" -a -s "-screen 0 640x480x24"
                $<TARGET_FILE:realityfactory_linux> --neutral-frame)
        set_tests_properties(rf_neutral_first_frame PROPERTIES
            WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/neutral-boot"
            FIXTURES_REQUIRED neutral_assets TIMEOUT 30
            ENVIRONMENT "SDL_VIDEODRIVER=x11;LIBGL_ALWAYS_SOFTWARE=1;ALSOFT_DRIVERS=null")
    endif()
endif()

message(STATUS "Reality Factory compile-closure units: ${RF_RUNTIME_SOURCE_COUNT}")
