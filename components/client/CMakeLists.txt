cmake_minimum_required(VERSION 3.14.7)

project(sentinel_client)

option(SENTINEL_CLIENT_DEMO "Whether or not to build the demo app." ON)

find_package(libuv CONFIG REQUIRED)

if(NOT TARGET sentinel::proto)
  add_subdirectory(../proto proto)
endif()

add_library(sentinel_client
  include/sentinel/client.h
  src/connection.cpp)

target_include_directories(sentinel_client
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_link_libraries(sentinel_client
  PUBLIC
    $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    sentinel::proto)

add_library(sentinel::client ALIAS sentinel_client)

if(SENTINEL_CLIENT_DEMO)

  if(NOT TARGET stb)
    add_subdirectory(../deps deps)
  endif()

  include(FetchContent)

  if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
  endif()

  function(add_demo name)

    set(target sentinel_demo_${name})

    add_executable(${target} ${ARGN})

    target_link_libraries(${target} PUBLIC sentinel::client)

    set_target_properties(${target} PROPERTIES OUTPUT_NAME ${name})

  endfunction()

  add_demo(minimal demo/minimal/main.cpp)

  add_demo(capture_audio
    demo/capture_audio/main.cpp
    demo/capture_audio/tinywav.h
    demo/capture_audio/tinywav.c)

  add_demo(capture demo/capture/main.cpp)

  target_link_libraries(sentinel_demo_capture PUBLIC stb)

endif()
