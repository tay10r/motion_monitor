cmake_minimum_required(VERSION 3.14.7)

add_library(motion_monitor_proto
  include/motion_monitor_proto.h
  src/read.cpp
  src/writer.cpp)

target_include_directories(motion_monitor_proto
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
