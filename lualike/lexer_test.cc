#include "lexer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "token.h"
#include "value.h"

namespace lualike::lexer {

using lexer::Lexer;

using token::Token;
using token::TokenKind;

using value::operator""_lua_int;
using value::operator""_lua_float;
using value::operator""_lua_str;

typedef std::pair<std::string, std::vector<Lexer::ReadTokenResult>>
    LexerTestParamT;

class LexerTest : public testing::TestWithParam<LexerTestParamT> {};

TEST_P(LexerTest, ReadAndCompareWithGiven) {
  const auto &[input, expected_result_sequence] = LexerTest::GetParam();

  std::istringstream prog{input};
  Lexer lexer{};

  for (const Lexer::ReadTokenResult &expected_result :
       expected_result_sequence) {
    const auto read_result = lexer.ReadToken(prog);
    EXPECT_EQ(read_result, expected_result);
  }

  ASSERT_THAT(lexer.ReadToken(prog),
              testing::VariantWith<LexerErr>(LexerErr::kEOF));
}

INSTANTIATE_TEST_SUITE_P(
    TestValidInputs, LexerTest,
    testing::ValuesIn(std::to_array<LexerTestParamT>({
        {"local", {Token{TokenKind::kKeywordLocal}}},

        {"local function and",
         {
             Token{TokenKind::kKeywordLocal},
             Token{TokenKind::kKeywordFunction},
             Token{TokenKind::kKeywordAnd},
         }},

        {"local empty_var = nil",
         {
             Token{TokenKind::kKeywordLocal},
             Token{TokenKind::kName, "empty_var"_lua_str},
             Token{TokenKind::kOtherEqual},
             Token{TokenKind::kKeywordNil},
         }},

        {"local fib_num_10 = 34",
         {Token{TokenKind::kKeywordLocal},
          Token{TokenKind::kName, "fib_num_10"_lua_str},
          Token{TokenKind::kOtherEqual},
          Token{TokenKind::kNumericConstant, 34_lua_int}}},

        {"local function empty_func() end",
         {
             Token{TokenKind::kKeywordLocal},
             Token{TokenKind::kKeywordFunction},
             Token{TokenKind::kName, "empty_func"_lua_str},
             Token{TokenKind::kOtherLeftParenthesis},
             Token{TokenKind::kOtherRightParenthesis},
             Token{TokenKind::kKeywordEnd},
         }},

        {"-- Very meaningfull variable\n\r"
         "local curr_year = \'2025\'",
         {Token{TokenKind::kKeywordLocal},
          Token{TokenKind::kName, "curr_year"_lua_str},
          Token{TokenKind::kOtherEqual},
          Token{TokenKind::kLiteralString, "2025"_lua_str}}},

        {"pi = 3.14159 -- Pi number actualy",
         {Token{TokenKind::kName, "pi"_lua_str}, Token{TokenKind::kOtherEqual},
          Token{TokenKind::kNumericConstant, 3.14159_lua_float}}},
    })));

// INSTANTIATE_TEST_SUITE_P(TestInvalidInputs, LexerTest,
//                          testing::ValuesIn(std::to_array<LexerTestParamT>({
//                              {"3.14.159", {LexerErr::kInvalidSymbolMet}},
//                          })));

}  // namespace lualike::lexer
