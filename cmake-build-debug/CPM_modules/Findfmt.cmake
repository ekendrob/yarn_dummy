include("/Users/ekendrob/Projects/yarn/build/cmake/CPM.cmake")
CPMAddPackage("NAME;fmt;GIT_REPOSITORY;https://github.com/fmtlib/fmt.git;GIT_SHALLOW;True;GIT_TAG;9.1.0;OPTIONS;FMT_INSTALL;;ON")
set(fmt_FOUND TRUE)