cmake_minimum_required(VERSION 3.14.7)

project(motion_monitor_client)

option(MOTION_MONITOR_DEMO "Whether or not to build the demo app." ON)

if(NOT TARGET motion_monitor_proto)
  add_subdirectory(../proto proto)
endif()

add_library(motion_monitor_client
  include/motion_monitor.h
  src/connection.cpp)

target_include_directories(motion_monitor_client
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(motion_monitor_client
  PUBLIC
    uv
    motion_monitor_proto)

if(MOTION_MONITOR_DEMO)

  if(NOT TARGET stb)
    add_subdirectory(../deps deps)
  endif()

  include(FetchContent)

  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()

  function(add_demo name)

    set(target motion_monitor_demo_${name})

    add_executable(${target} ${ARGN})

    target_link_libraries(${target} PUBLIC motion_monitor_client)

    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name})

  endfunction()

  add_demo(minimal demo/minimal/main.cpp)

  add_demo(capture demo/capture/main.cpp)
  target_link_libraries(motion_monitor_demo_capture PUBLIC stb)

endif()
