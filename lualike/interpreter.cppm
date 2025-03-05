module;

#include <cstdint>
#include <exception>
#include <generator>
#include <ranges>
#include <unordered_map>

export module lualike.interpreter;

import lualike.lexer;

namespace lexer = lualike::lexer;
namespace token = lualike::token;
namespace value = lualike::value;

export namespace lualike::interpreter {

enum class InterpreterErrKind : uint8_t { kEOF, kUnknownName, kInvalidToken };

struct InterpreterErr : std::exception {
  InterpreterErrKind error_kind;

  explicit InterpreterErr(InterpreterErrKind error_kind) noexcept;
};

class Interpreter {
  std::unordered_map<std::string, value::LualikeValue> local_names_;

  using TokensRangeT = std::generator<const token::Token&>;
  std::ranges::const_iterator_t<TokensRangeT> iter_;
  std::ranges::const_sentinel_t<TokensRangeT> sentinel_;

  explicit Interpreter(TokensRangeT&& tokens_range)
      : iter_(std::ranges::cbegin(tokens_range)),
        sentinel_(std::ranges::cend(tokens_range)) {}

  value::LualikeValue ReadAtom();
  value::LualikeValue ReadExpression(int min_precedence);

 public:
  template <typename InputT>
    requires lexer::InputTRequirements<InputT>
  static value::LualikeValue EvaluateExpression(InputT&& input);
};

const std::unordered_map<token::TokenKind, int> kBinOpsPrecedences = {{
    {token::TokenKind::kKeywordOr, 1},

    {token::TokenKind::kKeywordAnd, 2},

    {token::TokenKind::kOtherLessThan, 3},
    {token::TokenKind::kOtherGreaterThan, 3},
    {token::TokenKind::kOtherLessThanEqual, 3},
    {token::TokenKind::kOtherGreaterThanEqual, 3},
    {token::TokenKind::kOtherTildeEqual, 3},
    {token::TokenKind::kOtherDoubleEqual, 3},

    {token::TokenKind::kOtherPlus, 9},
    {token::TokenKind::kOtherMinus, 9},

    {token::TokenKind::kOtherAsterisk, 10},
    {token::TokenKind::kOtherSlash, 10},
    {token::TokenKind::kOtherDoubleSlash, 10},
    {token::TokenKind::kOtherPercent, 10},

    {token::TokenKind::kOtherCaret, 11},
}};

}  // namespace lualike::interpreter

namespace lualike::interpreter {

InterpreterErr::InterpreterErr(InterpreterErrKind error_kind) noexcept
    : error_kind(error_kind) {}

value::LualikeValue Interpreter::ReadAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kEOF);
  }

  const auto& token = *Interpreter::iter_;
  Interpreter::iter_++;

  switch (token.token_kind) {
    case token::TokenKind::kName: {
      const auto& name = std::get<std::string>(token.token_data);

      if (const auto find_result = Interpreter::local_names_.find(name);
          find_result != Interpreter::local_names_.cend()) {
        return find_result->second;
      }

      throw InterpreterErr(InterpreterErrKind::kUnknownName);
    }

    case token::TokenKind::kLiteral:
      return std::get<value::LualikeValue>(token.token_data);

      // case token::TokenKind::kOtherMinus:
      //   // Unary minus operator
      //   // return -(Interpreter::ReadAtom());
      //   throw InterpreterErr(
      //       InterpreterErrKind::kInvalidToken);  // Until operator- is
      //       implemented

      // case token::TokenKind::kKeywordNot:
      //   // Logical not operator
      //   // return !(Interpreter::ReadAtom());
      //   throw InterpreterErr(
      //       InterpreterErrKind::kInvalidToken);  // Until operator! is
      //       implemented

    case token::TokenKind::kOtherLeftParenthesis: {
      const value::LualikeValue inner_value = Interpreter::ReadExpression(1);

      if (Interpreter::iter_ == Interpreter::sentinel_) {
        throw InterpreterErr(InterpreterErrKind::kEOF);
      }

      if (const auto& token = *Interpreter::iter_;
          token.token_kind != token::TokenKind::kOtherRightParenthesis) {
        throw InterpreterErr(InterpreterErrKind::kInvalidToken);
      }

      Interpreter::iter_++;

      return inner_value;
    }

    default:
      throw InterpreterErr(InterpreterErrKind::kInvalidToken);
  }
}

value::LualikeValue Interpreter::ReadExpression(const int min_precedence) {
  auto result = Interpreter::ReadAtom();

  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto& token = *Interpreter::iter_;

    const auto find_result = kBinOpsPrecedences.find(token.token_kind);
    if (find_result == kBinOpsPrecedences.end()) {
      break;
    }

    const int precedence = find_result->second;
    if (precedence < min_precedence) {
      break;
    }

    int next_min_precendence = [precedence, min_precedence, &token] {
      // Right associative op.
      if (token.token_kind == token::TokenKind::kOtherCaret) {
        return precedence;
      }

      return precedence + 1;
    }();
    const auto rhs = Interpreter::ReadExpression(next_min_precendence);

    switch (token.token_kind) {
      case token::TokenKind::kOtherPlus:
        result = result + rhs;
        break;

      case token::TokenKind::kOtherMinus:
        result = result - rhs;
        break;

      case token::TokenKind::kOtherAsterisk:
        result = result * rhs;
        break;

      case token::TokenKind::kOtherSlash:
        result = result / rhs;
        break;

      default:
        return result;
    }

    Interpreter::iter_++;
  }

  return result;
}

template <typename InputT>
  requires lexer::InputTRequirements<InputT>
value::LualikeValue Interpreter::EvaluateExpression(InputT&& input) {
  auto tokens_range = lexer::ReadTokens(input);
  Interpreter interpreter(std::move(tokens_range));
  return interpreter.ReadExpression(1);
}

}  // namespace lualike::interpreter
