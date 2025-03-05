#include <gtest/gtest.h>

#include <generator>
#include <string_view>

import lualike.interpreter;
import lualike.value;

namespace value = lualike::value;
using value::operator""_lua_int;
using value::operator""_lua_float;
using value::operator""_lua_str;

namespace lualike::interpreter {

using InterpreterTestParamT = std::pair<std::string_view, value::LualikeValue>;

class InterpreterTest : public testing::TestWithParam<InterpreterTestParamT> {};

TEST_P(InterpreterTest, EvaluateAndCompareWithValid) {
  auto [input, expected_result] = InterpreterTest::GetParam();

  const auto actual_result = Interpreter::EvaluateExpression(input);

  ASSERT_EQ(actual_result, expected_result)
      << "Actual result: " << actual_result.ToString();
}

const InterpreterTestParamT kSingleDigit = {"1", 1_lua_int};

const InterpreterTestParamT kMultipleDigits = {"11111", 11111_lua_int};

const InterpreterTestParamT kFloat = {"3.14", 3.14_lua_float};

const InterpreterTestParamT kSimpleExpression = {"1 + 2 + 3", 6_lua_int};

const InterpreterTestParamT kHarderExpression = {"2 + 2 * 2", 6_lua_int};

INSTANTIATE_TEST_SUITE_P(TestValidInputs, InterpreterTest,
                         testing::Values(kSingleDigit, kMultipleDigits,
                                         kSimpleExpression, kHarderExpression));

}  // namespace lualike::interpreter
