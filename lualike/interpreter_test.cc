#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <expected>
#include <generator>
#include <string_view>

import lualike.interpreter;
import lualike.value;

using lualike::value::LualikeValue;

namespace lualike::interpreter {

MATCHER_P(LualikeEvaluatesTo, compare_to, "") {
  const auto eval_result = EvaluateExpression(std::string_view{arg}, std_env);

  if (!eval_result) {
    return false;
  }

  return *eval_result == LualikeValue{compare_to};
}

MATCHER(LualikeInterpretsSuccessfully, "") {
  const auto result = Interpret(std::string_view{arg}, std_env);

  return result.has_value();
}

TEST(InterpreterTest, ExpressionEvaluationTest) {
  EXPECT_THAT("1", LualikeEvaluatesTo(1));
  EXPECT_THAT("3.14 * 2", LualikeEvaluatesTo(3.14 * 2));
  EXPECT_THAT("1 + 2 + 3", LualikeEvaluatesTo(6));
  EXPECT_THAT("2 + 2 * 2", LualikeEvaluatesTo(6));
  EXPECT_THAT("2 + 2 * 2 + -4.0", LualikeEvaluatesTo(2.0));
  EXPECT_THAT("(2 + 2) * 2 + -4.0", LualikeEvaluatesTo(4.0));
}

TEST(InterpreterTest, InterpretValidStatementsTest) {
  EXPECT_THAT("local var = 3.14", LualikeInterpretsSuccessfully());
  EXPECT_THAT("var = 3.14", LualikeInterpretsSuccessfully());
}

TEST(InterpreterTest, InterpretValidProgramsTest) {
  EXPECT_THAT(";;;", LualikeInterpretsSuccessfully());

  EXPECT_THAT(
      "local var = 3.14 "
      "var = 228 ",
      LualikeInterpretsSuccessfully());

  EXPECT_THAT("return 1", LualikeInterpretsSuccessfully());
}

}  // namespace lualike::interpreter
