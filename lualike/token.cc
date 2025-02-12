#include "token.h"

#include <format>
#include <istream>

#include "lexer.h"

namespace lualike::token {

using lexer::Lexer;
using lexer::LexerErr;

std::istream &operator>>(std::istream &input_stream, Token &token) {
  Lexer lexer{};
  const auto read_result = lexer.ReadToken(input_stream);

  if (std::holds_alternative<Token>(read_result)) {
    token = std::get<Token>(read_result);
  }

  else if (std::holds_alternative<LexerErr>(read_result)) {
    const auto err = std::get<LexerErr>(read_result);
    switch (err) {
      case LexerErr::kEOF:
        input_stream.setstate(std::ios_base::eofbit);
        // No break to set failbit as well

      default:
        input_stream.setstate(std::ios_base::failbit);
        break;
    }
  }

  else {
    input_stream.setstate(std::ios_base::failbit);
  }

  return input_stream;
}

void PrintTo(const Token &token, std::ostream *output) {
  const auto token_kind_name = [&token]() -> std::string {
    switch (token.token_kind) {
      case TokenKind::kNone:
        return "none";

      case TokenKind::kName:
        return "name: ";

      case TokenKind::kLiteralString:
        return "string: ";

      case TokenKind::kNumericConstant:
        return "num: ";

      default:
        break;
    }

    const auto *find_result = std::ranges::find_if(
        token::kKeywordsMap,
        [&token](const auto &pair) { return pair.second == token.token_kind; });
    if (find_result != std::ranges::end(token::kKeywordsMap)) {
      return std::string{find_result->first};
    }

    find_result = std::ranges::find_if(
        token::kOtherTokensMap, [&token](const auto &pair) {
          return std::get<1>(pair) == token.token_kind;
        });
    if (find_result != std::ranges::end(token::kOtherTokensMap)) {
      return std::format("symbol: \'{}\'", find_result->first);
    }

    return std::format("unknown: {}", static_cast<int>(token.token_kind));
  }();

  *output << "<" << token_kind_name;

  if (token.token_data) {
    *output << " \"" << token.token_data.value().ToString() << "\"";
  }

  *output << ">";
}

}  // namespace lualike::token
