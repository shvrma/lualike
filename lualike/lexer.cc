#include "lexer.h"

#include <algorithm>
#include <iterator>

#include "token.h"
#include "value.h"

namespace lualike::lexer {

using token::Token;
using token::TokenKind;

using value::LualikeValue;

namespace {

template <auto next_array_size>
struct Vertex {
  std::array<int, next_array_size> next;
  TokenKind output = TokenKind::kNone;

  constexpr Vertex() noexcept { std::ranges::fill(Vertex::next, -1); }
};

}  // namespace

static constexpr auto kKeywordsTrie = []() consteval {
  constexpr int kAlphabetSize = 26;

  constexpr auto calc = [](auto trie) {
    int current_trie_size = 1;

    for (const auto &[word, tokenKind] : token::kKeywordsMap) {
      int current_node = 0;

      for (const char symbol : std::string_view{word}) {
        int idx = symbol - 'a';

        if (trie.at(current_node).next.at(idx) == -1) {
          trie.at(current_node).next.at(idx) = current_trie_size;
          current_trie_size++;
        }

        current_node = trie.at(current_node).next.at(idx);
      }

      trie.at(current_node).output = tokenKind;
    }

    return std::make_pair(trie, current_trie_size);
  };

  constexpr auto kMaxTrieSize = std::ranges::distance(
      token::kKeywordsMap | std::views::keys | std::views::join);
  constexpr auto kTrieSize =
      calc(std::array<Vertex<kAlphabetSize>, kMaxTrieSize>{}).second;

  return calc(std::array<Vertex<kAlphabetSize>, kTrieSize>{}).first;
}();

static constexpr int kOtherTokensMaxLength = std::ranges::max(
    token::kOtherTokensMap | std::views::keys |
    std::views::transform([](const auto str) { return str.length(); }));

static constexpr auto kOtherTokensAlphabet = []() consteval {
  constexpr auto all_symbols =
      token::kOtherTokensMap | std::views::keys | std::views::join;

  constexpr auto calculate = [all_symbols](auto alphabet) {
    auto iter = alphabet.begin();

    for (const char &symbol : all_symbols) {
      if (std::ranges::none_of(
              alphabet.begin(), iter,
              [symbol](const char cmp) { return cmp == symbol; })) {
        *iter = symbol;
        ++iter;
      }
    }

    return std::make_pair(alphabet,
                          std::ranges::distance(alphabet.begin(), iter));
  };

  constexpr int kMaxAlphabetSize = std::ranges::distance(all_symbols);
  constexpr int kAlphabetSize =
      calculate(std::array<char, kMaxAlphabetSize>{}).second;

  return calculate(std::array<char, kAlphabetSize>{}).first;
}();

// Prime numbers list: 11, 13, 17, 19, 23, 29, 31, 37, 41
static constexpr int kModuloDivisitorOfHashFunction = 37;

static constexpr int OtherTokensHashFunc(const char symbol) noexcept {
  return static_cast<int>(symbol) % kModuloDivisitorOfHashFunction;
}

static constexpr auto kOtherTokensTrie = []() consteval {
  // Validate hash function
  std::array<int, kOtherTokensAlphabet.size()> hashes{};
  std::ranges::copy(
      kOtherTokensAlphabet |
          std::views::transform([](const char symbol) constexpr -> int {
            return OtherTokensHashFunc(symbol);
          }),
      hashes.begin());

  std::ranges::sort(hashes);
  if (std::ranges::adjacent_find(hashes) != hashes.end()) {
    throw new std::invalid_argument(
        "Hash function doesnt produce unique values");
  }

  constexpr auto calc = [](auto trie) {
    auto current_trie_size = 1;

    for (const auto &[token, token_kind] : token::kOtherTokensMap) {
      auto current_node = 0;

      for (const char symbol : std::string_view{token}) {
        int idx = OtherTokensHashFunc(symbol);

        if (trie.at(current_node).next.at(idx) == -1) {
          trie.at(current_node).next.at(idx) = current_trie_size;
          current_trie_size++;
        }

        current_node = trie.at(current_node).next.at(idx);
      }

      trie.at(current_node).output = token_kind;
    }

    return std::make_pair(trie, current_trie_size);
  };

  constexpr auto kMaxTrieSize = std::ranges::max(
      token::kOtherTokensMap | std::views::keys | std::views::join);
  constexpr auto kTrieSize =
      calc(std::array<Vertex<kModuloDivisitorOfHashFunction>, kMaxTrieSize>{})
          .second;

  return calc(std::array<Vertex<kModuloDivisitorOfHashFunction>, kTrieSize>{})
      .first;
}();

static inline bool IsSpace(const char symbol) noexcept {
  return symbol == ' ' || symbol == '\f' || symbol == '\n' || symbol == '\r' ||
         symbol == '\t' || symbol == '\v';
}

static inline bool IsAlphabetic(const char symbol) noexcept {
  return (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z');
}

static inline bool IsNumeric(const char symbol) noexcept {
  return (symbol >= '0' && symbol <= '9');
}

static inline bool IsAlphanumeric(const char symbol) noexcept {
  return IsAlphabetic(symbol) || IsNumeric(symbol);
}

static inline bool IsOther(const char symbol) noexcept {
  return std::ranges::find(kOtherTokensAlphabet, symbol) !=
         std::ranges::end(kOtherTokensAlphabet);
}

LexerErr::LexerErr(LexerErrKind error_kind) noexcept : error_kind(error_kind) {}

template <typename LexerInputRangeT>
  requires LexerInputRangeTRequirements<LexerInputRangeT>
Lexer<LexerInputRangeT>::Lexer(LexerInputRangeT &&input_range) noexcept
    : iter_(input_range.begin()), sentinel_(input_range.end()) {}

template <typename LexerInputRangeT>
  requires LexerInputRangeTRequirements<LexerInputRangeT>
Token Lexer<LexerInputRangeT>::ConsumeWhitespace() {
  for (; Lexer::iter_ != Lexer::sentinel_; Lexer::iter_++) {
    char symbol = *Lexer::iter_;
    if (IsSpace(symbol)) {
      continue;
    }

    Lexer::token_data_accumulator_ = {};

    if (symbol == '_' || IsAlphabetic(symbol)) {
      Lexer::token_data_accumulator_ += symbol;
      return Lexer::ReadAlphanumeric();
    }

    if (IsOther(symbol)) {
      Lexer::token_data_accumulator_ += symbol;
      return ReadOtherToken();
    }

    if (symbol == '\'' || symbol == '\"') {
      return Lexer::ReadShortLiteralString(symbol);
    }

    if (symbol >= '0' && symbol <= '9') {
      Lexer::token_data_accumulator_ += symbol;
      return ReadNumericConstant();
    }

    throw LexerErr(LexerErrKind::kInvalidSymbolMet);
  }

  // EOF.
  return {.token_kind = TokenKind::kNone};
}

template <typename LexerInputRangeT>
  requires LexerInputRangeTRequirements<LexerInputRangeT>
Token Lexer<LexerInputRangeT>::ReadAlphanumeric() {
  for (; Lexer::iter_ != Lexer::sentinel_; Lexer::iter_++) {
    char symbol = *Lexer::iter_;

    if (IsAlphanumeric(symbol) || symbol == '_') {
      if (Lexer::token_data_accumulator_.length() ==
          Lexer::kMaxOutputAccumLength) {
        throw LexerErr(LexerErrKind::kTooLongToken);
      }

      Lexer::token_data_accumulator_ += symbol;
      continue;
    }

    if (IsSpace(symbol) || IsOther(symbol)) {
      break;
    }

    throw LexerErr(LexerErrKind::kInvalidSymbolMet);
  }

  const auto match_result = [this] {
    int vertex_idx = 0;

    for (const char symbol : Lexer::token_data_accumulator_) {
      if (symbol < 'a' || symbol > 'z') {
        return TokenKind::kNone;
      }

      vertex_idx = kKeywordsTrie.at(vertex_idx).next.at(symbol - 'a');

      if (vertex_idx == -1) {
        return TokenKind::kNone;
      }
    }

    return kKeywordsTrie.at(vertex_idx).output;
  }();

  if (match_result != TokenKind::kNone) {
    switch (match_result) {
      case TokenKind::kKeywordNil:
        return Token{.token_kind = TokenKind::kKeywordNil,
                     .token_data = {{.inner_value = LualikeValue::NilT{}}}};

      case TokenKind::kKeywordTrue:
        return Token{
            .token_kind = TokenKind::kKeywordTrue,
            .token_data = {{.inner_value = LualikeValue::BoolT{true}}}};

      case TokenKind::kKeywordFalse:
        return Token{
            .token_kind = TokenKind::kKeywordFalse,
            .token_data = {{.inner_value = LualikeValue::BoolT{false}}}};

      default:
        return Token{.token_kind = TokenKind::kKeywordFalse};
    }
  }

  else {
    return Token{.token_kind = TokenKind::kName,
                 .token_data = {{LualikeValue::StringT{
                     std::move(Lexer::token_data_accumulator_)}}}};
  }
}

template <typename LexerInputRangeT>
  requires LexerInputRangeTRequirements<LexerInputRangeT>
Token Lexer<LexerInputRangeT>::ReadOtherToken() {
  const auto try_match = [this] {
    auto vertex_idx = 0;

    for (const char symbol : Lexer::token_data_accumulator_) {
      vertex_idx =
          kOtherTokensTrie.at(vertex_idx).next.at(OtherTokensHashFunc(symbol));

      if (vertex_idx == -1) {
        return token::TokenKind::kNone;
      }
    }

    return kOtherTokensTrie.at(vertex_idx).output;
  };

  for (; Lexer::iter_ != Lexer::sentinel_; Lexer::iter_++) {
    char symbol = *Lexer::iter_;

    if (IsOther(symbol)) {
      if (Lexer::token_data_accumulator_.length() == kOtherTokensMaxLength) {
        throw LexerErr(LexerErrKind::kTooLongToken);
      }

      if (Lexer::token_data_accumulator_ == "-" && symbol == '-') {
        return Lexer::ConsumeComment();
      }

      // Match current output
      TokenKind first_match_result = try_match();

      Lexer::token_data_accumulator_ += symbol;

      TokenKind second_match_result = try_match();

      if (first_match_result != TokenKind::kNone &&
          second_match_result == TokenKind::kNone) {
        return {.token_kind = first_match_result};
      }

      continue;
    }

    if (IsSpace(symbol) || IsAlphanumeric(symbol)) {
      break;
    }

    throw LexerErr(LexerErrKind::kInvalidSymbolMet);
  }

  TokenKind match_result = try_match();

  if (match_result != TokenKind::kNone) {
    return {.token_kind = match_result};
  }
}

template <typename LexerInputRangeT>
  requires LexerInputRangeTRequirements<LexerInputRangeT>
Token Lexer<LexerInputRangeT>::ReadShortLiteralString(char delimiter) {
  bool is_escaped = false;

  for (; Lexer::iter_ != Lexer::sentinel_; Lexer::iter_++) {
    char symbol = *Lexer::iter_;

    if (is_escaped) {
      switch (symbol) {
        case 'a':
          Lexer::token_data_accumulator_ += '\a';
          break;

        case 'b':
          Lexer::token_data_accumulator_ += '\b';
          break;

        case 'f':
          Lexer::token_data_accumulator_ += '\f';
          break;

        case 'n':
          Lexer::token_data_accumulator_ += '\n';
          break;

        case 'r':
          Lexer::token_data_accumulator_ += '\r';
          break;

        case 't':
          Lexer::token_data_accumulator_ += '\t';
          break;

        case '\\':
          Lexer::token_data_accumulator_ += '\\';
          break;

        case '\"':
          Lexer::token_data_accumulator_ += '\"';
          break;

        case '\'':
          Lexer::token_data_accumulator_ += '\'';
          break;

        default:
          throw LexerErr(LexerErrKind::kInvalidSymbolMet);
      }

      is_escaped = false;
      continue;
    }

    if (symbol == '\\') {
      is_escaped = true;
      continue;
    }

    if (symbol == '\n') {
      throw LexerErr(LexerErrKind::kUnrecognizedEscapeSequence);
    }

    if (symbol == delimiter) {
      Lexer::iter_++;
      return {.token_kind = TokenKind::kLiteralString,
              .token_data = {{LualikeValue::StringT{
                  std::move(Lexer::token_data_accumulator_)}}}};
    }

    Lexer::token_data_accumulator_ += symbol;
  }

  throw LexerErr(LexerErrKind::kUnclosedStringLiteral);
}

template <typename LexerInputRangeT>
  requires LexerInputRangeTRequirements<LexerInputRangeT>
Token Lexer<LexerInputRangeT>::ReadNumericConstant() {
  bool has_met_fractional_part = false;

  for (; Lexer::iter_ != Lexer::sentinel_; Lexer::iter_++) {
    char symbol = *Lexer::iter_;

    if (IsNumeric(symbol)) {
      Lexer::token_data_accumulator_ += symbol;
      continue;
    }

    if (symbol == '.' || symbol == ',') {
      if (has_met_fractional_part) {
        throw LexerErr(LexerErrKind::kInvalidSymbolMet);
      }

      Lexer::token_data_accumulator_ += symbol;
      has_met_fractional_part = true;
      continue;
    }

    if (IsSpace(symbol) || IsOther(symbol)) {
      const auto try_make_result = [] {

      }();

      if (try_make_result.has_value()) {
        return {.token_kind = TokenKind::kNumericConstant,
                .token_data = try_make_result.value()};
      }
    }

    throw LexerErr(LexerErrKind::kExpectedDigit);
  }
}

// inline LexerState Lexer::ConsumeComment(
//     const char symbol, ConsumeCommentLexerState &state) noexcept {
//   if (symbol == '\n') {
//     return ConsumeWhitespaceLexerState{};
//   }

//   return state;
// }

// Lexer::ReadTokenResult Lexer::ReadToken(std::istream &input_stream)
// noexcept
// {
//   Lexer::state_ = ConsumeWhitespaceLexerState{};

//   while (input_stream) {
//     typedef std::istream::traits_type CharTraits;

//     const auto read_result = input_stream.peek();
//     const auto symbol = (read_result != CharTraits::eof())
//                             ? CharTraits::to_char_type(read_result)
//                             : '\n';

//     const auto processing_visitor = [this, symbol](auto &&state) ->
//     LexerState {
//       using T = std::decay_t<decltype(state)>;

//       if constexpr (std::is_same<T, ConsumeWhitespaceLexerState>()) {
//         return Lexer::ConsumeWhitespace(symbol, state);
//       }

//       else if constexpr (std::is_same<T, ReadAlphanumericLexerState>()) {
//         return Lexer::ReadAlphanumeric(symbol, state);
//       }

//       else if constexpr (std::is_same<T, ReadOtherTokenLexerState>()) {
//         return Lexer::ReadOtherToken(symbol, state);
//       }

//       else if constexpr (std::is_same<T,
//       ReadShortLiteralStringLexerState>())
//       {
//         return Lexer::ReadShortLiteralString(symbol, state);
//       }

//       else if constexpr (std::is_same<T, ReadNumericConstantLexerState>())
//       {
//         return Lexer::ReadNumericConstant(symbol, state);
//       }

//       else if constexpr (std::is_same<T, ConsumeCommentLexerState>()) {
//         return Lexer::ConsumeComment(symbol, state);
//       }

//       else if constexpr (std::is_same<T, std::monostate>() ||
//                          std::is_same<T, ReturnTokenLexerState>() ||
//                          std::is_same<T, ReturnErrorLexerState>()) {
//         // TODO(shvrma): add proper handling
//         return state;
//       }

//       else {
//         static_assert(false,
//                       "Seems that some state doesnt have a corresponding "
//                       "handling branch");
//       }
//     };

//     Lexer::state_ = std::visit(processing_visitor, Lexer::state_);

//     if (const auto exiting_state = std::visit(
//             [&input_stream](auto &&state) -> std::optional<ReadTokenResult>
//             {
//               using T = std::decay_t<decltype(state)>;

//               if constexpr (std::is_same<T, ReturnTokenLexerState>()) {
//                 if (state.should_consume_current) {
//                   input_stream.get();
//                 }

//                 return state.token;
//               }

//               else if constexpr (std::is_same<T, ReturnErrorLexerState>())
//               {
//                 return state.error;
//               }

//               return std::nullopt;
//             },
//             Lexer::state_)) {
//       return exiting_state.value();
//     }

//     if (input_stream.eof()) {
//       return LexerErr::kEOF;
//     }

//     input_stream.get();
//   }

//   return LexerErr::kInputNotOk;
// }

}  // namespace lualike::lexer
