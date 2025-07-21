# lualike

An interpreter with a syntax resembling Lua's one.

There is a lot still to do.

Not a serious product as I haven't take any Compiler Theory class.

The main reason for this, besides the learning purpouse, is to provide much lighter alternative (for example without the corouutines support) with clean and modern C++ API.

## Usage

`lualike::Interpret` function is the entry point that accepts a *std::ranges::view* of char's as an input (that is also to be a *std::ranges::random_access_range*).

Usage example:

```c++
const auto eval_result = lualike::Interpret(std::string_view{"return 1 + 2 * 3"});

if (eval_result.has_value()) {
    std::println("Program evaluated to a value: {}.", eval_result->value().ToString());
    // Should print: Program evaluated to a value: 7.
}
```

## Literature used

- <https://compilers.iecc.com/crenshaw/>
- <https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing>
- Modern Compiler Implementation in C by Andrew W. Appel (ISBN 0 521 58390 X)
