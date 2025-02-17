#ifndef LUALIKE_INTERPRETER_H_
#define LUALIKE_INTERPRETER_H_

#include <ranges>
#include <unordered_map>
#include <utility>

#include "lexer.h"
#include "value.h"

namespace lualike::interpreter {

enum class InterpreterErrKind : uint8_t { kEOF, kUnknownName, kInvalidToken };

struct InterpreterErr : std::exception {
  InterpreterErrKind error_kind;

  explicit InterpreterErr(InterpreterErrKind error_kind) noexcept;
};

template <typename InputT>
  requires lexer::InputTRequirements<InputT>
class Interpreter {
  std::unordered_map<std::string, value::LualikeValue> local_names_;

  using TokensRangeT =
      decltype(std::declval<lexer::Lexer<InputT>>().ReadToken());

  std::ranges::iterator_t<TokensRangeT> iter_;
  std::ranges::sentinel_t<TokensRangeT> sentinel_;

  value::LualikeValue ReadAtom();
  value::LualikeValue ReadExpression(int min_precedence);

 public:
  explicit Interpreter(InputT &&input) noexcept;

  value::LualikeValue EvaluateExpression();
};

}  // namespace lualike::interpreter

#endif  // LUALIKE_INTERPRETER_H_
