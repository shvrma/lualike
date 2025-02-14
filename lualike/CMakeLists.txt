cmake_minimum_required(VERSION 3.10)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(lualike lexer.cc token.cc value.cc interpreter.cc)

add_executable(lualike_test lexer_test.cc interpreter_test.cc)
target_link_libraries(lualike_test lualike GTest::gtest_main)

enable_testing()
include(GoogleTest)
gtest_discover_tests(lualike_test)
