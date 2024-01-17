cmake_minimum_required(VERSION 3.14.7)

project(motion_monitor)

option(FAKE_VIDEO_DEVICE "Whether or not to fake the video device API." OFF)

if(NOT TARGET motion_monitor_proto)
  add_subdirectory(../proto proto)
endif()

find_package(spdlog REQUIRED)

add_subdirectory(deps)

if(NOT FAKE_VIDEO_DEVICE)
  find_package(OpenCV REQUIRED)
endif()

add_library(motion_monitor
  src/video_device.h
  src/video_device.cpp
  src/motion_filter.h
  src/motion_filter.cpp
  src/server.h
  src/server.cpp
  src/image.h
  src/image.cpp)

if(FAKE_VIDEO_DEVICE)
  target_compile_definitions(motion_monitor PUBLIC FAKE_VIDEO_DEVICE=1)
endif()

target_link_libraries(motion_monitor
  PUBLIC
    ${OpenCV_LIBS}
    uv
    spdlog::spdlog
    motion_monitor_proto)

target_include_directories(motion_monitor
  PUBLIC
    ${OpenCV_INCLUDE_DIRS})

add_executable(motion_monitor_cli
  main.cpp)

target_link_libraries(motion_monitor_cli PUBLIC motion_monitor)

set_target_properties(motion_monitor_cli
  PROPERTIES
    OUTPUT_NAME motion_monitor)

install(TARGETS motion_monitor_cli motion_monitor
  RUNTIME DESTINATION bin
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib)
