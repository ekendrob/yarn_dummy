cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project (pin_capture)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# download CPM.cmake
file(
  DOWNLOAD
  https://github.com/cpm-cmake/CPM.cmake/releases/download/v0.40.2/CPM.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/cmake/CPM.cmake
  #EXPECTED_HASH SHA256=cc155ce02e7945e7b8967ddfaff0b050e958a723ef7aad3766d368940cb15494
)
include(build/cmake/CPM.cmake)

CPMAddPackage(
  NAME gtest
  GITHUB_REPOSITORY google/googletest
  GIT_TAG v1.15.2
  VERSION 1.15.2
  OPTIONS
      "INSTALL_GTEST OFF"
      "gtest_force_shared_crt ON"
)
enable_testing()
include(GoogleTest)

set(SYSTEMC_INCLUDE_DIR $ENV{SYSTEMC_HOME}/include)
set(SYSTEMC_LIBRARY_DIR $ENV{SYSTEMC_HOME}/lib)

add_library(pin_capture models/pin_capture/pin_capture.cc)
target_include_directories(pin_capture PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(pin_capture PRIVATE ${CMAKE_SOURCE_DIR}/libs/trusster/teal/systemc)
target_include_directories(pin_capture PRIVATE ${CMAKE_SOURCE_DIR}/libs/trusster/truss/cpp/inc/)
target_include_directories(pin_capture PRIVATE ${SYSTEMC_INCLUDE_DIR})
