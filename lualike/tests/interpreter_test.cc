#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string_view>

import lualike.interpreter;
import lualike.value;

namespace lualike::interpreter {

MATCHER_P(LualikeEvaluatesTo, expected_value, "") {
  const auto eval_result = interpreter::Interpret(std::string_view{arg});
  if (!eval_result.has_value()) {
    *result_listener << eval_result.error().what();
    return false;
  }
  if (!eval_result->has_value()) {
    *result_listener << "Interpreted successfully, but did not return a value";
    return false;
  }

  return ExplainMatchResult(
      testing::Eq(lualike::value::LualikeValue{expected_value}),
      eval_result->value(), result_listener);
}

MATCHER(LualikeRunsSuccessfully, "") {
  const auto eval_result = interpreter::Interpret(std::string_view{arg});
  if (!eval_result.has_value()) {
    *result_listener << eval_result.error().what();
    return false;
  }
  if (eval_result->has_value()) {
    *result_listener << "Expected no return value, but got one: "
                     << testing::PrintToString(eval_result->value());
    return false;
  }

  return true;
}

TEST(InterpreterTest, ExpressionEvaluationTest) {
  EXPECT_THAT("return 1", LualikeEvaluatesTo(1));
  EXPECT_THAT("return 3.14 * 2", LualikeEvaluatesTo(3.14 * 2));
  EXPECT_THAT("return 1 + 2 + 3", LualikeEvaluatesTo(6));
  EXPECT_THAT("return 2 + 2 * 2", LualikeEvaluatesTo(6));
  EXPECT_THAT("return 2 + 2 * 2 + -4.0", LualikeEvaluatesTo(2.0));
  EXPECT_THAT("return (2 + 2) * 2 + -4.0", LualikeEvaluatesTo(4.0));
}

TEST(InterpreterTest, InterpretValidStatementsTest) {
  EXPECT_THAT("local pi = 3.14", LualikeRunsSuccessfully());
  EXPECT_THAT("pi = 3.14", LualikeRunsSuccessfully());
  EXPECT_THAT(";;;", LualikeRunsSuccessfully());
  EXPECT_THAT("return;", LualikeRunsSuccessfully());
  EXPECT_THAT("return 1 + 2 + 3", LualikeEvaluatesTo(6));
}

TEST(InterpreterTest, InterpretValidProgramsTest) {
  EXPECT_THAT(
      "local pi_num = 3.14\n"
      "return pi_num",
      LualikeEvaluatesTo(3.14));
  EXPECT_THAT(
      "local pi_num = 3.14\n"
      "if false then pi_num = 1.81 end\n",
      LualikeRunsSuccessfully());
  EXPECT_THAT("if false then local pi_num = 3.14 else local pi_num = -1 end",
              LualikeRunsSuccessfully());

  EXPECT_THAT(
      "cond = true\n"
      "if cond then\n"
      "  return 3.14\n"
      "else\n"
      "  return -1\n"
      "end\n",
      LualikeEvaluatesTo(3.14));
}

}  // namespace lualike::interpreter
