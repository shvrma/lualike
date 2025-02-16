#include "token.h"

#include <format>
#include <sstream>

namespace token = lualike::token;

using token::Token;

std::string Token::ToString() const noexcept {
  std::ostringstream output{};

  const auto token_kind_name = [this]() -> std::string {
    switch (Token::token_kind) {
      case TokenKind::kNone:
        return "none";

      case TokenKind::kName:
        return "name";

      case TokenKind::kLiteralString:
        return "string";

      case TokenKind::kNumericConstant:
        return "num";

      default:
        break;
    }

    const auto *find_result = std::ranges::find_if(
        token::kKeywordsMap,
        [this](const auto &pair) { return pair.second == Token::token_kind; });
    if (find_result != std::ranges::end(token::kKeywordsMap)) {
      return std::string{find_result->first};
    }

    find_result =
        std::ranges::find_if(token::kOtherTokensMap, [this](const auto &pair) {
          return std::get<1>(pair) == Token::token_kind;
        });
    if (find_result != std::ranges::end(token::kOtherTokensMap)) {
      return std::format("symbol: <<< {} >>>", find_result->first);
    }

    return std::format("unknown: <<< {} >>>",
                       static_cast<int>(Token::token_kind));
  }();

  output << token_kind_name;

  if (Token::token_data) {
    output << "<<< " << Token::token_data.value().ToString() << " >>>";
  }

  return output.str();
}
