#ifndef LUALIKE_LEXER_H_
#define LUALIKE_LEXER_H_

#include <cstdint>
#include <expected>
#include <ranges>
#include <string>

#include "token.h"

namespace lualike::lexer {

enum class LexerErrKind : uint8_t {
  kEOF,
  kInvalidSymbolMet,
  kTooLongToken,
  kUnclosedStringLiteral,
  kUnrecognizedEscapeSequence,
  kExpectedDigit,
};

struct LexerErr : std::exception {
  LexerErrKind error_kind;

  explicit LexerErr(LexerErrKind error_kind) noexcept;
};

template <typename LexerInputRangeT>
concept LexerInputRangeTRequirements =
    std::ranges::input_range<LexerInputRangeT> &&
    std::same_as<char, std::ranges::range_value_t<LexerInputRangeT>>;

template <typename LexerInputRangeT>
  requires LexerInputRangeTRequirements<LexerInputRangeT>
class Lexer {
  std::ranges::iterator_t<LexerInputRangeT> iter_;
  std::ranges::sentinel_t<LexerInputRangeT> sentinel_;

  std::string token_data_accumulator_;
  static constexpr int kMaxOutputAccumLength = 16;

  // Initial call during token reading.
  // Pre: valid initial state of Lexer.
  token::Token ConsumeWhitespace();
  // Pre: single alphabetic symbol or undescore in accumulator.
  token::Token ReadAlphanumeric();
  // Pre: single symbol from alphabet of other tokens -
  // token::kOtherTokensAlphabet - in accumalator.
  token::Token ReadOtherToken();
  // Pre: accumulator is empty and delimiter is got
  token::Token ReadShortLiteralString(char delimiter);
  // Pre: single digit is in accumulator.
  token::Token ReadNumericConstant();
  // No precondition.
  token::Token ConsumeComment();

 public:
  explicit Lexer(LexerInputRangeT &&input_range) noexcept;

  // Iterates the contained ranged of symbols till syntatically valid token is
  // found, then returning it. In case of EOF returns token whose token_kind
  // field is set to token::TokenKind::kNone.
  //
  // Supposedly, the input should be valid. Lexer recognizes errors, but in the
  // case of recognition, a LexerErr is thrown.
  token::Token ReadToken();
};

}  // namespace lualike::lexer

#endif  // LUALIKE_LEXER_H_
