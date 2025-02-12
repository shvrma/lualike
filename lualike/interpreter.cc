#include "interpreter.h"

#include <istream>
#include <variant>

#include "token.h"
#include "value.h"

namespace lualike::interpreter {

namespace {

using token::kBinOperatorPrecedenceLevelMap;
using token::Token;
using token::TokenKind;

using value::LuaValue;

bool IsExpressionBeginning(const Token& token) noexcept {
  switch (token.token_kind) {
    case TokenKind::kKeywordNil:
    case TokenKind::kNumericConstant:
    case TokenKind::kName:
    case TokenKind::kKeywordTrue:
    case TokenKind::kKeywordFalse:
      return true;

    default:
      return false;
  }
}

bool IsBinop(const Token& token) noexcept {
  switch (token.token_kind) {
    case TokenKind::kOtherPlus:
    case TokenKind::kOtherMinus:
    case TokenKind::kOtherSlash:
    case TokenKind::kOtherAsterisk:
      return true;

    default:
      return false;
  }
}

}  // namespace

Interpreter::Interpreter(std::istream& input_stream) noexcept
    : iter_(input_stream) {}

EvaluateResult Interpreter::ReadAtom() noexcept {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    return InterpreterErr::kEOF;
  }
  auto token = *Interpreter::iter_;

  auto atom_value = [&token, this]() -> EvaluateResult {
    if (token.token_kind == TokenKind::kKeywordNil) {
      return LuaValue::MakeLuaNil();
    }

    if (token.token_kind == TokenKind::kNumericConstant) {
      return *token.token_data;
    }

    if (token.token_kind == TokenKind::kName) {
      if (const auto find_result = Interpreter::local_names_.find(
              token.token_data.value().ToString());
          find_result != Interpreter::local_names_.end()) {
        return find_result->second;
      }

      return InterpreterErr::kUnknownName;
    }

    if (token.token_kind == TokenKind::kOtherLeftParenthesis) {
      Interpreter::iter_++;

      auto inner_value = Interpreter::ReadExpression(1);

      if (Interpreter::iter_->token_kind != TokenKind::kOtherRightParenthesis) {
        return InterpreterErr::kInvalidToken;
      }

      return inner_value;
    }

    return InterpreterErr::kInvalidToken;
  }();
  Interpreter::iter_++;

  return atom_value;
}

EvaluateResult Interpreter::ReadExpression(int min_precedence) noexcept {
  LuaValue result{};
  if (const auto read_result = Interpreter::ReadAtom();
      std::holds_alternative<LuaValue>(read_result)) {
    result = std::get<LuaValue>(read_result);
  } else {
    return read_result;
  }

  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto token = *Interpreter::iter_;

    if (!IsBinop(token)) {
      break;
    }

    const int precedence =
        kBinOperatorPrecedenceLevelMap.find(token.token_kind)->second;
    if (precedence < min_precedence) {
      break;
    }

    Interpreter::iter_++;

    LuaValue rhs{};
    if (const auto read_result = Interpreter::ReadExpression(precedence + 1);
        std::holds_alternative<LuaValue>(read_result)) {
      rhs = std::get<LuaValue>(read_result);
    } else {
      return read_result;
    }

    if (token.token_kind == TokenKind::kOtherPlus) {
      result += rhs;
    }

    else if (token.token_kind == TokenKind::kOtherMinus) {
      result -= rhs;
    }

    else if (token.token_kind == TokenKind::kOtherAsterisk) {
      result *= rhs;
    }

    else if (token.token_kind == TokenKind::kOtherSlash) {
      result /= rhs;
    }

    else {
      return InterpreterErr::kInvalidToken;
    }

    // Interpreter::iter_++;
  }

  return result;
}

EvaluateResult Interpreter::Evaluate() noexcept {
  return Interpreter::ReadExpression(1);
}

}  // namespace lualike::interpreter
