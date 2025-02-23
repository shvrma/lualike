#include <gtest/gtest.h>

#include <algorithm>
#include <ranges>
#include <string_view>
#include <vector>

import lualike.lexer;

namespace token = lualike::token;
using token::Token;
using token::TokenKind;

namespace value = lualike::value;
using value::LualikeValue;
using value::operator""_lua_int;
using value::operator""_lua_float;
using value::operator""_lua_str;

namespace lualike::token {

void PrintTo(const token::Token& token, std::ostream* output_stream) {
  *output_stream << token.ToString();
}

}  // namespace lualike::token

namespace lualike::lexer {

using LexerTestParamT = std::pair<std::string_view, std::vector<Token>>;

class LexerTest : public testing::TestWithParam<LexerTestParamT> {};

TEST_P(LexerTest, ReadAndCompareWithGiven) {
  const auto& [input, expected_sequence] = LexerTest::GetParam();

  std::vector<Token> actual_sequence{};
  try {
    std::ranges::copy(
        Lexer<std::string_view>::Tokenize(std::string_view{input}),
        std::back_inserter(actual_sequence));
  } catch (LexerErr& err) {
    FAIL() << "Failed with err: " << static_cast<int>(err.error_kind);
  }

  const auto actual_stringified =
      actual_sequence | std::views::transform([](const auto& token) {
        return token.ToString();
      }) |
      std::views::join_with(';') | std::ranges::to<std::string>();
  ASSERT_TRUE(std::ranges::equal(actual_sequence, expected_sequence))
      << "Actual readed: " << actual_stringified;
}

const LexerTestParamT kSingleKeyword{"local",
                                     {Token{TokenKind::kKeywordLocal}}};

const LexerTestParamT kMultipleKeywords{"local function and",
                                        {
                                            Token{TokenKind::kKeywordLocal},
                                            Token{TokenKind::kKeywordFunction},
                                            Token{TokenKind::kKeywordAnd},
                                        }};

const LexerTestParamT kMultipleTokens{
    "local empty_var = nil",
    {
        Token{TokenKind::kKeywordLocal},
        Token{TokenKind::kName, "empty_var"_lua_str},
        Token{TokenKind::kOtherEqual},
        Token{TokenKind::kKeywordNil, LualikeValue(LualikeValue::NilT{})},
    }};

const LexerTestParamT kLiteralToken{
    "local fib_num_10 = 34",
    {Token{TokenKind::kKeywordLocal},
     Token{TokenKind::kName, "fib_num_10"_lua_str},
     Token{TokenKind::kOtherEqual}, Token{TokenKind::kLiteral, 34_lua_int}}};

const LexerTestParamT kConsecutiveOtherTokens{
    "local function empty_func() end",
    {
        Token{TokenKind::kKeywordLocal},
        Token{TokenKind::kKeywordFunction},
        Token{TokenKind::kName, "empty_func"_lua_str},
        Token{TokenKind::kOtherLeftParenthesis},
        Token{TokenKind::kOtherRightParenthesis},
        Token{TokenKind::kKeywordEnd},
    }};

const LexerTestParamT kStatementWithComment{
    "-- Very meaningfull variable\n\r"
    "local curr_year = \'2025\'",
    {Token{TokenKind::kKeywordLocal},
     Token{TokenKind::kName, "curr_year"_lua_str},
     Token{TokenKind::kOtherEqual},
     Token{TokenKind::kLiteral, "2025"_lua_str}}};

INSTANTIATE_TEST_SUITE_P(TestValidInputs, LexerTest,
                         testing::Values(kSingleKeyword, kMultipleKeywords,
                                         kMultipleTokens, kLiteralToken,
                                         kConsecutiveOtherTokens,
                                         kStatementWithComment));

}  // namespace lualike::lexer
