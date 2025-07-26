# lualike

An interpreter with a syntax resembling Lua's.

There is a lot still to do.

Not a serious product, as I haven't taken any Compiler Theory class.

The main reason for this, besides the learning purpose, is to provide a much lighter alternative (for example, without the coroutines support) with a clean and modern C++ API.

## Prerequisites

- A C++23 compiler with module support (Clang 16+, MSVC 19+, GCC 14+)
- CMake 3.28+ with Ninja (or another generator with modules enabled)

## Usage

`lualike::Interpret` function is the entry point that accepts a *std::ranges::view* of char's as an input (that is also to be a *std::ranges::random_access_range*).

Usage example:

```c++
#include <string_view>
#include <iostream>

import lualike.interpreter;

int main() {
  using namespace std::literals;
  const auto result = lualike::Interpret("return 1 + 2 * 3"sv);

  if (!result) {
    std::cerr << "Error: " << result.error().what() << "\n";
    return 1;
  }
  if (auto val = result->value(); val.has_value()) {
    std::cout << "Result: " << val->ToString() << "\n"; // prints "7"
  }
}

```

## Literature used

- <https://compilers.iecc.com/crenshaw/>
- <https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing>
- Modern Compiler Implementation in C by Andrew W. Appel (ISBN 0 521 58390 X)
