module;

#include <expected>
#include <generator>
#include <memory>
#include <optional>
#include <print>
#include <ranges>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "frozen/unordered_map.h"

module lualike.interpreter;

namespace lualike::interpreter {

constexpr auto kBinOpsPrecedences = frozen::make_unordered_map<TokenKind, int>({
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
});

LualikeValue Interpreter::ReadExpressionAtom() {
  if (iter_ == sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kExpectedExpression);
  }

  const auto token = *iter_;
  iter_++;

  switch (token.token_kind) {
    case TokenKind::kName: {
      const auto& name = std::get<std::string>(token.token_data);

      if (const auto find_result = Interpreter::local_names_.find(name);
          find_result != Interpreter::local_names_.end()) {
        return find_result->second;
      }

      if (const auto find_result = Interpreter::global_names_->find(name);
          find_result != Interpreter::global_names_->end()) {
        return find_result->second;
      }

      throw InterpreterErr(InterpreterErrKind::kUnknownName);
    }

    break;

    case TokenKind::kKeywordTrue:
      return {true};

    case TokenKind::kKeywordFalse:
      return {false};

    case TokenKind::kKeywordNil:
      return {};

    case TokenKind::kLiteral:
      return std::get<LualikeValue>(token.token_data);

    case TokenKind::kOtherMinus: {
      return -(Interpreter::ReadExpressionAtom());
    }

    case TokenKind::kKeywordNot: {
      return !(Interpreter::ReadExpressionAtom());
    }

    case TokenKind::kOtherLeftParenthesis: {
      const LualikeValue inner_value = Interpreter::ReadExpression();

      ExpectToken(TokenKind::kOtherRightParenthesis);

      return inner_value;
    }

    break;

    default:
      throw InterpreterErr(InterpreterErrKind::kExpectedExpression);
  }
}

LualikeValue Interpreter::ReadExpression(const int min_precedence) {
  auto result = ReadExpressionAtom();

  while (iter_ != sentinel_) {
    const auto token_kind = (*iter_).token_kind;

    const auto& find_result = kBinOpsPrecedences.find(token_kind);
    if (find_result == kBinOpsPrecedences.end()) {
      break;
    }

    const int precedence = find_result->second;
    if (precedence < min_precedence) {
      break;
    }

    iter_++;
    // Power operator is the only right-associative.
    const auto rhs = ReadExpression(
        (token_kind == TokenKind::kOtherCaret) ? precedence : precedence + 1);

    if (read_only) {
      continue;
    }

    switch (token_kind) {
      case TokenKind::kOtherPlus:
        result += rhs;

        break;

      case TokenKind::kOtherMinus:
        result -= rhs;

        break;

      case TokenKind::kOtherAsterisk:
        result *= rhs;

        break;

      case TokenKind::kOtherSlash:
        result /= rhs;

        break;

      case TokenKind::kOtherPercent:
        result %= rhs;

        break;

      case TokenKind::kOtherCaret:
        result.ExponentiateAndAssign(rhs);

        break;

      case TokenKind::kOtherDoubleEqual:
        result = {result == rhs};

        break;

      case TokenKind::kOtherTildeEqual:
        result = {result != rhs};

        break;

      default:
        throw std::logic_error("Unimplemented logic for binary operator.");
    }
  }

  return result;
}

void Interpreter::ReadName(bool is_local_decl) {
  const auto& token = *iter_;

  const auto variable_name = std::get<std::string>(token.token_data);

  iter_++;
  if (iter_ == sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kExpectedAssignmentOrFuncCall);
  }
  switch (token.token_kind) {
    case TokenKind::kOtherEqual: {
      iter_++;
      const auto variable_value = ReadExpression();

      if (is_local_decl) {
        const auto [_, was_successful] =
            local_names_.try_emplace(variable_name, variable_value);

        if (!was_successful) {
          throw InterpreterErr(
              InterpreterErrKind::kRedeclarationOfLocalVariable);
        }
      }

      else {
        global_names_->insert_or_assign(variable_name, variable_value);
      }
    }

    break;

    case TokenKind::kOtherLeftParenthesis:
      // TODO(shvrma): func call.

      break;

    default:
      throw InterpreterErr(InterpreterErrKind::kExpectedAssignmentOrFuncCall);
  }
}

std::vector<Token> Interpreter::CollectBlockTill(
    std::initializer_list<TokenKind> end_tokens, bool should_discard) {
  std::vector<Token> block_content{};

  while (iter_ != sentinel_) {
    const auto& token = *iter_;

    if (std::ranges::find(end_tokens, token.token_kind) != end_tokens.end()) {
      break;
    }

    if (token.token_kind == TokenKind::kKeywordIf) {
      block_content.append_range(CollectBlockTill({
          TokenKind::kKeywordEnd,
      }));
    }

    if (!should_discard) {
      block_content.push_back(token);
    }

    iter_++;
  }

  return block_content;
}

std::optional<LualikeValue> Interpreter::ReadBlock() {
  while (iter_ != sentinel_) {
    const auto& token = *iter_;

    switch (token.token_kind) {
      case TokenKind::kKeywordReturn:
        iter_++;

        if (iter_ == sentinel_) {
          return std::nullopt;
        }

        if (token.token_kind == TokenKind::kOtherSemicolon) {
          return std::nullopt;
        }

        return Interpreter::ReadExpression();

      case TokenKind::kKeywordLocal:
        iter_++;

        ReadName(true);

        break;

      case TokenKind::kName:
        ReadName(false);

        break;

      case TokenKind::kOtherSemicolon:
        iter_++;

        break;

      case TokenKind::kKeywordIf: {
        iter_++;

        if (auto early_return = ReadIfElseStatement()) {
          return early_return;
        }

        break;
      }

      // Not a valid statement.
      default:
        return std::nullopt;
    }
  }

  return std::nullopt;
}

std::optional<LualikeValue> Interpreter::ReadIfElseStatement() {
  const auto condition = Interpreter::ReadExpression();
  if (!std::holds_alternative<LualikeValue::BoolT>(condition.inner_value)) {
    // TODO(shvrma): throw error.
    return {};
  }
  const auto condition_value =
      std::get<LualikeValue::BoolT>(condition.inner_value);

  ExpectToken(TokenKind::kKeywordThen);

  auto if_block_content =
      CollectBlockTill({TokenKind::kKeywordElse, TokenKind::kKeywordEnd});

  if (iter_ == sentinel_) {
    // TODO(shvrma): throw error.
    return {};
  }

  std::optional<std::vector<Token>> else_block_content{};
  if (const auto& token = *iter_; token.token_kind == TokenKind::kKeywordElse) {
    iter_++;

    // Discard if condition holds, otherwise collect.
    else_block_content = std::make_optional(
        CollectBlockTill({TokenKind::kKeywordEnd}, condition_value));
  }

  ExpectToken(TokenKind::kKeywordEnd);

  // https://devblogs.microsoft.com/oldnewthing/20211103-00/?p=105870
  auto to_range =
      [](std::vector<Token>& block_content) -> std::generator<const Token&> {
    co_yield std::ranges::elements_of(block_content);
  };

  if (condition_value) {
    auto tokens_r = to_range(if_block_content);

    auto interpreter = Interpreter(tokens_r, global_names_);
    return interpreter.ReadBlock();
  }

  if (else_block_content.has_value()) {
    auto tokens_r = to_range(else_block_content.value());

    auto interpreter = Interpreter(tokens_r, global_names_);
    return interpreter.ReadBlock();
  }

  return std::nullopt;
}

template <std::ranges::view InputT>
std::expected<std::optional<LualikeValue>, InterpreterErr> Interpret(
    InputT input) noexcept {
  try {
    auto tokens_r = lexer::ReadTokens(input);
    auto interpreter =
        Interpreter(tokens_r, std::make_shared<Interpreter::NamesT>());
    return interpreter.ReadBlock();
  }

  catch (const InterpreterErr& err) {
    return std::unexpected(InterpreterErr(err));
  }

  catch (const lexer::LexerErr& err) {
    return std::unexpected(InterpreterErr(err));
  }

  catch (const std::exception& exception) {
    return std::unexpected(InterpreterErr(exception));
  }

  catch (...) {
    return std::unexpected(
        InterpreterErr(InterpreterErrKind::kInternalException));
  }
}

template std::expected<std::optional<LualikeValue>, InterpreterErr> Interpret(
    std::string_view);

}  // namespace lualike::interpreter
