cmake_minimum_required(VERSION 3.28)
project(lualike)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

FetchContent_Declare(
  frozen
  GIT_REPOSITORY https://github.com/serge-sans-paille/frozen.git
  GIT_TAG 1.2.0)
FetchContent_MakeAvailable(frozen)

add_library(lualike lualike/value.cc lualike/token.cc lualike/lexer.cc
                    lualike/interpreter.cc)
target_link_libraries(lualike frozen)
target_sources(
  lualike
  PUBLIC FILE_SET
         lualike_modules
         TYPE
         CXX_MODULES
         FILES
         lualike/value.ccm
         lualike/token.ccm
         lualike/lexer.ccm
         lualike/interpreter.ccm)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(lualike_test lualike/lexer_test.cc lualike/interpreter_test.cc
                            lualike/value_test.cc)
target_link_libraries(lualike_test lualike GTest::gtest_main GTest::gmock)

include(GoogleTest)
gtest_discover_tests(lualike_test)
