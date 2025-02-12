#ifndef LUALIKE_INTERPRETER_H_
#define LUALIKE_INTERPRETER_H_

#include <iterator>
#include <unordered_map>

#include "token.h"
#include "value.h"

namespace lualike::interpreter {

enum class InterpreterErr : uint8_t { kEOF, kUnknownName, kInvalidToken };

struct ReadExpressionInterpreterState {};

struct ReadPrefixExpressionInterpreterState {};

struct ReturnLuaValueInterpreterState {
  value::LuaValue output;
};

struct ReturnErrorInterpreterState {
  InterpreterErr error;
};

using InterpreterState =
    std::variant<ReadExpressionInterpreterState,
                 ReadPrefixExpressionInterpreterState,
                 ReturnLuaValueInterpreterState, ReturnErrorInterpreterState>;

using EvaluateResult = std::variant<value::LuaValue, InterpreterErr>;

class Interpreter {
  std::unordered_map<std::string, value::LuaValue> local_names_;

  std::istream_iterator<token::Token> iter_;
  std::istream_iterator<token::Token> sentinel_;

  // inline EvaluateResult ReadExpression() noexcept;
  // inline EvaluateResult ReadPrefixExpression() noexcept;

  EvaluateResult ReadAtom() noexcept;
  EvaluateResult ReadExpression(int min_precedence) noexcept;

 public:
  explicit Interpreter(std::istream &input_stream) noexcept;

  EvaluateResult Evaluate() noexcept;
};

}  // namespace lualike::interpreter

#endif  // LUALIKE_INTERPRETER_H_
