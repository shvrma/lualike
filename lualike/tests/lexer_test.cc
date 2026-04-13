#include "lualike/lexer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <ranges>
#include <string_view>
#include <vector>

namespace token = lualike::token;

using token::Token;
using token::TokenKind;

namespace lualike::lexer {

MATCHER_P(LualikeSyntaticlyEqualsTo, valid_tokens_seq, "") {
  std::vector<Token> actual_tokens_seq;
  auto input = std::string_view{arg};
  Lexer<std::string_view> lexer(input);
  while (auto token_opt = lexer.NextToken()) {
    actual_tokens_seq.push_back(token_opt.value());
  }

  return std::ranges::equal(
      actual_tokens_seq, valid_tokens_seq,
      [](const Token& lhs, const Token& rhs) {
        return lhs.token_kind == rhs.token_kind &&
               (rhs.source_span.empty() || lhs.source_span == rhs.source_span);
      });
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
                  {TokenKind::kIntLiteral, "34"}}));

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
      "-- Very meaningful variable\n"
      "local curr_year = \'2025\'",
      LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
          {TokenKind::kKeywordLocal},
          {TokenKind::kName, "curr_year"},
          {TokenKind::kOtherEqual},
          {TokenKind::kStringLiteral, "\'2025\'"}}));

  EXPECT_THAT("1", LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                       {TokenKind::kIntLiteral, "1"}}));

  EXPECT_THAT("1 + 2 * -3",
              LualikeSyntaticlyEqualsTo(std::initializer_list<Token>{
                  {TokenKind::kIntLiteral, "1"},
                  {TokenKind::kOtherPlus},
                  {TokenKind::kIntLiteral, "2"},
                  {TokenKind::kOtherAsterisk},
                  {TokenKind::kOtherMinus},
                  {TokenKind::kIntLiteral, "3"},
              }));
}

}  // namespace lualike::lexer
