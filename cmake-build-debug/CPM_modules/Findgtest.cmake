include("/Users/ekendrob/Projects/yarn/build/cmake/CPM.cmake")
CPMAddPackage("NAME;gtest;GITHUB_REPOSITORY;google/googletest;GIT_TAG;v1.15.2;VERSION;1.15.2;OPTIONS;INSTALL_GTEST OFF;gtest_force_shared_crt ON")
set(gtest_FOUND TRUE)