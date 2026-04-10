#include "lualike/parser.h"

#include <string>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "lualike/ast.h"

using lualike::parser::Parse;

MATCHER_P(LualikeParsesToAstDump, expected_dump, "") {
  const auto actual_ast = Parse(std::string_view{arg});
  if (!actual_ast.has_value()) {
    *result_listener << actual_ast.error().what();
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
