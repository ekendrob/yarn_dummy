if(EXISTS "/Users/ekendrob/Projects/yarn/cmake-build-debug/pin_capture_gtest[1]_tests.cmake")
  include("/Users/ekendrob/Projects/yarn/cmake-build-debug/pin_capture_gtest[1]_tests.cmake")
else()
  add_test(pin_capture_gtest_NOT_BUILT pin_capture_gtest_NOT_BUILT)
endif()
