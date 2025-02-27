module;

#include <cstdint>
#include <exception>
#include <generator>
#include <ranges>
#include <string>
#include <unordered_map>
#include <utility>

export module lualike.interpreter;

export import lualike.lexer;

namespace lualike::interpreter {

namespace lexer = lualike::lexer;
using lexer::InputTRequirements;
using lexer::Lexer;

namespace token = lualike::token;
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

export class Interpreter {
  std::unordered_map<std::string, LualikeValue> local_names_;

  using TokensRangeT = std::generator<Token>;
  std::ranges::const_iterator_t<TokensRangeT> iter_;
  std::ranges::const_sentinel_t<TokensRangeT> sentinel_;

  LualikeValue ReadAtom();
  LualikeValue ReadExpression(int min_precedence);

 public:
  explicit Interpreter(TokensRangeT& tokens_range);

  LualikeValue EvaluateExpression();
};

export const std::unordered_map<TokenKind, int> kBinOpsPrecedences = {{
    {TokenKind::kKeywordOr, 1},

    {TokenKind::kKeywordAnd, 2},

    {TokenKind::kOtherLessThan, 3},
    {TokenKind::kOtherGreaterThan, 3},
    {TokenKind::kOtherLessThanEqual, 3},
    {TokenKind::kOtherGreaterThanEqual, 3},
    {TokenKind::kOtherTildeEqual, 3},
    {TokenKind::kOtherDoubleEqual, 3},

    {TokenKind::kOtherPlus, 9},
    {TokenKind::kOtherMinus, 9},

    {TokenKind::kOtherAsterisk, 10},
    {TokenKind::kOtherSlash, 10},
    {TokenKind::kOtherDoubleSlash, 10},
    {TokenKind::kOtherPercent, 10},

    {TokenKind::kOtherCaret, 11},
}};

InterpreterErr::InterpreterErr(InterpreterErrKind error_kind) noexcept
    : error_kind(error_kind) {}

Interpreter::Interpreter(TokensRangeT& tokens_range)
    : iter_(std::ranges::cbegin(tokens_range)),
      sentinel_(std::ranges::cend(tokens_range)) {}

LualikeValue Interpreter::ReadAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kEOF);
  }

  const Token token = *Interpreter::iter_;
  Interpreter::iter_++;

  if (token.token_kind == TokenKind::kName) {
    if (const auto find_result =
            Interpreter::local_names_.find(token.token_data.value().ToString());
        find_result != Interpreter::local_names_.end()) {
      Interpreter::iter_++;
      return find_result->second;
    }

    throw InterpreterErr(InterpreterErrKind::kUnknownName);
  }

  if (token.token_data) {
    return token.token_data.value();
  }

  // if (token.token_kind == TokenKind::kOtherMinus) {
  //   return -(Interpreter::ReadAtom());
  // }

  // if (token.token_kind == TokenKind::kKeywordNot) {
  //   return !(Interpreter::ReadAtom());
  // }

  if (token.token_kind == TokenKind::kOtherLeftParenthesis) {
    const LualikeValue inner_value = Interpreter::ReadExpression(1);

    if (Interpreter::iter_ == Interpreter::sentinel_) {
      throw InterpreterErr(InterpreterErrKind::kEOF);
    }

    if (auto token = *Interpreter::iter_;
        token.token_kind != TokenKind::kOtherRightParenthesis) {
      throw InterpreterErr(InterpreterErrKind::kInvalidToken);
    }

    Interpreter::iter_++;

    return inner_value;
  }

  throw InterpreterErr(InterpreterErrKind::kInvalidToken);
}

LualikeValue Interpreter::ReadExpression(const int min_precedence) {
  LualikeValue result = Interpreter::ReadAtom();

  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto token = *Interpreter::iter_;
    Interpreter::iter_++;

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
      if (token.token_kind == TokenKind::kOtherCaret) {
        return precedence;
      }

      return precedence + 1;
    }();

    LualikeValue rhs = Interpreter::ReadExpression(next_min_precendence);

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
        return result;
    }
  }

  return result;
}

LualikeValue Interpreter::EvaluateExpression() {
  return Interpreter::ReadExpression(1);
}

}  // namespace lualike::interpreter
