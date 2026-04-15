# lualike

An interpreter with a syntax resembling Lua's.

There is a lot still to do.

Not a serious product, as I haven't taken any Compiler Theory class.

Besides the learning purpose, the main reason for this is to provide a much lighter alternative
(for example, without the coroutines support) with a clean and modern C++ API.

## Prerequisites

- A C++23 compiler
- CMake 3.28+

If you want to build the tests, make sure `GTest` is available to CMake. With
vcpkg, configure using your vcpkg toolchain file so `find_package(GTest CONFIG
REQUIRED)` can resolve it.

## Usage

`lualike::Interpret` is the main entry point and accepts either a
`std::string_view` or a `std::istream&`. The parser API follows the same
convention via `lualike::parser::Parse`.

Possible CMake configuration:

```cmake
cmake_minimum_required(VERSION 3.28) # Due to the requirement of the library's CMake config.
project(example_consumer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(lualike) # Before, copy the library folder as the sibling of the current CMake config.

add_executable(example_consumer main.cc)
target_link_libraries(example_consumer PRIVATE lualike)
```

The *main.cc* file itself:

```c++
#include <iostream>
#include <string_view>

#include "lualike/lualike.h"

int main() {
  using namespace std::literals;
  const auto result = lualike::Interpret("return 1 + 2 * 3"sv);

  if (!result) {
    std::cerr << "Error: " << result.error().what() << "\n";
    std::cerr << result.error().RenderPretty() << "\n";
    return 1;
  }

  std::cout << "Result: " << result->value().ToString() << "\n"; // prints "7"
}

```

```sh
cmake -B build
cmake --build build
```

## Literature Used

- <https://compilers.iecc.com/crenshaw/>
- <https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing>
- Modern Compiler Implementation in C by Andrew W. Appel (ISBN 0 521 58390 X)
