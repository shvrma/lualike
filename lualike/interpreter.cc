#include "interpreter.h"

#include <iterator>

#include "lexer.h"
#include "token.h"
#include "value.h"

namespace interpreter = lualike::interpreter;
using interpreter::Interpreter;

namespace token = lualike::token;
using token::kBinOpsPrecedences;
using token::Token;
using token::TokenKind;

namespace lexer = lualike::lexer;
using lexer::Lexer;

namespace value = lualike::value;
using value::LualikeValue;

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

template <typename InputT>
  requires lexer::InputTRequirements<InputT>
Interpreter<InputT>::Interpreter(InputT&& input) noexcept
    : iter_(input.begin()), sentinel_(input.end()) {}

template <typename InputRangeT>
  requires lexer::InputTRequirements<InputRangeT>
LualikeValue Interpreter<InputRangeT>::ReadAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kEOF);
  }
  auto token = *Interpreter::iter_;

  auto atom_value = [&token, this]() -> LualikeValue {
    if (token.token_kind == TokenKind::kName) {
      if (const auto find_result = Interpreter::local_names_.find(
              token.token_data.value().ToString());
          find_result != Interpreter::local_names_.end()) {
        return find_result->second;
      }

      throw InterpreterErr(InterpreterErrKind::kUnknownName);
    }

    if (token.token_data) {
      return token.token_data.value();
    }

    if (token.token_kind == TokenKind::kOtherLeftParenthesis) {
      Interpreter::iter_++;

      auto inner_value = Interpreter::ReadExpression(1);

      if (Interpreter::iter_->token_kind != TokenKind::kOtherRightParenthesis) {
        throw InterpreterErr(InterpreterErrKind::kInvalidToken);
      }

      return inner_value;
    }

    throw InterpreterErr(InterpreterErrKind::kInvalidToken);
  }();
  Interpreter::iter_++;

  return atom_value;
}

template <typename InputRangeT>
  requires lexer::InputTRequirements<InputRangeT>
LualikeValue Interpreter<InputRangeT>::ReadExpression(int min_precedence) {
  LualikeValue result = Interpreter::ReadAtom();

  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto token = *Interpreter::iter_;

    if (!IsBinop(token)) {
      break;
    }

    const int precedence = kBinOpsPrecedences.find(token.token_kind)->second;
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
