cmake_minimum_required(VERSION 3.28)
project(lualike)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

cmake_minimum_required(VERSION 3.28)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(lualike)
target_include_directories(lualike PRIVATE frozen/include)
target_sources(
  lualike
  PUBLIC FILE_SET
         lualike_modules
         TYPE
         CXX_MODULES
         FILES
         lualike/value.cppm
         lualike/token.cppm
         lualike/lexer.cppm
         lualike/interpreter.cppm
         lualike/lualike.cppm)

include(FetchContent)

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

add_executable(lualike_test lualike/lexer_test.cc lualike/interpreter_test.cc lualike/value_test.cc)
target_link_libraries(lualike_test GTest::gtest_main gmock lualike)

include(GoogleTest)
gtest_discover_tests(lualike_test)
