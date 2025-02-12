cmake_minimum_required(VERSION 3.11)
project(lualike)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(lualike)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt
    ON
    CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(${CMAKE_PROJECT_NAME}_test lualike/lexer_test.cc
                                          lualike/interpreter_test.cc)
target_link_libraries(${CMAKE_PROJECT_NAME}_test ${CMAKE_PROJECT_NAME}
                      GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(${CMAKE_PROJECT_NAME}_test)
