module;

#include <expected>
#include <optional>

#include "frozen/unordered_map.h"

module lualike.interpreter;

namespace lualike::interpreter {

template <typename TokensRangeT, bool read_only = false>
struct Interpreter {
  static constexpr auto kBinOpsPrecedences =
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

  std::unordered_map<std::string, value::LualikeValue> local_names_;
  std::unordered_map<std::string, value::LualikeValue>& global_names_;

  TokensRangeT tokens_r_;
  std::ranges::const_iterator_t<TokensRangeT> iter_;
  std::ranges::const_sentinel_t<TokensRangeT> sentinel_;

  explicit Interpreter(
      TokensRangeT&& tokens_range,
      std::unordered_map<std::string, value::LualikeValue>& global_names)
      : tokens_r_(std::move(tokens_range)),
        iter_(std::ranges::cbegin(tokens_r_)),
        sentinel_(std::ranges::cend(tokens_r_)),
        global_names_(global_names) {}

  value::LualikeValue ReadExpressionAtom();
  value::LualikeValue ReadExpression(int min_precedence = 1);

  // Expect curent token to be the name and several other to form a assigment
  // statement.
  void ReadName(bool is_local_decl);
  void ReadFunctionBody();
  std::optional<value::LualikeValue> ReadBlock();

  InterpretationResult ReadChunk() noexcept;
};

template <typename TokensRangeT, bool read_only>
value::LualikeValue Interpreter<TokensRangeT, read_only>::ReadExpressionAtom() {
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kExpectedExpression);
  }

  const auto token = *Interpreter::iter_;
  Interpreter::iter_++;

  switch (token.token_kind) {
    case token::TokenKind::kName: {
      // if constexpr (read_only) {
      // return {};
      // }

      // const auto& name = std::get<std::string>(token.token_data);
      const auto* const name = "";

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

    case token::TokenKind::kLiteral:
      // if constexpr (read_only) {
      // return {};
      // }

      return std::get<value::LualikeValue>(token.token_data);

    case token::TokenKind::kOtherMinus: {
      const auto to_negate = Interpreter::ReadExpressionAtom();

      // if constexpr (read_only) {
      // return {};
      // }

      return -to_negate;
    }

    case token::TokenKind::kKeywordNot: {
      const auto to_negate = Interpreter::ReadExpressionAtom();

      // if constexpr (read_only) {
      // return {};
      // }

      return !to_negate;
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

    default:
      throw InterpreterErr(InterpreterErrKind::kExpectedExpression);
  }
}

template <typename TokensRangeT, bool read_only>
value::LualikeValue Interpreter<TokensRangeT, read_only>::ReadExpression(
    const int min_precedence) {
  auto result = Interpreter::ReadExpressionAtom();

  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto token = *Interpreter::iter_;

    const auto& find_result = kBinOpsPrecedences.find(token.token_kind);
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

    // if constexpr (read_only) {
    //   continue;
    // }

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

template <typename TokensRangeT, bool read_only>
void Interpreter<TokensRangeT, read_only>::ReadFunctionBody() {
  Interpreter<TokensRangeT, true> interpreter{};
}

template <typename TokensRangeT, bool read_only>
void Interpreter<TokensRangeT, read_only>::ReadName(bool is_local_decl) {
  const auto& token = *Interpreter::iter_;

  // const auto variable_name =
  // std::move(std::get<std::string>(token.token_data));
  const auto* const variable_name = "";

  Interpreter::iter_++;
  if (Interpreter::iter_ == Interpreter::sentinel_) {
    throw InterpreterErr(InterpreterErrKind::kUnexpectedEOF);
  }
  if (token.token_kind != token::TokenKind::kOtherEqual) {
    throw InterpreterErr(
        InterpreterErrKind::kExpectedEquilitySignInVarDeclaration);
  }

  const auto variable_value = Interpreter::ReadExpression();

  if (is_local_decl) {
    const auto [_, was_successfull] =
        Interpreter::local_names_.try_emplace(variable_name, variable_value);

    if (!was_successfull) {
      throw InterpreterErr(InterpreterErrKind::kRedeclarationOfLocalVariable);
    }
  }

  else {
    Interpreter::global_names_.insert_or_assign(variable_name, variable_value);
  }
}

template <typename TokensRangeT, bool read_only>
std::optional<value::LualikeValue>
Interpreter<TokensRangeT, read_only>::ReadBlock() {
  while (Interpreter::iter_ != Interpreter::sentinel_) {
    const auto& token = *Interpreter::iter_;

    switch (token.token_kind) {
      case token::TokenKind::kKeywordReturn:
        Interpreter::iter_++;
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

template <typename TokensRangeT, bool read_only>
InterpretationResult
Interpreter<TokensRangeT, read_only>::ReadChunk() noexcept {
  try {
    // return Interpreter::ReadBlock();
    return {};
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

}  // namespace lualike::interpreter

template <typename InputT>
  requires std::ranges::view<InputT> && std::ranges::input_range<InputT> &&
           std::is_same_v<char, std::ranges::range_value_t<InputT>>
lualike::interpreter::InterpretationResult
lualike::interpreter::EvaluateExpression(
    InputT&& input, std::unordered_map<std::string, value::LualikeValue>&
                        global_names) noexcept {
  Interpreter interpreter(lexer::ReadTokens(std::forward<InputT>(input)),
                          global_names);

  return interpreter.ReadExpression();
}

template <typename InputT>
  requires std::ranges::view<InputT> && std::ranges::input_range<InputT> &&
           std::is_same_v<char, std::ranges::range_value_t<InputT>>
lualike::interpreter::InterpretationResult lualike::interpreter::Interpret(
    InputT&& input, std::unordered_map<std::string, value::LualikeValue>&
                        global_names) noexcept {
  Interpreter interpreter(lexer::ReadTokens(std::forward<InputT>(input)),
                          global_names);

  return interpreter.ReadChunk();
}
