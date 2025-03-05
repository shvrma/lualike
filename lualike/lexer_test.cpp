#include <gtest/gtest.h>

#include <generator>
#include <ranges>
#include <string_view>
#include <vector>

import lualike.lexer;

namespace token = lualike::token;
using token::Token;
using token::TokenKind;

using namespace std::string_literals;

namespace lualike::lexer {

using LexerTestParamT = std::pair<std::string_view, std::vector<Token>>;

class LexerTest : public testing::TestWithParam<LexerTestParamT> {};

TEST_P(LexerTest, ReadAndCompareWithGiven) {
  const auto& [input, expected_sequence] = LexerTest::GetParam();

  try {
    const auto actual_sequence = lexer::ReadTokens(std::string_view{input}) |
                                 std::ranges::to<std::vector<Token>>();
    ASSERT_EQ(actual_sequence, expected_sequence)
        << "Actualy read: "
        << (actual_sequence | std::views::transform([](const auto& token) {
              return token.ToString();
            }) |
            std::views::join_with(std::string{"; "}) |
            std::ranges::to<std::string>());
  }

  catch (LexerErr& err) {
    FAIL() << "Failed with err: " << static_cast<int>(err.error_kind);
  }
}

const LexerTestParamT kSingleKeyword{"local", {{TokenKind::kKeywordLocal}}};

const LexerTestParamT kMultipleKeywords{"local function and",
                                        {
                                            {TokenKind::kKeywordLocal},
                                            {TokenKind::kKeywordFunction},
                                            {TokenKind::kKeywordAnd},
                                        }};

const LexerTestParamT kOtherToken{"local empty_var = nil",
                                  {
                                      {TokenKind::kKeywordLocal},
                                      {TokenKind::kName, "empty_var"s},
                                      {TokenKind::kOtherEqual},
                                      {TokenKind::kKeywordNil},
                                  }};

const LexerTestParamT kLiteral{
    "local fib_num_10 = 34",
    {{TokenKind::kKeywordLocal},
     {TokenKind::kName, "fib_num_10"},
     {TokenKind::kOtherEqual},
     {TokenKind::kLiteral, value::LualikeValue{34}}}};

const LexerTestParamT kConsecutiveOtherTokens{
    "local function empty_func() end",
    {
        {TokenKind::kKeywordLocal},
        {TokenKind::kKeywordFunction},
        {TokenKind::kName, "empty_func"},
        {TokenKind::kOtherLeftParenthesis},
        {TokenKind::kOtherRightParenthesis},
        {TokenKind::kKeywordEnd},
    }};

const LexerTestParamT kStatementWithComment{
    "-- Very meaningfull variable\n\r"
    "local curr_year = \'2025\'",
    {{TokenKind::kKeywordLocal},
     {TokenKind::kName, "curr_year"},
     {TokenKind::kOtherEqual},
     {TokenKind::kLiteral, value::LualikeValue{"2025"}}}};

const LexerTestParamT kSingleNum{
    "1", {{TokenKind::kLiteral, value::LualikeValue{1}}}};

INSTANTIATE_TEST_SUITE_P(TestValidInputs, LexerTest,
                         testing::Values(kSingleKeyword, kMultipleKeywords,
                                         kOtherToken, kLiteral,
                                         kConsecutiveOtherTokens,
                                         kStatementWithComment, kSingleNum));

}  // namespace lualike::lexer
