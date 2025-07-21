#include <memory>
#include <string_view>

#include "gmock/gmock.h"
#include "gtest/gtest.h"

import lualike.ast;
import lualike.parser;
import lualike.value;

using lualike::ast::Block;
using lualike::ast::Expression;
using lualike::ast::IfStatement;
using lualike::ast::LiteralExpression;
using lualike::ast::Program;
using lualike::ast::ReturnStatement;
using lualike::ast::Statement;
using lualike::ast::VariableDeclaration;
using lualike::parser::Parse;
using lualike::value::LualikeValue; 

MATCHER_P(LualikeParsesTo, expected_ast, "") {
  const auto actual_ast = Parse(std::string_view{arg});
  if (!actual_ast.has_value()) {
    *result_listener << actual_ast.error().what();
    return false;
  }

  return ExplainMatchResult(testing::Eq(expected_ast), actual_ast.value(),
                            result_listener);
}

TEST(ParserTest, VariableDeclaration) {
  ASSERT_THAT("local a = 1",
              LualikeParsesTo(Program{{Statement{VariableDeclaration{
                  "a", Expression{LiteralExpression{LualikeValue{1}}}}}}}));

  ASSERT_THAT("local a",
              LualikeParsesTo(Program{{Statement{VariableDeclaration{"a"}}}}));
}

TEST(ParserTest, ReturnStatement) {
  ASSERT_THAT("return 1",
              LualikeParsesTo(Program{{Statement{ReturnStatement{
                  Expression{LiteralExpression{LualikeValue{1}}}}}}}));

  ASSERT_THAT("return",
              LualikeParsesTo(Program{{Statement{ReturnStatement{}}}}));
}

TEST(ParserTest, IfStatement) {
  ASSERT_THAT("if true then return 1 else return 2 end",
              LualikeParsesTo(Program{{Statement{IfStatement{
                  Expression{LiteralExpression{LualikeValue{true}}},
                  std::make_unique<Block>(Block{{Statement{ReturnStatement{
                      Expression{LiteralExpression{LualikeValue{1}}}}}}}),
                  std::make_unique<Block>(Block{{Statement{ReturnStatement{
                      Expression{LiteralExpression{LualikeValue{2}}}}}}})}}}}));
}
