#include <gtest/gtest.h>

#include <generator>
#include <string_view>

import lualike.interpreter;
import lualike.value;

namespace interpreter = lualike::interpreter;
using interpreter::Interpreter;

namespace value = lualike::value;
// using value::LualikeValue;
using value::operator""_lua_int;
using value::operator""_lua_float;
using value::operator""_lua_str;

using InterpreterTestParamT = std::pair<std::string_view, value::LualikeValue>;

class InterpreterTest : public testing::TestWithParam<InterpreterTestParamT> {};

TEST_P(InterpreterTest, ReadAndCompareWithGiven) {
  const auto &[input, expected_evaluation_result] = InterpreterTest::GetParam();

  auto interpreter = Interpreter(std::string_view{input});
  const auto actual_evaluation_result = interpreter.EvaluateExpression();

  ASSERT_EQ(actual_evaluation_result, expected_evaluation_result);
}

INSTANTIATE_TEST_SUITE_P(
    TestValidInputs, InterpreterTest,
    testing::ValuesIn(std::to_array<InterpreterTestParamT>({
        {"1", 1_lua_int},
        {"1 + 2", 3_lua_int},
        {"(1 + 2) + 3", 6_lua_int},
        {"2 * 2", 4_lua_int},
        {"(2 * 2)", 4_lua_int},
        {"(2 * 2) + 2", 6_lua_int},
        {"2 * 2 + 2", 6_lua_int},
        {"6 / 2", 3_lua_int},
    })));
