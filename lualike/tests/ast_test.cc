#include "lualike/ast.h"

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

namespace lualike::ast {

using value::LualikeValue;

TEST(AstTest, FormatsLiteralExpression) {
  Expression expr{LiteralExpression{LualikeValue{42}}};
  EXPECT_EQ(ToString(expr), "LiteralExpression: Number <42>\n");
}

TEST(AstTest, FormatsVariableExpression) {
  Expression expr{VariableExpression{"foo"}};
  EXPECT_EQ(ToString(expr), "VariableExpression: foo\n");
}

TEST(AstTest, FormatsUnaryExpression) {
  Expression expr{UnaryExpression{
      UnaryOperator::kNegate,
      std::make_unique<Expression>(LiteralExpression{LualikeValue{5}})}};
  EXPECT_EQ(ToString(expr),
            "UnaryExpression: -\n"
            "  LiteralExpression: Number <5>\n");

  Expression not_expr{UnaryExpression{
      UnaryOperator::kNot,
      std::make_unique<Expression>(VariableExpression{"flag"})}};
  EXPECT_EQ(ToString(not_expr),
            "UnaryExpression: not\n"
            "  VariableExpression: flag\n");
}

TEST(AstTest, FormatsBinaryExpression) {
  Expression expr{BinaryExpression{
      BinaryOperator::kAdd,
      std::make_unique<Expression>(VariableExpression{"a"}),
      std::make_unique<Expression>(LiteralExpression{LualikeValue{1}})}};
  EXPECT_EQ(ToString(expr),
            "BinaryExpression: +\n"
            "  VariableExpression: a\n"
            "  LiteralExpression: Number <1>\n");
}

TEST(AstTest, FormatsFunctionCallExpression) {
  std::vector<Expression> args;
  args.push_back(Expression{VariableExpression{"msg"}});
  
  Expression expr{FunctionCallExpression{
      std::make_unique<Expression>(VariableExpression{"print"}),
      std::move(args)}};

  EXPECT_EQ(ToString(expr),
            "FunctionCallExpression\n"
            "  Callee:\n"
            "    VariableExpression: print\n"
            "  Arguments:\n"
            "    VariableExpression: msg\n");
}

TEST(AstTest, FormatsVariableDeclaration) {
  Statement stmt_without_init{VariableDeclaration{"my_var", std::nullopt}};
  EXPECT_EQ(ToString(stmt_without_init),
            "VariableDeclaration: my_var\n");

  Statement stmt_with_init{VariableDeclaration{
      "count",
      Expression{LiteralExpression{LualikeValue{10}}}}};
  EXPECT_EQ(ToString(stmt_with_init),
            "VariableDeclaration: count\n"
            "  LiteralExpression: Number <10>\n");
}

TEST(AstTest, FormatsIfStatement) {
  auto then_b = std::make_unique<Block>();
  then_b->statements.push_back(Statement{ExpressionStatement{
      Expression{VariableExpression{"run_task"}}}});

  auto else_b = std::make_unique<Block>();
  else_b->statements.push_back(Statement{ReturnStatement{std::nullopt}});
  
  Statement if_stmt{IfStatement{
      Expression{VariableExpression{"cond"}},
      std::move(then_b),
      std::move(else_b)}};

  EXPECT_EQ(ToString(if_stmt),
            "IfStatement\n"
            "  Condition:\n"
            "    VariableExpression: cond\n"
            "  Then:\n"
            "    Block\n"
            "      ExpressionStatement\n"
            "        VariableExpression: run_task\n"
            "  Else:\n"
            "    Block\n"
            "      ReturnStatement\n");
}

TEST(AstTest, FormatsFunctionDeclaration) {
  auto body = std::make_unique<Block>();
  body->statements.push_back(Statement{ReturnStatement{
      Expression{VariableExpression{"x"}}}});
  
  Statement func_decl{FunctionDeclaration{
      "identity",
      {"x"},
      std::move(body)}};

  EXPECT_EQ(ToString(func_decl),
            "FunctionDeclaration: identity(x)\n"
            "  Block\n"
            "    ReturnStatement\n"
            "      VariableExpression: x\n");
}

}  // namespace lualike::ast
