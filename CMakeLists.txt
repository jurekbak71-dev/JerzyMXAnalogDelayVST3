cmake_minimum_required(VERSION 3.25)
project(JerzyMXAnalogDelay2 VERSION 2.1.2.0 DESCRIPTION "Jerzy MX Analog Delay 2 VST3")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(JERZY_BUILD_PLUGIN "Build the native VST3 and editor (OFF for portable DSP/state tests)" ON)

if(JERZY_BUILD_PLUGIN)

set(VST3_SDK_ROOT "" CACHE PATH "Path to a recursively cloned Steinberg vst3sdk")
if(VST3_SDK_ROOT AND EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    set(JERZY_VST3_SDK_DIR "${VST3_SDK_ROOT}")
    add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
else()
    include(FetchContent)
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG 3cdf9ca5d1f5b1b21e0a86832aa4abe55607bd96
        GIT_SHALLOW TRUE
        GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_MakeAvailable(vst3sdk)
    set(JERZY_VST3_SDK_DIR "${vst3sdk_SOURCE_DIR}")
endif()

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(JerzyMXAnalogDelay2 PACKAGE_NAME "MX Delay 2" SOURCES_LIST
    source/mxdelay_version.h source/mxdelay_ids.h source/mxdelay_params.h source/mxdelay_mechanics.h source/mxdelay_state.h
    source/engines/reel/TapeEngine.cpp source/mxdelay_dsp.h source/mxdelay_processor.h source/mxdelay_processor.cpp
    source/mxdelay_editor_geometry.h source/mxdelay_editor.h source/mxdelay_editor.cpp
    source/jerzy_vst_gui_kit.h source/jerzy_vst_gui_kit.cpp
    source/jerzy_gui_assets.h source/jerzy_gui_assets.cpp
    source/mxdelay_gui_views.h source/mxdelay_gui_views.cpp
    source/mxdelay_controller.h source/mxdelay_controller.cpp
    source/mxdelay_entry.cpp resource/mxdelay.uidesc
)
if(UNIX AND NOT APPLE)
    # VSTGUI references the SDK factory IID; static archives need a rescan on ELF.
    target_link_libraries(JerzyMXAnalogDelay2 PRIVATE "$<LINK_GROUP:RESCAN,sdk,vstgui_support>")
else()
    target_link_libraries(JerzyMXAnalogDelay2 PRIVATE sdk vstgui_support)
endif()
if(WIN32)
    target_link_libraries(JerzyMXAnalogDelay2 PRIVATE comctl32)
endif()
target_compile_features(JerzyMXAnalogDelay2 PUBLIC cxx_std_20)
smtg_target_configure_version_file(JerzyMXAnalogDelay2)
file(GLOB JERZY_GUI3D_RESOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resource/gui3d/*.png")
if(WIN32)
    # Steinberg's resource helper passes an unquoted package path to MSBuild
    # with PACKAGE_NAME "MX Delay 2". Use explicit, VERBATIM post-build
    # commands for paths containing spaces. The Windows VST3 module lives in
    # Contents/x86_64-win, while resources belong in Contents/Resources.
    set(JERZY_VST3_RESOURCE_DIR "$<TARGET_FILE_DIR:JerzyMXAnalogDelay2>/../Resources")
    add_custom_command(TARGET JerzyMXAnalogDelay2 POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${JERZY_VST3_RESOURCE_DIR}"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/resource/mxdelay.uidesc"
            "${JERZY_VST3_RESOURCE_DIR}/mxdelay.uidesc"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${JERZY_VST3_RESOURCE_DIR}/gui3d"
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            ${JERZY_GUI3D_RESOURCES} "${JERZY_VST3_RESOURCE_DIR}/gui3d"
        VERBATIM)
else()
    smtg_target_add_plugin_resources(JerzyMXAnalogDelay2 RESOURCES resource/mxdelay.uidesc)
    smtg_target_add_plugin_resources(JerzyMXAnalogDelay2 OUTPUT_SUBDIRECTORY gui3d RESOURCES ${JERZY_GUI3D_RESOURCES})
endif()
if(SMTG_MAC)
    smtg_target_set_bundle(JerzyMXAnalogDelay2 BUNDLE_IDENTIFIER "pl.jerzyaudio.mxanalogdelay2" COMPANY_NAME "Jerzy Audio")
endif()

endif() # JERZY_BUILD_PLUGIN
include(CTest)
if(BUILD_TESTING)
    add_executable(MXDelayDSPTests tests/mxdelay_test.cpp source/engines/reel/TapeEngine.cpp)
    target_include_directories(MXDelayDSPTests PRIVATE source)
    add_test(NAME MXDelayDSP COMMAND MXDelayDSPTests)
    add_executable(MXReverbTests tests/mxdelay_reverb_test.cpp source/engines/reel/TapeEngine.cpp)
    target_include_directories(MXReverbTests PRIVATE source)
    add_test(NAME MXReverb COMMAND MXReverbTests "${CMAKE_BINARY_DIR}/reverb-results")
    add_executable(MXDelayCompatibilityTests tests/mxdelay_compatibility_test.cpp)
    target_include_directories(MXDelayCompatibilityTests PRIVATE source)
    add_test(NAME MXDelayCompatibility COMMAND MXDelayCompatibilityTests)
    # DSP tests use assert; Release must execute the same checks as Debug.
    if(MSVC)
        target_compile_options(MXDelayDSPTests PRIVATE /UNDEBUG)
    else()
        target_compile_options(MXDelayDSPTests PRIVATE -UNDEBUG)
    endif()
endif()

if(BUILD_TESTING AND WIN32 AND JERZY_BUILD_PLUGIN)
    target_sources(JerzyMXAnalogDelay2 PRIVATE tests/mxdelay_render_probe.cpp)
    add_executable(MXDelayGUITests
        tests/mxdelay_gui_test.cpp
        ${JERZY_VST3_SDK_DIR}/public.sdk/source/vst/hosting/module_win32.cpp)
    target_compile_definitions(MXDelayGUITests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(MXDelayGUITests PRIVATE sdk_hosting user32 ole32 shell32)
    add_dependencies(MXDelayGUITests JerzyMXAnalogDelay2)
    add_test(NAME MXDelayGUI COMMAND MXDelayGUITests $<TARGET_FILE:JerzyMXAnalogDelay2>)
    set_tests_properties(MXDelayGUI PROPERTIES TIMEOUT 90)
endif()

if(BUILD_TESTING AND UNIX AND NOT APPLE AND JERZY_BUILD_PLUGIN)
    find_package(X11 REQUIRED)
    add_executable(MXDelayConsoleLinuxTests tests/mxdelay_console_linux_test.cpp
        ${JERZY_VST3_SDK_DIR}/public.sdk/source/vst/hosting/module_linux.cpp)
    target_include_directories(MXDelayConsoleLinuxTests PRIVATE source ${X11_INCLUDE_DIR})
    target_link_libraries(MXDelayConsoleLinuxTests PRIVATE sdk_hosting ${X11_LIBRARIES})
    add_dependencies(MXDelayConsoleLinuxTests JerzyMXAnalogDelay2)
    add_test(NAME MXDelayConsoleLinux COMMAND MXDelayConsoleLinuxTests $<TARGET_FILE_DIR:JerzyMXAnalogDelay2>/../..)
    set_tests_properties(MXDelayConsoleLinux PROPERTIES TIMEOUT 90)
endif()

if(BUILD_TESTING AND JERZY_BUILD_PLUGIN AND (WIN32 OR (UNIX AND NOT APPLE)))
    if(WIN32)
        set(MX_HOST_PLATFORM win32)
    else()
        set(MX_HOST_PLATFORM linux)
    endif()
    add_executable(MXReverbHostTests tests/mxdelay_reverb_host_test.cpp
        ${JERZY_VST3_SDK_DIR}/public.sdk/source/vst/hosting/module_${MX_HOST_PLATFORM}.cpp
        ${JERZY_VST3_SDK_DIR}/public.sdk/source/common/memorystream.cpp)
    target_include_directories(MXReverbHostTests PRIVATE source)
    target_link_libraries(MXReverbHostTests PRIVATE sdk_hosting)
    add_dependencies(MXReverbHostTests JerzyMXAnalogDelay2)
    if(WIN32)
        add_test(NAME MXReverbHost COMMAND MXReverbHostTests $<TARGET_FILE:JerzyMXAnalogDelay2>)
    else()
        add_test(NAME MXReverbHost COMMAND MXReverbHostTests $<TARGET_FILE_DIR:JerzyMXAnalogDelay2>/../..)
    endif()
endif()
