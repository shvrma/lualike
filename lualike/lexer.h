#ifndef LUALIKE_LEXER_H_
#define LUALIKE_LEXER_H_

#include <cstdint>
#include <string>
#include <variant>

#include "token.h"

namespace lualike::lexer {

enum class LexerErr : uint8_t {
  kEOF,
  kInvalidSymbolMet,
  kInputNotOk,
  kTooLongToken
};

struct ConsumeWhitespaceLexerState {};

struct ReadAlphanumericLexerState {};

struct ReadOtherTokenLexerState {};

struct ReadShortLiteralStringLexerState {
  char delimiter;
  bool is_escaped = false;
};

struct ReadNumericConstantLexerState {
  bool has_met_fractional_part = false;
};

struct ConsumeCommentLexerState {};

struct ReturnTokenLexerState {
  token::Token token;
  bool should_consume_current = false;
};

struct ReturnErrorLexerState {
  LexerErr error;
};

using LexerState =
    std::variant<std::monostate,

                 ConsumeWhitespaceLexerState, ReadAlphanumericLexerState,
                 ReadOtherTokenLexerState, ReadShortLiteralStringLexerState,
                 ReadNumericConstantLexerState, ConsumeCommentLexerState,

                 ReturnTokenLexerState, ReturnErrorLexerState>;

class Lexer {
  std::string output_accumulator_;
  static constexpr int kMaxOutputAccumLength = 16;

  LexerState state_;

  inline LexerState ConsumeWhitespace(
      char symbol, ConsumeWhitespaceLexerState &state) noexcept;
  inline LexerState ReadAlphanumeric(
      char symbol, ReadAlphanumericLexerState &state) noexcept;
  inline LexerState ReadOtherToken(char symbol,
                                   ReadOtherTokenLexerState &state) noexcept;
  inline LexerState ReadShortLiteralString(
      char symbol, ReadShortLiteralStringLexerState &state) noexcept;
  inline LexerState ReadNumericConstant(
      char symbol, ReadNumericConstantLexerState &state) noexcept;
  inline LexerState ConsumeComment(char symbol,
                                   ConsumeCommentLexerState &state) noexcept;

 public:
  using ReadTokenResult = std::variant<token::Token, LexerErr>;
  ReadTokenResult ReadToken(std::istream &input_stream) noexcept;
};

}  // namespace lualike::lexer

#endif  // LUALIKE_LEXER_H_
