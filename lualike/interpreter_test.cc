#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <format>
#include <string_view>

import lualike.interpreter;
import lualike.value;

namespace lualike::interpreter {

MATCHER_P(LualikeEvaluatesTo, compare_to, "") {
  using LLV = lualike::value::LualikeValue;

  const auto eval_result =
      Interpreter(std::string_view{arg}).EvaluateExpression();

  EXPECT_TRUE(eval_result.has_value());
  if constexpr (std::is_same_v<decltype(compare_to), LLV::FloatT>) {
    EXPECT_DOUBLE_EQ(std::get<LLV::FloatT>(eval_result.value().inner_value),
                     compare_to);
  } else {
    EXPECT_EQ(eval_result.value(), LLV{compare_to});
  }

  return true;
}

MATCHER(LualikeInterpretsSuccessfully, "") {
  const auto interpretation_result =
      Interpreter(std::string_view{arg}).Interpret();

  EXPECT_TRUE(interpretation_result.has_value()) << std::format(
      "After interpeting next program: \n{}\nGot an error: {}.", arg,
      static_cast<int>(interpretation_result.error().error_kind));

  return true;
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
  EXPECT_THAT("var = 3.14", LualikeInterpretsSuccessfully());
}

TEST(InterpreterTest, InterpretValidProgramsTest) {
  EXPECT_THAT(";;;", LualikeInterpretsSuccessfully());
  EXPECT_THAT("return 1", LualikeInterpretsSuccessfully());
  EXPECT_THAT(
      "local pi_num = 3.14\n"
      "return pi_num",
      LualikeInterpretsSuccessfully());
  EXPECT_THAT(
      "local pi_num = 3.14\n"
      "print(pi_num)\n"
      "return true",
      LualikeInterpretsSuccessfully());
}

}  // namespace lualike::interpreter
