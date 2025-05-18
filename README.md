# lualike

An interpreter with a syntax resembling Lua's one.

There is a lot still to do. Currently the functionality includes:

- Expression evaluation
- Variables assignments
- Conditionals

Not a serious product as I haven't take any Compiler Theory class, everything is done by *intuition* on how things work.

## Usage

`lualike::Interpret` function is the entry point that accepts a *std::ranges::view* of char's as an input. By default, template is instantiated for *std::string_view* type only.

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
