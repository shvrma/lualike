#ifndef LUALIKE_LEXER_H_
#define LUALIKE_LEXER_H_

#include <cstdint>
#include <exception>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

#include "lualike/token.h"

namespace lualike::lexer {

enum class LexerErrKind : uint8_t {
  kTooLongToken,
  kInvalidNumber,
  kInvalidName,
  kInvalidString,
  kInvalidSymbol,
};

struct LexerErr : std::exception {
  LexerErrKind error_kind;

  explicit LexerErr(LexerErrKind error_kind) noexcept
      : error_kind(error_kind) {}

  const char* what() const noexcept override {
    switch (error_kind) {
      case LexerErrKind::kTooLongToken:
        return "token too long";
      case LexerErrKind::kInvalidNumber:
        return "invalid number";
      case LexerErrKind::kInvalidName:
        return "invalid name";
      case LexerErrKind::kInvalidString:
        return "invalid string";
      case LexerErrKind::kInvalidSymbol:
        return "invalid symbol";
    }

    return "unknown lexer error";
  }
};

namespace {

inline bool IsSpace(char symbol) noexcept {
  return symbol == ' ' || symbol == '\f' || symbol == '\n' || symbol == '\r' ||
         symbol == '\t' || symbol == '\v';
}

inline bool IsAlphabetic(char symbol) noexcept {
  return (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z');
}

inline bool IsNumeric(char symbol) noexcept {
  return symbol >= '0' && symbol <= '9';
}

inline bool IsAlphanumeric(char symbol) noexcept {
  return IsAlphabetic(symbol) || IsNumeric(symbol);
}

}  // namespace

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
class Lexer {
  std::ranges::const_iterator_t<InputT> iter_;
  std::ranges::const_sentinel_t<InputT> sentinel_;
  std::ranges::const_iterator_t<InputT> input_begin_;

  token::Token ReadAlphanumeric(std::ranges::const_iterator_t<InputT> start);
  token::Token ReadShortLiteralString(char delimiter);
  token::Token ReadNumericConstant();

  token::Token FillTokenData(token::Token t,
                             std::ranges::const_iterator_t<InputT> start);

 public:
  explicit Lexer(InputT input)
      : iter_(std::ranges::cbegin(input)),
        sentinel_(std::ranges::cend(input)),
        input_begin_(std::ranges::cbegin(input)) {}

  std::optional<token::Token> NextToken();
};

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
token::Token Lexer<InputT>::ReadAlphanumeric(
    std::ranges::const_iterator_t<InputT> start) {
  while (iter_ != sentinel_) {
    const char symbol = *iter_;

    if (IsAlphanumeric(symbol) || symbol == '_') {
      ++iter_;
    } else {
      break;
    }
  }

  const std::string_view token_data(start, iter_);

  if (const auto find_result = token::kKeywordsMap.find(token_data);
      find_result != token::kKeywordsMap.cend()) {
    return {.token_kind = find_result->second};
  }

  return {.token_kind = token::TokenKind::kName};
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
token::Token Lexer<InputT>::ReadShortLiteralString(char delimiter) {
  while (iter_ != sentinel_) {
    const char symbol = *iter_;
    ++iter_;

    if (symbol == delimiter) {
      return {.token_kind = token::TokenKind::kStringLiteral};
    }
  }

  throw LexerErr(LexerErrKind::kInvalidString);
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
token::Token Lexer<InputT>::ReadNumericConstant() {
  bool has_met_fractional_part = false;

  while (iter_ != sentinel_) {
    const char symbol = *iter_;

    if (symbol == '.' || symbol == ',') {
      if (has_met_fractional_part) {
        throw LexerErr(LexerErrKind::kInvalidNumber);
      }

      has_met_fractional_part = true;
    } else if (!IsNumeric(symbol)) {
      break;
    }

    ++iter_;
  }

  if (has_met_fractional_part) {
    return {.token_kind = token::TokenKind::kFloatLiteral};
  }

  return {.token_kind = token::TokenKind::kIntLiteral};
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
std::optional<token::Token> Lexer<InputT>::NextToken() {
  while (iter_ != sentinel_) {
    if (IsSpace(*iter_)) {
      ++iter_;
      continue;
    }

    if (*iter_ == '-' && (iter_ + 1) != sentinel_ && *(iter_ + 1) == '-') {
      iter_ += 2;
      while (iter_ != sentinel_ && *iter_ != '\n') {
        ++iter_;
      }

      continue;
    }

    break;
  }

  if (iter_ == sentinel_) {
    return std::nullopt;
  }

  auto start = iter_;
  const char symbol = *iter_;

  if (IsAlphabetic(symbol) || symbol == '_') {
    ++iter_;
    return FillTokenData(ReadAlphanumeric(start), start);
  }

  if (IsNumeric(symbol)) {
    ++iter_;
    return FillTokenData(ReadNumericConstant(), start);
  }

  if (symbol == '\'' || symbol == '\"') {
    ++iter_;
    return FillTokenData(ReadShortLiteralString(symbol), start);
  }

  if ((iter_ + 1) != sentinel_) {
    if (const auto find_result =
            token::kOtherTwoCharTokensMap.find({iter_, iter_ + 2});
        find_result != token::kOtherTwoCharTokensMap.cend()) {
      iter_ += 2;
      return FillTokenData({.token_kind = find_result->second}, start);
    }
  }

  const auto find_result = token::kOtherSingleCharTokensMap.find(symbol);
  if (find_result != token::kOtherSingleCharTokensMap.cend()) {
    ++iter_;
    return FillTokenData({.token_kind = find_result->second}, start);
  }

  throw LexerErr(LexerErrKind::kInvalidSymbol);
}

template <std::ranges::view InputT>
  requires std::ranges::contiguous_range<InputT>
token::Token Lexer<InputT>::FillTokenData(
    token::Token next_token, std::ranges::const_iterator_t<InputT> start) {
  next_token.source_span =
      std::string_view(&*start, std::distance(start, iter_));
  return next_token;
}

}  // namespace lualike::lexer

#endif  // LUALIKE_LEXER_H_
