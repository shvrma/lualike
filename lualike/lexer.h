#ifndef LUALIKE_LEXER_H_
#define LUALIKE_LEXER_H_

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

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

class Lexer {
  std::string_view source_;
  size_t cursor_{};

  token::Token ReadAlphanumeric(size_t start);
  token::Token ReadShortLiteralString(char delimiter, size_t start);
  token::Token ReadNumericConstant(size_t start);

  token::Token FillTokenData(token::Token t, size_t start);

 public:
  explicit Lexer(std::string_view input) : source_(input) {}

  std::optional<token::Token> NextToken();
};

inline token::Token Lexer::ReadAlphanumeric(size_t start) {
  while (cursor_ < source_.size()) {
    const char symbol = source_[cursor_];

    if (IsAlphanumeric(symbol) || symbol == '_') {
      ++cursor_;
    } else {
      break;
    }
  }

  const auto token_data = source_.substr(start, cursor_ - start);

  if (const auto find_result = token::kKeywordsMap.find(token_data);
      find_result != token::kKeywordsMap.cend()) {
    return {.token_kind = find_result->second};
  }

  return {.token_kind = token::TokenKind::kName};
}

inline token::Token Lexer::ReadShortLiteralString(char delimiter,
                                                  size_t start) {
  while (cursor_ < source_.size()) {
    const char symbol = source_[cursor_];
    ++cursor_;

    if (symbol == delimiter) {
      return {.token_kind = token::TokenKind::kStringLiteral};
    }
  }

  throw MakeLexerError(LexerErrKind::kInvalidString, {start, cursor_});
}

inline token::Token Lexer::ReadNumericConstant(size_t start) {
  bool has_met_fractional_part = false;

  while (cursor_ < source_.size()) {
    const char symbol = source_[cursor_];

    if (symbol == '.' || symbol == ',') {
      if (has_met_fractional_part) {
        throw MakeLexerError(LexerErrKind::kInvalidNumber,
                             {start, cursor_ + 1});
      }

      has_met_fractional_part = true;
    } else if (!IsNumeric(symbol)) {
      break;
    }

    ++cursor_;
  }

  if (has_met_fractional_part) {
    return {.token_kind = token::TokenKind::kFloatLiteral};
  }

  return {.token_kind = token::TokenKind::kIntLiteral};
}

inline std::optional<token::Token> Lexer::NextToken() {
  while (cursor_ < source_.size()) {
    if (IsSpace(source_[cursor_])) {
      ++cursor_;
    }

    else if (source_[cursor_] == '-' && cursor_ + 1 < source_.size() &&
             source_[cursor_ + 1] == '-') {
      cursor_ += 2;
      while (cursor_ < source_.size() && source_[cursor_] != '\n') {
        ++cursor_;
      }

    }

    else {
      break;
    }
  }

  if (cursor_ == source_.size()) {
    return std::nullopt;
  }

  const size_t start = cursor_;
  const char symbol = source_[cursor_];

  if (IsAlphabetic(symbol) || symbol == '_') {
    ++cursor_;
    return FillTokenData(ReadAlphanumeric(start), start);
  }

  if (IsNumeric(symbol)) {
    ++cursor_;
    return FillTokenData(ReadNumericConstant(start), start);
  }

  if (symbol == '\'' || symbol == '\"') {
    ++cursor_;
    return FillTokenData(ReadShortLiteralString(symbol, start), start);
  }

  if (cursor_ + 1 < source_.size()) {
    if (const auto find_result =
            token::kOtherTwoCharTokensMap.find(source_.substr(cursor_, 2));
        find_result != token::kOtherTwoCharTokensMap.cend()) {
      cursor_ += 2;
      return FillTokenData({.token_kind = find_result->second}, start);
    }
  }

  const auto find_result = token::kOtherSingleCharTokensMap.find(symbol);
  if (find_result != token::kOtherSingleCharTokensMap.cend()) {
    ++cursor_;
    return FillTokenData({.token_kind = find_result->second}, start);
  }

  throw MakeLexerError(LexerErrKind::kInvalidSymbol, {start, cursor_ + 1});
}

inline token::Token Lexer::FillTokenData(token::Token next_token,
                                         size_t start) {
  next_token.source_span = source_.substr(start, cursor_ - start);
  next_token.span = {start, cursor_};
  return next_token;
}

}  // namespace lualike::lexer

#endif  // LUALIKE_LEXER_H_
