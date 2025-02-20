cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lualike)
target_sources(
  lualike
  PUBLIC FILE_SET
         lualike_modules
         TYPE
         CXX_MODULES
         FILES
         token.cppm
         value.cppm
         lexer.cppm
         interpreter.cppm
         lualike.cppm)

add_executable(lualike_test lexer_test.cpp interpreter_test.cpp)
target_link_libraries(lualike_test lualike GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(lualike_test)
