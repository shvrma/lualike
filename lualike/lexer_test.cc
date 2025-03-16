#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <generator>
#include <ranges>
#include <string_view>
#include <vector>

import lualike.lexer;

namespace token = lualike::token;
using token::Token;
using token::TokenKind;

namespace lualike::lexer {

MATCHER_P(LualikeSyntaticlyEqualsTo, valid_tokens_seq, "") {
  const auto actual_tokens_seq = lexer::ReadTokens(std::string_view{arg}) |
                                 std::ranges::to<std::vector<Token>>();

  return std::ranges::equal(actual_tokens_seq, valid_tokens_seq);
}

TEST(LexerTest, ReadAndCompareWithValidSeqTest) {
  EXPECT_THAT("local", LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                           {TokenKind::kKeywordLocal}}));

  EXPECT_THAT("local function and",
              LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                  {TokenKind::kKeywordLocal},
                  {TokenKind::kKeywordFunction},
                  {TokenKind::kKeywordAnd},
              }));

  EXPECT_THAT("local empty_var = nil",
              LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                  {TokenKind::kKeywordLocal},
                  {TokenKind::kName, "empty_var"},
                  {TokenKind::kOtherEqual},
                  {TokenKind::kKeywordNil},
              }));

  EXPECT_THAT("local fib_num_10 = 34",
              LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                  {TokenKind::kKeywordLocal},
                  {TokenKind::kName, "fib_num_10"},
                  {TokenKind::kOtherEqual},
                  {TokenKind::kLiteral, value::LualikeValue{34}}}));

  EXPECT_THAT("local function empty_func() end",
              LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                  {TokenKind::kKeywordLocal},
                  {TokenKind::kKeywordFunction},
                  {TokenKind::kName, "empty_func"},
                  {TokenKind::kOtherLeftParenthesis},
                  {TokenKind::kOtherRightParenthesis},
                  {TokenKind::kKeywordEnd},
              }));

  EXPECT_THAT(
      "-- Very meaningfull variable\n"
      "local curr_year = \'2025\'",
      LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
          {TokenKind::kKeywordLocal},
          {TokenKind::kName, "curr_year"},
          {TokenKind::kOtherEqual},
          {TokenKind::kLiteral, value::LualikeValue{"2025"}}}));

  EXPECT_THAT("1", LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                       {TokenKind::kLiteral, value::LualikeValue{1}}}));

  EXPECT_THAT("1 + 2 * -3",
              LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                  {TokenKind::kLiteral, value::LualikeValue{1}},
                  {TokenKind::kOtherPlus},
                  {TokenKind::kLiteral, value::LualikeValue{2}},
                  {TokenKind::kOtherAsterisk},
                  {TokenKind::kOtherMinus},
                  {TokenKind::kLiteral, value::LualikeValue{3}},
              }));
}

}  // namespace lualike::lexer
