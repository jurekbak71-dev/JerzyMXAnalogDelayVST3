cmake_minimum_required(VERSION 3.25)
project(JerzyMXAnalogDelay VERSION 0.5.0.0 DESCRIPTION "Jerzy MX Analog Delay VST3")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VST3_SDK_ROOT "" CACHE PATH "Path to a recursively cloned Steinberg vst3sdk")
if(VST3_SDK_ROOT AND EXISTS "${VST3_SDK_ROOT}/CMakeLists.txt")
    set(JERZY_VST3_SDK_DIR "${VST3_SDK_ROOT}")
    add_subdirectory("${VST3_SDK_ROOT}" "${CMAKE_BINARY_DIR}/vst3sdk")
else()
    include(FetchContent)
    FetchContent_Declare(vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_TAG master
        GIT_SHALLOW TRUE
        GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_MakeAvailable(vst3sdk)
    set(JERZY_VST3_SDK_DIR "${vst3sdk_SOURCE_DIR}")
endif()

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(JerzyMXAnalogDelay
    source/mxdelay_version.h source/mxdelay_ids.h source/mxdelay_params.h source/mxdelay_mechanics.h source/mxdelay_state.h
    source/mxdelay_dsp.h source/mxdelay_processor.h source/mxdelay_processor.cpp
    source/mxdelay_editor_geometry.h source/mxdelay_editor.h source/mxdelay_editor.cpp
    source/mxdelay_gui_views.h source/mxdelay_gui_views.cpp
    source/mxdelay_controller.h source/mxdelay_controller.cpp
    source/mxdelay_entry.cpp resource/mxdelay.uidesc
)
target_link_libraries(JerzyMXAnalogDelay PRIVATE sdk vstgui_support)
if(WIN32)
    target_link_libraries(JerzyMXAnalogDelay PRIVATE comctl32)
endif()
target_compile_features(JerzyMXAnalogDelay PUBLIC cxx_std_17)
smtg_target_configure_version_file(JerzyMXAnalogDelay)
smtg_target_add_plugin_resources(JerzyMXAnalogDelay RESOURCES resource/mxdelay.uidesc)
if(SMTG_MAC)
    smtg_target_set_bundle(JerzyMXAnalogDelay BUNDLE_IDENTIFIER "pl.jerzyaudio.mxanalogdelay" COMPANY_NAME "Jerzy Audio")
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(MXDelayDSPTests tests/mxdelay_test.cpp)
    target_include_directories(MXDelayDSPTests PRIVATE source)
    add_test(NAME MXDelayDSP COMMAND MXDelayDSPTests)
endif()

if(BUILD_TESTING AND WIN32)
    target_sources(JerzyMXAnalogDelay PRIVATE tests/mxdelay_render_probe.cpp)
    add_executable(MXDelayGUITests
        tests/mxdelay_gui_test.cpp
        ${JERZY_VST3_SDK_DIR}/public.sdk/source/vst/hosting/module_win32.cpp)
    target_compile_definitions(MXDelayGUITests PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)
    target_link_libraries(MXDelayGUITests PRIVATE sdk_hosting user32 ole32 shell32)
    add_dependencies(MXDelayGUITests JerzyMXAnalogDelay)
    add_test(NAME MXDelayGUI COMMAND MXDelayGUITests $<TARGET_FILE:JerzyMXAnalogDelay>)
    set_tests_properties(MXDelayGUI PROPERTIES TIMEOUT 90)
endif()
