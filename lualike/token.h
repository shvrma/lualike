#ifndef LUALIKE_TOKEN_H_
#define LUALIKE_TOKEN_H_

#include <array>
#include <cstdint>
#include <optional>
#include <unordered_map>

#include "value.h"

namespace lualike::token {

enum class TokenKind : uint8_t {
  kNone,

  kName,             // Same as identifier
  kNumericConstant,  // Same as literal value
  kLiteralString,

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
  kOtherCaret,  // ^
  kOtherHash,   // #
  kOtherAmpersand,
  kOtherTilde,  // ~
  kOtherPipe,   // |
  kOtherDoubleLessThan,
  kOtherDoubleGreaterThan,
  kOtherDoubleSlash,
  kOtherSpace,
  kOtherDoubleEqual,
  kOtherTildeEqual,  // ~=
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
  kOtherDoubleColon,
  kOtherSemicolon,
  kOtherColon,
  kOtherComma,
  kOtherDot,
  kOtherDoubleDot,
  kOtherTrimpleDot,

  // Single and double quote as well as double square brackets are skipped
  // as they delimit the Literal String
};

struct Token {
  TokenKind token_kind;
  std::optional<value::LuaValue> token_data;

  bool operator==(const Token &rhs) const noexcept = default;
};

std::istream &operator>>(std::istream &input_stream, Token &token);

void PrintTo(const Token &token, std::ostream *output);

constexpr auto kKeywordsMap =
    std::to_array<std::pair<std::string_view, TokenKind>>({
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
    });

constexpr auto kOtherTokensMap =
    std::to_array<std::pair<std::string_view, TokenKind>>({
        {"+", TokenKind::kOtherPlus},
        {"-", TokenKind::kOtherMinus},
        {"*", TokenKind::kOtherAsterisk},
        {"/", TokenKind::kOtherSlash},
        {"%", TokenKind::kOtherPercent},
        {"^", TokenKind::kOtherCaret},
        {"#", TokenKind::kOtherHash},
        {"&", TokenKind::kOtherAmpersand},
        {"~", TokenKind::kOtherTilde},
        {"|", TokenKind::kOtherPipe},
        {"<<", TokenKind::kOtherDoubleLessThan},
        {">>", TokenKind::kOtherDoubleGreaterThan},
        {"//", TokenKind::kOtherDoubleSlash},
        {"==", TokenKind::kOtherDoubleEqual},
        {"~=", TokenKind::kOtherTildeEqual},
        {"<=", TokenKind::kOtherLessThanEqual},
        {">=", TokenKind::kOtherGreaterThanEqual},
        {"<", TokenKind::kOtherLessThan},
        {">", TokenKind::kOtherGreaterThan},
        {"=", TokenKind::kOtherEqual},
        {"(", TokenKind::kOtherLeftParenthesis},
        {")", TokenKind::kOtherRightParenthesis},
        {"{", TokenKind::kOtherLeftFigureBracket},
        {"}", TokenKind::kOtherRightFigureBracket},
        {"[", TokenKind::kOtherLeftSquareBracket},
        {"]", TokenKind::kOtherRightSquareBracket},
        {"::", TokenKind::kOtherDoubleColon},
        {";", TokenKind::kOtherSemicolon},
        {":", TokenKind::kOtherColon},
        {",", TokenKind::kOtherComma},
        {".", TokenKind::kOtherDot},
        {"..", TokenKind::kOtherDoubleDot},
        {"...", TokenKind::kOtherTrimpleDot},
    });

const std::unordered_map<token::TokenKind, int> kBinOperatorPrecedenceLevelMap{
    {TokenKind::kKeywordOr, 1},

    {TokenKind::kKeywordAnd, 2},

    {TokenKind::kOtherLessThan, 3},      {TokenKind::kOtherGreaterThan, 3},
    {TokenKind::kOtherLessThanEqual, 3}, {TokenKind::kOtherGreaterThanEqual, 3},
    {TokenKind::kOtherTildeEqual, 3},    {TokenKind::kOtherDoubleEqual, 3},

    {TokenKind::kOtherPipe, 4},

    {TokenKind::kOtherTilde, 5},

    {TokenKind::kOtherAmpersand, 6},

    {TokenKind::kOtherPlus, 9},          {TokenKind::kOtherMinus, 9},

    {TokenKind::kOtherAsterisk, 10},     {TokenKind::kOtherSlash, 10},
    {TokenKind::kOtherDoubleSlash, 10},  {TokenKind::kOtherPercent, 10},
};

}  // namespace lualike::token

#endif
