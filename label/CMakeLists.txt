cmake_minimum_required(VERSION 3.14.7)

project(label)

if(NOT TARGET stb)
  add_subdirectory(../deps deps)
endif()

if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

FetchContent_Declare(uikit URL "https://github.com/tay10r/uikit/archive/refs/tags/v0.1.3.zip")
FetchContent_MakeAvailable(uikit)

add_executable(label
  main.cpp)

target_link_libraries(label PUBLIC uikit::uikit uikit::main stb)
