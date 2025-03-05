module;

#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <variant>

export module lualike.token;

export import lualike.value;

namespace value = lualike::value;

export namespace lualike::token {

enum class TokenKind : uint8_t {
  kNone,

  kName,  // Same as identifier
  kLiteral,

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
  kOtherDoubleSlash,
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
  kOtherSemicolon,
  kOtherColon,
  kOtherComma,
  kOtherDot,
};

struct Token {
  TokenKind token_kind;
  std::variant<std::monostate, std::string, value::LualikeValue> token_data;

  bool operator==(const Token &rhs) const = default;

  std::string ToString() const noexcept;
};

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
        {";", TokenKind::kOtherSemicolon},
        {":", TokenKind::kOtherColon},
        {",", TokenKind::kOtherComma},
        {".", TokenKind::kOtherDot},
    });

std::string Token::ToString() const noexcept {
  std::ostringstream output{};

  const auto token_kind_name = [this]() -> std::string {
    switch (Token::token_kind) {
      case TokenKind::kNone:
        return "none";

      case TokenKind::kName:
        return "name";

      case TokenKind::kLiteral:
        return "literal";

      default:
        break;
    }

    if (const auto &find_result =
            std::ranges::find_if(token::kKeywordsMap,
                                 [this](const auto &pair) {
                                   return pair.second == Token::token_kind;
                                 });
        find_result != std::ranges::end(kKeywordsMap)) {
      return std::string{find_result->first};
    }

    if (const auto &find_result = std::ranges::find_if(
            kOtherTokensMap,
            [this](const auto &pair) {
              return std::get<1>(pair) == Token::token_kind;
            });
        find_result != std::ranges::end(kOtherTokensMap)) {
      return std::format("symbol: <<< {} >>>", find_result->first);
    }

    return std::format("unknown: <<< {} >>>",
                       static_cast<int>(Token::token_kind));
  }();

  output << token_kind_name;

  if (std::holds_alternative<std::string>(Token::token_data)) {
    output << " <<< " << std::get<std::string>(Token::token_data) << " >>>";
  }

  else if (std::holds_alternative<value::LualikeValue>(Token::token_data)) {
    output << " <<< "
           << std::get<value::LualikeValue>(Token::token_data).ToString()
           << " >>>";
  }

  return output.str();
}

}  // namespace lualike::token
