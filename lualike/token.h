#ifndef LUALIKE_TOKEN_H_
#define LUALIKE_TOKEN_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace lualike::token {

struct SourceSpan {
  size_t begin{};
  size_t end{};

  constexpr bool empty() const noexcept { return begin >= end; }
  constexpr size_t size() const noexcept {
    return end > begin ? end - begin : 0;
  }

  bool operator==(const SourceSpan& rhs) const = default;
};

inline constexpr SourceSpan MergeSourceSpans(const SourceSpan& lhs,
                                             const SourceSpan& rhs) noexcept {
  if (lhs.empty()) {
    return rhs;
  }
  if (rhs.empty()) {
    return lhs;
  }

  return {std::min(lhs.begin, rhs.begin), std::max(lhs.end, rhs.end)};
}

enum class TokenKind : uint8_t {
  kNone,

  kName,
  kStringLiteral,
  kIntLiteral,
  kFloatLiteral,

  kKeywordAnd,
  kKeywordBreak,
  kKeywordDo,
  kKeywordElse,
  kKeywordElseif,
  kKeywordEnd,
  kKeywordFalse,
  kKeywordFor,
  kKeywordFunction,
  kKeywordGoto,
  kKeywordIf,
  kKeywordIn,
  kKeywordLocal,
  kKeywordNil,
  kKeywordNot,
  kKeywordOr,
  kKeywordRepeat,
  kKeywordReturn,
  kKeywordThen,
  kKeywordTrue,
  kKeywordUntil,
  kKeywordWhile,

  kOtherPlus,
  kOtherMinus,
  kOtherAsterisk,
  kOtherSlash,
  kOtherPercent,
  kOtherCaret,
  kOtherDoubleSlash,
  kOtherDoubleEqual,
  kOtherTildeEqual,
  kOtherLessThanEqual,
  kOtherGreaterThanEqual,
  kOtherLessThan,
  kOtherGreaterThan,
  kOtherEqual,
  kOtherLeftParenthesis,
  kOtherRightParenthesis,
  kOtherLeftFigureBracket,
  kOtherRightFigureBracket,
  kOtherLeftSquareBracket,
  kOtherRightSquareBracket,
  kOtherSemicolon,
  kOtherColon,
  kOtherComma,
  kOtherDot,
};

struct Token {
  TokenKind token_kind;
  std::string_view source_span;
  SourceSpan span;

  bool operator==(const Token& rhs) const = default;
};

inline const std::unordered_map<std::string_view, TokenKind> kKeywordsMap = {
    {"and", TokenKind::kKeywordAnd},
    {"break", TokenKind::kKeywordBreak},
    {"do", TokenKind::kKeywordDo},
    {"else", TokenKind::kKeywordElse},
    {"elseif", TokenKind::kKeywordElseif},
    {"end", TokenKind::kKeywordEnd},
    {"false", TokenKind::kKeywordFalse},
    {"for", TokenKind::kKeywordFor},
    {"function", TokenKind::kKeywordFunction},
    {"goto", TokenKind::kKeywordGoto},
    {"if", TokenKind::kKeywordIf},
    {"in", TokenKind::kKeywordIn},
    {"local", TokenKind::kKeywordLocal},
    {"nil", TokenKind::kKeywordNil},
    {"not", TokenKind::kKeywordNot},
    {"or", TokenKind::kKeywordOr},
    {"repeat", TokenKind::kKeywordRepeat},
    {"return", TokenKind::kKeywordReturn},
    {"then", TokenKind::kKeywordThen},
    {"true", TokenKind::kKeywordTrue},
    {"until", TokenKind::kKeywordUntil},
    {"while", TokenKind::kKeywordWhile},
};

inline const std::unordered_map<char, TokenKind> kOtherSingleCharTokensMap = {
    {'+', TokenKind::kOtherPlus},
    {'-', TokenKind::kOtherMinus},
    {'*', TokenKind::kOtherAsterisk},
    {'/', TokenKind::kOtherSlash},
    {'%', TokenKind::kOtherPercent},
    {'^', TokenKind::kOtherCaret},
    {'<', TokenKind::kOtherLessThan},
    {'>', TokenKind::kOtherGreaterThan},
    {'=', TokenKind::kOtherEqual},
    {'(', TokenKind::kOtherLeftParenthesis},
    {')', TokenKind::kOtherRightParenthesis},
    {';', TokenKind::kOtherSemicolon},
    {',', TokenKind::kOtherComma},
};

inline const std::unordered_map<std::string_view, TokenKind>
    kOtherTwoCharTokensMap = {
        {"//", TokenKind::kOtherDoubleSlash},
        {"==", TokenKind::kOtherDoubleEqual},
        {"~=", TokenKind::kOtherTildeEqual},
        {"<=", TokenKind::kOtherLessThanEqual},
        {">=", TokenKind::kOtherGreaterThanEqual},
};

}  // namespace lualike::token

#endif  // LUALIKE_TOKEN_H_
