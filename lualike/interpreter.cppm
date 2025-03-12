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

enum class InterpreterErrKind : uint8_t {
  kEOF,
  kUnknownName,
  kExpectedExpressionAtom,
  kUnclosedParanthesis,
  kNormallyImpossibleErr,
};

struct InterpreterErr : std::exception {
  InterpreterErrKind error_kind;

  explicit InterpreterErr(InterpreterErrKind error_kind) noexcept
      : error_kind(error_kind) {}

  const char* what() const noexcept override {
    return "Interpreter error occured!";
  }
};

class Interpreter {
  std::unordered_map<std::string, value::LualikeValue> local_names_;

  std::ranges::const_iterator_t<lexer::TokensRangeT> iter_;
  std::ranges::const_sentinel_t<lexer::TokensRangeT> sentinel_;

  explicit Interpreter(lexer::TokensRangeT&& tokens_range)
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

value::LualikeValue Interpreter::ReadAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kEOF);
  }

  const auto token = *Interpreter::iter_;
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

    case token::TokenKind::kOtherMinus:
      return -(Interpreter::ReadAtom());

    case token::TokenKind::kKeywordNot:
      // Logical not operator
      return !(Interpreter::ReadAtom());

    case token::TokenKind::kOtherLeftParenthesis: {
      const value::LualikeValue inner_value = Interpreter::ReadExpression(1);

      if (Interpreter::iter_ == Interpreter::sentinel_) {
        throw InterpreterErr(InterpreterErrKind::kEOF);
      }

      if (const auto& token = *Interpreter::iter_;
          token.token_kind != token::TokenKind::kOtherRightParenthesis) {
        throw InterpreterErr(InterpreterErrKind::kUnclosedParanthesis);
      }

      Interpreter::iter_++;

      return inner_value;
    }

    default:
      throw InterpreterErr(InterpreterErrKind::kExpectedExpressionAtom);
  }
}

value::LualikeValue Interpreter::ReadExpression(const int min_precedence) {
  auto result = Interpreter::ReadAtom();

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
    // Power operator is the only right-associative.
    const auto rhs = Interpreter::ReadExpression(
        (token.token_kind == token::TokenKind::kOtherCaret) ? precedence
                                                            : precedence + 1);

    switch (token.token_kind) {
      case token::TokenKind::kOtherPlus:
        result += rhs;
        break;

      case token::TokenKind::kOtherMinus:
        result -= rhs;
        break;

      case token::TokenKind::kOtherAsterisk:
        result *= rhs;
        break;

      case token::TokenKind::kOtherSlash:
        result /= rhs;
        break;

      default:
        throw InterpreterErr(InterpreterErrKind::kNormallyImpossibleErr);
    }
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
