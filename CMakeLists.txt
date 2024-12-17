cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

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

set(SYSTEMC_INCLUDE_DIR $ENV{SYSTEMC_HOME}/include)
set(SYSTEMC_LIBRARY_DIR $ENV{SYSTEMC_HOME}/lib)

project (pin_capture)
cpmaddpackage(
  NAME report
  SOURCE_DIR ${PROJECT_SOURCE_DIR}/libs/scp/report
)

add_library(${PROJECT_NAME} models/pin_capture/pin_capture.cc)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SYSTEMC_INCLUDE_DIR})

target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME} ${SYSTEMC_LIBRARY_DIR}/libsystemc${CMAKE_SHARED_LIBRARY_SUFFIX})
target_link_libraries(${PROJECT_NAME} reporting)


project (pin_capture_gtest)
enable_testing()
add_executable(${PROJECT_NAME} tests/pin_capture/pin_capture_tests.cc models/pin_capture/pin_capture.cc)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR})
target_include_directories(${PROJECT_NAME} PRIVATE ${SYSTEMC_INCLUDE_DIR})
# target_include_directories(${PROJECT_NAME} PRIVATE ${gtest_SOURCE_DIR}/googletest/include)
target_include_directories(${PROJECT_NAME} PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include> $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>)

target_link_libraries(${PROJECT_NAME} gtest)
target_link_libraries(${PROJECT_NAME} ${SYSTEMC_LIBRARY_DIR}/libsystemc${CMAKE_SHARED_LIBRARY_SUFFIX})
target_link_libraries(${PROJECT_NAME} reporting)

include(GoogleTest)
gtest_discover_tests(${PROJECT_NAME})
