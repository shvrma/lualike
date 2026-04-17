#include "lualike/parser.h"

#include <sstream>
#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lualike/ast.h"

using lualike::parser::Parse;

namespace {

std::string RenderErrorForTest(const lualike::error::Error& err) {
  return err.HasSourceText() ? err.RenderPretty() : err.RenderPlain();
}

}  // namespace

MATCHER_P(LualikeParsesToAstDump, expected_dump, "") {
  const auto actual_ast = Parse(std::string_view{arg});
  if (!actual_ast.has_value()) {
    *result_listener << RenderErrorForTest(actual_ast.error());
    return false;
  }

  const auto actual_dump = lualike::ast::ToString(actual_ast.value());
  if (actual_dump != expected_dump) {
    *result_listener << "actual dump:\n" << actual_dump;
    return false;
  }

  return true;
}

TEST(ParserTest, VariableDeclaration) {
  ASSERT_THAT("local a = 1", LualikeParsesToAstDump(R"(Block
  VariableDeclaration: a
    LiteralExpression: Number <1>
)"));

  ASSERT_THAT("local a", LualikeParsesToAstDump(R"(Block
  VariableDeclaration: a
)"));
}

TEST(ParserTest, ReturnStatement) {
  ASSERT_THAT("return 1", LualikeParsesToAstDump(R"(Block
  ReturnStatement
    LiteralExpression: Number <1>
)"));

  ASSERT_THAT("return", LualikeParsesToAstDump(R"(Block
  ReturnStatement
)"));
}

TEST(ParserTest, IfStatement) {
  ASSERT_THAT("if true then return 1 else return 2 end",
              LualikeParsesToAstDump(R"(Block
  IfStatement
    Condition:
      LiteralExpression: True
    Then:
      Block
        ReturnStatement
          LiteralExpression: Number <1>
    Else:
      Block
        ReturnStatement
          LiteralExpression: Number <2>
)"));
}

TEST(ParserTest, ReadsFromInputStream) {
  std::istringstream input("return 1 + 2");
  const auto actual_ast = Parse(input);
  ASSERT_TRUE(actual_ast.has_value()) << RenderErrorForTest(actual_ast.error());
  EXPECT_EQ(lualike::ast::ToString(actual_ast.value()), R"(Block
  ReturnStatement
    BinaryExpression: +
      LiteralExpression: Number <1>
      LiteralExpression: Number <2>
)");
}

TEST(ParserTest, RejectsTrailingTokensAfterReturn) {
  const auto actual_ast = Parse(std::string_view{"return 1 whatever"});
  ASSERT_FALSE(actual_ast.has_value());
  EXPECT_THAT(actual_ast.error().what(),
              testing::HasSubstr("Unexpected token after return statement"));
}
