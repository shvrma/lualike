#ifndef LUALIKE_LEXER_H_
#define LUALIKE_LEXER_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string_view>
#include <vector>

#include "lualike/error.h"
#include "lualike/token.h"

namespace lualike::lexer {

enum class LexerErrKind : uint8_t {
  kTooLongToken,
  kInvalidNumber,
  kInvalidName,
  kInvalidString,
  kInvalidSymbol,
};

inline std::string_view LexerErrKindToString(LexerErrKind error_kind) noexcept {
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

inline error::Error MakeLexerError(LexerErrKind error_kind,
                                   token::SourceSpan span) {
  return error::Error::Context(std::string(LexerErrKindToString(error_kind)),
                               span);
}

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

template <std::ranges::contiguous_range InputT>
class Lexer {
  using IterT = std::ranges::const_iterator_t<InputT>;
  using SentT = std::ranges::const_sentinel_t<InputT>;

  // *iter_* and *sentinel_* denotes the whole source range.
  // *input_begin_* denotes the beginning of the current token (depends on what
  // that token is).

  IterT iter_;
  SentT sentinel_;
  IterT input_begin_;

  token::SourceSpan MakeSpan(IterT begin, IterT end) const;
  token::Token ReadAlphanumeric(IterT start);
  token::Token ReadShortLiteralString(char delimiter, IterT start);
  token::Token ReadNumericConstant(IterT start);

  token::Token FillTokenData(token::Token t, IterT start);

 public:
  explicit Lexer(InputT input)
      : iter_(std::ranges::cbegin(input)),
        sentinel_(std::ranges::cend(input)),
        input_begin_(std::ranges::cbegin(input)) {}

  std::optional<token::Token> NextToken();
};

template <std::ranges::contiguous_range InputT>
token::SourceSpan Lexer<InputT>::MakeSpan(IterT begin, IterT end) const {
  return {static_cast<size_t>(std::distance(input_begin_, begin)),
          static_cast<size_t>(std::distance(input_begin_, end))};
}

template <std::ranges::contiguous_range InputT>
token::Token Lexer<InputT>::ReadAlphanumeric(IterT start) {
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

template <std::ranges::contiguous_range InputT>
token::Token Lexer<InputT>::ReadShortLiteralString(
    char delimiter, std::ranges::const_iterator_t<InputT> start) {
  while (iter_ != sentinel_) {
    const char symbol = *iter_;
    ++iter_;

    if (symbol == delimiter) {
      return {.token_kind = token::TokenKind::kStringLiteral};
    }
  }

  throw MakeLexerError(LexerErrKind::kInvalidString, MakeSpan(start, iter_));
}

template <std::ranges::contiguous_range InputT>
token::Token Lexer<InputT>::ReadNumericConstant(
    std::ranges::const_iterator_t<InputT> start) {
  bool has_met_fractional_part = false;

  while (iter_ != sentinel_) {
    const char symbol = *iter_;

    if (symbol == '.' || symbol == ',') {
      if (has_met_fractional_part) {
        throw MakeLexerError(LexerErrKind::kInvalidNumber,
                             MakeSpan(start, iter_ + 1));
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

template <std::ranges::contiguous_range InputT>
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
    return FillTokenData(ReadNumericConstant(start), start);
  }

  if (symbol == '\'' || symbol == '\"') {
    ++iter_;
    return FillTokenData(ReadShortLiteralString(symbol, start), start);
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

  throw MakeLexerError(LexerErrKind::kInvalidSymbol,
                       MakeSpan(start, iter_ + 1));
}

template <std::ranges::contiguous_range InputT>
token::Token Lexer<InputT>::FillTokenData(token::Token next_token,
                                          IterT start) {
  next_token.source_span =
      std::string_view(&*start, std::distance(start, iter_));
  next_token.span = MakeSpan(start, iter_);
  return next_token;
}

}  // namespace lualike::lexer

#endif  // LUALIKE_LEXER_H_
