module;

#include <expected>
#include <generator>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

#include "frozen/unordered_map.h"

module lualike.interpreter;

namespace lualike::interpreter {

struct LualikeFunctionRepr : value::LualikeFunction {
  std::vector<token::Token> body;

  explicit LualikeFunctionRepr(std::vector<std::string>&& func_args,
                               std::vector<token::Token>&& func_body)
      : LualikeFunction(std::move(func_args)), body(std::move(func_body)) {};

  std::optional<value::LualikeValue> Call(
      std::vector<value::LualikeValue> args) override {
    return {};
  };
};

struct LualikePrintFunc : value::LualikeFunction {
  explicit LualikePrintFunc() : LualikeFunction({"to_print"}) {};

  std::optional<value::LualikeValue> Call(
      std::vector<value::LualikeValue> args) override {
    std::cout << args.at(0).ToString() << '\n';

    return std::nullopt;
  };
};

std::unordered_map<std::string, value::LualikeValue> MakeDefaultGlobalEnv() {
  const value::LualikeValue::FuncT print_func =
      std::make_shared<LualikePrintFunc>();

  return {{"print", {print_func}}};
}

constexpr auto kBinOpsPrecedences =
    frozen::make_unordered_map<token::TokenKind, int>({
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
    });

template <typename InputT>
  requires InputTRequirements<InputT>
value::LualikeValue Interpreter<InputT>::ReadExpressionAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kExpectedExpression);
  }

  const auto token = *Interpreter::iter_;
  Interpreter::iter_++;

  switch (token.token_kind) {
    case token::TokenKind::kName: {
      if (Interpreter::read_only) {
        return {};
      }

      const auto& name = std::get<std::string>(token.token_data);

      if (const auto find_result = Interpreter::local_names_.find(name);
          find_result != Interpreter::local_names_.end()) {
        return find_result->second;
      }

      if (const auto find_result = Interpreter::global_names_.find(name);
          find_result != Interpreter::global_names_.end()) {
        return find_result->second;
      }

      throw InterpreterErr(InterpreterErrKind::kUnknownName);
    }

    break;

    case token::TokenKind::kKeywordTrue:
      return {true};

    case token::TokenKind::kKeywordFalse:
      return {false};

    case token::TokenKind::kKeywordNil:
      return {};

    case token::TokenKind::kLiteral:
      if (Interpreter::read_only) {
        return {};
      }

      return std::get<value::LualikeValue>(token.token_data);

    case token::TokenKind::kOtherMinus: {
      if (Interpreter::read_only) {
        return {};
      }

      return -(Interpreter::ReadExpressionAtom());
    }

    case token::TokenKind::kKeywordNot: {
      if (Interpreter::read_only) {
        return {};
      }

      return !(Interpreter::ReadExpressionAtom());
    }

    case token::TokenKind::kOtherLeftParenthesis: {
      const value::LualikeValue inner_value = Interpreter::ReadExpression(1);

      if (Interpreter::iter_ == Interpreter::sentinel_) {
        throw InterpreterErr(InterpreterErrKind::kExpectedExpression);
      }

      if (const auto& token = *Interpreter::iter_;
          token.token_kind != token::TokenKind::kOtherRightParenthesis) {
        throw InterpreterErr(InterpreterErrKind::kUnclosedParanthesis);
      }

      Interpreter::iter_++;

      return inner_value;
    }

    break;

    default:
      throw InterpreterErr(InterpreterErrKind::kExpectedExpression);
  }
}

template <typename InputT>
  requires InputTRequirements<InputT>
value::LualikeValue Interpreter<InputT>::ReadExpression(
    const int min_precedence) {
  auto result = Interpreter::ReadExpressionAtom();

  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto token_kind = (*Interpreter::iter_).token_kind;

    const auto& find_result = kBinOpsPrecedences.find(token_kind);
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
        (token_kind == token::TokenKind::kOtherCaret) ? precedence
                                                      : precedence + 1);

    if (Interpreter::read_only) {
      continue;
    }

    switch (token_kind) {
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

      case token::TokenKind::kOtherPercent:
        result %= rhs;
        break;

      case token::TokenKind::kOtherCaret:
        result.ExponentiateAndAssign(rhs);
        break;

      default:
        throw std::logic_error("Unimplemented logic for binary operator.");
    }
  }

  return result;
}

template <typename InputT>
  requires InputTRequirements<InputT>
std::vector<token::Token> Interpreter<InputT>::ReadFunctionBody() {
  auto original_iter = std::move(Interpreter::iter_);
  auto original_sent = Interpreter::sentinel_;

  std::vector<token::Token> accumulated_tokens{};
  auto wrap_around_r = [&]() -> lexer::TokensRangeT {
    while (original_iter != original_sent) {
      accumulated_tokens.push_back(*original_iter);
      co_yield *original_iter;
    }
  }();

  Interpreter::iter_ = std::ranges::cbegin(wrap_around_r);
  Interpreter::sentinel_ = std::ranges::cend(wrap_around_r);
  Interpreter::read_only = true;

  Interpreter::ReadBlock();

  Interpreter::iter_ = std::move(original_iter);
  Interpreter::sentinel_ = original_sent;
  Interpreter::read_only = false;

  if (Interpreter::iter_ != Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kExpectedEndKeywordAfterFuncBody);
  }
  if (const auto& token = *Interpreter::iter_;
      token.token_kind != token::TokenKind::kKeywordEnd) {
    throw InterpreterErr(InterpreterErrKind::kExpectedEndKeywordAfterFuncBody);
  }
  Interpreter::iter_++;

  return accumulated_tokens;
}

template <typename InputT>
  requires InputTRequirements<InputT>
void Interpreter<InputT>::ReadName(bool is_local_decl) {
  const auto& token = *Interpreter::iter_;
  const auto variable_name{std::get<std::string>(token.token_data)};

  Interpreter::iter_++;
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kUnexpectedEOF);
  }
  switch (token.token_kind) {
    case token::TokenKind::kOtherEqual: {
      Interpreter::iter_++;
      const auto variable_value = Interpreter::ReadExpression();

      if (is_local_decl) {
        const auto [_, was_successfull] = Interpreter::local_names_.try_emplace(
            variable_name, variable_value);

        if (!was_successfull) {
          throw InterpreterErr(
              InterpreterErrKind::kRedeclarationOfLocalVariable);
        }
      }

      else {
        Interpreter::global_names_.insert_or_assign(variable_name,
                                                    variable_value);
      }
    }

    break;

    case token::TokenKind::kOtherLeftParenthesis: {
      // Pass
    }

    break;

    default:
      throw InterpreterErr(InterpreterErrKind::kExpectedAssignmentOrFuncCall);
  }
}

template <typename InputT>
  requires InputTRequirements<InputT>
std::optional<value::LualikeValue> Interpreter<InputT>::ReadBlock() {
  while (Interpreter::iter_ != Interpreter::sentinel_) {
    switch (const auto& token = *Interpreter::iter_; token.token_kind) {
      case token::TokenKind::kKeywordReturn:
        Interpreter::iter_++;

        if (Interpreter::iter_ == Interpreter::sentinel_) {
          return std::nullopt;
        }
        if (const auto token = *Interpreter::iter_;
            token.token_kind == token::TokenKind::kOtherSemicolon) {
          return std::nullopt;
        }

        return Interpreter::ReadExpression();

      case token::TokenKind::kKeywordLocal:
        Interpreter::iter_++;
        Interpreter::ReadName(true);
        break;

      case token::TokenKind::kName:
        Interpreter::ReadName(false);
        break;

      case token::TokenKind::kOtherSemicolon:
        Interpreter::iter_++;
        break;

      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

template <typename InputT>
  requires InputTRequirements<InputT>
std::expected<value::LualikeValue, InterpreterErr>
Interpreter<InputT>::EvaluateExpression() noexcept {
  try {
    return Interpreter::ReadExpression();
  }

  catch (const InterpreterErr& err) {
    return std::unexpected(InterpreterErr(err));
  }

  catch (const lexer::LexerErr& err) {
    return std::unexpected(InterpreterErr(err));
  }

  catch (const std::exception& excepion) {
    return std::unexpected(InterpreterErr(excepion));
  }

  catch (...) {
    return std::unexpected(
        InterpreterErr(InterpreterErrKind::kInternalException));
  }
}

template <typename InputT>
  requires InputTRequirements<InputT>
std::expected<std::optional<value::LualikeValue>, InterpreterErr>
Interpreter<InputT>::Interpret() noexcept {
  try {
    return Interpreter::ReadBlock();
  }

  catch (const InterpreterErr& err) {
    return std::unexpected(InterpreterErr(err));
  }

  catch (const lexer::LexerErr& err) {
    return std::unexpected(InterpreterErr(err));
  }

  catch (const std::exception& excepion) {
    return std::unexpected(InterpreterErr(excepion));
  }

  catch (...) {
    return std::unexpected(
        InterpreterErr(InterpreterErrKind::kInternalException));
  }
}

template class Interpreter<std::string_view>;

}  // namespace lualike::interpreter
