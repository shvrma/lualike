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

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.16.0
)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(lualike_test lexer_test.cpp interpreter_test.cpp)
target_link_libraries(lualike_test GTest::gtest_main lualike)

include(GoogleTest)
gtest_discover_tests(lualike_test)
