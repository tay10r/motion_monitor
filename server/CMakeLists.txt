cmake_minimum_required(VERSION 3.14.7)

project(sentinel_server)

option(FAKE_VIDEO_DEVICE "Whether or not to fake the video device API." OFF)
option(ENABLE_TESTING "Whether or not to build the tests." OFF)
option(ENABLE_AUDIO "Whether or not to include audio support." ON)

set(BUNDLE_PATH "" CACHE PATH "The path to the UI files that get bundled with the server.")

set(BUILD_STATIC_LIBS TRUE CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS FALSE CACHE BOOL "" FORCE)

if(NOT TARGET sentinel::proto)
  add_subdirectory(../proto proto)
endif()

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

FetchContent_Declare(llhttp URL "${CMAKE_CURRENT_SOURCE_DIR}/../deps/llhttp.zip")
FetchContent_GetProperties(llhttp)

if(NOT llhttp_POPULATED)

  FetchContent_Populate(llhttp)

  add_library(llhttp
    ${llhttp_SOURCE_DIR}/include/llhttp.h
    ${llhttp_SOURCE_DIR}/src/api.c
    ${llhttp_SOURCE_DIR}/src/http.c
    ${llhttp_SOURCE_DIR}/src/llhttp.c)

  target_include_directories(llhttp
    PUBLIC
      "${llhttp_SOURCE_DIR}/include")

  add_library(llhttp::llhttp ALIAS llhttp)

endif()

find_package(spdlog CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(libuv CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(OpenCV REQUIRED)

add_subdirectory(../deps deps)

set(sources
  src/detector.h
  src/detector.cpp
  src/http_server.h
  src/http_server.cpp
  src/server.h
  src/server.cpp
  src/image.h
  src/image.cpp
  src/config.h
  src/config.cpp
  src/clock.h
  src/pipeline.h
  src/pipeline_runner.h
  src/pipeline_runner.cpp
  src/video_device.h
  src/video_pipeline.h
  src/video_pipeline.cpp
  src/camera_pipeline.h
  src/camera_pipeline.cpp)

if(ENABLE_AUDIO)
  list(APPEND sources
    src/microphone_device.h
    src/microphone_device.cpp
    src/microphone_pipeline.h
    src/microphone_pipeline.cpp)
endif()

set(video_device src/fake_video_device.cpp)

if(NOT FAKE_VIDEO_DEVICE)
  list(APPEND sources src/video_device.cpp)
else()
  list(APPEND sources
    src/video_device.cpp)
endif()

add_library(sentinel_server ${sources})

if(FAKE_VIDEO_DEVICE)
  target_compile_definitions(sentinel_server PUBLIC FAKE_VIDEO_DEVICE=1)
endif()

target_link_libraries(sentinel_server
  PUBLIC
    ${OpenCV_LIBS}
    $<IF:$<TARGET_EXISTS:libuv::uv_a>,libuv::uv_a,libuv::uv>
    spdlog::spdlog
    glm::glm
    sentinel::proto
    llhttp::llhttp
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp)

target_include_directories(sentinel_server
  PUBLIC
    ${OpenCV_INCLUDE_DIRS})

if(ENABLE_AUDIO)
  find_package(ALSA REQUIRED)
  target_link_libraries(sentinel_server PUBLIC ALSA::ALSA)
endif()

add_library(sentinel::server ALIAS sentinel_server)

add_executable(sentinel_server_cli
  main.cpp)

if(BUNDLE_PATH)

  target_compile_definitions(sentinel_server_cli PUBLIC WITH_BUNDLE=1)

  include(CMakeRC.cmake)

  cmrc_add_resource_library(sentinel_server_resources
    "${BUNDLE_PATH}/dashboard.js"
    "${BUNDLE_PATH}/dashboard.wasm"
    "${BUNDLE_PATH}/index.html"
    WHENCE "${BUNDLE_PATH}/")

  target_link_libraries(sentinel_server_cli PUBLIC sentinel_server_resources)

endif()

target_link_libraries(sentinel_server_cli PUBLIC sentinel::server)

set_target_properties(sentinel_server_cli
  PROPERTIES
    OUTPUT_NAME sentinel)

install(TARGETS sentinel_server_cli sentinel_server
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)

set(CPACK_PACKAGE_CONTACT "Taylor Holberton")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6")
include(CPack)

if(ENABLE_TESTING)
  find_package(GTest CONFIG REQUIRED)
  add_executable(sentinel_server_tests
    tests/test_config_validation.cpp
    tests/test_pipeline_runner.cpp)
  target_link_libraries(sentinel_server_tests PUBLIC sentinel_server GTest::gtest GTest::gtest_main)
  enable_testing()
endif()
