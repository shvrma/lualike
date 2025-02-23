module;

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <generator>
#include <ranges>
#include <utility>

export module lualike.lexer;

export import lualike.token;

namespace lualike::lexer {

namespace token = lualike::token;
using token::Token;
using token::TokenKind;

using lualike::value::LualikeValue;

export enum class LexerErrKind : uint8_t {
  kEOF,
  kInvalidSymbolMet,
  kTooLongToken,
  kUnclosedStringLiteral,
  kUnrecognizedEscapeSequence,

  kExpectedDigit,
  kExpectedAlphanumericOrUnderScore,
};

export struct LexerErr : std::exception {
  LexerErrKind error_kind;

  explicit LexerErr(LexerErrKind error_kind) noexcept;
};

export template <typename InputT>
concept InputTRequirements =
    std::ranges::input_range<InputT> && std::ranges::view<InputT> &&
    std::same_as<char, std::ranges::range_value_t<InputT>>;

export template <typename InputT>
  requires InputTRequirements<InputT>
class Lexer {
  std::ranges::const_iterator_t<InputT> iter_;
  std::ranges::const_sentinel_t<InputT> sentinel_;

  std::string token_data_accumulator_;
  static constexpr int kMaxOutputAccumLength = 16;

  explicit Lexer(const InputT &&input) noexcept;

  void ConsumeComment();

  // Pre: single alphabetic symbol or undescore in accumulator.
  Token ReadAlphanumeric();
  // Pre: single symbol from alphabet of other tokens -
  // token::kOtherTokensAlphabet - in accumalator.
  Token ReadOtherToken();
  // Pre: accumulator is empty and delimiter is got
  Token ReadShortLiteralString(char delimiter);
  // Pre: single digit is in accumulator.
  Token ReadNumericConstant();

  std::generator<const Token &> ReadTokens();

 public:
  // Iterates the contained ranged of symbols till syntatically valid token is
  // found, then returning it.
  // Supposedly, the input should be valid. Lexer recognizes errors, but in the
  // case of recognition, a LexerErr is thrown.
  static auto Tokenize(const InputT &&input)
      -> std::ranges::owning_view<std::generator<const Token &>>;
};

namespace {

template <auto next_array_size>
struct Vertex {
  std::array<int, next_array_size> next;
  TokenKind output = TokenKind::kNone;

  constexpr Vertex() noexcept { std::ranges::fill(Vertex::next, -1); }
};

}  // namespace

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
constexpr int kModuloDivisitorOfHashFunction = 37;

constexpr int OtherTokensHashFunc(const char symbol) noexcept {
  return static_cast<int>(symbol) % kModuloDivisitorOfHashFunction;
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

LexerErr::LexerErr(LexerErrKind error_kind) noexcept : error_kind(error_kind) {}

template <typename InputT>
  requires InputTRequirements<InputT>
Lexer<InputT>::Lexer(const InputT &&input) noexcept
    : iter_(std::ranges::cbegin(input)), sentinel_(std::ranges::cend(input)) {}

template <typename InputT>
  requires InputTRequirements<InputT>
Token Lexer<InputT>::ReadAlphanumeric() {
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

    throw LexerErr(LexerErrKind::kExpectedAlphanumericOrUnderScore);
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
        return Token{.token_kind = match_result};
    }
  }

  return Token{.token_kind = TokenKind::kName,
               .token_data = {{LualikeValue::StringT{
                   std::move(Lexer::token_data_accumulator_)}}}};
}

template <typename InputT>
  requires InputTRequirements<InputT>
Token Lexer<InputT>::ReadOtherToken() {
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

      TokenKind first_match_result = try_match();

      Lexer::token_data_accumulator_ += symbol;
      TokenKind second_match_result = try_match();

      if (first_match_result != TokenKind::kNone &&
          second_match_result == TokenKind::kNone) {
        return Token{.token_kind = first_match_result};
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
    return Token{.token_kind = match_result};
  }

  throw LexerErr(LexerErrKind::kInvalidSymbolMet);
}

template <typename InputT>
  requires InputTRequirements<InputT>
Token Lexer<InputT>::ReadShortLiteralString(char delimiter) {
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
      return {.token_kind = TokenKind::kLiteral,
              .token_data = {{LualikeValue::StringT{
                  std::move(Lexer::token_data_accumulator_)}}}};
    }

    Lexer::token_data_accumulator_ += symbol;
  }

  throw LexerErr(LexerErrKind::kUnclosedStringLiteral);
}

template <typename InputT>
  requires InputTRequirements<InputT>
Token Lexer<InputT>::ReadNumericConstant() {
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
      break;
    }

    throw LexerErr(LexerErrKind::kExpectedDigit);
  }

  const auto try_make = [this](auto type_tag) {
    decltype(type_tag) num{};

    const auto conv_result =
        std::from_chars(Lexer::token_data_accumulator_.data(),
                        Lexer::token_data_accumulator_.data() +
                            Lexer::token_data_accumulator_.size(),
                        num);

    if (conv_result.ec == std::errc::result_out_of_range) {
      throw LexerErr(LexerErrKind::kTooLongToken);
    }

    if (conv_result.ec != std::errc{}) {
      throw LexerErr(LexerErrKind::kInvalidSymbolMet);
    }

    return num;
  };

  if (has_met_fractional_part) {
    return {.token_kind = TokenKind::kLiteral,
            .token_data = {{try_make(LualikeValue::FloatT{})}}};
  }

  return {.token_kind = TokenKind::kLiteral,
          .token_data = {{try_make(LualikeValue::IntT{})}}};
}

template <typename InputT>
  requires InputTRequirements<InputT>
void Lexer<InputT>::ConsumeComment() {
  for (; Lexer::iter_ != Lexer::sentinel_; Lexer::iter_++) {
    char symbol = *Lexer::iter_;

    if (symbol == '\n') {
      return;
    }
  }
}

template <typename InputT>
  requires InputTRequirements<InputT>
std::generator<const Token &> Lexer<InputT>::ReadTokens() {
  while (Lexer::iter_ != Lexer::sentinel_) {
    char symbol = *Lexer::iter_;
    Lexer::iter_++;

    if (IsSpace(symbol)) {
      continue;
    }

    Lexer::token_data_accumulator_ = {};

    if (symbol == '\'' || symbol == '\"') {
      co_yield Lexer::ReadShortLiteralString(symbol);
    }

    else if (symbol == '_' || IsAlphabetic(symbol)) {
      Lexer::token_data_accumulator_ += symbol;
      co_yield Lexer::ReadAlphanumeric();
    }

    else if (IsOther(symbol)) {
      if (symbol == '-' && *Lexer::iter_ == '-') {
        Lexer::ConsumeComment();
        continue;
      }

      Lexer::token_data_accumulator_ += symbol;
      co_yield Lexer::ReadOtherToken();
    }

    else if (IsNumeric(symbol)) {
      Lexer::token_data_accumulator_ += symbol;
      co_yield Lexer::ReadNumericConstant();
    }

    else {
      throw LexerErr(LexerErrKind::kInvalidSymbolMet);
    }
  }

  co_return;
}

template <typename InputT>
  requires InputTRequirements<InputT>
auto Lexer<InputT>::Tokenize(const InputT &&input)
    -> std::ranges::owning_view<std::generator<const Token &>> {
  Lexer<InputT> lexer(std::move(input));
  return {lexer.ReadTokens()};
}

}  // namespace lualike::lexer
