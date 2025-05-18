module;

#include <generator>
#include <ranges>

#include "frozen/string.h"
#include "frozen/unordered_map.h"

module lualike.lexer;

using lualike::token::kKeywordsMap;
using lualike::token::kOtherSingleCharTokensMap;
using lualike::token::kOtherTwoCharTokensMap;
using lualike::token::Token;
using lualike::token::TokenKind;
using lualike::value::LualikeValue;

namespace lualike::lexer {

inline bool IsSpace(const char symbol) noexcept {
  return symbol == ' ' || symbol == '\f' || symbol == '\n' || symbol == '\r' ||
         symbol == '\t' || symbol == '\v';
}

inline bool IsAlphabetic(const char symbol) noexcept {
  return (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z');
}

inline bool IsNumeric(const char symbol) noexcept {
  return (symbol >= '0' && symbol <= '9');
}

inline bool IsAlphanumeric(const char symbol) noexcept {
  return IsAlphabetic(symbol) || IsNumeric(symbol);
}

template <std::ranges::view InputT>
Token Lexer<InputT>::ReadAlphanumeric() {
  while (iter_ != sentinel_) {
    char symbol = *iter_;

    if (IsAlphanumeric(symbol) || symbol == '_') {
      if (token_data_accumulator_.length() == kMaxOutputAccumLength) {
        throw LexerErr(LexerErrKind::kTooLongToken);
      }

      iter_++;
      token_data_accumulator_ += symbol;
    }

    else {
      break;
    }
  }

  if (const auto& find_result =
          token::kKeywordsMap.find(frozen::string{token_data_accumulator_});
      find_result != token::kKeywordsMap.cend()) {
    return {.token_kind = find_result->second};
  }

  return {.token_kind = token::TokenKind::kName,
          .token_data{std::move(token_data_accumulator_)}};
}

template <std::ranges::view InputT>
token::Token Lexer<InputT>::ReadShortLiteralString(char delimiter) {
  bool is_escaped = false;

  while (iter_ != sentinel_) {
    char symbol = *iter_;
    iter_++;

    const auto kEscapeSeqMap = frozen::make_unordered_map<char, char>({
        {'a', '\a'},
        {'b', '\b'},
        {'f', '\f'},
        {'n', '\n'},
        {'r', '\r'},
        {'t', '\t'},
        {'\\', '\\'},
        {'\"', '\"'},
        {'\'', '\''},
    });

    if (is_escaped) {
      const auto* find_result = kEscapeSeqMap.find(symbol);
      if (find_result == kEscapeSeqMap.cend()) {
        throw LexerErr(LexerErrKind::kInvalidString);
      }

      token_data_accumulator_ += find_result->second;

      is_escaped = false;
      continue;
    }

    if (symbol == '\\') {
      is_escaped = true;
      continue;
    }

    if (symbol == delimiter) {
      return {.token_kind = TokenKind::kLiteral,
              .token_data{LualikeValue{
                  LualikeValue::StringT{std::move(token_data_accumulator_)}}}};
    }

    token_data_accumulator_ += symbol;
  }

  throw LexerErr(LexerErrKind::kInvalidString);
}

template <std::ranges::view InputT>
Token Lexer<InputT>::ReadNumericConstant() {
  bool has_met_fractional_part = false;

  while (iter_ != sentinel_) {
    char symbol = *iter_;

    if (symbol == '.' || symbol == ',') {
      if (has_met_fractional_part) {
        throw LexerErr(LexerErrKind::kInvalidNumber);
      }

      has_met_fractional_part = true;
    }

    else if (!IsNumeric(symbol)) {
      break;
    }

    iter_++;
    token_data_accumulator_ += symbol;
  }

  if (has_met_fractional_part) {
    return {.token_kind = TokenKind::kLiteral,
            .token_data{LualikeValue{std::stod(token_data_accumulator_)}}};
  }

  return {.token_kind = TokenKind::kLiteral,
          .token_data{LualikeValue{std::stol(token_data_accumulator_)}}};
}

template <std::ranges::view InputT>
std::optional<Token> Lexer<InputT>::NextToken() {
  while (iter_ != sentinel_ && IsSpace(*iter_)) {
    iter_++;
  }

  if (iter_ == sentinel_) {
    return std::nullopt;
  }

  token_data_accumulator_.clear();

  char symbol = *iter_;
  iter_++;

  if (symbol == '\'' || symbol == '\"') {
    return ReadShortLiteralString(symbol);
  }

  if (symbol == '_' || IsAlphabetic(symbol)) {
    token_data_accumulator_ += symbol;
    return ReadAlphanumeric();
  }

  if (IsNumeric(symbol)) {
    token_data_accumulator_ += symbol;
    return ReadNumericConstant();
  }

  if (const auto* find_result = kOtherSingleCharTokensMap.find(symbol);
      find_result != token::kOtherSingleCharTokensMap.cend()) {
    if (iter_ == sentinel_) {
      return {{.token_kind = find_result->second}};
    }

    const char curr_symbol = *iter_;

    if (symbol == '-' && curr_symbol == '-') {
      iter_++;

      while (iter_ != sentinel_ && *iter_ != '\n') {
        iter_++;
      }

      return NextToken();
    }

    char long_token[] = {symbol, curr_symbol};
    if (const auto* const find_result = kOtherTwoCharTokensMap.find(long_token);
        find_result != kOtherTwoCharTokensMap.cend()) {
      iter_++;
      return {{.token_kind = find_result->second}};
    }

    return {{.token_kind = find_result->second}};
  }

  throw LexerErr(LexerErrKind::kInvalidSymbol);
}

template <std::ranges::view InputT>
std::generator<const Token&> ReadTokens(InputT input) {
  Lexer<InputT> lexer(input);

  while (const auto token = lexer.NextToken()) {
    co_yield token.value();
  }
}

template std::generator<const Token&> ReadTokens(std::string_view input);

}  // namespace lualike::lexer
