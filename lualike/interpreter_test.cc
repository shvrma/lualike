#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string_view>

import lualike.interpreter;
import lualike.value;

namespace lualike::interpreter {

MATCHER_P(LualikeEvaluatesTo, compare_to, "") {
  using V = lualike::value::LualikeValue;

  const auto eval_result = interpreter::Interpret(std::string_view{arg});

  EXPECT_TRUE(eval_result.has_value());
  EXPECT_TRUE(eval_result->has_value());

  if constexpr (std::is_same_v<decltype(compare_to), V::FloatT>) {
    EXPECT_DOUBLE_EQ(std::get<V::FloatT>(eval_result->value().inner_value),
                     compare_to);
  } else {
    EXPECT_EQ(eval_result->value(), V{compare_to});
  }

  return true;
}

MATCHER(LualikeInterpretsSuccessfully, "") {
  const auto interpretation_result = Interpret(std::string_view{arg});

  EXPECT_TRUE(interpretation_result.has_value())
      << "After interpreting next program: \n"
      << arg << "\nGot an error: "
      << static_cast<int>(interpretation_result.error().error_kind);

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
  EXPECT_THAT("local pi = 3.14", LualikeInterpretsSuccessfully());
  EXPECT_THAT("pi = 3.14", LualikeInterpretsSuccessfully());
  EXPECT_THAT(";;;", LualikeInterpretsSuccessfully());
  EXPECT_THAT("return;", LualikeInterpretsSuccessfully());
  EXPECT_THAT("return 1 + 2 + 3", LualikeInterpretsSuccessfully());
}

TEST(InterpreterTest, InterpretValidProgramsTest) {
  // EXPECT_THAT(
  //     "local pi_num = 3.14\n"
  //     "return pi_num",
  //     LualikeInterpretsSuccessfully());
  // EXPECT_THAT(
  //     "local pi_num = 3.14\n"
  //     "if false then pi_num = 1.81 end\n",
  //     LualikeInterpretsSuccessfully());
  // EXPECT_THAT("if false then local pi_num = 3.14 else local pi_num = -1 end",
  //             LualikeInterpretsSuccessfully());

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
