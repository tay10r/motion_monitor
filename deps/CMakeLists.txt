cmake_minimum_required(VERSION 3.14.7)

add_library(stb
  stb_image_write.h
  stb_image_write.c
  stb_image.h
  stb_image.c)

target_include_directories(stb PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
