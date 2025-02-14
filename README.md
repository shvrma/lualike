# lualike

An interpreter with a syntax resembling Lua's one.

There is a lot still to do. Currently it only evaluates expressions made up of literals and simple arithemtic operator (sum, difference, product, quotient).

## Usage

`lualike::interpreter::Interpreter` is the enter point whose constructor accepts a reference to char stream which forms a expression to be evaluated.

Usage example:

```c++

std::istringstream prog{"23 + 77"};
auto interpreter = Interpreter(prog);

const auto evaluation_result = interpreter.Evaluate();
const auto evaluated_value = std::get<LuaValue>(evaluation_result);

std::cout << evaluated_value.ToString();
// 100

```

## Literature used

- <https://compilers.iecc.com/crenshaw/>
- <https://eli.thegreenplace.net/2012/08/02/parsing-expressions-by-precedence-climbing>
- <https://cs.stanford.edu/people/eroberts/courses/soco/projects/2004-05/automata-theory/basics.html>
