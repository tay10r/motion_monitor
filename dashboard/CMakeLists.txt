cmake_minimum_required(VERSION 3.14.7)

project(sentinel_dashboard)

if(NOT TARGET sentinel::proto)
  add_subdirectory(../proto proto)
endif()

include(FetchContent)

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

FetchContent_Declare(uikit URL "https://github.com/tay10r/uikit/archive/main.zip")
FetchContent_MakeAvailable(uikit)

FetchContent_Declare(json URL "https://github.com/nlohmann/json/archive/refs/tags/v3.11.3.zip")
FetchContent_MakeAvailable(json)

set(sources
  main.cpp
  fetch_request.h
  config.h
  config.cpp
  dashboard.h
  dashboard.cpp
  telemetry_stream.h
  widget.h
  chart_widget.h
  chart_widget.cpp
  camera_widget.h
  camera_widget.cpp
  microphone_widget.h
  microphone_widget.cpp)

if(EMSCRIPTEN)
  list(APPEND sources
    fetch_request_emscripten.cpp)
else()
  list(APPEND sources
    fetch_request_fs.cpp)
endif()

add_executable(sentinel_dashboard WIN32 ${sources})

target_compile_features(sentinel_dashboard PUBLIC cxx_std_17)

set_target_properties(sentinel_dashboard
  PROPERTIES
    OUTPUT_NAME dashboard)

if(EMSCRIPTEN)
  target_link_options(sentinel_dashboard
    PUBLIC
      "SHELL: -s FETCH=1")
endif()

target_link_libraries(sentinel_dashboard
  PUBLIC
    sentinel_proto
    uikit::uikit
    uikit::main
    nlohmann_json::nlohmann_json)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/index.html.in"
  "${CMAKE_CURRENT_BINARY_DIR}/index.html"
  @ONLY)

