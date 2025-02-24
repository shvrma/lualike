module;

#include <cstdint>
#include <exception>
#include <generator>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>

export module lualike.interpreter;

import lualike.lexer;

namespace lualike::interpreter {

namespace lexer = lualike::lexer;
using lexer::Lexer;

namespace token = lualike::token;
using token::kBinOpsPrecedences;
using token::Token;
using token::TokenKind;

namespace value = lualike::value;
using value::LualikeValue;

export enum class InterpreterErrKind : uint8_t {
  kEOF,
  kUnknownName,
  kInvalidToken
};

export struct InterpreterErr : std::exception {
  InterpreterErrKind error_kind;

  explicit InterpreterErr(InterpreterErrKind error_kind) noexcept;
};

export template <typename InputT>
  requires lexer::InputTRequirements<InputT>
class Interpreter {
  std::unordered_map<std::string, LualikeValue> local_names_;

  using TokensRangeT = std::generator<const Token&>;
  std::ranges::const_iterator_t<TokensRangeT> iter_;
  std::ranges::const_sentinel_t<TokensRangeT> sentinel_;

  explicit Interpreter(TokensRangeT&& tokens_range);

  LualikeValue ReadAtom();
  LualikeValue ReadExpression(int min_precedence);

 public:
  explicit Interpreter(InputT&& input);

  LualikeValue EvaluateExpression();
};

InterpreterErr::InterpreterErr(InterpreterErrKind error_kind) noexcept
    : error_kind(error_kind) {}

template <typename InputT>
  requires lexer::InputTRequirements<InputT>
Interpreter<InputT>::Interpreter(TokensRangeT&& tokens_range)
    : iter_(std::ranges::cbegin(tokens_range)),
      sentinel_(std::ranges::cend(tokens_range)) {}

template <typename InputT>
  requires lexer::InputTRequirements<InputT>
Interpreter<InputT>::Interpreter(InputT&& input)
    : Interpreter(Lexer<InputT>::ReadTokens(std::move(input))) {}

template <typename InputRangeT>
  requires lexer::InputTRequirements<InputRangeT>
LualikeValue Interpreter<InputRangeT>::ReadAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kEOF);
  }

  return [this]() -> LualikeValue {
    const auto token = *Interpreter::iter_;

    if (token.token_kind == TokenKind::kName) {
      if (const auto find_result = Interpreter::local_names_.find(
              token.token_data.value().ToString());
          find_result != Interpreter::local_names_.end()) {
        Interpreter::iter_++;
        return find_result->second;
      }

      throw InterpreterErr(InterpreterErrKind::kUnknownName);
    }

    if (token.token_data) {
      Interpreter::iter_++;
      return token.token_data.value();
    }

    if (token.token_kind == TokenKind::kOtherLeftParenthesis) {
      Interpreter::iter_++;

      const LualikeValue inner_value = Interpreter::ReadExpression(1);

      if ((*Interpreter::iter_).token_kind !=
          TokenKind::kOtherRightParenthesis) {
        throw InterpreterErr(InterpreterErrKind::kInvalidToken);
      }

      Interpreter::iter_++;
      return inner_value;
    }

    throw InterpreterErr(InterpreterErrKind::kInvalidToken);
  }();
}

template <typename InputRangeT>
  requires lexer::InputTRequirements<InputRangeT>
LualikeValue Interpreter<InputRangeT>::ReadExpression(int min_precedence) {
  LualikeValue result = Interpreter::ReadAtom();

  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto token = *Interpreter::iter_;

    const auto find_result = kBinOpsPrecedences.find(token.token_kind);
    if (find_result == kBinOpsPrecedences.end()) {
      break;
    }

    const int precedence = find_result->second;
    if (precedence < min_precedence) {
      break;
    }

    Interpreter::iter_++;
    LualikeValue rhs = Interpreter::ReadAtom();

    switch (token.token_kind) {
      case TokenKind::kOtherPlus:
        result = result + rhs;
        break;

      case TokenKind::kOtherMinus:
        result = result - rhs;
        break;

      case TokenKind::kOtherAsterisk:
        result = result * rhs;
        break;

      case TokenKind::kOtherSlash:
        result = result / rhs;
        break;

      default:
        throw InterpreterErr(InterpreterErrKind::kInvalidToken);
    }

    Interpreter::iter_++;
  }

  return result;
}

template <typename InputRangeT>
  requires lexer::InputTRequirements<InputRangeT>
LualikeValue Interpreter<InputRangeT>::EvaluateExpression() {
  return Interpreter::ReadExpression(1);
}

}  // namespace lualike::interpreter
