# Natives Companion-Modul für Cult of the Lamb (01002E7016C46000) und Host-Tests.
# Aufbau nach dsmod_add_module in eden-duo-companions/native/modules/CMakeLists.txt.
cmake_minimum_required(VERSION 3.20)
project(cotl_companion C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(EDEN_SOURCE_ROOT "$ENV{HOME}/cotl-deps/eden-duo" CACHE PATH "Eden Duo v1.3.2.2 checkout (SDK headers)")
set(COMPANIONS_ROOT "$ENV{HOME}/cotl-deps/eden-duo-companions" CACHE PATH "eden-duo-companions checkout")
foreach(header dsmod_module_abi.h dsmod_module_extensions.h modules/dsmod_module_sdk.h)
    if(NOT EXISTS "${EDEN_SOURCE_ROOT}/src/core/mods/${header}")
        message(FATAL_ERROR "SDK header ${header} not found under EDEN_SOURCE_ROOT=${EDEN_SOURCE_ROOT}")
    endif()
endforeach()

if(ANDROID AND NOT ANDROID_STL STREQUAL "c++_static")
    message(FATAL_ERROR "Android builds require -DANDROID_STL=c++_static")
endif()

set(COTL_TITLE 01002E7016C46000)

# Unity-Leser aus eden-duo-companions (Chained Echoes) mit BC- und LZ4-Decoder
set(LZ4_LIB_DIR "$ENV{HOME}/cotl-deps/lz4/lib" CACHE PATH "lz4 v1.10.0 lib/ (lz4.c, lz4.h)")
set(CE_DIR "${COMPANIONS_ROOT}/native/modules")
add_library(ce_unity STATIC
    "${CE_DIR}/chained_echoes_unity.cpp"
    "${EDEN_SOURCE_ROOT}/externals/bc_decoder/bc_decoder.cpp"
    "${LZ4_LIB_DIR}/lz4.c")
target_include_directories(ce_unity PUBLIC "${CE_DIR}" "${EDEN_SOURCE_ROOT}/src"
    PRIVATE "${LZ4_LIB_DIR}" "${EDEN_SOURCE_ROOT}/externals/bc_decoder")
set_source_files_properties("${EDEN_SOURCE_ROOT}/externals/bc_decoder/bc_decoder.cpp"
    "${CE_DIR}/chained_echoes_unity.cpp" PROPERTIES COMPILE_OPTIONS -w) # Fremdcode
set_target_properties(ce_unity PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN ON)

add_library(cotl_core STATIC
    native/guest.cpp
    native/snapshot.cpp
    native/publish.cpp
    native/i2.cpp
    native/terms.cpp
    native/problems.cpp
    native/quests.cpp
    native/icons.cpp
    native/dungeon_map.cpp
    native/equipment.cpp
    native/font.cpp
    native/spine_atlas.cpp
)
target_include_directories(cotl_core PUBLIC native "${EDEN_SOURCE_ROOT}/src")
target_link_libraries(cotl_core PUBLIC ce_unity)
set_target_properties(cotl_core PROPERTIES CXX_VISIBILITY_PRESET hidden)

add_library(dsmod-cotl SHARED native/module.cpp)
target_link_libraries(dsmod-cotl PRIVATE cotl_core)
set(exports_file "${CMAKE_CURRENT_BINARY_DIR}/dsmod-cotl.exports")
file(WRITE "${exports_file}" "{ global: eden_dsmod_get_module; eden_dsmod_get_extensions; eden_dsmod_get_font_extensions; local: *; };\n")
set_target_properties(dsmod-cotl PROPERTIES
    PREFIX ""
    OUTPUT_NAME ${COTL_TITLE}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
target_link_options(dsmod-cotl PRIVATE "-Wl,--version-script=${exports_file}" "-Wl,--no-undefined")
if(ANDROID)
    target_link_options(dsmod-cotl PRIVATE "-Wl,--exclude-libs,ALL")
endif()

option(BUILD_TESTING "Build host tests" ON)
if(BUILD_TESTING AND NOT ANDROID)
    enable_testing()
    add_executable(cotl-tests
        tests/main.cpp
        tests/guest_test.cpp
        tests/module_test.cpp
        tests/snapshot_test.cpp
        tests/publish_test.cpp
        tests/i2_test.cpp
        tests/terms_test.cpp
        tests/problems_test.cpp
        tests/quests_test.cpp
        tests/icons_test.cpp
        tests/dungeon_map_test.cpp
        tests/font_test.cpp
        tests/spine_atlas_test.cpp
        native/module.cpp
    )
    target_include_directories(cotl-tests PRIVATE tests)
    target_link_libraries(cotl-tests PRIVATE cotl_core)
    add_test(NAME cotl-tests COMMAND cotl-tests)
endif()
