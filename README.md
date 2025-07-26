# lualike

An interpreter with a syntax resembling Lua's.

There is a lot still to do.

Not a serious product, as I haven't taken any Compiler Theory class.

Besides the learning purpose, the main reason for this is to provide a much lighter alternative (for example, without the coroutines support) with a clean and modern C++ API.

## Prerequisites

- A C++23 compiler with modules support (Clang 16+, MSVC 19+, GCC 14+)
- CMake 3.28+ with Ninja as build system (or another one with modules enabled)

Ninja is required as it is one of the supporting tools for the C++ modules. You should also specify an appropriate CMake generator that targets Ninja (for example, with *-G* flag and *Ninja* generator - *-G Ninja*).

## Usage

`lualike::Interpret` function is the entry point that accepts a *std::ranges::view* of char's as an input (that is also to be a *std::ranges::random_access_range*).

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
import lualike;

#include <iostream>
#include <string_view>

int main() {
  using namespace std::literals;
  const auto result = lualike::Interpret("return 1 + 2 * 3"sv);

  if (!result) {
    std::cerr << "Error: " << result.error().what() << "\n";
    return 1;
  }

  std::cout << "Result: " << result->value().ToString() << "\n"; // prints "7"
}

```

```sh
cmake -B build -G Ninja
```

## Literature used

- <https://compilers.iecc.com/crenshaw/>
- <https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing>
- Modern Compiler Implementation in C by Andrew W. Appel (ISBN 0 521 58390 X)
