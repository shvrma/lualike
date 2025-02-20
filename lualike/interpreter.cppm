module;

#include <cstdint>
#include <exception>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>

export module lualike.interpreter;

export import lualike.lexer;

export namespace lualike::interpreter {

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
      decltype(std::declval<lexer::Lexer<InputT>>().ReadTokens());
  TokensRangeT tokens_r_;
  std::ranges::iterator_t<TokensRangeT> iter_;
  std::ranges::sentinel_t<TokensRangeT> sentinel_;

  value::LualikeValue ReadAtom();
  value::LualikeValue ReadExpression(int min_precedence);

 public:
  explicit Interpreter(InputT&& input) noexcept;

  value::LualikeValue EvaluateExpression();
};

}  // namespace lualike::interpreter

namespace lualike::interpreter {

namespace lexer = lualike::lexer;
using lexer::Lexer;

namespace token = lualike::token;
using token::kBinOpsPrecedences;
using token::Token;
using token::TokenKind;

namespace value = lualike::value;
using value::LualikeValue;

template <typename InputT>
  requires lexer::InputTRequirements<InputT>
Interpreter<InputT>::Interpreter(InputT&& input) noexcept
    : tokens_r_(Lexer(std::move(input)).ReadTokens()),
      iter_(tokens_r_.begin()) {}

template <typename InputRangeT>
  requires lexer::InputTRequirements<InputRangeT>
LualikeValue Interpreter<InputRangeT>::ReadAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kEOF);
  }

  return [this]() -> LualikeValue {
    auto token = *Interpreter::iter_;

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

      auto inner_value = Interpreter::ReadExpression(1);

      token = *Interpreter::iter_;
      if (token.token_kind != TokenKind::kOtherRightParenthesis) {
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

    if (token.token_kind == TokenKind::kOtherPlus) {
      result = result + rhs;
    }

    else if (token.token_kind == TokenKind::kOtherMinus) {
      result = result - rhs;
    }

    else if (token.token_kind == TokenKind::kOtherAsterisk) {
      result = result * rhs;
    }

    else if (token.token_kind == TokenKind::kOtherSlash) {
      result = result / rhs;
    }

    else {
      throw InterpreterErr(InterpreterErrKind::kInvalidToken);
    }

    // Interpreter::iter_++;
  }

  return result;
}

template <typename InputRangeT>
  requires lexer::InputTRequirements<InputRangeT>
LualikeValue Interpreter<InputRangeT>::EvaluateExpression() {
  return Interpreter::ReadExpression(1);
}

}  // namespace lualike::interpreter
