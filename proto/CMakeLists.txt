cmake_minimum_required(VERSION 3.14.7)

project(sentinel_proto)

add_library(sentinel_proto
  include/sentinel/proto.h
  src/read.cpp
  src/writer.cpp
  src/queue.cpp)

target_include_directories(sentinel_proto
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

target_compile_features(sentinel_proto
  PRIVATE
    cxx_std_17)

add_library(sentinel::proto ALIAS sentinel_proto)
