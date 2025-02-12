#include "lexer.h"

#include <algorithm>
#include <istream>
#include <iterator>
#include <ranges>

#include "token.h"

namespace lualike::lexer {

using token::Token;
using token::TokenKind;

using value::LuaValue;

namespace {

template <auto next_array_size>
struct Vertex {
  std::array<int, next_array_size> next;
  TokenKind output = TokenKind::kNone;

  constexpr Vertex() noexcept { std::ranges::fill(Vertex::next, -1); }
};

constexpr auto kKeywordsTrie = []() consteval {
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

constexpr int kOtherTokensMaxLength = std::ranges::max(
    token::kOtherTokensMap | std::views::keys |
    std::views::transform([](const auto str) { return str.length(); }));

constexpr auto kOtherTokensAlphabet = []() consteval {
  constexpr auto all_symbols =
      token::kOtherTokensMap | std::views::keys | std::views::join;

  constexpr auto calculate = [all_symbols](auto alphabet) {
    std::input_or_output_iterator auto iter = alphabet.begin();

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
constexpr int kModuloDivisitorOfHashFunction = 37;

constexpr int OtherTokensHashFunc(const char symbol) noexcept {
  return symbol % kModuloDivisitorOfHashFunction;
}

constexpr auto kOtherTokensTrie = []() consteval {
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

inline bool IsSpace(const char symbol) noexcept {
  return symbol == ' ' || symbol == '\f' || symbol == '\n' || symbol == '\r' ||
         symbol == '\t' || symbol == '\v';
}

inline bool IsAlphabetic(const char symbol) noexcept {
  return (symbol >= 'a' && symbol <= 'z') || (symbol >= 'A' && symbol <= 'Z');
}

inline bool IsNumeric(const char symbol) noexcept {
  return (symbol >= '0' && symbol <= '9');
}

inline bool IsAlphanumeric(const char symbol) noexcept {
  return IsAlphabetic(symbol) || IsNumeric(symbol);
}

inline bool IsOther(const char symbol) noexcept {
  return std::ranges::find(kOtherTokensAlphabet, symbol) !=
         std::ranges::end(kOtherTokensAlphabet);
}

}  // namespace

LexerState Lexer::ConsumeWhitespace(
    const char symbol, ConsumeWhitespaceLexerState &state) noexcept {
  if (IsSpace(symbol)) {
    return state;
  }

  Lexer::output_accumulator_ = {};

  if (symbol == '_' || IsAlphabetic(symbol)) {
    Lexer::output_accumulator_ += symbol;
    return ReadAlphanumericLexerState{};
  }

  if (IsOther(symbol)) {
    Lexer::output_accumulator_ += symbol;
    return ReadOtherTokenLexerState{};
  }

  if (symbol == '\'' || symbol == '\"') {
    return ReadShortLiteralStringLexerState{
        .delimiter = symbol,
    };
  }

  if (symbol >= '0' && symbol <= '9') {
    Lexer::output_accumulator_ += symbol;
    return ReadNumericConstantLexerState{};
  }

  return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
}

LexerState Lexer::ReadAlphanumeric(const char symbol,
                                   ReadAlphanumericLexerState &state) noexcept {
  if (IsAlphanumeric(symbol) || symbol == '_') {
    if (Lexer::output_accumulator_.length() == Lexer::kMaxOutputAccumLength) {
      return ReturnErrorLexerState{.error = LexerErr::kTooLongToken};
    }

    Lexer::output_accumulator_ += symbol;
    return state;
  }

  if (IsSpace(symbol) || IsOther(symbol)) {
    const auto match_result = [&state, this] {
      int vertex_idx = 0;

      for (const char symbol : Lexer::output_accumulator_) {
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

    return ReturnTokenLexerState{
        .token = (match_result != TokenKind::kNone)
                     ? Token{.token_kind = match_result}
                     : Token{.token_kind = TokenKind::kName,
                             .token_data = LuaValue::MakeLuaString(
                                 std::move(Lexer::output_accumulator_))}};
  }

  return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
}

LexerState Lexer::ReadOtherToken(const char symbol,
                                 ReadOtherTokenLexerState &state) noexcept {
  const auto try_match = [&state, this] {
    auto vertex_idx = 0;

    for (const char symbol : Lexer::output_accumulator_) {
      vertex_idx =
          kOtherTokensTrie.at(vertex_idx).next.at(OtherTokensHashFunc(symbol));

      if (vertex_idx == -1) {
        return token::TokenKind::kNone;
      }
    }

    return kOtherTokensTrie.at(vertex_idx).output;
  };

  if (IsOther(symbol)) {
    if (Lexer::output_accumulator_.length() == kOtherTokensMaxLength) {
      return ReturnErrorLexerState{.error = LexerErr::kTooLongToken};
    }

    if (Lexer::output_accumulator_ == "-" && symbol == '-') {
      return ConsumeCommentLexerState{};
    }

    // Match current output
    TokenKind first_match_result = try_match();

    Lexer::output_accumulator_ += symbol;

    TokenKind second_match_result = try_match();

    if (first_match_result != TokenKind::kNone &&
        second_match_result == TokenKind::kNone) {
      return ReturnTokenLexerState{.token{.token_kind = first_match_result}};
    }

    return state;
  }

  if (IsSpace(symbol) || IsAlphanumeric(symbol)) {
    TokenKind match_result = try_match();

    if (match_result != TokenKind::kNone) {
      return ReturnTokenLexerState{.token{.token_kind = match_result}};
    }
  }

  return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
}

inline LexerState Lexer::ReadShortLiteralString(
    const char symbol, ReadShortLiteralStringLexerState &state) noexcept {
  if (state.is_escaped) {
    switch (symbol) {
      case 'a':
        Lexer::output_accumulator_ += '\a';
        break;

      case 'b':
        Lexer::output_accumulator_ += '\b';
        break;

      case 'f':
        Lexer::output_accumulator_ += '\f';
        break;

      case 'n':
        Lexer::output_accumulator_ += '\n';
        break;

      case 'r':
        Lexer::output_accumulator_ += '\r';
        break;

      case 't':
        Lexer::output_accumulator_ += '\t';
        break;

      case '\\':
        Lexer::output_accumulator_ += '\\';
        break;

      case '\"':
        Lexer::output_accumulator_ += '\"';
        break;

      case '\'':
        Lexer::output_accumulator_ += '\'';
        break;

      default:
        return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
    }

    state.is_escaped = false;
    return state;
  }

  if (symbol == '\\') {
    state.is_escaped = true;
    return state;
  }

  // Unescaped newline
  if (symbol == '\n') {
    return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
  }

  if (symbol == state.delimiter) {
    if (state.is_escaped) {
      return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
    }

    return ReturnTokenLexerState{
        .token{.token_kind = TokenKind::kLiteralString,
               .token_data = LuaValue::MakeLuaString(
                   std::move(Lexer::output_accumulator_))},
        .should_consume_current = true};
  }

  Lexer::output_accumulator_ += symbol;
  return state;
}

inline LexerState Lexer::ReadNumericConstant(
    const char symbol, ReadNumericConstantLexerState &state) noexcept {
  if (IsNumeric(symbol)) {
    Lexer::output_accumulator_ += symbol;
    return state;
  }

  if (symbol == '.' || symbol == ',') {
    if (state.has_met_fractional_part) {
      return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
    }

    Lexer::output_accumulator_ += symbol;
    state.has_met_fractional_part = true;
    return state;
  }

  if (IsSpace(symbol) || IsOther(symbol)) {
    const auto try_make_result =
        (state.has_met_fractional_part)
            ? LuaValue::TryMakeLuaFloat(Lexer::output_accumulator_)
            : LuaValue::TryMakeLuaInteger(Lexer::output_accumulator_);

    if (std::holds_alternative<LuaValue>(try_make_result)) {
      return ReturnTokenLexerState{
          .token{.token_kind = TokenKind::kNumericConstant,
                 .token_data = std::get<LuaValue>(try_make_result)}};
    }
  }

  return ReturnErrorLexerState{.error = LexerErr::kInvalidSymbolMet};
}

inline LexerState Lexer::ConsumeComment(
    const char symbol, ConsumeCommentLexerState &state) noexcept {
  if (symbol == '\n') {
    return ConsumeWhitespaceLexerState{};
  }

  return state;
}

Lexer::ReadTokenResult Lexer::ReadToken(std::istream &input_stream) noexcept {
  Lexer::state_ = ConsumeWhitespaceLexerState{};

  while (input_stream) {
    typedef std::istream::traits_type CharTraits;

    const auto read_result = input_stream.peek();
    const auto symbol = (read_result != CharTraits::eof())
                            ? CharTraits::to_char_type(read_result)
                            : '\n';

    const auto processing_visitor = [this, symbol](auto &&state) -> LexerState {
      using T = std::decay_t<decltype(state)>;

      if constexpr (std::is_same<T, ConsumeWhitespaceLexerState>()) {
        return Lexer::ConsumeWhitespace(symbol, state);
      }

      else if constexpr (std::is_same<T, ReadAlphanumericLexerState>()) {
        return Lexer::ReadAlphanumeric(symbol, state);
      }

      else if constexpr (std::is_same<T, ReadOtherTokenLexerState>()) {
        return Lexer::ReadOtherToken(symbol, state);
      }

      else if constexpr (std::is_same<T, ReadShortLiteralStringLexerState>()) {
        return Lexer::ReadShortLiteralString(symbol, state);
      }

      else if constexpr (std::is_same<T, ReadNumericConstantLexerState>()) {
        return Lexer::ReadNumericConstant(symbol, state);
      }

      else if constexpr (std::is_same<T, ConsumeCommentLexerState>()) {
        return Lexer::ConsumeComment(symbol, state);
      }

      else if constexpr (std::is_same<T, std::monostate>() ||
                         std::is_same<T, ReturnTokenLexerState>() ||
                         std::is_same<T, ReturnErrorLexerState>()) {
        // TODO(shvrma): add proper handling
        return state;
      }

      else {
        static_assert(false,
                      "Seems that some state doesnt have a corresponding "
                      "handling branch");
      }
    };

    Lexer::state_ = std::visit(processing_visitor, Lexer::state_);

    if (const auto exiting_state = std::visit(
            [&input_stream](auto &&state) -> std::optional<ReadTokenResult> {
              using T = std::decay_t<decltype(state)>;

              if constexpr (std::is_same<T, ReturnTokenLexerState>()) {
                if (state.should_consume_current) {
                  input_stream.get();
                }

                return state.token;
              }

              else if constexpr (std::is_same<T, ReturnErrorLexerState>()) {
                return state.error;
              }

              return std::nullopt;
            },
            Lexer::state_)) {
      return exiting_state.value();
    }

    if (input_stream.eof()) {
      return LexerErr::kEOF;
    }

    input_stream.get();
  }

  return LexerErr::kInputNotOk;
}

}  // namespace lualike::lexer
