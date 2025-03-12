#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <generator>
#include <string_view>

import lualike.interpreter;
import lualike.value;

using lualike::value::LualikeValue;

namespace lualike::interpreter {

MATCHER_P(LualikeEvaluatesTo, compare_to, "") {
  const auto eval_result = Interpreter::EvaluateExpression(arg);

  if constexpr (std::is_same_v<decltype(compare_to), LualikeValue::FloatT>) {
    // TODO(shvrma): floats comparison.
  }

  return eval_result == LualikeValue{compare_to};
}

TEST(InterpreterTest, ExpressionEvaluationTest) {
  using namespace std::literals;

  EXPECT_THAT("1"sv, LualikeEvaluatesTo(1));
  EXPECT_THAT("3.14 * 2"sv, LualikeEvaluatesTo(3.14 * 2));
  EXPECT_THAT("1 + 2 + 3"sv, LualikeEvaluatesTo(6));
  EXPECT_THAT("2 + 2 * 2"sv, LualikeEvaluatesTo(6));
  EXPECT_THAT("2 + 2 * 2 + -4.0"sv, LualikeEvaluatesTo(2.0));
  EXPECT_THAT("(2 + 2) * 2 + -4.0"sv, LualikeEvaluatesTo(4.0));
}

}  // namespace lualike::interpreter
