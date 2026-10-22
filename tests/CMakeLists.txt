# ARMOR-RADAR host tests: the hardware-independent core, built with any C++17 compiler.
# Copyright (C) 2026 JuanenRac (Electro Hobby 3D). GPL-3.0-or-later.
cmake_minimum_required(VERSION 3.16)
project(armor_radar_host_tests CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Wpedantic -Werror)

add_executable(test_core test_core.cpp)
add_executable(test_node test_node.cpp)
add_executable(test_release test_release.cpp)
add_executable(test_mqtt_tls test_mqtt_tls.cpp)
add_executable(test_sensors test_sensors.cpp)
add_executable(test_logic test_logic.cpp)
add_executable(test_family test_family.cpp)
add_executable(test_perimeter test_perimeter.cpp)
# the s3-wifi board profile (an ESP32-S3-WROOM-1 N16R8 with no Ethernet): the same core headers compiled for the other board
add_executable(test_board_wifi test_board_wifi.cpp)
target_compile_definitions(test_board_wifi PRIVATE ARMOR_BOARD_S3_WIFI=1)
add_executable(emit_samples emit_samples.cpp)

# rules shared with the other nodes (ARMOR-COMMON/firmware_base/tests)
add_executable(test_base test_base.cpp)
add_executable(test_shared test_shared.cpp)
add_executable(test_control test_control.cpp)
add_executable(test_diag test_diag.cpp)

enable_testing()
add_test(NAME core COMMAND test_core)
add_test(NAME node COMMAND test_node)
add_test(NAME release COMMAND test_release)
add_test(NAME mqtt_tls COMMAND test_mqtt_tls)
add_test(NAME sensors COMMAND test_sensors)
add_test(NAME logic COMMAND test_logic)
add_test(NAME family COMMAND test_family)
add_test(NAME perimeter COMMAND test_perimeter)
add_test(NAME board_wifi COMMAND test_board_wifi)
add_test(NAME base COMMAND test_base)
add_test(NAME shared COMMAND test_shared)
add_test(NAME control COMMAND test_control ${CMAKE_CURRENT_SOURCE_DIR}/../../ARMOR-COMMON/conformance)
add_test(NAME diag COMMAND test_diag)

# the panel's texts: eight languages in every row and a text for every error the firmware can answer with
find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME panel_text COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_panel_text.py)
  # no task keeps a big copy of the settings on its small stack, every task has the stack its job needs, GitHub's long redirects fit the HTTP buffer
  add_test(NAME task_safety COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_task_safety.py)
  # a saved time zone applies at once; the panel can still say that a restart is needed
  add_test(NAME save_paths COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_save_paths.py)
  # the Bluetooth set-up keeps the session of the phone under its lock, and a request or an answer never crosses to another connection
  add_test(NAME ble_session COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_ble_session.py)
  # the routes every node of the family serves, the routes the panel calls, and the stand-in node the panel is tried against
  add_test(NAME routes COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_routes.py)
  # the PC side of the transparent serial bridge, against a node made up for the test
  add_test(NAME bridge_client COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/check_bridge_client.py)
endif()
