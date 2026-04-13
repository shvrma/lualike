cmake_minimum_required(VERSION 3.28)
project(lualike)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lualike)
target_sources(
  lualike
  PRIVATE lualike/ast.cc lualike/value.cc
  PUBLIC FILE_SET
         HEADERS
         FILES
         lualike/ast.h
         lualike/interpreter.h
         lualike/lexer.h
         lualike/lualike.h
         lualike/parser.h
         lualike/token.h
         lualike/value.h)
target_include_directories(lualike PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(LUALIKE_BUILD_TESTS "Build lualike tests" ${PROJECT_IS_TOP_LEVEL})

if(LUALIKE_BUILD_TESTS)
  find_package(GTest CONFIG REQUIRED)

  enable_testing()

  add_executable(
    lualike_test lualike/tests/lexer_test.cc lualike/tests/interpreter_test.cc
                 lualike/tests/parser_test.cc lualike/tests/value_test.cc
                 lualike/tests/ast_test.cc)
  target_link_libraries(lualike_test PRIVATE lualike GTest::gmock_main)

  include(GoogleTest)
  gtest_discover_tests(lualike_test)
endif()
