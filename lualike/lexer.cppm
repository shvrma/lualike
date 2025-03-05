module;

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <generator>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <system_error>
#include <vector>

export module lualike.lexer;

export import lualike.token;

namespace token = lualike::token;
namespace value = lualike::value;

export namespace lualike::lexer {

enum class LexerErrKind : uint8_t {
  kEOF,
  kInvalidSymbolMet,
  kTooLongToken,
  kUnclosedStringLiteral,
  kUnrecognizedEscapeSequence,
  kInvalidNumber,
  kInvalidName,
};

struct LexerErr : std::exception {
  LexerErrKind error_kind;

  explicit LexerErr(LexerErrKind error_kind) noexcept;
};

template <typename InputT>
concept InputTRequirements =
    std::ranges::input_range<InputT> &&
    std::same_as<char, std::ranges::range_value_t<InputT>>;

template <typename InputT>
  requires InputTRequirements<InputT>
class Lexer {
  std::ranges::const_iterator_t<InputT> iter_;
  std::ranges::const_sentinel_t<InputT> sentinel_;

  std::string token_data_accumulator_;

  void ConsumeComment();

  // Pre: single alphabetic symbol or undescore in accumulator.
  token::Token ReadAlphanumeric();
  // Pre: single symbol from alphabet of other tokens -
  // token::token::kOtherTokensAlphabet - in accumalator.
  token::Token ReadOtherToken();
  // Pre: accumulator is empty and delimiter is got
  token::Token ReadShortLiteralString(char delimiter);
  // Pre: single digit is in accumulator.
  token::Token ReadNumericConstant();

 public:
  explicit Lexer(InputT&& input)
      : iter_(std::ranges::cbegin(input)),
        sentinel_(std::ranges::cend(input)) {}

  // Iterates the contained ranged of symbols till syntatically valid token is
  // found, then returning it.
  // Supposedly, the input should be valid. Lexer recognizes errors, but in the
  // case of recognition, a LexerErr is thrown.
  std::optional<token::Token> NextToken();
};

template <typename InputT>
  requires InputTRequirements<InputT>
std::generator<const token::Token&> ReadTokens(InputT&& input);

}  // namespace lualike::lexer

namespace lualike::lexer {

constexpr int kMaxOutputAccumLength = 16;

using VertexIdxT = int8_t;

template <auto next_array_size>
struct Vertex {
  std::array<VertexIdxT, next_array_size> next;
  token::TokenKind output = token::TokenKind::kNone;

  constexpr Vertex() noexcept { std::ranges::fill(Vertex::next, -1); }
};

const auto kKeywordsTrie = [] {
  constexpr int kAlphabetSize = 26;
  std::vector<Vertex<kAlphabetSize>> trie{};
  trie.push_back({});

  for (const auto& [word, token_kind] : token::kKeywordsMap) {
    VertexIdxT current_node = 0;

    for (const char symbol : word) {
      const VertexIdxT idx = static_cast<int8_t>(symbol - 'a');

      if (trie.at(current_node).next.at(idx) == -1) {
        trie.at(current_node).next.at(idx) =
            static_cast<VertexIdxT>(trie.size());
        trie.push_back({});
      }

      current_node = trie.at(current_node).next.at(idx);
    }

    trie.at(current_node).output = token_kind;
  }

  return trie;
}();

const auto kOtherTokensMaxLength = std::ranges::max(
    token::kOtherTokensMap | std::views::keys |
    std::views::transform([](const auto& str) { return str.length(); }));

const auto kOtherTokensAlphabet = [] {
  const auto all_symbols =
      token::kOtherTokensMap | std::views::keys | std::views::join;

  std::vector<char> alphabet{};

  for (const char symbol : all_symbols) {
    if (not std::ranges::contains(alphabet, symbol)) {
      alphabet.push_back(symbol);
    }
  }

  return alphabet;
}();

// Prime numbers list: 11, 13, 17, 19, 23, 29, 31, 37, 41.
constexpr VertexIdxT kModuloDivisitorOfHashFunction = 37;

constexpr VertexIdxT OtherTokensHashFunc(const char symbol) noexcept {
  return static_cast<VertexIdxT>(symbol % kModuloDivisitorOfHashFunction);
}

const auto kOtherTokensTrie = [] {
  // Validate hash function.
  std::vector<VertexIdxT> hashes{};
  std::ranges::copy(kOtherTokensAlphabet |
                        std::views::transform([](const char symbol) constexpr {
                          return OtherTokensHashFunc(symbol);
                        }),
                    std::back_inserter(hashes));

  std::ranges::sort(hashes);
  if (std::ranges::adjacent_find(hashes) != hashes.end()) {
    throw std::invalid_argument("Hash function doesnt produce unique values");
  }

  std::vector<Vertex<kModuloDivisitorOfHashFunction>> trie{};
  trie.push_back({});

  for (const auto& [token, token_kind] : token::kOtherTokensMap) {
    VertexIdxT current_node = 0;

    for (const char symbol : std::string_view{token}) {
      const auto idx = OtherTokensHashFunc(symbol);

      if (trie.at(current_node).next.at(idx) == -1) {
        trie.at(current_node).next.at(idx) =
            static_cast<VertexIdxT>(trie.size());
        trie.push_back({});
      }

      current_node = trie.at(current_node).next.at(idx);
    }

    trie.at(current_node).output = token_kind;
  }

  return trie;
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
  return std::ranges::contains(kOtherTokensAlphabet, symbol);
}

LexerErr::LexerErr(LexerErrKind error_kind) noexcept : error_kind(error_kind) {}

template <typename InputT>
  requires InputTRequirements<InputT>
inline void Lexer<InputT>::ConsumeComment() {
  while (Lexer::iter_ != Lexer::sentinel_) {
    char symbol = *Lexer::iter_;
    Lexer::iter_++;

    if (symbol == '\n') {
      return;
    }
  }
}

template <typename InputT>
  requires InputTRequirements<InputT>
token::Token Lexer<InputT>::ReadAlphanumeric() {
  while (Lexer::iter_ != Lexer::sentinel_) {
    char symbol = *Lexer::iter_;
    Lexer::iter_++;

    if (IsAlphanumeric(symbol) || symbol == '_') {
      if (Lexer::token_data_accumulator_.length() == kMaxOutputAccumLength) {
        throw LexerErr(LexerErrKind::kTooLongToken);
      }

      Lexer::token_data_accumulator_ += symbol;
      continue;
    }

    if (IsSpace(symbol) || IsOther(symbol)) {
      break;
    }

    throw LexerErr(LexerErrKind::kInvalidName);
  }

  const auto match_result = [this] {
    VertexIdxT vertex_idx = 0;

    for (const char symbol : Lexer::token_data_accumulator_) {
      if (symbol < 'a' || symbol > 'z') {
        return token::TokenKind::kNone;
      }

      vertex_idx = kKeywordsTrie.at(vertex_idx).next.at(symbol - 'a');

      if (vertex_idx == -1) {
        return token::TokenKind::kNone;
      }
    }

    return kKeywordsTrie.at(vertex_idx).output;
  }();

  if (match_result != token::TokenKind::kNone) {
    return token::Token{.token_kind = match_result};
  }

  return token::Token{.token_kind = token::TokenKind::kName,
                      .token_data{std::move(Lexer::token_data_accumulator_)}};
}

template <typename InputT>
  requires InputTRequirements<InputT>
token::Token Lexer<InputT>::ReadOtherToken() {
  const auto try_match = [this] {
    VertexIdxT vertex_idx = 0;

    for (const char symbol : Lexer::token_data_accumulator_) {
      vertex_idx =
          kOtherTokensTrie.at(vertex_idx).next.at(OtherTokensHashFunc(symbol));

      if (vertex_idx == -1) {
        return token::TokenKind::kNone;
      }
    }

    return kOtherTokensTrie.at(vertex_idx).output;
  };

  while (Lexer::iter_ != Lexer::sentinel_) {
    char symbol = *Lexer::iter_;
    Lexer::iter_++;

    if (IsOther(symbol)) {
      if (Lexer::token_data_accumulator_.length() == kOtherTokensMaxLength) {
        throw LexerErr(LexerErrKind::kTooLongToken);
      }

      token::TokenKind first_match_result = try_match();

      Lexer::token_data_accumulator_ += symbol;
      token::TokenKind second_match_result = try_match();

      if (first_match_result != token::TokenKind::kNone &&
          second_match_result == token::TokenKind::kNone) {
        return token::Token{.token_kind = first_match_result};
      }

      continue;
    }

    if (IsSpace(symbol) || IsAlphanumeric(symbol)) {
      break;
    }

    throw LexerErr(LexerErrKind::kInvalidSymbolMet);
  }

  token::TokenKind match_result = try_match();

  if (match_result != token::TokenKind::kNone) {
    return token::Token{.token_kind = match_result};
  }

  throw LexerErr(LexerErrKind::kInvalidSymbolMet);
}

template <typename InputT>
  requires InputTRequirements<InputT>
token::Token Lexer<InputT>::ReadShortLiteralString(char delimiter) {
  bool is_escaped = false;

  while (Lexer::iter_ != Lexer::sentinel_) {
    char symbol = *Lexer::iter_;
    Lexer::iter_++;

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
      return {.token_kind = token::TokenKind::kLiteral,
              .token_data{value::LualikeValue{value::LualikeValue::StringT{
                  std::move(Lexer::token_data_accumulator_)}}}};
    }

    Lexer::token_data_accumulator_ += symbol;
  }

  throw LexerErr(LexerErrKind::kUnclosedStringLiteral);
}

template <typename InputT>
  requires InputTRequirements<InputT>
token::Token Lexer<InputT>::ReadNumericConstant() {
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

    throw LexerErr(LexerErrKind::kInvalidNumber);
  }

  const auto make_num = [this](auto value_t) {
    decltype(value_t) num{};

    const auto& str_begin = Lexer::token_data_accumulator_.data();
    const auto& str_size = Lexer::token_data_accumulator_.size();
    auto [ptr, ec] = std::from_chars(str_begin, str_begin + str_size, num);
    if (ec != std::errc() || ptr != str_begin + str_size) {
      throw LexerErr(LexerErrKind::kInvalidNumber);
    }

    return num;
  };

  if (has_met_fractional_part) {
    return {.token_kind = token::TokenKind::kLiteral,
            .token_data{
                value::LualikeValue{make_num(value::LualikeValue::FloatT{})}}};
  }

  return {
      .token_kind = token::TokenKind::kLiteral,
      .token_data{value::LualikeValue{make_num(value::LualikeValue::IntT{})}}};
}

template <typename InputT>
  requires InputTRequirements<InputT>
std::optional<token::Token> Lexer<InputT>::NextToken() {
  while (iter_ != sentinel_ && IsSpace(*iter_)) {
    ++iter_;
  }

  if (iter_ == sentinel_) {
    return std::nullopt;
  }

  token_data_accumulator_.clear();

  char symbol = *iter_;
  ++iter_;

  if (symbol == '\'' || symbol == '\"') {
    return ReadShortLiteralString(symbol);
  }

  if (symbol == '_' || IsAlphabetic(symbol)) {
    token_data_accumulator_ += symbol;
    return ReadAlphanumeric();
  }

  if (IsOther(symbol)) {
    token_data_accumulator_ += symbol;
    return ReadOtherToken();
  }

  if (IsNumeric(symbol)) {
    token_data_accumulator_ += symbol;
    return ReadNumericConstant();
  }

  throw LexerErr(LexerErrKind::kInvalidSymbolMet);
}

template <typename InputT>
  requires InputTRequirements<InputT>
std::generator<const token::Token&> ReadTokens(InputT&& input) {
  Lexer<InputT> lexer(std::forward<InputT>(input));

  while (auto token = lexer.NextToken()) {
    co_yield *token;
  }
}

}  // namespace lualike::lexer
